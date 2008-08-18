/* sq_jlist.c -- job list handling for spq

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
#include <setjmp.h>
#include <signal.h>
#include <curses.h>
#ifdef	HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <ctype.h>
#include <errno.h>
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "pages.h"
#include "keynames.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "displayopt.h"

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

void	dochelp(WINDOW *, const int);
void	doerror(WINDOW *, const int);
void	dohelp(WINDOW *, struct sctrl *, const char *);
void	endhe(WINDOW *, WINDOW **);
void	pdisplay(void);
void	rpfile(void);
void	my_wjmsg(const int);
void	womsg(const int);
void	offersave(char *, const int);

int	view_errors(const int);
int	propts(void);
int	viewfile(void);
int	qopts(const jobno_t);
int	fmtprocess(char **, const char, struct sq_formatdef *, struct sq_formatdef *);
int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);

char	**wotjform(const char *, const int);
char	**wotjprin(const char *, const int);

void	tdisplay(WINDOW *, const time_t, const int, const int);
int	wtime(WINDOW *, const int, const int);

#define	BOXWID	1

static	struct	sctrl
 ud_dir = { $H{spq unqueue dir},	HELPLESS, 0, 0, 0, MAG_P|MAG_OK, 0L, 0L, (char *) 0 },
 ud_xf =  { $H{spq cmd file},		HELPLESS, NAMESIZE, 0, 0, MAG_P|MAG_OK, 0L, 0L, (char *) 0 },
 ud_jf =  { $H{spq job file},		HELPLESS, NAMESIZE, 0, 0, MAG_P|MAG_OK, 0L, 0L, (char *) 0 };

#define	MAXSTEP	6

extern	int	JLINES, HJLINES, PLINES, HPLINES, TPLINES;
extern	int	hadrfresh, wh_jtitline;
extern	time_t	hadalarm, lastalarm;
extern	char	scrkeep, confabort, nopage, helpclr;
extern	char	*Realuname;
extern	char	*spdir, *Curr_pwd;
extern	struct	spdet	*mypriv;
extern	struct	spr_req	jreq, oreq;
extern	struct	spq	JREQ;

#define	JREQS	jreq.spr_un.j.spr_jslot
#define	OREQ	oreq.spr_un.o.spr_jpslot

jobno_t	Cjobno = -1;
int	Jhline, Jeline;
static	int	more_above, more_below;
static	char	*more_amsg, *more_bmsg, *localptr, *yesmsg, *nomsg;

extern	WINDOW	*hjscr, *hpscr, *tpscr, *jscr, *pscr, *escr, *hlpscr;
WINDOW	*Ew;

/* Open job file. Allocate memory for it as well.  */

void	openjfile(void)
{
	if  (!jobshminit(1))  {
		print_error($E{Cannot open jshm});
		exit(E_JOBQ);
	}

	/* Get "more" messages.  */

	more_amsg = gprompt($P{Jobs more above});
	more_bmsg = gprompt($P{Jobs more below});
	localptr = gprompt($P{Spq local printer});
	yesmsg = gprompt($P{spq qmsg yes});
	nomsg = gprompt($P{spq qmsg no});
}

/* Find current job in queue and adjust pointers as required.  The
   idea is that we try to preserve the position on the screen of
   the current line (even if we are currently looking at the
   print file).  Option - if scrkeep set, move job but keep the rest.  */

void	cjfind(void)
{
	int	row;

	if  (scrkeep)  {
		if  (Jhline >= Job_seg.njobs  &&  (Jhline = Job_seg.njobs - JLINES) < 0)
			Jhline = 0;
		if  (Jeline - Jhline >= JLINES)
			Jeline = Jhline - JLINES - 1;
	}
	else  {
		for  (row = 0;  row < Job_seg.njobs;  row++)  {
			if  (Job_seg.jj_ptrs[row]->j.spq_job == Cjobno)  {

				/* Move top of screen line up/down queue
				   by same amount as current job.
				   This code assumes that Jeline - Jhline < JLINES
				   but that we may wind up with Jhline < 0; */

				Jhline += row - Jeline;
				Jeline = row;
				return;
			}
		}
	}
	if  (Jeline >= Job_seg.njobs)  {
		if  (Job_seg.njobs == 0)
			Jhline = Jeline = 0;
		else  {
			Jhline -= Jeline - Job_seg.njobs + 1;
			Jeline = Job_seg.njobs - 1;
		}
	}
}

static	int	r_max(const int a, const int b)
{
	return  a > b? a: b;
}

/* Stuff to implement the unqueue command */

