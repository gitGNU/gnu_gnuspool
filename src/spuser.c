/* spuser.c -- set user permissions

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
#include "incl_sig.h"
#include <pwd.h>
#include <sys/stat.h>
#include <curses.h>
#include <ctype.h>
#ifdef	HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "defaults.h"
#include "spuser.h"
#include "keynames.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "ecodes.h"
#include "files.h"
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"

#ifdef	OS_BSDI
#define	_begy	begy
#define	_begx	begx
#define	_maxy	maxy
#define	_maxx	maxx
#endif

#ifndef	getmaxyx
#define	getmaxyx(win,y,x)	((y) = (win)->_maxy, (x) = (win)->_maxx)
#endif
#ifndef	getbegyx
#define	getbegyx(win,y,x)	((y) = (win)->_begy, (x) = (win)->_begx)
#endif

#ifdef	TI_GNU_CC_BUG
int	LINES;			/* Not defined anywhere without extern */
#endif

#define	BOXWID	1

extern	struct	sphdr	Spuhdr;
struct	spdet	*ulist;

#define	SORT_NONE	0
#define	SORT_USER	1

char	iflag,
	cflag,
	alphsort = SORT_NONE;

extern	char	helpclr, helpbox, errbox;	/* Def moved to library wgets */
extern	void	dohelp(WINDOW *, struct sctrl *, const char *);
extern	void	doerror(WINDOW *, const int);

char	*Curr_pwd;

int	Current_user,
	Top_user,
	hchanges,
	more_above,
	more_below;

#define	ELINES	4

WINDOW	*hscr,
	*dscr,
	*tpscr;

int	DLINES,
	HLINES,
	TPLINES,
	DEFLINE;

struct	perm	{
	int	number, nextstate;
	char	*string;
	ULONG	flg, sflg, rflg;
}  ptab[] = {
	{ $PN{Priv descr edit admin file}, -1, (char *) 0,	PV_ADMIN, ALLPRIVS, ~PV_ADMIN },
	{ $PN{Priv descr override class}, -1, (char *) 0,	PV_COVER, PV_COVER, ~PV_COVER },
	{ $PN{Priv descr stop spooler}, -1, (char *) 0,	PV_SSTOP, PV_SSTOP, ~PV_SSTOP },
	{ $PN{Priv descr use other forms}, -1, (char *) 0,	PV_FORMS, PV_FORMS, ~PV_FORMS },
	{ $PN{Priv descr use other ptrs}, -1, (char *) 0,	PV_OTHERP, PV_OTHERP, ~PV_OTHERP },
	{ $PN{Priv descr change priority on Q}, -1, (char *) 0, PV_CPRIO, PV_CPRIO, ~(PV_CPRIO|PV_ANYPRIO) },
	{ $PN{Priv descr edit other users jobs}, -1, (char *) 0,PV_OTHERJ, PV_OTHERJ|PV_VOTHERJ, ~PV_OTHERJ },
	{ $PN{Priv descr select printer list}, -1, (char *) 0,	PV_PRINQ, PV_PRINQ, ~(PV_PRINQ|PV_HALTGO|PV_ADDDEL) },
	{ $PN{Priv descr halt printers}, -1, (char *) 0,	PV_HALTGO, PV_PRINQ|PV_HALTGO, ~(PV_HALTGO|PV_ADDDEL) },
	{ $PN{Priv descr add printers}, -1, (char *) 0,	PV_ADDDEL, PV_PRINQ|PV_HALTGO|PV_ADDDEL, ~PV_ADDDEL },
	{ $PN{Priv descr any priority Q}, -1, (char *) 0,	PV_ANYPRIO, PV_ANYPRIO|PV_CPRIO, ~PV_ANYPRIO },
	{ $PN{Priv descr own defaults}, -1, (char *) 0,		PV_CDEFLT, PV_CDEFLT, ~PV_CDEFLT },
	{ $PN{Priv descr unqueue jobs}, -1, (char *) 0,		PV_UNQUEUE, PV_UNQUEUE, ~PV_UNQUEUE },
	{ $PN{Priv descr view other jobs}, -1, (char *) 0,	PV_VOTHERJ, PV_VOTHERJ, ~(PV_OTHERJ|PV_VOTHERJ) },
	{ $PN{Priv descr remote jobs}, -1, (char *) 0,		PV_REMOTEJ, PV_REMOTEJ, ~PV_REMOTEJ },
	{ $PN{Priv descr remote printers}, -1, (char *) 0,	PV_REMOTEP, PV_REMOTEP, ~PV_REMOTEP },
	{ $PN{Priv descr access queue}, -1, (char *) 0,		PV_ACCESSOK, PV_ACCESSOK, ~(PV_ACCESSOK|PV_FREEZEOK) },
	{ $PN{Priv descr freeze options}, -1, (char *) 0,	PV_FREEZEOK, PV_ACCESSOK|PV_FREEZEOK, ~PV_FREEZEOK }
};

/* This flag tells us whether we have called the routine to turn the
   error codes into messages or not */

static	char	code_expanded = 0,
		Win_setup;

static	char	*s_class,
		*ns_class,
		*lt_class,
		*gt_class,
		*s_perm,
		*ns_perm,
		*lt_perm,
		*gt_perm,
		*more_amsg,
		*more_bmsg,
		*deflt_str;

#define	MAXPERM	(sizeof (ptab)/sizeof(struct perm))

struct	perm	*plist[MAXPERM+1];

#define	USNAM_COL	0
#define	DEFPRI_COL	8
#define	MINPRI_COL	12
#define	MAXPRI_COL	16
#define	COPIES_COL	20
#define	DEFFORM_COL	24
#define	DEFPTR_COL	42
#define	CLASS_COL	58
#define	PRIV_COL	70

/* Tables for use by wnum */

#define	NULLCH		((char *) 0)

static	struct	sctrl
  wnt_udp = { $H{spuser def pri help},		HELPLESS, 3, 0, DEFPRI_COL, MAG_P, 1L, 255L, NULLCH },
  wnt_sdp = { $H{spuser sys def pri help},	HELPLESS, 3, 0, DEFPRI_COL, MAG_P, 1L, 255L, NULLCH },
  wnt_ulp = { $H{spuser min pri help},		HELPLESS, 3, 0, MINPRI_COL, MAG_P, 1L, 255L, NULLCH },
  wnt_slp = { $H{spuser sys min pri help},	HELPLESS, 3, 0, MINPRI_COL, MAG_P, 1L, 255L, NULLCH },
  wnt_uhp = { $H{spuser max pri help},		HELPLESS, 3, 0, MAXPRI_COL, MAG_P, 1L, 255L, NULLCH },
  wnt_shp = { $H{spuser sys max pri help},	HELPLESS, 3, 0, MAXPRI_COL, MAG_P, 1L, 255L, NULLCH },
  wnt_ucp = { $H{spuser max copies help},	HELPLESS, 3, 0, COPIES_COL, MAG_P, 0L, 255L, NULLCH },
  wnt_scp = { $H{spuser sys max copies help},	HELPLESS, 3, 0, COPIES_COL, MAG_P, 0L, 255L, NULLCH };

/* Ditto for wgets */

extern  int  propts();

extern	char **uhelpform(const char *, const int);
extern	char **syshelpform(const char *, const int);

