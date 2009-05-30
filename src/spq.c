/* spq.c -- spq main program

   Copyright 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#ifdef M88000
#include <unistd.h>
#endif /* M88000 */
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <setjmp.h>
#include <curses.h>
#ifdef	HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_sig.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "keynames.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "xfershm.h"
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"

#ifdef	TI_GNU_CC_BUG
int	LINES;			/* Not defined anywhere without extern */
#endif

#ifdef	OS_BSDI
#define	_begy	begy
#define	_begx	begx
#define	_maxy	maxy
#define	_maxx	maxx
#endif

#ifndef	getbegyx
#define	getbegyx(win,y,x)	((y) = (win)->_begy, (x) = (win)->_begx)
#endif

#define	P_DONT_CARE	100

#define	BOXWID	1

void	cjfind(void);
void	jdisplay(void);
void	openjfile(void);
void	openpfile(void);
void	pdisplay(void);
void	rpfile(void);

int	p_process(void);
int	j_process(void);

char *	get_jobtitle(int);
char *	get_ptrtitle(void);

FILE	*Cfile;
char	Win_setup, jset;
static	jmp_buf	Mj;

#define	IPC_MODE	0600
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;

unsigned	Pollinit,	/* Initial polling */
		Pollfreq;	/* Current polling frequency */

time_t	hadalarm,
	lastalarm;

int	hadrfresh,
	wh_jtitline,
	wh_ptitline;

char	scrkeep,
	helpclr,
	helpbox,
	errbox,
	nopage,
	pfirst = P_DONT_CARE,
	confabort = 1;

DEF_DISPOPTS;

char	*Realuname;

struct	spdet	*mypriv;

struct	spr_req	jreq,
		preq,
		oreq;
struct	spq	JREQ;
struct	spptr	PREQ;

int	JLINES,
	PLINES,
	HJLINES,
	HPLINES,
	TPLINES;

WINDOW	*hjscr,
	*hpscr,
	*tpscr,
	*jscr,
	*pscr,
	*hlpscr,
	*escr;

char	*ptdir,
	*spdir,
	*Curr_pwd;

#ifdef	HAVE_TERMIOS_H
struct	termios	orig_term;
#else
struct	termio	orig_term;
#endif

/* If we get a message error die appropriately */

void	msg_error(const int ret)
{
	if  (Win_setup)  {
		int	save_errno = errno; /* In cases endwin clobbers it */
		clear();
		refresh();
		endwinkeys();
		errno = save_errno;
		Win_setup = 0;
	}
	print_error(ret);
	Ctrl_chan = -1;
	exit(E_SETUP);
}

/* For when we run out of memory.....  */

void	nomem(void)
{
#ifndef	HAVE_ATEXIT
	void	exit_cleanup(void);
#endif
	if  (Win_setup)  {
		clear();
		refresh();
		endwinkeys();
		Win_setup = 0;
	}
	fprintf(stderr, "Ran out of memory\n");
#ifndef	HAVE_ATEXIT
	exit_cleanup();
#endif
	exit(E_NOMEM);
}

/* Write messages to scheduler.  */

void	womsg(const int act)
{
	oreq.spr_un.o.spr_act = (USHORT) act;
	if  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)
	     msg_error(errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error});
}

/* Exit cleanup function - turn off curses and log off spshed */

void	exit_cleanup(void)
{
	if  (Win_setup)  {
		clear();
		refresh();
		endwinkeys();
	}
	if  (Ctrl_chan >= 0)
		womsg(SO_DMON);
}

void	my_wjmsg(const int act)
{
	int	ret;
	jreq.spr_un.j.spr_act = (USHORT) act;
	if  ((ret = wjmsg(&jreq, &JREQ)))
		msg_error(ret);
}

void	my_wpmsg(const int act)
{
	int	ret;
	preq.spr_un.p.spr_act = (USHORT) act;
	if  ((ret = wpmsg(&preq, &PREQ)))
		msg_error(ret);
}

/* This notes signals from (presumably) the scheduler.  */