int	dounqueue(const struct spq *jp, const int nodelete)
{
	static	char	*udprog, *udprompt, *dirprompt, *xfilep, *jfilep;
	static	int	maxp;
	int	jsy, jmsy, dummy;
	PIDTYPE	pid;
	WINDOW	*cw;
	char	*Exist, *Direc, *Expdir, *Xfname, *Jfname, *arg0;
	char	**ap, *argbuf[8];
	struct	stat	sbuf;
	char	jobnobuf[40];

	/* Set these up in case we want to argue.  */

	disp_str = (char *) jp->spq_file;
	disp_arg[0] = jp->spq_job;

	/* First time round, read prompts */

	if  (!udprompt)  {
		udprog = envprocess(DUMPJOB);
		udprompt = gprompt($P{spq unqueue prompt});
		dirprompt = gprompt($P{spq unqueue dir});
		xfilep = gprompt($P{spq cmd file});
		jfilep = gprompt($P{spq job file});
		maxp = r_max(strlen(dirprompt), r_max(strlen(xfilep), strlen(jfilep)));
		ud_xf.col = ud_jf.col = ud_dir.col = maxp + BOXWID + 1;
		ud_dir.size = COLS - maxp - BOXWID - 2;
	}

	/* Create sub-window to input details */

	getbegyx(jscr, jsy, dummy);
	getmaxyx(jscr, jmsy, dummy);
	if  (jmsy < 4+2*BOXWID)  {
		doerror(jscr, $E{No space for unqueue box});
		return  0;
	}

	if  ((cw = newwin(5+2*BOXWID, 0, jsy + (jmsy - 5-2*BOXWID) / 2, 0)) == (WINDOW *) 0)  {
		doerror(jscr, $E{No subwin for unqueue});
		return  0;
	}

#ifdef	HAVE_TERMINFO
	box(cw, 0, 0);
#else
	box(cw, '|', '-');
#endif
	mvwprintw(cw, BOXWID, BOXWID, udprompt, (char *) jp->spq_file, jp->spq_job);
	mvwaddstr(cw, 2+BOXWID, BOXWID, dirprompt);
	ws_fill(cw, 2+BOXWID, &ud_dir, Curr_pwd);
	mvwaddstr(cw, 3+BOXWID, BOXWID, xfilep);
	mvwaddstr(cw, 4+BOXWID, BOXWID, jfilep);

	/* Read directory field */

	reset_state();
	Ew = cw;

	Exist = Curr_pwd;
	for  (;;)  {
		if  (!(Direc = wgets(cw, 2+BOXWID, &ud_dir, Exist)))  {
			delwin(cw);
#ifdef	CURSES_MEGA_BUG
			clear();
			refresh();
#endif
			return  -1;
		}
		if  (Direc[0] == '\0')
			Expdir = stracpy(Exist);
		else  if  (strchr(Direc, '~'))  {
			char	*nd = unameproc(Direc, Curr_pwd, Realuid);
			if  (!nd)  {
				doerror(cw, $E{No home directory for user});
				continue;
			}
			Expdir = envprocess(nd);
			free(nd);
		}
		else
			Expdir = envprocess(Direc);

		if  (!Expdir)  {
			disp_str = Direc;
			doerror(cw, $E{Bad envirs in unq});
			continue;
		}

		Exist = Direc[0]? Direc: Curr_pwd;

		if  (Expdir[0] != '/')  {
			disp_str = Expdir;
			doerror(cw, $E{Not absolute path});
			free(Expdir);
			continue;
		}

		if  (chdir(Expdir) >= 0)
			break;
		disp_str = Expdir;
		doerror(cw, $E{Unqueue no chdir});
		free(Expdir);
	}

	Exist = "";
	for  (;;)  {
		if  (!(Xfname = wgets(cw, 3+BOXWID, &ud_xf, Exist)))  {
			free(Expdir);
			delwin(cw);
#ifdef	CURSES_MEGA_BUG
			clear();
			refresh();
#endif
			chdir(spdir);
			return  -1;
		}

		disp_str = Xfname;
		if  (strchr(Xfname, '/'))  {
			doerror(cw, $E{Unqueue bad file name});
			continue;
		}

		if  (stat(Xfname, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) == S_IFREG)
			break;
		doerror(cw, $E{Unqueue not flat file});
		Exist = Xfname;
	}

	Xfname = stracpy(Xfname);
	Exist = "";
	for  (;;)  {
		if  (!(Jfname = wgets(cw, 4+BOXWID, &ud_jf, Exist)))  {
			free(Xfname);
			free(Expdir);
			delwin(cw);
#ifdef	CURSES_MEGA_BUG
			clear();
			refresh();
#endif
			chdir(spdir);
			return  -1;
		}

		disp_str = Jfname;
		if  (strchr(Jfname, '/'))  {
			doerror(cw, $E{Unqueue bad file name});
			continue;
		}

		if  (stat(Jfname, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) == S_IFREG)
			break;
		doerror(cw, $E{Unqueue not flat file});
		Exist = Jfname;
	}

	/* Ok now do the business */

	delwin(cw);
#ifdef	CURSES_MEGA_BUG
	clear();
	refresh();
#endif
	chdir(spdir);

	if  ((pid = fork()))  {
		int	status;

		free(Expdir);
		free(Xfname);

		if  (pid < 0)  {
			doerror(jscr, $E{Unqueue no fork});
			return  -1;
		}
#ifdef	HAVE_WAITPID
		while  (waitpid(pid, &status, 0) < 0)
			;
#else
		while  (wait(&status) != pid)
			;
#endif
		if  (status == 0)	/* All ok */
			return  1;
		if  (status & 0xff)  {
			disp_arg[9] = status & 0xff;
			doerror(jscr, $E{Unqueue program fault});
			return  -1;
		}
		status = (status >> 8) & 0xff;
		disp_arg[0] = jp->spq_job;
		disp_str = (char *) jp->spq_file;
		switch  (status)  {
		default:
			disp_arg[1] = status;
			doerror(jscr, $E{Unqueue misc error});
			return  -1;
		case  E_JDFNFND:
			doerror(jscr, $E{Unqueue spool not found});
			return  -1;
		case  E_JDNOCHDIR:
			doerror(jscr, $E{Unqueue dir not found});
			return  -1;
		case  E_JDFNOCR:
			doerror(jscr, $E{Unqueue no create});
			return  -1;
		case  E_JDJNFND:
			doerror(jscr, $E{Unqueue unknown job});
			return  -1;
		}
	}

	setuid(Realuid);
	chdir(Curr_pwd);	/* So it picks up the right config file */
	if  (jp->spq_netid)
		sprintf(jobnobuf, "%s:%ld", look_host(jp->spq_netid), (long) jp->spq_job);
	else
		sprintf(jobnobuf, "%ld", (long) jp->spq_job);
	ap = argbuf;
	*ap++ = (arg0 = strrchr(udprog, '/'))? arg0 + 1: udprog;
	if  (nodelete)
		*ap++ = "-n";
	*ap++ = jobnobuf;
	*ap++ = Expdir;
	*ap++ = Xfname;
	*ap++ = Jfname;
	*ap++ = (char *) 0;
	execv(udprog, argbuf);
	exit(E_SETUP);
}

char	*job_format;
#define	DEFAULT_FORMAT	"%3n %<6N %6u %14h %13f%5Q%5S %3c %3p %14P"
static	char	*bigbuff;
typedef	int	fmt_t;
#include "inline/jfmt_wattn.c"
#include "inline/jfmt_class.c"
#include "inline/jfmt_ppf.c"
#include "inline/jfmt_hold.c"
#include "inline/jfmt_sizek.c"
#include "inline/jfmt_krchd.c"
#include "inline/jfmt_jobno.c"
#include "inline/jfmt_oddev.c"
#include "inline/jfmt_ptr.c"
#include "inline/jfmt_pgrch.c"
#include "inline/jfmt_range.c"
#include "inline/jfmt_szpgs.c"
#include "inline/jfmt_nptim.c"
#include "inline/jfmt_user.c"
#include "inline/jfmt_puser.c"
#include "inline/jfmt_stime.c"
#include "inline/jfmt_mattn.c"
#include "inline/jfmt_cps.c"
#include "inline/jfmt_form.c"
#include "inline/jfmt_title.c"
#include "inline/jfmt_loco.c"
#include "inline/jfmt_mail.c"
#include "inline/jfmt_prio.c"
#include "inline/jfmt_retn.c"
#include "inline/jfmt_supph.c"
#include "inline/jfmt_ptime.c"
#include "inline/jfmt_write.c"
#include "inline/jfmt_origh.c"
#include "inline/jfmt_delim.c"
static	int	jcnt;
#include "inline/jfmt_seq.c"

/* Mapping of format characters (assumed A-Z a-z) and format routines */

#define	NULLCP	(char *) 0

