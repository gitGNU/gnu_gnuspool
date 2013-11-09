/* cgifndjb.h -- for CGI routines - declarations for job number/printer name decoding

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

struct  jobswanted      {
        jobno_t         jno;            /* Job number */
        netid_t         host;           /* Host id */
        const  struct  spq  *jp;        /* Job structure pointer */
};
struct  ptrswanted      {
        char            *ptrname;       /* Job number */
        netid_t         host;           /* Host id */
        const  struct  spptr  *pp;      /* Ptr structure pointer */
};

int     decode_jnum(char *, struct jobswanted *);
int     decode_pname(char *, struct ptrswanted *);
extern  const Hashspq *find_job(struct jobswanted *);
extern  const Hashspptr *find_ptr(struct ptrswanted *);
