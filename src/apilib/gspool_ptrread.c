/* gspool_ptrread.c -- API fetch attributes of printer

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
#include <sys/types.h>
#include "gspool.h"
#include "xtapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"

extern int	gspool_read(const int, char *, unsigned);
extern int	gspool_write(const int, char *, unsigned);
extern int	gspool_rmsg(const struct api_fd *, struct api_msg *);
extern int	gspool_wmsg(const struct api_fd *, struct api_msg *);
extern struct	api_fd *gspool_look_fd(const int);

static int	ptr_readrest(struct api_fd *fdp, struct apispptr *result)
{
	int		ret;
	struct	apispptr	res;

	/* The message is followed by the ptr details.  */

	if  ((ret = gspool_read(fdp->sockfd, (char *) &res, sizeof(res))))
		return  ret;

	/* And now do all the byte-swapping */

	result->apispp_netid = res.apispp_netid;
	result->apispp_rslot = ntohl((ULONG) res.apispp_rslot);

	result->apispp_pid = ntohl((ULONG) res.apispp_pid);
	result->apispp_job = ntohl((ULONG) res.apispp_job);
	result->apispp_rjhostid = res.apispp_rjhostid;
	result->apispp_rjslot = ntohl((ULONG) res.apispp_rjslot);
	result->apispp_jslot = ntohl((ULONG) res.apispp_jslot);

	result->apispp_state = res.apispp_state;
	result->apispp_sflags = res.apispp_sflags;
	result->apispp_dflags = res.apispp_dflags;
	result->apispp_netflags = res.apispp_netflags;

	result->apispp_class = ntohl(res.apispp_class);
	result->apispp_minsize = ntohl((ULONG) res.apispp_minsize);
	result->apispp_maxsize = ntohl((ULONG) res.apispp_maxsize);

	result->apispp_extrn = ntohs(res.apispp_extrn);
	result->apispp_resvd = 0;

	strncpy(result->apispp_dev, res.apispp_dev, LINESIZE+1);
	strncpy(result->apispp_form, res.apispp_form, MAXFORM+1);
	strncpy(result->apispp_ptr, res.apispp_ptr, PTRNAMESIZE+1);
	strncpy(result->apispp_feedback, res.apispp_feedback, PFEEDBACK+1);
	strncpy(result->apispp_comment, res.apispp_comment, COMMENTSIZE+1);

	return  GSPOOL_OK;
}

int  gspool_ptrread(const int fd, const unsigned flags, const slotno_t slotno, struct apispptr *result)
{
	int		ret;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg		msg;

	if  (!fdp)
		return  GSPOOL_INVALID_FD;
	msg.code = API_PTRREAD;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->pserial);
	msg.un.reader.slotno = htonl(slotno);
	if  ((ret = gspool_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = gspool_rmsg(fdp, &msg)))
		return  ret;;
	if  (msg.un.r_reader.seq != 0)
		fdp->pserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return	ptr_readrest(fdp, result);
}

int  gspool_ptrfindslot(const int fd, const unsigned flags, const char *name, const netid_t nid, slotno_t *slotno)
{
	int		ret;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg		msg;
	char	ptrname[PTRNAMESIZE+1];

	if  (!fdp)
		return  GSPOOL_INVALID_FD;

	strncpy(ptrname, name, PTRNAMESIZE);
	ptrname[PTRNAMESIZE] = '\0';

	msg.code = API_FINDPTRSLOT;
	msg.un.ptrfind.flags = htonl(flags);
	msg.un.ptrfind.netid = nid;
	if  ((ret = gspool_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = gspool_write(fdp->sockfd, ptrname, sizeof(ptrname))))
		return  ret;
	if  ((ret = gspool_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_find.seq != 0)
		fdp->pserial = ntohl(msg.un.r_find.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	if  (slotno)
		*slotno = ntohl(msg.un.r_find.slotno);
	return  GSPOOL_OK;
}

int  gspool_ptrfind(const int fd, const unsigned flags, const char *name, const netid_t nid, slotno_t *slotno, struct apispptr *result)
{
	int		ret;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg		msg;
	char	ptrname[PTRNAMESIZE+1];

	if  (!fdp)
		return  GSPOOL_INVALID_FD;

	strncpy(ptrname, name, PTRNAMESIZE);
	ptrname[PTRNAMESIZE] = '\0';

	msg.code = API_FINDPTR;
	msg.un.ptrfind.flags = htonl(flags);
	msg.un.ptrfind.netid = nid;
	if  ((ret = gspool_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = gspool_write(fdp->sockfd, ptrname, sizeof(ptrname))))
		return  ret;
	if  ((ret = gspool_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_find.seq != 0)
		fdp->pserial = ntohl(msg.un.r_find.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	if  (slotno)
		*slotno = ntohl(msg.un.r_find.slotno);
	return  ptr_readrest(fdp, result);
}
