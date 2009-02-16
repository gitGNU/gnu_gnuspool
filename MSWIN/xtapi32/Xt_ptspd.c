/* Xt_ptspd.c -- write user default permissions

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
#include <io.h>
#include <winsock.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int	xt_write(const SOCKET, char *, unsigned),
			xt_rmsg(const struct api_fd *, struct api_msg *),
			xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

int	xt_putspd(const int fd, const struct apisphdr *res)
{
	int		ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;
	struct	apisphdr	buf;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_PUTSPD;

	/* And now do all the byte-swapping */

	buf.sph_minp = res->sph_minp;
	buf.sph_maxp = res->sph_maxp;
	buf.sph_defp = res->sph_defp;
	strncpy(buf.sph_form, res->sph_form, MAXFORM);
	strncpy(buf.sph_formallow, res->sph_formallow, ALLOWFORMSIZE);
	strncpy(buf.sph_ptr, res->sph_ptr, PTRNAMESIZE);
	strncpy(buf.sph_ptrallow, res->sph_ptrallow, JPTRNAMESIZE);
	buf.sph_form[MAXFORM] = '\0';
	buf.sph_formallow[ALLOWFORMSIZE] = '\0';
	buf.sph_ptr[PTRNAMESIZE] = '\0';
	buf.sph_ptrallow[JPTRNAMESIZE] = '\0';
	buf.sph_flgs = htonl(res->sph_flgs);
	buf.sph_class = htonl(res->sph_class);
	buf.sph_cps = res->sph_cps;
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
