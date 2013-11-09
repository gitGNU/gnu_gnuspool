/* xtnet_api.c -- API handling for xtnetserv

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
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_sig.h"
#include <errno.h>
#include "errnums.h"
#include "incl_unix.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "spuser.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "xfershm.h"
#include "ecodes.h"
#include "client_if.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xtnet_ext.h"
#include "xtapi_int.h"
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

#define SLEEPTIME       5

extern  struct  sphdr   Spuhdr;

#ifdef  USING_FLOCK
#define JLOCK           jobshm_lock()
#define JUNLOCK         jobshm_unlock()
#define PTRS_LOCK       ptrshm_lock()
#define PTRS_UNLOCK     ptrshm_unlock()
#else
#define SEM_OP(buf, num)        while  (semop(Sem_chan, buf, num) < 0  &&  errno == EINTR)
#define JLOCK                   SEM_OP(jr, 2)
#define JUNLOCK                 SEM_OP(ju, 1)
#define PTRS_LOCK               SEM_OP(pr, 2)
#define PTRS_UNLOCK             SEM_OP(pu, 1)
#endif

struct  api_status      {
        int             sock;                   /* TCP socket */
        int             prodsock;               /* Socket for refresh messages */
        netid_t         hostid;                 /* Who we are speaking to 0=local host */
        int_ugid_t      realuid;                /* User id in question */
        classcode_t     classcode;              /* Class code in effect */
        enum  {  NOT_LOGGED = 0, LOGGED_IN_UNIX = 1, LOGGED_IN_WIN = 2, LOGGED_IN_WINU = 3 }    is_logged;
        ULONG           jser;                   /* Job shm serial */
        ULONG           pser;                   /* Ptr shm serial */
        struct  spdet   hispriv;                /* Permissions structure in effect */
        struct  api_msg inmsg;                  /* Input message */
        struct  api_msg outmsg;                 /* Output message */
        struct  sockaddr_in     apiaddr;        /* Address for binding prod socket */
        struct  sockaddr_in     apiret;         /* Address for sending prods */
};

extern int  rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
extern FILE *net_feed(const int, const netid_t, const slotno_t, const jobno_t);

/* Initialise status thing so we can have it auto */

static  void    init_status(struct api_status *ap)
{
        BLOCK_ZERO(ap, sizeof(struct api_status));
        ap->sock = ap->prodsock = -1;
}

/* Exit and abort pending jobs */

static  void  abort_exit(const int n)
{
        unsigned        cnt;
        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)  {
                struct  pend_job   *pj = &pend_list[cnt];
                if  (pj->out_f)  {
                        fclose(pj->out_f);
                        pj->out_f = (FILE *) 0;
                        unlink(pj->tmpfl);
                        unlink(pj->pgfl);
                }
        }
        exit(n);
}

/* Write messages to scheduler. */

static  void  womsg(struct api_status *ap, const int act, const ULONG arg)
{
        struct  spr_req         shrq;
        shrq.spr_mtype = MT_SCHED;
        shrq.spr_un.o.spr_act = (USHORT) act;
        shrq.spr_un.o.spr_arg1 = ap->realuid;
        shrq.spr_un.o.spr_jpslot = arg;
        shrq.spr_un.o.spr_pid = getpid();
        msgsnd(Ctrl_chan, (struct msgbuf *) &shrq, sizeof(struct sp_omsg), 0); /* Wait until it goes */
}

static  void  awjmsg(const int act, const slotno_t slot, struct spq *jp)
{
        struct  spr_req         shrq;
        shrq.spr_mtype = MT_SCHED;
        shrq.spr_un.j.spr_act = act;
        shrq.spr_un.j.spr_seq = 0;
        shrq.spr_un.j.spr_netid = 0;
        shrq.spr_un.j.spr_jslot = slot;
        shrq.spr_un.j.spr_pid = getpid();
        while  (wjmsg(&shrq, jp) != 0 && errno == EAGAIN)
                sleep(SLEEPTIME);
}

static  void  awpmsg(const int act, const slotno_t slot, struct spptr *pp)
{
        struct  spr_req         shrq;
        shrq.spr_mtype = MT_SCHED;
        shrq.spr_un.p.spr_act = act;
        shrq.spr_un.p.spr_seq = 0;
        shrq.spr_un.p.spr_netid = 0;
        shrq.spr_un.p.spr_pslot = slot;
        shrq.spr_un.p.spr_pid = getpid();
        while  (wpmsg(&shrq, pp) != 0 && errno == EAGAIN)
                sleep(SLEEPTIME);
}

