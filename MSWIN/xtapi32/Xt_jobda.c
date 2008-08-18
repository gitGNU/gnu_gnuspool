/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_jobdata.c: created by John Collins on Mon Mar 14 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_jobda.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_jobda.c,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 22.1  1995/01/13  17:06:57  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:22:26  jmc
 * Brand new Release 21
 *
 * Revision 20.2  1994/05/24  12:37:07  jmc
 * Work around problem with recvfrom.
 *
 * Revision 20.1  1994/03/24  17:25:52  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 */

#include <sys/types.h>
#include <string.h>
#include <winsock.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int  xt_read(const SOCKET, char *, unsigned),
	    xt_write(const SOCKET, char *, unsigned),
	    xt_rmsg(const struct api_fd *, struct api_msg *),
	    xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct	api_fd *xt_look_fd(const int);

static	int	xt_jdread(const int	fd,
					  const int outfile,
					  int (*func)(int,void*,unsigned),
					  const unsigned flags,
					  const slotno_t slotno,
					  const unsigned op)
{
	int	ret;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  xtapi_dataerror = XT_INVALID_FD;

	msg.code = (unsigned char) op;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->jserial);
	msg.un.reader.slotno = htonl(slotno);
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  xtapi_dataerror = ret;

	if  ((ret = xt_rmsg(fdp, &msg)))
		return  xtapi_dataerror = ret;
	if  (msg.un.r_reader.seq != 0)
		fdp->jserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  xtapi_dataerror = ntohs(msg.retcode);

	if  (op == API_JOBPBRK)  {
		int		bcount;
		struct	apipages	pfp;
		char	buffer[XTA_BUFFSIZE];

		if  ((ret = xt_rmsg(fdp, &msg)) != 0)
			return  xtapi_dataerror = ret;
		if  (msg.code != API_DATAOUT  ||  (bcount = ntohs(msg.un.jobdata.nbytes)) != sizeof(pfp))
			return  xtapi_dataerror = XT_BADREAD;
		if  ((ret = xt_read(fdp->sockfd, (char *) &pfp, sizeof(pfp))) != 0)
			return  xtapi_dataerror = ret;

		pfp.delimnum = ntohl(pfp.delimnum);
		pfp.deliml = ntohl(pfp.deliml);
		pfp.lastpage = ntohl(pfp.lastpage);
		if  ((*func)(outfile, (char *) &pfp, sizeof(pfp)) != sizeof(pfp))
			return  xtapi_dataerror = XT_BADWRITE;

		/* Read in delimiter */

		if  ((ret = xt_rmsg(fdp, &msg)) != 0)
			return  xtapi_dataerror = ret;

		if  (msg.code != API_DATAOUT  ||
		     (bcount = ntohs(msg.un.jobdata.nbytes)) != (int) pfp.deliml)
			return  xtapi_dataerror = XT_BADREAD;
        
		while  (bcount > 0)  {
			int  nbytes = bcount;
			if  (nbytes > sizeof(buffer))
				nbytes = sizeof(buffer);
			if  ((ret = xt_read(fdp->sockfd, buffer, nbytes)) != 0)
				return  ret;
			(*func)(outfile, buffer, nbytes);
			bcount -= nbytes;
		}

		/* Read in page offsets */

		if  ((ret = xt_rmsg(fdp, &msg)) != 0)
			return  xtapi_dataerror = ret;
		if  (msg.code != API_DATAOUT)
			return  xtapi_dataerror = XT_BADREAD;

		bcount = ntohs(msg.un.jobdata.nbytes);
		
		while  (bcount > 0)  {
			int	 nbytes = bcount, lcnt, cnt;
			LONG	*offs = (LONG *) buffer;
			if  (nbytes > sizeof(buffer))
				nbytes = sizeof(buffer);
        	if  ((ret = xt_read(fdp->sockfd, buffer, nbytes)) != 0)
				return  ret;				
			lcnt = nbytes / sizeof(LONG);
			for  (cnt = 0;  cnt < lcnt;  cnt++)
				offs[cnt] = ntohl(offs[cnt]);
			(*func)(outfile, buffer, (unsigned) nbytes);
		}
	}
	else  {
		int	bcount;
		char	buffer[XTA_BUFFSIZE];
        
		for  (;;)  {
			if  ((ret = xt_rmsg(fdp, &msg)) != 0)
				return  xtapi_dataerror = ret;
			if  (msg.code != API_DATAOUT)
				break;
			if  ((bcount = ntohs(msg.un.jobdata.nbytes)) <= 0)
				break;
			if  ((ret = xt_read(fdp->sockfd, buffer, (unsigned) bcount)) != 0)
				return  xtapi_dataerror = ret;
			(*func)(outfile, buffer, (unsigned) bcount);
		}
	}
	return  XT_OK;
}

int	xt_jobdata(const int fd,
		   const int outfile,
		   int (*func)(int,void*,unsigned),
		   const unsigned flags,
		   const slotno_t slotno)
{
	return  xt_jdread(fd, outfile, func, flags, slotno, API_JOBDATA);
}

int	xt_jobpbrk(const int fd,
		   const int outfile,
		   int (*func)(int,void*,unsigned),
		   const unsigned flags,
		   const slotno_t slotno)
{
	return  xt_jdread(fd, outfile, func, flags, slotno, API_JOBPBRK);
}
