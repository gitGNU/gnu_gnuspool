/* sq_plist.c -- spq printer list handling

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
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include <curses.h>
#ifdef	HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <ctype.h>
#include <errno.h>
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
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

#ifndef	getbegyx
#define	getbegyx(win,y,x)	((y) = (win)->_begy, (x) = (win)->_begx)
#endif

#define	DEFAULT_FORMAT	" %14p %10d %16f %>8s %3x %7j %7u %6w"

void	dochelp(WINDOW *, const int);
void	doerror(WINDOW *, const int);
void	dohelp(WINDOW *, struct sctrl *, const char *);
void	endhe(WINDOW *, WINDOW **);
void	jdisplay(void);
void	womsg(const int);
void	my_wjmsg(const int);
void	my_wpmsg(const int);
void	offersave(char *, const int);

int	propts(void);
int	view_errors(const int);
int	fmtprocess(char **, const char, struct sq_formatdef *, struct sq_formatdef *);

char	**wotpprin(const char *, const int);
char	**wottty(const char *, const int);
char	**wotpform(const char *, const int);

static	struct	sctrl
 wst_aptr  = { $PH{spq pptr help},	wotpprin, PTRNAMESIZE,0,  1,  MAG_P|MAG_NL|MAG_FNL|MAG_LONG, 0L, 0L, (char *) 0 },
 wst_alin  = { $PH{spq device help},	wottty,   LINESIZE,0, 16,  MAG_OK|MAG_P|MAG_NL|MAG_FNL|MAG_LONG, 0L, 0L, (char *) 0 },
 wst_aform = { $PH{spq pform help},	wotpform, MAXFORM, 0, 27,  MAG_P|MAG_NL|MAG_FNL|MAG_LONG, 0L, 0L, (char *) 0 },
 wst_nform = { $PH{spq new pform help},	wotpform, MAXFORM, 0, 27,  MAG_P|MAG_NL|MAG_FNL|MAG_LONG, 0L, 0L, (char *) 0 },
 wst_adescr = { $PH{spq descr help},	HELPLESS, COMMENTSIZE, 0, -1, MAG_OK|MAG_LONG, 0L, 0L, (char *) 0};

/* whexnum stuff */

static	struct	sctrl
  wht_pcl = { $H{spq pclass help}, HELPLESS, 32, 0, 0, MAG_P, 0L, U_MAX_CLASS, (char *) 0 };

/* Vector of states - assumed given prompt codes consecutively.  */

static	char	*statenames[SPP_NSTATES];

static	char	*ptr_format, *bigbuff;

extern	int	HPLINES, PLINES, hadrfresh, wh_ptitline;
extern	time_t	hadalarm, lastalarm;
extern	char	scrkeep, helpclr;
extern	char	*Curr_pwd;
extern	unsigned	Pollinit;
extern	struct	spdet	*mypriv;
extern	struct	spr_req	preq, jreq, oreq;
extern	struct	spq	JREQ;
extern	struct	spptr	PREQ;

#define	PREQS	preq.spr_un.p.spr_pslot
#define	OREQ	oreq.spr_un.o.spr_jpslot

char	*current_prin;
int	Phline, Peline;
extern	WINDOW	*hjscr, *hpscr, *tpscr, *jscr, *pscr;

static	int	more_above,
		more_below,
		lnsmsg,
		lnemsg;
static	char	*more_amsg,	/* More after type message */
		*more_bmsg,	/* More before type message */
		*halteoj,	/* Halt at end of job message */
		*namsg,		/* Non-aligned message */
		*nsmsg,		/* Network indication start */
		*nemsg,		/* Network indication end */
		*intermsg,	/* Interrupted */
		*bypassa,	/* Bypass align confirm */
		*reinsta;	/* Reinstate align */

/* Open print file - now a shared memory segment.  */

void	openpfile(void)
{
	int	i;

	if  (!ptrshminit(1))  {
		print_error($E{Cannot open pshm});
		exit(E_PRINQ);
	}

	/* Get "more" messages.  */

	if  (PLINES > 2)  {
		more_amsg = gprompt($P{Ptrs more above});
		more_bmsg = gprompt($P{Ptrs more below});
	}
	bypassa = gprompt($P{Bypass align});
	reinsta = gprompt($P{Reinstate align});

	/* Read state names */

	for  (i = 0;  i < SPP_NSTATES;  i++)
		statenames[i] = gprompt($P{Printer status null}+i);
	halteoj = gprompt($P{Printer heoj});
	intermsg = gprompt($P{Printer interrupted});
	namsg = gprompt($P{Printer not aligned});
	nsmsg = gprompt($P{Netdev start str});
	nemsg = gprompt($P{Netdev end str});
	lnsmsg = strlen(nsmsg);
	lnemsg = strlen(nemsg);
}

/* Read printer list, checking on poll frequency. */

void	rpfile(void)
{
	unsigned oldnpp = Ptr_seg.npprocesses;
	readptrlist(1);
	if  (Ptr_seg.npprocesses)  {
		if  (!oldnpp)
			alarm(Pollinit);
	}
	else  if  (oldnpp)
		alarm(0);
}

