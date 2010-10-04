/* qopts.c -- spq options and things using curses

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

#include <curses.h>
#include <sys/types.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_sig.h"
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "spuser.h"
#include "keynames.h"
#include "sctrl.h"
#include "magic_ch.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "displayopt.h"

#define	SECSPERDAY	(24 * 60 * 60L)

#define	HALTATCOL	45

extern  void  cjfind();
extern	void  dochelp(WINDOW *, const int);
extern	void  doerror(WINDOW *, const int);
extern	void  endhe(WINDOW *, WINDOW **);
extern  void  rpfile();
extern	void	my_wjmsg(const int);
extern	char	**wotjform(const char *, const int);
extern	char	**wotjprin(const char *, const int);
extern	void	tdisplay(WINDOW *, const time_t, const int, const int);
extern	int	wtime(WINDOW *, const int, const int);
#ifndef	HAVE_ATEXIT
extern  void  exit_cleanup();
#endif

extern	struct	spdet	*mypriv;

extern	char	helpclr;
extern	int	hadrfresh;
extern	struct	spr_req	jreq;
#define	JREQS	jreq.spr_un.j.spr_jslot
extern	struct	spq	JREQ;

static	char	*emsg,
		*yesmsg,
		*nomsg,
		*percentmsg;

static	int	yesnlng;

#define	NULLCH		((char *) 0)

struct	ltab	{
	int	helpcode;
	char	*message;
	char	row, col, size, stickrow;
	void	(*dfn)(const struct ltab *);
	int	(*fn)(const struct ltab *);
};

/* Routine to send message hopefully securely.
   Gyrations are to try to avoid the signal arriving
   the instant you do the my_wjmsg. */

static	void  cmsg()
{
#ifdef	HAVE_SIGACTION

	sigset_t  sset, uset;
	sigemptyset(&sset);
	sigfillset(&uset);
	sigaddset(&sset, QRFRESH);
	sigdelset(&uset, QRFRESH);
	sigprocmask(SIG_BLOCK, &sset, (sigset_t *) 0);
	my_wjmsg(SJ_CHNG);
	sigsuspend(&uset);
	sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);

#elif	defined(HAVE_SIGSET)

	sighold(QRFRESH);
	my_wjmsg(SJ_CHNG);
	sigpause(QRFRESH); /* The sematics of which are different... */

#elif	(defined(HAVE_SIGVEC) && defined(SV_INTERRUPT)) || defined(HAVE_SIGVECTOR)

	sigsetmask(sigmask(QRFRESH));
	my_wjmsg(SJ_CHNG);
	sigpause(0);
	sigsetmask(0);

#else
	my_wjmsg(SJ_CHNG);
	/* Could hang if signal arrives HERE */
	pause();
#endif
}

static	void  qd_title(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_file);
}

static	void  qd_supph(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_NOH? yesmsg: nomsg);
}

static	void  qd_form(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_form);
}

static	void  qd_ptr(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_ptr);
}

static	void  qd_puser(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_puname);
}

static	void  qd_mail(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_MAIL? yesmsg: nomsg);
}

static	void  qd_wmsg(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_WRT? yesmsg: nomsg);
}

static	void  qd_mattn(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_MATTN? yesmsg: nomsg);
}

static	void  qd_wattn(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_WATTN? yesmsg: nomsg);
}

static	void  qd_class(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, hex_disp(JREQ.spq_class, 1));
}

static	void  qd_flags(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_flags);
}

static	void  qd_retain(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_RETN? yesmsg: nomsg);
}

static	void  qd_num(const struct ltab *lt, const LONG num)
{
	move(lt->row, lt->col);

	if  (num > LOTSANDLOTS)
		printw("%*s", lt->size, emsg);
	else
		printw("%*ld", lt->size, num);
}

static	void  qd_cps(const struct ltab *lt)
{
	qd_num(lt, (LONG) JREQ.spq_cps);
}

static	void  qd_pri(const struct ltab *lt)
{
	qd_num(lt, (LONG) JREQ.spq_pri);
}