static  void  setup_prod(struct api_status *ap)
{
        BLOCK_ZERO(&ap->apiret, sizeof(ap->apiret));
        ap->apiret.sin_family = AF_INET;
        ap->apiret.sin_addr.s_addr = htonl(INADDR_ANY);
        if  ((ap->prodsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                return;
        if  (bind(ap->prodsock, (struct sockaddr *) &ap->apiret, sizeof(ap->apiret)) < 0)  {
                close(ap->prodsock);
                ap->prodsock = -1;
        }
}

static  void  unsetup_prod(struct api_status *ap)
{
        if  (ap->prodsock >= 0)  {
                close(ap->prodsock);
                ap->prodsock = -1;
        }
}

static  void  proc_refresh(struct api_status *ap)
{
        int     prodj = 0, prodp = 0;

        if  (ap->jser != Job_seg.dptr->js_serial)  {
                ap->jser = Job_seg.dptr->js_serial;
                prodj++;
        }
        if  (ap->pser != Ptr_seg.dptr->ps_serial)  {
                ap->pser = Ptr_seg.dptr->ps_serial;
                prodp++;
        }

        if  (ap->prodsock < 0  ||  !(prodj || prodp))
                return;

        BLOCK_ZERO(&ap->apiaddr, sizeof(ap->apiaddr));
        ap->apiaddr.sin_family = AF_INET;
        ap->apiaddr.sin_addr.s_addr = ap->hostid;
        ap->apiaddr.sin_port = apipport;

        if  (prodj)  {
                ap->outmsg.code = API_JOBPROD;
                ap->outmsg.un.r_reader.seq = htonl(ap->jser);
                if  (sendto(ap->prodsock, (char *) &ap->outmsg, sizeof(ap->outmsg), 0, (struct sockaddr *) &ap->apiaddr, sizeof(ap->apiaddr)) < 0)  {
                        close(ap->prodsock);
                        ap->prodsock = -1;
                        return;
                }
        }
        if  (prodp)  {
                ap->outmsg.code = API_PTRPROD;
                ap->outmsg.un.r_reader.seq = htonl(ap->pser);
                if  (sendto(ap->prodsock, (char *) &ap->outmsg, sizeof(ap->outmsg), 0, (struct sockaddr *) &ap->apiaddr, sizeof(ap->apiaddr)) < 0)  {
                        close(ap->prodsock);
                        ap->prodsock = -1;
                        return;
                }
        }
}

static  void  pushout(struct api_status *ap, char *cbufp, unsigned obytes)
{
        int     xbytes;

        while  (obytes != 0)  {
                if  ((xbytes = write(ap->sock, cbufp, obytes)) < 0)  {
                        if  (errno == EINTR)
                                continue;
                        abort_exit(0);
                }
                cbufp += xbytes;
                obytes -= xbytes;
        }
}

static  void  pullin(struct api_status *ap, char *cbufp, unsigned ibytes)
{
        int     xbytes;

        while  (ibytes != 0)  {
                if  ((xbytes = read(ap->sock, cbufp, ibytes)) <= 0)  {
                        if  (xbytes < 0  &&  errno == EINTR)  {
                                while  (hadrfresh)  {
                                        hadrfresh = 0;
                                        proc_refresh(ap);
                                }
                                continue;
                        }
                        abort_exit(0);
                }
                cbufp += xbytes;
                ibytes -= xbytes;
        }
}

static  void    put_reply(struct api_status *ap)
{
        pushout(ap, (char *) &ap->outmsg, sizeof(ap->outmsg));
}

static  void    get_message(struct api_status *ap)
{
        pullin(ap, (char *) &ap->inmsg, sizeof(ap->inmsg));
}

static  void  err_result(struct api_status *ap, const int code, const ULONG seq)
{
        ap->outmsg.code = 0;
        ap->outmsg.retcode = htons((SHORT) code);
        ap->outmsg.un.r_reader.seq = htonl(seq);
        put_reply(ap);
}

static  void  swapinj(struct spq *to, const struct spq *from)
{
        to->spq_job = ntohl((ULONG) from->spq_job);
        to->spq_netid = 0L;
        to->spq_orighost = ext2int_netid_t(from->spq_orighost);
        to->spq_rslot = 0L;
        to->spq_time = ntohl((ULONG) from->spq_time);
        to->spq_proptime = 0L;
        to->spq_starttime = ntohl((ULONG) from->spq_starttime);
        to->spq_hold = ntohl((ULONG) from->spq_hold);
        to->spq_nptimeout = ntohs(from->spq_nptimeout);
        to->spq_ptimeout = ntohs(from->spq_ptimeout);
        to->spq_size = ntohl((ULONG) from->spq_size);
        to->spq_posn = ntohl((ULONG) from->spq_posn);
        to->spq_pagec = ntohl((ULONG) from->spq_pagec);
        to->spq_npages = ntohl((ULONG) from->spq_npages);

        to->spq_cps = from->spq_cps;
        to->spq_pri = from->spq_pri;
        to->spq_wpri = ntohs((USHORT) from->spq_wpri);

        to->spq_jflags = ntohs(from->spq_jflags);
        to->spq_sflags = from->spq_sflags;
        to->spq_dflags = from->spq_dflags;

        to->spq_extrn = ntohs(from->spq_extrn);
        to->spq_pglim = ntohs(from->spq_pglim);

        to->spq_class = ntohl(from->spq_class);
        to->spq_pslot = ntohl(-1L);

        to->spq_start = ntohl((ULONG) from->spq_start);
        to->spq_end = ntohl((ULONG) from->spq_end);
        to->spq_haltat = ntohl((ULONG) from->spq_haltat);

        to->spq_uid = ntohl(from->spq_uid);

        strncpy(to->spq_uname, from->spq_uname, UIDSIZE+1);
        strncpy(to->spq_puname, from->spq_puname, UIDSIZE+1);
        strncpy(to->spq_file, from->spq_file, MAXTITLE+1);
        strncpy(to->spq_form, from->spq_form, MAXFORM+1);
        strncpy(to->spq_ptr, from->spq_ptr, JPTRNAMESIZE+1);
        strncpy(to->spq_flags, from->spq_flags, MAXFLAGS+1);
}

static  void  swapinp(struct spptr *to, const struct spptr *from)
{
        to->spp_netid = from->spp_netid;
        to->spp_rslot = ntohl((ULONG) from->spp_rslot);

        to->spp_pid = ntohl((ULONG) from->spp_pid);
        to->spp_job = ntohl((ULONG) from->spp_job);
        to->spp_rjhostid = from->spp_rjhostid;
        to->spp_rjslot = ntohl((ULONG) from->spp_rjslot);
        to->spp_jslot = ntohl((ULONG) from->spp_jslot);

        to->spp_state = from->spp_state;
        to->spp_sflags = from->spp_sflags;
        to->spp_dflags = from->spp_dflags;
        to->spp_netflags = from->spp_netflags;

        to->spp_minsize = ntohl((ULONG) from->spp_minsize);
        to->spp_maxsize = ntohl((ULONG) from->spp_maxsize);
        to->spp_class = ntohl(from->spp_class);

        to->spp_extrn = ntohs(from->spp_extrn);
        to->spp_resvd = 0;

        strncpy(to->spp_dev, from->spp_dev, LINESIZE+1);
        strncpy(to->spp_form, from->spp_form, MAXFORM+1);
        strncpy(to->spp_ptr, from->spp_ptr, PTRNAMESIZE+1);
        strncpy(to->spp_feedback, from->spp_feedback, PFEEDBACK+1);
        strncpy(to->spp_comment, from->spp_comment, COMMENTSIZE+1);
}

/* Reread job file if necessary.  */

void  rerjobfile()
{
#ifdef  USING_MMAP
        if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)
#else
        if  (Job_seg.dinf.base != Job_seg.dptr->js_did)
#endif
        {
                JLOCK;
                jobgrown();
                JUNLOCK;
        }
}

/* Ditto printer list.  */

void  rerpfile()
{
#ifdef  USING_MMAP
        if  (Ptr_seg.inf.segsize != Job_seg.dptr->js_psegid)
#else
        if  (Ptr_seg.inf.base != Job_seg.dptr->js_psegid)
#endif
        {
                PTRS_LOCK;
                ptrgrown();
                PTRS_UNLOCK;
        }
}

static  void reply_joblist(struct api_status *ap)
{
        ULONG           flags = ntohl(ap->inmsg.un.lister.flags);
        unsigned        njobs;
        LONG            jind;
        slotno_t        *rbuf, *rbufp;

        ap->outmsg.code = API_JOBLIST;
        ap->outmsg.retcode = XT_OK;
        JLOCK;
#ifdef  USING_MMAP
        if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)
#else
        if  (Job_seg.dinf.base != Job_seg.dptr->js_did)
#endif
                jobgrown();
        njobs = 0;

        jind = Job_seg.dptr->js_q_head;
        while  (jind >= 0L)  {
                const  struct  spq  *jp = &Job_seg.jlist[jind].j;
                jind = Job_seg.jlist[jind].q_nxt;
                if  ((jp->spq_class & ap->classcode) == 0)
                        continue;
                if  (flags & XT_FLAG_LOCALONLY  &&  jp->spq_netid != 0)
                        continue;
                if  (flags & XT_FLAG_USERONLY  &&  jp->spq_uid != ap->realuid)
                        continue;
                njobs++;
        }

        ap->outmsg.un.r_lister.nitems = htonl((ULONG) njobs);
        ap->outmsg.un.r_lister.seq = htonl(Job_seg.dptr->js_serial);
        put_reply(ap);
        if  (njobs == 0)  {
                JUNLOCK;
                return;
        }
        if  (!(rbuf = (slotno_t *) malloc(njobs * sizeof(slotno_t))))
                nomem();
        rbufp = rbuf;
        jind = Job_seg.dptr->js_q_head;
        while  (jind >= 0L)  {
                LONG    nind = jind;
                const  struct  spq  *jp = &Job_seg.jlist[jind].j;
                jind = Job_seg.jlist[jind].q_nxt;
                if  ((jp->spq_class & ap->classcode) == 0)
                        continue;
                if  (flags & XT_FLAG_LOCALONLY  &&  jp->spq_netid != 0)
                        continue;
                if  (flags & XT_FLAG_USERONLY  &&  jp->spq_uid != ap->realuid)
                        continue;
                *rbufp++ = htonl((ULONG) nind);
        }
        JUNLOCK;

        /* Splat the thing out */

        pushout(ap, (char *) rbuf, sizeof(slotno_t) * njobs);
        free((char *) rbuf);
}

static  void  reply_ptrlist(struct api_status *ap)
{
        ULONG           flags = ntohl(ap->inmsg.un.lister.flags);
        LONG            pind;
        unsigned        nptrs;
        slotno_t        *rbuf, *rbufp;

        ap->outmsg.code = API_PTRLIST;
        ap->outmsg.retcode = XT_OK;
        PTRS_LOCK;
#ifdef  USING_MMAP
        if  (Ptr_seg.inf.segsize != Job_seg.dptr->js_psegid)
#else
        if  (Ptr_seg.inf.base != Job_seg.dptr->js_psegid)
#endif
                ptrgrown();
        nptrs = 0;
        pind = Ptr_seg.dptr->ps_l_head;
        while  (pind >= 0L)  {
                const  struct  spptr  *pp = &Ptr_seg.plist[pind].p;
                pind = Ptr_seg.plist[pind].l_nxt;
                if  (pp->spp_state == SPP_NULL)
                        continue;
                if  ((pp->spp_class & ap->classcode) == 0)
                        continue;
                if  (flags & XT_FLAG_LOCALONLY  &&  pp->spp_netid != 0)
                        continue;
                nptrs++;
        }
        ap->outmsg.un.r_lister.nitems = htonl((ULONG) nptrs);
        ap->outmsg.un.r_lister.seq = htonl(Ptr_seg.dptr->ps_serial);
        put_reply(ap);
        if  (nptrs == 0)  {
                PTRS_UNLOCK;
                return;
        }
        if  (!(rbuf = (slotno_t *) malloc(nptrs * sizeof(slotno_t))))
                nomem();
        rbufp = rbuf;
        pind = Ptr_seg.dptr->ps_l_head;
        while  (pind >= 0L)  {
                LONG    nind = pind;
                const  struct  spptr  *pp = &Ptr_seg.plist[pind].p;
                pind = Ptr_seg.plist[pind].l_nxt;
                if  (pp->spp_state == SPP_NULL)
                        continue;
                if  ((pp->spp_class & ap->classcode) == 0)
                        continue;
                if  (flags & XT_FLAG_LOCALONLY  &&  pp->spp_netid != 0)
                        continue;
                *rbufp++ = htonl((ULONG) nind);
        }
        PTRS_UNLOCK;

        /* Splat the thing out */

        pushout(ap, (char *) rbuf, sizeof(slotno_t) * nptrs);
        free((char *) rbuf);
}

