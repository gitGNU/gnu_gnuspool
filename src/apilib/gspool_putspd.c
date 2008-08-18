/* gspool_putspd.c -- API save default user permissions

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

extern int	gspool_write(const int, char *, unsigned);
extern int	gspool_rmsg(const struct api_fd *, struct api_msg *);
extern int	gspool_wmsg(const struct api_fd *, struct api_msg *);
extern struct	api_fd *gspool_look_fd(const int);

int	gspool_putspd(const int fd, const struct apisphdr *res)
{
	int		ret;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg		msg;
	struct	apisphdr	buf;

	if  (!fdp)
		return  GSPOOL_INVALID_FD;
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
	if  ((ret = gspool_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = gspool_write(fdp->sockfd, (char *) &buf, sizeof(buf))))
		return  ret;
	if  ((ret = gspool_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  GSPOOL_OK;
}