static	void  qd_startp(const struct ltab *lt)
{
	qd_num(lt, (LONG) (JREQ.spq_start + 1L));
}

static	void  qd_hatp(const struct ltab *lt)
{
	qd_num(lt, (LONG) (JREQ.spq_haltat + 1L));
}

static	void  qd_endp(const struct ltab *lt)
{
	qd_num(lt, (LONG) (JREQ.spq_end + 1L));
}

static	void  qd_sodd(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_ODDP? yesmsg: nomsg);
}

static	void  qd_seven(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_EVENP? yesmsg: nomsg);
}

static	void  qd_revoe(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_REVOE? yesmsg: nomsg);
}

static	void  qd_printed(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_dflags & SPQ_PRINTED? yesmsg: nomsg);
}

static	void  qd_local(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, JREQ.spq_jflags & SPQ_LOCALONLY? yesmsg: nomsg);
}

static	void  qd_dinp(const struct ltab *lt)
{
	qd_num(lt, (LONG) JREQ.spq_nptimeout);
}

static	void  qd_dip(const struct ltab *lt)
{
	qd_num(lt, (LONG) JREQ.spq_ptimeout);
}

static	void  qd_hold(const struct ltab *lt)
{
	tdisplay(stdscr, (time_t) JREQ.spq_hold, lt->row, lt->col);
}

static	int  qo_title(const struct ltab *lt)
{
	char	*str;
	struct	sctrl	wst_file;

	wst_file.helpcode = lt->helpcode;
	wst_file.helpfn = HELPLESS;
	wst_file.size = lt->size;
	wst_file.col = lt->col;
	wst_file.magic_p = MAG_OK|MAG_R|MAG_CRS|MAG_NL;
	wst_file.msg = (char *) 0;

	if  ((str = wgets(stdscr, lt->row, &wst_file, JREQ.spq_file)))  {
		strncpy(JREQ.spq_file, str, MAXTITLE);
		cmsg();
		return  $K{key eol};
	}
	return  wst_file.retv;
}

static	int  qo_form(const struct ltab *lt)
{
	char	*str;
	struct	sctrl	wst_form;

	wst_form.helpcode = lt->helpcode;
	wst_form.helpfn = wotjform;
	wst_form.size = lt->size;
	wst_form.col = lt->col;
	wst_form.magic_p = MAG_P|MAG_R|MAG_NL|MAG_FNL|MAG_CRS;
	wst_form.msg = (char *) 0;

	if  ((str = wgets(stdscr, lt->row, &wst_form, JREQ.spq_form)))  {
		if  (!((mypriv->spu_flgs & PV_FORMS)  ||  qmatch(mypriv->spu_formallow, str)))  {
			disp_str = mypriv->spu_formallow;
			doerror(stdscr, $E{spq cannot select forms});
			ws_fill(stdscr, lt->row, &wst_form, JREQ.spq_form);
			refresh();
			return  0;
		}
		strncpy(JREQ.spq_form, str, MAXFORM);
		cmsg();
		return  $K{key eol};
	}
	return  wst_form.retv;
}

static	int  qo_ptr(const struct ltab *lt)
{
	char	*str;
	struct	sctrl	wst_ptr;

	wst_ptr.helpcode = lt->helpcode;
	wst_ptr.helpfn = wotjprin;
	wst_ptr.size = lt->size;
	wst_ptr.col = lt->col;
	wst_ptr.magic_p = MAG_P|MAG_R|MAG_CRS|MAG_NL;
	wst_ptr.msg = (char *) 0;

	str = wgets(stdscr, lt->row, &wst_ptr, JREQ.spq_ptr);
	if  (str)  {
		if  (!((mypriv->spu_flgs & PV_OTHERP)  ||  issubset(mypriv->spu_ptrallow, str)))  {
			disp_str = mypriv->spu_ptrallow;
			doerror(stdscr, $E{spq cannot select ptrs});
			ws_fill(stdscr, lt->row, &wst_ptr, JREQ.spq_ptr);
			refresh();
			return  0;
		}
		strncpy(JREQ.spq_ptr, str, JPTRNAMESIZE);
		cmsg();
		return  $K{key eol};
	}
	return  wst_ptr.retv;
}

