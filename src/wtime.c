/* wtime.c -- prompt for time and date in curses routines

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
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "magic_ch.h"
#include "keynames.h"
#include "incl_unix.h"
#include "sctrl.h"

#define	SECSPERDAY	(24 * 60 * 60L)

static	char	*Nohold, *weekdays[7], *monthnames[12];
static	char	month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

struct	colmarks	{
	int	wd_col;		/* First char of weekday */
	int	md_col;		/* First digit of month day */
	int	mon_col;	/* First char of month name */
	int	year_col;	/* First digit of year */
};

static	struct	colmarks  tcp;	/* Someone will object to that name */

extern	struct	spr_req	jreq;
extern	struct	spq	JREQ;

extern	char	helpclr;

void	dochelp(WINDOW *, const int);
void	doerror(WINDOW *, const int);
void	endhe(WINDOW *, WINDOW **);

static void	initnames(void)
{
	int	i;

	if  (Nohold)
		return;
	Nohold = gprompt($P{spq qmsg nohold});
	for  (i = 0;  i < 7;  i++)
		weekdays[i] = gprompt($P{Sunday}+i);
	for  (i = 0;  i < 12;  i++)
		monthnames[i] = gprompt($P{January}+i);
}

/* Display time and update starting columns.  We do this in case the
   geyser wants different lengths for days of the week and months */

void  tdisplay(WINDOW *w, const time_t t, const int row, const int col)
{
	int	dummy;
	struct	tm	*tp = localtime(&t);

	initnames();
	wmove(w, row, col);
	wclrtoeol(w);
	if  (t == 0L)  {
		waddstr(w, Nohold);
		return;
	}
	wprintw(w, "%.2d:%.2d:%.2d ", tp->tm_hour, tp->tm_min, tp->tm_sec);
	getyx(w, dummy, tcp.wd_col);
	wprintw(w, "%s ", weekdays[tp->tm_wday]);
	getyx(w, dummy, tcp.md_col);
	wprintw(w, "%.2d ", tp->tm_mday);
	getyx(w, dummy, tcp.mon_col);
	wprintw(w, "%s ", monthnames[tp->tm_mon]);
	getyx(w, dummy, tcp.year_col);
	wprintw(w, "%d", tp->tm_year + 1900);
}

/* Get digits of time.

   Return:		0 abort
			1 ok	*/