static	struct	sctrl
  wst_ufm = { $H{spuser user form type help},
		      uhelpform, DEFPTR_COL-DEFFORM_COL, 0, DEFFORM_COL, MAG_P|MAG_LONG|MAG_NL|MAG_FNL, 0L, 0L, NULLCH },
  wst_sfm = { $H{spuser sys form type help},
		      syshelpform, DEFPTR_COL-DEFFORM_COL, 0, DEFFORM_COL, MAG_P|MAG_LONG|MAG_NL|MAG_FNL, 0L, 0L, NULLCH },
  wst_upt = { $H{spuser user ptr help},
		      HELPLESS, CLASS_COL-DEFPTR_COL, 0, DEFPTR_COL, MAG_P|MAG_LONG, 0L, 0L, NULLCH },
  wst_spt = { $H{spuser sys ptr help},
		      HELPLESS, CLASS_COL-DEFPTR_COL, 0, DEFPTR_COL, MAG_P|MAG_LONG, 0L, 0L, NULLCH };

#ifdef	HAVE_TERMIOS_H
struct	termios	orig_term;
#else
struct	termio	orig_term;
#endif

void  exit_cleanup()
{
	if  (Win_setup)  {
		clear();
		refresh();
		endwinkeys();
	}
}

/* For when we run out of memory.....  */

