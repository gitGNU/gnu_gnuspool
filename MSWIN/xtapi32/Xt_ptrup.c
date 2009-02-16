/* Xt_ptrup.c -- update printer details

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
#include <winsock.h>
#include <io.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int  xt_write(const SOCKET, char *, unsigned),
	    xt_rmsg(const struct api_fd *, struct api_msg *),
	    xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

extern void	xt_ptrswap(struct apispptr *, const struct apispptr *);

int	xt_ptrupd(const int fd, const unsigned flags, const slotno_t slotno, const struct apispptr *newptr)
{
	int	ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;
	struct	apispptr	res;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_PTRUPD;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->pserial);
	msg.un.reader.slotno = htonl(slotno);
	xt_ptrswap(&res, newptr);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_write(fdp->sockfd, (char *) &res, sizeof(res))))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_reader.seq != 0)
		fdp->pserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  XT_OK;
}
