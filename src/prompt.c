/* prompt.c -- generate lists of things for spq

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_dir.h"
#include <sys/stat.h>
#include <curses.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "spuser.h"
#include "incl_unix.h"
#ifndef HAVE_LONG_FILE_NAMES
#include "inline/owndirops.c"
#endif

#define MALLINIT        5
#define MALLINC         3

extern  uid_t   Daemuid;

extern  char    *current_prin;
extern  char    *ptdir;

static  char *pathjoin(const char *d1, const char *d2, const char *f)
{
        char    *d;
        static  char    slash[] = "/";

        if  (d2)  {
                if  ((d = (char *) malloc((unsigned) (strlen(d1) + strlen(d2) + strlen(f) + 3))) == (char *) 0)
                        nomem();
                strcpy(d, d1);
                strcat(d, slash);
                strcat(d, d2);
                strcat(d, slash);
                strcat(d, f);
        }
        else  {
                if  ((d = (char *) malloc((unsigned) (strlen(d1) + strlen(f) + 2))) == (char *) 0)
                        nomem();
                strcpy(d, d1);
                strcat(d, slash);
                strcat(d, f);
        }
        return  d;
}

int  isform(const char *d1, const char *d2, const char *f)
{
        char    *d;
        struct  stat    sbuf;

        if  (strchr(DEF_SUFCHARS, f[0]))
                return  0;

        d = pathjoin(d1, d2, f);
        if  (stat(d, &sbuf) < 0 || (sbuf.st_mode & S_IFMT) != S_IFREG || sbuf.st_uid != Daemuid)  {
                free(d);
                return  0;
        }
        free(d);
        return  1;
}

int  isprin(const char *d1, const char *d2, const char *f)
{
        char    *d;
        struct  stat    sbuf;

        d = pathjoin(d1, d2, f);
        if  (stat(d, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) != S_IFDIR || sbuf.st_uid != Daemuid)  {
                free(d);
                return  0;
        }
        free(d);
        return  1;
}

int  isaterm(const char *d1, const char *d2, const char *f)
{
        char    *d = pathjoin(d1, d2, f);
        struct  stat    sbuf;

        if  (strncmp(f, "lp", 2) != 0  &&  (strncmp(f, "tty", 3) != 0 || f[3] == '\0'))
                return  0;

        if  (stat(d, &sbuf) < 0)  {
                free(d);
                return  0;
        }

        free(d);

        if  ((sbuf.st_mode & S_IFMT) != S_IFCHR)
                return  0;

        if  (sbuf.st_uid == Daemuid)  {
                if  ((sbuf.st_mode & 0600) != 0600)
                        return  0;
        }
        else  if  ((sbuf.st_mode & 0006) != 0006)
                return  0;
        return  1;
}

char    **listdir(const char *d1, const char *d2, int (*chkfunc)(const char *, const char *, const char *), const char *sofar)
{
        DIR     *dfd;
        struct  dirent  *dp;
        char    **result, *d;
        unsigned  rcnt, msize, sfl = 0;

        if  (d2)  {
                d = pathjoin(d1, (char *) 0, d2);
                dfd = opendir(d);
                free(d);
        }
        else
                dfd = opendir(d1);

        if  (dfd == (DIR *) 0)
                return  (char **) 0;

        if  ((result = (char **) malloc((MALLINIT+1) * sizeof(char *))) == 0)
                nomem();

        if  (sofar)
                sfl = strlen(sofar);

        msize = MALLINIT;
        rcnt = 0;

        while  ((dp = readdir(dfd)) != (struct dirent *) 0)  {
                if  (dp->d_name[0] == '.'  &&
                     (dp->d_name[1] == '\0' ||
                      (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
                        continue;
                if  (strncmp(dp->d_name, sofar, (int) sfl) != 0)
                        continue;

                if  (chkfunc  &&  !(*chkfunc)(d1, d2, dp->d_name))
                        continue;

                if  (rcnt >= msize)  {
                        msize += MALLINC;
                        if  ((result = (char **) realloc((char *) result, (msize+1) * sizeof(char *))) == 0)
                                nomem();
                }
                result[rcnt] = stracpy(dp->d_name);
                rcnt++;
        }
        closedir(dfd);
        result[rcnt] = (char *) 0;
        return  result;
}

char **p_prins(const char *sofar)
{
        char    **result, **rp;
        const  Hashspptr  **pp, **ep;
        int     sfl = 0;

        if  ((result = (char **) malloc((unsigned) ((Ptr_seg.nptrs + 1) * sizeof(char *)))) == (char **) 0)
                nomem();

        rp = result;

        if  (sofar)
                sfl = strlen(sofar);
        ep = &Ptr_seg.pp_ptrs[Ptr_seg.nptrs];
        for  (pp = &Ptr_seg.pp_ptrs[0];  pp < ep;  pp++)
                if  (strncmp((*pp)->p.spp_ptr, sofar, sfl) == 0)
                        *rp++ = stracpy((*pp)->p.spp_ptr);
        *rp = (char *) 0;
        return  result;
}

char **p_forms(const char *sofar)
{
        char    **result, **rp;
        const  Hashspptr  **pp, **ep;
        int     sfl = 0;

        if  ((result = (char **) malloc((unsigned) ((Ptr_seg.nptrs + 1) * sizeof(char *)))) == (char **) 0)
                nomem();

        rp = result;

        if  (sofar)
                sfl = strlen(sofar);
        ep = &Ptr_seg.pp_ptrs[Ptr_seg.nptrs];
        for  (pp = &Ptr_seg.pp_ptrs[0];  pp < ep;  pp++)
                if  (strncmp((*pp)->p.spp_form, sofar, sfl) == 0)
                        *rp++ = stracpy((*pp)->p.spp_form);
        *rp = (char *) 0;
        return  result;
}

char **j_prins(const char *sofar)
{
        char    **result, **rp;
        const Hashspq  **jpp, **bj, **ej;
        int     sfl = 0;
        unsigned        pcnt = 0;

        /* Allocate a result buffer equal to the size of the
           number of printers.  The assumption is that there will
           be rather less than the number of jobs, particularly
           jobs with printers specified. However since we
           introduced fancy patterns, we had better test the
           boundary.  */

        if  ((result = (char **) malloc((unsigned) ((Ptr_seg.nptrs + 1) * sizeof(char *)))) == (char **) 0)
                nomem();

        rp = result;

        if  (sofar)
                sfl = strlen(sofar);

        ej = &Job_seg.jj_ptrs[Job_seg.njobs];
        for  (jpp = Job_seg.jj_ptrs;  jpp < ej;  jpp++)  {
                const  struct  spq  *jp = &(*jpp)->j;

                /* Prune out jobs with no printer specified */

                if  (jp->spq_ptr[0] == '\0')
                        continue;

                if  (strncmp(jp->spq_ptr, sofar, sfl) != 0)
                        continue;

                /* Avoid storing duplicates, as we only allocated a
                   buffer nptrs big.  */

                for  (bj = Job_seg.jj_ptrs;  bj < jpp;  bj++)
                        if  (strcmp((*bj)->j.spq_ptr, jp->spq_ptr) == 0)
                                goto  skipit;
                *rp++ = stracpy(jp->spq_ptr);
                pcnt++;         /* Make sure we don't overflow */
                if  (pcnt >= Ptr_seg.nptrs)
                        break;
        skipit:
                ;
        }
        *rp = (char *) 0;
        return  result;
}