static int check_valid_job(struct api_status *ap, const ULONG flags, const struct spq *jp)
{
        if  (jp->spq_job == 0 || (jp->spq_class & ap->classcode) == 0 ||
             ((flags & XT_FLAG_LOCALONLY)  &&  jp->spq_netid != 0)  ||
             ((flags & XT_FLAG_USERONLY)  &&  jp->spq_uid != ap->realuid))
                return  0;
        return  1;
}

static void  job_read_rest(struct api_status *ap, const struct spq *jp)
{
        struct  spq     outjob;

        outjob.spq_job = htonl((ULONG) jp->spq_job);
        outjob.spq_netid = int2ext_netid_t(jp->spq_netid);
        outjob.spq_orighost = int2ext_netid_t(jp->spq_orighost);
        outjob.spq_rslot = htonl(jp->spq_rslot);
        outjob.spq_time = htonl((ULONG) jp->spq_time);
        outjob.spq_proptime = 0L;
        outjob.spq_starttime = htonl((ULONG) jp->spq_starttime);
        outjob.spq_hold = htonl((ULONG) jp->spq_hold);
        outjob.spq_nptimeout = htons(jp->spq_nptimeout);
        outjob.spq_ptimeout = htons(jp->spq_ptimeout);
        outjob.spq_size = htonl((ULONG) jp->spq_size);
        outjob.spq_posn = htonl((ULONG) jp->spq_posn);
        outjob.spq_pagec = htonl((ULONG) jp->spq_pagec);
        outjob.spq_npages = htonl((ULONG) jp->spq_npages);

        outjob.spq_cps = jp->spq_cps;
        outjob.spq_pri = jp->spq_pri;
        outjob.spq_wpri = htons((USHORT) jp->spq_wpri);

        outjob.spq_jflags = htons(jp->spq_jflags);
        outjob.spq_sflags = jp->spq_sflags;
        outjob.spq_dflags = jp->spq_dflags;

        outjob.spq_extrn = htons(jp->spq_extrn);
        outjob.spq_pglim = htons(jp->spq_pglim);

        outjob.spq_class = htonl(jp->spq_class);
        outjob.spq_pslot = htonl(jp->spq_pslot);

        outjob.spq_start = htonl((ULONG) jp->spq_start);
        outjob.spq_end = htonl((ULONG) jp->spq_end);
        outjob.spq_haltat = htonl((ULONG) jp->spq_haltat);

        outjob.spq_uid = htonl(jp->spq_uid);

        strncpy(outjob.spq_uname, jp->spq_uname, UIDSIZE+1);
        strncpy(outjob.spq_puname, jp->spq_puname, UIDSIZE+1);
        strncpy(outjob.spq_file, jp->spq_file, MAXTITLE+1);
        strncpy(outjob.spq_form, jp->spq_form, MAXFORM+1);
        strncpy(outjob.spq_ptr, jp->spq_ptr, JPTRNAMESIZE+1);
        strncpy(outjob.spq_flags, jp->spq_flags, MAXFLAGS+1);
        pushout(ap, (char *) &outjob, sizeof(outjob));
}

static  void  reply_jobread(struct api_status *ap)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG   seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG   flags = ntohl(ap->inmsg.un.reader.flags);
        const  struct  spq  *jp;

        rerjobfile();
        if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
                err_result(ap, XT_SEQUENCE, Job_seg.dptr->js_serial);
                return;
        }
        if  (slotno >= Job_seg.dptr->js_maxjobs)  {
                err_result(ap, XT_INVALIDSLOT, Job_seg.dptr->js_serial);
                return;
        }
        jp = &Job_seg.jlist[slotno].j;
        if  (check_valid_job(ap, flags, jp))  {
                ap->outmsg.code = API_JOBREAD;
                ap->outmsg.retcode = XT_OK;
                ap->outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
                put_reply(ap);
                job_read_rest(ap, jp);
        }
        else
                err_result(ap, XT_UNKNOWN_JOB, Job_seg.dptr->js_serial);
}

static void reply_jobfind(struct api_status *ap)
{
        jobno_t jn = ntohl(ap->inmsg.un.jobfind.jobno);
        netid_t nid = ext2int_netid_t(ap->inmsg.un.jobfind.netid);
        ULONG   flags = ntohl(ap->inmsg.un.jobfind.flags);
        LONG    jind;
        const  struct  spq  *jp;

        JLOCK;
#ifdef  USING_MMAP
        if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)
#else
        if  (Job_seg.dinf.base != Job_seg.dptr->js_did)
#endif
                jobgrown();
        for  (jind = Job_seg.hashp_jno[jno_jhash(jn)]; jind >= 0L;
              jind = Job_seg.jlist[jind].nxt_jno_hash)  {
                const   Hashspq *hjp = &Job_seg.jlist[jind];
                if  (hjp->j.spq_job == jn  &&  hjp->j.spq_netid == nid)
                        goto  gotit;
        }
        JUNLOCK;
        goto  badjob;
 gotit:
        JUNLOCK;
        jp = &Job_seg.jlist[jind].j;
        if  (check_valid_job(ap, flags, jp))  {
                ap->outmsg.code = ap->inmsg.code;
                ap->outmsg.retcode = XT_OK;
                ap->outmsg.un.r_find.seq = htonl(Job_seg.dptr->js_serial);
                ap->outmsg.un.r_find.slotno = htonl((ULONG) jind);
                put_reply(ap);
                if  (ap->inmsg.code == API_FINDJOB)
                        job_read_rest(ap, jp);
                return;
        }
 badjob:
        err_result(ap, XT_UNKNOWN_JOB, Job_seg.dptr->js_serial);
}

static int  check_valid_ptr(struct api_status *ap, const ULONG flags, const struct spptr *pp)
{
        if  (pp->spp_state == SPP_NULL  ||
             (pp->spp_class & ap->classcode) == 0  ||
             ((flags & XT_FLAG_LOCALONLY)  &&  pp->spp_netid != 0))
                return  0;
        return  1;
}

static void  ptr_read_rest(struct api_status *ap, const struct spptr *pp)
{
        struct  spptr   outptr;

        outptr.spp_job = htonl((ULONG) pp->spp_job);
        outptr.spp_rjhostid = int2ext_netid_t(pp->spp_rjhostid);
        outptr.spp_rjslot = htonl((ULONG) pp->spp_rjslot);
        outptr.spp_jslot = htonl((ULONG) pp->spp_jslot);
        outptr.spp_minsize = htonl((ULONG) pp->spp_minsize);
        outptr.spp_maxsize = htonl((ULONG) pp->spp_maxsize);
        outptr.spp_netid = ext2int_netid_t(pp->spp_netid);
        outptr.spp_rslot = htonl((ULONG) pp->spp_rslot);
        outptr.spp_pid = htonl((ULONG) pp->spp_pid);
        outptr.spp_class = htonl(pp->spp_class);

        outptr.spp_resvd = 0;

        outptr.spp_state = pp->spp_state;
        outptr.spp_sflags = pp->spp_sflags;
        outptr.spp_dflags = pp->spp_dflags;
        outptr.spp_netflags = pp->spp_netflags;

        strncpy(outptr.spp_dev, pp->spp_dev, LINESIZE+1);
        strncpy(outptr.spp_form, pp->spp_form, MAXFORM+1);
        strncpy(outptr.spp_ptr, pp->spp_ptr, PTRNAMESIZE+1);
        strncpy(outptr.spp_feedback, pp->spp_feedback, PFEEDBACK+1);
        strncpy(outptr.spp_comment, pp->spp_comment, COMMENTSIZE+1);
        pushout(ap, (char *) &outptr, sizeof(outptr));
}

