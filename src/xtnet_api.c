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
#ifdef	NETWORK_VERSION
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_sig.h"
#include <errno.h>
#include "errnums.h"
#include "incl_net.h"
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
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

#define	SLEEPTIME	5

static	int	prodsock = -1;
static	struct	sockaddr_in	apiaddr;
static	struct	sockaddr_in	apiret;

extern	struct	sphdr	Spuhdr;

#ifdef	USING_FLOCK
#define	JLOCK		jobshm_lock()
#define	JUNLOCK		jobshm_unlock()
#define	PTRS_LOCK	ptrshm_lock()
#define	PTRS_UNLOCK	ptrshm_unlock()
#else
#define	SEM_OP(buf, num)	while  (semop(Sem_chan, buf, num) < 0  &&  errno == EINTR)
#define	JLOCK			SEM_OP(jr, 2)
#define	JUNLOCK			SEM_OP(ju, 1)
#define	PTRS_LOCK		SEM_OP(pr, 2)
#define	PTRS_UNLOCK		SEM_OP(pu, 1)
#endif

extern int  rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
extern FILE *net_feed(const int, const netid_t, const slotno_t, const jobno_t);

/* Exit and abort pending jobs */

static	void  abort_exit(const int n)
{
	unsigned	cnt;
	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)  {
		struct	pend_job   *pj = &pend_list[cnt];
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

void  womsg(const int act, const ULONG arg)
{
	sp_req.spr_un.o.spr_act = (USHORT) act;
	sp_req.spr_un.o.spr_arg1 = Realuid;
	sp_req.spr_un.o.spr_jpslot = arg;
	sp_req.spr_un.o.spr_pid = getpid();
	msgsnd(Ctrl_chan, (struct msgbuf *) &sp_req, sizeof(struct sp_omsg), 0); /* Wait until it goes */
}

static	void  setup_prod()
{
	BLOCK_ZERO(&apiret, sizeof(apiret));
	apiret.sin_family = AF_INET;
	apiret.sin_addr.s_addr = htonl(INADDR_ANY);
	apiret.sin_port = 0;
	if  ((prodsock = socket(AF_INET, SOCK_DGRAM, udpproto)) < 0)
		return;
	if  (bind(prodsock, (struct sockaddr *) &apiret, sizeof(apiret)) < 0)  {
		close(prodsock);
		prodsock = -1;
	}
}

static	void  unsetup_prod()
{
	if  (prodsock >= 0)  {
		close(prodsock);
		prodsock = -1;
	}
}

static	void  proc_refresh(const netid_t whofrom)
{
	struct	api_msg	outmsg;
	static	ULONG	jser, pser;
	int	prodj = 0, prodp = 0;

	if  (jser != Job_seg.dptr->js_serial)  {
		jser = Job_seg.dptr->js_serial;
		prodj++;
	}
	if  (pser != Ptr_seg.dptr->ps_serial)  {
		pser = Ptr_seg.dptr->ps_serial;
		prodp++;
	}

	if  (prodsock < 0  ||  !(prodj || prodp))
		return;

	BLOCK_ZERO(&apiaddr, sizeof(apiaddr));
	apiaddr.sin_family = AF_INET;
	apiaddr.sin_addr.s_addr = whofrom;
	apiaddr.sin_port = apipport;

	if  (prodj)  {
		outmsg.code = API_JOBPROD;
		outmsg.un.r_reader.seq = htonl(jser);
		if  (sendto(prodsock, (char *) &outmsg, sizeof(outmsg), 0, (struct sockaddr *) &apiaddr, sizeof(apiaddr)) < 0)  {
			close(prodsock);
			prodsock = -1;
			return;
		}
	}
	if  (prodp)  {
		outmsg.code = API_PTRPROD;
		outmsg.un.r_reader.seq = htonl(pser);
		if  (sendto(prodsock, (char *) &outmsg, sizeof(outmsg), 0, (struct sockaddr *) &apiaddr, sizeof(apiaddr)) < 0)  {
			close(prodsock);
			prodsock = -1;
			return;
		}
	}
}

static	void  pushout(const int sock, char *cbufp, unsigned obytes)
{
	int	xbytes;

	while  (obytes != 0)  {
		if  ((xbytes = write(sock, cbufp, obytes)) < 0)  {
			if  (errno == EINTR)
				continue;
			abort_exit(0);
		}
		cbufp += xbytes;
		obytes -= xbytes;
	}
}

static	void  pullin(const int sock, char *cbufp, unsigned ibytes)
{
	int	xbytes;

	while  (ibytes != 0)  {
		if  ((xbytes = read(sock, cbufp, ibytes)) < 0)  {
			if  (errno == EINTR)
				continue;
			abort_exit(0);
		}
		cbufp += xbytes;
		ibytes -= xbytes;
	}
}

static	void  err_result(const int sock, const int code, const ULONG seq)
{
	struct	api_msg	outmsg;
	outmsg.code = 0;
	outmsg.retcode = htons((SHORT) code);
	outmsg.un.r_reader.seq = htonl(seq);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
}

static	void  swapinj(struct spq *to, const struct spq *from)
{
	to->spq_job = ntohl((ULONG) from->spq_job);
	to->spq_netid = 0L;
	to->spq_orighost = from->spq_orighost == myhostid || from->spq_orighost == localhostid? 0: from->spq_orighost;
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

static	void  swapinp(struct spptr *to, const struct spptr *from)
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
#ifdef	USING_MMAP
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
#ifdef	USING_MMAP
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

static	void reply_joblist(const int sock, const classcode_t classcode, const ULONG flags)
{

	unsigned	njobs;
	LONG		jind;
	slotno_t	*rbuf, *rbufp;
	struct	api_msg	outmsg;

	outmsg.code = API_JOBLIST;
	outmsg.retcode = 0;
	JLOCK;
#ifdef	USING_MMAP
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
		if  ((jp->spq_class & classcode) == 0)
			continue;
		if  (flags & XT_FLAG_LOCALONLY  &&  jp->spq_netid != 0)
			continue;
		if  (flags & XT_FLAG_USERONLY  &&  jp->spq_uid != Realuid)
			continue;
		njobs++;
	}

	outmsg.un.r_lister.nitems = htonl((ULONG) njobs);
	outmsg.un.r_lister.seq = htonl(Job_seg.dptr->js_serial);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (njobs == 0)  {
		JUNLOCK;
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "joblist", "OK-none");
		return;
	}
	if  (!(rbuf = (slotno_t *) malloc(njobs * sizeof(slotno_t))))
		nomem();
	rbufp = rbuf;
	jind = Job_seg.dptr->js_q_head;
	while  (jind >= 0L)  {
		LONG	nind = jind;
		const  struct  spq  *jp = &Job_seg.jlist[jind].j;
		jind = Job_seg.jlist[jind].q_nxt;
		if  ((jp->spq_class & classcode) == 0)
			continue;
		if  (flags & XT_FLAG_LOCALONLY  &&  jp->spq_netid != 0)
			continue;
		if  (flags & XT_FLAG_USERONLY  &&  jp->spq_uid != Realuid)
			continue;
		*rbufp++ = htonl((ULONG) nind);
	}
	JUNLOCK;

	/* Splat the thing out */

	pushout(sock, (char *) rbuf, sizeof(slotno_t) * njobs);
	free((char *) rbuf);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "joblist", "OK");
}

