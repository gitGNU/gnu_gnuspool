/* Xt_gtspd.c -- get default user permissions

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