RETSIGTYPE	markit(int sig)
{
#ifdef	UNSAFE_SIGNALS		/* QUICK!!!! */
	signal(sig, markit);
#endif
	if  (sig != QRFRESH)  {
#ifndef	HAVE_ATEXIT
		exit_cleanup();
#endif
		exit(E_SIGNAL);
	}
	hadrfresh++;
	if  (jset)  {
#ifdef	HAVE_SIGACTION
#ifndef	SA_NODEFER
		sigset_t	nset;
		sigemptyset(&nset);
		sigaddset(&nset, sig);
		sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *) 0);
#endif
#elif	defined(STRUCT_SIG)
		sigsetmask(sigsetmask(~0) & ~sigmask(sig));
#elif	defined(HAVE_SIGSET)
		sigrelse(sig);
#endif
		longjmp(Mj, 1);
	}
}

/* This deals with alarm calls whilst polling.  */

RETSIGTYPE	pollit(int n)
{
	static	ULONG	lastserial;

#ifdef	UNSAFE_SIGNALS
	signal(SIGALRM, pollit);
#endif
	time(&lastalarm);

	/* Reset alarm only if there are processes around which might touch the job queue.  */

	if  (Ptr_seg.npprocesses == 0)
		return;

	/* If the job queue has not changed, then halve the frequency
	   of polling.  If it has changed, double it.  */

	if  (lastserial == Job_seg.dptr->js_serial)  {
		Pollfreq <<= 1;
		if  (Pollfreq > POLLMAX)
			Pollfreq = POLLMAX;
	}
	else  {
		Pollfreq >>= 1;
		if  (Pollfreq < POLLMIN)
			Pollfreq = POLLMIN;
	}
	lastserial = Job_seg.dptr->js_serial;
	alarm(Pollfreq);
}

/* Other signals are errors Suppress final message....  */

RETSIGTYPE	sigerr(int n)
{
	Ctrl_chan = -1;
#ifndef	HAVE_ATEXIT
	exit_cleanup();
#endif
	exit(E_SIGNAL);
}

/* Generate help message.  */