static	void  reply_ptrlist(const int sock, const classcode_t classcode, const ULONG flags)
{
	LONG		pind;
	unsigned	nptrs;
	slotno_t	*rbuf, *rbufp;
	struct	api_msg	outmsg;

	outmsg.code = API_PTRLIST;
	outmsg.retcode = 0;
	PTRS_LOCK;
#ifdef	USING_MMAP
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
		if  ((pp->spp_class & classcode) == 0)
			continue;
		if  (flags & XT_FLAG_LOCALONLY  &&  pp->spp_netid != 0)
			continue;
		nptrs++;
	}
	outmsg.un.r_lister.nitems = htonl((ULONG) nptrs);
	outmsg.un.r_lister.seq = htonl(Ptr_seg.dptr->ps_serial);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (nptrs == 0)  {
		PTRS_UNLOCK;
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrlist", "OK-none");
		return;
	}
	if  (!(rbuf = (slotno_t *) malloc(nptrs * sizeof(slotno_t))))
		nomem();
	rbufp = rbuf;
	pind = Ptr_seg.dptr->ps_l_head;
	while  (pind >= 0L)  {
		LONG	nind = pind;
		const  struct  spptr  *pp = &Ptr_seg.plist[pind].p;
		pind = Ptr_seg.plist[pind].l_nxt;
		if  (pp->spp_state == SPP_NULL)
			continue;
		if  ((pp->spp_class & classcode) == 0)
			continue;
		if  (flags & XT_FLAG_LOCALONLY  &&  pp->spp_netid != 0)
			continue;
		*rbufp++ = htonl((ULONG) nind);
	}
	PTRS_UNLOCK;

	/* Splat the thing out */

	pushout(sock, (char *) rbuf, sizeof(slotno_t) * nptrs);
	free((char *) rbuf);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "ptrlist", "OK");
}

static int check_valid_job(const classcode_t classcode, const ULONG flags, const struct spq *jp, const	char *tmsg)
{
	if  (jp->spq_job == 0 || (jp->spq_class & classcode) == 0 ||
	     ((flags & XT_FLAG_LOCALONLY)  &&  jp->spq_netid != 0)  ||
	     ((flags & XT_FLAG_USERONLY)  &&  jp->spq_uid != Realuid))  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, tmsg, "unkjob");
		return  0;
	}
	return  1;
}

static void  job_read_rest(const int sock, const struct spq *jp)
{
	struct	spq	outjob;

	outjob.spq_job = htonl((ULONG) jp->spq_job);
	outjob.spq_netid = jp->spq_netid == 0? myhostid: jp->spq_netid;
	outjob.spq_orighost = jp->spq_orighost == 0? myhostid: jp->spq_orighost;
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
	pushout(sock, (char *) &outjob, sizeof(outjob));
}

static	void  reply_jobread(const int sock, const classcode_t classcode, const slotno_t	slotno, const ULONG seq, const ULONG flags)
{
	const  struct  spq  *jp;

	rerjobfile();
	if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
		err_result(sock, XT_SEQUENCE, Job_seg.dptr->js_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobread", "seqerr");
		return;
	}
	if  (slotno >= Job_seg.dptr->js_maxjobs)  {
		err_result(sock, XT_INVALIDSLOT, Job_seg.dptr->js_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobread", "invslot");
		return;
	}
	jp = &Job_seg.jlist[slotno].j;
	if  (check_valid_job(classcode, flags, jp, "jobread"))  {
		struct	api_msg	outmsg;
		outmsg.code = API_JOBREAD;
		outmsg.retcode = 0;
		outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		job_read_rest(sock, jp);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobread", "OK");
	}
	else
		err_result(sock, XT_UNKNOWN_JOB, Job_seg.dptr->js_serial);
}

static void reply_jobfind(const	int sock, const	classcode_t classcode, const unsigned code, const jobno_t jn, const netid_t nid, const ULONG flags)
{
	LONG	jind;
	const  struct  spq  *jp;

	JLOCK;
#ifdef	USING_MMAP
	if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)
#else
	if  (Job_seg.dinf.base != Job_seg.dptr->js_did)
#endif
		jobgrown();
	for  (jind = Job_seg.hashp_jno[jno_jhash(jn)]; jind >= 0L;
	      jind = Job_seg.jlist[jind].nxt_jno_hash)  {
		const	Hashspq	*hjp = &Job_seg.jlist[jind];
		if  (hjp->j.spq_job == jn  &&  hjp->j.spq_netid == nid)
			goto  gotit;
	}
	JUNLOCK;
	goto  badjob;
 gotit:
	JUNLOCK;
	jp = &Job_seg.jlist[jind].j;
	if  (check_valid_job(classcode, flags, jp, "findjob"))  {
		struct	api_msg	outmsg;
		outmsg.code = code;
		outmsg.retcode = 0;
		outmsg.un.r_find.seq = htonl(Job_seg.dptr->js_serial);
		outmsg.un.r_find.slotno = htonl((ULONG) jind);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (code == API_FINDJOB)
			job_read_rest(sock, jp);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "findjob", "OK");
		return;
	}
 badjob:
	err_result(sock, XT_UNKNOWN_JOB, Job_seg.dptr->js_serial);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "findjob", "unkjob");
}

