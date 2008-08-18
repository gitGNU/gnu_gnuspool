/* spu_propts.c -- option handling for spuser

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
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "spuser.h"
#include "keynames.h"
#include "sctrl.h"
#include "magic_ch.h"
#include "ecodes.h"
#include "incl_unix.h"

void	dohelp(WINDOW *, struct sctrl *, const char *);
void	doerror(WINDOW *, const int);
void	endhe(WINDOW *, WINDOW **);

int	spitoption(const int, const int, FILE *, const int, const int);
int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

#ifndef	HAVE_ATEXIT
void	exit_cleanup(void);
#endif

extern	WINDOW	*escr,
		*hlpscr,
		*Ew;

extern	char	iflag,
		cflag,
		alphsort,
		helpclr,
		helpbox,
		errbox;

extern	char	*Curr_pwd;

#define	SORT_NONE	0	/* THESE WANT MOVING!! */
#define	SORT_USER	1

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

/* Bodge help for when we don't have a specific thing to
   do other than display a message code.  */

void	dochelp(WINDOW *wp, const int code)
{
	struct	sctrl	xx;
	xx.helpcode = code;
	xx.helpfn = (char **(*)()) 0;

	dohelp(wp, &xx, (char *) 0);
}

static	void	prd_mode(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, iflag? lt->prompts[2]: cflag? lt->prompts[1]: lt->prompts[0]);
}

static	void	prd_sort(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, alphsort? lt->prompts[1]: lt->prompts[0]);
}

static	void	prd_helpbox(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, helpbox? lt->prompts[1]: lt->prompts[0]);
}

static	void	prd_errbox(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, errbox? lt->prompts[1]: lt->prompts[0]);
}

static	void	prd_helpclr(const struct ltab *lt)
{
	mvaddstr(lt->row, lt->col, helpclr? lt->prompts[1]: lt->prompts[0]);
}

static	int	pro_bool(const struct ltab *lt, unsigned *b)
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
			doerror(stdscr, $E{Spuser options unknown cmd});
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

static	int	pro_mode(const struct ltab *lt)
{
	unsigned  current = iflag? 2: cflag? 1: 0;
	unsigned  orig = current;
	int	ch = pro_bool(lt, &current);

	if  (current != orig)  {
		if  (current > 1)  {
			iflag = 1;
			cflag = 0;
		}
		else  if  (current)  {
			iflag = 0;
			cflag = 1;
		}
		else  {
			iflag = 0;
			cflag = 0;
		}
	}
	return  ch;
}

static	int	pro_sort(const struct ltab *lt)
{
	unsigned  current = alphsort;
	int	ch = pro_bool(lt, &current);

	if  (current != alphsort)
		alphsort = (char) current;
	return  ch;
}

static	int	pro_helpbox(const struct ltab *lt)
{
	unsigned  current = helpbox;
	int	ch = pro_bool(lt, &current);

	if  (current != helpbox)
		helpbox = (char) current;
	return  ch;
}

static	int	pro_errbox(const struct ltab *lt)
{
	unsigned  current = errbox;
	int	ch = pro_bool(lt, &current);

	if  (current != errbox)
		errbox = (char) current;
	return  ch;
}

static	int	pro_helpclr(const struct ltab *lt)
{
	unsigned  current = helpclr;
	int	ch = pro_bool(lt, &current);

	if  (current != helpclr)
		helpclr = (char) current;
	return  ch;
}

static	struct	ltab  ltab[] = {
{	$HPN{spuser opt mode}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_mode, pro_mode	},
{	$HPN{spuser opt sort}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_sort, pro_sort	},
{	$HPN{spuser opt helpbox}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_helpbox, pro_helpbox	},
{	$HPN{spuser opt errbox}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_errbox, pro_errbox	},
{	$HPN{spuser opt helpclr}, NULLCH, {NULLCH, NULLCH, NULLCH}, 0, 0, 0, prd_helpclr, pro_helpclr	}
};

#define	TABNUM	(sizeof(ltab)/sizeof(struct ltab))

static	struct	ltab	*lptrs[TABNUM+1];
static	int	comeinat;
static	char	**title;