void	nomem()
{
	if  (Win_setup)  {
		clear();
		refresh();
		endwinkeys();
		Win_setup = 0;
	}
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

/* Provide lists of forms */

char **uhelpform(const char *sofar, const int hf)
{
	unsigned  ucnt, sfl = 0;
	char	**result, **rp;

	if  (sofar)
		sfl = strlen(sofar);

	/* There cannot be more form types than the number of users can there....  */

	result = (char **) malloc((Npwusers + 2) * sizeof(char *));
	if  (!result)
		return  result;
	rp = result;
	if  (strncmp(Spuhdr.sph_form, sofar, sfl) == 0)
		*rp++ = stracpy(Spuhdr.sph_form);
	for  (ucnt = 0;  ucnt < Npwusers;  ucnt++)  {
		if  (strncmp(ulist[ucnt].spu_form, sofar, sfl) == 0)  {
			char	**prev;
			for  (prev = result;  prev < rp;  prev++)
				if  (strcmp(ulist[ucnt].spu_form, *prev) == 0)
					goto  gotit;
			*rp++ = stracpy(ulist[ucnt].spu_form);
		}
	gotit:
		;
	}
	*rp++ = (char *) 0;
	return  result;
}

char **syshelpform(const char *sofar, const int hf)
{
	unsigned  ucnt, sfl = 0;
	char	**result, **rp;

	if  (sofar)
		sfl = strlen(sofar);

	/* There cannot be more form types than the number of users can there.... */

	result = (char **) malloc((Npwusers + 1) * sizeof(char *));
	if  (!result)
		return  result;
	rp = result;
	for  (ucnt = 0;  ucnt < Npwusers;  ucnt++)  {
		if  (strncmp(ulist[ucnt].spu_form, sofar, sfl) == 0)  {
			char	**prev;
			for  (prev = result;  prev < rp;  prev++)
				if  (strcmp(ulist[ucnt].spu_form, *prev) == 0)
					goto  gotit;
			*rp++ = stracpy(ulist[ucnt].spu_form);
		}
	gotit:
		;
	}
	*rp++ = (char *) 0;
	return  result;
}

/* Expand privilege codes into messages */

static	void  expcodes()
{
	int	i, j, look4, permstart;

	if  (code_expanded)
		return;

	permstart = helpnstate($P{Priv descr edit admin file}-1);

	for  (i = 0;  i < MAXPERM;  i++)
		ptab[i].string = gprompt(ptab[i].number);
	for  (i = 0;  i < MAXPERM;  i++)
		ptab[i].nextstate = helpnstate(ptab[i].number);

	i = 0;
	look4 = permstart;
	for  (;;)  {
		for  (j = 0;  j < MAXPERM;  j++)
			if  (ptab[j].number == look4)  {
				plist[i] = &ptab[j];
				i++;
				look4 = ptab[j].nextstate;
				goto  dun;
			}
		disp_arg[9] = look4;
		print_error($E{spuser privs missing code});
		exit(E_BADCFILE);
	dun:
		if  (look4 < 0)  {
			if  (i != MAXPERM)  {
				print_error($E{spuser privs scrambled});
				exit(E_BADCFILE);
			}
			break;
		}
	}

	code_expanded = 1;
}

/* Deal with signals.  */

RETSIGTYPE  catchit(int n)
{
#ifndef	HAVE_ATEXIT
	exit_cleanup();
#endif
	exit(E_SIGNAL);
}

/* Bodge dohelp for when we don't have a specific thing to do other
   than display a message code.  */

static	void  dochelp(WINDOW *wp, int code)
{
	struct	sctrl	xx;
	xx.helpcode = code;
	xx.helpfn = HELPLESS;

	dohelp(wp, &xx, NULLCH);
}

void  endhe(WINDOW *owin, WINDOW **wpp)
{
	delwin(*wpp);
	*wpp = (WINDOW *) 0;
	if  (TPLINES > 0)  {
		touchwin(tpscr);
#ifdef HAVE_TERMINFO
		wnoutrefresh(tpscr);
#else
		wrefresh(tpscr);
#endif
	}
	touchwin(owin);
	if  (owin != stdscr)  {
		WINDOW	*unwin = hscr;
		if  (owin == hscr)
			unwin = dscr;
		touchwin(unwin);
#ifdef HAVE_TERMINFO
		wnoutrefresh(unwin);
#else
		wrefresh(unwin);
#endif
	}
#ifdef HAVE_TERMINFO
	wnoutrefresh(owin);
	doupdate();
#else
	wrefresh(owin);
#endif
}

int	cpriv(char *uname, ULONG *fp)
{
	ULONG	 result = *fp;
	int	srow, erow, currow, ch, i, err_no, changes = 0;
	char	*pp;
	char	pcol[MAXPERM];
	static	char	*yes, *no;

	Ew = stdscr;

	/* First time in get yes/no strings */

	if  (!yes)  {
		yes = gprompt($P{Spuser yes});
		no = gprompt($P{Spuser no});
	}

	/* Likewise expand out privilege names */

	expcodes();

	clear();

	if  (uname)  {
		char	*hp;
		standout();
		disp_str = uname;
		hp = gprompt($P{Privileges for user});
		pp = gprompt($P{User is allowed});
		printw(hp, uname);
		standend();
		addstr("\n\n");
		free(hp);
	}
	else  {
		char	*hp;
		hp = gprompt($P{Default privileges});
		pp = gprompt($P{Default is to allow});
		printw("%s\n\n", hp);
		free(hp);
	}

	getyx(stdscr, srow, i);

	for  (i = 0;  i < MAXPERM;  i++)  {
		printw(pp, uname);
		printw(" %s: ", plist[i]->string);
		getyx(stdscr, erow, pcol[i]);
		printw("%s\n", plist[i]->flg & result? yes: no);
	}
	free(pp);
	select_state($S{spuser privs state});

	currow = srow;
	move(srow, pcol[0]);
	refresh();

	for  (;;)  {
		do  ch = getkey(MAG_P|MAG_A);
		while  (ch == EOF  &&  (hlpscr || escr));

		if  (hlpscr)  {
			endhe(stdscr, &hlpscr);
			if  (helpclr)
				continue;
		}
		if  (escr)
			endhe(stdscr, &escr);

		switch  (ch)  {
		default:
			err_no = $E{spuser bad cmd setting privs};
		err:
			doerror(stdscr, err_no);
		case  EOF:
			continue;

		case  $K{key help}:
			dochelp(stdscr, uname?
				$H{spuser privs state}:
				$H{spuser def privs state});
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			refresh();
			continue;

		case  $K{key eol}:
			if  (currow >= erow)
				goto  fin;
		case  $K{key cursor down}:
			currow++;
			if  (currow > erow)  {
				currow--;
				err_no = $E{spuser privs off bottom};
				goto  err;
			}
mc:			move(currow, pcol[currow-srow]);
			refresh();
			continue;

		case  $K{key cursor up}:
			if  (currow <= srow)  {
				err_no = $E{spuser privs off top};
				goto  err;
			}
			currow--;
			goto  mc;

		case  $K{key top}:
			currow = srow;
			goto  mc;

		case  $K{key bottom}:
			currow = erow;
			goto  mc;

		case  $K{spuser key toggle}:
			if  (result & plist[currow - srow]->flg)
				goto  sfalse;
		case  $K{spuser key yes}:
			changes++;
			result |= plist[currow - srow]->sflg;
			goto  fixperm;

		case  $K{spuser key no}:
		sfalse:
			changes++;
			result &= plist[currow - srow]->rflg;
			goto  fixperm;

		case  $K{spuser key defs}:
			if  (!uname)  {
				err_no = $E{spuser already editing defs};
				goto  err;
			}
			if  (result == Spuhdr.sph_flgs)
				continue;
			result = Spuhdr.sph_flgs;
			changes++;
		fixperm:
			for  (i = 0;  i < MAXPERM;  i++)  {
				move(srow+i, pcol[i]);
				clrtoeol();
				addstr(plist[i]->flg & result? yes: no);
			}
			move(currow, pcol[currow-srow]);
			refresh();
			continue;

		case  $K{key halt}:
		fin:
			if  (!changes)  {
#ifdef	CURSES_MEGA_BUG
				clear();
				refresh();
#endif
				return  0;
			}
			*fp = result;
			if  (!uname)  {
				pp = gprompt($P{Copy to everyone but you});
				clear();
				standout();
				mvaddstr(LINES/2, 0, pp);
				standend();
				refresh();
				do  {
					ch = getkey(MAG_A|MAG_P);
					if  (ch == $K{key help})
						dochelp(stdscr, $H{Copy to everyone but you});
				}  while  (ch != $K{spuser key yes} && ch != $K{spuser key no});

				if  (ch == $K{spuser key yes})
					for  (i = 0;  i < Npwusers;  i++)  {
						int_ugid_t uu = ulist[i].spu_user;
						/* We don't really need this check technically but it gets the display right */
						if  (uu != Realuid && uu != Daemuid && uu != ROOTID)
							ulist[i].spu_flgs = result;
					}
				free(pp);
			}
#ifdef	CURSES_MEGA_BUG
			clear();
			refresh();
#endif
			return  1;
		}
	}
}

static void  disp_user(int unum, int row, const int andclear)
{
	struct  spdet  *up = &ulist[unum];
	char	*msg;
	ULONG	exclp;
	classcode_t	exclc;

	wmove(dscr, row, USNAM_COL);
	if  (andclear)
		wclrtoeol(dscr);
	wprintw(dscr, "%.7s", prin_uname((uid_t) up->spu_user));
	wn_fill(dscr, row, &wnt_udp, (LONG) up->spu_defp);
	wn_fill(dscr, row, &wnt_ulp, (LONG) up->spu_minp);
	wn_fill(dscr, row, &wnt_uhp, (LONG) up->spu_maxp);
	wn_fill(dscr, row, &wnt_ucp, (LONG) up->spu_cps);
	ws_fill(dscr, row, &wst_ufm, up->spu_form);
	ws_fill(dscr, row, &wst_upt, up->spu_ptr);
	msg = s_class;
	exclc = up->spu_class ^ Spuhdr.sph_class;
	if  (exclc != 0)  {
		msg = ns_class;
		if  ((exclc & Spuhdr.sph_class) == 0)
			msg = gt_class;
		else  if  ((exclc & ~Spuhdr.sph_class) == 0)
			msg = lt_class;
	}
	mvwaddstr(dscr, row, CLASS_COL, msg);
	msg = s_perm;
	exclp = up->spu_flgs ^ Spuhdr.sph_flgs;
	if  (exclp != 0)  {
		msg = ns_perm;
		if  ((exclp & Spuhdr.sph_flgs) == 0)
			msg = gt_perm;
		else  if  ((exclp & ~Spuhdr.sph_flgs) == 0)
			msg = lt_perm;
	}
	mvwaddstr(dscr, row, PRIV_COL, msg);
}

/* Fill up the screen.  If cflg is set, clear first and start again.  */

void  display(int cflg)
{
	int	i, row;

	if  (cflg)  {
		clear();
		refresh();

		werase(dscr);

		touchwin(hscr);
		wn_fill(hscr, DEFLINE, &wnt_sdp, (LONG) Spuhdr.sph_defp);
		wn_fill(hscr, DEFLINE, &wnt_slp, (LONG) Spuhdr.sph_minp);
		wn_fill(hscr, DEFLINE, &wnt_shp, (LONG) Spuhdr.sph_maxp);
		wn_fill(hscr, DEFLINE, &wnt_scp, (LONG) Spuhdr.sph_cps);
		ws_fill(hscr, DEFLINE, &wst_sfm, Spuhdr.sph_form);
		ws_fill(hscr, DEFLINE, &wst_spt, Spuhdr.sph_ptr);
#ifdef HAVE_TERMINFO
		if  (TPLINES > 0)  {
			touchwin(tpscr);
			wnoutrefresh(tpscr);
		}
		wnoutrefresh(dscr);
		wnoutrefresh(hscr);
		doupdate();
#else
		if  (TPLINES > 0)  {
			touchwin(tpscr);
			wrefresh(tpscr);
		}
		wrefresh(dscr);
		wrefresh(hscr);
#endif
	}

	werase(dscr);
	row = 0;
	more_above = 0;

	if (Top_user > 0) {
		wstandout(dscr);
		mvwprintw(dscr, row, (COLS-(int)strlen(more_amsg))/2, more_amsg, Top_user);
		wstandend(dscr);
		row++;
		more_above = 1;
	}

	more_below = 0;

	for  (i = Top_user;  i < Npwusers && row < DLINES;  i++, row++)  {
		if  (row >= (DLINES - 1)  &&  i < Npwusers - 1)  {
			wstandout(dscr);
			mvwprintw(dscr, row, (COLS - (int) strlen(more_bmsg))/2, more_bmsg, Npwusers - i);
			wstandend(dscr);
			more_below = 1;
			break;
		}
		disp_user(i, row, 0);
	}

	row = Current_user - Top_user;
	if  (row < 0)  {
		Current_user = Top_user;
		row = 0;
	}
	if  (row >= DLINES - more_above - more_below)  {
		row = DLINES - more_above - more_below - 1;
		Current_user = Top_user + row;
	}
	row += more_above;
#ifdef	CURSES_OVERLAP_BUG
	touchwin(dscr);
#endif
	wmove(dscr, row, 0);
	wrefresh(dscr);
}

void  screeninit(int hh)
{
	int	hrows, i;
#ifdef TOWER
	struct	termio	aswas, asis;
#endif
#ifdef	STRUCT_SIG
	struct	sigstruct_name  ze;
	ze.sighandler_el = catchit;
	sigmask_clear(ze);
	ze.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGINT, &ze, (struct sigstruct_name *) 0);
	sigact_routine(SIGQUIT, &ze, (struct sigstruct_name *) 0);
	sigact_routine(SIGHUP, &ze, (struct sigstruct_name *) 0);
#else
	signal(SIGINT, catchit);
	signal(SIGQUIT, catchit);
	signal(SIGHUP, catchit);
#endif
#ifdef TOWER
	ioctl(0, TCGETA, &aswas);
#endif
#ifdef M88000
	if  (sysconf(_SC_JOB_CONTROL) < 0)
		fprintf(stderr, "WARNING: No job control\n");
#endif
	initscr();
	raw();
	nonl();
	noecho();

#ifdef	TOWER

	/* Restore the port's hardware to what it was before curses
	   did its dirty deed.  */

	ioctl(0, TCGETA, &asis);
	asis.c_cflag = aswas.c_cflag;
	ioctl(0, TCSETA, &asis);
#endif
	Win_setup = 1;

	deflt_str = gprompt($P{Spuser default string});

	if  (hh)  {
		char	**hv, **hvi, **tp;
		hv = helphdr('T');
		tp = helphdr('U');
		count_hv(hv, &hrows, &i);
		count_hv(tp, &TPLINES, &i);
		HLINES = hrows + 2;
		DEFLINE = hrows;
		DLINES = LINES - HLINES - TPLINES;
		hscr = newwin(HLINES, 0, 0, 0);
		dscr = newwin(DLINES, 0, HLINES, 0);
		if  (!hscr  ||  !dscr)
			nomem();
		if  (TPLINES > 0  &&  !(tpscr = newwin(TPLINES, 0, HLINES+DLINES, 0)))
			nomem();
		mvwaddstr(hscr, DEFLINE, USNAM_COL, deflt_str);
		for  (i = 0, hvi = hv;  *hvi;  i++, hvi++)  {
			mvwhdrstr(hscr, i, 0, *hvi);
			free(*hvi);
		}
		free((char *) hv);
		if  (TPLINES > 0)  {
			for  (i = 0, hvi = tp;  *hvi;  i++, hvi++)  {
				mvwhdrstr(tpscr, i, 0, *hvi);
				free(*hvi);
			}
			free((char *) tp);
		}
		s_class = gprompt($P{Class std});
		ns_class = gprompt($P{Non std class});
		lt_class = gprompt($P{Class less than});
		gt_class = gprompt($P{Class greater than});
		s_perm = gprompt($P{Perm std});
		ns_perm = gprompt($P{Non std perm});
		lt_perm = gprompt($P{Perm less than});
		gt_perm = gprompt($P{Perm greater than});
		more_amsg = gprompt($P{Spuser more above});
		more_bmsg = gprompt($P{Spuser more below});
	}
}

