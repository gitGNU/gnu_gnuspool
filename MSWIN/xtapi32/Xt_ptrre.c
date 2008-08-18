/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_ptrread.c: created by John Collins on Fri Mar 11 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_ptrre.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_ptrre.c,v $
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
			xt_write(const SOCKET, char *, unsigned),
			xt_rmsg(const struct api_fd *, struct api_msg *),
			xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

static int	ptr_readrest(struct api_fd *fdp, struct apispptr * result)
{
	int		ret;
	struct	apispptr	res;

	/* The message is followed by the ptr details.  */

	if  ((ret = xt_read(fdp->sockfd, (char *) &res, sizeof(res))))
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

	return  XT_OK;
}

int	xt_ptrread(const int fd, const unsigned	flags, const slotno_t slotno, struct apispptr *result)
{
	int		ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_PTRREAD;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->pserial);
	msg.un.reader.slotno = htonl(slotno);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;;
	if  (msg.un.r_reader.seq != 0)
		fdp->pserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return	ptr_readrest(fdp, result);
}

int	xt_ptrfindslot(const int fd, const unsigned flags, const char *name, const netid_t nid, slotno_t *slotno)
{
	int		ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;
	char	ptrname[PTRNAMESIZE+1];

	if  (!fdp)
		return  XT_INVALID_FD;

	strncpy(ptrname, name, PTRNAMESIZE);
	ptrname[PTRNAMESIZE] = '\0';

	msg.code = API_FINDPTRSLOT;
	msg.un.ptrfind.flags = htonl(flags);
	msg.un.ptrfind.netid = nid;
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_write(fdp->sockfd, ptrname, sizeof(ptrname))))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_find.seq != 0)
		fdp->pserial = ntohl(msg.un.r_find.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	if  (slotno)
		*slotno = ntohl(msg.un.r_find.slotno);
	return  XT_OK;
}

int	xt_ptrfind(const int fd, const unsigned	flags, const char *name, const netid_t nid, slotno_t *slotno, struct apispptr *result)
{
	int		ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;
	char	ptrname[PTRNAMESIZE+1];

	if  (!fdp)
		return  XT_INVALID_FD;

	strncpy(ptrname, name, PTRNAMESIZE);
	ptrname[PTRNAMESIZE] = '\0';

	msg.code = API_FINDPTR;
	msg.un.ptrfind.flags = htonl(flags);
	msg.un.ptrfind.netid = nid;
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_write(fdp->sockfd, ptrname, sizeof(ptrname))))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_find.seq != 0)
		fdp->pserial = ntohl(msg.un.r_find.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	if  (slotno)
		*slotno = ntohl(msg.un.r_find.slotno);
	return  ptr_readrest(fdp, result);
}