struct	sq_formatdef
	uppertab[] = { /* A-Z */
{	$P{job fmt title}+'A'-1,6,	$K{spq key job wattn},	NULLCP, NULLCP,	fmt_wattn	},	/* A */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* B */
{	$P{job fmt title}+'C'-1,32,	$K{spq key job class},	NULLCP, NULLCP,	fmt_class	},	/* C */
{	$P{job fmt title}+'D'-1,4,	0,			NULLCP, NULLCP,	fmt_delim	},	/* D */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* E */
{	$P{job fmt title}+'F'-1,MAXFLAGS-20,$K{spq key job flags},NULLCP, NULLCP,fmt_ppflags	},	/* F */
{	$P{job fmt title}+'G'-1,10,	$K{spq key job hat},	NULLCP, NULLCP,	fmt_hat		},	/* G */
{	$P{job fmt title}+'H'-1,14,	$K{spq key job hold time},NULLCP, NULLCP, fmt_hold	},	/* H */
{	0,			0,	0,			NULLCP, NULLCP, 0		},	/* I */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* J */
{	$P{job fmt title}+'K'-1,6,	0,			NULLCP, NULLCP,	fmt_sizek	},	/* K */
{	$P{job fmt title}+'L'-1,6,	0,			NULLCP, NULLCP, fmt_kreached	},	/* L */
{	0,			0,	0,			NULLCP, NULLCP, 0		},	/* M */
{	$P{job fmt title}+'N'-1,6,	0,			NULLCP, NULLCP, fmt_jobno	},	/* N */
{	$P{job fmt title}+'O'-1,6,	$K{spq key job oddeven},NULLCP, NULLCP,	fmt_oddeven	},	/* O */
{	$P{job fmt title}+'P'-1,PTRNAMESIZE-4,$K{spq key job printer},NULLCP, NULLCP,fmt_printer},	/* P */
{	$P{job fmt title}+'Q'-1,6,	0,			NULLCP, NULLCP,	fmt_pgreached	},	/* Q */
{	$P{job fmt title}+'R'-1,10,	$K{spq key job range},	NULLCP, NULLCP, fmt_range	},	/* R */
{	$P{job fmt title}+'S'-1,6,	0,			NULLCP, NULLCP, fmt_szpages	},	/* S */
{	$P{job fmt title}+'T'-1,6,	$K{spq key job notp del},NULLCP, NULLCP,fmt_nptime	},	/* T */
{	$P{job fmt title}+'U'-1,UIDSIZE-2,$K{spq key job user},	NULLCP, NULLCP, fmt_puser	},	/* U */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* V */
{	$P{job fmt title}+'W'-1,14,	0,			NULLCP, NULLCP,	fmt_stime	},	/* W */
{	0,			0,	0,			NULLCP, NULLCP, 0		},	/* X */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* Y */
{	0,			0,	0,			NULLCP, NULLCP,	0		}	/* Z */
},
	lowertab[] = { /* a-z */
{	$P{job fmt title}+'a'-1,6,	$K{spq key job mattn},	NULLCP, NULLCP,	fmt_mattn	},	/* a */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* b */
{	$P{job fmt title}+'c'-1,3,	$K{spq key job copies},	NULLCP, NULLCP,	fmt_cps		},	/* c */
{	$P{job fmt title}+'d'-1,2,	0,			NULLCP, NULLCP,	fmt_delimnum	},	/* d */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* e */
{	$P{job fmt title}+'f'-1,MAXFORM-4,$K{spq key job form type},NULLCP, NULLCP,fmt_form	},	/* f */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* g */
{	$P{job fmt title}+'h'-1,MAXTITLE-6,$K{spq key job header},NULLCP, NULLCP,fmt_title	},	/* h */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* i */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* j */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* k */
{	$P{job fmt title}+'l'-1,6,	$K{spq key job loco},	NULLCP, NULLCP,	fmt_localonly	},	/* l */
{	$P{job fmt title}+'m'-1,6,	$K{spq key job mail},	NULLCP, NULLCP,	fmt_mail	},	/* m */
{	$P{job fmt title}+'n'-1,3,	0,			NULLCP, NULLCP,	fmt_seq		},	/* n */
{	$P{job fmt title}+'o'-1,HOSTNSIZE-6,0,			NULLCP, NULLCP, fmt_orighost	},	/* o */
{	$P{job fmt title}+'p'-1,3,	$K{spq key job priority},NULLCP, NULLCP,fmt_prio	},	/* p */
{	$P{job fmt title}+'q'-1,6,	$K{spq key job retain},	NULLCP, NULLCP,	fmt_retain	},	/* q */
{	0,			0,	0,			NULLCP, NULLCP, 0		},	/* r */
{	$P{job fmt title}+'s'-1,6,	$K{spq key job noh},	NULLCP, NULLCP, fmt_supph	},	/* s */
{	$P{job fmt title}+'t'-1,6,	$K{spq key job pdel},	NULLCP, NULLCP, fmt_ptime	},	/* t */
{	$P{job fmt title}+'u'-1,UIDSIZE-2,0,			NULLCP, NULLCP,	fmt_user	},	/* u */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* v */
{	$P{job fmt title}+'w'-1,6,	$K{spq key job write},	NULLCP, NULLCP,	fmt_write	},	/* w */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* x */
{	0,			0,	0,			NULLCP, NULLCP,	0		},	/* y */
{	0,			0,	0,			NULLCP, NULLCP,	0		}	/* z */
};

static int	upd_copies(const int row, struct sctrl *scp)
{
	int	num;
	if  (!(mypriv->spu_flgs & PV_ANYPRIO))
		scp->vmax = mypriv->spu_cps;
	num = chk_wnum(jscr, row, scp, (LONG) JREQ.spq_cps, 3);
	if  (num >= 0  &&  num != (int) JREQ.spq_cps)  {
		JREQ.spq_cps = num;
		return  -1;
	}
	return  0;
}

static int	upd_prio(const int row, struct sctrl *scp)
{
	int	num;
	if  (!(mypriv->spu_flgs & PV_ANYPRIO))  {
		scp->vmax = mypriv->spu_maxp;
		scp->min = mypriv->spu_minp;
	}
	if  (!(mypriv->spu_flgs & PV_CPRIO))
		return  $E{spq cannot reprio};
	num = chk_wnum(jscr, row, scp, (LONG) JREQ.spq_pri, 3);
	if  (num > 0  &&  num != (int) JREQ.spq_pri)  {
		JREQ.spq_pri = (unsigned char) num;
		return  -1;
	}
	return  0;
}

static int	upd_printer(const int row, struct sctrl *scp)
{
	char	*str = chk_wgets(jscr, row, scp, JREQ.spq_ptr, JPTRNAMESIZE);
	if  (!str)
		return  0;
	if  (!((mypriv->spu_flgs & PV_OTHERP)  ||  issubset(mypriv->spu_ptrallow, str)))  {
		if  (scp->col >= 0)
			ws_fill(jscr, row, scp, JREQ.spq_ptr);
		disp_str = mypriv->spu_ptrallow;
		return  $E{spq cannot select ptrs};
	}
	strncpy(JREQ.spq_ptr, str, JPTRNAMESIZE);
	return  -1;
}

static int	upd_form(const int row, struct sctrl *scp)
{
	char	*str = chk_wgets(jscr, row, scp, JREQ.spq_form, MAXFORM);
	if  (!str)
		return  0;
	if  (!((mypriv->spu_flgs & PV_FORMS)  ||  qmatch(mypriv->spu_formallow, str)))  {
		if  (scp->col >= 0)
			ws_fill(jscr, row, scp, JREQ.spq_form);
		disp_str = mypriv->spu_formallow;
		return  $E{spq cannot select forms};
	}
	strncpy(JREQ.spq_form, str, MAXFORM);
	return  -1;
}

static int	upd_title(const int row, struct sctrl *scp)
{
	char	*str = chk_wgets(jscr, row, scp, JREQ.spq_file, MAXTITLE);
	if  (str)  {
		strncpy(JREQ.spq_file, str, MAXTITLE);
		return  -1;
	}
	return  0;
}

static int	upd_hold(const int row, struct sctrl *scp)
{
	WINDOW	*awin;
	char	*prmpt;
	int	nrow, ncol, ret;

	getbegyx(jscr, nrow, ncol);
	if  (!(awin = newwin(1, 0, nrow + row, 0)))
		return  0;
	prmpt = gprompt(scp->helpcode);
	waddstr(awin, prmpt);
	free(prmpt);
	getyx(awin, nrow, ncol);
	tdisplay(awin, (time_t) JREQ.spq_hold, nrow, ncol);
	ret = wtime(awin, nrow, ncol);
	delwin(awin);
	touchwin(jscr);
	return  ret != 0? -1: 0;
}

static int	upd_npdel(const int row, struct sctrl *scp)
{
	int	num = wnum(jscr, row, scp, (LONG) JREQ.spq_nptimeout);
	if  (num > 0  &&  num != (int) JREQ.spq_nptimeout)  {
		JREQ.spq_nptimeout = (USHORT) num;
		return  -1;
	}
	return  0;
}

static int	upd_pdel(const int row, struct sctrl *scp)
{
	int	num = wnum(jscr, row, scp, (LONG) JREQ.spq_ptimeout);
	if  (num > 0  &&  num != (int) JREQ.spq_ptimeout)  {
		JREQ.spq_ptimeout = (USHORT) num;
		return  -1;
	}
	return  0;
}

static int gbool(WINDOW *awin, const int row, const int col, struct sctrl *scp, const int existing)
{
	int	ch;

	wmove(awin, row, col);
	wrefresh(awin);

	for  (;;)  {
		do  ch = getkey(MAG_A|MAG_P);
		while  (ch == EOF  &&  (hlpscr || escr));
		if  (hlpscr)  {
			endhe(awin, &hlpscr);
			if  (helpclr)
				continue;
		}
		if  (escr)
			endhe(awin, &escr);

		switch  (ch)  {
		default:
			doerror(awin, $E{spq set boolean state});

		case  EOF:
			continue;

		case  $K{key help}:
			dochelp(awin, scp->helpcode);
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			wrefresh(awin);
			continue;

		case  $K{key yes}:
			return  1;

		case  $K{key no}:
			return  0;

		case  $K{key toggle}:
			return  !existing;

		case  $K{key halt}:
		case  $K{key eol}:
		case  $K{key erase}:
			return  existing;
		}
	}
}

