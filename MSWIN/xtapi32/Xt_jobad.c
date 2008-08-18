/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xt_jobadd.c: created by John Collins on Fri Mar 11 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xt_jobad.c,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xt_jobad.c,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 22.1  1995/01/13  17:06:57  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:22:26  jmc
 * Brand new Release 21
 *
 * Revision 20.2  1994/03/30  12:03:20  jmc
 * Fix botched declaration.
 *
 * Revision 20.1  1994/03/24  17:25:52  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 */

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
