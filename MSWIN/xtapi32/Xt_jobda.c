/* Xt_jobda.c -- job data

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