static int  check_valid_ptr(const classcode_t classcode, const ULONG flags, const struct spptr *pp, const char *tmsg)
{
	if  (pp->spp_state == SPP_NULL  ||
	     (pp->spp_class & classcode) == 0  ||
	     ((flags & XT_FLAG_LOCALONLY)  &&  pp->spp_netid != 0))  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, tmsg, "unkptr");
		return  0;
	}
	return  1;
}

static void  ptr_read_rest(const int sock, const struct spptr *pp)
{
	struct	spptr	outptr;

	outptr.spp_job = htonl((ULONG) pp->spp_job);
	outptr.spp_rjhostid = pp->spp_rjhostid == 0? myhostid: pp->spp_rjhostid;
	outptr.spp_rjslot = htonl((ULONG) pp->spp_rjslot);
	outptr.spp_jslot = htonl((ULONG) pp->spp_jslot);
	outptr.spp_minsize = htonl((ULONG) pp->spp_minsize);
	outptr.spp_maxsize = htonl((ULONG) pp->spp_maxsize);
	outptr.spp_netid = pp->spp_netid == 0? myhostid: pp->spp_netid;
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
	pushout(sock, (char *) &outptr, sizeof(outptr));
}

static void  reply_ptrread(const int sock, const classcode_t classcode, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	const  struct  spptr  *pp;

	rerpfile();
	if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Ptr_seg.dptr->ps_serial)  {
		err_result(sock, XT_SEQUENCE, Ptr_seg.dptr->ps_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrread", "seq");
		return;
	}
	if  (slotno >= Ptr_seg.dptr->ps_maxptrs)  {
		err_result(sock, XT_INVALIDSLOT, Ptr_seg.dptr->ps_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrread", "invslot");
		return;
	}

	pp = &Ptr_seg.plist[slotno].p;
	if  (check_valid_ptr(classcode, flags, pp, "ptrread"))  {
		struct	api_msg	outmsg;
		outmsg.code = API_PTRREAD;
		outmsg.retcode = 0;
		outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		ptr_read_rest(sock, pp);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrread", "OK");
	}
	else
		err_result(sock, XT_UNKNOWN_PTR, Ptr_seg.dptr->ps_serial);
}

static void reply_ptrfind(const	int sock, const	classcode_t classcode, const unsigned code, const netid_t nid, const ULONG flags)
{
	LONG		pind;
	const  struct  spptr  *pp;
	char		ptrname[PTRNAMESIZE+1];

	pullin(sock, ptrname, sizeof(ptrname));
	for  (pind = Ptr_seg.dptr->ps_l_head; pind >= 0L; pind = Ptr_seg.plist[pind].l_nxt)  {
		pp = &Ptr_seg.plist[pind].p;
		if  (pp->spp_netid == nid  &&  strcmp(pp->spp_ptr, ptrname) == 0)
			goto  gotit;
	}
	goto  badptr;
 gotit:
	if  (check_valid_ptr(classcode, flags, pp, "findptr"))  {
		struct	api_msg	outmsg;
		outmsg.code = code;
		outmsg.retcode = 0;
		outmsg.un.r_find.seq = htonl(Ptr_seg.dptr->ps_serial);
		outmsg.un.r_find.slotno = htonl((ULONG) pind);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (code == API_FINDPTR)
			ptr_read_rest(sock, pp);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "findptr", "OK");
		return;
	}
 badptr:
	err_result(sock, XT_UNKNOWN_PTR, Ptr_seg.dptr->ps_serial);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "findptr", "unkptr");
}


static int reply_jobdel(const struct spdet *priv, const classcode_t classcode, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	const  struct  spq  *jp;
	rerjobfile();

	if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdel", "seq");
		return  XT_SEQUENCE;
	}

	if  (slotno >= Job_seg.dptr->js_maxjobs)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdel", "invslot");
		return  XT_INVALIDSLOT;
	}

	jp = &Job_seg.jlist[slotno].j;
	if  (!check_valid_job(classcode, flags, jp, "jobdel"))
		return  XT_UNKNOWN_JOB;
	if  (!(priv->spu_flgs & PV_OTHERJ)  &&  jp->spq_uid != Realuid)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdel", "noperm");
		return  XT_NOPERM;
	}
	if  (!(jp->spq_dflags & SPQ_PRINTED || flags & XT_FLAG_FORCE))  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdel", "notptd");
		return  XT_NOTPRINTED;
	}
	womsg(SO_AB, slotno);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "jobdel", "OK");
	return  XT_OK;
}

static	int reply_ptrop(const struct spdet *priv, const classcode_t classcode, const slotno_t slotno, const ULONG seq, const ULONG flags, const ULONG op)
{
	const  struct  spptr  *pp;
	unsigned	reqflag;
	int		mustberun;

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
	if  (!(priv->spu_flgs & reqflag))  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrop", "noperm");
		return  XT_NOPERM;
	}
	rerpfile();
	if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Ptr_seg.dptr->ps_serial)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrop", "seq");
		return  XT_SEQUENCE;
	}

	if  (slotno >= Ptr_seg.dptr->ps_maxptrs)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrop", "invslot");
		return XT_INVALIDSLOT;
	}

	pp = &Ptr_seg.plist[slotno].p;
	if  (!check_valid_ptr(classcode, flags, pp, "ptrop"))
		return  XT_UNKNOWN_PTR;

	if  (mustberun)  {
		if  (pp->spp_state < SPP_PROC)  {
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "ptrop", "notrun");
			return  XT_PTR_NOTRUNNING;
		}
	}
	else  if  (pp->spp_state >= SPP_PROC)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrop", "running");
		return  XT_PTR_RUNNING;
	}

	womsg((int) op, slotno);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "ptrop", "OK");
	return  XT_OK;
}