static void  reply_ptrread(struct api_status *ap)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG   seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG   flags = ntohl(ap->inmsg.un.reader.flags);
        const  struct  spptr  *pp;

        rerpfile();
        if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Ptr_seg.dptr->ps_serial)  {
                err_result(ap, XT_SEQUENCE, Ptr_seg.dptr->ps_serial);
                return;
        }
        if  (slotno >= Ptr_seg.dptr->ps_maxptrs)  {
                err_result(ap, XT_INVALIDSLOT, Ptr_seg.dptr->ps_serial);
                return;
        }

        pp = &Ptr_seg.plist[slotno].p;
        if  (check_valid_ptr(ap, flags, pp))  {
                ap->outmsg.code = API_PTRREAD;
                ap->outmsg.retcode = XT_OK;
                ap->outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
                put_reply(ap);
                ptr_read_rest(ap, pp);
        }
        else
                err_result(ap, XT_UNKNOWN_PTR, Ptr_seg.dptr->ps_serial);
}

static void reply_ptrfind(struct api_status *ap)
{
        netid_t         nid = ext2int_netid_t(ap->inmsg.un.ptrfind.netid);
        ULONG           flags = ntohl(ap->inmsg.un.ptrfind.flags);
        LONG            pind;
        const  struct  spptr  *pp;
        char            ptrname[PTRNAMESIZE+1];

        pullin(ap, ptrname, sizeof(ptrname));
        for  (pind = Ptr_seg.dptr->ps_l_head; pind >= 0L; pind = Ptr_seg.plist[pind].l_nxt)  {
                pp = &Ptr_seg.plist[pind].p;
                if  (pp->spp_netid == nid  &&  strcmp(pp->spp_ptr, ptrname) == 0)
                        goto  gotit;
        }
        goto  badptr;
 gotit:
        if  (check_valid_ptr(ap, flags, pp))  {
                ap->outmsg.code = ap->inmsg.code;
                ap->outmsg.retcode = XT_OK;
                ap->outmsg.un.r_find.seq = htonl(Ptr_seg.dptr->ps_serial);
                ap->outmsg.un.r_find.slotno = htonl((ULONG) pind);
                put_reply(ap);
                if  (ap->inmsg.code == API_FINDPTR)
                        ptr_read_rest(ap, pp);
                return;
        }
 badptr:
        err_result(ap, XT_UNKNOWN_PTR, Ptr_seg.dptr->ps_serial);
}

static int reply_jobdel(struct api_status *ap)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG   seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG   flags = ntohl(ap->inmsg.un.reader.flags);
        const  struct  spq  *jp;

        rerjobfile();

        if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
                return  XT_SEQUENCE;
        if  (slotno >= Job_seg.dptr->js_maxjobs)
                return  XT_INVALIDSLOT;
        jp = &Job_seg.jlist[slotno].j;
        if  (!check_valid_job(ap, flags, jp))
                return  XT_UNKNOWN_JOB;
        if  (!(ap->hispriv.spu_flgs & PV_OTHERJ)  &&  jp->spq_uid != ap->realuid)
                return  XT_NOPERM;
        if  (!(jp->spq_dflags & SPQ_PRINTED || flags & XT_FLAG_FORCE))
                return  XT_NOTPRINTED;
        womsg(ap, SO_AB, slotno);
        ap->outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
        return  XT_OK;
}

static  int reply_ptrop(struct api_status *ap, const ULONG op)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG   seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG   flags = ntohl(ap->inmsg.un.reader.flags);
        const  struct  spptr  *pp;
        unsigned        reqflag;
        int             mustberun;

        switch  (op)  {
        default:
                return  XT_UNKNOWN_COMMAND;
        case  SO_DELP:
                reqflag = PV_ADDDEL;
                mustberun = 0;
                break;
        case  SO_PGO:
                reqflag = PV_HALTGO;
                mustberun = 0;
                break;
        case  SO_PHLT:
        case  SO_PSTP:
        case  SO_INTER:
                reqflag = PV_HALTGO;
                mustberun = 1;
                break;
        case  SO_OYES:
        case  SO_ONO:
        case  SO_PJAB:
        case  SO_RSP:
                reqflag = PV_PRINQ;
                mustberun = 1;
                break;
        }
        if  (!(ap->hispriv.spu_flgs & reqflag))
                return  XT_NOPERM;
        rerpfile();
        if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Ptr_seg.dptr->ps_serial)
                return  XT_SEQUENCE;

        if  (slotno >= Ptr_seg.dptr->ps_maxptrs)
                return XT_INVALIDSLOT;

        pp = &Ptr_seg.plist[slotno].p;
        if  (!check_valid_ptr(ap, flags, pp))
                return  XT_UNKNOWN_PTR;

        if  (mustberun)  {
                if  (pp->spp_state < SPP_PROC)
                        return  XT_PTR_NOTRUNNING;
        }
        else  if  (pp->spp_state >= SPP_PROC)
                return  XT_PTR_RUNNING;

        womsg(ap, (int) op, slotno);
        ap->outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
        return  XT_OK;
}

static int reply_jobupd(struct api_status *ap)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG   seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG   flags = ntohl(ap->inmsg.un.reader.flags);
        const  struct  spq  *jp;
        struct  spq     injob, rjob;
        struct  spdet   *priv = &ap->hispriv;

        pullin(ap, (char *) &injob, sizeof(injob));
        swapinj(&rjob, &injob);

        rerjobfile();
        if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
                return  XT_SEQUENCE;

        if  (slotno >= Job_seg.dptr->js_maxjobs)
                return  XT_INVALIDSLOT;

        jp = &Job_seg.jlist[slotno].j;
        if  (!check_valid_job(ap, flags, jp))
                return  XT_UNKNOWN_JOB;
        if  (!(priv->spu_flgs & PV_OTHERJ)  &&  jp->spq_uid != Realuid)
                return  XT_NOPERM;

        /* Check bits about the changes which the user has to have
           permission to fiddle with */

        if  (!(priv->spu_flgs & PV_FORMS)  &&
             strcmp(jp->spq_form, rjob.spq_form) != 0  &&
             !qmatch(priv->spu_formallow, rjob.spq_form))
                return  XTNR_BAD_FORM;

        if  (!(priv->spu_flgs & PV_OTHERP)  &&
             strcmp(jp->spq_ptr, rjob.spq_ptr) != 0  &&
             !issubset(priv->spu_ptrallow, rjob.spq_ptr))
                return  XTNR_BAD_PTR;

        if  (jp->spq_pri != rjob.spq_pri)  {
                if  (rjob.spq_pri == 0)
                        return  XT_BAD_PRIORITY;
                if  (!(priv->spu_flgs & PV_CPRIO))
                        return  XT_BAD_PRIORITY;
                if  (!(priv->spu_flgs & PV_ANYPRIO)  &&
                     (rjob.spq_pri < priv->spu_minp || rjob.spq_pri > priv->spu_maxp))
                        return  XT_BAD_PRIORITY;
        }
        if  (jp->spq_cps != rjob.spq_cps  &&
             !(priv->spu_flgs & PV_ANYPRIO)  &&  rjob.spq_cps > priv->spu_cps)
                return  XT_BAD_COPIES;
        if  (!(priv->spu_flgs & PV_COVER))
                rjob.spq_class &= priv->spu_class;
        if  (rjob.spq_class == 0)
                return  XT_ZERO_CLASS;

        /* Quietly fix anything else we don't like.  */

        if  (rjob.spq_nptimeout == 0)
                rjob.spq_nptimeout = QNPTIMEOUT;
        if  (rjob.spq_ptimeout == 0)
                rjob.spq_ptimeout = QPTIMEOUT;
        if  (rjob.spq_start > rjob.spq_end)  {
                rjob.spq_start = 0L;
                rjob.spq_end = 0x7ffffffeL;
        }
        if  (rjob.spq_hold != 0  &&  (time_t) rjob.spq_hold < time((time_t *) 0))
                rjob.spq_hold = 0;

        awjmsg(SJ_CHNG, slotno, &rjob);
        ap->outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
        return  XT_OK;
}