void	dohelp(WINDOW * owin, struct sctrl * scp, const char * prefix)
{
	char	**hv, **examples = (char **) 0;
	int	hrows, hcols, erows, ecols, cols, rows;
	int	begy, cy, cx, startrow, startcol, k, edrows, edcols;
	int	rpadding, icpadding;
	int	i, j;

	if  (*(hv = helpvec(scp->helpcode, 'H')) == (char *) 0)  {
		free((char *) hv);
		disp_arg[0] = scp->helpcode;
		hv = helpvec($E{Missing help code}, 'E');
	}

	if  (scp->helpfn)
		examples = (*scp->helpfn)(prefix, 1);

	count_hv(hv, &hrows, &hcols);
	count_hv(examples, &erows, &ecols);
	cols = hcols;
	edcols = 1;
	edrows = erows;
	rpadding = 0;
	icpadding = 1;

	if  (ecols > cols)
		cols = ecols;
	else  if  ((edcols = hcols / (ecols + 1)) <= 0)
			edcols = 1;
	else  {
		if  (edcols > erows)
			edcols = erows;
		if  ((i = edcols - 1) > 0)  {
			edrows = (erows - 1) / edcols + 1;
			rpadding = cols - edcols * ecols;
			icpadding = rpadding / i;
			if  (icpadding > 5)
				icpadding = 5;
			rpadding = (cols - ecols * edcols - icpadding * i) / 2;
		}
	}

	rows = hrows + edrows;
	if  (helpbox)  {
		rows += 2 * BOXWID;
		cols += 2 * BOXWID;
	}

	if  (rows >= LINES)  {
		edrows -= rows - LINES + 1;
		rows = LINES-1;
	}

	/* Find absolute cursor position and try to create window avoiding it.  */

	getbegyx(owin, begy, cx);
	getyx(owin, cy, cx);
	cy += begy;
	if  ((startrow = cy - rows/2) < 0)
		startrow = 0;
	else  if  (startrow + rows > LINES)
		startrow = LINES - rows;
	if  ((startcol = cx - cols/2) < 0)
		startcol = 0;
	else  if  (startcol + cols > COLS)
		startcol = COLS - cols;

	if  (cx + cols + 2 < COLS)
		startcol = COLS - cols - 1;
	else  if  (cx - cols - 1 >= 0)
		startcol = cx - cols - 1;
	else  if  (cy + rows + 2 < LINES)
		startrow = cy + 2;
	else  if  (cy - rows - 1 >= 0)
		startrow = cy - rows - 1;

	if  ((hlpscr = newwin(rows <= 0? 1: rows, cols, startrow, startcol)) == (WINDOW *) 0)
		nomem();

	if  (helpbox)  {
#ifdef	HAVE_TERMINFO
		box(hlpscr, 0, 0);
#else
		box(hlpscr, '|', '-');
#endif
		for  (i = 0;  i < hrows;  i++)
			mvwaddstr(hlpscr, i+BOXWID, BOXWID, hv[i]);

		for  (i = 0;  i < edrows;  i++)  {
			int	ccol = rpadding + BOXWID;
			wmove(hlpscr, i + hrows + BOXWID, ccol);
			for  (j = 0;  j < edcols - 1;  j++)  {
				if  ((k = i + j*edrows) < erows)
					waddstr(hlpscr, examples[k]);
				ccol += ecols + icpadding;
				wmove(hlpscr, i + hrows + BOXWID, ccol);
			}
			if  ((k = i + (edcols - 1) * edrows) < erows)
				waddstr(hlpscr, examples[k]);
		}
	}
	else  {
		wstandout(hlpscr);

		for  (i = 0;  i < hrows;  i++)  {
			mvwaddstr(hlpscr, i, 0, hv[i]);
			for  (cx = strlen(hv[i]);  cx < cols;  cx++)
				waddch(hlpscr, ' ');
		}

		for  (i = 0;  i < edrows;  i++)  {
			wmove(hlpscr, i+hrows, 0);
			for  (cx = 0;  cx < rpadding;  cx++)
				waddch(hlpscr, ' ');
			for  (j = 0;;  j++)  {
				if  ((k = i + j*edrows) < erows)  {
					waddstr(hlpscr, examples[k]);
					k = strlen(examples[k]);
				}
				else
					k = 0;
				for  (;  k < ecols;  k++)
					waddch(hlpscr, ' ');
				if  (j >= edcols - 1)  {
					for  (k = (edcols - 1) * (ecols + icpadding) + ecols;  k < cols;  k++)
						waddch(hlpscr, ' ');
					break;
				}
				for  (k = 0;  k < icpadding;  k++)
					waddch(hlpscr, ' ');
			}
		}
	}
	freehelp(hv);
	freehelp(examples);

#ifdef HAVE_TERMINFO
	wnoutrefresh(hlpscr);
	wnoutrefresh(owin);
	doupdate();
#else
	wrefresh(hlpscr);
	wrefresh(owin);
#endif
}

/* Bodge the above for when we don't have a specific thing to do other
   than display a message code.  */

void	dochelp(WINDOW *wp, const int code)
{
	struct	sctrl	xx;
	xx.helpcode = code;
	xx.helpfn = (char **(*)()) 0;

	dohelp(wp, &xx, (char *) 0);
}

/* Get rid of a help or error message */

