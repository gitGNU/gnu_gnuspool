/* wnum.c -- get number using curses rouines

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
#include "defaults.h"
#include "errnums.h"
#include "keynames.h"
#include "sctrl.h"
#include "magic_ch.h"
#include "incl_unix.h"

void    doerror(WINDOW *, const int);
void    dohelp(WINDOW *, struct sctrl *, const char *);
void    endhe(WINDOW *, WINDOW **);

extern  char    helpclr;

/* Basic routine to just fill in the field.  */

void  wn_fill(WINDOW *wp, const int row, const struct sctrl *scp, const LONG value)
{
        mvwprintw(wp, row, scp->col, "%*ld", scp->size, value);
}

LONG  wnum(WINDOW *wp, const int row, struct sctrl *scp, const LONG exist)
{
        LONG    result = 0L;
        int     ch, chcnt = 0, err_no, err_off, isfirst = 1, changes = 0;

        /* Initialise help/error message values and strings.
           "err_off" add 4 to select different error message if
           we can't set the string */

        Ew = wp;
        disp_arg[0] = scp->min;
        disp_arg[1] = scp->vmax;
        disp_arg[2] = exist;
        if  (!(disp_str = scp->msg))
                err_off = $S{Wnum no field name};

        wmove(wp, row, (int) (scp->col + scp->size) - 1);
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
                        if  (scp->magic_p & MAG_R)  {
                                if  (changes)
                                        wn_fill(wp, row, scp, exist);
                                scp->retv = (SHORT) ch;
                                return  -1L;
                        }
                        err_no = isfirst? $E{wnum unknown cmd}: $E{wnum invalid char};
                err:
                        doerror(wp, err_no + err_off);
                        continue;

                case '0':case '1':case '2':case '3':case '4':
                case '5':case '6':case '7':case '8':case '9':

                        isfirst = 0;
                        chcnt++;
                        result = result * 10L + (LONG) (ch - '0');
                        if  (result > scp->vmax)  {
                                err_no = $E{wnum value too large};
                                goto  ereset;
                        }
        rcnt:
                        changes++;
                        wn_fill(wp, row, scp, result);
        cnt:
                        wmove(wp, row, (int) (scp->col + scp->size) - 1);
                        wrefresh(wp);
                        continue;

                case  $K{key eol}:
                        if  (chcnt == 0)  {
                                scp->retv = 0;
                                return -2L;
                        }
                        if  (result >= scp->min)
                                return  result;
                        err_no = $E{wnum value too small};
                ereset:
                        chcnt = 0;
                        result = 0L;
                        changes++;
                        wn_fill(wp, row, scp, exist);
                        wmove(wp, row, (int)(scp->col + scp->size) - 1);
                        goto  err;

                case  $K{key kill}:
                        chcnt = 0;
                        result = 0L;
                        changes++;
                        wn_fill(wp, row, scp, exist);
                        goto  cnt;

                case  $K{key erase}:
                        if  (result == 0L)
                                continue;
                        result /= 10L;
                        if  (result == 0L)
                                isfirst = 1;
                        goto  rcnt;

                case  $K{key abort}:
                        wn_fill(wp, row, scp, exist);
                        scp->retv = 0;
                        wrefresh(wp);
                        return  -1L;
                }
        }
}
