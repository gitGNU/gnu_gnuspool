/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_jobread.c: created by John Collins on Fri Mar 11 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_jobre.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_jobre.c,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 23.4  2000/08/28 20:58:39  jmc
 * Add new facilities to API for job and printer find.
 * Fix bug in jobdata and jobpbrk.
 *
 * Revision 23.3  1999/04/06 10:20:57  jmc
 * Mostly changes to job shared memory - split and make read-only.
 *
 * Revision 23.2  1998/02/24 10:27:13  jmc
 * Revisions for new-style configure.
 *
 * Revision 23.1  1996/02/13 09:02:51  jmc
 * Brand New Release 23.
 *
 * Revision 22.1  1995/01/13  17:06:57  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:22:26  jmc
 * Brand new Release 21
 *
 * Revision 20.1  1994/03/24  17:25:52  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 */

#include <sys/types.h>
#include <io.h>
#include <string.h>
#include <winsock.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int	xt_read(const SOCKET, char *, unsigned),
		xt_rmsg(const struct api_fd *, struct api_msg *),
		xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

static int	job_readrest(struct api_fd *fdp, struct apispq * result)
{
	int		ret;
	struct	apispq	res;

	if  ((ret = xt_read(fdp->sockfd, (char *) &res, sizeof(res))))
		return  ret;

	/* And now do all the byte-swapping */

	result->apispq_job = ntohl((ULONG) res.apispq_job);
	result->apispq_netid = res.apispq_netid;
	result->apispq_orighost = res.apispq_orighost;
	result->apispq_rslot = ntohl(res.apispq_rslot);
	result->apispq_time = ntohl((ULONG) res.apispq_time);
	result->apispq_proptime = 0L;
	result->apispq_starttime = ntohl((ULONG) res.apispq_starttime);
	result->apispq_hold = ntohl((ULONG) res.apispq_hold);
	result->apispq_nptimeout = ntohs(res.apispq_nptimeout);
	result->apispq_ptimeout = ntohs(res.apispq_ptimeout);
	result->apispq_size = ntohl((ULONG) res.apispq_size);
	result->apispq_posn = ntohl((ULONG) res.apispq_posn);
	result->apispq_pagec = ntohl((ULONG) res.apispq_pagec);
	result->apispq_npages = ntohl((ULONG) res.apispq_npages);

	result->apispq_cps = res.apispq_cps;
	result->apispq_pri = res.apispq_pri;
	result->apispq_wpri = ntohs((USHORT) res.apispq_wpri);

	result->apispq_jflags = ntohs(res.apispq_jflags);
	result->apispq_sflags = res.apispq_sflags;
	result->apispq_dflags = res.apispq_dflags;

	result->apispq_extrn = ntohs(res.apispq_extrn);
	result->apispq_pglim = 0;

	result->apispq_class = ntohl(res.apispq_class);

	result->apispq_pslot = ntohl(res.apispq_pslot);

	result->apispq_start = ntohl((ULONG) res.apispq_start);
	result->apispq_end = ntohl((ULONG) res.apispq_end);
	result->apispq_haltat = ntohl((ULONG) res.apispq_haltat);

	result->apispq_uid = ntohl(res.apispq_uid);

	strncpy(result->apispq_uname, res.apispq_uname, UIDSIZE+1);
	strncpy(result->apispq_puname, res.apispq_puname, UIDSIZE+1);
	strncpy(result->apispq_file, res.apispq_file, MAXTITLE+1);
	strncpy(result->apispq_form, res.apispq_form, MAXFORM+1);
	strncpy(result->apispq_ptr, res.apispq_ptr, JPTRNAMESIZE+1);
	strncpy(result->apispq_flags, res.apispq_flags, MAXFLAGS+1);
	return  XT_OK;
}

int	xt_jobread(const int fd, const unsigned	flags, const slotno_t slotno, struct apispq *result)
{
	int	ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  XT_INVALID_FD;                                                               
	msg.code = API_JOBREAD;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->jserial);
	msg.un.reader.slotno = htonl(slotno);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_reader.seq != 0)
		fdp->jserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);

	/* The message is followed by the job details. */

	return  job_readrest(fdp, result);
}

int	xt_jobfindslot(const int fd, const unsigned flags, const jobno_t jn, const netid_t nid, slotno_t *slot)
{
	int		ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_FINDJOBSLOT;
	msg.un.jobfind.flags = htonl(flags);
	msg.un.jobfind.netid = nid;
	msg.un.jobfind.jobno = htonl(jn);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_find.seq != 0)
		fdp->jserial = ntohl(msg.un.r_find.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	if  (slot)
		*slot = ntohl(msg.un.r_find.slotno);
	return  XT_OK;
}

int	xt_jobfind(const int fd, const unsigned flags, const jobno_t jn, const netid_t nid, slotno_t *slot, struct apispq *jp)
{
	int		ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_FINDJOB;
	msg.un.jobfind.flags = htonl(flags);
	msg.un.jobfind.netid = nid;
	msg.un.jobfind.jobno = htonl(jn);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_find.seq != 0)
		fdp->jserial = ntohl(msg.un.r_find.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	if  (slot)
		*slot = ntohl(msg.un.r_find.slotno);
	return  job_readrest(fdp, jp);
}