void	endhe(WINDOW *owin, WINDOW **wpp)
{
	delwin(*wpp);
	*wpp = (WINDOW *) 0;
	if  (owin == stdscr)  {
		touchwin(stdscr);
		refresh();
		return;
	}

	if  (HJLINES > 0)
		touchwin(hjscr);
	if  (TPLINES > 0)
		touchwin(tpscr);
	touchwin(jscr);

	if  (PLINES > 0)  {
		if  (HPLINES > 0)
			touchwin(hpscr);
		touchwin(pscr);
#ifdef HAVE_TERMINFO
		if  (HPLINES > 0)
			wnoutrefresh(hpscr);
		wnoutrefresh(pscr);
#else
		if  (owin != pscr)  {
			if  (HPLINES > 0)
				wrefresh(hpscr);
			wrefresh(pscr);
		}
#endif
	}
#ifdef HAVE_TERMINFO
	if  (HJLINES > 0)
		wnoutrefresh(hjscr);
	if  (TPLINES > 0)
		wnoutrefresh(tpscr);
	wnoutrefresh(jscr);
#else
	if  (HJLINES > 0)
		wrefresh(hjscr);
	if  (TPLINES > 0)
		wrefresh(tpscr);
	wrefresh(jscr);
#endif
	if  (PLINES > 0  &&  owin == pscr)  {
#ifdef	HAVE_TERMINFO
		if  (HPLINES > 0)
			wnoutrefresh(hpscr);
		wnoutrefresh(pscr);
#else
		if  (HPLINES > 0)
			wrefresh(hpscr);
		wrefresh(pscr);
#endif
	}
	if  (owin != jscr  &&  owin != pscr)  {
		touchwin(owin);
#ifdef	HAVE_TERMINFO
		wnoutrefresh(owin);
#else
		wrefresh(owin);
#endif
	}
#ifdef	HAVE_TERMINFO
	doupdate();
#endif
}

/* Generate error message avoiding (if possible) current cursor position.  */

void	doerror(WINDOW *wp, const int Errnum)
{
	char	**ev;
	int	erows, ecols, rows, cols;
	int	begy, cy, startrow, startcol, i, l;

#ifdef	HAVE_TERMINFO
	flash();
#else
	putchar('\007');
#endif

	if  (*(ev = helpvec(Errnum, 'E')) == (char *) 0)  {
		free((char *) ev);
		disp_arg[0] = Errnum;
		ev = helpvec($E{Missing error code}, 'E');
	}

	count_hv(ev, &erows, &ecols);
	rows = erows;
	cols = ecols;

	if  (errbox)  {
		rows += BOXWID * 2;
		cols += BOXWID * 2;
	}

	/* Silly person might make error messages too big.  */

	if  (cols > COLS)  {
		ecols -= cols - COLS;
		cols = COLS;
	}

	/* Find absolute cursor position and try to create window avoiding it.  */

	getbegyx(wp, begy, i);
	getyx(wp, cy, i);
	cy += begy;
	if  (cy >= LINES/2)
		startrow = 0;
	else
		startrow = LINES - rows;
	startcol = (COLS - cols) / 2;

	if  ((escr = newwin(rows <= 0? 1: rows, cols, startrow, startcol)) == (WINDOW *) 0)
		nomem();

	if  (errbox)  {
#ifdef	HAVE_TERMINFO
		box(escr, 0, 0);
#else
		box(escr, '|', '-');
#endif
		for  (i = 0;  i < erows;  i++)
			mvwaddstr(escr, i+BOXWID, BOXWID, ev[i]);
	}
	else  {
		wstandout(escr);

		for  (i = 0;  i < erows;  i++)  {
			mvwaddstr(escr, i, 0, ev[i]);
			for  (l = strlen(ev[i]);  l < ecols;  l++)
				waddch(escr, ' ');
		}
	}
	freehelp(ev);

#ifdef HAVE_TERMINFO
	wnoutrefresh(escr);
	wnoutrefresh(wp);
	doupdate();
#else
	wrefresh(escr);
	wrefresh(wp);
#endif
}

/* This accepts input from the screen.  */

