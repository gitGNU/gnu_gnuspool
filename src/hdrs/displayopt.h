/* displayopt.h -- what we want to view in user interfaces

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

enum  jincl_t  {
        JINCL_NONULL,           /* Don't include jobs with no printer specified
                                   (when restricting to a given printer) */
        JINCL_NULL,             /* Include jobs with specified printer only */
        JINCL_ALL               /* Include all jobs */
};

enum  jrestrict_t  {
        JRESTR_ALL,             /* No restriction */
        JRESTR_UNPRINT,         /* Unprinted jobs only */
        JRESTR_PRINT            /* Printed jobs only */
};

enum  netrestrict_t  {
        NRESTR_NONE,            /* No restriction */
        NRESTR_LOCALONLY        /* Local info only */
};

enum  sortp_t  {
        SORTP_NONE,             /* No sorting */
        SORTP_BYNAME            /* Sort by name */
};

typedef  struct  {
        enum  jincl_t           opt_jinclude;   /* Decide what jobs to include when restricting printers */
        enum  jrestrict_t       opt_jprindisp;  /* Decide what to display depending on whether printed */
        enum  netrestrict_t     opt_localonly;  /* Display remote jobs/printers or not */
        enum  sortp_t           opt_sortptrs;   /* Sort printer names */
        classcode_t             opt_classcode;  /* Option classcode - for restricting display */
        char                    *opt_restru;    /* Restrict to given user(s) */
        char                    *opt_restrp;    /* Restrict to given printer(s) */
        char                    *opt_restrt;    /* Restrict to given title */
}  dispopt_t;

extern  dispopt_t       Displayopts;

extern void     readjoblist(const int);
extern void     readptrlist(const int);
