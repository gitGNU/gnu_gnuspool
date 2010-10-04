/* errnums.h -- all the error/help handling declarations

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

extern  LONG    disp_arg[];
extern  const   char    *disp_str,
                        *disp_str2,
                        *progname;
extern  FILE    *Cfile;

extern void  print_error(const int);
extern void  fprint_error(FILE *, const int);
extern void  freehelp(char **);
extern void  count_hv(char **, int *, int *);

extern int  helpnstate(const int);
extern int  helprdn();

extern char *help_readl(int *);
extern char *helpprmpt(const int);
extern char *gprompt(const int);
extern char **helpvec(const int, const char);
extern char **helphdr(const char);
extern char **mmangle(char **);
extern char *prin_size(LONG);