void	process(int inpq)
{
	int	res;

#ifdef	STRUCT_SIG
	struct	sigstruct_name	z;
	z.sighandler_el = markit;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG | SIGACT_INTSELF;
	sigact_routine(QRFRESH, &z, (struct sigstruct_name *) 0);
	z.sighandler_el = pollit;
	sigact_routine(SIGALRM, &z, (struct sigstruct_name *) 0);
#else
	/* signal is #defined as sigset on suitable systems */
	signal(QRFRESH, markit);
	signal(SIGALRM, pollit);
#endif
	oreq.spr_un.o.spr_arg1 = Realuid;
	womsg(SO_MON);

	if  (!setjmp(Mj))
		jset = 1;

	while  (!hadrfresh)
#if	(defined(HAVE_SIGVEC) && defined(SV_INTERRUPT)) || defined(HAVE_SIGVECTOR)
		sigpause(0);
#else
		pause();
#endif
	jset = 0;

	for  (;;)  {
		hadrfresh = 0;
		rpfile();
		readjoblist(1);
		if  (pscr)
			pdisplay();
		cjfind();
		jdisplay();
		for  (;;)  {
			if  (inpq == P_DONT_CARE)  {
				if  (Job_seg.njobs != 0)
					inpq = 0;
				else
					inpq = 1;
			}
			if  (inpq)
				res = p_process();
			else
				res = j_process();

			/* Res = 0, quit, -1 refresh, 1 other screen */

			if  (res < 0)
				break;

			if  (res == 0)  {
#ifndef	HAVE_ATEXIT
				exit_cleanup();
#endif
				exit(0);
			}
			inpq = !inpq;
		}
	}
}

/* Set up terminal suitably.  */

void	wstart(void)
{
	int	i;
	char	**hvi, **hj, **tp, *jtitle, *ptitle;
#ifdef TOWER
	struct	termio	aswas, asis;
#endif
#ifdef	STRUCT_SIG
	struct	sigstruct_name  ze;
	ze.sighandler_el = sigerr;
	sigmask_clear(ze);
	ze.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGINT, &ze, (struct sigstruct_name *) 0);
	sigact_routine(SIGQUIT, &ze, (struct sigstruct_name *) 0);
	sigact_routine(SIGHUP, &ze, (struct sigstruct_name *) 0);
	sigact_routine(SIGTERM, &ze, (struct sigstruct_name *) 0);
#ifndef	DEBUG
	sigact_routine(SIGBUS, &ze, (struct sigstruct_name *) 0);
	sigact_routine(SIGSEGV, &ze, (struct sigstruct_name *) 0);
	sigact_routine(SIGILL, &ze, (struct sigstruct_name *) 0);
	sigact_routine(SIGFPE, &ze, (struct sigstruct_name *) 0);
#endif
#else  /* Not STRUCT_SIG */
	signal(SIGINT, sigerr);
	signal(SIGQUIT, sigerr);
	signal(SIGHUP, sigerr);
	signal(SIGTERM, sigerr);
#ifndef	DEBUG
	signal(SIGBUS, sigerr);
	signal(SIGSEGV, sigerr);
	signal(SIGILL, sigerr);
	signal(SIGFPE, sigerr);
#endif
#endif /* Signal type */
#ifdef TOWER
	ioctl(0, TCGETA, &aswas);
#endif
#ifdef M88000
	/* Some versions of curses invoke this even if we don't */
	if  (sysconf(_SC_JOB_CONTROL) < 0)
		fprintf(stderr, "WARNING: No job control\n");
#endif
	initscr();
	raw();
	nonl();
	noecho();

#ifdef	TOWER
	/* Restore the port's hardware to what it was before curses did its dirty deed.  */

	ioctl(0, TCGETA, &asis);
	asis.c_cflag = aswas.c_cflag;
	ioctl(0, TCSETA, &asis);