static	void  copyu(int n)
{
	struct	spdet	*up = &ulist[n];
	up->spu_defp = Spuhdr.sph_defp;
	up->spu_minp = Spuhdr.sph_minp;
	up->spu_maxp = Spuhdr.sph_maxp;
	up->spu_cps = Spuhdr.sph_cps;
	strncpy(up->spu_form, Spuhdr.sph_form, MAXFORM);
	strncpy(up->spu_formallow, Spuhdr.sph_formallow, ALLOWFORMSIZE);
	strncpy(up->spu_ptr, Spuhdr.sph_ptr, PTRNAMESIZE);
	strncpy(up->spu_ptrallow, Spuhdr.sph_ptrallow, JPTRNAMESIZE);
}

static void  user_macro(const int up, const int num)
{
	char	*prompt = helpprmpt(num + $P{Job or User macro}), *str;
	static	char	*execprog;
	PIDTYPE	pid;
	int	status, refreshscr = 0;
#ifdef	HAVE_TERMIOS_H
	struct	termios	save;
#else
	struct	termio	save;
#endif

	if  (!prompt)  {
		disp_arg[0] = num + $P{Job or User macro};
		doerror(dscr, $E{Macro error});
		return;
	}
	if  (!execprog)
		execprog = envprocess(EXECPROG);

	str = prompt;
	if  (*str == '!')  {
		str++;
		refreshscr++;
	}

	if  (num == 0)  {
		int	usy, usx;
		struct	sctrl	dd;
		wclrtoeol(dscr);
		waddstr(dscr, str);
		getyx(dscr, usy, usx);
		dd.helpcode = $H{Job or User macro};
		dd.helpfn = HELPLESS;
		dd.size = COLS - usx;
		dd.col = usx;
		dd.magic_p = MAG_P|MAG_OK;
		dd.min = dd.vmax = 0;
		dd.msg = (char *) 0;
		str = wgets(dscr, usy, &dd, "");
		if  (!str || str[0] == '\0')  {
			free(prompt);
			return;
		}
		if  (*str == '!')  {
			str++;
			refreshscr++;
		}
	}

	if  (refreshscr)  {
#ifdef	HAVE_TERMIOS_H
		tcgetattr(0, &save);
		tcsetattr(0, TCSADRAIN, &orig_term);
#else
		ioctl(0, TCGETA, &save);
		ioctl(0, TCSETAW, &orig_term);
#endif
	}

	if  ((pid = fork()) == 0)  {
		char	*argbuf[3];
		argbuf[0] = str;
		argbuf[1] = prin_uname((uid_t) ulist[up].spu_user);
		argbuf[2] = (char *) 0;
		if  (!refreshscr)  {
			close(0);
			close(1);
			close(2);
			Ignored_error = dup(dup(open("/dev/null", O_RDWR)));
		}
		execv(execprog, argbuf);
		exit(255);
	}
	free(prompt);
	if  (pid < 0)  {
		doerror(dscr, $E{Macro fork failed});
		return;
	}
#ifdef	HAVE_WAITPID
	while  (waitpid(pid, &status, 0) < 0)
		;
#else
	while  (wait(&status) != pid)
		;
#endif

	if  (refreshscr)  {
#ifdef	HAVE_TERMIOS_H
		tcsetattr(0, TCSADRAIN, &save);
#else
		ioctl(0, TCSETAW, &save);
#endif
		wrefresh(curscr);
	}

	if  (status != 0)  {
		if  (status & 255)  {
			disp_arg[0] = status & 255;
			doerror(dscr, $E{Macro command gave signal});
		}
		else  {
			disp_arg[0] = (status >> 8) & 255;
			doerror(dscr, $E{Macro command error});
		}
	}
}

static int  getpatt(WINDOW *w, const int row, const int state, const unsigned width, char *res, const unsigned mag, char *msg)
{
	char	*prompt, *str;
	struct	sctrl	sc;

	sc.helpcode = state;
	sc.helpfn = HELPLESS;
	sc.size = width;
	sc.retv = 0;
	sc.magic_p = mag;
	sc.min = 0L;
	sc.vmax = 0L;
	sc.msg = msg;

	prompt = gprompt(state);
	sc.col = strlen(prompt);

	wmove(w, row, 0);
	wclrtoeol(w);
	waddstr(w, prompt);
	free(prompt);
	waddstr(w, res);
	for  (;;)  {
		str = wgets(w, row, &sc, res);
		if  (!str)
			return  0;
		if  (repattok(str))  {
			strncpy(res, str, width);
			return  1;
		}
		doerror(w, $E{spuser regexp pattern error});
	}
}