typedef	int	fmt_t;
#include "inline/pfmt_ab.c"
#include "inline/pfmt_class.c"
#include "inline/pfmt_dev.c"
#include "inline/pfmt_form.c"
#include "inline/pfmt_heoj.c"
#include "inline/pfmt_pid.c"
#include "inline/pfmt_jobno.c"
#include "inline/pfmt_loco.c"
#include "inline/pfmt_msg.c"
#include "inline/pfmt_na.c"
#include "inline/pfmt_ptr.c"
#include "inline/pfmt_state.c"
#include "inline/pfmt_ostat.c"
#include "inline/pfmt_user.c"
#include "inline/pfmt_minsz.c"
#include "inline/pfmt_maxsz.c"
#include "inline/pfmt_limit.c"
#include "inline/pfmt_shrk.c"

#define	NULLCP	(char *) 0

static	struct	sq_formatdef
	lowertab[] = { /* a-z */
	{	$P{Printer title}+'a'-1,	6,	0,	NULLCP, NULLCP,	fmt_ab		},	/* a */
	{	0,				0,	0,	NULLCP, NULLCP,	0		},	/* b */
	{	$P{Printer title}+'c'-1,	32,	0,	NULLCP, NULLCP,	fmt_class	},	/* c */
	{	$P{Printer title}+'d'-1,	LINESIZE-4,0,	NULLCP, NULLCP, fmt_device	},	/* d */
	{	$P{Printer title}+'e'-1,	COMMENTSIZE-10,0,NULLCP,NULLCP,	fmt_comment	},	/* e */
	{	$P{Printer title}+'f'-1,	MAXFORM-4,0,	NULLCP, NULLCP, fmt_form	},	/* f */
	{	0,				0,	0,	NULLCP, NULLCP,	0		},	/* g */
	{	$P{Printer title}+'h'-1,	6,	0,	NULLCP, NULLCP,	fmt_heoj	},	/* h */
	{	$P{Printer title}+'i'-1,	5,	0,	NULLCP, NULLCP,	fmt_pid		},	/* i */
	{	$P{Printer title}+'j'-1,	6,	0,	NULLCP, NULLCP,	fmt_jobno	},	/* j */
	{	0,				0,	0,	NULLCP, NULLCP,	0		},	/* k */
	{	$P{Printer title}+'l'-1,	6,	0,	NULLCP, NULLCP,	fmt_localonly	},	/* l */
	{	$P{Printer title}+'m'-1,	10,	0,	NULLCP, NULLCP,	fmt_message	},	/* m */
	{	$P{Printer title}+'n'-1,	6,	0,	NULLCP, NULLCP,	fmt_na		},	/* n */
	{	0,				0,	0,	NULLCP, NULLCP,	0		},	/* o */
	{	$P{Printer title}+'p'-1,	PTRNAMESIZE-4,0,NULLCP, NULLCP, fmt_printer	},	/* p */
	{	0,				0,	0,	NULLCP, NULLCP,	0		},	/* q */
	{	0,				0,	0,	NULLCP, NULLCP, 0		},	/* r */
	{	$P{Printer title}+'s'-1,	8,	0,	NULLCP, NULLCP, fmt_state	},	/* s */
	{	$P{Printer title}+'t'-1,	8,	0,	NULLCP, NULLCP, fmt_ostate	},	/* t */
	{	$P{Printer title}+'u'-1,	UIDSIZE-2,0,	NULLCP, NULLCP, fmt_user	},	/* u */
	{	0,				0,	0,	NULLCP, NULLCP,	0		},	/* v */
	{	$P{Printer title}+'w'-1,	4,	0,	NULLCP, NULLCP,	fmt_shriek	},	/* w */
	{	$P{Printer title}+'x'-1,	2,	0,	NULLCP, NULLCP,	fmt_limit	},	/* x */
	{	$P{Printer title}+'y'-1,	6,	0,	NULLCP, NULLCP,	fmt_minsize	},	/* y */
	{	$P{Printer title}+'z'-1,	6,	0,	NULLCP, NULLCP,	fmt_maxsize	}	/* z */
};

char  *get_ptrtitle(void)
{
	int	nn, obuflen, isrjust;
	struct	sq_formatdef	*fp;
	char	*cp, *rp, *result, *mp;

	if  (!ptr_format  &&  !(ptr_format = helpprmpt($P{Spq ptr default format})))
		ptr_format = stracpy(DEFAULT_FORMAT);

	/* Initialise to say we have to generate a prompt */

	wst_aptr.col = wst_alin.col = wst_aform.col = wst_nform.col = wst_adescr.col = -1;
	wst_aptr.size = PTRNAMESIZE;
	wst_alin.size = LINESIZE;
	wst_aform.size = wst_nform.size = MAXFORM;
	wst_adescr.size = COMMENTSIZE;

	/* Initial pass to discover how much space to allocate */

	obuflen = 1;
	cp = ptr_format;
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

	/* Allocate space for title result and output buffer */

	result = malloc((unsigned) obuflen);
	if  (bigbuff)
		free(bigbuff);
	bigbuff = malloc(4 * obuflen);
	if  (!result ||  !bigbuff)
		nomem();

	/* Now set up title
	   Actually this is a waste of time if we aren't actually displaying
	   same, but we needed the buffer.  */

	rp = result;
	cp = ptr_format;
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

		if  (islower(*cp))
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
		case  'f':
			wst_aform.col = wst_nform.col = (SHORT) (rp - result);
			wst_aform.size = wst_nform.size = (USHORT) nn;
			isrjust = 0;
			break;
		case  'd':
			wst_alin.col = (SHORT) (rp - result);
			wst_alin.size = (USHORT) nn;
			isrjust = 0;
			break;
		case  'p':
			wst_aptr.col = (SHORT) (rp - result);
			wst_aptr.size = (USHORT) nn;
			isrjust = 0;
			break;
		case  'e':
			wst_adescr.col = (SHORT) (rp - result);
			wst_adescr.size = (USHORT) nn;
			isrjust = 0;
			break;
		case  'i':
		case  'j':
			isrjust = 1;
			break;
		}

		if  (fp->statecode == 0)
			continue;

		/* Get title message if we don't have it.
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

#ifdef	HAVE_TERMINFO
#define	DISP_CHAR(w, ch)	waddch(w, (chtype) ch);
#else
#define	DISP_CHAR(w, ch)	waddch(w, ch);
#endif

/* Display contents of printer list.  Don't put it on screen yet.  */

