/* sq_propts.c -- spq program option handling

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
#include <sys/stat.h>
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
#include "helpargs.h"
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

extern  void  dochelp(WINDOW *, const int);
extern  void  doerror(WINDOW *, const int);
extern  void  endhe(WINDOW *, WINDOW **);

extern  int  spitoption(const int, const int, FILE *, const int, const int);
extern  int  proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

extern  char **wotjform(const char *, const int);
extern  char **wotjprin(const char *, const int);
#ifndef	HAVE_ATEXIT
extern  void  exit_cleanup();
#endif

extern	struct	spdet	*mypriv;

#define	P_DONT_CARE	100	/* Must move this!!! */

extern	char	scrkeep,
		helpclr,
		helpbox,
		errbox,
		nopage,
		pfirst,
		confabort;

extern	char	*Curr_pwd,
		*spdir;

extern	int	hadrfresh;
extern	unsigned	Pollinit;

extern	int	PLINES;
int	int_PLINES;		/* One we save */

#define	NULLCH		((char *) 0)

#define	NUMPROMPTS	3

struct	ltab	{
	int	helpcode;
	char	*message;
	char	*prompts[NUMPROMPTS];
	char	row, col, size;
	void	(*dfn)(const struct ltab *);
	int	(*fn)(const struct ltab *);
};

static void  prd_printer(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, Displayopts.opt_restrp? Displayopts.opt_restrp: "");
}

static void  prd_title(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, Displayopts.opt_restrt? Displayopts.opt_restrt: "");
}

static void  prd_incjobs(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, Displayopts.opt_jinclude==JINCL_ALL? lt->prompts[2]:
				   Displayopts.opt_jinclude==JINCL_NULL? lt->prompts[1]:
				   lt->prompts[0]);
}

static void  prd_user(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, Displayopts.opt_restru? Displayopts.opt_restru: "");
}

static void  prd_unprinted(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, Displayopts.opt_jprindisp==JRESTR_PRINT? lt->prompts[2]:
				   Displayopts.opt_jprindisp==JRESTR_UNPRINT? lt->prompts[1]:
				   lt->prompts[0]);
}