static int  getclass(WINDOW *w, const int row, const int state, classcode_t *exist, char *msg)
{
	char	*prompt;
	classcode_t	res;
	struct	sctrl	sc;

	sc.helpcode = state;
	sc.helpfn = HELPLESS;
	sc.size = 32;
	sc.retv = 0;
	sc.magic_p = MAG_P;
	sc.min = 0L;
	sc.vmax = U_MAX_CLASS;
	sc.msg = msg;

	prompt = gprompt(state);
	sc.col = strlen(prompt);

	wmove(w, row, 0);
	wclrtoeol(w);
	waddstr(w, prompt);
	free(prompt);
	waddstr(w, hex_disp(*exist, 1));
	res = whexnum(w, row, &sc, *exist);
	select_state($H{spuser interactive state});
	if  (res == *exist)
		return  0;
	*exist = res;
	return  1;
}

/* Spit out a prompt for a search string */

static	char *gsearchs(const int isback)
{
	int	row;
	char	*gstr;
	struct	sctrl	ss;
	static	char	*lastmstr;
	static	char	*sforwmsg, *sbackwmsg;

	if  (!sforwmsg)  {
		sforwmsg = gprompt($P{Fsearch user});
		sbackwmsg = gprompt($P{Rsearch user});
	}

	ss.helpcode = $H{Fsearch user};
	gstr = isback? sbackwmsg: sforwmsg;
	ss.helpfn = HELPLESS;
	ss.size = 30;
	ss.retv = 0;
	ss.col = (SHORT) strlen(gstr);
	ss.magic_p = MAG_OK;
	ss.min = 0L;
	ss.vmax = 0L;
	ss.msg = (char *) 0;
	row = Current_user - Top_user + more_above;
	mvwaddstr(dscr, row, 0, gstr);
	wclrtoeol(dscr);

	if  (lastmstr)  {
		ws_fill(dscr, row, &ss, lastmstr);
		gstr = wgets(dscr, row, &ss, lastmstr);
		if  (!gstr)
			return  (char *) 0;
		if  (gstr[0] == '\0')
			return  lastmstr;
	}
	else  {
		for  (;;)  {
			gstr = wgets(dscr, row, &ss, "");
			if  (!gstr)
				return  (char *) 0;
			if  (gstr[0])
				break;
			doerror(dscr, $E{Rsearch user});
		}
	}
	if  (lastmstr)
		free(lastmstr);
	return  lastmstr = stracpy(gstr);
}

/* Match a job string "vstr" against a pattern string "mstr" */

static	int  smatchit(const char *vstr, const char *mstr)
{
	const	char	*tp, *mp;
	while  (*vstr)  {
		tp = vstr;
		mp = mstr;
		while  (*mp)  {
			if  (*mp != '.'  &&  toupper(*mp) != toupper(*tp))
				goto  ng;
			mp++;
			tp++;
		}
		return  1;
	ng:
		vstr++;
	}
	return  0;
}

/* Only match user name for now, but write like this to allow for
   future e-x-p-a-n-s-i-o-n.  */

static	int  smatch(const int mline, const char *mstr)
{
	return  smatchit(prin_uname((uid_t) ulist[mline].spu_user), mstr);
}

/* Search for string in user name
   Return 0 - found (Current_user and Top_user suitably mangled)
   otherwise return error code */

static	int  dosearch(const int isback)
{
	char	*mstr = gsearchs(isback);
	int	mline;

	if  (!mstr)
		return  0;

	if  (isback)  {
		for  (mline = Current_user - 1;  mline >= 0;  mline--)
			if  (smatch(mline, mstr))
				goto  gotit;
		for  (mline = Npwusers - 1;  mline >= Current_user;  mline--)
			if  (smatch(mline, mstr))
				goto  gotit;
	}
	else  {
		for  (mline = Current_user + 1;  (unsigned) mline < Npwusers;  mline++)
			if  (smatch(mline, mstr))
				goto  gotit;
		for  (mline = 0;  mline <= Current_user;  mline++)
			if  (smatch(mline, mstr))
				goto  gotit;
	}
	return  $E{Search user not found};

 gotit:
	Current_user = mline;
	if  (Current_user < Top_user  ||  Current_user - Top_user + more_above + more_below >= DLINES)
		Top_user = Current_user;
	return  0;
}

/* This accepts input from the screen.  */