void	pdisplay(void)
{
	int	row, pcnt;

	werase(pscr);

	more_above = 0;
	more_below = 0;
	row = 0;
	pcnt = Phline;

	if  (PLINES > 2  &&  pcnt > 0) {
		wstandout(pscr);
		mvwprintw(pscr,
				 row,
				 (COLS - (int) strlen(more_amsg))/2,
				 more_amsg,
				 pcnt);
		wstandend(pscr);
		more_above = 1;
		row++;
	}

	for  (;  pcnt < Ptr_seg.nptrs  &&  row < PLINES;  pcnt++, row++) {
		const  struct  spptr  *pp = &Ptr_seg.pp_ptrs[pcnt]->p;
		struct	sq_formatdef  *fp;
		char	*cp = ptr_format, *lbp;
		int	currplace = -1, lastplace, nn, inlen, dummy;

		if (PLINES > 2  &&  row == PLINES - 1  &&  pcnt < Ptr_seg.nptrs - 1)  {
			wstandout(pscr);
			mvwprintw(pscr,
					 row,
					 (COLS - (int) strlen(more_bmsg))/2,
					 more_bmsg,
					 Ptr_seg.nptrs - pcnt);
			wstandend(pscr);
			more_below = 1;
			if  (Peline >= pcnt)
				Peline = pcnt - 1;
			break;
		}

		wmove(pscr, row, 0);

		while  (*cp)  {
			int	skiprest = 0;
			if  (*cp != '%')  {
				DISP_CHAR(pscr, *cp);
				cp++;
				continue;
			}
			cp++;
			lastplace = -1;
			if  (*cp == '<')  {
				lastplace = currplace;
				cp++;
			}
			else   if  (*cp == '>')  {
				skiprest = 1;
				cp++;
			}
			nn = 0;
			do  nn = nn * 10 + *cp++ - '0';
			while  (isdigit(*cp));

			/* Get format char */

			if  (islower(*cp))
				fp = &lowertab[*cp - 'a'];
			else  {
				if  (*cp)
					cp++;
				continue;
			}
			cp++;
			if  (!fp->fmt_fn)
				continue;
			getyx(pscr, dummy, currplace);
			inlen = (fp->fmt_fn)(pp, nn);
			lbp = bigbuff;
			if  (inlen > nn)  {
				if  (skiprest)  {
					int	wcol = currplace;
					do  {
						DISP_CHAR(pscr, *lbp);
						lbp++;
					}  while  (*lbp  &&  ++wcol < COLS);
					break;
				}
				if  (lastplace >= 0)  {
					wmove(pscr, row, lastplace);
					nn = currplace + nn - lastplace;
					inlen = (fp->fmt_fn)(pp, nn);
				}
			}
			while  (inlen > 0  &&  nn > 0)  {
				DISP_CHAR(pscr, *lbp);
				lbp++;
				inlen--;
				nn--;
			}
			if  (nn > 0)  {
				int	ccol;
				getyx(pscr, dummy, ccol);
				wmove(pscr, dummy, ccol+nn);
			}
		}
	}
}

/* Parse device entry with <>s round it.  */

static void	parse_dev(char *str)
{
	if  (strncmp(str, nsmsg, lnsmsg) == 0)  {
		int	tl;
		str += lnsmsg;
		tl = strlen(str);
		if  (tl > lnemsg  &&  strcmp(str + tl - lnemsg, nemsg) == 0)
			str[tl - lnemsg] = '\0';
		PREQ.spp_netflags = SPP_LOCALNET;
	}
	else
		PREQ.spp_netflags = 0;
	strncpy(PREQ.spp_dev, str, LINESIZE);
}

/* Add a printer.  Return 1 if not aborted.  */

