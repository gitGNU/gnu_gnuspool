/* rspccgi.c -- remote CGI ops on printers

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
#include "files.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "rcgilib.h"

char    *realuname;
int                     gspool_fd;
struct  apispdet        mypriv;

int                     Njobs, Nptrs;
struct  apispq          *job_list;
slotno_t                *jslot_list;
struct  ptr_with_slot   *ptr_sl_list;

/* For when we run out of memory.....  */

void  nomem()
{
        fprintf(stderr, "Ran out of memory\n");
        exit(E_NOMEM);
}

struct  argop  {
        const   char    *name;          /* Name of parameter case insens */
        int     (*arg_fn)(struct apispptr *, const struct argop *);
        short   typ;                    /* Type of parameter */
#define AO_BOOL         0
#define AO_ULONG        1
#define AO_STRING       2

        unsigned  char  off;                    /* Turn off */
        unsigned  char  had;
        union  {
                USHORT          ao_boolbit;
                ULONG           ao_ulong;
                char            *ao_string;
        }  ao_un;
        struct  argop   *next;
};

int  arg_netflags(struct apispptr *pp, const struct argop *ao)
{
        USHORT  bit = ao->ao_un.ao_boolbit;
        if  (ao->off)
                pp->apispp_netflags &= ~bit;
        else
                pp->apispp_netflags |= bit;
        return  1;
}

int  arg_class(struct apispptr *pp, const struct argop *ao)
{
        ULONG   rcl = ao->ao_un.ao_ulong;
        if  (!(mypriv.spu_flgs & PV_COVER))
                rcl &= mypriv.spu_class;
        if  (rcl == 0)
                return  0;
        pp->apispp_class = rcl;
        return  1;
}

int  arg_form(struct apispptr *pp, const struct argop *ao)
{
        strncpy(pp->apispp_form, ao->ao_un.ao_string, MAXFORM);
        return  1;
}

int  arg_dev(struct apispptr *pp, const struct argop *ao)
{
        strncpy(pp->apispp_dev, ao->ao_un.ao_string, LINESIZE);
        return  1;
}

int  arg_descr(struct apispptr *pp, const struct argop *ao)
{
        strncpy(pp->apispp_comment, ao->ao_un.ao_string, COMMENTSIZE);
        return  1;
}

struct  argop  aolist[] =  {
        {       "form", arg_form,       AO_STRING       },
        {       "dev",  arg_dev,        AO_STRING       },
        {       "descr",arg_descr,      AO_STRING       },
        {       "network",arg_netflags, AO_BOOL,        0,0,    { (USHORT) APISPP_LOCALNET }    },
        {       "loco", arg_netflags,   AO_BOOL,        0,0,    { (USHORT) APISPP_LOCALONLY }   },
        {       "class",arg_class,      AO_ULONG        }
};

struct  argop   *aochain;

struct  actop  {
        char    *name;
        USHORT  proc_running;           /* 1 If to be applied to running processes */
        USHORT  act_code;
}  actlist[] =  {
        {       "start",        0,      PRINOP_PGO      },
        {       "heoj",         1,      PRINOP_PHLT     },
        {       "halt",         1,      PRINOP_PSTP     },
        {       "alok",         1,      PRINOP_OYES     },
        {       "alnok",        1,      PRINOP_ONO      },
        {       "int",          1,      PRINOP_INTER    },
        {       "pab",          1,      PRINOP_PJAB     },
        {       "prst",         1,      PRINOP_RSP      }
};


void  list_op(char *arg, char *cp)
{
        int     cnt;

        *cp = '\0';

        for  (cnt = 0;  cnt < sizeof(aolist) / sizeof(struct argop);  cnt++)  {
                struct  argop  *aop = &aolist[cnt];
                if  (ncstrcmp(aop->name, arg) == 0)  {
                        *cp++ = '=';
                        switch  (aop->typ)  {
                        case  AO_BOOL:
                                switch  (*cp)  {
                                case  'y':case  'Y':
                                case  't':case  'T':
                                        aop->off = 0;
                                        break;
                                case  'n':case  'N':
                                case  'f':case  'F':
                                        aop->off = 255;
                                        break;
                                default:
                                        goto  badarg;
                                }
                                break;
                        case  AO_ULONG:
                                if  (!isdigit(*cp))
                                        goto  badarg;
                                aop->ao_un.ao_ulong = strtoul(cp, (char **) 0, 0);
                                break;
                        case  AO_STRING:
                                aop->ao_un.ao_string = cp;
                                break;
                        }
                        if  (!aop->had)  {
                                aop->had = 1;
                                aop->next = aochain;
                                aochain = aop;
                        }
                        return;
                }
        }

        *cp++ = '=';
 badarg:
        if  (html_out_cparam_file("badcarg", 1, arg))
                exit(E_USAGE);
        html_error(arg);
        exit(E_SETUP);
}