static	int	gettdigs(WINDOW *w, const int row, const int col)
{
	int	dignum, coladd, ch, err_no, dig, ctim, newtim;
	time_t	ht;
	struct	tm	*t;

	if  (JREQ.spq_hold == 0)  {
		wmove(w, row, col);
		wrefresh(w);
		select_state($S{spq nohold time state});
		for  (;;)  {
			do  ch = getkey(MAG_A|MAG_P);
			while  (ch == EOF  &&  (hlpscr || escr));
			if  (hlpscr)  {
				endhe(w, &hlpscr);
				if  (helpclr)
					continue;
			}
			if  (escr)
				endhe(w, &escr);
			switch  (ch)  {
			default:
				doerror(w, $E{spq nohold command error});

			case  EOF:
				continue;

			case  $K{key help}:
				dochelp(w, $H{spq nohold time state});
				continue;

			case  $K{key refresh}:
				wrefresh(curscr);
				wrefresh(w);
				continue;

			case  $K{key halt}:
			case  $K{key eol}:
			case  $K{key cursor down}:
			case  $K{key cursor up}:
			case  $K{key erase}:
				reset_state();
				return  ch;

			case  $K{spq key set hold time}:
				break;
			}
			break;
		}

		/* Ok he wants to set it */

		JREQ.spq_hold = time((time_t *) 0) + 60;
		wmove(w, row, col);
		tdisplay(w, (time_t) JREQ.spq_hold, row, col);
		select_state($S{spq hold time state});
	}

	ht = JREQ.spq_hold;
	t = localtime(&ht);
	dignum = 0;
	coladd = 0;

	for  (;;)  {
		wmove(w, row, col+coladd);
		wrefresh(w);

		do  ch = getkey(MAG_P|MAG_A);
		while  (ch == EOF  &&  (hlpscr || escr));
		if  (hlpscr)  {
			endhe(w, &hlpscr);
			if  (helpclr)
				continue;
		}
		if  (escr)
			endhe(w, &escr);

		switch  (ch)  {
		case  EOF:
			continue;
		default:
			err_no = $E{spq hold command error};
		err:
			doerror(w, err_no);
			continue;

		case  $K{key help}:
			dochelp(w, dignum < 2? $H{spq hold time hours}: $H{spq hold time minutes});
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			continue;

		case  $K{key abort}:
			return  0;

		case  $K{key eol}:
		case  $K{key cursor up}:
		case  $K{key cursor down}:
		case  $K{key halt}:
		case  $K{key erase}:
			if  ((time_t) JREQ.spq_hold > time((time_t *) 0))
				return  ch;
			err_no = $E{spq hold time passed};
			goto  err;

		case  $K{spq key hold unset}:
			JREQ.spq_hold = 0;
			tdisplay(w, (time_t) JREQ.spq_hold, row, col);
			return  $K{key eol};

		case  $K{key left}:
			if  (dignum <= 0)  {
				err_no = $E{spq hold time past lhs};
				goto  err;
			}
			dignum--;
			if  (--coladd == 2  ||  coladd == 5)
				coladd--;
			continue;

		case  $K{key right}:
			if  (dignum >= 5)
				return  $K{key right};
			dignum++;
			if  (++coladd == 2 || coladd == 5)
				coladd++;
			continue;

		case '0':case '1':case '2':case '3':case '4':
		case '5':case '6':case '7':case '8':case '9':

			dig = ch - '0';

			/* What happens next depends on whether we are
			   at the first or second digit of the
			   hours / minutes fields */

			switch  (dignum)  {
			case  0:	/* Tens of hours */
				if  (dig > 2)  {
				thbd:
					err_no = $E{spq hold time hours};
					goto  err;
				}
				ctim = t->tm_hour;
				newtim = ctim % 10 + dig * 10;
				if  (newtim > 20)  {
					newtim = 20;
#ifdef	HAVE_TERMINFO
					waddch(w, (chtype) ch);
#else
					waddch(w, ch);
#endif
					waddch(w, '0');
				}
				else
#ifdef	HAVE_TERMINFO
					waddch(w, (chtype) ch);
#else
					waddch(w, ch);
#endif
				coladd++;
				dignum++;
				JREQ.spq_hold += (newtim - ctim) * 60 * 60;
				t->tm_hour = newtim;
				continue;

			case  1:	/* Hours */
				if  (t->tm_hour >= 20  &&  dig > 3)
					goto  thbd;
				ctim = t->tm_hour;
				newtim = (ctim / 10) * 10 + dig;
#ifdef	HAVE_TERMINFO
				waddch(w, (chtype) ch);
#else
				waddch(w, ch);
#endif
				coladd += 2;
				dignum++;
				JREQ.spq_hold += (newtim - ctim) * 60 * 60;
				t->tm_hour = newtim;
				continue;

			case  2:	/* Tens of minutes */
				if  (dig > 5)  {
					err_no = $E{spq hold time minutes};
					goto  err;
				}
				ctim = t->tm_min;
				newtim = ctim % 10 + dig * 10;
#ifdef	HAVE_TERMINFO
				waddch(w, (chtype) ch);
#else
				waddch(w, ch);
#endif
				coladd++;
				dignum++;
				JREQ.spq_hold += (newtim - ctim) * 60;
				t->tm_min = newtim;
				continue;

			case  3:	/* Minutes  */
				ctim = t->tm_min;
				newtim = (ctim / 10) * 10 + dig;
#ifdef	HAVE_TERMINFO
				waddch(w, (chtype) ch);
#else
				waddch(w, ch);
#endif
				coladd += 2;
				dignum++;
				JREQ.spq_hold += (newtim - ctim) * 60;
				t->tm_min = newtim;
				continue;

			case  4:	/* Tens of seconds */
				if  (dig > 5)  {
					err_no = $E{spq hold time minutes};
					goto  err;
				}
				ctim = t->tm_sec;
				newtim = ctim % 10 + dig * 10;
#ifdef	HAVE_TERMINFO
				waddch(w, (chtype) ch);
#else
				waddch(w, ch);
#endif
				coladd++;
				dignum++;
				JREQ.spq_hold += newtim - ctim;
				t->tm_sec = newtim;
				continue;

			case  5:	/* Seconds  */
				ctim = t->tm_sec;
				newtim = (ctim / 10) * 10 + dig;
#ifdef	HAVE_TERMINFO
				waddch(w, (chtype) ch);
#else
				waddch(w, ch);
#endif
				JREQ.spq_hold += newtim - ctim;
				t->tm_sec = newtim;
				return  $K{key right};
			}

		case  $K{spq key increment}:

			/* Increment case follows through entire date
			   if necessary.  */

			switch  (dignum)  {
			case  5:
				JREQ.spq_hold++;
				break;
			case  4:
				JREQ.spq_hold += 10;
				break;
			case  3:
				JREQ.spq_hold += 60;
				break;
			case  2:
				JREQ.spq_hold += 60 * 10;
				break;
			case  1:
				JREQ.spq_hold += 60 * 60;
				break;
			case  0:
				JREQ.spq_hold += 60 * 60 * 12;
				break;
			}

			/* Warning: This will break if the positions
			   of hours and minutes digits change.  */

			ht = JREQ.spq_hold;
			t = localtime(&ht);
			tdisplay(w, (time_t) JREQ.spq_hold, row, col);
			continue;

		case  $K{spq key decrement}:

			switch  (dignum)  {
			case  5:
				JREQ.spq_hold--;
				break;
			case  4:
				JREQ.spq_hold -= 10;
				break;
			case  3:
				JREQ.spq_hold -= 60;
				break;
			case  2:
				JREQ.spq_hold -= 60 * 10;
				break;
			case  1:
				JREQ.spq_hold -= 60 * 60;
				break;
			case  0:
				JREQ.spq_hold -= 60 * 60 * 12;
				break;
			}

			/* Warning: See previous warning for increment case.  */

			ht = JREQ.spq_hold;
			t = localtime(&ht);
			tdisplay(w, (time_t) JREQ.spq_hold, row, col);
			continue;
		}
	}
}