static	int	addprin(void)
{
	int	newrow = Ptr_seg.nptrs - Phline + more_above;
	char	*str;

	if  (newrow >= PLINES)  {
		Phline = Peline = Ptr_seg.nptrs - 1;
		pdisplay();
		newrow = PLINES - 1;
	}

	str = chk_wgets(pscr, newrow, &wst_aptr, "", PTRNAMESIZE);
	if  (!str)
		goto  abortadd;

	strncpy(PREQ.spp_ptr, str, PTRNAMESIZE);

	str = chk_wgets(pscr, newrow, &wst_alin, "", LINESIZE);
	if  (!str)
		goto  abortadd;

	parse_dev(str);
	PREQ.spp_extrn = 0;
	current_prin = wst_aform.msg = PREQ.spp_ptr;

	wmove(pscr, newrow, (int) wst_aform.col);
	wclrtoeol(pscr);

	str = chk_wgets(pscr, newrow, &wst_aform, "", MAXFORM);
	current_prin = (char *) 0;
	if  (!str)
		goto  abortadd;

	strncpy(PREQ.spp_form, str, MAXFORM);

	str = chk_wgets(pscr, newrow, &wst_adescr, "", COMMENTSIZE);
	if  (str == (char *) 0)
		goto  abortadd;
	strncpy(PREQ.spp_comment, str, COMMENTSIZE);

	PREQ.spp_class = Displayopts.opt_classcode;

	/* Clear anyway in case scheduler decides not to...  */

	return  1;

abortadd:
	wmove(pscr, newrow, 0);
	wclrtoeol(pscr);
	return  0;
}

/* Change class of printer.  */

static	int	chngcls(void)
{
	struct  spptr  *pp = &PREQ;
	int	newrow = Ptr_seg.nptrs - Phline + more_above;
	int	row, col;
	classcode_t	newc;
	static	char	*prompt;

	if  (!prompt)
		prompt = gprompt($P{spq pclass help});

	if  (newrow >= PLINES)
		newrow = PLINES - 1;

	wmove(pscr, newrow, 0);
	wclrtoeol(pscr);
	wprintw(pscr, prompt, pp->spp_ptr);
	getyx(pscr, row, col);
	wht_pcl.col = (unsigned char) col;

	for  (;;)  {
		wh_fill(pscr, row, &wht_pcl, pp->spp_class);
		newc = whexnum(pscr, newrow, &wht_pcl, pp->spp_class);

		if  (!(mypriv->spu_flgs & PV_COVER))
			newc &= mypriv->spu_class;

		if  (newc != 0)
			break;

		disp_str = hex_disp(mypriv->spu_class, 0);
		doerror(pscr, $E{spq zero code error});
	}

	wmove(pscr, newrow, 0);
	wclrtoeol(pscr);
	if  (pp->spp_class == newc)
		return  0;
	pp->spp_class = newc;
	return  1;
}

/* Change limit on printer.  */

static	int	chnglmt(const int code, ULONG *lp)
{
	struct  spptr  *pp = &PREQ;
	int	newrow = Ptr_seg.nptrs - Phline + more_above;
	int	row, col;
	LONG	num;
	char	*cprompt;
	static	char	*prompt1, *prompt2;
	struct	sctrl	wnt_lmt;

	if  (!prompt1)  {
		prompt1 = gprompt($P{Lower job size limit});
		prompt2 = gprompt($P{Upper job size limit});
	}
	cprompt = code == $P{Lower job size limit}? prompt1: prompt2;

	if  (newrow >= PLINES)
		newrow = PLINES - 1;

	wmove(pscr, newrow, 0);
	wclrtoeol(pscr);
	wprintw(pscr, cprompt, pp->spp_ptr);
	getyx(pscr, row, col);

	wnt_lmt.helpcode = code;
	wnt_lmt.helpfn = HELPLESS;
	wnt_lmt.size = 10;
	wnt_lmt.col = (unsigned char) col;
	wnt_lmt.magic_p = MAG_P;
	wnt_lmt.min = 0L;
	wnt_lmt.vmax = 999999999L;
	wnt_lmt.msg = PREQ.spp_ptr;

	wn_fill(pscr, row, &wnt_lmt, (LONG) *lp);
	num = wnum(pscr, row, &wnt_lmt, (LONG) *lp);
	if  (num >= 0)  {
		*lp = num;
		return  1;
	}
	return  0;
}

/* Confirm that we really want to bypass alignment on printer.  */

