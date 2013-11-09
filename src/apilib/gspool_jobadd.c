/* gspool_jobadd.c -- create spool job

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
#include <errno.h>
#include "gspool.h"
#include "xtapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"

extern int	gspool_read(const int, char *, unsigned);
extern int	gspool_write(const int, char *, unsigned);
extern int	gspool_rmsg(const struct api_fd *, struct api_msg *);
extern int	gspool_wmsg(const struct api_fd *, struct api_msg *);
extern struct	api_fd *gspool_look_fd(const int);
extern void	gspool_jobswap(struct apispq *, const struct apispq *);

static void	jabort(const struct api_fd *fdp, const jobno_t jobno)
{
	struct	api_msg	msg;
	msg.code = API_DATAABORT;
	msg.un.jobdata.jobno = htonl(jobno);
	gspool_wmsg(fdp, &msg);
}

FILE *gspool_jobadd(const int fd, struct apispq *newjob, const char *delim, const unsigned deliml, const unsigned delimnum)
{
	int		ret;
	LONG		cpid;
	jobno_t		jobno;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg	msg;
	struct	apispq	res;
	struct	apipages	pf;
	int	pfd[2];

	if  (!fdp)  {
		gspool_dataerror = GSPOOL_INVALID_FD;
		return  (FILE *) 0;
	}

	msg.code = API_JOBADD;
	msg.un.jobdata.jobno = htonl((jobno_t) getpid());
	gspool_jobswap(&res, newjob);
	res.apispq_dflags &= APISPQ_ERRLIMIT | APISPQ_PGLIMIT; /* I didnt mean a ~ here */
	if  (delim  &&  (delimnum > 1 || deliml > 1  ||  delim[0] != '\f'))  {
		res.apispq_dflags |= APISPQ_PAGEFILE;
		pf.delimnum = htonl(delimnum);
		pf.deliml = htonl(deliml);
		pf.lastpage = 0;
	}
	else
		res.apispq_dflags &= ~APISPQ_PAGEFILE;
	if  ((ret = gspool_wmsg(fdp, &msg)))  {
		gspool_dataerror = ret;
		return  (FILE *) 0;
	}
	if  ((ret = gspool_write(fdp->sockfd, (char *) &res, sizeof(res))))  {
		gspool_dataerror = ret;
		return  (FILE *) 0;
	}
	if  ((res.apispq_dflags & APISPQ_PAGEFILE)  &&
	     ((ret = gspool_write(fdp->sockfd, (char *) &pf, sizeof(pf)))  ||
	     (ret = gspool_write(fdp->sockfd, (char *) delim, (unsigned) deliml))))  {
		gspool_dataerror = ret;
		return  (FILE *) 0;
	}
	if  ((ret = gspool_rmsg(fdp, &msg)))  {
		gspool_dataerror = ret;
		return  (FILE *) 0;
	}
	if  (msg.retcode != 0)  {
		gspool_dataerror = (SHORT) ntohs(msg.retcode);
		return  (FILE *) 0;
	}

	/* Return job number in apispq_job if the caller is interested.  */

	newjob->apispq_job = jobno = ntohl(msg.un.jobdata.jobno);

	/* Ok blast away at the job data */

	if  (pipe(pfd) < 0)  {
		jabort(fdp, jobno);
		gspool_dataerror = GSPOOL_CHILDPROC;
		return  (FILE *) 0;
	}
	if  ((cpid = fork()) != 0)  {
		int	status;
#ifndef	HAVE_WAITPID
		PIDTYPE	rpid;
#endif
		if  (cpid < 0)  {
			jabort(fdp, jobno);
			gspool_dataerror = GSPOOL_CHILDPROC;
			return  (FILE *) 0;
		}
		close(pfd[0]);
#ifdef	HAVE_WAITPID
		while  (waitpid(cpid, &status, 0) < 0  &&  errno == EINTR)
#else
		while  ((rpid = wait(&status)) != cpid  &&  (rpid >= 0 || errno == EINTR))
			;
#endif
		if  (status != 0)  {
			gspool_dataerror = GSPOOL_CHILDPROC;
			return  (FILE *) 0;
		}
		return fdopen(pfd[1], "w");
	}
	else  {
		/* Child process....  Fork again so parent doesn't
		   have to worry about zombies other than me on a
		   Monday morning.  */

		int	bcount;
		char	buffer[XTA_BUFFSIZE];

		close(pfd[1]);

		if  ((cpid = fork()) != 0)
			exit(cpid < 0? 255: 0);

		msg.code = API_DATAIN;
		msg.un.jobdata.jobno = htonl(jobno);
		while  ((bcount = read(pfd[0], buffer, XTA_BUFFSIZE)) > 0)  {
			msg.un.jobdata.nbytes = htons(bcount);
			gspool_wmsg(fdp, &msg);
			gspool_write(fdp->sockfd, buffer, (unsigned) bcount);
		}
		msg.code = API_DATAEND;
		gspool_wmsg(fdp, &msg);
		exit(0);
	}
}

int	gspool_jobres(const int fd, jobno_t *jno)
{
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  GSPOOL_INVALID_FD;

	gspool_rmsg(fdp, &msg);
	if  (msg.retcode == 0  &&  jno)
		*jno = ntohl(msg.un.jobdata.jobno);
	return  (SHORT) ntohs(msg.retcode);
}