void  apply_ops(char *arg)
{
        int                     ret;
        struct  argop           *aop;
        struct  apispptr        ptr;
        struct  ptrswanted      pw;

        if  (!decode_pname(arg, &pw))  {
                if  (html_out_cparam_file("badcarg", 1, arg))
                        exit(E_USAGE);
                html_error(arg);
                exit(E_SETUP);
        }
        if  (gspool_ptrfind(gspool_fd, GSPOOL_FLAG_IGNORESEQ, pw.ptrname, pw.host, &pw.slot, &ptr) < 0)  {
                html_out_cparam_file("ptrgone", 1, arg);
                exit(E_NOJOB);
        }

        if  ((mypriv.spu_flgs & (PV_PRINQ|PV_HALTGO)) != (PV_PRINQ|PV_HALTGO) ||
             (ptr.apispp_netid != dest_hostid  && !(mypriv.spu_flgs & PV_REMOTEP)))  {
                html_out_cparam_file("nopriv", 1, arg);
                exit(E_NOPRIV);
        }

        if  (!aochain)          /* Nothing to do how boring */
                return;

        if  (ptr.apispp_state >= API_PRPROC)  {
                html_out_or_err("badstate", 1);
                exit(E_USAGE);
        }

        for  (aop = aochain;  aop;  aop = aop->next)
                if  (!(*aop->arg_fn)(&ptr, aop))  {
                        html_out_or_err("badargs", 1);
                        exit(E_USAGE);
                }

        if  ((ret = gspool_ptrupd(gspool_fd, GSPOOL_FLAG_IGNORESEQ, pw.slot, &ptr)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                exit(E_NOPRIV);
        }
}

void  apply_action(struct actop *aop, char *arg)
{
        int                     ret;
        struct  apispptr        ptr;
        struct  ptrswanted      pw;

        if  (!decode_pname(arg, &pw))  {
                if  (html_out_cparam_file("badcarg", 1, arg))
                        exit(E_USAGE);
                html_error(arg);
                exit(E_SETUP);
        }
        if  (gspool_ptrfind(gspool_fd, GSPOOL_FLAG_IGNORESEQ, pw.ptrname, pw.host, &pw.slot, &ptr) < 0)  {
                html_out_cparam_file("ptrgone", 1, arg);
                exit(E_NOJOB);
        }

        if  ((mypriv.spu_flgs & (PV_PRINQ|PV_HALTGO)) != (PV_PRINQ|PV_HALTGO) ||
             (ptr.apispp_netid  && !(mypriv.spu_flgs & PV_REMOTEP)))  {
                html_out_cparam_file("nopriv", 1, arg);
                exit(E_NOPRIV);
        }

        if  (ptr.apispp_state >= API_PRPROC)  {
                if  (!aop->proc_running)
                        goto  badstate;
        }
        else  if  (aop->proc_running)
                goto  badstate;

        if  ((ret = gspool_ptrop(gspool_fd, GSPOOL_FLAG_IGNORESEQ, pw.slot, aop->act_code)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                exit(E_SETUP);
        }
        return;

badstate:
        html_out_or_err("badstate", 1);
        exit(E_USAGE);
}

void  perform_update(char **args)
{
        char    **ap = args, *arg = *ap;

        if  (!arg)
                return;

        if  (!strchr(arg, '='))  {
                int     cnt;
                for  (cnt = 0;  cnt < sizeof(actlist)/sizeof(struct actop);  cnt++)  {
                        if  (ncstrcmp(actlist[cnt].name, arg) == 0)  {
                                for  (ap++;  (arg = *ap);  ap++)
                                        apply_action(&actlist[cnt],  arg);
                                return;
                        }
                }
                /* Drop through... */
        }

        for  (;  (arg = *ap);  ap++)  {
                char    *cp = strchr(arg, '=');
                if  (cp)
                        list_op(arg, cp);
                else
                        apply_ops(arg);
        }
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    **newargs;
        int_ugid_t      chku;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        html_openini();
        hash_hostfile();
        Effuid = geteuid();
        if  ((chku = lookup_uname(SPUNAME)) == UNKNOWN_UID)
                Daemuid = ROOTID;
        else
                Daemuid = chku;
        newargs = cgi_arginterp(argc, argv, CGI_AI_REMHOST|CGI_AI_SUBSID);
        /* Side effect of cgi_arginterp is to set Realuid */
        Cfile = open_cfile(MISC_UCONFIG, "rest.help");
        realuname = prin_uname(Realuid);
        setgid(getgid());
        setuid(Realuid);
        api_open(realuname);
        perform_update(newargs);
        html_out_or_err("chngok", 1);
        return  0;
}