int	confbypa(const char *msg, const int code, const struct spptr *pp)
{
	int	begy, y, x;
	WINDOW	*awin;

	getbegyx(pscr, begy, x);
	getyx(pscr, y, x);
	awin = newwin(1, 0, begy + y, 0);
	disp_str = pp->spp_ptr;
	disp_str2 = pp->spp_form;
	wprintw(awin, (char *) msg, pp->spp_ptr);
	wrefresh(awin);
	select_state(code);
	Ew = pscr;

	for  (;;)  {
		do	x = getkey(MAG_A|MAG_P);
		while  (x == EOF);

		if  (hlpscr)  {
			endhe(awin, &hlpscr);
			if  (helpclr)
				continue;
		}
		if  (escr)
			endhe(awin, &escr);

		if  (x == $K{key help})  {
			dochelp(awin, code);
			continue;
		}
		if  (x == $K{key refresh})  {
			wrefresh(curscr);
			wrefresh(awin);
			continue;
		}
		if  (x == $K{key yes}  ||  x == $K{key no})
			break;
		doerror(awin, code);
	}
	delwin(awin);
	touchwin(pscr);
	wrefresh(pscr);
	if  (x == $K{key no})
		return 0;
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
		sforwmsg = gprompt($P{Fsearch ptr});
		sbackwmsg = gprompt($P{Rsearch ptr});
	}

	ss.helpcode = $H{Fsearch ptr};
	gstr = isback? sbackwmsg: sforwmsg;
	ss.helpfn = HELPLESS;
	ss.size = 30;
	ss.retv = 0;
	ss.col = (SHORT) strlen(gstr);
	ss.magic_p = MAG_OK;
	ss.min = 0L;
	ss.vmax = 0L;
	ss.msg = NULLCP;
	row = Peline - Phline + more_above;
	mvwaddstr(pscr, row, 0, gstr);
	wclrtoeol(pscr);

	if  (lastmstr)  {
		ws_fill(pscr, row, &ss, lastmstr);
		gstr = wgets(pscr, row, &ss, lastmstr);
		if  (!gstr)
			return  NULLCP;
		if  (gstr[0] == '\0')
			return  lastmstr;
	}
	else  {
		for  (;;)  {
			gstr = wgets(pscr, row, &ss, "");
			if  (!gstr)
				return  NULLCP;
			if  (gstr[0])
				break;
			doerror(pscr, $E{Rsearch ptr});
		}
	}
	if  (lastmstr)
		free(lastmstr);
	return  lastmstr = stracpy(gstr);
}

/* Match a printer string "vstr" against a pattern string "mstr" */

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
	const  struct  spptr  *pp = &Ptr_seg.pp_ptrs[mline]->p;
	return	smatchit(pp->spp_ptr, mstr) ||
		smatchit(pp->spp_dev, mstr) ||
		smatchit(pp->spp_form, mstr);
}

/* Search for string in printer name/dev/form
   Return 0 - need to redisplay ptrs (Phline and Peline suitably mangled)
   otherwise return error code */

static	int	dosearch(const int isback)
{
	char	*mstr = gsearchs(isback);
	int	mline;

	if  (!mstr)
		return  0;

	if  (isback)  {
		for  (mline = Peline - 1;  mline >= 0;  mline--)
			if  (smatch(mline, mstr))
				goto  gotit;
		for  (mline = Ptr_seg.nptrs - 1;  mline >= Peline;  mline--)
			if  (smatch(mline, mstr))
				goto  gotit;
	}
	else  {
		for  (mline = Peline + 1;  (unsigned) mline < Ptr_seg.nptrs;  mline++)
			if  (smatch(mline, mstr))
				goto  gotit;
		for  (mline = 0;  mline <= Peline;  mline++)
			if  (smatch(mline, mstr))
				goto  gotit;
	}
	return  $E{Search ptr not found};

 gotit:
	Peline = mline;
	if  (Peline < Phline  ||  Peline - Phline + more_above + more_below >= PLINES)
		Phline = Peline;
	return  0;
}

static void	ptr_macro(const struct spptr *pp, const int num)
{
	char	*prompt = helpprmpt(num + $P{Printer macro}), *str;
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
		doerror(pscr, $E{Macro error});
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
		int	psy, psx;
		struct	sctrl	dd;
		wclrtoeol(pscr);
		waddstr(pscr, str);
		getyx(pscr, psy, psx);
		dd.helpcode = $H{Printer macro};
		dd.helpfn = HELPLESS;
		dd.size = COLS - psx;
		dd.col = psx;
		dd.magic_p = MAG_P|MAG_OK;
		dd.min = dd.vmax = 0;
		dd.msg = (char *) 0;
		str = wgets(pscr, psy, &dd, "");
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
		char	nbuf[PTRNAMESIZE+HOSTNSIZE+2];
		argbuf[0] = str;
		if  (pp)  {
			if  (pp->spp_netid)  {
				sprintf(nbuf, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
				argbuf[1] = nbuf;
			}
			else
				argbuf[1] = (char *) pp->spp_ptr;
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
		doerror(pscr, $E{Macro fork failed});
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
			doerror(pscr, $E{Macro command gave signal});
		}
		else  {
			disp_arg[0] = (status >> 8) & 255;
			doerror(pscr, $E{Macro command error});
		}
	}
}

/* This accepts input from the screen.
   Return -1 for Q refresh, 0 to exit, 1 to change to job queue.  */