static	int	getdw(WINDOW *w, const int row, const int acol)
{
	int	col, ch, err_no;

	col = tcp.wd_col;

	for  (;;)  {
		wmove(w, row, col);
		wrefresh(w);

		do  ch = getkey(MAG_P|MAG_A);
		while  (ch == EOF  &&  (hlpscr || escr));
		if  (hlpscr)  {
			endhe(w, &hlpscr);
			if  (helpclr)
				continue;
		}
		if  (escr)
			endhe(w, &escr);

		switch  (ch)  {
		case  EOF:
			continue;
		default:
			err_no = $E{spq hold time day of week};
		err:
			doerror(w, err_no);
			continue;

		case  $K{key help}:
			dochelp(w, $H{spq hold time day of week});
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			continue;

		case  $K{key abort}:
			return  0;

		case  $K{key eol}:
		case  $K{key cursor up}:
		case  $K{key cursor down}:
		case  $K{key halt}:
		case  $K{key erase}:
			if  ((time_t) JREQ.spq_hold > time((time_t *) 0))
				return  ch;
			err_no = $E{spq hold time passed};
			goto  err;

		case  $K{spq key hold unset}:
			JREQ.spq_hold = 0;
			tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
			return  $K{key eol};

		case  $K{key left}:
		case  $K{key right}:
			return  ch;

		case  $K{spq key increment}:
			JREQ.spq_hold += SECSPERDAY;
			tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
			continue;

		case  $K{spq key decrement}:
			JREQ.spq_hold -= SECSPERDAY;
			tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
			continue;
		}
	}
}