static int reply_jobupd(const int sock, struct spdet *priv, const classcode_t classcode, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	const  struct  spq  *jp;
	struct	spq	injob, rjob;

	pullin(sock, (char *) &injob, sizeof(injob));
	swapinj(&rjob, &injob);

	rerjobfile();
	if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobupd", "seqerr");
		return  XT_SEQUENCE;
	}
	if  (slotno >= Job_seg.dptr->js_maxjobs)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobupd", "invslot");
		return  XT_INVALIDSLOT;
	}
	jp = &Job_seg.jlist[slotno].j;
	if  (!check_valid_job(classcode, flags, jp, "jobupd"))
		return  XT_UNKNOWN_JOB;
	if  (!(priv->spu_flgs & PV_OTHERJ)  &&  jp->spq_uid != Realuid)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobupd", "noperm");
		return  XT_NOPERM;
	}

	/* Check bits about the changes which the user has to have
	   permission to fiddle with */

	if  (!(priv->spu_flgs & PV_FORMS)  &&
	     strcmp(jp->spq_form, rjob.spq_form) != 0  &&
	     !qmatch(priv->spu_formallow, rjob.spq_form))  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobupd", "badform");
		return  XTNR_BAD_FORM;
	}
	if  (!(priv->spu_flgs & PV_OTHERP)  &&
	     strcmp(jp->spq_ptr, rjob.spq_ptr) != 0  &&
	     !issubset(priv->spu_ptrallow, rjob.spq_ptr))  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobupd", "badptr");
		return  XTNR_BAD_PTR;
	}

	if  (jp->spq_pri != rjob.spq_pri)  {
		if  (rjob.spq_pri == 0)  {
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "jobupd", "badpri");
			return  XT_BAD_PRIORITY;
		}
		if  (!(priv->spu_flgs & PV_CPRIO))  {
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "jobupd", "badpri");
			return  XT_BAD_PRIORITY;
		}
		if  (!(priv->spu_flgs & PV_ANYPRIO)  &&
		     (rjob.spq_pri < priv->spu_minp || rjob.spq_pri > priv->spu_maxp))  {
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "jobupd", "badpri");
			return  XT_BAD_PRIORITY;
		}
	}
	if  (jp->spq_cps != rjob.spq_cps  &&
	     !(priv->spu_flgs & PV_ANYPRIO)  &&  rjob.spq_cps > priv->spu_cps)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobupd", "badcps");
		return  XT_BAD_COPIES;
	}
	if  (!(priv->spu_flgs & PV_COVER))
		rjob.spq_class &= priv->spu_class;
	if  (rjob.spq_class == 0)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobupd", "zerocl");
		return  XT_ZERO_CLASS;
	}

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

	/* Well we seem to have run out of excuses for not doing the
	   business don't we....  */

	sp_req.spr_un.j.spr_act = SJ_CHNG;
	sp_req.spr_un.j.spr_jslot = slotno;
	sp_req.spr_un.j.spr_pid = getpid();
	while  (wjmsg(&sp_req, &rjob) != 0  &&  errno == EAGAIN)
		sleep(SLEEPTIME);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "jobupd", "OK");
	return  XT_OK;
}

static	int  reply_ptradd(const int sock, const struct spdet *priv)
{
	struct	spptr	inptr, rptr;

	pullin(sock, (char *) &inptr, sizeof(inptr));
	swapinp(&rptr, &inptr);
	if  (!(priv->spu_flgs & PV_ADDDEL))  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptradd", "noperm");
		return  XT_NOPERM;
	}
	if  (rptr.spp_dev[0] == '\0' ||  rptr.spp_ptr[0] == '\0'  ||  rptr.spp_form[0] == '\0')  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptradd", "nullfield");
		return  XT_PTR_NULL;
	}
	if  (!(priv->spu_flgs & PV_COVER))
		rptr.spp_class &= priv->spu_class;
	if  (rptr.spp_class == 0)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptradd", "zerocl");
		return  XT_ZERO_CLASS;
	}
	rptr.spp_netid = 0;
	rptr.spp_minsize = 0;
	rptr.spp_maxsize = 0;
	sp_req.spr_un.p.spr_act = SP_ADDP;
	sp_req.spr_un.p.spr_pid = getpid();
	while  (wpmsg(&sp_req, &rptr) != 0  &&  errno == EAGAIN)
		sleep(SLEEPTIME);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "ptradd", "OK");
	return  XT_OK;
}

static int reply_ptrupd(const int sock, const struct spdet *priv, const classcode_t classcode, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	const	struct	spptr	*pp;
	struct	spptr	inptr, rptr;

	pullin(sock, (char *) &inptr, sizeof(inptr));
	swapinp(&rptr, &inptr);
	if  (!(priv->spu_flgs & PV_PRINQ))  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrupd", "noperm");
		return  XT_NOPERM;
	}

	rerpfile();
	if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Ptr_seg.dptr->ps_serial)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrupd", "seq");
		return  XT_SEQUENCE;
	}
	if  (slotno >= Ptr_seg.dptr->ps_maxptrs)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrupd", "invslot");
		return  XT_INVALIDSLOT;
	}
	pp = &Ptr_seg.plist[slotno].p;
	if  (!check_valid_ptr(classcode, flags, pp, "ptrupd"))
		return  XT_UNKNOWN_PTR;
	if  (pp->spp_state >= SPP_PROC)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrupd", "running");
		return  XT_PTR_RUNNING;
	}

	if  (rptr.spp_dev[0] == '\0' ||  rptr.spp_ptr[0] == '\0'  ||  rptr.spp_form[0] == '\0')  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrupd", "nullfld");
		return  XT_PTR_NULL;
	}

	/* Don't allow device changes if not allowed */

	if  (!(priv->spu_flgs & PV_ADDDEL)  &&  strcmp(rptr.spp_dev, pp->spp_dev) != 0)  {
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "ptrupd", "nopriv");
		return  XT_PTR_CDEV;
	}

	/* Well we seem to have run out of excuses for not doing the
	   business don't we....  */

	sp_req.spr_un.p.spr_act = SP_CHGP;
	sp_req.spr_un.p.spr_pslot = slotno;
	sp_req.spr_un.p.spr_pid = getpid();
	while  (wpmsg(&sp_req, &rptr) != 0  &&  errno == EAGAIN)
		sleep(SLEEPTIME);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "ptrupd", "OK");
	return  XT_OK;
}