int	p_process(void)
{
	int	changes = 0, currow, err_no;
	int	ch, i;
	char	*str;
	static	char	*confdelp = (char *) 0;

	Ew = pscr;

	if  (Phline >= Ptr_seg.nptrs) {
		changes++;
		if  ((Phline = Ptr_seg.nptrs - 1) < 0)
			Phline = 0;
	}

	if  (Peline < Phline)
		Peline = Phline;
	else  if  (Peline >= Ptr_seg.nptrs  &&  (Peline = Ptr_seg.nptrs - 1) < 0)
		Peline = 0;

	if  (changes)  {
		pdisplay();
		changes = 0;
	}

	select_state($S{spq ptr cmd state});

#ifdef HAVE_TERMINFO
	wnoutrefresh(jscr);
#else
	wrefresh(jscr);
#endif
Pmove:
	currow = Peline - Phline + more_above;
	wmove(pscr, currow, 0);

Prefresh:
#ifdef HAVE_TERMINFO
	wnoutrefresh(pscr);
	doupdate();
#else
	wrefresh(pscr);
#endif
nextin:
	if  (hadrfresh)
		return  -1;

	if  (hadalarm != lastalarm)  {
		hadalarm = lastalarm;
		if  (Job_seg.dptr->js_serial != Job_seg.Last_ser)  {
			rpfile();
			pdisplay();
			readjoblist(1);
			jdisplay();
#ifdef	HAVE_TERMINFO
			wnoutrefresh(jscr);
#else
			wrefresh(jscr);
#endif
			goto  Pmove;
		}
		if  (Ptr_seg.dptr->ps_serial != Ptr_seg.Last_ser)  {
			rpfile();
			pdisplay();
			goto  Pmove;
		}
	}

 nextin2:
	do  ch = getkey(MAG_A|MAG_P);
	while  (ch == EOF  &&  (hlpscr || escr));

	if  (hlpscr)  {
		endhe(pscr, &hlpscr);
		if  (helpclr)
			goto  nextin2;
	}
	if  (escr)
		endhe(pscr, &escr);

	switch  (ch)  {

	case  EOF:
		goto  nextin;

	default:
		err_no = $E{spq ptr unknown cmd};
	perr:
		doerror(pscr, err_no);
		goto  nextin;

	case  $K{key help}:
		dochelp(pscr, $H{spq ptr cmd state});
		goto  nextin;

	case  $K{key refresh}:
		wrefresh(curscr);
		goto  Prefresh;		/*  Must refresh to restore curs */

	/* Move up or down with.  */

	case  $K{key cursor down}:
		Peline++;
		if  (Peline >= Ptr_seg.nptrs)  {
			Peline--;
bpr:			err_no = $E{spq plist off bottom};
			goto  perr;
		}
		if  (++currow >= PLINES - more_below)  {
			Phline++;
			if  (!more_above  &&  PLINES > 2)
				Phline++;
			pdisplay();
		}
		goto  Pmove;

	case  $K{key cursor up}:
		if  (Peline <= 0)  {
epr:			err_no = $E{spq plist off top};
			goto  perr;
		}
		Peline--;
		if  (--currow < more_above)  {
			Phline = Peline;
			currow = more_above;
			if (more_above  &&  Peline == 1)
				Phline = 0;
			pdisplay();
		}
		goto  Pmove;

	/* Up/Down full or half screen */

	case  $K{key screen down}:
		if  (Phline + PLINES - more_above >= Ptr_seg.nptrs)
			goto  bpr;
		Phline += PLINES - more_above - more_below;
		Peline += PLINES - more_above - more_below;
		if  (Peline >= Ptr_seg.nptrs)
			Peline = Ptr_seg.nptrs - 1;
redr:
		pdisplay();
		currow = Peline - Phline + more_above;
		if  (more_below  &&  currow > PLINES - 2)
			Peline--;
		goto  Pmove;

	case  $K{key half screen down}:
		i = (PLINES - more_above - more_below) / 2;
		if  (Phline + i >= Ptr_seg.nptrs  ||  (Phline == 0  &&  (i + 1) >= Ptr_seg.nptrs))
			goto  bpr;
		if  ((Phline += i) <= 1)
			Phline = 2;
		currow -= i;
		if  (Peline < Phline)
			Peline = Phline;
		goto  redr;

	case  $K{key half screen up}:
		if  (Phline <= 0)
			goto  epr;
		Phline -= (PLINES - more_above - more_below) / 2;
	restu:
		if  (Phline <= 1)
			Phline = 0;
		pdisplay();
		i = PLINES - more_above - more_below;
		if  (Peline - Phline >= i)
			Peline = Phline + i - 1;
		goto  Pmove;

	case  $K{key screen up}:
		if  (Phline <= 0)
			goto  epr;
		Phline -= PLINES - more_below - more_above;
		goto  restu;

	case  $K{key top}:
		if  (Phline != Peline  &&  Peline != 0)  {
			Peline = Phline < 0? 0: Phline;
			goto  Pmove;
		}
		Phline = 0;
		Peline = 0;
		goto  redr;

	case  $K{key bottom}:
		i = Phline + PLINES - more_above - more_below - 1;
		if  (Peline < i  &&  i < Ptr_seg.nptrs - 1)  {
			Peline = i;
			goto  Pmove;
		}
		if  (Ptr_seg.nptrs > PLINES)
			Phline = PLINES < 3? Ptr_seg.nptrs - PLINES: Ptr_seg.nptrs - PLINES + 1;
		else
			Phline = 0;
		Peline = Ptr_seg.nptrs - 1;
		goto  redr;

	case  $K{key forward search}:
	case  $K{key backward search}:
		if  ((err_no = dosearch(ch == $K{key backward search})) != 0)
			doerror(pscr, err_no);
		goto  redr;

	/* Go to other window.  */

	case  $K{spq key job window}:
		return  1;

	/* Go home.  */

	case  $K{key halt}:
		return  0;

	case  $K{key save opts}:
		propts();
		goto  refill;

	case  $K{spq key view sys err}:
		if  ((ch = view_errors(0)) <= 0)  {
			err_no = ch < 0? $E{Log file nomem}: $E{No log file yet};
			goto  perr;
		}
	refill:
		select_state($S{spq ptr cmd state});
		Ew = pscr;
#ifdef	CURSES_MEGA_BUG
		clear();
		refresh();
#endif
		if  (hjscr)
			touchwin(hjscr);
		touchwin(jscr);
		if  (hpscr)
			touchwin(hpscr);
		if  (tpscr)
			touchwin(tpscr);
		touchwin(pscr);
#ifdef HAVE_TERMINFO
		if  (hjscr)
			wnoutrefresh(hjscr);
		if  (hpscr)
			wnoutrefresh(hpscr);
		if  (tpscr)
			wnoutrefresh(tpscr);
		wnoutrefresh(jscr);
#else
		if  (hjscr)
			wrefresh(hjscr);
		if  (hpscr)
			wrefresh(hpscr);
		if  (tpscr)
			wrefresh(tpscr);
		wrefresh(jscr);
#endif
		goto  Pmove;

	/* Now for the stuff to change things.  */

	case  $K{spq key halt printer}:
	case  $K{spq key stop printer}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		if  (!(mypriv->spu_flgs & PV_HALTGO))
			goto  nohgperm;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state < SPP_PROC)
			goto  notrun;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))  {
		notrem:
			err_no = $E{No remote ptr priv};
			goto  perr;
		}
		OREQ = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		womsg(ch == $K{spq key stop printer}? SO_PSTP: SO_PHLT);
		goto  nextin;

	case  $K{spq key start printer}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		if  (!(mypriv->spu_flgs & PV_HALTGO))
			goto  nohgperm;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state >= SPP_PROC  &&
		     !(Ptr_seg.pp_ptrs[Peline]->p.spp_sflags & SPP_HEOJ))
			goto  notstop;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))
			goto  notrem;
		OREQ = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		womsg(SO_PGO);
		goto  nextin;

	case  $K{spq key interrupt printer}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		if  (!(mypriv->spu_flgs & PV_HALTGO))
			goto  nohgperm;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state != SPP_RUN)
			goto  notrun;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))
			goto  notrem;
		OREQ = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		womsg(SO_INTER);
		goto  nextin;

	case  $K{spq key restart printer}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state != SPP_RUN)
			goto  notrun;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))
			goto  notrem;
		OREQ = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		womsg(SO_RSP);
		goto  nextin;

	case  $K{spq key printer yes}:
	case  $K{spq key printer no}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))
			goto  notrem;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state != SPP_OPER)  {
			int	res;
			const  struct  spptr  *pp = &Ptr_seg.pp_ptrs[Peline]->p;
			if  (pp->spp_state != SPP_WAIT)
				goto  notaw;
			res = ch == $K{spq key printer yes}?
				confbypa(bypassa, $HE{Bypass align}, pp):
				confbypa(reinsta, $HE{Reinstate align}, pp);
			select_state($S{spq ptr cmd state});
			Ew = pscr;
			if  (!res)
				goto  nextin;
		}
		OREQ = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		womsg(ch == $K{spq key printer yes}? SO_OYES: SO_ONO);
		goto  nextin;

	case  $K{spq key delete printer}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid)  {
			err_no = $E{Cannot delete remote printers};
			goto  perr;
		}
		if  (!(mypriv->spu_flgs & PV_ADDDEL))
			goto  noadddel;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state >= SPP_PROC)
			goto  notstop;
		if  (!confdelp)
			confdelp = gprompt($P{Confirm delete printer});
		ch = confbypa(confdelp, $HE{Confirm delete printer}, &Ptr_seg.pp_ptrs[Peline]->p);
		select_state($S{spq ptr cmd state});
		Ew = pscr;
		if  (ch)  {
			OREQ = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
			womsg(SO_DELP);
		}
		goto  nextin;

	case  $K{spq key abort printer}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state != SPP_RUN)
			goto  notrun;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))
			goto  notrem;
		OREQ = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		womsg(SO_PJAB);
		goto  nextin;

	case  $K{spq key printer}:
	case  $K{spq key device}:
	case  $K{spq key description}:
	case  $K{spq key ptr local only}:
	case  $K{spq key ptr exported}:

		if  (!(mypriv->spu_flgs & PV_ADDDEL))
			goto  noadddel;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid)  {
			err_no = $E{Cannot delete remote printers};
			goto  perr;
		}

	case  $K{spq key printer form}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state >= SPP_PROC)
			goto  notstop;

		/* Copy details of job into request buffer
		   before we start munging it.  */

		PREQ = Ptr_seg.pp_ptrs[Peline]->p;
		PREQS = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		if  (PREQ.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))
			goto  notrem;

		current_prin = wst_nform.msg = PREQ.spp_ptr;

		switch  (ch)  {
		case  $K{spq key printer form}:
			str = chk_wgets(pscr, currow, &wst_nform, PREQ.spp_form, MAXFORM);
			current_prin = (char *) 0;
			if  (str != (char *) 0)  {
				changes++;
				strncpy(PREQ.spp_form, str, MAXFORM);
			}
			break;

		case  $K{spq key printer}:
			str = chk_wgets(pscr, currow, &wst_aptr, PREQ.spp_ptr, PTRNAMESIZE);
			if  (str != (char *) 0)  {
				changes++;
				strncpy(PREQ.spp_ptr, str, PTRNAMESIZE);
			}
			break;

		case  $K{spq key device}:
		{
			char *dstr;
			if  (!(dstr = malloc((unsigned) (lnsmsg + lnemsg + LINESIZE + 1))))
				nomem();
			if  (PREQ.spp_netflags & SPP_LOCALNET)
				sprintf(dstr, "%s%s%s", nsmsg, PREQ.spp_dev, nemsg);
			else
				strcpy(dstr, PREQ.spp_dev);
			str = chk_wgets(pscr, currow, &wst_alin, dstr, LINESIZE);
			if  (str != (char *) 0)  {
				changes++;
				parse_dev(str);
			}
			free(dstr);
		}
			break;

		case  $K{spq key description}:
			str = chk_wgets(pscr, currow, &wst_adescr, PREQ.spp_comment, COMMENTSIZE);
			if  (str != (char *) 0)  {
				changes++;
				if  (*str)
					strncpy(PREQ.spp_comment, str, COMMENTSIZE);
			}
			break;

		case  $K{spq key ptr local only}:
			if  (PREQ.spp_netflags & SPP_LOCALONLY)
				goto  nextin2;
			PREQ.spp_netflags |= SPP_LOCALONLY;
			changes++;
			break;

		case  $K{spq key ptr exported}:
			if  (!(PREQ.spp_netflags & SPP_LOCALONLY))
				goto  nextin2;
			PREQ.spp_netflags &= ~SPP_LOCALONLY;
			changes++;
			break;
		}
		if  (changes)  {
			changes = 0;
			my_wpmsg(SP_CHGP);
		}
		goto  Pmove;

	case  $K{spq key add printer}:
		if  (!(mypriv->spu_flgs & PV_ADDDEL))
			goto  noadddel;
		if  (addprin())
			my_wpmsg(SP_ADDP);
		goto redr;

	case  $K{spq key printer class}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		PREQ = Ptr_seg.pp_ptrs[Peline]->p;
		PREQS= Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		if  (!(mypriv->spu_flgs & PV_ADDDEL))
			goto  noadddel;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_state >= SPP_PROC)
			goto  notstop;
		if  (Ptr_seg.pp_ptrs[Peline]->p.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))
			goto  notrem;
		if  (chngcls())
			my_wpmsg(SP_CHGP);
		select_state($S{spq ptr cmd state});
		pdisplay();
		goto  Pmove;

	case  $K{spq key lower limit}:
	case  $K{spq key upper limit}:
		if  (Ptr_seg.nptrs == 0)
			goto  noprinsyet;
		PREQ = Ptr_seg.pp_ptrs[Peline]->p;
		PREQS = Ptr_seg.pp_ptrs[Peline] - Ptr_seg.plist;
		if  (!(mypriv->spu_flgs & PV_HALTGO))
			goto  nohgperm;
		if  (PREQ.spp_state >= SPP_PROC)
			goto  notstop;
		if  (PREQ.spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))
			goto  notrem;
		if  (ch == $K{spq key lower limit})
			ch = chnglmt($PH{Lower job size limit}, &PREQ.spp_minsize);
		else
			ch = chnglmt($PH{Upper job size limit}, &PREQ.spp_maxsize);
		if  (ch)
			my_wpmsg(SP_CHGP);
		pdisplay();
		goto  Pmove;

	case  $K{spq key printer format}:
		if  (!(mypriv->spu_flgs & PV_ACCESSOK))  {
			err_no = $E{spq no access privilege};
			goto  perr;
		}
		err_no = fmtprocess(&ptr_format, 'Y', (struct sq_formatdef *) 0, lowertab);
		str = get_ptrtitle();
		if  (wh_ptitline >= 0)  {
			wmove(hpscr, wh_ptitline, 0);
			wclrtoeol(hpscr);
			waddstr(hpscr, str);
		}
		free(str);
		if  (err_no)	/* Records changes */
			offersave(ptr_format, $P{Spq ptr default format});
		pdisplay();
		goto  refill;

	noprinsyet:
		err_no = $E{No printers yet};
		goto	perr;
	nohgperm:
		err_no = $E{No halt go priv};
		goto  perr;
	noadddel:
		err_no = $E{No add priv};
		goto  perr;
	notrun:
		err_no = $E{Printer not running};
		goto  perr;
	notstop:
		err_no = $E{Printer is running};
		goto  perr;
	notaw:
		err_no = $E{Printer not aw oper};
		goto  perr;

	case $K{key exec}:  case $K{key exec}+1:case $K{key exec}+2:case $K{key exec}+3:case $K{key exec}+4:
	case $K{key exec}+5:case $K{key exec}+6:case $K{key exec}+7:case $K{key exec}+8:case $K{key exec}+9:
		ptr_macro(Peline >= Ptr_seg.nptrs? (const struct spptr *) 0: &Ptr_seg.pp_ptrs[Peline]->p, ch - $K{key exec});
		pdisplay();
		if  (escr)  {
			touchwin(escr);
			wrefresh(escr);
		}
		goto  Pmove;
	}
}