static void  prd_localonly(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, Displayopts.opt_localonly != NRESTR_NONE? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_psort(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, Displayopts.opt_sortptrs != SORTP_NONE? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_class(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, hex_disp(Displayopts.opt_classcode, 1));
}

static void  prd_confabort(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, confabort > 1? lt->prompts[2]: confabort? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_helpbox(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, helpbox? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_errbox(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, errbox? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_helpclr(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, helpclr? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_nopage(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, nopage? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_scrkeep(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, scrkeep? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_plist(const struct ltab *lt)
{
	mvprintw(lt->row, lt->col, "%*d", lt->size, int_PLINES);
}

static void  prd_pfirst(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, pfirst == P_DONT_CARE? lt->prompts[2]: pfirst? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_pollinit(const struct ltab *lt)
{
	mvprintw(lt->row, lt->col, "%*d", lt->size, Pollinit);
}

static int  pro_string(const struct ltab *lt, char **strp, helpfn_t helpfn)
{
	char	*str, *orig = *strp? *strp: "";
	struct	sctrl	wst_str;

	wst_str.helpcode = lt->helpcode * 10;
	wst_str.helpfn = wotjprin;
	wst_str.size = lt->size;
	wst_str.col = lt->col;
	wst_str.magic_p = MAG_OK|MAG_R|MAG_CRS|MAG_NL;
	wst_str.msg = (char *) 0;

	str = wgets(stdscr, lt->row, &wst_str, orig);
	if  (str != (char *) 0  &&  strcmp(str, orig) != 0)  {
		if  (*strp)  {
			free(*strp);
			*strp = (char *) 0;
		}
		if  (str[0] == '\0')
			ws_fill(stdscr, lt->row, &wst_str, "");
		else
			*strp = stracpy(str);
		hadrfresh++;		/*  Force reread jobs/printers */
		Job_seg.Last_ser = 0;
		Ptr_seg.Last_ser = 0;
		return  $K{key eol};
	}
	return  wst_str.retv == 0? $K{key eol}: wst_str.retv;
}

static int  pro_printer(const struct ltab *lt)
{
	return  pro_string(lt, &Displayopts.opt_restrp, wotjprin);
}

static int  pro_title(const struct ltab *lt)
{
	return  pro_string(lt, &Displayopts.opt_restrt, HELPLESS);
}

static int  pro_user(const struct ltab *lt)
{
	return  pro_string(lt, &Displayopts.opt_restru, gen_ulist);
}

static	int  pro_bool(const struct ltab *lt, unsigned *b)
{
	int	ch;
	unsigned  origb = *b;

	for  (;;)  {
		move(lt->row, lt->col);
		refresh();
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
			doerror(stdscr, $E{spq options unknownc});

		case  EOF:
			continue;

		case  $K{key help}:
			dochelp(stdscr, lt->helpcode * 10);
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			refresh();
			continue;

		case  $K{key guess}:
			++*b;
			if  (*b >= NUMPROMPTS  ||  lt->prompts[*b] == (char *) 0)
				*b = 0;
			clrtoeol();
			addstr(lt->prompts[*b]);
			continue;

		case  $K{key erase}:
			*b = origb;

		case  $K{key halt}:
		case  $K{key cursor down}:
		case  $K{key eol}:
		case  $K{key cursor up}:
			return  ch;
		}
	}
}

static int  pro_incjobs(const struct ltab *lt)
{
	unsigned  current = (unsigned) Displayopts.opt_jinclude;
	int	ch = pro_bool(lt, &current);

	if  (current != (unsigned) Displayopts.opt_jinclude)  {
		Displayopts.opt_jinclude = (enum jincl_t) current;
		hadrfresh++;		/*  Force reread jobs/printers */
		Job_seg.Last_ser = 0;
	}
	return  ch;
}

static int  pro_unprinted(const struct ltab *lt)
{
	unsigned  current = (unsigned) Displayopts.opt_jprindisp;
	int	ch = pro_bool(lt, &current);

	if  (current != (unsigned) Displayopts.opt_jprindisp)  {
		Displayopts.opt_jprindisp = (enum jrestrict_t) current;
		hadrfresh++;		/*  Force reread jobs/printers */
		Job_seg.Last_ser = 0;
	}
	return  ch;
}

static int  pro_localonly(const struct ltab *lt)
{
	unsigned  current = (unsigned) Displayopts.opt_localonly;
	int	ch = pro_bool(lt, &current);

	if  (current != (unsigned) Displayopts.opt_localonly)  {
		Displayopts.opt_localonly = (enum netrestrict_t) current;
		hadrfresh++;		/*  Force reread jobs/printers */
		Job_seg.Last_ser = 0;
		Ptr_seg.Last_ser = 0;
	}
	return  ch;
}

static int  pro_psort(const struct ltab *lt)
{
	unsigned	current = (unsigned) Displayopts.opt_sortptrs;
	int	ch = pro_bool(lt, &current);

	if  (current != (unsigned) Displayopts.opt_sortptrs)  {
		Displayopts.opt_sortptrs = (enum sortp_t) current;
		hadrfresh++;		/*  Force reread jobs/printers */
		Ptr_seg.Last_ser = 0;
	}
	return  ch;
}

static int  pro_class(const struct ltab *lt)
{
	classcode_t	in;
	struct	sctrl	wht_cl;

	wht_cl.helpcode = lt->helpcode * 10;
	wht_cl.helpfn = HELPLESS;
	wht_cl.size = lt->size;
	wht_cl.col = lt->col;
	wht_cl.magic_p = MAG_P|MAG_R;
	wht_cl.min = 1L;
	wht_cl.vmax = LOTSANDLOTS;
	wht_cl.msg = (char *) 0;

	in = whexnum(stdscr, lt->row, &wht_cl, Displayopts.opt_classcode);
	reset_state();
	if  (in == Displayopts.opt_classcode  &&  wht_cl.retv == $K{key halt})
		return  wht_cl.retv;

	if  (!(mypriv->spu_flgs & PV_COVER))
		in &= mypriv->spu_class;

	if  (in == 0L)  {
		mvaddstr(lt->row, lt->col, hex_disp(Displayopts.opt_classcode, 1));
		disp_str = hex_disp(mypriv->spu_class, 0);
		doerror(stdscr, $E{spq options eff zero class});
		return  0;
	}

	Displayopts.opt_classcode = in;
	hadrfresh++;		/*  Force reread jobs/printers */
	Job_seg.Last_ser = 0;
	Ptr_seg.Last_ser = 0;
	return  wht_cl.retv? wht_cl.retv: $K{key eol};
}

static int  pro_confabort(const struct ltab *lt)
{
	unsigned  current = confabort;
	int	ch = pro_bool(lt, &current);

	if  (current != confabort)
		confabort = (char) current;
	return  ch;
}

static int  pro_helpbox(const struct ltab *lt)
{
	unsigned  current = helpbox;
	int	ch = pro_bool(lt, &current);

	if  (current != helpbox)
		helpbox = (char) current;
	return  ch;
}

static int  pro_errbox(const struct ltab *lt)
{
	unsigned  current = errbox;
	int	ch = pro_bool(lt, &current);

	if  (current != errbox)
		errbox = (char) current;
	return  ch;
}

static int  pro_helpclr(const struct ltab *lt)
{
	unsigned  current = helpclr;
	int	ch = pro_bool(lt, &current);

	if  (current != helpclr)
		helpclr = (char) current;
	return  ch;
}

static int  pro_nopage(const struct ltab *lt)
{
	unsigned  current = nopage;
	int	ch = pro_bool(lt, &current);

	if  (current != nopage)  {
		nopage = (char) current;
		hadrfresh++;		/*  Force reread jobs/printers */
		Job_seg.Last_ser = 0;
	}
	return  ch;
}

static int  pro_scrkeep(const struct ltab *lt)
{
	unsigned  current = scrkeep;
	int	ch = pro_bool(lt, &current);

	if  (current != scrkeep)
		scrkeep = (char) current;
	return  ch;
}

static int  pro_plist(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_npt;

	wnt_npt.helpcode = lt->helpcode * 10;
	wnt_npt.helpfn = HELPLESS;
	wnt_npt.size = lt->size;
	wnt_npt.col = lt->col;
	wnt_npt.magic_p = MAG_P|MAG_R;
	wnt_npt.min = 1L;
	wnt_npt.vmax = MAX_PLINES;
	wnt_npt.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_npt, (LONG) int_PLINES);
	if  (in < 0)  {
		if  (wnt_npt.retv)
			return  wnt_npt.retv;
		return	0;
	}
	if  (int_PLINES != in)
		int_PLINES = in;
	return  $K{key eol};
}

static int  pro_pfirst(const struct ltab *lt)
{
	unsigned  current = pfirst == P_DONT_CARE? 2: pfirst? 1: 0;
	unsigned  orig = current;
	int	ch = pro_bool(lt, &current);

	if  (current != orig)
		pfirst = (char) (current > 1? P_DONT_CARE: current);
	return  ch;
}

static int  pro_pollinit(const struct ltab *lt)
{
	LONG	in;
	struct	sctrl	wnt_pt;

	wnt_pt.helpcode = lt->helpcode * 10;
	wnt_pt.helpfn = HELPLESS;
	wnt_pt.size = lt->size;
	wnt_pt.col = lt->col;
	wnt_pt.magic_p = MAG_P|MAG_R;
	wnt_pt.min = POLLMIN;
	wnt_pt.vmax = POLLMAX;
	wnt_pt.msg = (char *) 0;

	in = wnum(stdscr, lt->row, &wnt_pt, (LONG) Pollinit);
	if  (in < 0)  {
		if  (wnt_pt.retv)
			return  wnt_pt.retv;
		return	0;
	}

	if  (Pollinit != in)
		Pollinit = in;
	return  $K{key eol};
}

static	struct	ltab  ltab[] = {
{	$PHN{spq opt rptr}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, JPTRNAMESIZE, prd_printer, pro_printer	},
{	$PHN{spq opt rusr}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, UIDSIZE, prd_user, pro_user			},
{	$PHN{spq opt rtitle}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, MAXTITLE, prd_title, pro_title		},
{	$PHN{spq opt limitj}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_incjobs, pro_incjobs			},
{	$PHN{spq opt disponly}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_unprinted, pro_unprinted		},
{	$PHN{spq opt loco}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_localonly, pro_localonly		},
{	$PHN{spq opt sort}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_psort, pro_psort			},
{	$PHN{spq opt class}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 32, prd_class, pro_class			},
{	$PHN{spq opt confdel}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_confabort, pro_confabort		},
{	$PHN{spq opt helpbox}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_helpbox, pro_helpbox		},
{	$PHN{spq opt errbox}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_errbox, pro_errbox			},
{	$PHN{spq opt helpclr}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_helpclr, pro_helpclr		},
{	$PHN{spq opt pagec}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_nopage, pro_nopage			},
{	$PHN{spq opt curs}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_scrkeep, pro_scrkeep			},
{	$PHN{spq opt plist}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 6, prd_plist, pro_plist			},
{	$PHN{spq opt cursentry}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_pfirst, pro_pfirst		},
{	$PHN{spq opt refresh}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 3, prd_pollinit, pro_pollinit		}
};

#define	TABNUM	(sizeof(ltab)/sizeof(struct ltab))

static	struct	ltab	*lptrs[TABNUM+1];
static	int	comeinat;
static	char	**title;

static	void  initnames()
{
	int	i, next;
	struct  ltab	*lt;
	int	curr, hrows, cols, look4, rowstart, pstart;
	int	nextstate[TABNUM];

	title = helphdr('=');
	count_hv(title, &hrows, &cols);

	/* Slurp up standard messages */

	if  ((rowstart = helpnstate($N{spq opt start row})) <= 0)
		rowstart = $N{spq opt rptr};
	if  ((pstart = helpnstate($N{spq opt init cursor})) <= 0)
		pstart = rowstart;

	if  (rowstart < $P{spq opt rptr} || rowstart >= $P{spq opt rptr} + TABNUM)  {
		disp_arg[9] = rowstart;
	bads:
		doerror(stdscr, $E{spq options bad state code});
		refresh();
		do  i = getkey(MAG_A|MAG_P);
		while	(i == EOF);
#ifndef	HAVE_ATEXIT
		exit_cleanup();
#endif
		exit(E_BADCFILE);
	}

	if  (pstart < $P{spq opt rptr} || pstart >= $P{spq opt rptr} + TABNUM)  {
		disp_arg[9] = pstart;
		goto  bads;
	}

	for  (i = 0, lt = &ltab[0]; lt < &ltab[TABNUM]; i++, lt++)  {
		lt->message = gprompt(lt->helpcode);
		lt->col = strlen(lt->message) + 1;
	}

	/* Do this in a second loop to avoid too many reads...  */

	for  (i = 0;  i < TABNUM;  i++)  {
		int	j;
		for  (j = 0;  j < NUMPROMPTS;  j++)  {
			char	*pr = helpprmpt(ltab[i].helpcode * 10 + j);
			if  (!pr)
				break;
			ltab[i].prompts[j] = pr;
		}
	}

	for  (i = 0; i < TABNUM; i++)
		nextstate[i] = -1;

	look4 = rowstart;
	do  {
		next = helpnstate(look4);
		if  (next >= 0)  {
			if  (next < $N{spq opt rptr} || next >= $N{spq opt rptr} + TABNUM)  {
				disp_arg[9] = next;
				goto  bads;
			}
			if  (nextstate[next-$N{spq opt rptr}] > 0)  {
				disp_arg[9] = next;
				disp_arg[8] = nextstate[next-$N{spq opt rptr}];
				doerror(stdscr, $E{spq options duplicated state code});
				refresh();
				do  i = getkey(MAG_A|MAG_P);
				while	(i == EOF);
#ifndef	HAVE_ATEXIT
				exit_cleanup();
#endif
				exit(E_BADCFILE);
			}
		}
		nextstate[look4 - $N{spq opt rptr}] = next;
		look4 = next;
	}  while  (next > 0);

	comeinat = 0;
	next = 0;
	curr = rowstart;
	do  {
		if  (pstart == curr)
			comeinat = next;
		lt = &ltab[curr-$N{spq opt rptr}];
		lt->row = hrows + next;
		lptrs[next] = lt;
		next++;
		curr = nextstate[curr-$N{spq opt rptr}];
	}  while  (curr > 0);
	lptrs[next] = (struct ltab *) 0;
}

static	int  askyorn(const int code)
{
	char	*prompt = gprompt(code);
	int	ch;

	select_state($S{spq ask yorn state});
	clear();
	mvaddstr(LINES/2, 0, prompt);
	addch(' ');
	refresh();
	if  (escr)  {
		touchwin(escr);
		wrefresh(escr);
		refresh();
	}
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
			doerror(stdscr, $E{Unknown command askyorn});

		case  EOF:
			continue;

		case  $K{key help}:
			dochelp(stdscr, code);
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			refresh();
			continue;

		case  $K{key yes}:
			clear();
			refresh();
			return  1;

		case  $K{key no}:
			clear();
			refresh();
			return  0;
		}
	}
}

void  spit_options(FILE *dest, const char *name)
{
	int	cancont = 0, cnt;
	fprintf(dest, "%s", name);

	cancont = spitoption(helpbox? $A{spq help box}: $A{spq no help box}, $A{spq explain}, dest, '=', cancont);
	cancont = spitoption(errbox? $A{spq error box}: $A{spq no error box}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption(helpclr? $A{spq losechar}: $A{spq keepchar}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption($A{spq no confirm abort}, $A{spq explain}, dest, ' ', cancont);
	for  (cnt = 0;  cnt < confabort;  cnt++)
		cancont = spitoption($A{spq confirm abort}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption(nopage? $A{spq no page counts}: $A{spq page counts}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption(scrkeep? $A{spq cursor keep}: $A{spq cursor follow}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption(Displayopts.opt_localonly != NRESTR_NONE? $A{spq local}: $A{spq remotes}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption(pfirst == P_DONT_CARE? $A{spq dont care}: pfirst? $A{spq ptr screen}: $A{spq jobs screen}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption(Displayopts.opt_jprindisp == JRESTR_PRINT? $A{spq printed jobs}:
			     Displayopts.opt_jprindisp == JRESTR_UNPRINT? $A{spq unprinted jobs}:
			     $A{spq all jobs}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption(Displayopts.opt_jinclude == JINCL_ALL? $A{spq include all}:
				Displayopts.opt_jinclude == JINCL_NULL? $A{spq include null}:
				$A{spq no include null}, $A{spq explain}, dest, ' ', cancont);
	cancont = spitoption(Displayopts.opt_sortptrs != SORTP_NONE? $A{spq ptrs sorted}: $A{spq ptrs unsorted}, $A{spq explain}, dest, ' ', cancont);
	spitoption($A{spq only queue}, $A{spq explain}, dest, ' ', 0);
	fprintf(dest, Displayopts.opt_restrp? " \'%s\'": " -", Displayopts.opt_restrp);
	spitoption($A{spq just user}, $A{spq explain}, dest, ' ', 0);
	fprintf(dest, Displayopts.opt_restru? " \'%s\'": " -", Displayopts.opt_restru);
	spitoption($A{spq just title}, $A{spq explain}, dest, ' ', 0);
	fprintf(dest, Displayopts.opt_restrt? " \'%s\'": " -", Displayopts.opt_restrt);
	spitoption($A{spq classcode}, $A{spq explain}, dest, ' ', 0);
	fprintf(dest, " %s", hex_disp(Displayopts.opt_classcode, 0));
	spitoption($A{spq number printers}, $A{spq explain}, dest, ' ', 0);
	fprintf(dest, " %d", int_PLINES);
	spitoption($A{spq refreshtime}, $A{spq explain}, dest, ' ', 0);
	fprintf(dest, " %d\n", Pollinit);
}

static	void  ask_build()
{
	char	*hd;
	int	ret, i;
	static	char	spq[] = "SPQ";

	if  (!askyorn($P{Save parameters}))
		return;

	hd = envprocess("${HOME-/}");
	if  (strcmp(Curr_pwd, hd) == 0)  {
		disp_str = hd;
		if  (!askyorn($P{Save in current home directory}))  {
			free(hd);
			return;
		}
	}
	else  {
		disp_str = Curr_pwd;
		if  (askyorn($P{Save in current directory}))  {
			if  ((ret = proc_save_opts(Curr_pwd, spq, spit_options)) == 0)  {
				free(hd);
				return;
			}
			disp_str = Curr_pwd;
			doerror(stdscr, ret);
		}
		disp_str = hd;
		if  (!askyorn($P{Save in home directory}))  {
			free(hd);
			return;
		}
	}

	if  ((ret = proc_save_opts(hd, spq, spit_options)) == 0)  {
		free(hd);
		return;
	}
	disp_str = hd;
	doerror(stdscr, ret);
	free(hd);
	do  i = getkey(MAG_A|MAG_P);
	while	(i == EOF);
}

/* This accepts input from the screen.  */

int  propts()
{
	int	ch, i, whichel;
	struct  ltab	*lt;
	char	**hv;
	static	char	doneinit = 0;

	if  (!doneinit)  {
		doneinit = 1;
		int_PLINES = PLINES;
		initnames();
	}

	Ew = stdscr;
	whichel = comeinat;
	clear();
	if  (title)  for  (hv = title, i = 0;  *hv;  i++, hv++)
		mvwhdrstr(stdscr, i, 0, *hv);
	for  (i = 0;  (lt = lptrs[i]);  i++)  {
		lt = lptrs[i];
		mvaddstr(lt->row, 0, lt->message);
		(*lt->dfn)(lt);
	}
#ifdef	CURSES_OVERLAP_BUG
	touchwin(stdscr);
#endif
	refresh();
	reset_state();

	while  ((lt = lptrs[whichel]))  {
		lt = lptrs[whichel];
		ch = (*lt->fn)(lt);
		switch  (ch)  {
		default:
		case  $K{key eol}:
		case  $K{key cursor down}:
			whichel++;
			break;
		case  $K{key erase}:
		case  $K{key cursor up}:
			if  (--whichel < 0)
				whichel = 0;
			break;
		case  $K{key halt}:
			goto  ret;
		}
	}
 ret:
	if  (mypriv->spu_flgs & PV_FREEZEOK)
		ask_build();
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
	return  1;
}

/* Variants of wgets and wnum for when we are possibly not displaying
   the relevant item.  */

char  *chk_wgets(WINDOW *w, const int row, struct sctrl *sc, const char *existing, const int minsize)
{
	int	begy, y, x;
	char	*prompt, *ret;
	WINDOW	*awin;

	/* If we are displaying it in the job/variable list, just
	   carry on as usual.  */

	if  (sc->col >= 0)
		return  wgets(w, row, sc, existing);

	if  ((int) sc->size < minsize)
		sc->size = (USHORT) minsize;

	getbegyx(w, begy, x);
	getyx(w, y, x);

	if  (!(awin = newwin(1, 0, begy + y, x)))
		return	(char *) 0;

	/* Put out the prompt code and the existing string in a 1-line
	   window for the purpose, and invoke wgets on that.  */

	prompt = gprompt(sc->helpcode);
	waddstr(awin, prompt);
	free(prompt);
	getyx(awin, y, x);
	sc->col = (SHORT) x;			/* Patch to after prompt */
	waddstr(awin, (char *) existing);
	ret = wgets(awin, 0, sc, existing);
	sc->col = -1;				/* Patch back */

	delwin(awin);

	touchwin(w);
	wrefresh(w);
	return  ret;
}

LONG  chk_wnum(WINDOW *w, const int row, struct sctrl *sc, const LONG existing, const int minsize)
{
	int	begy, y, x;
	char	*prompt;
	LONG	ret;
	WINDOW	*awin;

	if  (sc->col >= 0)
		return  wnum(w, row, sc, existing);

	if  ((int) sc->size < minsize)
		sc->size = (USHORT) minsize;

	getbegyx(w, begy, x);
	getyx(w, y, x);

	if  (!(awin = newwin(1, 0, begy + y, x)))
		return  -1L;

	prompt = gprompt(sc->helpcode);
	waddstr(awin, prompt);
	free(prompt);
	getyx(awin, y, x);
	sc->col = (SHORT) x;
	wprintw(awin, "%*d", (int) sc->size, existing);
	ret = wnum(awin, 0, sc, existing);
	sc->col = -1;
	delwin(awin);
	touchwin(w);
	wrefresh(w);
	return  ret;
}

/* Generate help message for format codes */

static	struct	sq_formatdef	*codeshelp_u,
				*codeshelp_l;

static	char **codeshelp(const char *prefixnotused, const int hnotused)
{
	unsigned  codecount = 1, cnt;
	struct	sq_formatdef	*fp;
	char	**result, **rp,	*msg;
	unsigned	lng;

	if  (codeshelp_u)
		for  (cnt = 0;  cnt < 26;  cnt++)
			if  (codeshelp_u[cnt].statecode != 0)
				codecount++;
	if  (codeshelp_l)
		for  (cnt = 0;  cnt < 26;  cnt++)
			if  (codeshelp_l[cnt].statecode != 0)
				codecount++;

	if  (!(result = (char **) malloc(codecount * sizeof(char *))))
		nomem();

	rp = result;
	if  (codeshelp_u)
		for  (cnt = 0;  cnt < 26;  cnt++)  {
			fp = &codeshelp_u[cnt];
			if  (fp->statecode == 0)
				continue;
			if  (!fp->explain)
				fp->explain = gprompt(fp->statecode + 500);
			lng = strlen(fp->explain) + 4;
			if  (!(msg = malloc(lng)))
				nomem();
			sprintf(msg, "%c  %s", cnt + 'A', fp->explain);
			*rp++ = msg;
		}
	if  (codeshelp_l)
		for  (cnt = 0;  cnt < 26;  cnt++)  {
			fp = &codeshelp_l[cnt];
			if  (fp->statecode == 0)
				continue;
			if  (!fp->explain)
				fp->explain = gprompt(fp->statecode + 500);
			lng = strlen(fp->explain) + 4;
			if  (!(msg = malloc(lng)))
				nomem();
			sprintf(msg, "%c  %s", cnt + 'a', fp->explain);
			*rp++ = msg;
		}
	*rp = (char *) 0;
	return  result;
}

#define	LTAB_P		1
#define	RTAB_P		4
#define	FMTWID_P	10
#define	CODE_P		15
#define	EXPLAIN_P	18

static	struct	sctrl	wns_wid = { $PH{spq field width}, ((char **(*)()) 0), 3, 0, FMTWID_P, MAG_P, 1L, 100L, (char *) 0 },
			wss_code = { $PH{spq field code}, codeshelp, 1, 0, CODE_P, MAG_P, 0L, 0L, (char *) 0 },
			wss_sep = { $PH{spq field sep}, ((char **(*)()) 0), 40, 0, 1, MAG_OK, 0L, 0L, (char *) 0 },
			wst_fname = { $PH{spq field fname}, ((char **(*)()) 0), 40, 0, 1, MAG_OK, 0L, 0L, (char *) 0 };

struct	formatrow	{
	const  char	*f_field;	/* field explain or sep */
	USHORT		f_length;	/* length of field */
	unsigned  char	f_issep;	/* 0=field 1=sep */
	unsigned  char  f_flag;		/* 1=shift left 2=skip right*/
	char		f_code;		/* field code */
};

static	struct  formatrow  *flist;
static	int		f_num;
static	unsigned	f_max;
static	char		*ltabmk, *rtabmk;

#define	INITNUM		20
#define	INCNUM		10

static	void  conv_fmt(const char *fmt, struct sq_formatdef *utab, struct sq_formatdef *ltab)
{
	struct	formatrow	*fr;

	f_num = 0;
	f_max = INITNUM;
	if  (!(flist = (struct formatrow *) malloc(INITNUM * sizeof(struct formatrow))))
		nomem();

	while  (*fmt)  {
		if  (f_num >= (int) f_max)  {
			f_max += INCNUM;
			if  (!(flist = (struct formatrow *) realloc((char *) flist, (unsigned) (f_max * sizeof(struct formatrow)))))
				nomem();
		}
		fr = &flist[f_num];

		if  (*fmt != '%')  {
			const	char	*fmtp = fmt;
			char	*nfld;
			fr->f_issep = 1;
			fr->f_length = 0;
			fr->f_flag = 0;
			do	fr->f_length++;
			while  (*++fmt  &&  *fmt != '%');
			if  (!(nfld = malloc((unsigned) (fr->f_length + 1))))
				nomem();
			strncpy(nfld, fmtp, (unsigned) fr->f_length);
			nfld[fr->f_length] = '\0';
			fr->f_field = nfld;
		}
		else  {
			USHORT	nn;
			struct	sq_formatdef	*fp;

			fr->f_issep = 0;
			fr->f_flag = 0;
			if  (*++fmt == '<')  {
				fmt++;
				fr->f_flag = 1;
			}
			else  if  (*fmt == '>')  {
				fmt++;
				fr->f_flag = 2;
			}
			nn = 0;
			do  nn = nn * 10 + *fmt++ - '0';
			while  (isdigit(*fmt));
			fr->f_length = nn;

			if  (isupper(*fmt))
				fp = &utab[*fmt - 'A'];
			else  if  (ltab  &&  islower(*fmt))
				fp = &ltab[*fmt - 'a'];
			else  {
				if  (*fmt)
					fmt++;
				continue;
			}
			fr->f_code = *fmt++;
			if  (!fp->explain)
				fp->explain = gprompt(fp->statecode + 500);
			fr->f_field = fp->explain;
		}
		f_num++;
	}
}

static	char *unconv_fmt()
{
	int	cnt;
	char	*cp;
	struct	formatrow  *fr = flist;
	char	cbuf[256];

	cp = cbuf;
	for  (cnt = 0;  cnt < f_num;  fr++, cnt++)  {
		if  (fr->f_issep)  {
			const	char	*fp;
			unsigned	lng;
			if  ((cp - cbuf) + fr->f_length >= sizeof(cbuf) - 1)
				break;
			fp = fr->f_field;
			lng = fr->f_length;
			while  (lng != 0)  {
				*cp++ = *fp++;
				lng--;
			}
		}
		else  {
			if  ((cp - cbuf) + 1 + 1 + 3 + 1 + 1 >= sizeof(cbuf))
				break;
			*cp++ = '%';
			if  (fr->f_flag & 1)
				*cp++ = '<';
			else  if  (fr->f_flag & 2)
				*cp++ = '>';
#ifdef	CHARSPRINTF
			sprintf(cp, "%u%c", fr->f_length, fr->f_code);
			cp += strlen(cp);
#else
			cp += sprintf(cp, "%u%c", fr->f_length, fr->f_code);
#endif
		}
	}
	*cp = '\0';
	return  stracpy(cbuf);
}

static void  freeflist()
{
	int	cnt;

	for  (cnt = 0;  cnt < f_num;  cnt++)
		if  (flist[cnt].f_issep)
			free((char *) flist[cnt].f_field);
	free((char *) flist);
}

#ifdef	HAVE_TERMINFO
#define	DISP_CHAR(w, ch)	waddch(w, (chtype) ch);
#else
#define	DISP_CHAR(w, ch)	waddch(w, ch);
#endif

static	void  fmtdisplay(char **fmt_hdr, int start)
{
	char	**hv;
	int	rr;

	if  (!ltabmk)  {
		ltabmk = gprompt($P{Left tab field});
		rtabmk = gprompt($P{Right tab field});
	}

#ifdef	OS_DYNIX
	clear();
#else
	erase();
#endif

	for  (rr = 0, hv = fmt_hdr;  *hv;  rr++, hv++)
		mvwhdrstr(stdscr, rr, 0, *hv);

	for  (;  start < f_num  &&  rr < LINES;  start++, rr++)  {
		struct	formatrow	*fr = &flist[start];

		if  (fr->f_issep)  {
			int	cnt = fr->f_length;
			const	char	*cp = fr->f_field;
			move(rr, 0);
			DISP_CHAR(stdscr, '\"');
			while  (cnt > 0)  {
				DISP_CHAR(stdscr, *cp);
				cp++;
				cnt--;
			}
			DISP_CHAR(stdscr, '\"');
		}
		else  {
			if  (fr->f_flag & 1)
				mvaddstr(rr, LTAB_P, ltabmk);
			else  if  (fr->f_flag & 2)
				mvaddstr(rr, RTAB_P, rtabmk);
			wn_fill(stdscr, rr, &wns_wid, (int) fr->f_length);
			move(rr, CODE_P);
			DISP_CHAR(stdscr, fr->f_code);
			mvaddstr(rr, EXPLAIN_P, (char *) fr->f_field);
		}
	}
#ifdef	CURSES_OVERLAP_BUG
	touchwin(stdscr);
#endif
}

/* View/edit formats */

int  fmtprocess(char **fmt, const char hch, struct sq_formatdef *utab, struct sq_formatdef *ltab)
{
	int	ch, err_no, hrows, tilines, start, srow, currow, incr, cnt, insertwhere, changes = 0;
	char	**fmt_hdr;

	/* Save these for help function */

	codeshelp_u = utab;
	codeshelp_l = ltab;

	fmt_hdr = helphdr(hch);
	count_hv(fmt_hdr, &hrows, &err_no);
	tilines = LINES - hrows;
	conv_fmt(*fmt, utab, ltab);

	start = 0;
	srow = 0;

	Ew = stdscr;
	select_state($H{spq field set state});
 Fdisp:
	fmtdisplay(fmt_hdr, start);
 Fmove:
	currow = srow - start + hrows;
	wmove(stdscr, currow, 0);

 Frefresh:

	refresh();

 nextin:
	do  ch = getkey(MAG_A|MAG_P);
	while  (ch == EOF  &&  (hlpscr || escr));

	if  (hlpscr)  {
		endhe(stdscr, &hlpscr);
		if  (helpclr)
			goto  nextin;
	}
	if  (escr)
		endhe(stdscr, &escr);

	switch  (ch)  {
	case  EOF:
		goto  nextin;
	default:
		err_no = $E{spq fields unknownc};
	err:
		doerror(stdscr, err_no);
		goto  nextin;

	case  $K{key help}:
		dochelp(stdscr, $H{spq field set state});
		goto  nextin;

	case  $K{key refresh}:
		wrefresh(curscr);
		goto  Frefresh;

	/* Move up or down.  */

	case  $K{key cursor down}:
		srow++;
		if  (srow >= f_num)  {
			srow--;
ev:			err_no = $E{spq fields off bottom};
			goto  err;
		}
		currow++;
		if  (currow - hrows < tilines)
			goto  Fmove;
		start++;
		goto  Fdisp;

	case  $K{key cursor up}:
		if  (srow <= 0)  {
bv:			err_no = $E{spq fields off top};
			goto  err;
		}
		srow--;
		if  (srow >= start)
			goto  Fmove;
		start = srow;
		goto  Fdisp;

	/* Half/Full screen up/down */

	case  $K{key screen down}:
		incr = tilines;
	gotitd:
		if  (srow + incr >= f_num)
			goto  ev;
		start += incr;
		srow += incr;
		goto  Fdisp;

	case  $K{key half screen down}:
		incr = tilines / 2;
		goto  gotitd;

	case  $K{key half screen up}:
		incr = tilines / 2;
		goto  gotitu;

	case  $K{key screen up}:
		incr = tilines;
		if  (srow - incr < 0)
			goto  bv;
	gotitu:
		start -= incr;
		srow -= incr;
		goto  Fdisp;

	case  $K{key top}:
		if  (srow == start)  {
			srow = start = 0;
			goto  Fdisp;
		}
		srow = start;
		goto  Fmove;

	case  $K{key bottom}:
		incr = tilines - 1;
		if  (start + incr >= f_num)
			incr = f_num - start - 1;
		if  (srow < start + incr)  {
			srow = start + incr;
			goto  Fmove;
		}
		srow = f_num - 1;
		if  (srow < 0)
			srow = 0;
		start = tilines >= f_num? 0: f_num - tilines;
		goto  Fdisp;

	case  $K{key halt}:

		/* May need to restore clobbered heading */

		if  (changes)  {
			free(*fmt);
			*fmt = unconv_fmt();
		}
		freeflist();
#ifdef	CURSES_MEGA_BUG
		clear();
		refresh();
#endif
		return  changes;

	case  $K{key delete field}:
		if  (f_num <= 0)  {
none2:			err_no = $E{spq fields none yet};
			goto  err;
		}
		f_num--;
		if  (flist[srow].f_issep)
			free((char *) flist[srow].f_field);
		if  (srow >= f_num)  {
			srow--;
			if  (srow < start)
				start = srow;
			if  (srow < 0)
				start = srow = 0;
		}
		else  for  (cnt = srow;  cnt < f_num;  cnt++)
			flist[cnt] = flist[cnt+1];
		changes++;
		goto  Fdisp;

	case  $K{spq key field width}:
		if  (f_num <= 0)
			goto  none2;
		if  (flist[srow].f_issep)  {
	issep:
			err_no = $E{spq fields is sep};
			goto  err;
		}
	{
		LONG  res = wnum(stdscr, currow, &wns_wid, (LONG) flist[srow].f_length);
		if  (res >= 0L)
			flist[srow].f_length = (USHORT) res;
		changes++;
		goto  Fmove;
	}

	case  $K{spq key left flag}:
		if  (f_num <= 0)
			goto  none2;
		if  (flist[srow].f_issep)
			goto  issep;
		move(currow, LTAB_P);
		if  (flist[srow].f_flag & 1)  {
			flist[srow].f_flag &= ~1;
			for  (incr = strlen(ltabmk);  incr > 0;  incr--)
				DISP_CHAR(stdscr, ' ');
		}
		else  {
			flist[srow].f_flag |= 1;
			addstr(ltabmk);
		}
		changes++;
		goto  Fmove;

	case  $K{spq key right flag}:
		if  (f_num <= 0)
			goto  none2;
		if  (flist[srow].f_issep)
			goto  issep;
		move(currow, RTAB_P);
		if  (flist[srow].f_flag & 2)  {
			flist[srow].f_flag &= ~2;
			for  (incr = strlen(rtabmk);  incr > 0;  incr--)
				DISP_CHAR(stdscr, ' ');
		}
		else  {
			flist[srow].f_flag |= 2;
			addstr(rtabmk);
		}
		changes++;
		goto  Fmove;

	case  $K{spq key field code}:
		if  (f_num <= 0)
			goto  none2;
		if  (flist[srow].f_issep)
			goto  issep;
		{
			char	*resc, fld[2];
			struct	formatrow	*fr = &flist[srow];
			struct	sq_formatdef	*fp;
			fld[0] = fr->f_code;
			fld[1] = '\0';
			if  (!(resc = wgets(stdscr, currow, &wss_code, fld)))  {
				move(currow, EXPLAIN_P);
				clrtoeol();
				addstr((char *) fr->f_field);
				goto  Fmove;
			}
			if  (utab  &&  isupper(resc[0])  &&  utab[resc[0] - 'A'].statecode != 0)
				fp = &utab[resc[0] - 'A'];
			else  if  (ltab  &&  islower(resc[0])  &&  ltab[resc[0] - 'a'].statecode != 0)
				fp = &ltab[resc[0] - 'a'];
			else  {
				move(currow, CODE_P);
				DISP_CHAR(stdscr, fld[0]);
				move(currow, 0);
				err_no = $E{spq fields undef code};
				goto  err;
			}
			fr->f_code = resc[0];
			if  (!fp->explain)
				fp->explain = gprompt(fp->statecode + 500);
			fr->f_field = fp->explain;
			move(currow, EXPLAIN_P);
			clrtoeol();
			addstr(fp->explain);
		}
		changes++;
		goto  Fmove;

	case  $K{spq key sep string}:
		if  (f_num <= 0)
			goto  none2;
		if  (!flist[srow].f_issep)  {
			err_no = $E{spq fields is not sep};
			goto  err;
		}
		{
			char	*resc;
			int	len;
			if  (!(resc = wgets(stdscr, currow, &wss_sep, flist[srow].f_field)))
				goto  Fmove;
			free((char *) flist[srow].f_field);
			if  ((len = strlen(resc)) <= 0)
				flist[srow].f_field = stracpy(" ");
			else  {
				if  (resc[len-1] == '\"')
					resc[--len] = '\0';
				flist[srow].f_field = stracpy(resc);
			}
			move(currow, 0);
			clrtoeol();
			printw("\"%s\"", flist[srow].f_field);
			flist[srow].f_length = (USHORT) len;
			changes++;
			goto  Fmove;
		}

	case  $K{spq key sep before}:
		insertwhere = srow;
		goto  iseprest;
	case  $K{spq key sep after}:
		insertwhere = srow + 1;
	iseprest:
		clrtoeol();
		refresh();
	{
		char	*resc, *fres;
		int	len;
		struct  formatrow	*fr;
		if  (!(resc = wgets(stdscr, currow, &wss_sep, "")))
			goto  Fdisp;
		if  ((len = strlen(resc)) <= 0)  {
			fres = stracpy(" ");
			len = 1;
		}
		else  {
			if  (resc[len-1] == '\"')
				resc[--len] = '\0';
			fres = stracpy(resc);
		}
		if  (f_num >= (int) f_max)  {
			f_max += INCNUM;
			if  (!(flist = (struct formatrow *) realloc((char *) flist, (unsigned) (f_max * sizeof(struct formatrow)))))
				nomem();
		}
		for  (cnt = f_num-1;  cnt >= insertwhere;  cnt--)
			flist[cnt+1] = flist[cnt];
		f_num++;
		fr = &flist[insertwhere];
		fr->f_issep = 1;
		fr->f_length = (USHORT) len;
		fr->f_flag = 0;
		fr->f_field = fres;
		srow = insertwhere;
		if  (srow >= start + tilines)
			start = srow - tilines + 1;
		changes++;
		goto   Fdisp;
	}
	case  $K{spq key field before}:
		insertwhere = srow;
		goto  ifldrest;
	case  $K{spq key field after}:
		insertwhere = srow + 1;
	ifldrest:
		clrtoeol();
		refresh();
	{
		char	*resc;
		int	code;
		LONG	res;
		struct	sq_formatdef	*fp;
		struct	formatrow	*fr;
		if  (!(resc = wgets(stdscr, currow, &wss_code, "")))  {
			move(currow, 0);
			clrtoeol();
			goto  Fdisp;
		}
		code = resc[0];
		if  (utab  &&  isupper(code)  &&  utab[code - 'A'].statecode != 0)
			fp = &utab[code - 'A'];
		else  if  (ltab  &&  islower(code)  &&  ltab[code - 'a'].statecode != 0)
			fp = &ltab[code - 'a'];
		else  {
			fmtdisplay(fmt_hdr, start);
			move(currow, 0);
			err_no = $E{spq fields undef code};
			goto  err;
		}
		wn_fill(stdscr, currow, &wns_wid, fp->sugg_width);
		res = wnum(stdscr, currow, &wns_wid, fp->sugg_width);
		if  (res < 0L)  {
			if  (res != -2L)
				goto  Fdisp;
			res = fp->sugg_width;
		}
		if  (f_num >= (int) f_max)  {
			f_max += INCNUM;
			if  (!(flist = (struct formatrow *) realloc((char *) flist, (unsigned) (f_max * sizeof(struct formatrow)))))
				nomem();
		}
		for  (cnt = f_num-1;  cnt >= insertwhere;  cnt--)
			flist[cnt+1] = flist[cnt];
		f_num++;
		fr = &flist[insertwhere];
		fr->f_issep = 0;
		fr->f_length = (SHORT) res;
		fr->f_flag = 0;
		fr->f_code = (char) code;
		if  (!fp->explain)
			fp->explain = gprompt(fp->statecode + 500);
		fr->f_field = fp->explain;
		srow = insertwhere;
		if  (srow >= start + tilines)
			start = srow - tilines + 1;
		changes++;
		goto   Fdisp;
	}
	}
}

static	void  createnewhelp(FILE *ifl, FILE *ofl, const int statecode, char *fmt)
{
	int	ch, ch2, nn, hadit = 0;

	rewind(ifl);
	while  ((ch = getc(ifl)) != EOF)  {

		/* Ignore lines which don't start with a numeric string.  */

		if  (!isdigit(ch))  {
			while  (ch != '\n'  &&  ch != EOF)  {
				putc(ch, ofl);
				ch = getc(ifl);
			}
			if  (ch == EOF)
				break;
			putc(ch, ofl);
			continue;
		}

		/* Read in number, see if interesting */

		nn = 0;
		do  {
			nn = nn * 10 + ch - '0';
			ch = getc(ifl);
		}  while  (isdigit(ch));

		/* Check it interests us */

		if  (toupper(ch) != 'P'  ||  nn != statecode)  {
			fprintf(ofl, "%d%c", nn, ch);
		putrest:
			while  ((ch = getc(ifl)) != '\n'  &&  ch != EOF)
				putc(ch, ofl);
			if  (ch == EOF)
				break;
			putc(ch, ofl);
			continue;
		}

		/* Check terminated by colon */

		ch2 = getc(ifl);
		if  (ch2 != ':')  {
			fprintf(ofl, "%d%c%c", nn, ch, ch2);
			goto  putrest;
		}

		/* Discard rest of line */

		while  ((ch = getc(ifl)) != '\n'  &&  ch != EOF)
			;

		/* Splurge out replacement string */

		if  (hadit)
			continue;
		fprintf(ofl, "%dP:%s\n", nn, fmt);
		hadit = 1;
	}
	if  (!hadit)  {
		time_t	now = time((time_t *) 0);
		struct  tm  *tp = localtime(&now);
		fprintf(ofl, "\nNew formats %.2d/%.2d/%.2d %.2d:%.2d:%.2d\n\n",
			       tp->tm_year % 100, tp->tm_mon+1, tp->tm_mday,
			       tp->tm_hour, tp->tm_min, tp->tm_sec);
		fprintf(ofl, "%dP:%s\n", statecode, fmt);
	}
}

static	char	*wfile;
extern	char	Cvarname[];
extern	char	*Helpfile_path;

static	void  make_confline(FILE *fp, const char *vname)
{
	fprintf(fp, "%s=%s\n", vname, wfile);
}

void  offersave(char *fmt, const int statecode)
{
	char	*hd, *wd, *str;
	int	prow, pcol, ret;
	FILE	*ofp;
	static	char	*fnprompt;

	if  (!(mypriv->spu_flgs & PV_FREEZEOK) || !askyorn($P{Save format codes}))
		return;

	hd = envprocess("${HOME-/}");
	if  (strcmp(Curr_pwd, hd) == 0)  {
		disp_str = hd;
		if  (!askyorn($P{Save in current home directory}))  {
			free(hd);
			return;
		}
		wd = stracpy(hd);
	}
	else  {
		disp_str = Curr_pwd;
		if  (askyorn($P{Save in current directory}))
			wd = stracpy(Curr_pwd);
		else  {
			disp_str = hd;
			if  (!askyorn($P{Save in home directory}))  {
				free(hd);
				return;
			}
			wd = stracpy(hd);
		}
	}
	free(hd);

	if  (chdir(wd) < 0)  {
		free(wd);
		return;
	}
	if  (!fnprompt)
		fnprompt = gprompt($P{spq field fname});
	reset_state();
	clear();
	mvaddstr(LINES/2, 0, fnprompt);
	addch(' ');
	getyx(stdscr, prow, pcol);
	wst_fname.col = (SHORT) pcol;
	for  (;;)  {
		struct	stat	sbuf1, sbuf2;
		if  (!(str = wgets(stdscr, prow, &wst_fname, "")))  {
			Ignored_error = chdir(spdir);
			free(wd);
			return;
		}
		if  (stat(str, &sbuf1) >= 0)  {
			if  ((sbuf1.st_mode & S_IFMT) != S_IFREG)  {
				doerror(stdscr, $E{spq help not flat file});
				continue;
			}
			fstat(fileno(Cfile), &sbuf2);
			if  (sbuf1.st_dev == sbuf2.st_dev && sbuf1.st_ino == sbuf2.st_ino)  {
				doerror(stdscr, $E{spq help would overwrite});
				continue;
			}
			disp_str = str;
			if  (!askyorn($P{Sure you want to overwrite}))
				continue;
			SWAP_TO(Realuid);
			ofp = fopen(str, "w+");
			SWAP_TO(Daemuid);
			if  (!ofp)  {
				doerror(stdscr, $E{spq help file cannot open});
				continue;
			}
		}
		else  {
			SWAP_TO(Realuid);
			ofp = fopen(str, "w+");
			SWAP_TO(Daemuid);
			if  (!ofp)  {
				disp_str = str;
				doerror(stdscr, $E{spq help file cannot create});
				continue;
			}
		}
		createnewhelp(Cfile, ofp, statecode, fmt);
		break;
	}

	wfile = stracpy(str);
	if  ((ret = proc_save_opts(wd, Cvarname, make_confline)))  {
		disp_str = wd;
		disp_str2 = wfile;
		doerror(stdscr, ret);
		do  ret = getkey(MAG_A|MAG_P);
		while	(ret == EOF);
	}
	Ignored_error = chdir(spdir);
	free(wfile);
	if  (!(wfile = malloc((unsigned) (strlen(wd) + strlen(str) + 2))))
		nomem();
	sprintf(wfile, "%s/%s", wd, str);
	free(wd);
	free(Helpfile_path);
	Helpfile_path = wfile;
	fclose(Cfile);
	Cfile = ofp;
	rewind(Cfile);
}
