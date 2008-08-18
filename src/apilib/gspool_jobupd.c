/* gspool_jobupd.c -- update job attributes

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

extern int	gspool_write(const int, char *, unsigned);
extern int	gspool_rmsg(const struct api_fd *, struct api_msg *);
extern int	gspool_wmsg(const struct api_fd *, struct api_msg *);
extern struct	api_fd *gspool_look_fd(const int);
extern void	gspool_jobswap(struct apispq *, const struct apispq *);

int	gspool_jobupd(const int fd, const unsigned flags, const slotno_t slotno, const struct apispq *newjob)
{
	int	ret;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg	msg;
	struct	apispq	res;

	if  (!fdp)
		return  GSPOOL_INVALID_FD;
	msg.code = API_JOBUPD;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->jserial);
	msg.un.reader.slotno = htonl(slotno);
	gspool_jobswap(&res, newjob);
	if  ((ret = gspool_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = gspool_write(fdp->sockfd, (char *) &res, sizeof(res))))
		return  ret;
	if  ((ret = gspool_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_reader.seq != 0)
		fdp->jserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  GSPOOL_OK;
}
