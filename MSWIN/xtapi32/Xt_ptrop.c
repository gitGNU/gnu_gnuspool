/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_ptrop.c: created by John Collins on Mon Mar 14 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_ptrop.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_ptrop.c,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 22.1  1995/01/13  17:06:57  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:22:26  jmc
 * Brand new Release 21
 *
 * Revision 20.1  1994/03/24  17:25:52  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 */

#include <sys/types.h>
#include <winsock.h>
#include <io.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int	xt_rmsg(const struct api_fd *, struct api_msg *),
		xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

int	xt_ptrop(const int fd, const unsigned flags, const slotno_t	slotno, const unsigned op)
{
	int	ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg		msg;

	if  (!fdp)
		return  XT_INVALID_FD;
	msg.code = API_PTROP;
	msg.un.pop.flags = htonl(flags);
	msg.un.pop.seq = htonl(fdp->pserial);
	msg.un.pop.slotno = htonl(slotno);
	msg.un.pop.op = htonl(op);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xt_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_reader.seq != 0)
		fdp->pserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  XT_OK;
}