int  process()
{
	LONG	num;
	int	ch, changes = 0, u_p, err_no;
	char	*str;
	static	char	*cch;

	Ew = dscr;

	Current_user = 0;
	Top_user = 0;

	display(1);
	select_state($H{spuser interactive state});

	for  (;;)  {
		u_p = Current_user - Top_user + more_above;

	nextin:
		do  ch = getkey(MAG_A|MAG_P);
		while  (ch == EOF && (hlpscr || escr));

	gotit:
		if  (hlpscr)  {
			endhe(dscr, &hlpscr);
			if  (helpclr)
				goto  nextin;
		}
		if  (escr)
			endhe(dscr, &escr);

		switch  (ch)  {
		case  EOF:
			continue;
		default:
			err_no = $E{spuser unknown command};
		err:
			doerror(dscr, err_no);
			continue;

		case  $K{key help}:
			dochelp(dscr, $H{spuser interactive state});
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			wrefresh(dscr);		/*  Restore cursor  */
			continue;

		case  $K{key cursor down}:
			Current_user++;
			u_p++;
			if  (Current_user >= Npwusers)  {
				Current_user--;
ej:				err_no = $E{spuser off bottom};
				goto  err;
			}
			if  (u_p >= DLINES - more_below)  {
				if  (++Top_user <= 2)
					Top_user++;
				display(0);
			}
			else  {
				wmove(dscr, u_p, 0);
				wrefresh(dscr);
			}
			continue;

		case  $K{key cursor up}:
			if  (Current_user <= 0)  {
bj:				err_no = $E{spuser off top};
				goto  err;
			}
			Current_user--;
			u_p--;
			if  (Current_user < Top_user)  {
				Top_user = Current_user;
				if (Current_user == 1)
					Top_user = 0;
				display(0);
			}
			else  {
				wmove(dscr, u_p, 0);
				wrefresh(dscr);
			}
			continue;

		case  $K{key top}:
			if  (Current_user != Top_user && Current_user != 0)  {
				Current_user = Top_user < 0? 0: Top_user;
				u_p = more_above;
				wmove(dscr, u_p, 0);
				wrefresh(dscr);
				continue;
			}
			Current_user = 0;
			Top_user = 0;
			display(0);
			continue;

		case  $K{key bottom}:
			ch = Top_user + DLINES - more_above - more_below - 1;
			if  (Current_user < ch  &&  ch < Npwusers - 1)  {
				Current_user = ch;
				u_p = Current_user - Top_user + more_above;
				wmove(dscr, u_p, 0);
				wrefresh(dscr);
				continue;
			}
			if  (Npwusers > DLINES)
				Top_user = Npwusers - DLINES + 1;
			else
				Top_user = 0;
			Current_user = Npwusers - 1;
			display(0);
			continue;

		case  $K{spuser key forward search}:
		case  $K{spuser key backward search}:
			if  ((err_no = dosearch(ch == $K{spuser key backward search})) != 0)  {
				doerror(dscr, err_no);
				disp_user(Current_user, u_p, 1);
				goto  ucnt;
			}
			display(0);
			continue;

		case  $K{spuser key def user form}:
			wst_ufm.msg = prin_uname((uid_t) ulist[Current_user].spu_user);
			str = wgets(dscr, u_p, &wst_ufm, ulist[Current_user].spu_form);
			if  (str)  {
				changes++;
				strncpy(ulist[Current_user].spu_form, str, MAXFORM);
			}
			if  (wst_ufm.retv > 0)		/* Overflowed field */
				disp_user(Current_user, u_p, 1);
			goto  ucnt;

		case  $K{spuser key sys def form}:
			str = wgets(hscr, DEFLINE, &wst_sfm, Spuhdr.sph_form);
			if  (str)  {
				hchanges++;
				strncpy(Spuhdr.sph_form, str, MAXFORM);
			}
			if  (wst_sfm.retv <= 0)
				goto  ucnt;
		refdef:
			wmove(hscr, DEFLINE, 0);
			wclrtoeol(hscr);
			mvwaddstr(hscr, DEFLINE, USNAM_COL, deflt_str);
			wn_fill(hscr, DEFLINE, &wnt_sdp, (LONG) Spuhdr.sph_defp);
			wn_fill(hscr, DEFLINE, &wnt_slp, (LONG) Spuhdr.sph_minp);
			wn_fill(hscr, DEFLINE, &wnt_shp, (LONG) Spuhdr.sph_maxp);
			wn_fill(hscr, DEFLINE, &wnt_scp, (LONG) Spuhdr.sph_cps);
			ws_fill(hscr, DEFLINE, &wst_sfm, Spuhdr.sph_form);
			ws_fill(hscr, DEFLINE, &wst_spt, Spuhdr.sph_ptr);
			wrefresh(hscr);
			goto  ucnt;

		case  $K{spuser key def user ptr}:
			wst_upt.msg = prin_uname((uid_t) ulist[Current_user].spu_user);
			str = wgets(dscr, u_p, &wst_upt, ulist[Current_user].spu_ptr);
			if  (str)  {
				changes++;
				strncpy(ulist[Current_user].spu_ptr, str, PTRNAMESIZE);
			}
			if  (wst_upt.retv > 0)		/* Overflowed field */
				disp_user(Current_user, u_p, 1);
			goto  ucnt;

		case  $K{spuser key sys def ptr}:
			str = wgets(hscr, DEFLINE, &wst_spt, Spuhdr.sph_ptr);
			if  (str)  {
				hchanges++;
				strncpy(Spuhdr.sph_ptr, str, PTRNAMESIZE);
			}
			if  (wst_spt.retv <= 0)
				goto  ucnt;
			goto  refdef;

		case  $K{spuser key user restrict form}:
			changes += getpatt(dscr,
					   u_p,
					   $HP{spuser user form patt},
					   ALLOWFORMSIZE,
					   ulist[Current_user].spu_formallow,
					   MAG_OK|MAG_P|MAG_NL|MAG_FNL,
					   prin_uname((uid_t) ulist[Current_user].spu_user));
			disp_user(Current_user, u_p, 1);
			goto  ucnt;

		case  $K{spuser key sys restrict form}:
			hchanges += getpatt(hscr,
					    DEFLINE,
					    $HP{spuser sys form patt},
					    ALLOWFORMSIZE,
					    Spuhdr.sph_formallow,
					    MAG_OK|MAG_P|MAG_NL|MAG_FNL,
					    (char *) 0);
			goto  refdef;

		case  $K{spuser key user restrict ptr}:
			changes += getpatt(dscr,
					    u_p,
					    $HP{spuser user ptr patt},
					    JPTRNAMESIZE,
					    ulist[Current_user].spu_ptrallow,
					    MAG_OK|MAG_P,
					    prin_uname((uid_t) ulist[Current_user].spu_user));
			disp_user(Current_user, u_p, 1);
			goto  ucnt;

		case  $K{spuser key sys restrict ptr}:
			hchanges += getpatt(hscr,
					    DEFLINE,
					    $HP{spuser sys ptr patt},
					    JPTRNAMESIZE,
					    Spuhdr.sph_ptrallow,
					    MAG_OK|MAG_P,
					    (char *) 0);
			goto  refdef;

		case  $K{spuser key user def pri}:
			wnt_udp.msg = prin_uname((uid_t) ulist[Current_user].spu_user);
			num = wnum(dscr, u_p, &wnt_udp, (LONG) ulist[Current_user].spu_defp);
			if  (num > 0L)  {
				changes++;
				ulist[Current_user].spu_defp = (unsigned char) num;
			}
		ucnt:
			wmove(dscr, u_p, 0);
			wrefresh(dscr);
			continue;

		case  $K{spuser key sys def pri}:
			num = wnum(hscr, DEFLINE, &wnt_sdp, (LONG) Spuhdr.sph_defp);
			if  (num > 0L)  {
				hchanges++;
				Spuhdr.sph_defp = (unsigned char) num;
			}
			goto  ucnt;

		case  $K{spuser key user min pri}:
			wnt_ulp.msg = prin_uname((uid_t) ulist[Current_user].spu_user);
			num = wnum(dscr, u_p, &wnt_ulp, (LONG) ulist[Current_user].spu_minp);
			if  (num > 0L)  {
				changes++;
				ulist[Current_user].spu_minp = (unsigned char) num;
			}
			goto  ucnt;

		case  $K{spuser key def min pri}:
			num = wnum(hscr, DEFLINE, &wnt_slp, (LONG) Spuhdr.sph_minp);
			if  (num > 0L)  {
				hchanges++;
				Spuhdr.sph_minp = (unsigned char) num;
			}
			goto  ucnt;

		case  $K{spuser key user max pri}:
			wnt_uhp.msg = prin_uname((uid_t) ulist[Current_user].spu_user);
			num = wnum(dscr, u_p, &wnt_uhp, (LONG) ulist[Current_user].spu_maxp);
			if  (num > 0L)  {
				changes++;
				ulist[Current_user].spu_maxp = (unsigned char) num;
			}
			goto  ucnt;

		case  $K{spuser key def max pri}:
			num = wnum(hscr, DEFLINE, &wnt_shp, (LONG) Spuhdr.sph_maxp);
			if  (num > 0L)  {
				hchanges++;
				Spuhdr.sph_maxp = (unsigned char) num;
			}
			goto  ucnt;

		case  $K{spuser key sys max copies}:
			num = wnum(hscr, DEFLINE, &wnt_scp, (LONG) Spuhdr.sph_cps);
			if  (num >= 0L)  {
				hchanges++;
				Spuhdr.sph_cps = (unsigned char) num;
			}
			goto  ucnt;

		case  $K{spuser key user max copies}:
			wnt_ucp.msg = prin_uname((uid_t) ulist[Current_user].spu_user);
			num = wnum(dscr, u_p, &wnt_ucp, (LONG) ulist[Current_user].spu_cps);
			if  (num >= 0L)  {
				changes++;
				ulist[Current_user].spu_cps = (unsigned char) num;
			}
			goto  ucnt;

		case  $K{spuser key user charge}:
			if  (!cch)
				cch = gprompt($P{Spuser current charge});
			mvwprintw(dscr, u_p, 0, cch, prin_uname((uid_t) ulist[Current_user].spu_user), 0);
			wrefresh(dscr);
			ch = getkey(MAG_A|MAG_P);
			disp_user(Current_user, u_p, 1);
			wmove(dscr, u_p, 0);
			wrefresh(dscr);
			if  (helpclr)
				goto  nextin;
			goto  gotit;

		case  $K{spuser key sys def to all}:
			for  (ch = 0;  ch < Npwusers;  ch++)
				copyu(ch);
			display(0);
			changes++;
			continue;

		case  $K{spuser key sys def to user}:
			copyu(Current_user);
			disp_user(Current_user, u_p, 1);
			changes++;
			continue;

		case  $K{spuser key user priv}:
			changes += cpriv(prin_uname((uid_t) ulist[Current_user].spu_user), &ulist[Current_user].spu_flgs);
		refill:
			select_state($H{spuser interactive state});
			Ew = dscr;
			display(1);
			continue;

		case  $K{spuser key sys priv}:
			hchanges += cpriv(NULLCH, &Spuhdr.sph_flgs);
			goto  refill;

		case  $K{key save opts}:
			propts();
			goto  refill;

		case  $K{spuser key user class}:
			changes += getclass(dscr,
					    u_p,
					    $HP{spuser class code help},
					    &ulist[Current_user].spu_class,
					    prin_uname((uid_t) ulist[Current_user].spu_user));
			disp_user(Current_user, u_p, 1);
			goto  ucnt;

		case  $K{spuser key sys class}:
			if  (getclass(hscr,
				       DEFLINE,
				       $HP{spuser sys class code help},
				       &Spuhdr.sph_class,
				       (char *) 0))  {
				char	*pp = gprompt($P{spuser askyorn});
				hchanges++;
				clear();
				standout();
				mvaddstr(LINES/2, 0, pp);
				free(pp);
				standend();
				refresh();
				Ew = stdscr;
				select_state($S{spuser askyorn});
				do  {
					ch = getkey(MAG_A|MAG_P);
					if  (ch == $K{key help})
						dochelp(stdscr, $H{spuser askyorn});
				}  while  (ch != $K{spuser key yes} && ch != $K{spuser key no});
				select_state($S{spuser interactive state});
				Ew = dscr;
				if  (TPLINES > 0)  {
					touchwin(tpscr);
					wrefresh(tpscr);
				}
				touchwin(hscr);
				wrefresh(hscr);
				touchwin(dscr);
				if  (ch == $K{spuser key yes})   {
					for  (num = 0L; num < (LONG) Npwusers; num++)
						ulist[num].spu_class = Spuhdr.sph_class;
				}
				display(0);
				wrefresh(dscr);
			}
			goto  ucnt;

		case  $K{key halt}:
			clear();
			refresh();
			endwinkeys();
			Win_setup = 0;
			return  changes;

		case  $K{key screen down}:
			if  (Top_user + DLINES - more_above >= Npwusers)
				goto  ej;
			Top_user += DLINES - more_above - more_below;
			if  ((Current_user += DLINES - more_above - more_below) >= Npwusers)
				Current_user = Npwusers - 1;
			display(0);
			continue;

		case  $K{key half screen down}:
			if  (Top_user + DLINES/2 >= Npwusers)
				goto  ej;
			Top_user += (DLINES - more_above - more_below) / 2;
			if  (Current_user < Top_user)
				Current_user = Top_user;
			display(0);
			continue;

		case  $K{key half screen up}:
			if  (Top_user <= 0)
				goto  bj;
			Top_user -= (DLINES - more_above - more_below) /2;
		restu:
			if  (Top_user <= 1) {
				Top_user = 0;
				more_above = 0;
			}
			if ((!more_below) &&
			   (Top_user + (2*DLINES) + 1 - more_above > Npwusers) &&  (more_above))
				Top_user++;
			if  (Current_user - Top_user >= (DLINES - more_above - more_below))
				Current_user = Top_user + DLINES - 1 - more_above - more_below;
			display(0);
			continue;

		case  $K{key screen up}:
			if  (Top_user <= 0)
				goto  bj;
			Top_user -= DLINES - more_above - more_below;
			goto  restu;

		case  $K{spuser key exec}:  case  $K{spuser key exec}+1:case  $K{spuser key exec}+2:case  $K{spuser key exec}+3:case  $K{spuser key exec}+4:
		case  $K{spuser key exec}+5:case  $K{spuser key exec}+6:case  $K{spuser key exec}+7:case  $K{spuser key exec}+8:case  $K{spuser key exec}+9:
			user_macro(Current_user, ch - $K{spuser key exec});
			display(0);
			if  (escr)  {
				touchwin(escr);
				wrefresh(escr);
			}
			continue;
		}
	}
}

