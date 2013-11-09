/* mmangle.c -- process vector of strings with % constructs in

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

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "defaults.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "errnums.h"

extern  int     save_errno;

const   char    *progname,
                *disp_str,
                *disp_str2;
char            *Helpfile_path;

LONG    disp_arg[10];

static  void    mangline(char **mp)
{
        char    *linep, *newline, *restline, *perc;
        int     n, firstlng, dellng, inslng, restlng;
        const   char    *ins;
        char    numb[30];

        numb[0] = '\0';

        for  (linep = *mp;  (perc = strchr(linep, '%'));  linep = restline)  {

                dellng = 2;             /* Which it is most of the time */
                ins = numb;

                switch  (perc[1])  {
                default:        /* Don't understand this, so ignore it */
                        restline = perc + 1;
                        continue;

                case  '%':
                        ins = "%";
                        break;

                case  'E':
                        n = save_errno;
                        ins = strerror(n);
                        break;

                case  'P':
                        ins = progname;
                        break;

                case  'F':
                        ins = Helpfile_path;
                        break;

                case  'U':
                        ins = prin_uname(geteuid());
                        break;

                case  'R':
                        ins = prin_uname(getuid());
                        break;

                case  'G':
                        ins = prin_gname(getegid());
                        break;

                case  'H':
                        ins = prin_gname(getgid());
                        break;

                case  'p':
                        sprintf(numb, "%ld", (long) getpid());
                        break;

                case  's':
                        ins = disp_str;
                        break;

                case  't':
                        ins = disp_str2;
                        break;

                case  'g':
                case  'u':
                case  'D':
                case  'T':
                case  'x':
                case  'o':
                case  'd':
                case  'c':
                        n = perc[2] - '0';
                        if  (n < 0 || n > 9)  {
                                restline = perc + 2;
                                continue;
                        }

                        dellng++;

                        switch  (perc[1])  {
                        case  'g':
                                ins = prin_gname((gid_t) disp_arg[n]);
                                break;
                        case  'u':
                                ins = prin_uname((uid_t) disp_arg[n]);
                                break;
                        case  'D':
                        {
                                struct  tm      *tp;
                                int     day, mon;
                                time_t  w = disp_arg[n];
                                tp = localtime(&w);
                                day = tp->tm_mday;
                                mon = tp->tm_mon+1;
#ifdef  HAVE_TM_ZONE
                                if  (tp->tm_gmtoff <= -4 * 60 * 60)
#else
                                if  (timezone >= 4 * 60 * 60)
#endif
                                { /* Dyslexic pirates at you-know-where */
                                        day = mon;
                                        mon = tp->tm_mday;
                                }
                                sprintf(numb, "%.2d/%.2d/%.4d", day, mon, tp->tm_year + 1900);
                                break;
                        }
                        case  'T':
                        {
                                struct  tm      *tp;
                                time_t  w = disp_arg[n];
                                tp = localtime(&w);
                                sprintf(numb, "%.2d:%.2d:%.2d", tp->tm_hour, tp->tm_min, tp->tm_sec);
                                break;
                        }

                        case  'x':
                                sprintf(numb, "%lx", (long) disp_arg[n]);
                                break;
                        case  'o':
                                sprintf(numb, "%lo", (long) disp_arg[n]);
                                break;
                        case  'd':
                                sprintf(numb, "%ld", (long) disp_arg[n]);
                                break;
                        case  'c':
                                if  ((ULONG) disp_arg[n] < ' ')
                                        sprintf(numb, "^%c", (int) (disp_arg[n] + '@'));
                                else  if  (disp_arg[n] < 0 || disp_arg[n] > '~')
                                        sprintf(numb, "\\x%.2x", ((unsigned) disp_arg[n]) & 255);
                                else
                                        sprintf(numb, "%c", (int) disp_arg[n]);
                                break;
                        }
                        break;
                }
                if  (!ins)
                        ins = "<null>";
                firstlng = perc - *mp;
                inslng = strlen(ins);
                restlng = strlen(perc + dellng);
                if  (!(newline = malloc((unsigned) (firstlng + inslng + restlng + 1))))
                        nomem();
                strncpy(newline, *mp, firstlng);
                strcpy(newline + firstlng, ins);
                strcpy(newline + firstlng + inslng, perc + dellng);
                restline = newline + firstlng + inslng;
                free(*mp);
                *mp = newline;
        }
}

/* Perform %-substitutions on each line of vector */

char  **mmangle(char **mvec)
{
        char    **mp;

        for  (mp = mvec;  *mp;  mp++)
                mangline(mp);
        return  mvec;
}