static int	upd_bool(const int row, struct sctrl *scp)
{
	WINDOW	*awin;
	char	*prmpt;
	int	nrow, ncol, ret;
	USHORT	wbit = (USHORT) scp->vmax;
	USHORT	curr = (USHORT) JREQ.spq_jflags & wbit;

	getbegyx(jscr, nrow, ncol);
	if  (!(awin = newwin(1, 0, nrow + row, 0)))
		return  0;
	prmpt = gprompt(scp->helpcode);
	waddstr(awin, prmpt);
	free(prmpt);
	getyx(awin, nrow, ncol);
	waddstr(awin, curr? yesmsg: nomsg);
	select_state($S{spq set boolean state});
	ret = gbool(awin, nrow, ncol, scp, curr);
	delwin(awin);
	touchwin(jscr);
	if  (ret)  {
		if  (curr)
			return  0;
		JREQ.spq_jflags |= wbit;
		return  -1;
	}
	if  (!curr)
		return  0;
	JREQ.spq_jflags &= ~wbit;
	return  -1;
}

static int	upd_class(const int row, struct sctrl *scp)
{
	classcode_t  in = whexnum(jscr, row, scp, JREQ.spq_class);
	if  (in == JREQ.spq_class)
		return  0;
	if  (!(mypriv->spu_flgs & PV_COVER))
		in &= mypriv->spu_class;
	if  (in == 0L)  {
		mvwaddstr(jscr, row, scp->col, hex_disp(JREQ.spq_class, 1));
		disp_str = hex_disp(mypriv->spu_class, 0);
		return  $E{spq set zero class};
	}
	JREQ.spq_class = in;
	return  -1;
}

static int	upd_range(const int row, struct sctrl *scp)
{
	WINDOW	*awin;
	char	*prmpt;
	int	nrow, ncol;
	LONG	start, end;
	char	cbuf[30];

	getbegyx(jscr, nrow, ncol);
	if  (!(awin = newwin(1, 0, nrow + row, 0)))
		return  0;
	prmpt = gprompt(scp->helpcode);
	waddstr(awin, prmpt);
	free(prmpt);
	getyx(awin, nrow, ncol);
	scp->size = COLS - ncol - 1;
	scp->col = ncol;
	if  (JREQ.spq_start != 0)
		if  (JREQ.spq_end < LOTSANDLOTS)
			sprintf(cbuf, "%ld-%ld", JREQ.spq_start+1L, JREQ.spq_end+1L);
		else
			sprintf(cbuf, "%ld-", JREQ.spq_start+1L);
	else  if  (JREQ.spq_end < LOTSANDLOTS)
		sprintf(cbuf, "-%ld", JREQ.spq_end+1L);
	else
		cbuf[0] = '\0';
	for  (;;)  {
		mvwaddstr(awin, nrow, ncol, cbuf);
		start = JREQ.spq_start;
		end = JREQ.spq_end;
		if  (!(prmpt = wgets(awin, nrow, scp, cbuf)))
			break;
		start = 0;
		if  (*prmpt == '\0')  {
			end = LOTSANDLOTS;
			break;
		}
		if  (isdigit(*prmpt))  {
			do	start = start * 10 + *prmpt++ - '0';
			while  (isdigit(*prmpt));
			start--;
		}
		if  (start < 0  ||  *prmpt++ != '-')  {
		badrange:
			doerror(jscr, $E{spq bad range format}); /* Might not get cleared??? */
			continue;
		}
		if  (*prmpt == '\0')  {
			end = LOTSANDLOTS;
			break;
		}
		end = 0;
		while  (isdigit(*prmpt))
			end = end * 10 + *prmpt++ - '0';
		end--;
		if  (end < 0  ||  *prmpt)
			goto  badrange;
		if  (start <= end)
			break;
		doerror(jscr, $E{end page less than start page});
	}
	delwin(awin);
	if  (start == JREQ.spq_start  &&  end == JREQ.spq_end)
		return  0;
	JREQ.spq_start = start;
	JREQ.spq_end = end;
	return  -1;
}

static int	upd_hat(const int row, struct sctrl *scp)
{
	LONG	num;
	if  (JREQ.spq_haltat == 0)		/* Just ignore it */
		return  0;
	num = wnum(jscr, row, scp, JREQ.spq_haltat+1L);
	if  (num < 0)
		return  0;
	num--;
	if  (num == JREQ.spq_haltat)
		return  0;
	if  (num > JREQ.spq_end)  {
		wn_fill(jscr, row, scp, JREQ.spq_haltat+1L);
		return  $E{end page less than haltat page};
	}
	JREQ.spq_haltat = num;
	return  -1;
}

static int	upd_oddeven(const int row, struct sctrl *scp)
{
	WINDOW	*awin;
	char	*prmpt;
	int	orow, ocol, erow, ecol, rrow, rcol, yesnlng;
	USHORT	curr = JREQ.spq_jflags & (SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);
	USHORT	new = 0;

	yesnlng = strlen(yesmsg);
	if  ((orow = strlen(nomsg)) > yesnlng)
		yesnlng = orow;
	getbegyx(jscr, rrow, rcol);
	if  (!(awin = newwin(1, 0, rrow + row, 0)))
		return  0;
	prmpt = gprompt($P{spq oe odd});
	waddstr(awin, prmpt);
	free(prmpt);
	getyx(awin, orow, ocol);
	wprintw(awin, "%-*s", yesnlng, curr & SPQ_ODDP? yesmsg: nomsg);
	prmpt = gprompt($P{spq oe even});
	waddstr(awin, prmpt);
	free(prmpt);
	getyx(awin, erow, ecol);
	wprintw(awin, "%-*s", yesnlng, curr & SPQ_EVENP? yesmsg: nomsg);
	prmpt = gprompt($P{spq oe reverse});
	waddstr(awin, prmpt);
	free(prmpt);
	getyx(awin, rrow, rcol);
	wprintw(awin, "%-*s", yesnlng, curr & SPQ_REVOE? yesmsg: nomsg);
	select_state($S{spq set boolean state});
	if  (gbool(awin, orow, ocol, scp, curr & SPQ_ODDP))  {
		new |= SPQ_ODDP;
		if  (curr & SPQ_EVENP)  {
			curr &= ~SPQ_EVENP;
			mvwprintw(awin, erow, ecol, "%-*s", yesnlng, nomsg);
		}
	}
	if  ((curr ^ new) & SPQ_ODDP)
		mvwprintw(awin, orow, ocol, "%-*s", yesnlng, new & SPQ_ODDP? yesmsg: nomsg);
	if  (gbool(awin, erow, ecol, scp, curr & SPQ_EVENP))  {
		new |= SPQ_EVENP;
		if  (new & SPQ_ODDP)  {
			new &= ~SPQ_ODDP;
			mvwprintw(awin, orow, ocol, "%-*s", yesnlng, nomsg);
		}
	}
	if  ((curr ^ new) & SPQ_EVENP)
		mvwprintw(awin, erow, ecol, "%-*s", yesnlng, new & SPQ_EVENP? yesmsg: nomsg);
	if  (gbool(awin, rrow, rcol, scp, curr & SPQ_REVOE)  &&  new & (SPQ_ODDP|SPQ_EVENP))
		new |= SPQ_REVOE;
	delwin(awin);
	touchwin(jscr);
	if  ((JREQ.spq_jflags & (SPQ_ODDP|SPQ_EVENP|SPQ_REVOE)) != new)  {
		JREQ.spq_jflags &= ~(SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);
		JREQ.spq_jflags |= new;
		return  -1;
	}
	return  0;
}

