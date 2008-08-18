/* gspool_getspd.c -- get user permission defaults

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
extern struct	api_fd *gspool_look_fd(const int);

int	gspool_getspd(const int fd, struct apisphdr *res)
{
	int		ret;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg		msg;
	struct	apisphdr	buf;

	if  (!fdp)
		return  GSPOOL_INVALID_FD;
	msg.code = API_GETSPD;
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
	return  GSPOOL_OK;
}