#endif
	Win_setup = 1;

	jtitle = get_jobtitle(nopage);
	ptitle = get_ptrtitle();
	wh_jtitline = wh_ptitline = -1;

	/* Set up windows.  NB we allow for no headers if we want.  */

	hj = helphdr('J');
	count_hv(hj, &HJLINES, &i);
	JLINES = LINES - HJLINES;
	tp = helphdr('T');
	count_hv(tp, &TPLINES, &i);
	JLINES -= TPLINES;

	if  (PLINES > 0)  {
		char	**hp = helphdr('D');

		count_hv(hp, &HPLINES, &i);
		JLINES -= HPLINES + PLINES;

		if  (JLINES <= 0)
			nomem();

		if  (HPLINES > 0)  {
			if  ((hpscr = newwin(HPLINES, 0, JLINES+HJLINES, 0)) == (WINDOW *) 0)
				nomem();

			for  (i = 0, hvi = hp;  *hvi;  i++, hvi++)  {
				if  (hvi[0][0] == 'p'  &&  hvi[0][1] == '\0')
					mvwaddstr(hpscr, wh_ptitline = i, 0, ptitle);
				else
					mvwhdrstr(hpscr, i, 0, *hvi);
				free(*hvi);
			}
			free((char *) hp);
			wrefresh(hpscr);
		}

		if  ((pscr = newwin(PLINES, 0, JLINES+HJLINES+HPLINES, 0)) == (WINDOW *) 0)
			nomem();
	}
	if  (TPLINES > 0)  {
		if  ((tpscr = newwin(TPLINES, 0, JLINES+PLINES+HJLINES+HPLINES, 0)) == (WINDOW *) 0)
			nomem();
		for  (i = 0, hvi = tp;  *hvi;  i++, hvi++)  {
			mvwhdrstr(tpscr, i, 0, *hvi);
			free(*hvi);
		}
		free((char *) tp);
		wrefresh(tpscr);
	}

	if  (HJLINES > 0)  {
		if  ((hjscr = newwin(HJLINES, 0, 0, 0)) == (WINDOW *) 0)
			nomem();
		for  (i = 0, hvi = hj;  *hvi;  i++, hvi++)  {
			if  (hvi[0][0] == 'j'  &&  hvi[0][1] == '\0')
				mvwaddstr(hjscr, wh_jtitline = i, 0, jtitle);
			else
				mvwhdrstr(hjscr, i, 0, *hvi);
			free(*hvi);
		}
		free((char *) hj);
		wrefresh(hjscr);
	}

	free(jtitle);
	free(ptitle);

	jscr = newwin(JLINES, 0, HJLINES, 0);
}

OPTION(o_explain)
{
	print_error($E{spq options});
	exit(0);
}

#include "inline/o_boxes.c"
#include "inline/o_dloco.c"
#include "inline/o_dpage.c"
#include "inline/o_justq.c"
#include "inline/o_justu.c"
#include "inline/o_allj.c"
#include "inline/o_classc.c"
#include "inline/o_jinclall.c"
#include "inline/o_psort.c"

OPTION(o_scrkeep)
{
	scrkeep = 1;
	return  OPTRESULT_OK;
}

OPTION(o_noscrkeep)
{
	scrkeep = 0;
	return  OPTRESULT_OK;
}

OPTION(o_dontcare)
{
	pfirst = P_DONT_CARE;
	return  OPTRESULT_OK;
}

OPTION(o_jfirst)
{
	pfirst = 0;
	return  OPTRESULT_OK;
}

OPTION(o_pfirst)
{
	pfirst = 1;
	return  OPTRESULT_OK;
}

OPTION(o_confabort)
{
	confabort++;
	return  OPTRESULT_OK;
}

OPTION(o_noconfabort)
{
	confabort = 0;
	return  OPTRESULT_OK;
}

OPTION(o_nump)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;

	num = atoi(arg);
	if  (num > MAX_PLINES)  {
		disp_arg[0] = num;
		disp_arg[1] = MAX_PLINES;
		print_error($E{Too many printers displayed});
		exit(E_USAGE);
	}
	PLINES = num;
	return  OPTRESULT_ARG_OK;
}

OPTION(o_refreshtime)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;

	num = atoi(arg);
	if  (num < POLLMIN || num > POLLMAX)  {
		disp_str = arg;
		disp_arg[0] = POLLMIN;
		disp_arg[1] = POLLMAX;
		print_error($E{Poll parameter out of range});
		exit(E_USAGE);
	}
	Pollinit = Pollfreq = num;

	return  OPTRESULT_ARG_OK;
}

/* Defaults and proc table for arg interp.  */