static	void  api_jobstart(const int sock, struct hhash *frp, const jobno_t jobno)
{
	int			ret;
	char			*dp = (char *) 0;
	struct	pend_job	*pj;
	struct	spq		injob;
	struct	pages		inpf;
	struct	api_msg		outmsg;

	outmsg.code = API_JOBADD;
	outmsg.retcode = 0;
	pullin(sock, (char *) &injob, sizeof(injob));
	if  (injob.spq_dflags & SPQ_PAGEFILE)  {	/* Don't need to swap - it's an unsigned char */
		unsigned    deliml;
		pullin(sock, (char *) &inpf, sizeof(inpf));
		deliml = ntohl(inpf.deliml);
		if  (!(dp = malloc((unsigned) deliml)))
			nomem();
		pullin(sock, dp, (unsigned) deliml);
	}

	if  (!(pj = add_pend(frp->rem.hostid)))  {
		outmsg.retcode = htons(XT_NOMEM_QF);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobadd", "nomem");
		return;
	}
	swapinj(&pj->jobout, &injob);
	pj->jobout.spq_jflags &= ~(SPQ_CLIENTJOB|SPQ_ROAMUSER);
	if  (frp->rem.ht_flags & HT_DOS)  {
		pj->jobout.spq_jflags |= SPQ_CLIENTJOB;
		if  (frp->rem.ht_flags & HT_ROAMUSER)
			pj->jobout.spq_jflags |= SPQ_ROAMUSER;
	}
	pj->jobout.spq_uid = Realuid;
	strcpy(pj->jobout.spq_uname, prin_uname(Realuid));
	if  ((ret = validate_job(&pj->jobout)) != 0)  {
		outmsg.retcode = htons((SHORT)XT_CONVERT_XTNR(ret));
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobadd", "invjob");
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
	pj->jobn = jobno;
	pj->out_f = goutfile(&pj->jobn, pj->tmpfl, pj->pgfl, 1);
	outmsg.un.jobdata.jobno = htonl(pj->jobn);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "jobadd", "OK");
}

static	void  api_jobcont(const int sock, const jobno_t jobno, const USHORT nbytes)
{
	unsigned		cnt;
	unsigned	char	*bp;
	struct	pend_job	*pj;
	char	inbuffer[XTA_BUFFSIZE];	/* XTA_BUFFSIZE always >= nbytes */

	pullin(sock, inbuffer, nbytes);
	if  (!(pj = find_j_by_jno(jobno)))
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
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "datain", "OK");
}

static	void  api_jobfinish(const int sock, const jobno_t jobno)
{
	int			ret;
	struct	pend_job	*pj;
	struct	api_msg		outmsg;

	outmsg.code = API_DATAEND;
	outmsg.un.jobdata.jobno = htonl(jobno);

	if  (!(pj = find_j_by_jno(jobno)))
		outmsg.retcode = htons(XT_UNKNOWN_JOB);
	else  if  ((ret = scan_job(pj)) != XTNQ_OK  &&  ret != XTNR_WARN_LIMIT)  {
		abort_job(pj);
		outmsg.retcode = htons((SHORT)XT_CONVERT_XTNR(ret));
	}
	else  {
		outmsg.retcode = htons(ret);
		sp_req.spr_un.j.spr_act = SJ_ENQ;
		sp_req.spr_un.j.spr_pid = getpid();
		pj->jobout.spq_job = jobno;
		pj->jobout.spq_time = time((time_t *) 0);
		pj->jobout.spq_orighost = pj->clientfrom == myhostid || pj->clientfrom == localhostid? 0: pj->clientfrom;
		while  (wjmsg(&sp_req, &pj->jobout) != 0  &&  errno == EAGAIN)
			sleep(SLEEPTIME);
		fclose(pj->out_f);
		pj->out_f = (FILE *) 0;
		if  (pj->delim)  {
			free(pj->delim);
			pj->delim = (char *) 0;
		}
	}
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "dataend", "OK");
}

static	void  api_jobabort(const int sock, const jobno_t jobno)
{
	struct	pend_job	*pj;
	struct	api_msg		outmsg;

	outmsg.code = API_DATAABORT;
	outmsg.retcode = 0;
	outmsg.un.jobdata.jobno = htonl(jobno);

	if  (!(pj = find_j_by_jno(jobno)))
		outmsg.retcode = htons(XT_UNKNOWN_JOB);
	else
		abort_job(pj);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "jobabort", "OK");
}

static void api_jobdata(const int sock, const struct spdet *priv, const classcode_t classcode, const slotno_t slotno, const ULONG seq, const ULONG flags, const int op)
{
	const  struct  spq  *jp;
	struct	api_msg	outmsg;

	outmsg.code = (unsigned char) op;
	outmsg.retcode = 0;

	rerjobfile();
	outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
	if  (!(flags & XT_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
		outmsg.retcode = htons(XT_SEQUENCE);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "seq");
		return;
	}

	if  (slotno >= Job_seg.dptr->js_maxjobs)  {
		outmsg.retcode = htons(XT_INVALIDSLOT);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "invslot");
		return;
	}
	jp = &Job_seg.jlist[slotno].j;
	if  (!check_valid_job(classcode, flags, jp, "jobdata"))  {
		outmsg.retcode = htons(XT_UNKNOWN_JOB);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "unkjob");
		return;
	}
	if  (!(priv->spu_flgs & PV_OTHERJ)  &&  jp->spq_uid != Realuid)  {
		outmsg.retcode = htons(XT_NOPERM);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "noperm");
		return;
	}

	if  (op == API_JOBPBRK)  {
		int		ret;
#ifndef	WORDS_BIGENDIAN
		int		cnt;
#endif
		char		*delim;
		unsigned	pagenum = 0, deliml;
		LONG		*pageoffs = (LONG *) 0;
		struct	pages	pfp;
		if  ((ret = rdpgfile(jp, &pfp, &delim, &pagenum, &pageoffs)) == 0)  {
			outmsg.retcode = htons(XT_BAD_PF);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "jobdata", "nopf");
			return;
		}
		if  (ret < 0)  {
			outmsg.retcode = htons(XT_NOMEM_PF);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "jobdata", "nomempf");
			return;
		}
		deliml = pfp.deliml;
#ifndef	WORDS_BIGENDIAN
		pfp.delimnum = htonl(pfp.delimnum);
		pfp.deliml = htonl(pfp.deliml);
		pfp.lastpage = htonl(pfp.lastpage);
		for  (cnt = 0;  cnt <= jp->spq_npages;  cnt++)
			pageoffs[cnt] = htonl(pageoffs[cnt]);