static	int  qo_flags(const struct ltab *lt)
{
	int	siz;
	char	*str;
	struct	sctrl	wst_flags;

	wst_flags.helpcode = lt->helpcode;
	wst_flags.helpfn = HELPLESS;
	wst_flags.col = lt->col;
	siz = COLS - lt->col;
	wst_flags.size = (USHORT) (lt->size > siz? siz: lt->size);
	wst_flags.magic_p = MAG_OK|MAG_R|MAG_NL|MAG_CRS;
	wst_flags.msg = (char *) 0;

	if  ((str = wgets(stdscr, lt->row, &wst_flags, JREQ.spq_flags)))  {
		strncpy(JREQ.spq_flags, str, MAXFLAGS);
		cmsg();
		return  $K{key eol};
	}
	return  wst_flags.retv;
}

static	int  qo_puser(const struct ltab *lt)
{
	char	*str, *origu;
	struct	sctrl	wst_user;

	origu = JREQ.spq_puname;
	wst_user.helpcode = lt->helpcode;
	wst_user.helpfn = gen_ulist;
	wst_user.size = lt->size;
	wst_user.col = lt->col;
	wst_user.magic_p = MAG_P|MAG_R|MAG_NL|MAG_FNL|MAG_CRS;
	wst_user.msg = (char *) 0;

	for  (;;)  {
		if  (!(str = wgets(stdscr, lt->row, &wst_user, origu)))
			return  wst_user.retv;
#ifdef	NOTNEEDED_I_DONT_THINK
		if  (str[0] == '\0')  {
			ws_fill(stdscr, lt->row, &wst_user, origu);
			refresh();
			return  $K{key eol};
		}
#endif
		if  (lookup_uname(str) != UNKNOWN_UID)  {
			strncpy(JREQ.spq_puname, str, UIDSIZE);
			cmsg();
			return  $K{key eol};
		}
		disp_str = str;
		doerror(stdscr, $E{Unknown user in spq});
		ws_fill(stdscr, lt->row, &wst_user, origu);
	}
}

static	int  qo_bool(const struct ltab *lt, unsigned b)
{
	int	ch;

	select_state($S{spq set boolean state});
	move(lt->row, lt->col);
	refresh();

	for  (;;)  {
		do  ch = getkey(MAG_A|MAG_P);
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
			doerror(stdscr, $E{spq set boolean state});
		case  EOF:
			continue;

		case  $K{key help}:
			dochelp(stdscr, lt->helpcode);
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			refresh();
			continue;

		case  $K{key yes}:
			if  (b)
				return  $K{key eol};
			printw("%-*s", yesnlng, yesmsg);
			refresh();
			reset_state();
			return  1;

		case  $K{key no}:
			if  (!b)
				return  $K{key eol};
			printw("%-*s", yesnlng, nomsg);
			refresh();
			reset_state();
			return  0;

		case  $K{key toggle}:
			printw("%-*s", yesnlng, b? nomsg: yesmsg);
			refresh();
			reset_state();
			return  -1;

		case  $K{key halt}:
		case  $K{key cursor down}:
		case  $K{key eol}:
		case  $K{key cursor up}:
		case  $K{key erase}:
			reset_state();
			return  ch;
		}
	}
}