static	const	Argdefault	Adefs[] = {
  {  '?', $A{spq explain}	},
  {  'h', $A{spq keepchar}	},
  {  'H', $A{spq losechar}	},
  {  's', $A{spq cursor follow}	},
  {  'n', $A{spq cursor keep}	},
  {  'l', $A{spq local}		},
  {  'r', $A{spq remotes}	},
  {  'd', $A{spq dont care}	},
  {  'j', $A{spq jobs screen}	},
  {  'p', $A{spq ptr screen}	},
  {  'e', $A{spq no page counts}},
  {  'E', $A{spq page counts}	},
  {  'a', $A{spq confirm abort}	},
  {  'A', $A{spq no confirm abort}},
  {  'q', $A{spq only queue}	},
  {  'P', $A{spq number printers}},
  {  'C', $A{spq classcode}	},
  {  'R', $A{spq refreshtime}	},
  {  'b', $A{spq help box}	},
  {  'B', $A{spq no help box}	},
  {  'm', $A{spq error box}	},
  {  'M', $A{spq no error box}	},
  {  'y', $A{spq unprinted jobs}},
  {  'Y', $A{spq all jobs}	},
  {  'u', $A{spq just user}	},
  {  't', $A{spq just title}	},
  {  'D', $A{spq printed jobs}	},
  {  'z', $A{spq include null}	},
  {  'Z', $A{spq no include null}},
  {  'I', $A{spq include all}	},
  {  'U', $A{spq ptrs unsorted}	},
  {  'S', $A{spq ptrs sorted}	},
  {  0, 0 }
};

optparam   optprocs[] = {
o_explain,	o_nohelpclr,	o_helpclr,	o_noscrkeep,
o_scrkeep,	o_localonly,	o_nolocalonly,	o_dontcare,
o_jfirst,	o_pfirst,	o_nopage,	o_page,
o_confabort,	o_noconfabort,	o_justq,	o_nump,
o_classcode,	o_refreshtime,	o_helpbox,	o_nohelpbox,
o_errbox,	o_noerrbox,	o_justnp,	o_allj,
o_justu,	o_justt,	o_justp,	o_jinclnull,
o_jinclnonull,	o_jinclall,	o_punsorted,	o_psorted
};

char	Cvarname[] = "SPQCONF";

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	int	ret;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	/* If we haven't got a directory, use the current */

	if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
		Curr_pwd = runpwd();

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(Cvarname, "spq.help");
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	PLINES = DEFAULT_PLINES;
	Pollinit = Pollfreq = DEFAULT_REFRESH;
	mypriv = getspuser(Realuid);
	if  ((mypriv->spu_flgs & (PV_OTHERJ|PV_VOTHERJ)) != (PV_OTHERJ|PV_VOTHERJ))
		Realuname = prin_uname(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	argv = optprocess(argv, Adefs, optprocs, $A{spq explain}, $A{spq ptrs sorted}, 1);
	SWAP_TO(Daemuid);

	if  (PLINES == 0  || !(mypriv->spu_flgs & PV_PRINQ))
		pfirst = 0;

	/* Change directory.  */

	spdir = envprocess(SPDIR);

	if  (chdir(spdir) < 0)  {
		print_error($E{Cannot chdir});
		exit(E_NOCHDIR);
	}
	ptdir = envprocess(PTDIR);

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		print_error($E{Spooler not running});
		exit(E_NOTRUN);
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif

	if  ((ret = init_xfershm(1)))  {
		print_error(ret);
		exit(E_SETUP);
	}

#ifdef	HAVE_ATEXIT
	atexit(exit_cleanup);
#endif

#ifdef	HAVE_TERMIOS_H
	tcgetattr(0, &orig_term);
#else
	ioctl(0, TCGETA, &orig_term);
#endif

#ifdef	OS_ARIX
	/* Arix curses breaks if you start it up after attaching shms */
	setupkeys();
	wstart();
#endif

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	openjfile();
	openpfile();
	oreq.spr_mtype = jreq.spr_mtype = preq.spr_mtype = MT_SCHED;
	oreq.spr_un.o.spr_pid = preq.spr_un.p.spr_pid = jreq.spr_un.j.spr_pid = getpid();

#ifndef	OS_ARIX		/*  See above */
	setupkeys();

	/* Initialise windows on terminal.  */

	wstart();
#endif
	process(pfirst);
	return  0;		/* Not really reached */
}
