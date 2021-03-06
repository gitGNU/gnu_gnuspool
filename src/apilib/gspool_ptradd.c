/* gspool_ptradd.c -- API crete printer entry

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
#include "incl_net.h"
#include "incl_unix.h"

extern int	gspool_read(const int, char *, unsigned);
extern int	gspool_write(const int, char *, unsigned);
extern int	gspool_rmsg(const struct api_fd *, struct api_msg *);
extern int	gspool_wmsg(const struct api_fd *, struct api_msg *);
extern struct	api_fd *gspool_look_fd(const int);
extern void	gspool_ptrswap(struct apispptr *, const struct apispptr *);

int	gspool_ptradd(const int fd, const struct apispptr *newptr)
{
	int	ret;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg		msg;
	struct	apispptr	res;

	if  (!fdp)
		return  GSPOOL_INVALID_FD;
	msg.code = API_PTRADD;
	gspool_ptrswap(&res, newptr);
	if  ((ret = gspool_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = gspool_write(fdp->sockfd, (char *) &res, sizeof(res))))
		return  ret;
	if  ((ret = gspool_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_reader.seq != 0)
		fdp->pserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  GSPOOL_OK;
}