static  int  reply_ptradd(struct api_status *ap)
{
        struct  spptr   inptr, rptr;
        struct  spdet   *priv = &ap->hispriv;

        pullin(ap, (char *) &inptr, sizeof(inptr));
        swapinp(&rptr, &inptr);
        if  (!(priv->spu_flgs & PV_ADDDEL))
                return  XT_NOPERM;
        if  (rptr.spp_dev[0] == '\0' ||  rptr.spp_ptr[0] == '\0'  ||  rptr.spp_form[0] == '\0')
                return  XT_PTR_NULL;
        if  (!(priv->spu_flgs & PV_COVER))
                rptr.spp_class &= priv->spu_class;
        if  (rptr.spp_class == 0)
                return  XT_ZERO_CLASS;
        rptr.spp_netid = 0;
        rptr.spp_minsize = 0;
        rptr.spp_maxsize = 0;
        awpmsg(SP_ADDP, 0, &rptr);
        ap->outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
        return  XT_OK;
}

static int reply_ptrupd(struct api_status *ap)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG   seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG   flags = ntohl(ap->inmsg.un.reader.flags);
        const   struct  spptr   *pp;
        struct  spptr   inptr, rptr;
        struct  spdet   *priv = &ap->hispriv;

        pullin(ap, (char *) &inptr, sizeof(inptr));
        swapinp(&rptr, &inptr);
        if  (!(priv->spu_flgs & PV_PRINQ))
                return  XT_NOPERM;

        rerpfile();
        if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Ptr_seg.dptr->ps_serial)
                return  XT_SEQUENCE;
        if  (slotno >= Ptr_seg.dptr->ps_maxptrs)
                return  XT_INVALIDSLOT;
        pp = &Ptr_seg.plist[slotno].p;
        if  (!check_valid_ptr(ap, flags, pp))
                return  XT_UNKNOWN_PTR;
        if  (pp->spp_state >= SPP_PROC)
                return  XT_PTR_RUNNING;

        if  (rptr.spp_dev[0] == '\0' ||  rptr.spp_ptr[0] == '\0'  ||  rptr.spp_form[0] == '\0')
                return  XT_PTR_NULL;

        /* Don't allow device changes if not allowed */

        if  (!(priv->spu_flgs & PV_ADDDEL)  &&  strcmp(rptr.spp_dev, pp->spp_dev) != 0)
                return  XT_PTR_CDEV;

        awpmsg(SP_CHGP, slotno, &rptr);
        ap->outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
        return  XT_OK;
}

static  void  api_jobstart(struct api_status *ap)
{
        int                     ret;
        char                    *dp = (char *) 0;
        struct  pend_job        *pj;
        struct  spq             injob;
        struct  pages           inpf;

        ap->outmsg.retcode = XT_OK;
        pullin(ap, (char *) &injob, sizeof(injob));

        if  (injob.spq_dflags & SPQ_PAGEFILE)  {        /* Don't need to swap - it's an unsigned char */
                unsigned    deliml;
                pullin(ap, (char *) &inpf, sizeof(inpf));
                deliml = ntohl(inpf.deliml);
                if  (!(dp = malloc((unsigned) deliml)))
                        nomem();
                pullin(ap, dp, (unsigned) deliml);
        }

        if  (!(pj = add_pend(ap->hostid)))  {
                ap->outmsg.retcode = htons(XT_NOMEM_QF);
                put_reply(ap);
                return;
        }

        swapinj(&pj->jobout, &injob);

        pj->jobout.spq_jflags &= ~(SPQ_CLIENTJOB|SPQ_ROAMUSER);

        if  (ap->is_logged != LOGGED_IN_UNIX)  {
                pj->jobout.spq_jflags |= SPQ_CLIENTJOB;
                if  (ap->is_logged == LOGGED_IN_WINU)
                        pj->jobout.spq_jflags |= SPQ_ROAMUSER;
        }
        pj->jobout.spq_uid = ap->realuid;
        strcpy(pj->jobout.spq_uname, prin_uname(ap->realuid));
        if  ((ret = validate_job(&pj->jobout)) != 0)  {
                ap->outmsg.retcode = htons((SHORT)XT_CONVERT_XTNR(ret));
                put_reply(ap);
                return;
        }
        pj->jobout.spq_size = 0; /* We don't believe a word or 4 bytes come to that */
        if  (pj->jobout.spq_dflags & SPQ_PAGEFILE)  {
                pj->pageout.delimnum = ntohl(inpf.delimnum);
                pj->pageout.deliml = ntohl(inpf.deliml);
        }
        else  {
                pj->pageout.delimnum = 1;
                pj->pageout.deliml = 1;
        }
        pj->delim = dp;
        pj->pageout.lastpage = 0;
        pj->jobn = ntohl(ap->inmsg.un.jobdata.jobno);
        pj->out_f = goutfile(&pj->jobn, pj->tmpfl, pj->pgfl, 1);
        ap->outmsg.un.jobdata.jobno = htonl(pj->jobn);
        put_reply(ap);
}

static  void  api_jobcont(struct api_status *ap)
{
        USHORT                  nbytes = ntohs(ap->inmsg.un.jobdata.nbytes);
        unsigned                cnt;
        unsigned        char    *bp;
        struct  pend_job        *pj;
        char    inbuffer[XTA_BUFFSIZE]; /* XTA_BUFFSIZE always >= nbytes */

        pullin(ap, inbuffer, nbytes);
        if  (!(pj = find_j_by_jno(ntohl(ap->inmsg.un.jobdata.jobno))))
                return;
        bp = (unsigned char *) inbuffer;
        for  (cnt = 0;  cnt < nbytes;  cnt++)  {
                if  (putc(*bp, pj->out_f) == EOF)  {
                        abort_job(pj);
                        return;
                }
                bp++;
                pj->jobout.spq_size++;
        }
}

static  void  api_jobfinish(struct api_status *ap)
{
        int                     ret;
        struct  pend_job        *pj;
        jobno_t                 jobno = ntohl(ap->inmsg.un.jobdata.jobno);

        ap->outmsg.code = API_DATAEND;
        ap->outmsg.un.jobdata.jobno = ap->inmsg.un.jobdata.jobno;               /* Don't need ntohl and back */

        if  (!(pj = find_j_by_jno(jobno)))
                ap->outmsg.retcode = htons(XT_UNKNOWN_JOB);
        else  if  ((ret = scan_job(pj)) != XTNQ_OK  &&  ret != XTNR_WARN_LIMIT)  {
                abort_job(pj);
                ap->outmsg.retcode = htons((SHORT)XT_CONVERT_XTNR(ret));
        }
        else  {
                ap->outmsg.retcode = htons(ret);
                pj->jobout.spq_job = jobno;
                pj->jobout.spq_time = time((time_t *) 0);
                pj->jobout.spq_orighost = ap->hostid;
                fclose(pj->out_f);
                awjmsg(SJ_ENQ, 0, &pj->jobout);
                pj->out_f = (FILE *) 0;
                if  (pj->delim)  {
                        free(pj->delim);
                        pj->delim = (char *) 0;
                }
        }
        put_reply(ap);
}

static  void  api_jobabort(struct api_status *ap)
{
        struct  pend_job        *pj;
        jobno_t                 jobno = ntohl(ap->inmsg.un.jobdata.jobno);

        ap->outmsg.code = API_DATAABORT;
        ap->outmsg.retcode = XT_OK;
        ap->outmsg.un.jobdata.jobno = htonl(jobno);
        if  (!(pj = find_j_by_jno(jobno)))
                ap->outmsg.retcode = htons(XT_UNKNOWN_JOB);
        else
                abort_job(pj);
        put_reply(ap);
}