#endif

		/* Say ok, push out page structure, then delimiter,
		   then vector of offsets. Each is preceded by a message
		   saying how much to expect.  */

		pushout(sock, (char *) &outmsg, sizeof(outmsg));

		outmsg.code = API_DATAOUT;
		outmsg.un.jobdata.jobno = htonl(jp->spq_job);
		outmsg.un.jobdata.nbytes = htons(sizeof(pfp));
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		pushout(sock, (char *) &pfp, sizeof(pfp));

		outmsg.un.jobdata.nbytes = htons((USHORT) deliml);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		pushout(sock, delim, deliml);
		free(delim);

		deliml = (jp->spq_npages + 1) * sizeof(LONG);
		outmsg.un.jobdata.nbytes = htons((USHORT) deliml);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		pushout(sock, (char *) pageoffs, deliml);
		free((char *) pageoffs);
	}
	else  {
		int	inbp, ch;
		FILE	*jfile;
		char	buffer[XTA_BUFFSIZE];
		jfile = jp->spq_netid?
			net_feed(FEED_NPSP, jp->spq_netid, jp->spq_rslot, jp->spq_job):
			fopen(mkspid(SPNAM, jp->spq_job), "r");
		if  (!jfile)  {
			outmsg.retcode = htons(XT_UNKNOWN_JOB);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			return;
		}

		/* Say ok */

		pushout(sock, (char *) &outmsg, sizeof(outmsg));

		/* Read the file and splat it out.  */

		outmsg.code = API_DATAOUT;
		outmsg.un.jobdata.jobno = htonl(jp->spq_job);
		inbp = 0;
		while  ((ch = getc(jfile)) != EOF)  {
			buffer[inbp++] = (char) ch;
			if  (inbp >= sizeof(buffer))  {
				outmsg.un.jobdata.nbytes = htons(inbp);
				pushout(sock, (char *) &outmsg, sizeof(outmsg));
				pushout(sock, buffer, inbp);
				inbp = 0;
			}
		}
		fclose(jfile);
		if  (inbp > 0)  {
			outmsg.un.jobdata.nbytes = htons(inbp);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			pushout(sock, buffer, inbp);
			inbp = 0;
		}
	}

	/* Mark end of data */

	outmsg.code = API_DATAEND;
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "jobdata", "OK");
}

