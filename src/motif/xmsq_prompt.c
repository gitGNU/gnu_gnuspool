/* xmsq_prompt.c -- xmspq generate lists of things

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_dir.h"
#include <sys/stat.h>
#undef  CONST                   /* Because CONST is redefined by some peoples' Intrinsic.h */
#include <Xm/MessageB.h>
#include "config.h"             /* Because CONST is undefed by some people's Intrinsic.h */
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "spuser.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "errnums.h"
#include "xmsq_ext.h"

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

#define MALLINIT        5
#define MALLINC         3

#ifndef HAVE_LONG_FILE_NAMES
#include "inline/owndirops.c"
#endif

static char *pathjoin(const char *d1, const char *d2, const char *f)
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

static int  isform(const char *d1, const char *d2, const char *f)
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

static int  isprin(const char *d1, const char *d2, const char *f)
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

static int  isaterm(const char *d1, const char *d2, const char *f)
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

static char **listdir(const char *d1, const char *d2, int (*chkfunc)(const char *,const char *,const char *))
{
        DIR     *dfd;
        struct  dirent  *dp;
        char    **result, *d;
        unsigned  rcnt, msize;

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

        msize = MALLINIT;
        rcnt = 0;

        while  ((dp = readdir(dfd)) != (struct dirent *) 0)  {
                if  (dp->d_name[0] == '.'  &&
                     (dp->d_name[1] == '\0' ||
                      (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
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

static char **p_prins()
{
        char    **result, **rp;
        const  Hashspptr  **pp, **ep;

        if  ((result = (char **) malloc((unsigned) ((Ptr_seg.nptrs + 1) * sizeof(char *)))) == (char **) 0)
                nomem();

        rp = result;

        ep = &Ptr_seg.pp_ptrs[Ptr_seg.nptrs];
        for  (pp = &Ptr_seg.pp_ptrs[0];  pp < ep;  pp++)
                *rp++ = stracpy((*pp)->p.spp_ptr);
        *rp = (char *) 0;
        return  result;
}

static char **p_forms()
{
        char    **result, **rp;
        const  Hashspptr  **pp, **ep;

        if  ((result = (char **) malloc((unsigned) ((Ptr_seg.nptrs + 1) * sizeof(char *)))) == (char **) 0)
                nomem();

        rp = result;

        ep = &Ptr_seg.pp_ptrs[Ptr_seg.nptrs];
        for  (pp = &Ptr_seg.pp_ptrs[0];  pp < ep;  pp++)
                *rp++ = stracpy((*pp)->p.spp_form);
        *rp = (char *) 0;
        return  result;
}

static char **j_prins()
{
        char    **result, **rp;
        const  Hashspq  **jpp, **bj, **ej;

        /* Allocate a result buffer equal to the size of the number of
           printers.  The assumption is that there will be rather
           less than the number of jobs, particularly jobs with
           printers specified.  */

        if  ((result = (char **) malloc((unsigned) ((Ptr_seg.nptrs + 1) * sizeof(char *)))) == (char **) 0)
                nomem();

        rp = result;

        ej = &Job_seg.jj_ptrs[Job_seg.njobs];
        for  (jpp = Job_seg.jj_ptrs;  jpp < ej;  jpp++)  {
                const struct spq *jp = &(*jpp)->j;

                /* Prune out jobs with no printer specified */

                if  (jp->spq_ptr[0] == '\0')
                        continue;

                /* Avoid storing duplicates, as we only allocated a
                   buffer Ptr_seg.nptrs big.  */

                for  (bj = Job_seg.jj_ptrs;  bj < jpp;  bj++)
                        if  (strcmp((*bj)->j.spq_ptr, jp->spq_ptr) == 0)
                                goto  skipit;
                *rp++ = stracpy(jp->spq_ptr);
        skipit:
                ;
        }
        *rp = (char *) 0;
        return  result;
}

static char **j_forms()
{
        char    **result;
        const  Hashspq  **jpp, **ej;
        int     msize = MALLINIT, rsize = 0;

        if  ((result = (char **) malloc((unsigned) ((MALLINIT + 1) * sizeof(char *)))) == (char **) 0)
                nomem();

        ej = &Job_seg.jj_ptrs[Job_seg.njobs];
        for  (jpp = Job_seg.jj_ptrs;  jpp < ej;  jpp++)  {
                const struct  spq  *jp = &(*jpp)->j;
                int     i;
                for  (i = 0;  i < rsize;  i++)
                        if  (strcmp(jp->spq_form, result[i]) == 0)
                                goto  hadit;
                if  (rsize >= msize)  {
                        msize += MALLINC;
                        if  ((result = (char **)
                              realloc((char *) result,
                                      (unsigned) ((msize + 1) * sizeof(char *)))) == (char **) 0)
                                nomem();
                }
                result[rsize] = stracpy(jp->spq_form);
                rsize++;
        hadit:
                ;
        }
        result[rsize] = (char *) 0;
        return  result;
}

static char **mconcat(char **m1, char **m2)
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

static char **squeeze(char **m)
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

static char **listpfdirs(const struct spptr *current_prin)
{
        char    **result = (char **) 0, **p, **pp;

        if  ((p = p_prins()))  {
                for  (pp = p;  *pp;  pp++)  {
                        result = mconcat(result, listdir(ptdir, *pp, isform));
                        free(*pp);
                }
                free((char *) p);
        }
        if  (current_prin)
                result = mconcat(result, listdir(ptdir, current_prin->spp_ptr, isform));
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

char **makefvec(FILE *f)
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

char **wotjprin()
{
        return  mconcat(p_prins(), listdir(ptdir, (char *) 0, isprin));
}

char **wotjform()
{
        return  squeeze(mconcat(p_forms(), listpfdirs((const struct spptr *) 0)));
}

char **wotpform()
{
        return  squeeze(mconcat(j_forms(), listpfdirs((const struct spptr *) 0)));
}

char **wotpprin()
{
        return  squeeze(mconcat(j_prins(), listdir(ptdir, (char *) 0, isprin)));
}

char **wottty()
{
        return  listdir("/dev", (char *) 0, isaterm);
}

/* Look at proposed device and comment about it with a code (as a
   subsequent argument to Confirm()), or 0 if nothing wrong.  */

int  validatedev(char *devname)
{
        char    *name;
        struct  stat    sbuf;
        char    fullpath[PATH_MAX];

        if  (devname[0] == '/')
                name = devname;
        else  {
                sprintf(fullpath, "/dev/%s", devname);
                name = fullpath;
        }
        if  (stat(name, &sbuf) < 0)
                return  $PH{Device does not exist};

        disp_arg[8] = sbuf.st_uid;

        if  (sbuf.st_uid != Daemuid)  {
                if  ((sbuf.st_mode & 022) != 022)
                        return  $P{Device not writeable};
                return  $PH{Device not owned};
        }
        if  ((sbuf.st_mode & 0600) != 0600)
                return  $PH{Owned but not writable};
        switch  (sbuf.st_mode & S_IFMT)  {
        case  S_IFBLK:
                return  $PH{Device is block device};
        case  S_IFREG:
                return  $PH{Device is flat file};
        case  S_IFIFO:
                return  $PH{Device is FIFO};
        }
        return  0;
}
