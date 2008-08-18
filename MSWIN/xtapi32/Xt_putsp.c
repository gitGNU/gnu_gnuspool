/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_putspu.c: created by John Collins on Tue Mar 15 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_putsp.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_putsp.c,v $
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
#include <io.h>
#include <winsock.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int	xt_write(const SOCKET, char *, unsigned),
		xt_rmsg(const struct api_fd *, struct api_msg *),
		xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

int	xt_putspu(const int fd, const char *username, const struct apispdet *res)
{
	int	ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;
	struct	apispdet	buf;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_PUTSPU;
	strncpy(msg.un.us.username, username? username: fdp->username, UIDSIZE);
	msg.un.us.username[UIDSIZE] = '\0';

	/* And now do all the byte-swapping */

	buf.spu_isvalid = res->spu_isvalid;
	buf.spu_user = htonl((ULONG) res->spu_user);
	buf.spu_minp = res->spu_minp;
	buf.spu_maxp = res->spu_maxp;
	buf.spu_defp = res->spu_defp;
	strncpy(buf.spu_form, res->spu_form, MAXFORM);
	strncpy(buf.spu_formallow, res->spu_formallow, ALLOWFORMSIZE);
	strncpy(buf.spu_ptr, res->spu_ptr, PTRNAMESIZE);
	strncpy(buf.spu_ptrallow, res->spu_ptrallow, JPTRNAMESIZE);
	buf.spu_form[MAXFORM] = '\0';
	buf.spu_formallow[MAXFORM] = '\0';
	buf.spu_ptr[PTRNAMESIZE] = '\0';
	buf.spu_ptrallow[JPTRNAMESIZE] = '\0';
	buf.spu_flgs = htonl(res->spu_flgs);
	buf.spu_class = htonl(res->spu_class);
	buf.spu_cps = res->spu_cps;
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_write(fdp->sockfd, (char *) &buf, sizeof(buf))))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  XT_OK;
}
