/* gspool_getspu.c -- get permissions for user

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
extern int	gspool_rmsg(const struct api_fd *, struct api_msg *);
extern int	gspool_wmsg(const struct api_fd *, struct api_msg *);
extern struct	api_fd *	gspool_look_fd(const int);

int	gspool_getspu(const int fd, const char *username, struct apispdet *res)
{
	int	ret;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg		msg;
	struct	apispdet	buf;

	if  (!fdp)
		return  GSPOOL_INVALID_FD;
	msg.code = API_GETSPU;
	strncpy(msg.un.us.username, username? username: fdp->username, UIDSIZE);
	msg.un.us.username[UIDSIZE] = '\0';
	if  ((ret = gspool_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = gspool_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);

	/* The message is followed by the details.  */

	if  ((ret = gspool_read(fdp->sockfd, (char *) &buf, sizeof(buf))))
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
	return  GSPOOL_OK;
}
