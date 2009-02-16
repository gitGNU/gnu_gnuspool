/* Xt_getsp.c -- get user permissions

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
