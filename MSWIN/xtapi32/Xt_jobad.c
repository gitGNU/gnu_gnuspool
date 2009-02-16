/* Xt_jobad.c -- add job

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
#include <errno.h>
#include <string.h>
#include <winsock.h>
#include <process.h>
#include "xtapi.h"
#include "xtapi_in.h"

extern int	xt_read(const SOCKET, char *, unsigned),
		xt_write(const SOCKET, char *, unsigned),
		xt_rmsg(const struct api_fd *, struct api_msg *),
		xt_wmsg(const struct api_fd *, struct api_msg *);
			
extern	struct	api_fd *xt_look_fd(const int);

extern void	xt_jobswap(struct apispq *, const struct apispq *);

static	jobno_t	gen_jobno()
{
	static	char	doneit = 0;
	static	jobno_t	result;
	
	if  (!doneit)  {
		doneit = 1;
		result = ((unsigned long) getpid()) % 0x7ffffU;
	}
	else
		result++;
	return  result;
}
		
int	xt_jobadd(const int fd,
		  const int	infile,
		  int	(*func)(int,void*,unsigned),
		  struct apispq *newjob,
		  const char *delim,
		  const unsigned deliml,
		  const unsigned delimnum)
{
	int		ret, bcount;
	jobno_t		jobno;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg	msg;
	struct	apispq	res;
	struct	apipages	pf;
	char	buffer[XTA_BUFFSIZE];

	if  (!fdp)
		return  xtapi_dataerror = XT_INVALID_FD;

	msg.code = API_JOBADD;
	msg.un.jobdata.jobno = (jobno_t) htonl(gen_jobno());
	xt_jobswap(&res, newjob);
	res.apispq_dflags &= APISPQ_ERRLIMIT | APISPQ_PGLIMIT; /* I didnt mean a ~ here */
	if  (delim  &&  (delimnum > 1 || deliml > 1  ||  delim[0] != '\f'))  {
		res.apispq_dflags |= APISPQ_PAGEFILE;
		pf.delimnum = htonl(delimnum);
		pf.deliml = htonl(deliml);
		pf.lastpage = 0;
	}
	else
		res.apispq_dflags &= ~APISPQ_PAGEFILE;

	if  ((ret = xt_wmsg(fdp, &msg)))
		return  xtapi_dataerror = ret;

	if  ((ret = xt_write(fdp->sockfd, (char *) &res, sizeof(res))))
		return  xtapi_dataerror = ret;

	if  ((res.apispq_dflags & APISPQ_PAGEFILE)  &&
	     ((ret = xt_write(fdp->sockfd, (char *) &pf, sizeof(pf)))  ||
	     (ret = xt_write(fdp->sockfd, (char *) delim, (unsigned) deliml))))
		return  xtapi_dataerror = ret;

	if  ((ret = xt_rmsg(fdp, &msg)))
		return  xtapi_dataerror = ret;

	if  (msg.retcode != 0)
		return  xtapi_dataerror = ntohs(msg.retcode);

	/* Return job number in apispq_job if the caller is interested. */

	newjob->apispq_job = jobno = ntohl(msg.un.jobdata.jobno);
	msg.code = API_DATAIN;
	msg.un.jobdata.jobno = htonl(jobno);
	while  ((bcount = (*func)(infile, buffer, XTA_BUFFSIZE)) > 0)  {
		msg.un.jobdata.nbytes = htons((short)bcount);
		if  ((ret = xt_wmsg(fdp, &msg))  ||
			 (ret = xt_write(fdp->sockfd, buffer, (unsigned) bcount)))
			 return  xtapi_dataerror = ret;
	}
	msg.code = API_DATAEND;
	if  ((ret = xt_wmsg(fdp, &msg))  ||  (ret = xt_rmsg(fdp, &msg)))
		return  xtapi_dataerror = ret;
	return  xtapi_dataerror = ntohs(msg.retcode);
}