static	void	initnames(void)
{
	int	i, next;
	struct  ltab	*lt;
	int	curr, hrows, cols, look4, rowstart, pstart;
	int	nextstate[TABNUM];

	title = helphdr('=');
	count_hv(title, &hrows, &cols);

	/* Slurp up standard messages */

	if  ((rowstart = helpnstate($N{spuser opt start row})) <= 0)
		rowstart = $P{spuser opt mode};
	if  ((pstart = helpnstate($N{spuser opt init cursor})) <= 0)
		pstart = rowstart;

	if  (rowstart < $N{spuser opt mode} || rowstart >= $N{spuser opt mode} + TABNUM)  {
		disp_arg[9] = rowstart;
	bads:
		doerror(stdscr, $E{spuser opts bad state code});
		refresh();
		do  i = getkey(MAG_A|MAG_P);
		while	(i == EOF);
#ifndef	HAVE_ATEXIT
		exit_cleanup();
#endif
		exit(E_BADCFILE);
	}

	if  (pstart < $N{spuser opt mode} || pstart >= $N{spuser opt mode} + TABNUM)  {
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
			if  (next < $N{spuser opt mode} || next >= $N{spuser opt mode} + TABNUM)  {
				disp_arg[9] = next;
				goto  bads;
			}
			if  (nextstate[next-$N{spuser opt mode}] > 0)  {
				disp_arg[9] = next;
				disp_arg[8] = nextstate[next-$N{spuser opt mode}];
				doerror(stdscr, $E{spuser opts duplicated state code});
				refresh();
				do  i = getkey(MAG_A|MAG_P);
				while	(i == EOF);
#ifndef	HAVE_ATEXIT
				exit_cleanup();
#endif
				exit(E_BADCFILE);
			}
		}
		nextstate[look4 - $N{spuser opt mode}] = next;
		look4 = next;
	}  while  (next > 0);

	comeinat = 0;
	next = 0;
	curr = rowstart;
	do  {
		if  (pstart == curr)
			comeinat = next;
		lt = &ltab[curr-$N{spuser opt mode}];
		lt->row = hrows + next;
		lptrs[next] = lt;
		next++;
		curr = nextstate[curr-$N{spuser opt mode}];
	}  while  (curr > 0);
	lptrs[next] = (struct ltab *) 0;
}

static	int	askyorn(const int code)
{
	char	*prompt = gprompt(code);
	int	ch;

	select_state($H{spuser askyorn});
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
			doerror(stdscr, $E{spuser options unknown command});

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

void	spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;
	fprintf(dest, "%s", name);

	cancont = spitoption(iflag? $A{spuser admin}:
			     cflag? $A{spuser formprio}:
			     $A{spuser display},
			     $A{spuser explain}, dest, '=', cancont);
	cancont = spitoption(alphsort? $A{spuser sort user}:
			     $A{spuser sort uid},
			     $A{spuser explain}, dest, ' ', cancont);
	cancont = spitoption(helpbox? $A{spuser help box}:
			     $A{spuser no help box},
			     $A{spuser explain}, dest, ' ', cancont);
	cancont = spitoption(errbox? $A{spuser error box}:
			     $A{spuser no error box},
			     $A{spuser explain}, dest, ' ', cancont);
	cancont = spitoption(helpclr? $A{spuser losechar}:
			     $A{spuser keepchar},
			     $A{spuser explain}, dest, ' ', cancont);
	putc('\n', dest);
}

static	void	ask_build(void)
{
	char	*hd;
	int	ret, i;
	static	char	spuser[] = "SPUSER";

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
			if  ((ret = proc_save_opts(Curr_pwd, spuser, spit_options)) == 0)  {
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

	if  ((ret = proc_save_opts(hd, spuser, spit_options)) == 0)  {
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

int	propts(void)
{
	int	ch;
	struct  ltab	*lt;
	int	i, whichel;
	char	**hv;
	static	char	doneinit = 0;

	if  (!doneinit)  {
		doneinit = 1;
		initnames();
	}

	Ew = stdscr;
	whichel = comeinat;
	clear();
	if  (title)  for  (hv = title, i = 0;  *hv;  i++, hv++)
		mvwhdrstr(stdscr, i, 0, *hv);

	for  (i = 0;  (lt = lptrs[i]);  i++)  {
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
