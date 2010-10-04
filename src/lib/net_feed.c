/* net_feed.c -- get spool files from remote machines

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

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#ifdef	NETWORK_VERSION
#include "incl_net.h"
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "incl_unix.h"

/* Grab hold of a spool file on a remote system.  */

FILE *net_feed(const int type, const netid_t netid, const slotno_t jslot, const jobno_t jobno)
{
	int			sock;
	FILE			*result;
	struct	feeder		fd;
	struct	sockaddr_in	sin;

	if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return  (FILE *) 0;

	/* Set up bits and pieces
	   The port number is set up in the job shared memory segment.  */

	sin.sin_family = AF_INET;
	sin.sin_port = Job_seg.dptr->js_viewport;
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = netid;

	if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(sock);
		return  (FILE *) 0;
	}

	/* Send out initial packet saying what we want.  In fact the
	   job slot number doesn't matter except for paged jobs.  */

	fd.fdtype = (char) type;
	fd.jobslot = htonl(jslot);
	fd.jobno = htonl(jobno);
	if  (write(sock, (char *) &fd, sizeof(fd)) != sizeof(fd))  {
		close(sock);
		return  (FILE *) 0;
	}

	/* Create result descriptor.
	   We are strictly speaking cheating saying it's read-only, but
	   we don't plan to do any writing and stdio doesn't mind.  */

	if  ((result = fdopen(sock, "r")) == (FILE *) 0)  {
		close(sock);
		return  (FILE *) 0;
	}
#ifdef	SETVBUF_REVERSED
	setvbuf(result, _IOFBF, (char *) 0, BUFSIZ);
#else
	setvbuf(result, (char *) 0, _IOFBF, BUFSIZ);
#endif
	return  result;
}

#else

/* Dummy version to keep linker happy */

FILE *net_feed(const int type, const netid_t netid, const slotno_t jslot, const jobno_t	jobno)
{
	return  (FILE *) 0;
}
#endif