static void api_jobdata(struct api_status *ap)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG   seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG   flags = ntohl(ap->inmsg.un.reader.flags);
        const  struct  spq  *jp;

        ap->outmsg.code = ap->inmsg.code;
        ap->outmsg.retcode = XT_OK;

        rerjobfile();
        ap->outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
        if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
                ap->outmsg.retcode = htons(XT_SEQUENCE);
                put_reply(ap);
                return;
        }

        if  (slotno >= Job_seg.dptr->js_maxjobs)  {
                ap->outmsg.retcode = htons(XT_INVALIDSLOT);
                put_reply(ap);
                return;
        }
        jp = &Job_seg.jlist[slotno].j;
        if  (!check_valid_job(ap, flags, jp))  {
                ap->outmsg.retcode = htons(XT_UNKNOWN_JOB);
                put_reply(ap);
                return;
        }
        if  (!(ap->hispriv.spu_flgs & PV_VOTHERJ)  &&  jp->spq_uid != ap->realuid)  {
                ap->outmsg.retcode = htons(XT_NOPERM);
                put_reply(ap);
                return;
        }

        if  (ap->inmsg.code == API_JOBPBRK)  {
                int             ret;
#ifndef WORDS_BIGENDIAN
                int             cnt;
#endif
                char            *delim;
                unsigned        pagenum = 0, deliml;
                LONG            *pageoffs = (LONG *) 0;
                struct  pages   pfp;
                if  ((ret = rdpgfile(jp, &pfp, &delim, &pagenum, &pageoffs)) == 0)  {
                        ap->outmsg.retcode = htons(XT_BAD_PF);
                        put_reply(ap);
                        return;
                }
                if  (ret < 0)  {
                        ap->outmsg.retcode = htons(XT_NOMEM_PF);
                        put_reply(ap);
                        return;
                }
                deliml = pfp.deliml;
#ifndef WORDS_BIGENDIAN
                pfp.delimnum = htonl(pfp.delimnum);
                pfp.deliml = htonl(pfp.deliml);
                pfp.lastpage = htonl(pfp.lastpage);
                for  (cnt = 0;  cnt <= jp->spq_npages;  cnt++)
                        pageoffs[cnt] = htonl(pageoffs[cnt]);
#endif

                /* Say ok, push out page structure, then delimiter,
                   then vector of offsets. Each is preceded by a message
                   saying how much to expect.  */

                put_reply(ap);
                ap->outmsg.code = API_DATAOUT;
                ap->outmsg.un.jobdata.jobno = htonl(jp->spq_job);
                ap->outmsg.un.jobdata.nbytes = htons(sizeof(pfp));
                put_reply(ap);
                pushout(ap, (char *) &pfp, sizeof(pfp));
                ap->outmsg.un.jobdata.nbytes = htons((USHORT) deliml);
                put_reply(ap);
                pushout(ap, delim, deliml);
                free(delim);
                deliml = (jp->spq_npages + 1) * sizeof(LONG);
                ap->outmsg.un.jobdata.nbytes = htons((USHORT) deliml);
                put_reply(ap);
                pushout(ap, (char *) pageoffs, deliml);
                free((char *) pageoffs);
        }
        else  {
                int     inbp, ch;
                FILE    *jfile;
                char    buffer[XTA_BUFFSIZE];
                jfile = jp->spq_netid?
                        net_feed(FEED_NPSP, jp->spq_netid, jp->spq_rslot, jp->spq_job):
                        fopen(mkspid(SPNAM, jp->spq_job), "r");
                if  (!jfile)  {
                        ap->outmsg.retcode = htons(XT_UNKNOWN_JOB);
                        put_reply(ap);
                        return;
                }

                /* Say ok (we set XT_OK earlier) */

                put_reply(ap);

                /* Read the file and splat it out.  */

                ap->outmsg.code = API_DATAOUT;
                ap->outmsg.un.jobdata.jobno = htonl(jp->spq_job);
                inbp = 0;
                while  ((ch = getc(jfile)) != EOF)  {
                        buffer[inbp++] = (char) ch;
                        if  (inbp >= sizeof(buffer))  {
                                ap->outmsg.un.jobdata.nbytes = htons(inbp);
                                put_reply(ap);
                                pushout(ap, buffer, inbp);
                                inbp = 0;
                        }
                }
                fclose(jfile);
                if  (inbp > 0)  {
                        ap->outmsg.un.jobdata.nbytes = htons(inbp);
                        put_reply(ap);
                        pushout(ap, buffer, inbp);
                        inbp = 0;
                }
        }

        /* Mark end of data */

        ap->outmsg.code = API_DATAEND;
        put_reply(ap);
}


void  process_pwchk(struct api_status *ap)
{
        char    pwbuf[API_PASSWDSIZE+1];

        ap->outmsg.code = ap->inmsg.code;
        strncpy(ap->outmsg.un.signon.username, ap->inmsg.un.signon.username, WUIDSIZE);
        ap->outmsg.retcode = htons(XT_NO_PASSWD);
        put_reply(ap);
        pullin(ap, pwbuf, sizeof(pwbuf));
        if  (!checkpw(prin_uname(ap->realuid), pwbuf))  {
                err_result(ap, XT_PASSWD_INVALID, 0);
                abort_exit(0);
        }
}

static  void    set_classcode(struct api_status *ap)
{
        /* NB!!! Assumes classcode in same position in "local_signon" as "signon" */
        ap->classcode = ntohl(ap->inmsg.un.signon.classcode);
        if  (!(ap->hispriv.spu_flgs & PV_COVER))
                ap->classcode &= ap->hispriv.spu_class;
        if  (ap->classcode == 0)
                ap->classcode = ap->hispriv.spu_class;
}

static  void    signon_ok(struct api_status *ap)
{
        int_ugid_t      uuid;

        if  ((uuid = lookup_uname(ap->inmsg.un.signon.username)) == UNKNOWN_UID)  {
                err_result(ap, XT_UNKNOWN_USER, 0);
                abort_exit(0);
        }
        ap->realuid = uuid;
        ap->hispriv = *getspuentry(ap->realuid);
        set_classcode(ap);
        ap->is_logged = LOGGED_IN_UNIX;
}

static  void  process_wlogin(struct api_status *ap)
{
        struct  winuhash  *wp;

        ap->realuid = (wp = lookup_winoruu(ap->inmsg.un.signon.username))?  wp->uuid: Defaultuid;
        process_pwchk(ap);
        ap->hispriv = *getspuentry(ap->realuid);
        set_classcode(ap);
        ap->is_logged = LOGGED_IN_WIN;
}

/* This is where we sign on without a password */

static  void  process_signon(struct api_status *ap)
{
        struct  hhash   *hp;
        struct  winuhash  *wp;
        struct  alhash  *alu;

        if  (!ap->hostid)  {            /* Local login, we believe the user name given */
                signon_ok(ap);
                return;
        }

        /* OK - we might be relying on it being a Windows user, or a UNIX-user from another host.
           See if we know the host - if not a Windows machine, accept the connection. */

        hp = lookup_hhash(ap->hostid);
        if  (hp  &&  !hp->isclient)  {
                signon_ok(ap);
                return;
        }

        /* See if we know the Windows user name */

        if  (!(wp = lookup_winoruu(ap->inmsg.un.signon.username)))  {
                err_result(ap, XT_UNKNOWN_USER, 0);
                return;
        }

        /* Check if it's an auto-login user and it matches */

        if  (!(alu = find_autoconn(ap->hostid))  ||  wp->uuid != alu->uuid)  {
                err_result(ap, XT_PASSWD_INVALID, 0);
                return;
        }
        ap->realuid = alu->uuid;
        ap->hispriv = *getspuentry(ap->realuid);
        set_classcode(ap);
        ap->is_logged = LOGGED_IN_WINU;
}

/* Login - presume it's a UNIX source unless we know better */