static int	upd_postuser(const int row, struct sctrl *scp)
{
	char	*str = wgets(jscr, row, scp, JREQ.spq_puname);
	if  (!str)
		return  0;
	if  (lookup_uname(str) == UNKNOWN_UID)  {
		ws_fill(jscr, row, scp, JREQ.spq_puname);
		disp_str = str;
		return  $E{Unknown user in spq};
	}
	strncpy(JREQ.spq_puname, str, UIDSIZE);
	return  -1;
}

static int	upd_flags(const int row, struct sctrl *scp)
{
	char	*str = wgets(jscr, row, scp, JREQ.spq_flags);
	if  (str)  {
		strncpy(JREQ.spq_flags, str, MAXFLAGS);
		return  -1;
	}
	return  0;
}

/* Job attribute keys for building stuff up with.  */

struct	attrib_key	job_attribs[] = {
{	$PH{spq number copies},	HELPLESS, -1, 0, MAG_P, 0L, 255L, upd_copies	},	/* copies */
{	$PH{spq enter priority},HELPLESS, -1, 0, MAG_P, 1L, 255L, upd_prio	},	/* priority */
{	$PH{spq printer help},	wotjprin, -1, 0, MAG_OK|MAG_P|MAG_LONG, 0L, 0L, upd_printer},	/* printer */
{	$PH{spq form help},	wotjform, -1, 0, MAG_P|MAG_NL|MAG_FNL|MAG_LONG, 0L, 0L, upd_form},/* form type */
{	$PH{spq title help},	HELPLESS, -1, 0, MAG_OK|MAG_LONG, 0L, 0L, upd_title},	/* header */

/* After this point we don't ever allow updates if they aren't displayed */

{	$PH{Qopt hold time},	HELPLESS, -1, 0, 0, 0L, 0L, upd_hold		},	/* hold time */
{	$H{Qopt del not printed}, HELPLESS, -1, 0, 0, 1L, 0xffffL, upd_npdel	},	/* notp del */
{	$H{Qopt del printed},	HELPLESS, -1, 0, 0, 1L, 0xffffL, upd_pdel	},	/* pdel */
{	$PH{Qopt retn},		HELPLESS, -1, 0, 0, 0L, (LONG) SPQ_RETN, upd_bool},	/* retain */
{	$PH{Qopt nohdr},	HELPLESS, -1, 0, 0, 0L, (LONG) SPQ_NOH, upd_bool},	/* noh */
{	$PH{Qopt write msg},	HELPLESS, -1, 0, 0, 0L, (LONG) SPQ_WRT, upd_bool},	/* write */
{	$PH{Qopt mail msg},	HELPLESS, -1, 0, 0, 0L, (LONG) SPQ_MAIL, upd_bool},	/* mail */
{	$PH{Qopt wattn},	HELPLESS, -1, 0, 0, 0L, (LONG) SPQ_WATTN, upd_bool},	/* wattn */
{	$PH{Qopt mattn},	HELPLESS, -1, 0, 0, 0L, (LONG) SPQ_MATTN, upd_bool},	/* mattn */
{	$PH{Qopt local only},	HELPLESS, -1, 0, 0, 0L, (LONG) SPQ_LOCALONLY, upd_bool},/* loco */
{	$H{Qopt class},		HELPLESS, -1, 0, MAG_P, 1L, U_MAX_CLASS, upd_class},	/* class */
{	$PH{spq range help},	HELPLESS, -1, 0, MAG_OK|MAG_P, 0L, 0L, upd_range},	/* range */
{	$H{spq hat help},	HELPLESS, -1, 0, MAG_P, 1L, LOTSANDLOTS, upd_hat},	/* hat */
{	$H{spq oddeven help},	HELPLESS, -1, 0, 0, 0L, 0L, upd_oddeven		},	/* oddeven */
{	$H{Qopt post user},	gen_ulist,-1, 0, MAG_P|MAG_NL|MAG_FNL, 0L, 0L, upd_postuser},	/* user */
{	$H{Qopt flags},		HELPLESS, -1, 0, MAG_OK, 0L, 0L, upd_flags	}	/* flags */
};

#ifdef	HAVE_TERMINFO
#define	DISP_CHAR(w, ch)	waddch(w, (chtype) ch);
#else
#define	DISP_CHAR(w, ch)	waddch(w, ch);
#endif

char *	get_jobtitle(int nopage)
{
	int	nn, obuflen, isrjust;
	struct	sq_formatdef	*fp;
	char	*cp, *rp, *result, *mp;

	if  (job_format)  {	/* Done before, reset "col" fields */
		struct	attrib_key  *atp;
		for  (atp = job_attribs;  atp < &job_attribs[sizeof(job_attribs)/sizeof(struct attrib_key)];  atp++)
			atp->col = -1;
	}
	else  if  (!(job_format = helpprmpt($P{Spq job default fmt}+nopage)))
		job_format = stracpy(DEFAULT_FORMAT);

	/* Initial pass to discover how much space to allocate */

	obuflen = 1;
	cp = job_format;
	while  (*cp)  {
		if  (*cp++ != '%')  {
			obuflen++;
			continue;
		}
		if  (*cp == '<' || *cp == '>')
			cp++;
		nn = 0;
		do  nn = nn * 10 + *cp++ - '0';
		while  (isdigit(*cp));
		obuflen += nn;
		if  (isalpha(*cp))
			cp++;
	}

	/* Allocate space for title and result */

	result = malloc((unsigned) obuflen);
	if  (bigbuff)
		free(bigbuff);
	bigbuff = malloc(4 * obuflen);
	if  (!result || !bigbuff)
		nomem();

	/* Now set up title.
	   Actually this is a waste of time if we aren't actually displaying
	   same, but we needed the buffer.  */

	rp = result;
	cp = job_format;
	while  (*cp)  {
		if  (*cp != '%')  {
			*rp++ = *cp++;
			continue;
		}
		cp++;

		/* Get width */

		if  (*cp == '<' || *cp == '>')
			cp++;
		nn = 0;
		do  nn = nn * 10 + *cp++ - '0';
		while  (isdigit(*cp));

		/* Get format char */

		if  (isupper(*cp))
			fp = &uppertab[*cp - 'A'];
		else  if  (islower(*cp))
			fp = &lowertab[*cp - 'a'];
		else  {
			if  (*cp)
				cp++;
			continue;
		}
		switch  (*cp++)  {
		default:
			isrjust = 0;
			break;
		case  'G':	/* Halt at */
		case  'K':	/* Size in K */
		case  'N':	/* Jobnum */
		case  'S':	/* Size in pages */
		case  'T':	/* Nptime */
		case  'c':	/* Copies */
		case  'p':	/* Prio */
		case  't':	/* Ptime */
			isrjust = 1;
			break;
		}
		if  (fp->keycode)  {
			struct	attrib_key  *atp = &job_attribs[fp->keycode - $K{spq key job copies}];
			atp->col = (SHORT) (rp - result);
			atp->size = (USHORT) nn;
		}
		if  (fp->statecode == 0)
			continue;

		/* Get title message if we don't have it
		   Insert into result */

		if  (!fp->msg)
			fp->msg = gprompt(fp->statecode);

		mp = fp->msg;
		if  (isrjust)  {
			int	lng = strlen(mp);
			while  (lng < nn)  {
				*rp++ = ' ';
				nn--;
			}
		}
		while  (nn > 0  &&  *mp)  {
			*rp++ = *mp++;
			nn--;
		}
		while  (nn > 0)  {
			*rp++ = ' ';
			nn--;
		}
	}

	/* Trim trailing spaces */

	for  (rp--;  rp >= result  &&  *rp == ' ';  rp--)
		;
	*++rp = '\0';

	return  result;
}

/* Display contents of job file.  */

