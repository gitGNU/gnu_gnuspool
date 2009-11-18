/* dohe.c -- generate curses help and error messages

   Copyright 2009 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "defaults.h"
#include "errnums.h"
#include "sctrl.h"
#include "keynames.h"
#include "magic_ch.h"
#include "incl_unix.h"

#define	BOXWID	1

char	helpbox,
	errbox;

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

extern	void	nomem();

/* Generate help message.  */

void	dohelp(WINDOW *owin, struct sctrl *scp, const char *prefix)
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