char **j_forms(const char *sofar)
{
        char    **result;
        const Hashspq  **jpp, **ej;
        int     sfl = 0, msize = MALLINIT, rsize = 0;

        if  ((result = (char **) malloc((unsigned) ((MALLINIT + 1) * sizeof(char *)))) == (char **) 0)
                nomem();

        if  (sofar)
                sfl = strlen(sofar);
        ej = &Job_seg.jj_ptrs[Job_seg.njobs];
        for  (jpp = Job_seg.jj_ptrs;  jpp < ej;  jpp++)  {
                const struct  spq  *jp = &(*jpp)->j;
                if  (strncmp(jp->spq_form, sofar, sfl) == 0)  {
                        int     i;
                        for  (i = 0;  i < rsize;  i++)
                                if  (strcmp(jp->spq_form, result[i]) == 0)
                                        goto  hadit;
                        if  (rsize >= msize)  {
                                msize += MALLINC;
                                if  ((result = (char **) realloc((char *) result,
                                                 (unsigned) ((msize + 1) * sizeof(char *)))) == (char **) 0)
                                        nomem();
                        }
                        result[rsize] = stracpy(jp->spq_form);
                        rsize++;
                hadit:
                        ;
                }
        }
        result[rsize] = (char *) 0;
        return  result;
}