void	jdisplay(void)
{
	int	row;

	werase(jscr);

	/* Better not let too big a gap develop....  */

	more_above = 0;

	if  (Jhline < - MAXSTEP)
		Jhline = 0;

	if  ((jcnt = Jhline) < 0)  {
		row = - Jhline;
		jcnt = 0;
	}
	else
		row = 0;

	if  (Jhline > 0)  {
		if  (Jhline <= 1)
			jcnt = Jhline = 0;
		else  {
			wstandout(jscr);
			mvwprintw(jscr,
					 row,
					 (COLS - (int) strlen(more_amsg))/2,
					 more_amsg,
					 Jhline);
			wstandend(jscr);
			row++;
			more_above = 1;
		}
	}

	more_below = 0;

	for  (;  jcnt < Job_seg.njobs  &&  row < JLINES;  jcnt++, row++)  {
		const  struct  spq  *jp = &Job_seg.jj_ptrs[jcnt]->j;
		struct	sq_formatdef  *fp;
		char	*cp = job_format, *lbp;
		int	currplace = -1, lastplace, nn, inlen, dummy;

		if  (jp->spq_job == 0)
			break;

		if  (row == JLINES - 1  &&  jcnt < Job_seg.njobs - 1) {
			wstandout(jscr);
			mvwprintw(jscr,
					 row,
					 (COLS - (int) strlen(more_bmsg)) / 2,
					 more_bmsg,
					 Job_seg.njobs - jcnt);
			wstandend(jscr);
			more_below = 1;
			if  (Jeline >= jcnt)
				Jeline = jcnt - 1;
			break;
		}

		wmove(jscr, row, 0);

		while  (*cp)  {
			if  (*cp != '%')  {
				DISP_CHAR(jscr, *cp);
				cp++;
				continue;
			}
			cp++;
			lastplace = -1;
			if  (*cp == '<')  {
				lastplace = currplace;
				cp++;
			}
			else  if  (*cp == '>')
				cp++;
			nn = 0;
			do  nn = nn * 10 + *cp++ - '0';
			while  (isdigit(*cp));

			/* Get format char */

			if  (isupper(*cp))
				fp = &uppertab[*cp - 'A'];
			else  if  (islower(*cp))
				fp = &lowertab[*cp - 'a'];
			else  {
				if  (*cp)
					cp++;
				continue;
			}
			cp++;
			if  (!fp->fmt_fn)
				continue;
			getyx(jscr, dummy, currplace);
			inlen = (fp->fmt_fn)(jp, nn);
			lbp = bigbuff;
			if  (inlen > nn  &&  lastplace >= 0)  {
				wmove(jscr, row, lastplace);
				nn = currplace + nn - lastplace;
				inlen = (fp->fmt_fn)(jp, nn);
			}
			while  (inlen > 0  &&  nn > 0)  {
				DISP_CHAR(jscr, *lbp);
				lbp++;
				inlen--;
				nn--;
			}
			if  (nn > 0)  {
				int	ccol;
				getyx(jscr, dummy, ccol);
				wmove(jscr, dummy, ccol+nn);
			}
		}
	}
#ifdef	CURSES_OVERLAP_BUG
	touchwin(jscr);
#endif
}

static	void	abortjob(const slotno_t jslot)
{
	static	char	*cnfmsgp, *cnfmsgnp;
	static	WINDOW	*awin;

	if  (confabort > 1 || (confabort && !(JREQ.spq_dflags & SPQ_PRINTED)))  {
		int	begy, y, x;
		getbegyx(jscr, begy, x);
		getyx(jscr, y, x);
		if  (!cnfmsgp)  {
			cnfmsgnp = gprompt($P{Not printed sure del});
			cnfmsgp = gprompt($P{spq confirm delete});
			awin = newwin(1, 0, begy + y, 0);
		}
		else  {
			mvwin(awin, begy + y, 0);
			werase(awin);
		}
		wprintw(awin, JREQ.spq_dflags & SPQ_PRINTED? cnfmsgp: cnfmsgnp, JREQ.spq_job);
		wrefresh(awin);
		select_state($S{spq confirm delete});
		Ew = jscr;

		for  (;;)  {
			do  x = getkey(MAG_A|MAG_P);
			while  (x == EOF && (hlpscr || escr));

			if  (hlpscr)  {
				endhe(jscr, &hlpscr);
				touchwin(awin);
				wrefresh(awin);
				if  (helpclr)
					continue;
			}
			if  (escr)  {
				endhe(jscr, &escr);
				touchwin(awin);
				wrefresh(awin);
			}
			if  (x == $K{key help})  {
				dochelp(awin, $H{spq confirm delete});
				continue;
			}
			if  (x == $K{key refresh})  {
				wrefresh(curscr);
				wrefresh(awin);
				continue;
			}
			if  (x == $K{key yes}  ||  x == $K{key no})
				break;
			doerror(awin, $E{spq confirm delete});
		}
		touchwin(jscr);
		wrefresh(jscr);
		if  (x == $K{key no})
			return;
	}
	OREQ = jslot;
	womsg(SO_AB);
}

/* Spit out a prompt for a search string */

static	char *	gsearchs(const int isback)
{
	int	row;
	char	*gstr;
	struct	sctrl	ss;
	static	char	*lastmstr;
	static	char	*sforwmsg, *sbackwmsg;

	if  (!sforwmsg)  {
		sforwmsg = gprompt($P{Fsearch job});
		sbackwmsg = gprompt($P{Rsearch job});
	}

	ss.helpcode = $H{Fsearch job};
	gstr = isback? sbackwmsg: sforwmsg;
	ss.helpfn = HELPLESS;
	ss.size = 30;
	ss.retv = 0;
	ss.col = (SHORT) strlen(gstr);
	ss.magic_p = MAG_OK;
	ss.min = 0L;
	ss.vmax = 0L;
	ss.msg = NULLCP;
	row = Jeline - Jhline + more_above;
	mvwaddstr(jscr, row, 0, gstr);
	wclrtoeol(jscr);

	if  (lastmstr)  {
		ws_fill(jscr, row, &ss, lastmstr);
		gstr = wgets(jscr, row, &ss, lastmstr);
		if  (!gstr)
			return  NULLCP;
		if  (gstr[0] == '\0')
			return  lastmstr;
	}
	else  {
		for  (;;)  {
			gstr = wgets(jscr, row, &ss, "");
			if  (!gstr)
				return  NULLCP;
			if  (gstr[0])
				break;
			doerror(jscr, $E{Rsearch job});
		}
	}
	if  (lastmstr)
		free(lastmstr);
	return  lastmstr = stracpy(gstr);
}

/* Match a job string "vstr" against a pattern string "mstr" */

static	int	smatchit(const char *vstr, const char *mstr)
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

static	int	smatch(const int mline, const char *mstr)
{
	const  struct  spq  *jp = &Job_seg.jj_ptrs[mline]->j;
	return  smatchit(jp->spq_file, mstr) ||
		smatchit(jp->spq_form, mstr) ||
		smatchit(jp->spq_ptr, mstr) ||
		smatchit(jp->spq_uname, mstr);
}

/* Search for string in job title/printer/form/user
   Return 0 - need to redisplay jobs (Jhline and Jeline suitably mangled)
   otherwise return error code */

static	int	dosearch(const int isback)
{
	char	*mstr = gsearchs(isback);
	int	mline;

	if  (!mstr)
		return  0;

	if  (isback)  {
		for  (mline = Jeline - 1;  mline >= 0;  mline--)
			if  (smatch(mline, mstr))
				goto  gotit;
		for  (mline = Job_seg.njobs - 1;  mline >= Jeline;  mline--)
			if  (smatch(mline, mstr))
				goto  gotit;
	}
	else  {
		for  (mline = Jeline + 1;  (unsigned) mline < Job_seg.njobs;  mline++)
			if  (smatch(mline, mstr))
				goto  gotit;
		for  (mline = 0;  mline <= Jeline;  mline++)
			if  (smatch(mline, mstr))
				goto  gotit;
	}
	return  $E{Search job not found};

 gotit:
	Jeline = mline;
	if  (Jeline < Jhline  ||  Jeline - Jhline + more_above + more_below >= JLINES)
		Jhline = Jeline;
	return  0;
}