static	int  qo_wmsg(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_WRT))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_jflags ^= SPQ_WRT;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_WRT;
		break;
	case  1:
		JREQ.spq_jflags |= SPQ_WRT;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_mail(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_MAIL))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_jflags ^= SPQ_MAIL;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_MAIL;
		break;
	case  1:
		JREQ.spq_jflags |= SPQ_MAIL;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_wattn(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_WATTN))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_jflags ^= SPQ_WATTN;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_WATTN;
		break;
	case  1:
		JREQ.spq_jflags |= SPQ_WATTN;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_mattn(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_MATTN))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_jflags ^= SPQ_MATTN;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_MATTN;
		break;
	case  1:
		JREQ.spq_jflags |= SPQ_MATTN;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_supph(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_NOH))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_jflags ^= SPQ_NOH;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_NOH;
		break;
	case  1:
		JREQ.spq_jflags |= SPQ_NOH;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_retain(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_RETN))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_jflags ^= SPQ_RETN;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_RETN;
		break;
	case  1:
		JREQ.spq_jflags |= SPQ_RETN;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_sodd(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_ODDP))  {
	default:
		return  ch;
	case  -1:
		if  (JREQ.spq_jflags & SPQ_ODDP)  {
			JREQ.spq_jflags &= ~SPQ_ODDP;
			break;
		}
	case  1:
		JREQ.spq_jflags |= SPQ_ODDP;
		JREQ.spq_jflags &= ~SPQ_EVENP;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_ODDP;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_seven(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_EVENP))  {
	default:
		return  ch;
	case  -1:
		if  (JREQ.spq_jflags & SPQ_EVENP)  {
			JREQ.spq_jflags &= ~SPQ_EVENP;
			break;
		}
	case  1:
		JREQ.spq_jflags |= SPQ_EVENP;
		JREQ.spq_jflags &= ~SPQ_ODDP;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_EVENP;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_local(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_LOCALONLY))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_jflags ^= SPQ_LOCALONLY;
		break;
	case  1:
		JREQ.spq_jflags |= SPQ_LOCALONLY;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_LOCALONLY;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_revoe(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_jflags & SPQ_REVOE))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_jflags ^= SPQ_REVOE;
		break;
	case  1:
		JREQ.spq_jflags |= SPQ_REVOE;
		break;
	case  0:
		JREQ.spq_jflags &= ~SPQ_REVOE;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_printed(const struct ltab *lt)
{
	int	ch;

	switch  (ch = qo_bool(lt, JREQ.spq_dflags & SPQ_PRINTED))  {
	default:
		return  ch;
	case  -1:
		JREQ.spq_dflags ^= SPQ_PRINTED;
		break;
	case  1:
		JREQ.spq_dflags |= SPQ_PRINTED;
		break;
	case  0:
		JREQ.spq_dflags &= ~SPQ_PRINTED;
		break;
	}
	cmsg();
	return  $K{key eol};
}

static	int  qo_cps(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_cps;

	wnt_cps.helpcode = lt->helpcode;
	wnt_cps.helpfn = HELPLESS;
	wnt_cps.size = lt->size;
	wnt_cps.col = lt->col;
	wnt_cps.magic_p = MAG_P|MAG_A|MAG_R;
	wnt_cps.min = 0L;
	wnt_cps.vmax = 255L;
	wnt_cps.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_cps, (LONG) JREQ.spq_cps);
	if  (in < 0)
		return  wnt_cps.retv;
	JREQ.spq_cps = (unsigned char) in;
	cmsg();
	return  $K{key eol};
}

static	int  qo_pri(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_pri;

	wnt_pri.helpcode = lt->helpcode;
	wnt_pri.helpfn = HELPLESS;
	wnt_pri.size = lt->size;
	wnt_pri.col = lt->col;
	wnt_pri.magic_p = MAG_P|MAG_A|MAG_R;
	wnt_pri.min = 1L;
	wnt_pri.vmax = 255L;
	wnt_pri.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_pri, (LONG) JREQ.spq_pri);
	if  (in < 0)
		return  wnt_pri.retv;
	JREQ.spq_pri = (unsigned char) in;
	cmsg();
	return  $K{key eol};
}

static	int  qo_dinp(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_npt;

	wnt_npt.helpcode = lt->helpcode;
	wnt_npt.helpfn = HELPLESS;
	wnt_npt.size = lt->size;
	wnt_npt.col = lt->col;
	wnt_npt.magic_p = MAG_P|MAG_A|MAG_R;
	wnt_npt.min = 1L;
	wnt_npt.vmax = 0xffffL;
	wnt_npt.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_npt, (LONG) JREQ.spq_nptimeout);
	if  (in < 0)
		return  wnt_npt.retv;
	JREQ.spq_nptimeout = (USHORT) in;
	cmsg();
	return  $K{key eol};
}

