/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_getspd.c: created by John Collins on Tue Mar 15 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_gtspd.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_gtspd.c,v $
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

int	xt_getspd(const int fd, struct apisphdr *res)
{
	int		ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;
	struct	apisphdr	buf;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_GETSPD;
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

	res->sph_minp = buf.sph_minp;
	res->sph_maxp = buf.sph_maxp;
	res->sph_defp = buf.sph_defp;
	strncpy(res->sph_form, buf.sph_form, MAXFORM);
	strncpy(res->sph_formallow, buf.sph_formallow, ALLOWFORMSIZE);
	strncpy(res->sph_ptr, buf.sph_ptr, PTRNAMESIZE);
	strncpy(res->sph_ptrallow, buf.sph_ptrallow, JPTRNAMESIZE);
	res->sph_form[MAXFORM] = '\0';
	res->sph_formallow[ALLOWFORMSIZE] = '\0';
	res->sph_ptr[PTRNAMESIZE] = '\0';
	res->sph_ptrallow[JPTRNAMESIZE] = '\0';
	res->sph_flgs = ntohl(buf.sph_flgs);
	res->sph_class = ntohl(buf.sph_class);
	res->sph_cps = buf.sph_cps;
	return  XT_OK;
}
