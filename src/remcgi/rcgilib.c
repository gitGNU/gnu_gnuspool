/* rcgilib.c -- library routines for remote CGI routines

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

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "gspool.h"
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <errno.h>
#include "network.h"
#include "ecodes.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "rcgilib.h"

int  numeric(const char *x)
{
        while  (*x)  {
                if  (!isdigit(*x))
                        return  0;
                x++;
        }
        return  1;
}

int  decode_jnum(char *jnum, struct jobswanted *jwp)
{
        char    *cp;

        if  ((cp = strchr(jnum, ':')))  {
                *cp = '\0';
                if  ((jwp->host = look_hostname(jnum)) == 0L)  {
                        *cp = ':';
                        disp_str = jnum;
                        return  $E{Unknown host name};
                }
                *cp++ = ':';
        }
        else  {
                jwp->host = dest_hostid;
                cp = jnum;
        }
        if  (!numeric(cp)  ||  (jwp->jno = (jobno_t) atol(cp)) == 0)  {
                disp_str = jnum;
                return  $E{job num not numeric};
        }
        return  0;
}

int  decode_pname(char *pname, struct ptrswanted *pwp)
{
        char    *cp;

        if  ((cp = strchr(pname, ':')))  {
                *cp = '\0';
                if  ((pwp->host = look_hostname(pname)) == 0L)
                        return  0;
                if  (pwp->host == dest_hostid)
                        pwp->host = 0L;
                *cp++ = ':';
        }
        else  {
                pwp->host = dest_hostid;
                cp = pname;
        }

        pwp->ptrname = stracpy(cp);
        return  1;
}

struct apispptr *find_ptr(const slotno_t sl)
{
        struct  ptr_with_slot   *ps;

        for  (ps = ptr_sl_list;  ps < &ptr_sl_list[Nptrs];  ps++)
                if  (ps->slot == sl)
                        return  &ps->ptr;
        return  (struct apispptr *) 0;
}

int  find_ptr_by_name(const char **namep, struct ptr_with_slot *rptr)
{
        const   char    *name = *namep;
        const   char    *colp = strchr(name, ':');
        int     pncnt = 0, ret;
        netid_t nid = dest_hostid;
        char    pn[PTRNAMESIZE+1];

        if  (colp)  {
                char    hostn[HOSTNSIZE+1];
                if  (colp - name > HOSTNSIZE)
                        return  0;
                strncpy(hostn, name, colp - name);
                hostn[colp - name] = '\0';
                if  ((nid = my_look_hostname(hostn)) == 0)
                        return  0;
                name = colp + 1;
        }

        while  (isalnum(*name)  ||  *name == '_')
                if  (pncnt < PTRNAMESIZE)
                        pn[pncnt++] = *name++;
        pn[pncnt] = '\0';

        if  ((ret = gspool_ptrfind(gspool_fd, GSPOOL_FLAG_IGNORESEQ, pn, nid, &rptr->slot, &rptr->ptr)) < 0)  {
                if  (ret != GSPOOL_UNKNOWN_PTR)  {
                        html_disperror($E{Base for API errors} + ret);
                        exit(E_NETERR);
                }
                return  0;
        }
        return  1;
}

void  api_open(char *realuname)
{
        int     ret;

        disp_str = dest_hostname;
        gspool_fd = gspool_open(dest_hostname, (char *) 0, 0xFFFFFFFF);
        if  (gspool_fd < 0)  {
                html_disperror($E{Base for API errors} + gspool_fd);
                exit(E_NETERR);
        }

        if  ((ret = gspool_getspu(gspool_fd, realuname, &mypriv)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                exit(E_NETERR);
        }
}

void  read_jobqueue(const unsigned afl)
{
        slotno_t        *slots;
        int             ret;

        disp_str = dest_hostname;

        if  ((ret = gspool_joblist(gspool_fd, afl, &Njobs, &slots)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                exit(0);
        }

        if  (Njobs > 0)  {
                int     cnt, actual, ret;

                if  (!(job_list = (struct apispq *) malloc((unsigned) (sizeof(struct apispq) * Njobs))))
                        html_nomem();
                if  (!(jslot_list = (slotno_t *) malloc((unsigned) (sizeof(slotno_t) * Njobs))))
                        html_nomem();

                for  (cnt = actual = 0;  cnt < Njobs;  cnt++)  {
                        if  ((ret = gspool_jobread(gspool_fd, GSPOOL_FLAG_IGNORESEQ, slots[cnt], &job_list[actual])) < 0)  {
                                if  (ret == GSPOOL_UNKNOWN_JOB)
                                        continue;
                                html_disperror($E{Base for API errors} + ret);
                                exit(E_NETERR);
                        }
                        jslot_list[actual] = slots[cnt];
                        actual++;
                }
                Njobs = actual;
        }
}

void  api_readptrs(const unsigned afl)
{
        slotno_t        *slots;
        int             ret;

        disp_str = dest_hostname;

        if  ((ret = gspool_ptrlist(gspool_fd, afl, &Nptrs, &slots)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                exit(E_NETERR);
        }

        if  (Nptrs > 0)  {
                int     cnt;
                if  (!(ptr_sl_list = (struct ptr_with_slot *) malloc((unsigned) (sizeof(struct ptr_with_slot) * Nptrs))))
                        html_nomem();
                for  (cnt = 0;  cnt < Nptrs;  cnt++)  {
                        ptr_sl_list[cnt].slot = slots[cnt];
                        if  ((ret = gspool_ptrread(gspool_fd, GSPOOL_FLAG_IGNORESEQ, slots[cnt], &ptr_sl_list[cnt].ptr)) < 0)  {
                                html_disperror($E{Base for API errors} + ret);
                                exit(E_NETERR);
                        }
                }
        }
}
