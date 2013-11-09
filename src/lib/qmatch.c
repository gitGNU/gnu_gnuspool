/* qmatch.c -- match using glob-style patterns

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
#include <ctype.h>
#include "incl_unix.h"

extern int  isinrange(const int, const char *, int *);
extern char *match_comma(char *);

static int  ematch(char *pattern, const char *value)
{
        int     cnt;

        for  (;;)  {

                switch  (*pattern)  {
                case  '\0':
                        if  (*value == '\0')
                                return  1;
                        return  0;

                default:
                        if  (*pattern != *value  &&
                             (!(isalpha(*pattern) && isalpha(*value)) ||  toupper(*pattern) != toupper(*value)))
                                return  0;
                        pattern++;
                        value++;
                        continue;

                case  '?':
                        if  (*value == '\0')
                                return  0;
                        pattern++;
                        value++;
                        continue;

                case  '*':
                        pattern++;
                        for  (cnt = strlen(value); cnt >= 0;  cnt--)
                                if  (ematch(pattern, value+cnt))
                                        return  1;
                        return  0;

                case  '[':
                        if  (*value == '\0')
                                return  0;
                        if  (!isinrange(*value, pattern, &cnt))
                                return  0;
                        value++;
                        pattern += cnt;
                        continue;
                }
        }
}

int  qmatch(char *pattern, const char *value)
{
        int     res;
        char    *cp;

        do  {
                cp = match_comma(pattern);
                if  (cp)  {
                        *cp = '\0';
                        res = ematch(pattern, value);
                        *cp = ',';
                        pattern = cp + 1;
                }
                else
                        res = ematch(pattern, value);
                if  (res)
                        return  1;
        }  while  (cp);

        /* Not found...  */

        return  0;
}