static	int  qo_dip(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_pt;

	wnt_pt.helpcode = lt->helpcode;
	wnt_pt.helpfn = HELPLESS;
	wnt_pt.size = lt->size;
	wnt_pt.col = lt->col;
	wnt_pt.magic_p = MAG_P|MAG_A|MAG_R;
	wnt_pt.min = 1L;
	wnt_pt.vmax = 0xffffL;
	wnt_pt.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_pt, (LONG) JREQ.spq_ptimeout);
	if  (in < 0)
		return  wnt_pt.retv;
	JREQ.spq_ptimeout = (USHORT) in;
	cmsg();
	return  $K{key eol};
}

static	int  qo_startp(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_start;

	wnt_start.helpcode = lt->helpcode;
	wnt_start.helpfn = HELPLESS;
	wnt_start.size = lt->size;
	wnt_start.col = lt->col;
	wnt_start.magic_p = MAG_P|MAG_A|MAG_R;
	wnt_start.min = 1L;
	wnt_start.vmax = LOTSANDLOTS;
	wnt_start.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_start, (LONG) (JREQ.spq_start + 1L));
	if  (in < 0)
		return  wnt_start.retv;

	JREQ.spq_start = in - 1L;
	cmsg();
	return  $K{key eol};
}

static	int  qo_hatp(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_haltat;

	wnt_haltat.helpcode = lt->helpcode;
	wnt_haltat.helpfn = HELPLESS;
	wnt_haltat.size = lt->size;
	wnt_haltat.col = lt->col;
	wnt_haltat.magic_p = MAG_P|MAG_A|MAG_R;
	wnt_haltat.min = 1L;
	wnt_haltat.vmax = LOTSANDLOTS;
	wnt_haltat.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_haltat, (LONG) (JREQ.spq_haltat + 1L));
	if  (in < 0)
		return  wnt_haltat.retv;

	JREQ.spq_haltat = in - 1L;
	cmsg();
	return  $K{key eol};
}

static	int  qo_endp(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_end;

	wnt_end.helpcode = lt->helpcode;
	wnt_end.helpfn = HELPLESS;
	wnt_end.size = lt->size;
	wnt_end.col = lt->col;
	wnt_end.magic_p = MAG_P|MAG_A|MAG_R;
	wnt_end.min = 1L;
	wnt_end.vmax = LOTSANDLOTS;
	wnt_end.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_end, (LONG) (JREQ.spq_end + 1L));
	if  (in < 0)
		return  wnt_end.retv;

	JREQ.spq_end = in - 1L;
	cmsg();
	return  $K{key eol};
}

static	int  qo_class(const struct ltab *lt)
{
	classcode_t  in;
	struct	sctrl	wht_cl;

	wht_cl.helpcode = lt->helpcode;
	wht_cl.helpfn = HELPLESS;
	wht_cl.size = lt->size;
	wht_cl.col = lt->col;
	wht_cl.magic_p = MAG_P|MAG_R;
	wht_cl.min = 1L;
	wht_cl.vmax = LOTSANDLOTS;
	wht_cl.msg = (char *) 0;

	in = whexnum(stdscr, lt->row, &wht_cl, JREQ.spq_class);
	reset_state();
	if  (in != JREQ.spq_class)  {
		if  (!(mypriv->spu_flgs & PV_COVER))
			in &= mypriv->spu_class;
		if  (in == 0L)  {
			mvaddstr(lt->row, lt->col, hex_disp(JREQ.spq_class, 1));
			disp_str = hex_disp(mypriv->spu_class, 0);
			doerror(stdscr, $E{spq set zero class});
			return  0;
		}
		JREQ.spq_class = in;
		cmsg();
	}
	return  wht_cl.retv? wht_cl.retv: $K{key eol};
}