static  void    process_login(struct api_status *ap)
{
        struct  hhash   *hp;

        if  (!(ap->hostid  &&  (((hp = lookup_hhash(ap->hostid))  &&  hp->isclient)  ||  find_autoconn(ap->hostid))))  {
                /* We think it's a UNIX host */
                int_ugid_t  uuid;

                if  ((uuid = lookup_uname(ap->inmsg.un.signon.username)) == UNKNOWN_UID)  {
                        err_result(ap, XT_UNKNOWN_USER, 0);
                        abort_exit(0);
                }
                ap->realuid = uuid;
                process_pwchk(ap);
                ap->hispriv = *getspuentry(ap->realuid);
                set_classcode(ap);
                ap->is_logged = LOGGED_IN_UNIX;
        }
        else
                process_wlogin(ap);
}

static  void    process_locallogin(struct api_status *ap)
{
        int_ugid_t  fromuid, touid;
        struct  spdet   *mpriv;

        if  (ap->hostid)  {
                err_result(ap, XT_UNKNOWN_USER, 0);
                abort_exit(0);
        }

        fromuid = ntohl(ap->inmsg.un.local_signon.fromuser);
        touid = ntohl(ap->inmsg.un.local_signon.touser);
        if  (fromuid == UNKNOWN_UID  ||  touid == UNKNOWN_UID)  {
                err_result(ap, XT_UNKNOWN_USER, 0);
                abort_exit(0);
        }

        mpriv = getspuentry(fromuid);
        if  (fromuid != touid  &&  !(mpriv->spu_flgs & PV_MASQ))  {
                err_result(ap, XT_NOPERM, 0);
                abort_exit(0);
        }
        ap->realuid = touid;
        ap->hispriv = *mpriv;
        set_classcode(ap);
        ap->is_logged = LOGGED_IN_UNIX;
}

void  process_api()
{
        struct  api_status      apistat;
        int             cnt, ret;
        PIDTYPE         pid;
        struct  spdet   *mpriv;

        init_status(&apistat);

        if  ((apistat.sock = tcp_serv_accept(apirsock, &apistat.hostid)) < 0)
                return;

        if  ((pid = fork()) < 0)  {
                print_error($E{Internal cannot fork});
                return;
        }

#ifndef BUGGY_SIGCLD
        if  (pid != 0)  {
                close(apistat.sock);
                return;
        }
#else
        /* Make the process the grandchild so we don't have to worry
           about waiting for it later.  */

        if  (pid != 0)  {
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, (int *) 0, 0) < 0  &&  errno == EINTR)
                        ;
#else
                PIDTYPE wpid;
                while  ((wpid = wait((int *) 0)) != pid  &&  (wpid >= 0 || errno == EINTR))
                        ;
#endif
                close(apistat.sock);
                return;
        }
        if  (fork() != 0)
                exit(0);