char **mconcat(char **m1, char **m2)
{
        unsigned        k1 = 0, k2 = 0;
        char    **m, **r, **result;

        if  (m1 == (char **) 0)
                return  m2;
        if  (m2 == (char **) 0)
                return  m1;

        for  (m = m1;  *m;  m++)
                k1++;
        for  (m = m2;  *m;  m++)
                k2++;
        if  ((result = (char **) malloc(sizeof(char *) * (k1+k2+1))) == (char **) 0)
                nomem();

        r = result;
        m = m1;
        while  (*m)
                *r++ = *m++;
        m = m2;
        while  (*m)
                *r++ = *m++;
        *r = (char *) 0;

        free((char *) m1);
        free((char *) m2);
        return  result;
}

char  **squeeze(char **m)
{
        char    **p, **q, **r;

        for  (p = m;  *p;  p++)
                for  (q = p+1;  *q;  q++)
                        while  (strcmp(*p, *q) == 0)  {
                                free(*q);
                                r = q;
                                do  r[0] = r[1];
                                while  (*++r);
                                if  (!*q)
                                        break;
                        }
        return  m;
}

char  **listpfdirs(const char *sofar)
{
        char    **result = (char **) 0, **p, **pp;

        if  (current_prin)
                return  listdir(ptdir, current_prin, isform, sofar);

        if  ((p = p_prins("")))  {
                for  (pp = p;  *pp;  pp++)  {
                        result = mconcat(result, listdir(ptdir, *pp, isform, sofar));
                        free(*pp);
                }
                free((char *) p);
        }
        return  result;
}

FILE *hexists(const char *dir, const char *d2)
{
        char    *fname;
        static  char    *hname;
        FILE    *result;

        if  (!hname)
                hname = envprocess(HELPNAME);
        fname = pathjoin(dir, d2, hname);
        result = fopen(fname, "r");
        free(fname);
        return  result;
}

static  char **makefvec(FILE *f)
{
        char    **result, *ln;
        int     msize, rcnt, ch;
        unsigned  l;
        LONG    w;

        if  ((result = (char **) malloc((MALLINIT+1) * sizeof(char *))) == 0)
                nomem();

        msize = MALLINIT;
        rcnt = 0;

        while  ((ch = getc(f)) != EOF)  {
                l = 1;
                w = ftell(f) - 1;
                while  (ch != '\n'  &&  ch != EOF)  {
                        l++;
                        ch = getc(f);
                }
                if  ((ln = (char *) malloc(l)) == (char *) 0)
                        nomem();
                fseek(f, (long) w, 0);
                l = 0;
                while  ((ch = getc(f)) != '\n'  &&  ch != EOF)  {
                        ln[l] = (char) ch;
                        l++;
                }
                ln[l] = '\0';

                if  (rcnt >= msize)  {
                        msize += MALLINC;
                        if  ((result = (char **) realloc((char *) result, (msize+1) * sizeof(char *))) == 0)
                                nomem();
                }
                result[rcnt] = ln;
                rcnt++;
        }
        result[rcnt] = (char *) 0;
        fclose(f);
        return  result;
}

char **wotjprin(const char *sofar, const int hf)
{
        FILE    *f;

        if  (hf  &&  (f = hexists(ptdir, (char *) 0)))
                return  makefvec(f);

        return  mconcat(p_prins(sofar), listdir(ptdir, (char *) 0, isprin, sofar));
}

char **wotjform(const char *sofar, const int hf)
{
        return  squeeze(mconcat(p_forms(sofar), listpfdirs(sofar)));
}

char **wotpform(const char *sofar, const int hf)
{
        FILE    *f;

        if  (hf  &&  current_prin  &&  (f = hexists(ptdir, current_prin)))
                return  makefvec(f);

        return  squeeze(mconcat(j_forms(sofar), listpfdirs(sofar)));
}

char **wotpprin(const char *sofar, const int hf)
{
        FILE    *f;

        if  (hf  &&  (f = hexists(ptdir, (char *) 0)))
                return  makefvec(f);

        return  squeeze(mconcat(j_prins(sofar), listdir(ptdir, (char *) 0, isprin, sofar)));
}

char **wottty(const char *sofar, const int hf)
{
        return  listdir("/dev", (char *) 0, isaterm, sofar);
}