static	int  qo_hold(const struct ltab *lt)
{
	int	ret;
	time_t	origtime = JREQ.spq_hold;

	if  ((ret = wtime(stdscr, lt->row, lt->col)) == 0)
		return  $K{key eol};
	if  ((time_t) JREQ.spq_hold != origtime)
		cmsg();
	return  ret;
}

static	struct	ltab  ltab[] = {
{	$PHN{Qopt title},	NULLCH, 0, 0, MAXTITLE, 0, qd_title, qo_title	},
{	$PHN{Qopt nohdr},	NULLCH, 0, 0, 0, 0, qd_supph, qo_supph		},
{	$PHN{Qopt form type},	NULLCH, 0, 0, MAXFORM, 0, qd_form, qo_form	},
{	$PHN{Qopt copies},	NULLCH, 0, 0, 4, 0, qd_cps, qo_cps		},
{	$PHN{Qopt printer},	NULLCH, 0, 0, JPTRNAMESIZE, 0, qd_ptr, qo_ptr	},
{	$PHN{Qopt prio},	NULLCH, 0, 0, 4, 0, qd_pri, qo_pri		},
{	$PHN{Qopt post user},	NULLCH, 0, 0, 10, 0, qd_puser, qo_puser		},
{	$PHN{Qopt mail msg},	NULLCH, 0, 0, 0, 1, qd_mail, qo_mail		},
{	$PHN{Qopt write msg},	NULLCH, 0, 0, 0, 1, qd_wmsg, qo_wmsg		},
{	$PHN{Qopt mattn},	NULLCH, 0, 0, 0, 1, qd_mattn, qo_mattn		},
{	$PHN{Qopt wattn},	NULLCH, 0, 0, 0, 1, qd_wattn, qo_wattn		},
{	$PHN{Qopt class},	NULLCH, 0, 0, 32, 0, qd_class, qo_class		},
{	$PHN{Qopt flags},	NULLCH, 0, 0, MAXFLAGS, 0, qd_flags, qo_flags	},
{	$PHN{Qopt retn},	NULLCH, 0, 0, 0, 0, qd_retain, qo_retain	},
{	$PHN{Qopt start page},	NULLCH, 0, 0, 8, 0, qd_startp, qo_startp	},
{	$PHN{Qopt hat page},	NULLCH, 0, 0, 8, 1, qd_hatp, qo_hatp		},
{	$PHN{Qopt end page},	NULLCH, 0, 0, 8, 0, qd_endp, qo_endp		},
{	$PHN{Qopt skip odd},	NULLCH, 0, 0, 0, 1, qd_sodd, qo_sodd		},
{	$PHN{Qopt skip even},	NULLCH, 0, 0, 0, 1, qd_seven, qo_seven		},
{	$PHN{Qopt reverse skip},NULLCH, 0, 0, 0, 1, qd_revoe, qo_revoe		},
{	$PHN{Qopt printed},	NULLCH, 0, 0, 0, 0, qd_printed, qo_printed	},
{	$PHN{Qopt del not printed}, NULLCH, 0, 0, 6, 0, qd_dinp, qo_dinp	},
{	$PHN{Qopt del printed}, NULLCH, 0, 0, 6, 0, qd_dip, qo_dip		},
{	$PHN{Qopt hold time},	NULLCH, 0, 0, 0, 0, qd_hold, qo_hold		},
{	$PHN{Qopt local only},	NULLCH, 0, 0, 0, 0, qd_local, qo_local		}
};

#define	TABNUM	(sizeof(ltab)/sizeof(struct ltab))

static	struct	ltab	*lptrs[TABNUM+1];
static	int	comeinat;