static void	job_macro(const struct spq *jp, const int num)
{
	char	*prompt = helpprmpt(num + $P{Job or User macro}), *str;
	static	char	*execprog;
	PIDTYPE	pid;
	int	status, refreshscr = 0;
#ifdef	HAVE_TERMIOS_H
	struct	termios	save;
	extern	struct	termios	orig_term;
#else
	struct	termio	save;
	extern	struct	termio	orig_term;
#endif

	if  (!prompt)  {
		disp_arg[0] = num;
		doerror(jscr, $E{Macro error});
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
		int	jsy, jsx;
		struct	sctrl	dd;
		wclrtoeol(jscr);
		waddstr(jscr, str);
		getyx(jscr, jsy, jsx);
		dd.helpcode = $H{Job or User macro};
		dd.helpfn = HELPLESS;
		dd.size = COLS - jsx;
		dd.col = jsx;
		dd.magic_p = MAG_P|MAG_OK;
		dd.min = dd.vmax = 0;
		dd.msg = (char *) 0;
		str = wgets(jscr, jsy, &dd, "");
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
		char	nbuf[20+HOSTNSIZE];
		char	*argbuf[3];
		argbuf[0] = str;
		if  (jp)  {
			if  (jp->spq_netid)
				sprintf(nbuf, "%s:%ld", look_host(jp->spq_netid), (long) jp->spq_job);
			else
				sprintf(nbuf, "%ld", (long) jp->spq_job);
			argbuf[1] = nbuf;
			argbuf[2] = (char *) 0;
		}
		else
			argbuf[1] = (char *) 0;
		if  (!refreshscr)  {
			close(0);
			close(1);
			close(2);
			dup(dup(open("/dev/null", O_RDWR)));
		}
		chdir(Curr_pwd);
		execv(execprog, argbuf);
		exit(255);
	}
	free(prompt);
	if  (pid < 0)  {
		doerror(jscr, $E{Macro fork failed});
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
			doerror(jscr, $E{Macro command gave signal});
		}
		else  {
			disp_arg[0] = (status >> 8) & 255;
			doerror(jscr, $E{Macro command error});
		}
	}
}

/* This accepts input from the screen.  */

