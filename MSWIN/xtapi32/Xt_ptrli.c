/* Xt_ptrli.c -- list printers

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
#include <malloc.h>
#include <winsock.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int	xt_read(const SOCKET, char *, unsigned),
		xt_rmsg(const struct api_fd *, struct api_msg *),
		xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

int	xt_ptrlist(const int fd, const unsigned	flags, int *np, slotno_t  **slots)
{
	int		ret;
	unsigned	numptrs;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  XT_INVALID_FD;
		
	msg.code = API_PTRLIST;
	msg.un.lister.flags = htonl(flags);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);

	/* Get number of printers */

	fdp->pserial = ntohl(msg.un.r_lister.seq);
	numptrs = (unsigned) ntohl(msg.un.r_lister.nitems);
	if  (np)
		*np = (int) numptrs;

	/* Try to allocate enough space to hold the list.
	   If we don't succeed we'd better carry on reading it
	   so we don't get out of sync. */

	if  (numptrs != 0)  {
		unsigned  nbytes = numptrs * sizeof(slotno_t), cnt;
		slotno_t  *sp;
		if  (nbytes > fdp->bufmax)  {
			if  (fdp->bufmax != 0)  {
				free(fdp->buff);
				fdp->bufmax = 0;
				fdp->buff = (char *) 0;
			}
			if  (!(fdp->buff = malloc(nbytes)))  {
				unsigned  cnt;
				for  (cnt = 0;  cnt < numptrs;  cnt++)  {
					ULONG  slurp;
					if  ((ret = xt_read(fdp->sockfd, (char *) &slurp, sizeof(slurp))))
						return  ret;
				}
				return  XT_NOMEM;
			}
			fdp->bufmax = nbytes;
		}
		if  ((ret = xt_read(fdp->sockfd, fdp->buff, nbytes)))
			return  ret;

		sp = (slotno_t *) fdp->buff;
		for  (cnt = 0;  cnt < numptrs;  cnt++)  {
			*sp = ntohl(*sp);
			sp++;
		}
	}

	/* Set up answer */

	if  (slots)
		*slots = (slotno_t *) fdp->buff;
	return  XT_OK;
}