static	void  initnames()
{
	int	i, next;
	struct  ltab	*lt;
	int	curr, hrows, nextrow, cols, look4, rowstart, pstart, laststick;
	char	**hp;
	int	nextstate[TABNUM];

	disp_str = "";
	disp_arg[0] = disp_arg[1] = 0;

	/* Count lines in header to see where the action starts */

	hp = helphdr('O');
	count_hv(hp, &hrows, &cols);
	freehelp(hp);

	/* Slurp up standard messages */

	percentmsg = gprompt($P{spq qmsg percent complete});
	emsg = gprompt($P{spq qmsg end});
	yesmsg = gprompt($P{spq qmsg yes});
	nomsg = gprompt($P{spq qmsg no});
	yesnlng = strlen(yesmsg);
	if  ((i = strlen(nomsg)) > yesnlng)
		yesnlng = i;

	if  ((rowstart = helpnstate($N{Qopt Row start})) <= 0)
		rowstart = $N{Qopt title};
	if  ((pstart = helpnstate($N{Qopt Initial prompt})) <= 0)
		pstart = rowstart;

	if  (rowstart < $N{Qopt title} || rowstart >= $N{Qopt title} + TABNUM)  {
		disp_arg[9] = rowstart;
	bads:
		doerror(stdscr, $E{spq qopts bad state code});
		refresh();
		do  i = getkey(MAG_A|MAG_P);
		while	(i == EOF);
#ifndef	HAVE_ATEXIT
		exit_cleanup();
#endif
		exit(E_BADCFILE);
	}

	if  (pstart < $N{Qopt title} || pstart >= $N{Qopt title} + TABNUM)  {
		disp_arg[9] = pstart;
		goto  bads;
	}

	for  (lt = &ltab[0]; lt < &ltab[TABNUM]; lt++)  {
		lt->message = gprompt(lt->helpcode);
		lt->col = strlen(lt->message) + 1;
	}

	for  (i = 0; i < TABNUM; i++)
		nextstate[i] = -1;

	look4 = rowstart;
	do  {
		next = helpnstate(look4);
		if  (next >= 0)  {
			if  (next < $N{Qopt title} || next >= $N{Qopt title} + TABNUM)  {
				disp_arg[9] = next;
				goto  bads;
			}
			if  (nextstate[next-$N{Qopt title}] > 0)  {
				disp_arg[9] = next;
				disp_arg[8] = nextstate[next-$N{Qopt title}];
				doerror(stdscr, $E{spq qopts duplicated state code});
				refresh();
				do  i = getkey(MAG_A|MAG_P);
				while	(i == EOF);
#ifndef	HAVE_ATEXIT
				exit_cleanup();
#endif
				exit(E_BADCFILE);
			}
		}
		nextstate[look4 - $N{Qopt title}] = next;
		look4 = next;
	}  while  (next > 0);

	comeinat = 0;
	next = 0;
	curr = rowstart;
	nextrow = hrows;
	laststick = 0;
	do  {
		if  (pstart == curr)
			comeinat = next;
		lt = &ltab[curr-$N{Qopt title}];
		if  (lt->stickrow)  {
			if  (lt->helpcode == $N{Qopt hat page})  {
				lt->col += HALTATCOL;
				lt->row = nextrow;
				laststick = 0;
			}
			else  {
				if  (laststick + lt->col + yesnlng + 8 > COLS)
					nextrow++;
				else
					lt->col += laststick;
				laststick = lt->col + yesnlng + 1;
				lt->row = nextrow;
			}
		}
		else  {
			if  (laststick)  {
				nextrow++;
				laststick = 0;
			}
			lt->row = nextrow++;
		}
		lptrs[next] = lt;
		next++;
		curr = nextstate[curr-$N{Qopt title}];
	}  while  (curr > 0);
	lptrs[next] = (struct ltab *) 0;
}

