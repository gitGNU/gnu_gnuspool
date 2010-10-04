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
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "defaults.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "errnums.h"

extern	int	save_errno;

const	char	*progname,
		*disp_str,
		*disp_str2;
char		*Helpfile_path;

LONG	disp_arg[10];

static char *concat(const char *first, const char *insert, const char *rmb, const char *rme)
{
	int	firstbit = rmb - first;
	int	inlng = strlen(insert);
	char	*result;

	if  ((result = (char *) malloc((unsigned) (firstbit + inlng + strlen(rme) + 1))) == (char *) 0)
		nomem();
	strncpy(result, first, firstbit);
	strncpy(result+firstbit, insert, inlng);
	strcpy(result+firstbit+inlng, rme);
	return  result;
}

char  **mmangle(char **mvec)
{
	int	n;
	char	**mp, *line, *cp, *newline;
	const	char	*fmt;
	char	numb[30];

	for  (mp = mvec;  *mp;  mp++)  {
	restart:
		for  (line = *mp;  (cp = strchr(line, '%'));  line = cp + 1)  {
			switch  (cp[1])  {
			default:
				continue;
			case  'E':
				n = save_errno;
				newline = concat(*mp, strerror(n), cp, cp + 2);
				break;

			case  'P':
				newline = concat(*mp, progname, cp, cp + 2);
				break;

                        case  'F':
                                newline = concat(*mp, Helpfile_path, cp, cp + 2);
                                break;

			case  'U':
				newline = concat(*mp, prin_uname(geteuid()), cp, cp + 2);
				break;

			case  'R':
				newline = concat(*mp, prin_uname(getuid()), cp, cp + 2);
				break;

			case  'G':
				newline = concat(*mp, prin_gname(getegid()), cp, cp + 2);
				break;

			case  'H':
				newline = concat(*mp, prin_gname(getgid()), cp, cp + 2);
				break;

			case  'p':
				sprintf(numb, "%ld", (long) getpid());
				newline = concat(*mp, numb, cp, cp + 2);
				break;

			case  's':
				if  (!(fmt = disp_str))
					disp_str = "<null>";
				newline = concat(*mp, disp_str, cp, cp+2);
				break;

			case  't':
				if  (!(fmt = disp_str2))
					disp_str2 = "<null>";
				newline = concat(*mp, disp_str2, cp, cp+2);
				break;

			case  'g':
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				newline = concat(*mp, prin_gname((gid_t) disp_arg[n]), cp, cp+3);
				break;

			case  'u':
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				newline = concat(*mp, prin_uname((uid_t) disp_arg[n]), cp, cp+3);
				break;

			case  'D':
			{
				struct	tm	*tp;
				int	day, mon;
				time_t	w;
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				w = disp_arg[n];
				tp = localtime(&w);
				day = tp->tm_mday;
				mon = tp->tm_mon+1;
#ifdef	HAVE_TM_ZONE
				if  (tp->tm_gmtoff <= -4 * 60 * 60)
#else
				if  (timezone >= 4 * 60 * 60)
#endif
				{ /* Dyslexic pirates at you-know-where */
					day = mon;
					mon = tp->tm_mday;
				}
				sprintf(numb, "%.2d/%.2d/%.4d", day, mon, tp->tm_year + 1900);
				newline = concat(*mp, numb, cp, cp+3);
				break;
			}

			case  'T':
			{
				struct	tm	*tp;
				time_t	w;
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				w = disp_arg[n];
				tp = localtime(&w);
				sprintf(numb, "%.2d:%.2d:%.2d", tp->tm_hour, tp->tm_min, tp->tm_sec);
				newline = concat(*mp, numb, cp, cp+3);
				break;
			}

			case  'x':
				fmt = "%lx";
				goto  fcont;
			case  'o':
				fmt = "%lo";
				goto  fcont;
			case  'd':
				fmt = "%ld";
			fcont:
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
			fcont2:
				sprintf(numb, fmt, disp_arg[n]);
			fcont3:
				newline = concat(*mp, numb, cp, cp + 3);
				break;
			case  'c':
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				if  ((ULONG) disp_arg[n] < ' ')  {
					sprintf(numb, "^%c", (int) (disp_arg[n] + '@'));
					goto  fcont3;
				}
				else  if  (disp_arg[n] < 0 || disp_arg[n] > '~')
					fmt = "\\x%.2lx";
				else
					fmt = "%lc";
				goto  fcont2;
			}
			free(*mp);
			*mp = newline;
			goto  restart;
		}
	}
	return  mvec;
}
