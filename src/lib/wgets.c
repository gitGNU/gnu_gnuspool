/* wgets.c -- get string using curses routine

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
#include <ctype.h>
#include "defaults.h"
#include "errnums.h"
#include "sctrl.h"
#include "keynames.h"
#include "magic_ch.h"
#include "incl_unix.h"

void    doerror(WINDOW *, const int);
void    endhe(WINDOW *, WINDOW **);
void    dohelp(WINDOW *, struct sctrl *, const char *);

char    helpclr;        /* This now defined here */

/* Define these here to reduce dependencies on external libraries */

WINDOW  *escr, *hlpscr, *Ew;

#define MAXSTR  80

static  char    result[MAXSTR+1];

void  ws_fill(WINDOW *wp, const int row, const struct sctrl *scp, const char *value)
{
        mvwprintw(wp, row, scp->col, "%-*s", scp->size, value);
}

char *wgets(WINDOW *wp, const int row, struct sctrl *scp, const char *exist)
{
        int     posn = 0, optline = 0, hadch = 0, overflow = 0, ch, err_no;
        char    **optvec = (char **) 0;

        Ew = wp;
        disp_str = scp->msg;

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
                if  (escr)  {
                        endhe(wp, &escr);
                        disp_str = scp->msg;
                }

                switch  (ch)  {
                case  EOF:
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        wrefresh(wp);
                        continue;

                case  $K{key help}:
                        result[posn] = '\0';
                        dohelp(wp, scp, result);
                        continue;

                case  $K{key guess}:
                        result[posn] = '\0';
                        if  (!optvec)  {
                                if  (scp->helpfn == HELPLESS)
                                        goto  unknc;
                                optvec = (*scp->helpfn)(result, 0);
                                if  (!optvec)  {
                                nonef:
                                        err_no = $E{wgets string error};
                                        disp_str = result;
                                        goto  err;
                                }
                                if  (!optvec[0])  {
                                        free((char *) optvec);
                                        optvec = (char **) 0;
                                        goto  nonef;
                                }
                                optline = 0;
                        }
                        else
                                optline++;

                        if  (optvec[optline] == (char *) 0)
                                optline = 0;

                        /* Remember what we had before....  */

                        strcpy(result, optvec[optline]);
                        posn = strlen(result);
                        hadch++;
                        ws_fill(wp, row, scp, result);
                        if  (posn > (int) scp->size)  { /* Mostly obscurity for field codes */
                                overflow++;
                                wclrtoeol(wp);
                                wmove(wp, row, (int) (scp->col + scp->size));
                        }
                        else
                                wmove(wp, row, (int) scp->col + posn);
                        wrefresh(wp);
                        continue;

                case  $K{key cursor up}:
                case  $K{key cursor down}:
                case  $K{key halt}:
                        if  (scp->magic_p & MAG_CRS)  {
                                ws_fill(wp, row, scp, exist);
                                scp->retv = (SHORT) ch;
                                return  (char *) 0;
                        }
                default:
                        if  (!isascii(ch))
                                goto  unknc;

                        if  (isprint(ch)  &&  (scp->magic_p & MAG_OK))
                                goto  stuffch;

                        if  (isalnum(ch) || ch == '.' || ch == '-' || ch == '_')
                                goto  stuffch;

                        if  (scp->magic_p & MAG_R)  {
                                ws_fill(wp, row, scp, exist);
                                scp->retv = (SHORT) ch;
                                return  (char *) 0;
                        }
                unknc:
                        err_no = posn == 0? $E{wgets unknown command}: $E{wgets invalid char};
                err:
                        doerror(wp, err_no);
                        continue;

                stuffch:
                        hadch++;
                        if  ((posn >= (int) scp->size && !(scp->magic_p & MAG_LONG)) || posn >= MAXSTR)  {
                                err_no = $E{wgets string too long};
                                goto  err;
                        }

                        if  (posn <= 0)  {      /*  Clear it  */
                                ws_fill(wp, row, scp, "");
                                wmove(wp, row, (int) scp->col);
                        }
                        result[posn++] = (char) ch;
                        if  (posn > scp->size)
                                overflow++;
#ifdef  HAVE_TERMINFO
                        waddch(wp, (chtype) ch);
#else
                        waddch(wp, ch);
#endif
                finch:
                        wrefresh(wp);
                        if  (optvec)  {
                                freehelp(optvec);
                                optvec = (char **) 0;
                        }
                        continue;

                case  $K{key kill}:
                        posn = 0;
                        ws_fill(wp, row, scp, exist);
                        wmove(wp, row, (int) scp->col);
                        goto  finch;

                case  $K{key erase}:
                        if  (posn <= 0)
                                continue;
                        mvwaddch(wp, row, (int) (scp->col + --posn), ' ');
                        wmove(wp, row, (int) scp->col + posn);
                        goto  finch;

                case  $K{key eol}:
                        if  (optvec)
                                freehelp(optvec);
                        if  ((!hadch  &&  scp->magic_p & MAG_NL)  ||  (posn == 0  &&  scp->magic_p & MAG_FNL))  {
                                scp->retv = 0;
                                return  (char *) 0;
                        }
                        result[posn] = '\0';
                        if  (scp->magic_p & MAG_LONG)
                                scp->retv = overflow > 0? 1: -1;
                        return  result;

                case  $K{key abort}:
                        ws_fill(wp, row, scp, exist);
                        wrefresh(wp);
                        if  (optvec)
                                freehelp(optvec);
                        scp->retv = (scp->magic_p & MAG_LONG) ? 1: 0;
                        return  (char *) 0;
                }
        }
}