#define	PRIO_ROW	3
#define	FORM_ROW	7
#define	PTR_ROW		9
#define	LIMS_ROW	14

int  change(struct spdet *priv)
{
	int	pcol, fcol, ptrcol, row;
	int	ch, err_no, changes = 0;
	LONG	num;
	char	*str;
	struct	sctrl	wst_ucfm, wst_ucpt, wnt_cdf;

	/* Initialise fields - form/ptr/pri */

	wst_ucfm.helpcode = $H{spuser new form type help};
	wst_ucfm.helpfn = HELPLESS;
	wst_ucfm.size = MAXFORM;
	wst_ucfm.retv = 0;
	wst_ucfm.magic_p = MAG_P;
	wst_ucfm.min = 0L;
	wst_ucfm.vmax = 0L;
	wst_ucfm.msg = NULLCH;

	wst_ucpt.helpcode = $H{spuser new ptr help};
	wst_ucpt.helpfn = HELPLESS;
	wst_ucpt.size = PTRNAMESIZE;
	wst_ucpt.retv = 0;
	wst_ucpt.magic_p = MAG_P;
	wst_ucpt.min = 0L;
	wst_ucpt.vmax = 0L;
	wst_ucpt.msg = NULLCH;

	wnt_cdf.helpcode = $H{spuser own def pri help};
	wnt_cdf.helpfn = HELPLESS;
	wnt_cdf.size = 3;
	wnt_cdf.retv = 0;
	wnt_cdf.magic_p = MAG_P;
	wnt_cdf.min = 1L;
	wnt_cdf.vmax = 255L;
	wnt_cdf.msg = NULLCH;

 restart:
	str = gprompt($P{Cdefs params for user});
	standout();
	mvprintw(0, 0, str, prin_uname(Realuid));
	standend();
	free(str);

	str = gprompt($P{Cdefs prios});
	mvprintw(LIMS_ROW,
			0,
			str,	/*  The format!!!!  */
			priv->spu_minp,
			priv->spu_maxp,
			Spuhdr.sph_defp);
	free(str);

	/* Insert details of form and printer restrictions if applicable */

	row = LIMS_ROW + 1;

	if  (!(priv->spu_flgs & PV_FORMS))  {
		str = gprompt($P{Cdefs formlim});
		mvprintw(row, 0, str, priv->spu_formallow);
		free(str);
		row++;
	}
	if  (!(priv->spu_flgs & PV_OTHERP))  {
		str = gprompt($P{Cdefs ptrlim});
		mvprintw(row, 0, str, priv->spu_ptrallow);
		free(str);
		row++;
	}

	/* Display existing priority */

	str = gprompt($P{Cdefs def pri});
	mvaddstr(PRIO_ROW, 20, str);
	free(str);
	getyx(stdscr, ch, pcol);
	wnt_cdf.col = (unsigned char) pcol;
	wn_fill(stdscr, PRIO_ROW, &wnt_cdf, (LONG) priv->spu_defp);

	/* Display existing form */

	str = gprompt($P{Cdefs def form});
	mvaddstr(FORM_ROW, 0, str);
	free(str);
	getyx(stdscr, ch, fcol);
	wst_ucfm.col = (unsigned char) fcol;
	ws_fill(stdscr, FORM_ROW, &wst_ucfm, priv->spu_form);

	/* Display existing ptr */

	str = gprompt($P{Cdefs def ptr});
	mvaddstr(PTR_ROW, 0, str);
	free(str);
	getyx(stdscr, ch, ptrcol);
	wst_ucpt.col = (unsigned char) ptrcol;
	ws_fill(stdscr, PTR_ROW, &wst_ucpt, priv->spu_ptr);

	select_state($S{spuser cdef state});
	Ew = stdscr;

	for  (;;)  {
		move(LINES-1, COLS-1);
		refresh();

	cont:
		do  ch = getkey(MAG_A|MAG_P);
		while  (ch == EOF && (hlpscr || escr));

		if  (hlpscr)  {
			endhe(stdscr, &hlpscr);
			if  (helpclr)
				continue;
		}
		if  (escr)
			endhe(stdscr, &escr);

		switch  (ch)  {
		case  EOF:
			continue;
		default:
			err_no = $E{spuser cdef unknown cmd};
		err:
			doerror(stdscr, err_no);
			continue;

		case  $K{key help}:
			dochelp(stdscr, $H{spuser cdef state});
			goto  cont;

		case  $K{key refresh}:
			wrefresh(curscr);
			refresh();
			goto  cont;

		case  $K{key save opts}:
			propts();
			goto  restart;

		case  $K{spuser key own def pri}:
			wnt_cdf.min = (LONG) priv->spu_minp;
			wnt_cdf.vmax = (LONG) priv->spu_maxp;
			num = wnum(stdscr, PRIO_ROW, &wnt_cdf, (LONG) priv->spu_defp);
			if  (num > 0L)  {
				changes++;
				priv->spu_defp = (unsigned char) num;
			}
			continue;

		case  $K{spuser key own def form}:
			wst_ucfm.col = (unsigned char) fcol;
			str = wgets(stdscr, FORM_ROW, &wst_ucfm, priv->spu_form);
			if  (str)  {
				if  (*str == '\0')  {
					err_no = $E{spuser no form type};
					goto  err;
				}
				if  (!(priv->spu_flgs & PV_FORMS)  &&  !qmatch(priv->spu_formallow, str))  {
					err_no = $E{spuser disallowed form type};
					goto  err;
				}
				strncpy(priv->spu_form, str, MAXFORM);
				changes++;
			}
			continue;

		case  $K{spuser key own def ptr}:
			wst_ucpt.col = (unsigned char) ptrcol;
			str = wgets(stdscr, PTR_ROW, &wst_ucpt, priv->spu_ptr);
			if  (str)  {
				if  (!(priv->spu_flgs & PV_OTHERP)  &&  !qmatch(priv->spu_ptrallow, str))  {
					err_no = $E{spuser disallowed ptr type};
					goto  err;
				}
				strncpy(priv->spu_ptr, str, PTRNAMESIZE);
				changes++;
			}
			continue;

		case  $K{key halt}:
			clear();
			refresh();
			endwinkeys();
			return  changes;
		}
	}
}

