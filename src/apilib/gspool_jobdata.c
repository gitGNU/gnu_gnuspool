/* gspool_jobdata.c -- fetch job data

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
#include "pages.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "incl_sig.h"

extern int	gspool_read(const int, char *, unsigned);
extern int	gspool_write(const int, char *, unsigned);
extern int	gspool_rmsg(const struct api_fd *, struct api_msg *);
extern int	gspool_wmsg(const struct api_fd *, struct api_msg *);
extern struct	api_fd *gspool_look_fd(const int);

static	void  soakupdata(const struct api_fd *fdp)
{
	int	bcount;
	struct	api_msg	msg;
	char	buffer[XTA_BUFFSIZE];

	while  (gspool_rmsg(fdp, &msg) == 0  &&  msg.code == API_DATAOUT  &&
		(bcount = ntohs(msg.un.jobdata.nbytes)) > 0)
		if  (gspool_read(fdp->sockfd, buffer, (unsigned) bcount) != 0)
			return;
}

static FILE *gspool_jdread(const int fd, const unsigned flags, const slotno_t slotno, const unsigned op)
{
	int	ret;
	PIDTYPE	cpid;
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg	msg;
	int	pfd[2];

	if  (!fdp)  {
		gspool_dataerror = GSPOOL_INVALID_FD;
		return  (FILE *) 0;
	}
	msg.code = (unsigned char) op;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->jserial);
	msg.un.reader.slotno = htonl(slotno);
	if  ((ret = gspool_wmsg(fdp, &msg)))  {
		gspool_dataerror = ret;
		return  (FILE *) 0;
	}
	if  ((ret = gspool_rmsg(fdp, &msg)))  {
		gspool_dataerror = ret;
		return  (FILE *) 0;
	}
	if  (msg.un.r_reader.seq != 0)
		fdp->jserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)  {
		gspool_dataerror = (SHORT) ntohs(msg.retcode);
		return  (FILE *) 0;
	}

	/* Ok blast away at the job data */

	if  (pipe(pfd) < 0)  {
		gspool_dataerror = GSPOOL_CHILDPROC;
		soakupdata(fdp);
		return  (FILE *) 0;
	}
	if  ((cpid = fork()) != 0)  {
		int	status;
#ifndef	HAVE_WAITPID
		PIDTYPE	rpid;
#endif
		if  (cpid < 0)  {
			gspool_dataerror = GSPOOL_CHILDPROC;
			soakupdata(fdp);
			return  (FILE *) 0;
		}
		close(pfd[1]);
#ifdef	HAVE_WAITPID
		while   (waitpid(cpid, &status, 0) < 0  &&  errno == EINTR)
			;
#else
		while  ((rpid = wait(&status)) != cpid  &&  (rpid >= 0 || errno == EINTR))
			;
#endif
		if  (status != 0)  {
			gspool_dataerror = GSPOOL_CHILDPROC;
			soakupdata(fdp);
			return  (FILE *) 0;
		}
		return fdopen(pfd[0], "r");
	}
	else  {
		/* Child process....  Fork again so parent doesn't
		   have to worry about zombies other than me on a
		   Monday morning.  */

		close(pfd[0]);
		signal(SIGPIPE, SIG_IGN);

		if  ((cpid = fork()) != 0)
			_exit(cpid < 0? 255: 0);

		if  (op == API_JOBPBRK)  {
			int	bcount, cnt;
#ifndef	WORDS_BIGENDIAN
			int	lcnt;
#endif
			char	*delim;
			LONG	*offs;
			struct	pages	pfp;
			if  (fdp->buff)
				free(fdp->buff);
			if  (gspool_rmsg(fdp, &msg) != 0  ||
			     msg.code != API_DATAOUT  ||
			     (bcount = ntohs(msg.un.jobdata.nbytes)) != sizeof(pfp)  ||
			     gspool_read(fdp->sockfd, (char *) &pfp, sizeof(pfp)) != 0)
				_exit(0);
#ifndef	WORDS_BIGENDIAN
			pfp.delimnum = ntohl(pfp.delimnum);
			pfp.deliml = ntohl(pfp.deliml);
			pfp.lastpage = ntohl(pfp.lastpage);
#endif
			gspool_write(pfd[1], (char *) &pfp, sizeof(pfp));

			/* Read in delimiter */

			if  (!(delim = malloc((unsigned) pfp.deliml)))
				_exit(255); /* What to do?? */
			if  (gspool_rmsg(fdp, &msg) != 0  ||
			     msg.code != API_DATAOUT  ||
			     (bcount = ntohs(msg.un.jobdata.nbytes)) != (int) pfp.deliml  ||
			     gspool_read(fdp->sockfd, delim, bcount) != 0)
				_exit(0);

			gspool_write(pfd[1], delim, (unsigned) pfp.deliml);
			free(delim);

			/* Read in page offsets */

			if  (gspool_rmsg(fdp, &msg) != 0  || msg.code != API_DATAOUT)
				_exit(0);

			bcount = ntohs(msg.un.jobdata.nbytes);
			if  (!(offs = (LONG *) malloc((unsigned) bcount)))
				_exit(255);
			if  (gspool_read(fdp->sockfd, (char *) offs, bcount) != 0)
				_exit(0);
#ifndef	WORDS_BIGENDIAN
			lcnt = bcount / sizeof(LONG);
			for  (cnt = 0;  cnt < lcnt;  cnt++)
				offs[cnt] = ntohl(offs[cnt]);
#endif
			gspool_write(pfd[1], (char *) offs, (unsigned) bcount);
		}
		else  {
			int	bcount;
			char	buffer[XTA_BUFFSIZE];

			while  (gspool_rmsg(fdp, &msg) == 0  && msg.code == API_DATAOUT  &&
				(bcount = ntohs(msg.un.jobdata.nbytes)) > 0)  {
				if  (gspool_read(fdp->sockfd, buffer, (unsigned) bcount) != 0)
					_exit(0);
				gspool_write(pfd[1], buffer, bcount);
			}
		}
		close(pfd[1]);
		_exit(0);
	}
}

FILE *gspool_jobdata(const int fd, const unsigned flags, const slotno_t slotno)
{
	return  gspool_jdread(fd, flags, slotno, API_JOBDATA);
}

FILE  *gspool_jobpbrk(const int fd, const unsigned flags, const slotno_t slotno)
{
	return  gspool_jdread(fd, flags, slotno, API_JOBPBRK);
}