int	j_process(void)
{
	int	state, err_no, i;
	int	ch, currow;
	char	*str;
	struct	attrib_key	*atp;
	struct	sctrl	opjob;

	Ew = jscr;

	/* Set up key state according to whether we've got access to
	   the print queue or not.  */

	state = $SH{spq job state no ptr};

	if  (PLINES > 0)  {
		if  (mypriv->spu_flgs & PV_PRINQ)
			state = $SH{spq job state};
#ifdef HAVE_TERMINFO
		wnoutrefresh(pscr);
#else
		wrefresh(pscr);
#endif
	}
	select_state(state);

Jmove:
	currow = Jeline - Jhline + more_above;
	wmove(jscr, currow, 0);
	Cjobno = Job_seg.njobs != 0? Job_seg.jj_ptrs[Jeline]->j.spq_job: -1;

Jrefresh:

#ifdef HAVE_TERMINFO
	wnoutrefresh(jscr);
	doupdate();
#else
	wrefresh(jscr);
#endif

nextin:
	if  (hadrfresh)
		return  -1;
	if  (hadalarm != lastalarm)  {
		hadalarm = lastalarm;
		if  (pscr  &&  Ptr_seg.dptr->ps_serial != Ptr_seg.Last_ser)  {
			rpfile();
			pdisplay();
			readjoblist(1);
			jdisplay();
#ifdef	HAVE_TERMINFO
			wnoutrefresh(pscr);
#else
			wrefresh(pscr);
#endif
			goto  Jmove;
		}
		if  (Job_seg.dptr->js_serial != Job_seg.Last_ser)  {
			readjoblist(1);
			jdisplay();
			goto  Jmove;
		}
	}

 nextin2:
	do  ch = getkey(MAG_A|MAG_P);
	while  (ch == EOF  &&  (hlpscr || escr));

	if  (hlpscr)  {
		endhe(jscr, &hlpscr);
		if  (helpclr)
			goto  nextin2;
	}
	if  (escr)
		endhe(jscr, &escr);

	switch  (ch)  {
	case  EOF:
		goto  nextin;

	/* Error case - bell character and try again.  */

	default:
	unknownc:
		err_no = $E{spq unknown command};
	err:
		doerror(jscr, err_no);
		goto  nextin;

	case  $K{key help}:
		dochelp(jscr, state);
		goto  nextin;

	case  $K{key refresh}:
		wrefresh(curscr);
		goto  Jrefresh;

	/* Move up or down.  */

	case  $K{key cursor down}:
		Jeline++;
		if  (Jeline >= Job_seg.njobs)  {
			Jeline--;
bj:			err_no = $E{spq job off bottom};
			goto  err;
		}

		if  (++currow >= JLINES - more_below) {
			Jhline++;
			if  (!more_above)
				Jhline++;
			jdisplay();
		}
		goto  Jmove;

	case  $K{key cursor up}:
		if  (Jeline <= 0)  {
ej:			err_no = $E{spq job off top};
			goto  err;
		}
		Jeline--;
		if  (--currow < more_above) {
			Jhline = Jeline;
			if  (more_above  &&  Jeline == 1)
				Jhline = 0;
			jdisplay();
		}
		goto  Jmove;

	/* Half/Full screen up/down */

	case  $K{key screen down}:
		if  (Jhline + JLINES - more_above >= Job_seg.njobs)
			goto  bj;
		Jhline += JLINES - more_above - more_below;
		Jeline += JLINES - more_above - more_below;
		if  (Jeline >= Job_seg.njobs)
			Jeline = Job_seg.njobs - 1;
	redr:
		jdisplay();
		currow = Jeline - Jhline + more_above;
		if  (more_below  &&  currow > JLINES-2)
			Jeline--;
		goto  Jmove;

	case  $K{key half screen down}:
		i = (JLINES - more_above - more_below) / 2;
		if  (Jhline + i >= Job_seg.njobs)
			goto  bj;
		Jhline += i;
		if  (Jeline < Jhline)
			Jeline = Jhline;
		goto  redr;

	case  $K{key half screen up}:
		if  (Jhline <= 0)
			goto  ej;
		Jhline -= (JLINES - more_above - more_below) / 2;
	restu:
		if  (Jhline == 1)
			Jhline = 0;
		jdisplay();
		if  (Jeline - Jhline >= JLINES - more_above - more_below)
			Jeline = Jhline + JLINES - more_above - more_below - 1;
		goto  Jmove;

	case  $K{key screen up}:
		if  (Jhline <= 0)
			goto  ej;
		Jhline -= JLINES - more_above - more_below;
		goto  restu;

	case  $K{key top}:
		if  (Jhline != Jeline  &&  Jeline != 0)  {
			Jeline = Jhline < 0? 0: Jhline;
			goto  Jmove;
		}
		Jhline = 0;
		Jeline = 0;
		goto  redr;

	case  $K{key bottom}:
		ch = Jhline + JLINES - more_above - more_below - 1;
		if  (Jeline < ch  &&  ch < Job_seg.njobs - 1)  {
			Jeline = ch;
			goto  Jmove;
		}
		if  (Job_seg.njobs > JLINES)
			Jhline = Job_seg.njobs - JLINES + 1;
		else
			Jhline = 0;
		Jeline = Job_seg.njobs - 1;
		goto  redr;

	case  $K{key forward search}:
	case  $K{key backward search}:
		if  ((err_no = dosearch(ch == $K{key backward search})) != 0)
			doerror(jscr, err_no);
		goto  redr;

		/* Go to other window.
		   We physically cannot get this character if we're in state 2.  */

	case  $K{spq key other window}:
		if  (state != $H{spq job state})
			goto  unknownc;
		return  1;

		/* Go home.  */

	case  $K{key halt}:
		return  0;

	case  $K{spq key view sys err}:
		if  ((ch = view_errors(1)) > 0)
			goto  refill;
		err_no = ch < 0? $E{Log file nomem}: $E{No log file yet};
		goto  err;

	case  $K{key save opts}:
		propts();
		goto  refill;

	case  $K{spq key job format}:
		if  (!(mypriv->spu_flgs & PV_ACCESSOK))
			goto  noacc;
		err_no = fmtprocess(&job_format, 'X', uppertab, lowertab);
		str = get_jobtitle(nopage);
		if  (wh_jtitline >= 0)  {
			wmove(hjscr, wh_jtitline, 0);
			wclrtoeol(hjscr);
			waddstr(hjscr, str);
		}
		free(str);
		if  (err_no)	/* Records changes */
			offersave(job_format, $P{Spq job default fmt}+nopage);
		jdisplay();
		goto  refill;

		/* And now for the stuff to change things.
		   It must be own job unless privileged.  */

	case  $K{spq key view job}:
		if  (Jeline >= Job_seg.njobs)  {
			err_no = $E{spq no jobs to process};
			goto  err;
		}
		if  (!(mypriv->spu_flgs & PV_VOTHERJ)  &&  strcmp(Realuname, Job_seg.jj_ptrs[Jeline]->j.spq_uname) != 0)  {
			err_no = $E{spq cannot look at job};
			goto  err;
		}
		goto  crest;

	case  $K{spq key abort job}:
	case  $K{spq key job other options}:
	case  $K{spq key unqueue job}:
	case  $K{spq key copy job}:

	case  $K{spq key job copies}:
	case  $K{spq key job priority}:
	case  $K{spq key job printer}:
	case  $K{spq key job form type}:
	case  $K{spq key job header}:
	case  $K{spq key job hold time}:
	case  $K{spq key job notp del}:
	case  $K{spq key job pdel}:
	case  $K{spq key job retain}:
	case  $K{spq key job noh}:
	case  $K{spq key job write}:
	case  $K{spq key job mail}:
	case  $K{spq key job wattn}:
	case  $K{spq key job mattn}:
	case  $K{spq key job loco}:
	case  $K{spq key job class}:
	case  $K{spq key job range}:
	case  $K{spq key job hat}:
	case  $K{spq key job oddeven}:
	case  $K{spq key job user}:
	case  $K{spq key job flags}:

		if  (Jeline >= Job_seg.njobs)  {
			err_no = $E{spq no jobs to process};
			goto  err;
		}

		if  (!(mypriv->spu_flgs & PV_OTHERJ)  &&  strcmp(Realuname, Job_seg.jj_ptrs[Jeline]->j.spq_uname) != 0)  {
			err_no = $E{spq job not yours};
			goto  err;
		}
	crest:

		/* Copy details of job into request buffer before we
		   start munging it.  */

		JREQ = Job_seg.jj_ptrs[Jeline]->j;
		JREQS = Job_seg.jj_ptrs[Jeline] - Job_seg.jlist;

		if  (JREQ.spq_netid  &&  !(mypriv->spu_flgs & PV_REMOTEJ))  {
			err_no = $E{spq no remote job priv};
			goto  err;
		}

		switch  (ch)  {
		case  $K{spq key abort job}:
			abortjob(JREQS);
			select_state(state);
			Ew = jscr;
			goto  nextin;

		case  $K{spq key view job}:
			if  (viewfile())
				my_wjmsg(SJ_CHNG);
			goto  refill;

		case  $K{spq key job other options}:
			if  (!(mypriv->spu_flgs & PV_ACCESSOK))  {
			noacc:
				err_no = $E{spq no access privilege};
				goto  err;
			}
			if  (qopts(JREQ.spq_job)  &&  pscr)
				pdisplay();
#ifdef	CURSES_OVERLAP_BUG
		refill:
			jdisplay();
#else
			jdisplay();
		refill:
#endif
			select_state(state);
			Ew = jscr;
			clear();
			refresh();
			if  (tpscr)
				touchwin(tpscr);
			if  (hpscr)
				touchwin(hpscr);
			if  (pscr)
				touchwin(pscr);
			if  (hjscr)
				touchwin(hjscr);
			touchwin(jscr);
			Cjobno = Job_seg.njobs != 0? Job_seg.jj_ptrs[Jeline]->j.spq_job: -1;
#ifdef HAVE_TERMINFO
			if  (tpscr)
				wnoutrefresh(tpscr);
			if  (hjscr)
				wnoutrefresh(hjscr);
			if  (hpscr)
				wnoutrefresh(hpscr);
			if  (pscr)
				wnoutrefresh(pscr);
			if  (escr)  {
				wnoutrefresh(jscr);
				touchwin(escr);
				wnoutrefresh(escr);
			}
			wmove(jscr, currow, 0);
			wnoutrefresh(jscr);
			doupdate();
#else
			if  (tpscr)
				wrefresh(tpscr);
			if  (hjscr)
				wrefresh(hjscr);
			if  (hpscr)
				wrefresh(hpscr);
			if  (pscr)
				wrefresh(pscr);
			if  (escr)  {
				wrefresh(jscr);
				touchwin(escr);
				wrefresh(escr);
			}
			wmove(jscr, currow, 0);
			wrefresh(jscr);
#endif
			goto  nextin;

		case  $K{spq key unqueue job}:
		case  $K{spq key copy job}:
			if  (!(mypriv->spu_flgs & PV_UNQUEUE))  {
				err_no = $E{spq cannot unqueue};
				goto  err;
			}
			if  (dounqueue(&Job_seg.jj_ptrs[Jeline]->j, ch == $K{spq key copy job}) == 0)
				goto  nextin;
			goto  refill;
		}

		/* Get here if command is an attribute-changing thing.
		   Build up 'sctrl' structure from attribute table.
		   If no column assigned, then reject unless it is a
		   historically important field and the guy has access ok
		   permission. */

		atp = &job_attribs[ch - $K{spq key job copies}];
		if  (atp->col < 0  &&  (!(mypriv->spu_flgs & PV_ACCESSOK) || ch >= $K{spq key job hold time}))
			goto  unknownc;

		opjob.helpcode = atp->helpcode;
		opjob.helpfn = atp->helpfn;
		opjob.col = atp->col;
		opjob.size = atp->size;
		opjob.retv = 0;
		opjob.magic_p = atp->magic_p;
		opjob.min = atp->min;
		opjob.vmax = atp->vmax;
		opjob.msg = (char *) 0;

		/* Do the business....

		   functions return -1 if they make changes,
		   0 if nothing >0 for error code.  */

		err_no = (*atp->actfn)(currow, &opjob);
		select_state(state);
		if  (err_no > 0)  {
			wmove(jscr, currow, 0);
			goto  err;
		}
		if  (err_no < 0)
			my_wjmsg(SJ_CHNG);
		goto  Jmove;

	case $K{key exec}:  case $K{key exec}+1:case $K{key exec}+2:case $K{key exec}+3:case $K{key exec}+4:
	case $K{key exec}+5:case $K{key exec}+6:case $K{key exec}+7:case $K{key exec}+8:case $K{key exec}+9:
		job_macro(Jeline >= Job_seg.njobs? (const struct spq *) 0: &Job_seg.jj_ptrs[Jeline]->j, ch - $K{key exec});
		jdisplay();
		if  (escr)  {
			touchwin(escr);
			wrefresh(escr);
		}
		goto  Jmove;
	}
}