static	void  qodisp()
{
	int	i;
	struct  ltab	*lt;
	char	**hp;

	clear();

	disp_arg[0] = JREQ.spq_job;
	disp_str = JREQ.spq_uname;

	if  ((hp = helphdr('O')))  {
		char	**hpp;

		for  (i = 0, hpp = hp;  *hpp;  i++, hpp++)  {
			mvwhdrstr(stdscr, i, 0, *hpp);
			if  (i == 0)  {
				int	row, col;
				getyx(stdscr, row, col);
				tdisplay(stdscr, (time_t) JREQ.spq_time, row, col);
				if  (JREQ.spq_dflags & SPQ_PQ)  {
					LONG	place = (JREQ.spq_posn * 100) / JREQ.spq_size;
					addch(' ');
					printw(percentmsg, place);
				}
			}
			free(*hpp);
		}
		free((char *) hp);
	}

	for  (i = 0;  (lt = lptrs[i]);  i++)  {
		if  (lt->stickrow)  {
			if  (lt->helpcode == $N{Qopt hat page})  {
				if  (JREQ.spq_haltat == 0L)
					continue;
				move(lt->row, HALTATCOL);
			}
			else
				move(lt->row, lt->col - strlen(lt->message) - 1);
		}
		else
			move(lt->row, 0);
		addstr(lt->message);
		(*lt->dfn)(lt);
	}
#ifdef	CURSES_OVERLAP_BUG
	touchwin(stdscr);
#endif
	refresh();
}

/* This accepts input from the screen.
   JREQS (slot number) already set up in sq_jlist */

int  qopts(const jobno_t Jnum)
{
	char	**ev;
	int	ch, rows, cols, row, col, i, whichel, pchanges = 0;
	struct  ltab	*lt;

	if  (!percentmsg)
		initnames();

	Ew = stdscr;
	whichel = comeinat;
 redo:
	qodisp();

	if  (hadrfresh)  {
	snargle:
		do  {
			hadrfresh = 0;
			rpfile();
			readjoblist(1);
			cjfind();
			pchanges++;	/* Do pdisplay() if needed */
		}  while  (hadrfresh);

		if  (Job_seg.jlist[JREQS].j.spq_job == Jnum)  {
			JREQ = Job_seg.jlist[JREQS].j;
			goto  redo;
		}

		ev = helpvec($E{spq job disappeared}, 'E');

		if  (*ev == (char *) 0)  {
			free((char *) ev);
			disp_arg[0] = $E{spq job disappeared};
			ev = helpvec($E{Missing error code}, 'E');
		}
		count_hv(ev, &rows, &cols);
		if  (rows <= 0)
			goto  ret;
		clear();
		standout();
		col = (COLS - cols) / 2;
		for  (i = 0, row = (LINES - rows)/2; i < rows;  i++, row++)
			mvprintw(row, col, "%s", ev[i]);
		freehelp(ev);
		standend();
		refresh();
		reset_state();
		do	ch = getkey(MAG_A|MAG_P);
		while	(ch == EOF);
		goto  ret;
	}

 retry:
	while  ((lt = lptrs[whichel]))  {
		if  (lt->stickrow  && lt->helpcode == $N{Qopt hat page} && JREQ.spq_haltat == 0L)  {
			whichel++;
			continue;
		}
		ch = (*lt->fn)(lt);
		switch  (ch)  {
		default:
			continue;
		case  0:
		case  $K{key eol}:
		case  $K{key cursor down}:
			whichel++;
			break;
		case  $K{key erase}:
		case  $K{key cursor up}:
			if  (--whichel < 0)
				whichel = 0;
			else  if  (lptrs[whichel]->stickrow  &&
				   lptrs[whichel]->helpcode == $N{Qopt hat page}  &&
				   JREQ.spq_haltat == 0L)  {
				if  (--whichel < 0)
					whichel = 1;
			}
			break;
		case  $K{key halt}:
			goto  ret1;
		}
		if  (hadrfresh)
			goto  snargle;
	}

 ret1:
	if  (JREQ.spq_start > JREQ.spq_end)  {
		doerror(stdscr, $E{end page less than start page});
		if  (--whichel < 0)
			whichel = 0;
		goto  retry;
	}
	if  (JREQ.spq_haltat > JREQ.spq_end)  {
		doerror(stdscr, $E{end page less than haltat page});
		if  (--whichel < 0)
			whichel = 0;
		goto  retry;
	}

 ret:
	if  (escr)  {
		delwin(escr);
		escr = (WINDOW *) 0;
	}
	if  (hlpscr)  {
		delwin(hlpscr);
		hlpscr = (WINDOW *) 0;
	}
#ifdef	CURSES_MEGA_BUG
	clear();
	refresh();
#endif
	return  pchanges;
}