static	int	getdm(WINDOW *w, const int row, const int acol)
{
	int	col, dignum, ch, err_no, dig, newday, cday, cmon;
	time_t	ht = JREQ.spq_hold;
	struct	tm	*t = localtime(&ht);

	cday = t->tm_mday;
	cmon = t->tm_mon;
	col = tcp.md_col;	/* NB This may move!!! */
	dignum = 1;

	/* Check leap year stuff is ok (this will fall over in 2100
	   but I'll be pushing up daisies by then).  */

	month_days[1] = t->tm_year % 4 == 0? 29: 28;

	for  (;;)  {

		wmove(w, row, col+dignum);
		wrefresh(w);

	nextin:
		do  ch = getkey(MAG_P|MAG_A);
		while   (ch == EOF  &&  (hlpscr || escr));
		if  (hlpscr)  {
			endhe(w, &hlpscr);
			if  (helpclr)
				goto  nextin;
		}
		if  (escr)
			endhe(w, &escr);

		switch  (ch)  {
		case  EOF:
			continue;
		default:
			err_no = $E{spq hold time day of month};
		err:
			doerror(w, err_no);
			continue;

		case  $K{key help}:
			dochelp(w, $H{spq hold time day of month});
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			continue;

		case  $K{key abort}:
			return  0;

		case  $K{key eol}:
		case  $K{key cursor up}:
		case  $K{key cursor down}:
		case  $K{key halt}:
		case  $K{key erase}:
			if  ((time_t) JREQ.spq_hold > time((time_t *) 0))
				return  ch;
			err_no = $E{spq hold command error};
			goto  err;

		case  $K{spq key hold unset}:
			JREQ.spq_hold = 0;
			tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
			return  $K{key eol};

		case  $K{key left}:
			if  (dignum > 0)  {
				dignum = 0;
				continue;
			}
			return  ch;

		case  $K{key right}:
			if  (dignum <= 0)  {
				dignum++;
				continue;
			}
			return  ch;

		case '0':case '1':case '2':case '3':case '4':
		case '5':case '6':case '7':case '8':case '9':

			dig = ch - '0';

			/* What happens next depends on whether
			   we are at the first or second digit of
			   the day of month field */

			if  (dignum == 0)  {
				if  (dig > 3)  {
					err_no = $E{spq hold time day of month};
					goto  err;
				}
				newday = cday % 10 + dig * 10;
				dignum++;
			}
			else
				newday = (cday / 10) * 10 + dig;

			if  (newday > month_days[cmon])
				newday = month_days[cmon];
			JREQ.spq_hold += (newday - cday) * SECSPERDAY;
			ht = JREQ.spq_hold;
			t = localtime(&ht);
			cday = t->tm_mday;
			cmon = t->tm_mon;
			break;

		case  $K{spq key increment}:
			JREQ.spq_hold += (dignum == 0)? 10 * SECSPERDAY: SECSPERDAY;
			ht = JREQ.spq_hold;
			t = localtime(&ht);
			cday = t->tm_mday;
			cmon = t->tm_mon;
			break;

		case  $K{spq key decrement}:
			JREQ.spq_hold -= (dignum == 0)? 10 * SECSPERDAY: SECSPERDAY;
			ht = JREQ.spq_hold;
			t = localtime(&ht);
			cday = t->tm_mday;
			cmon = t->tm_mon;
			break;
		}

		tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
		col = tcp.md_col;
	}
}

static	int	getmon(WINDOW *w, const int row, const int acol)
{
	int	col, ch, err_no;
	time_t	ht = JREQ.spq_hold;
	struct	tm	*t = localtime(&ht);

	col = tcp.mon_col;	/* NB This may move!!! */

	for  (;;)  {
		wmove(w, row, col);
		wrefresh(w);

	nextin:
		do  ch = getkey(MAG_P|MAG_A);
		while  (ch == EOF  &&  (hlpscr || escr));
		if  (hlpscr)  {
			endhe(w, &hlpscr);
			if  (helpclr)
				goto  nextin;
		}
		if  (escr)
			endhe(w, &escr);

		switch  (ch)  {
		case  EOF:
			continue;
		default:
			err_no = $E{spq hold time month};
		err:
			doerror(w, err_no);
			continue;

		case  $K{key help}:
			dochelp(w, $H{spq hold time month});
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			continue;

		case  $K{key abort}:
			return  0;

		case  $K{key eol}:
		case  $K{key cursor up}:
		case  $K{key cursor down}:
		case  $K{key halt}:
		case  $K{key erase}:
			if  ((time_t) JREQ.spq_hold > time((time_t *) 0))
				return  ch;
			err_no = $E{spq hold command error};
			goto  err;

		case  $K{spq key hold unset}:
			JREQ.spq_hold = 0;
			tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
			return  $K{key eol};

		case  $K{key left}:
		case  $K{key right}:
			return  ch;

		case  $K{spq key increment}:
			month_days[1] = t->tm_year % 4 == 0? 29: 28;
			JREQ.spq_hold += month_days[t->tm_mon] * SECSPERDAY;
			if  (t->tm_mon < 11  &&  t->tm_mday > month_days[t->tm_mon+1])
				JREQ.spq_hold -= (t->tm_mday - month_days[t->tm_mon+1]) * SECSPERDAY;
			ht = JREQ.spq_hold;
			t = localtime(&ht);
			break;

		case  $K{spq key decrement}:
			month_days[1] = t->tm_year % 4 == 0? 29: 28;
			JREQ.spq_hold -= month_days[t->tm_mon] * SECSPERDAY;
			if  (t->tm_mon > 0  &&  t->tm_mday > month_days[t->tm_mon-1])
				JREQ.spq_hold += (t->tm_mday - month_days[t->tm_mon+1]) * SECSPERDAY;
			ht = JREQ.spq_hold;
			t = localtime(&ht);
			break;
		}

		tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
		col = tcp.mon_col;
	}
}