void  display_info(struct spdet *mypriv)
{
	disp_str = mypriv->spu_form;
	disp_str2 = mypriv->spu_formallow;
	disp_arg[0] = mypriv->spu_cps;
	disp_arg[1] = mypriv->spu_minp;
	disp_arg[2] = mypriv->spu_maxp;
	disp_arg[3] = mypriv->spu_defp;
	disp_arg[4] = 0;
	fprint_error(stdout, $E{Spuser display values});
	disp_str = mypriv->spu_ptr;
	disp_str2 = mypriv->spu_ptrallow;
	fprint_error(stdout, $E{Spuser ptr display values});
	disp_str = hex_disp(mypriv->spu_class, 0);
	fprint_error(stdout, $E{Spuser display class});

	if  (mypriv->spu_flgs != 0)  {
		struct  perm	*cp, **ppp;
		char	*can;

		fprint_error(stdout, $E{Spuser display privs});
		can = gprompt($P{Spuser you may});
		expcodes();

		for  (ppp = plist;  (cp = *ppp);  ppp++)
			if  (mypriv->spu_flgs & cp->flg)
				printf("%s %s.\n", can, cp->string);
	}
}

int  sort_u(struct spdet *a, struct spdet *b)
{
	return  strcmp(prin_uname((uid_t) a->spu_user), prin_uname((uid_t) b->spu_user));
}

int  sort_id(struct spdet *a, struct spdet *b)
{
	return  (ULONG) a->spu_user < (ULONG) b->spu_user ? -1: (ULONG) a->spu_user == (ULONG) b->spu_user? 0: 1;
}

OPTION(o_explain)
{
	print_error($E{spuser options});
	exit(0);
}

OPTION(o_display)
{
	iflag = 0;
	cflag = 0;
	return  OPTRESULT_OK;
}

OPTION(o_formprio)
{
	iflag = 0;
	cflag = 1;
	return  OPTRESULT_OK;
}

OPTION(o_interact)
{
	iflag = 1;
	cflag = 0;
	return  OPTRESULT_OK;
}

#include "inline/o_usort.c"
#include "inline/o_boxes.c"

/* Defaults and proc table for arg interp.  */

static	const	Argdefault	Adefs[] = {
  {  '?', $A{spuser explain}	},
  {  'd', $A{spuser display}	},
  {  'c', $A{spuser formprio}	},
  {  'i', $A{spuser admin}	},
  {  'u', $A{spuser sort user}	},
  {  'n', $A{spuser sort uid}	},
  {  'H', $A{spuser keepchar}	},
  {  'h', $A{spuser losechar}	},
  {  'b', $A{spuser help box}	},
  {  'B', $A{spuser no help box}},
  {  'm', $A{spuser error box}	},
  {  'M', $A{spuser no error box}},
  {  0, 0 }
};

optparam  optprocs[] = {
o_explain,	o_display,	o_formprio,	o_interact,	o_usort,
o_nsort,	o_nohelpclr,	o_helpclr,	o_helpbox,	o_nohelpbox,
o_errbox,	o_noerrbox
};

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
	struct	spdet	*mypriv;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.2 $", 0);

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
	Cfile = open_cfile("SPUSERCONF", "spuser.help");
	SCRAMBLID_CHECK
	argv = optprocess(argv, Adefs, optprocs, $A{spuser explain}, $A{spuser no error box}, 1);
	SWAP_TO(Daemuid);

	mypriv = getspuentry(Realuid);		/* Always returns something or bombs */

	if  (iflag)  {
		if  (!(mypriv->spu_flgs & PV_ADMIN))  {
			print_error($E{Spuser no admin priv});
			exit(E_NOPRIV);
		}
#ifdef	HAVE_TERMIOS_H
		tcgetattr(0, &orig_term);
#else
		ioctl(0, TCGETA, &orig_term);
#endif
		setupkeys();
		screeninit(1);
#ifdef	HAVE_ATEXIT
		atexit(exit_cleanup);
#endif
		ulist = getspulist();
		if  (alphsort == SORT_USER)
			qsort(QSORTP1 ulist, Npwusers, sizeof(struct spdet), QSORTP4 sort_u);
		if  (process() || hchanges)  {
			if  (alphsort == SORT_USER)
				qsort(QSORTP1 ulist, Npwusers, sizeof(struct spdet), QSORTP4 sort_id);
			putspulist(ulist);
		}
	}
	else  if  (cflag)  {
		if  (!(mypriv->spu_flgs & PV_CDEFLT))  {
			print_error($E{Spuser no chng form priv});
			exit(E_NOPRIV);
		}
		mypriv = getspuentry(Realuid);
#ifdef	HAVE_TERMIOS_H
		tcgetattr(0, &orig_term);
#else
		ioctl(0, TCGETA, &orig_term);
#endif
		setupkeys();
		screeninit(0);
#ifdef	HAVE_ATEXIT
		atexit(exit_cleanup);
#endif
		if  (change(mypriv))
			putspuentry(mypriv);
	}
	else
		display_info(getspuser(Realuid));

	return  0;
}
