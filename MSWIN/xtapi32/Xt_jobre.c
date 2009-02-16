/* Xt_jobre.c -- read job

   Copyright 2009 Free Software Foundation, Inc.

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
