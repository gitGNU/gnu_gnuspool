/* whexnum.c -- get class code with curses routines

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

#include <sys/types.h>
#include <curses.h>
#include <ctype.h>
#include "defaults.h"
#include "spuser.h"
#include "errnums.h"
#include "sctrl.h"
#include "keynames.h"
#include "magic_ch.h"
#include "incl_unix.h"

void	doerror(WINDOW *, const int);
void	dohelp(WINDOW *, struct sctrl *, const char *);
void	endhe(WINDOW *, WINDOW **);

extern	char	helpclr;

void  wh_fill(WINDOW *wp, const int row, const struct sctrl *scp, const classcode_t value)
{
	mvwaddstr(wp, row, scp->col, hex_disp(value, 1));
}

classcode_t whexnum(WINDOW *wp, const int row, struct sctrl *scp, const classcode_t exist)
{
	classcode_t	result = exist;
	int	ch;
	int	whichcol = 0, isfirst = 1;
	int	err_no, err_off = 0;

	/* Initialise help/error message values and strings.  */

	Ew = wp;
	select_state($S{whexnum state});
	scp->retv = 0;
	if  ((disp_str = scp->msg) == (char *) 0)
		err_off = $S{Wnum no field name};

	wmove(wp, row, (int) scp->col);
	wrefresh(wp);

	for  (;;)  {
		do  ch = getkey(scp->magic_p);
		while  (ch == EOF  &&  (hlpscr || escr));
		if  (hlpscr)  {
			endhe(wp, &hlpscr);
			if  (helpclr)
				continue;
		}
		if  (escr)
			endhe(wp, &escr);

		switch  (ch)  {
		case  EOF:
			continue;

		case  $K{key refresh}:
			wrefresh(curscr);
			wrefresh(wp);
			continue;

		case  $K{key help}:
			dohelp(wp, scp, (char *) 0);
			continue;

		default:
			if  (isalpha(ch))  {
				int	v = isupper(ch)? ch - 'A': ch - 'a' + 16;
				if  (toupper(ch) <= 'P')  {
					isfirst = 0;
					result |= (1L << v);
					whichcol = v + 1;
					goto  redisp;
				}
			}
			if  (scp->magic_p & MAG_R)  {
				/* In case where we are returning
				   terminating char, we allow the
				   result to go through (mostly qopts.c).  */
				if  (result == 0)
					goto  zero;
				scp->retv = (SHORT) ch;
				return  result;
			}
			err_no = isfirst? $E{whexnum unknown cmd}: $E{whexnum invalid char};
		err:
			doerror(wp, err_no+err_off);
			continue;

		case  $K{key toggle}:
			if  (whichcol >= (int) scp->size)  {
		toobig:		err_no = $E{whexnum size error};
				goto  err;
			}
			result ^= (1L << whichcol);
		redisp:
			wh_fill(wp, row, scp, result);
		movedc:
			wmove(wp, row, (int) (scp->col + whichcol));
			wrefresh(wp);
			continue;

		case  $K{key yes}:
			if  (whichcol >= (int) scp->size)
				goto  toobig;
			result |= 1L << whichcol;
			goto  redisp;

		case  $K{key no}:
			if  (whichcol >= (int) scp->size)
				goto  toobig;
			result &= ~(1L << whichcol);
			goto  redisp;

		case  $K{key all true}:
			result = (classcode_t) 0xFFFFFFFFL;
			goto  redisp;

		case  $K{key all false}:
			result = 0;
			goto  redisp;

		case  $K{key left}:
			if  (whichcol > 0)
				whichcol--;
			goto  movedc;

		case  $K{key right}:
			if  (whichcol < (int) scp->size)
				whichcol++;
			goto  movedc;

		case  $K{key eol}:
			if  (result != 0)
				return	result;
		zero:
			err_no = $E{whexnum class zero};
			whichcol = 0;
			result = exist;
			wh_fill(wp, row, scp, result);
			wmove(wp, row, (int) scp->col);
			wrefresh(wp);
			goto  err;

		case  $K{key kill}:
			isfirst = 1;
			whichcol = 0;
			result = exist;
			goto  movedc;

		case  $K{key abort}:
			wh_fill(wp, row, scp, exist);
			wrefresh(wp);
			return	exist;
		}
	}
}