static	int	getyr(WINDOW *w, const int row, const int acol)
{
	int	col, ch, err_no;
	time_t	ht = JREQ.spq_hold;
	struct	tm	*t = localtime(&ht);

	col = tcp.year_col+3;	/* NB This may move!!! */

	for  (;;)  {
		wmove(w, row, col);
		wrefresh(w);

	nextin:
		do  ch = getkey(MAG_P|MAG_A);
		while  (ch == EOF  &&  (hlpscr || escr));
		if  (hlpscr)  {
			endhe(w, &hlpscr);
			if  (helpclr)
				goto  nextin;
		}
		if  (escr)
			endhe(w, &escr);

		switch  (ch)  {
		case  EOF:
			continue;
		default:
			err_no = $E{spq hold time year};
		err:
			doerror(w, err_no);
			continue;

		case  $K{key help}:
			dochelp(w, $H{spq hold time year});
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			continue;

		case  $K{key abort}:
			return  0;

		case  $K{key eol}:
		case  $K{key cursor up}:
		case  $K{key cursor down}:
		case  $K{key right}:
		case  $K{key halt}:
		case  $K{key erase}:
			if  ((time_t) JREQ.spq_hold > time((time_t *) 0))
				return  ch;
			err_no = $E{spq hold time passed};
			goto  err;

		case  $K{spq key hold unset}:
			JREQ.spq_hold = 0;
			tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
			return  $K{key eol};

		case  $K{key left}:
			return  ch;

			/* Increment and decrement year.

			   All this messing around with leap years
			   gives me a whole new dimension of sympathy
			   for Brutus & Cassius. */

		case  $K{spq key increment}:
			JREQ.spq_hold += 365 * SECSPERDAY;
			if  (t->tm_mon > 1)  {
				if  (t->tm_year % 4 == 3)
					JREQ.spq_hold += SECSPERDAY;
			}
			else  {
				if  (t->tm_mon == 1)  {
					if  (t->tm_mday < 29)
						JREQ.spq_hold += SECSPERDAY;
				}
				else  if  (t->tm_year % 4 == 0)
					JREQ.spq_hold += SECSPERDAY;
			}
			ht = JREQ.spq_hold;
			t = localtime(&ht);
			break;

		case  $K{spq key decrement}:
			if  (t->tm_year <= 90)  {
				err_no = $E{spq hold time antique};
				goto  err;
			}
			JREQ.spq_hold -= 365 * SECSPERDAY;

			if  (t->tm_mon > 1)  {
				if  (t->tm_year % 4 == 0)
					JREQ.spq_hold -= SECSPERDAY;
			}
			else  {
				if  (t->tm_mon == 1)  {
					if  (t->tm_mday == 29)
						JREQ.spq_hold -= SECSPERDAY;
				}
				else  if  (t->tm_year % 4 == 1)
					JREQ.spq_hold -= SECSPERDAY;
			}
			ht = JREQ.spq_hold;
			t = localtime(&ht);
			break;
		}

		tdisplay(w, (time_t) JREQ.spq_hold, row, acol);
		col = tcp.year_col + 3;
	}
}

int	wtime(WINDOW *w, const int row, const int col)
{
	int	i, ret;
	static	int	(*fns[])(WINDOW *, const int, const int) = { gettdigs, getdw, getdm, getmon, getyr };
	time_t	origtime = JREQ.spq_hold;

	initnames();
	select_state($H{spq hold time state});

	i = 0;
	while  (i < sizeof(fns)/sizeof(int (*)()))  {
		switch  (ret = (*fns[i])(w, row, col))  {
		case  0:
			JREQ.spq_hold = (LONG) origtime;
			tdisplay(w, (time_t) JREQ.spq_hold, row, col);
			break;
		case  $K{key left}:
			i--;
			continue;
		case  $K{key right}:
			ret = $K{key eol};
			i++;
			continue;
		case  $K{key eol}:
		case  $K{key cursor up}:
		case  $K{key cursor down}:
		case  $K{key halt}:
		case  $K{key erase}:
			break;
		}
		break;
	}
	reset_state();
	return  ret;
}