#endif

        /* We are now a separate process...
           Clean up irrelevant stuff to do with pending UDP jobs
           See what the guy wants.
           At this stage we only want to "login".  */

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)  {
                struct  pend_job  *pj = &pend_list[cnt];
                if  (pj->out_f)  {
                        fclose(pj->out_f);
                        pj->out_f = (FILE *) 0;
                }
                if  (pj->delim)  {
                        free(pj->delim);
                        pj->delim = (char *) 0;
                }
                pj->clientfrom = 0;
        }

        /* We need to log in first before we get reminders about jobs and printers,
           so we don't need to worry, register it after we logged in */

        get_message(&apistat);

        switch  (apistat.inmsg.code)  {
        default:
                err_result(&apistat, XT_SEQUENCE, 0);
                abort_exit(0);

        case  API_WLOGIN:
                process_wlogin(&apistat);
                break;

        case  API_LOCALLOGIN:
                process_locallogin(&apistat);
                break;

        case  API_LOGIN:
                process_login(&apistat);
                break;

        case  API_SIGNON:
                process_signon(&apistat);
                break;
        }

        /* Ok we made it */

        apistat.outmsg.code = apistat.inmsg.code;
        apistat.outmsg.retcode = XT_OK;
        apistat.outmsg.un.r_signon.classcode = htonl(apistat.classcode);
        apistat.outmsg.un.r_signon.servuid = htonl(apistat.realuid);            /* Pass over realuid to save lookups */
        put_reply(&apistat);

        /* So do main loop waiting for something to happen */

        for  (;;)  {
                while  (hadrfresh)  {
                        hadrfresh = 0;
                        proc_refresh(&apistat);
                }

                get_message(&apistat);

                switch  (apistat.outmsg.code = apistat.inmsg.code)  {
                default:
                        ret = XT_UNKNOWN_COMMAND;
                        break;

                case  API_SIGNOFF:
                        abort_exit(0);

                case  API_JOBLIST:
                        reply_joblist(&apistat);
                        continue;

                case  API_PTRLIST:
                        reply_ptrlist(&apistat);
                        continue;

                case  API_JOBREAD:
                        reply_jobread(&apistat);
                        continue;

                case  API_PTRREAD:
                        reply_ptrread(&apistat);
                        continue;

                case  API_FINDJOBSLOT:
                case  API_FINDJOB:
                        reply_jobfind(&apistat);
                        continue;

                case  API_FINDPTRSLOT:
                case  API_FINDPTR:
                        reply_ptrfind(&apistat);
                        continue;

                case  API_JOBDEL:
                        ret = reply_jobdel(&apistat);
                        break;

                case  API_PTRDEL:
                        ret = reply_ptrop(&apistat, (ULONG) SO_DELP);
                        break;

                case  API_PTROP:
                        ret = reply_ptrop(&apistat, ntohl(apistat.inmsg.un.pop.op));
                        break;

                case  API_JOBADD:
                        api_jobstart(&apistat);
                        continue;
                case  API_DATAIN:
                        api_jobcont(&apistat);
                        continue;
                case  API_DATAEND:
                        api_jobfinish(&apistat);
                        continue;
                case  API_DATAABORT: /* We'll be lucky if we get this */
                        api_jobabort(&apistat);
                        continue;

                case  API_PTRADD:
                        ret = reply_ptradd(&apistat);
                        break;

                case  API_JOBUPD:
                        ret = reply_jobupd(&apistat);
                        break;

                case  API_PTRUPD:
                        ret = reply_ptrupd(&apistat);
                        break;

                case  API_JOBDATA:
                case  API_JOBPBRK:
                        api_jobdata(&apistat);
                        continue;

                case  API_REQPROD:
                        setup_prod(&apistat);
                        continue;

                case  API_UNREQPROD:
                        unsetup_prod(&apistat);
                        continue;

                case  API_GETSPU:
                {
                        int_ugid_t  ouid = apistat.realuid;
                        struct  spdet  outspdet;

                        /* Just pass a bufferful of nulls to mean current user (but support old way) */
                        if  (apistat.inmsg.un.us.username[0])  {
                                if  ((ouid = lookup_uname(apistat.inmsg.un.us.username)) == UNKNOWN_UID)  {
                                        ret = XT_UNKNOWN_USER;
                                        break;
                                }
                                if  (ouid != apistat.realuid  &&  !(apistat.hispriv.spu_flgs & PV_ADMIN))  {
                                        ret = XT_NOPERM;
                                        break;
                                }
                        }

                        /* Still re-read it in case something changed */

                        mpriv = getspuentry(ouid);
                        outspdet.spu_isvalid = mpriv->spu_isvalid;
                        outspdet.spu_user = htonl((ULONG) mpriv->spu_user);
                        outspdet.spu_minp = mpriv->spu_minp;
                        outspdet.spu_maxp = mpriv->spu_maxp;
                        outspdet.spu_defp = mpriv->spu_defp;
                        strncpy(outspdet.spu_form, mpriv->spu_form, MAXFORM);
                        strncpy(outspdet.spu_formallow, mpriv->spu_formallow, ALLOWFORMSIZE);
                        strncpy(outspdet.spu_ptr, mpriv->spu_ptr, PTRNAMESIZE);
                        strncpy(outspdet.spu_ptrallow, mpriv->spu_ptrallow, JPTRNAMESIZE);
                        outspdet.spu_flgs = htonl(mpriv->spu_flgs);
                        outspdet.spu_class = htonl(mpriv->spu_class);
                        outspdet.spu_cps = mpriv->spu_cps;
                        apistat.outmsg.retcode = XT_OK;
                        put_reply(&apistat);
                        pushout(&apistat, (char *) &outspdet, sizeof(outspdet));
                        continue;
                }
                case  API_GETSPD:
                {
                        struct  sphdr  outsphdr;
                        outsphdr.sph_version = Spuhdr.sph_version;
                        strncpy(outsphdr.sph_form, Spuhdr.sph_form, MAXFORM);
                        strncpy(outsphdr.sph_formallow, Spuhdr.sph_formallow, ALLOWFORMSIZE);
                        strncpy(outsphdr.sph_ptr, Spuhdr.sph_ptr, PTRNAMESIZE);
                        strncpy(outsphdr.sph_ptrallow, Spuhdr.sph_ptrallow, JPTRNAMESIZE);
                        outsphdr.sph_lastp = htonl(Spuhdr.sph_lastp);
                        outsphdr.sph_minp = Spuhdr.sph_minp;
                        outsphdr.sph_maxp = Spuhdr.sph_maxp;
                        outsphdr.sph_defp = Spuhdr.sph_defp;
                        outsphdr.sph_cps = Spuhdr.sph_cps;
                        outsphdr.sph_flgs = htonl(Spuhdr.sph_flgs);
                        outsphdr.sph_class = htonl(Spuhdr.sph_class);
                        apistat.outmsg.retcode = XT_OK;
                        put_reply(&apistat);
                        pushout(&apistat, (char *) &outsphdr, sizeof(outsphdr));
                        continue;
                }

                case  API_PUTSPU:
                {
                        int_ugid_t  ouid = apistat.realuid;
                        struct  spdet   inspdet, rspdet;

                        pullin(&apistat, (char *) &inspdet, sizeof(inspdet));

                        if  (apistat.inmsg.un.us.username[0])  {
                                if  ((ouid = lookup_uname(apistat.inmsg.un.us.username)) == UNKNOWN_UID)  {
                                        ret = XT_UNKNOWN_USER;
                                        break;
                                }
                                if  (!(apistat.hispriv.spu_flgs & PV_CDEFLT) || (ouid != apistat.realuid  &&  !(apistat.hispriv.spu_flgs & PV_ADMIN)))  {
                                        ret = XT_NOPERM;
                                        break;
                                }
                        }

                        rspdet.spu_isvalid = inspdet.spu_isvalid;
                        rspdet.spu_user = ouid;
                        rspdet.spu_minp = inspdet.spu_minp;
                        rspdet.spu_maxp = inspdet.spu_maxp;
                        rspdet.spu_defp = inspdet.spu_defp;
                        strncpy(rspdet.spu_form, inspdet.spu_form, MAXFORM);
                        strncpy(rspdet.spu_formallow, inspdet.spu_formallow, ALLOWFORMSIZE);
                        strncpy(rspdet.spu_ptr, inspdet.spu_ptr, PTRNAMESIZE);
                        strncpy(rspdet.spu_ptrallow, inspdet.spu_ptrallow, JPTRNAMESIZE);
                        rspdet.spu_flgs = ntohl(inspdet.spu_flgs);
                        rspdet.spu_class = ntohl(inspdet.spu_class);
                        rspdet.spu_cps = inspdet.spu_cps;
                        if  (rspdet.spu_class == 0)  {
                                ret = XT_ZERO_CLASS;
                                break;
                        }
                        if  (rspdet.spu_minp == 0 || rspdet.spu_maxp == 0 || rspdet.spu_defp == 0)  {
                                ret = XT_BAD_PRIORITY;
                                break;
                        }
                        if  (rspdet.spu_form[0] == '\0')  {
                                ret = XT_BAD_FORM;
                                break;
                        }

                        /* In case something has changed */

                        if  (ouid == apistat.realuid)  {
                                mpriv = getspuentry(ouid);
                                apistat.hispriv = *mpriv;
                                if  (!(apistat.hispriv.spu_flgs & PV_ADMIN))  {
                                        /* Disallow everything except prio and form
                                           We already checked PV_CDEFLT */
                                        if  (rspdet.spu_minp != apistat.hispriv.spu_minp ||
                                             rspdet.spu_maxp != apistat.hispriv.spu_maxp  ||
                                             rspdet.spu_flgs != apistat.hispriv.spu_flgs  ||
                                             rspdet.spu_class != apistat.hispriv.spu_class  ||
                                             rspdet.spu_cps != apistat.hispriv.spu_cps)  {
                                                ret = XT_NOPERM;
                                                break;
                                        }
                                }
                        }
                        mpriv = getspuentry(ouid);
                        mpriv->spu_minp = rspdet.spu_minp;
                        mpriv->spu_maxp = rspdet.spu_maxp;
                        mpriv->spu_defp = rspdet.spu_defp;
                        strncpy(mpriv->spu_form, rspdet.spu_form, MAXFORM);
                        strncpy(mpriv->spu_formallow, rspdet.spu_formallow, ALLOWFORMSIZE);
                        strncpy(mpriv->spu_ptr, rspdet.spu_ptr, PTRNAMESIZE);
                        strncpy(mpriv->spu_ptrallow, rspdet.spu_ptrallow, JPTRNAMESIZE);
                        mpriv->spu_flgs = rspdet.spu_flgs;
                        mpriv->spu_class = rspdet.spu_class;
                        mpriv->spu_cps = rspdet.spu_cps;
                        putspuentry(mpriv);
                        if  (ouid == apistat.realuid)
                                apistat.hispriv = *mpriv;
                        ret = XT_OK;
                        break;
                }

                case  API_PUTSPD:
                {
                        struct  sphdr   insphdr, rsphdr;

                        pullin(&apistat, (char *) &insphdr, sizeof(insphdr));
                        if  (!(apistat.hispriv.spu_flgs & PV_ADMIN))  {
                                ret = XT_NOPERM;
                                break;
                        }
                        strncpy(rsphdr.sph_form, insphdr.sph_form, MAXFORM);
                        strncpy(rsphdr.sph_formallow, insphdr.sph_formallow, ALLOWFORMSIZE);
                        strncpy(rsphdr.sph_ptr, insphdr.sph_ptr, PTRNAMESIZE);
                        strncpy(rsphdr.sph_ptrallow, insphdr.sph_ptrallow, JPTRNAMESIZE);
                        rsphdr.sph_minp = insphdr.sph_minp;
                        rsphdr.sph_maxp = insphdr.sph_maxp;
                        rsphdr.sph_defp = insphdr.sph_defp;
                        rsphdr.sph_cps = insphdr.sph_cps;
                        rsphdr.sph_flgs = ntohl(insphdr.sph_flgs);
                        rsphdr.sph_class = ntohl(insphdr.sph_class);
                        if  (rsphdr.sph_class == 0)  {
                                ret = XT_ZERO_CLASS;
                                break;
                        }
                        if  (rsphdr.sph_minp == 0 || rsphdr.sph_maxp == 0 || rsphdr.sph_defp == 0)  {
                                ret = XT_BAD_PRIORITY;
                                break;
                        }
                        if  (rsphdr.sph_form[0] == '\0')  {
                                ret = XT_BAD_FORM;
                                break;
                        }

                        /* Re-read file to get locking open */

                        mpriv = getspuentry(apistat.realuid);
                        apistat.hispriv = *mpriv;
                        Spuhdr.sph_minp = rsphdr.sph_minp;
                        Spuhdr.sph_maxp = rsphdr.sph_maxp;
                        Spuhdr.sph_defp = rsphdr.sph_defp;
                        strncpy(Spuhdr.sph_form, rsphdr.sph_form, MAXFORM);
                        strncpy(Spuhdr.sph_formallow, rsphdr.sph_formallow, ALLOWFORMSIZE);
                        strncpy(Spuhdr.sph_ptr, rsphdr.sph_ptr, PTRNAMESIZE);
                        strncpy(Spuhdr.sph_ptrallow, rsphdr.sph_ptrallow, JPTRNAMESIZE);
                        Spuhdr.sph_flgs = rsphdr.sph_flgs;
                        Spuhdr.sph_class = rsphdr.sph_class;
                        Spuhdr.sph_cps = rsphdr.sph_cps;
                        Spuhdr.sph_version = GNU_SPOOL_MAJOR_VERSION;
                        putspuhdr();
                        ret = XT_OK;
                        break;
                }
                }
                apistat.outmsg.retcode = htons(ret);
                put_reply(&apistat);
        }
}

