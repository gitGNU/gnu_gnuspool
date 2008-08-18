/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_getspu.c: created by John Collins on Tue Mar 15 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_getsp.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_getsp.c,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
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
#include <string.h>
#include <winsock.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int	xt_read(const SOCKET, char *, unsigned),
		xt_rmsg(const struct api_fd *, struct api_msg *),
		xt_wmsg(const struct api_fd *, struct api_msg *);

extern struct	api_fd *xt_look_fd(const int);

int	xt_getspu(const int fd, const char *username, struct apispdet *res)
{
	int	ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;
	struct	apispdet	buf;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_GETSPU;
	strncpy(msg.un.us.username, username? username: fdp->username, UIDSIZE);
	msg.un.us.username[UIDSIZE] = '\0';
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);

	/* The message is followed by the details. */

	if  ((ret = xt_read(fdp->sockfd, (char *) &buf, sizeof(buf))))
		return  ret;

	/* And now do all the byte-swapping */

	res->spu_isvalid = buf.spu_isvalid;
	res->spu_user = ntohl((ULONG) buf.spu_user);
	res->spu_minp = buf.spu_minp;
	res->spu_maxp = buf.spu_maxp;
	res->spu_defp = buf.spu_defp;
	strncpy(res->spu_form, buf.spu_form, MAXFORM);
	strncpy(res->spu_formallow, buf.spu_formallow, ALLOWFORMSIZE);
	strncpy(res->spu_ptr, buf.spu_ptr, PTRNAMESIZE);
	strncpy(res->spu_ptrallow, buf.spu_ptrallow, JPTRNAMESIZE);
	res->spu_form[MAXFORM] = '\0';
	res->spu_formallow[ALLOWFORMSIZE] = '\0';
	res->spu_ptr[PTRNAMESIZE] = '\0';
	res->spu_ptrallow[JPTRNAMESIZE] = '\0';
	res->spu_flgs = ntohl(buf.spu_flgs);
	res->spu_class = ntohl(buf.spu_class);
	res->spu_cps = buf.spu_cps;
	return  XT_OK;
}