void  process_api()
{
	int		sock, inbytes, cnt, ret;
	classcode_t	classcode;
	PIDTYPE		pid;
	netid_t		whofrom;
	int_ugid_t	realuid;
	struct	hhash	*frp;
	struct	spdet	*mpriv;
	struct	spdet	hispriv;
	struct	api_msg	inmsg;
	struct	api_msg	outmsg;
	const	char	*tcode = "";

	if  ((sock = tcp_serv_accept(apirsock, &whofrom)) < 0)
		return;

	if  ((pid = fork()) < 0)  {
		print_error($E{Internal cannot fork});
		return;
	}

#ifndef	BUGGY_SIGCLD
	if  (pid != 0)  {
		close(sock);
		return;
	}
#else
	/* Make the process the grandchild so we don't have to worry
	   about waiting for it later.  */

	if  (pid != 0)  {
#ifdef	HAVE_WAITPID
		while  (waitpid(pid, (int *) 0, 0) < 0  &&  errno == EINTR)
			;
#else
		PIDTYPE	wpid;
		while  ((wpid = wait((int *) 0)) != pid  &&  (wpid >= 0 || errno == EINTR))
			;
#endif
		close(sock);
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

	while  ((inbytes = read(sock, (char *) &inmsg, sizeof(inmsg))) != sizeof(inmsg))  {
		if  (inbytes >= 0  ||  errno != EINTR)
			abort_exit(0);
		while  (hadrfresh)  {
			hadrfresh = 0;
			proc_refresh(whofrom);
		}
	}

	frp = find_remote(whofrom);

	/* If this is from a DOS user reject it if he hasn't logged
	   in.  Support "roaming" client users, by a login protocol.
	   This is only available from clients.  */

	if  (inmsg.code == API_LOGIN)  {
		if  (frp)  {
			if  (!(frp->rem.ht_flags & HT_DOS))  {
				if  (tracing & TRACE_APICONN)
					trace_op(ROOTID, "api-badlogin");
				err_result(sock, XT_UNKNOWN_COMMAND, 0);
				abort_exit(0);
			}
			if  (frp->rem.ht_flags & HT_ROAMUSER)  {

				/* We are a DHCP client (roaming user) on a known host.
				   See if the user name has changed and if so do we recognise it. */

				if  (strcmp(frp->dosname, inmsg.un.signon.username) != 0)  { /* Name change */
					struct	cluhash	 *cp;
					if  (!(cp = update_roam_name(frp, inmsg.un.signon.username)))  {
						if  (tracing & TRACE_APICONN)
							trace_op_res(ROOTID, "api-loginr-unkuser", inmsg.un.signon.username);
						err_result(sock, XT_UNKNOWN_USER, 0);
						abort_exit(0);
					}

					/* Set UAL_NOK if we need a password or if there is a default machine name
					   which is not the same one as we are talking about. The code below will
					   check the password. We compare the netid_ts as that is easier than
					   worrying about aliases. This silently allows IP addresses too. */

					frp->flags = (cp->rem.ht_flags & HT_PWCHECK  ||
						      (cp->machname  &&  look_hostname(cp->machname) != whofrom)) ? UAL_NOK: UAL_OK;
				}
			}
			else  {
				/* Non roam case we've seen the machine but we need to check the user name makes sense */

				if  (ncstrcmp(frp->actname, inmsg.un.signon.username) != 0)  {
					if  (!update_nonroam_name(frp, inmsg.un.signon.username))  {
						if  (tracing & TRACE_APICONN)
							trace_op_res(ROOTID, "api-login-unkuser", inmsg.un.signon.username);
						err_result(sock, XT_UNKNOWN_USER, 0);
						abort_exit(0);
					}
					frp->flags = (frp->rem.ht_flags & HT_PWCHECK  ||
						      (frp->dosname[0]  &&  ncstrcmp(frp->dosname, frp->actname) != 0))? UAL_NOK: UAL_OK;
				}
			}
		}
		else  {		/* We don't know the machine (or we think we don't) */
			struct	cluhash  *cp;

			if  (!(cp = new_roam_name(whofrom, &frp, inmsg.un.signon.username)))  {
				if  (tracing & TRACE_APICONN)
					trace_op_res(ROOTID, "api-login-um-unkuser", inmsg.un.signon.username);
				err_result(sock, XT_UNKNOWN_USER, 0);
				abort_exit(0);
			}
			frp->flags = (cp->rem.ht_flags & HT_PWCHECK  ||
				      (cp->machname  &&  look_hostname(cp->machname) != whofrom)) ? UAL_NOK: UAL_OK;
		}

		/* Now for any password-checking. */

		outmsg.code = inmsg.code;
		strncpy(outmsg.un.signon.username, frp->actname, UIDSIZE);

		if  (frp->flags != UAL_OK)  {
			char	pwbuf[API_PASSWDSIZE+1];
			outmsg.retcode = htons(XT_NO_PASSWD);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			pullin(sock, pwbuf, sizeof(pwbuf));
			frp->flags = checkpw(frp->actname, pwbuf)? UAL_OK: UAL_INVP;
			if  (frp->flags != UAL_OK)  {
				if  (tracing & TRACE_APICONN)
					trace_op_res(ROOTID, "Loginfail", frp->actname);
				err_result(sock, XT_PASSWD_INVALID, 0);
				abort_exit(0);
			}
		}
		outmsg.retcode = XT_OK;
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		/* No tell_friends (or equiv) as we don't need it in spshed */
	}
	else  {			/* Not starting with login */
		if  (inmsg.code != API_SIGNON)  {
			if  (tracing & TRACE_APICONN)
				trace_op(ROOTID, "Loginseq");
			err_result(sock, XT_SEQUENCE, 0);
			abort_exit(0);
		}
		if  (!frp)  {
			if  (tracing & TRACE_APICONN)
				trace_op(ROOTID, "Signonunku");
			err_result(sock, XT_UNKNOWN_USER, 0);
			abort_exit(0);
		}

		if  (frp->rem.ht_flags & HT_DOS)  {

			/* Possibly let a roaming user change his name.  */

			if  (frp->rem.ht_flags & HT_ROAMUSER)  {
				if  (ncstrcmp(inmsg.un.signon.username, frp->actname) != 0)  {
					struct	cluhash		*cp;
					if  (!(cp = update_roam_name(frp, inmsg.un.signon.username)))  {
						if  (tracing & TRACE_APICONN)
							trace_op_res(ROOTID, "api-signon-unkuser", inmsg.un.signon.username);
						err_result(sock, XT_UNKNOWN_USER, 0);
						abort_exit(0);
					}
					/* We didn't forget to tell_friends here - not a login, just a different user */
					frp->flags = cp->rem.ht_flags & HT_PWCHECK? UAL_NOK: UAL_OK;
				}
			}
			else  {
				if  (ncstrcmp(frp->actname, inmsg.un.signon.username) != 0)  {
					if  (!update_nonroam_name(frp, inmsg.un.signon.username))  {
						if  (tracing & TRACE_APICONN)
							trace_op_res(ROOTID, "api-signonnr-unkuser", inmsg.un.signon.username);
						err_result(sock, XT_UNKNOWN_USER, 0);
						abort_exit(0);
					}
					frp->flags = (frp->rem.ht_flags & HT_PWCHECK  ||
						      (frp->dosname[0]  &&
						       ncstrcmp(frp->dosname, frp->actname) != 0))? UAL_NOK : UAL_OK;
				}
			}

			if  (frp->flags != UAL_OK)  {
				if  (tracing & TRACE_APICONN)
					trace_op_res(ROOTID, "Signonfail", frp->actname);
				err_result(sock, frp->flags == UAL_NOK? XT_NO_PASSWD: frp->flags == UAL_INVU? XT_UNKNOWN_USER: XT_PASSWD_INVALID, 0);
				abort_exit(0);
			}

			if  ((realuid = lookup_uname(frp->actname)) == UNKNOWN_UID)  {
				if  (tracing & TRACE_APICONN)
					trace_op_res(ROOTID, "Unkuser", frp->actname);
				err_result(sock, XT_UNKNOWN_USER, 0);
				abort_exit(0);
			}
		}
		else		/* Non-DOS case */
			frp->actname = stracpy(inmsg.un.signon.username);
	}

	if  ((realuid = lookup_uname(frp->actname)) == UNKNOWN_UID)  {
		if  (tracing & TRACE_APICONN)
			trace_op_res(ROOTID, "unkuser", frp->actname);
		err_result(sock, XT_UNKNOWN_USER, 0);
		abort_exit(0);
	}

	/* Need to make a copy as subsequent calls overwrite static
	   space in getspuentry */

	mpriv = getspuentry(realuid);
	Realuid = realuid;
	if  (tracing & TRACE_APICONN)
		trace_op_res(Realuid, "Login-OK", frp->actname);

	hispriv = *mpriv;
	classcode = ntohl(inmsg.un.signon.classcode);
	if  (!(hispriv.spu_flgs & PV_COVER))
		classcode &= hispriv.spu_class;
	if  (classcode == 0)
		classcode = hispriv.spu_class;

	/* Ok we made it */

	outmsg.code = inmsg.code;
	outmsg.retcode = 0;
	outmsg.un.r_signon.classcode = htonl(classcode);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));

	/* So do main loop waiting for something to happen */

 restart:
	for  (;;)  {
		while  (hadrfresh)  {
			hadrfresh = 0;
			proc_refresh(whofrom);
		}
		if  ((inbytes = read(sock, (char *) &inmsg, sizeof(inmsg))) != sizeof(inmsg))  {
			if  (inbytes >= 0  ||  errno != EINTR)
				abort_exit(0);
			goto  restart;
		}
		switch  (outmsg.code = inmsg.code)  {
		default:
			ret = XT_UNKNOWN_COMMAND;
			break;

		case  API_SIGNOFF:
			if  (tracing & TRACE_APICONN)
				trace_op(Realuid, "logoff");
			abort_exit(0);

		case  API_JOBLIST:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "joblist");
			reply_joblist(sock, classcode, ntohl(inmsg.un.lister.flags));
			continue;

		case  API_PTRLIST:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "ptrlist");
			reply_ptrlist(sock, classcode, ntohl(inmsg.un.lister.flags));
			continue;

		case  API_JOBREAD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobread");
			reply_jobread(sock,
				      classcode,
				      ntohl(inmsg.un.reader.slotno),
				      ntohl(inmsg.un.reader.seq),
				      ntohl(inmsg.un.reader.flags));
			continue;

		case  API_PTRREAD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "ptrread");
			reply_ptrread(sock,
				      classcode,
				      ntohl(inmsg.un.reader.slotno),
				      ntohl(inmsg.un.reader.seq),
				      ntohl(inmsg.un.reader.flags));
			continue;

		case  API_FINDJOBSLOT:
		case  API_FINDJOB:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "findjob");
			reply_jobfind(sock,
				      classcode,
				      inmsg.code,
				      ntohl(inmsg.un.jobfind.jobno),
				      inmsg.un.jobfind.netid == myhostid? 0: inmsg.un.jobfind.netid,
				      ntohl(inmsg.un.jobfind.flags));
			continue;

		case  API_FINDPTRSLOT:
		case  API_FINDPTR:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "findptr");
			reply_ptrfind(sock,
				      classcode,
				      inmsg.code,
				      inmsg.un.jobfind.netid == myhostid? 0: inmsg.un.ptrfind.netid,
				      ntohl(inmsg.un.ptrfind.flags));
			continue;

		case  API_JOBDEL:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobdel");
			ret = reply_jobdel(&hispriv,
					   classcode,
					   ntohl(inmsg.un.reader.slotno),
					   ntohl(inmsg.un.reader.seq),
					   ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
			break;

		case  API_PTRDEL:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "ptrdel");
			ret = reply_ptrop(&hispriv,
					  classcode,
					  ntohl(inmsg.un.reader.slotno),
					  ntohl(inmsg.un.reader.seq),
					  ntohl(inmsg.un.reader.flags),
					  (ULONG) SO_DELP);
			outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
			break;

		case  API_PTROP:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "ptrop");
			ret = reply_ptrop(&hispriv,
					  classcode,
					  ntohl(inmsg.un.pop.slotno),
					  ntohl(inmsg.un.pop.seq),
					  ntohl(inmsg.un.pop.flags),
					  ntohl(inmsg.un.pop.op));
			outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
			break;

		case  API_JOBADD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobadd");
			api_jobstart(sock, frp, ntohl(inmsg.un.jobdata.jobno));
			continue;
		case  API_DATAIN:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "datain");
			api_jobcont(sock, ntohl(inmsg.un.jobdata.jobno), ntohs(inmsg.un.jobdata.nbytes));
			continue;
		case  API_DATAEND:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "dataend");
			api_jobfinish(sock, ntohl(inmsg.un.jobdata.jobno));
			continue;
		case  API_DATAABORT: /* We'll be lucky if we get this */
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "dataabort");
			api_jobabort(sock, ntohl(inmsg.un.jobdata.jobno));
			continue;

		case  API_PTRADD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "ptradd");
			ret = reply_ptradd(sock, &hispriv);
			outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
			break;

		case  API_JOBUPD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobupd");
			ret = reply_jobupd(sock,
					   &hispriv,
					   classcode,
					   ntohl(inmsg.un.reader.slotno),
					   ntohl(inmsg.un.reader.seq),
					   ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
			break;

		case  API_PTRUPD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "ptrupd");
			ret = reply_ptrupd(sock,
					   &hispriv,
					   classcode,
					   ntohl(inmsg.un.reader.slotno),
					   ntohl(inmsg.un.reader.seq),
					   ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Ptr_seg.dptr->ps_serial);
			break;

		case  API_JOBDATA:
		case  API_JOBPBRK:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobdata");
			api_jobdata(sock,
				    &hispriv,
				    classcode,
				    ntohl(inmsg.un.reader.slotno),
				    ntohl(inmsg.un.reader.seq),
				    ntohl(inmsg.un.reader.flags),
				    inmsg.code);
			continue;

		case  API_REQPROD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "onmon");
			setup_prod();
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "onmon", "OK");
			continue;

		case  API_UNREQPROD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "offmon");
			unsetup_prod();
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "offmon", "OK");
			continue;

		case  API_GETSPU:
		{
			int_ugid_t	ouid;
			struct	spdet  outspdet;
			tcode = "getspd";
			if  ((ouid = lookup_uname(inmsg.un.us.username)) == UNKNOWN_UID)  {
				ret = XT_UNKNOWN_USER;
				break;
			}
			if  (ouid != realuid  &&  !(hispriv.spu_flgs & PV_ADMIN))  {
				ret = XT_NOPERM;
				break;
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
			outmsg.retcode = 0;
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			pushout(sock, (char *) &outspdet, sizeof(outspdet));
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, tcode, "OK");
			continue;
		}
		case  API_GETSPD:
		{
			struct	sphdr  outsphdr;
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "getspd");
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
			outmsg.retcode = 0;
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			pushout(sock, (char *) &outsphdr, sizeof(outsphdr));
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "getbtd", "OK");
			continue;
		}

		case  API_PUTSPU:
		{
			int_ugid_t	ouid;
			struct	spdet	inspdet, rspdet;

			tcode = "putspu";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);

			pullin(sock, (char *) &inspdet, sizeof(inspdet));

			if  ((ouid = lookup_uname(inmsg.un.us.username)) == UNKNOWN_UID)  {
				ret = XT_UNKNOWN_USER;
				break;
			}
			if  (!(hispriv.spu_flgs & PV_CDEFLT) ||
			     (ouid != realuid  &&  !(hispriv.spu_flgs & PV_ADMIN)))  {
				ret = XT_NOPERM;
				break;
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

			if  (ouid == realuid)  {
				mpriv = getspuentry(ouid);
				hispriv = *mpriv;
				if  (!(hispriv.spu_flgs & PV_ADMIN))  {
					/* Disallow everything except prio and form
					   We already checked PV_CDEFLT */
					if  (rspdet.spu_minp != hispriv.spu_minp ||
					     rspdet.spu_maxp != hispriv.spu_maxp  ||
					     rspdet.spu_flgs != hispriv.spu_flgs  ||
					     rspdet.spu_class != hispriv.spu_class  ||
					     rspdet.spu_cps != hispriv.spu_cps)  {
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
			if  (ouid == realuid)
				hispriv = *mpriv;
			ret = XT_OK;
			break;
		}

		case  API_PUTSPD:
		{
			struct	sphdr	insphdr, rsphdr;

			tcode = "putspd";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			pullin(sock, (char *) &insphdr, sizeof(insphdr));
			if  (!(hispriv.spu_flgs & PV_ADMIN))  {
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

			mpriv = getspuentry(realuid);
			hispriv = *mpriv;
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
		outmsg.retcode = htons(ret);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, tcode, ret == XT_OK? "OK": "Failed");
	}
}

#else  /* NETWORK_VERSION */

/* Dummy routine as some C compilers choke over null files */

void	foo1()
{
	return;
}
#endif /* !NETWORK_VERSION */
