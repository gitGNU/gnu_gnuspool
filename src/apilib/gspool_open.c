/* gspool_open.c -- API open connection

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
#include "gspool.h"
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <pwd.h>
#include <errno.h>
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "xtapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "incl_sig.h"
#include "files.h"
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "services.h"

#ifndef	_NFILE
#define	_NFILE	64
#endif

#define	MAXFDS	(_NFILE / 3)

static	unsigned	api_max;
static	struct	api_fd	apilist[MAXFDS];

int	gspool_dataerror;

int	gspool_write(const int fd, char *buff, unsigned size)
{
	int	obytes;
	while  (size != 0)  {
		if  ((obytes = write(fd, buff, size)) < 0)  {
			if  (errno == EINTR)
				continue;
			return  GSPOOL_BADWRITE;
		}
		size -= obytes;
		buff += obytes;
	}
	return  0;
}

int	gspool_read(const int fd, char *buff, unsigned size)
{
	int	ibytes;
	while  (size != 0)  {
		if  ((ibytes = read(fd, buff, size)) < 0)  {
			if  (errno == EINTR)
				continue;
			return  GSPOOL_BADREAD;
		}
		size -= ibytes;
		buff += ibytes;
	}
	return  0;
}

int	gspool_wmsg(const struct api_fd *fdp, struct api_msg *msg)
{
	return  gspool_write(fdp->sockfd, (char *) msg, sizeof(struct api_msg));
}

int	gspool_rmsg(const struct api_fd *fdp, struct api_msg *msg)
{
	return  gspool_read(fdp->sockfd, (char *) msg, sizeof(struct api_msg));
}

struct	api_fd  *gspool_look_fd(const int fd)
{
	struct	api_fd	*result;

	if  (fd < 0  ||  (unsigned) fd >= api_max)
		return  (struct api_fd *) 0;
	result = &apilist[fd];
	if  (result->sockfd < 0)
		return  (struct api_fd *) 0;
	return  result;
}

#if defined(FASYNC) && defined(F_SETOWN)
static	struct api_fd *find_prod(const int fd)
{
	unsigned  cnt;
	for  (cnt = 0;  cnt < api_max;  cnt++)
		if  (apilist[cnt].prodfd == fd)
			return  &apilist[cnt];
	return  (struct api_fd *) 0;
}
#endif

int	gspool_open(const char *hostname, const char *servname, const classcode_t classcode)
{
	int	portnum, sock, ret;
	unsigned	result;
	struct	api_fd	*ret_fd;
	netid_t	hostid;
	const	char	*serv;
	struct	passwd	*pw;
	struct	hostent	*hp;
	struct	servent	*sp;
	struct	sockaddr_in	sin;
	struct	api_msg	outmsg;

	hp = gethostbyname(hostname);
	if  (!hp)  {
		endhostent();
		return  GSPOOL_INVALID_HOSTNAME;
	}
	hostid = * (LONG *) hp->h_addr;
	endhostent();

	serv = servname? servname: API_DEFAULT_SERVICE;
	if  (!(sp = getservbyname(serv, "tcp")))
		sp = getservbyname(serv, "TCP");
	if  (!sp)  {
		endservent();
		return  servname? GSPOOL_INVALID_SERVICE: GSPOOL_NODEFAULT_SERVICE;
	}
	portnum = ntohs(sp->s_port);
	endservent();

	/* And now for the groovy socket stuff....  */

	sin.sin_family = AF_INET;
	sin.sin_port = htons(portnum);
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = hostid;

	if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return  GSPOOL_NOSOCKET;

	if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(sock);
		return  GSPOOL_NOCONNECT;
	}


	/* Allocate ourselves a "file descriptor" and stuff our stuff in it.  */

	for  (result = 0;  result < api_max;  result++)
		if  (apilist[result].sockfd < 0)
			goto  found;
	if  (++api_max >= MAXFDS)
		return  GSPOOL_NOMEM;
 found:
	ret_fd = &apilist[result];
	ret_fd->portnum = (SHORT) portnum;
	ret_fd->sockfd = (SHORT) sock;
	ret_fd->prodfd = -1;
	ret_fd->classcode = classcode;
	ret_fd->hostid = hostid;
	ret_fd->jobfn = (void (*)()) 0;
	ret_fd->ptrfn = (void (*)()) 0;
	ret_fd->jserial = 0;
	ret_fd->pserial = 0;
	ret_fd->bufmax = 0;
	ret_fd->buff = (char *) 0;
	pw = getpwuid(geteuid());
	strncpy(ret_fd->username, pw? pw->pw_name: SPUNAME, UIDSIZE);
	ret_fd->username[UIDSIZE] = '\0';
	outmsg.code = API_SIGNON;
	strcpy(outmsg.un.signon.username, ret_fd->username);
	outmsg.un.signon.classcode = htonl(classcode);
	if  ((ret = gspool_wmsg(ret_fd, &outmsg)))  {
		gspool_close((int) result);
		return  ret;
	}
	if  ((ret = gspool_rmsg(ret_fd, &outmsg)))  {
		gspool_close((int) result);
		return  ret;
	}
	if  (outmsg.retcode != 0)  {
		gspool_close((int) result);
		return  (SHORT) ntohs(outmsg.retcode);
	}
	ret_fd->classcode = ntohl(outmsg.un.r_signon.classcode);
	return  (int) result;
}

#if	defined(FASYNC) && defined(F_SETOWN)
static	void	procpoll(int fd)
{
	struct	api_fd	*fdp;
	struct	api_msg	imsg;
	SOCKLEN_T		repl = sizeof(struct sockaddr_in);
	struct	sockaddr_in	reply_addr;

	if  (!(fdp = find_prod(fd)))
		return;

	if  (recvfrom(fdp->prodfd, (char *) &imsg, sizeof(imsg), 0, (struct sockaddr *) &reply_addr, &repl) < 0)
		return;
	switch  (imsg.code)  {
	default:
		return;
	case  API_JOBPROD:
		if  (fdp->jobfn)
			(*fdp->jobfn)((int) (fdp - apilist));
		fdp->jserial = ntohl(imsg.un.r_reader.seq);
		return;
	case  API_PTRPROD:
		if  (fdp->ptrfn)
			(*fdp->ptrfn)((int) (fdp - apilist));
		fdp->pserial = ntohl(imsg.un.r_reader.seq);
		return;
	}
}

static	RETSIGTYPE	catchpoll(int n)
{
	unsigned	cnt;
	int		fd;

	int	highfd = -1, nret;
	fd_set	ready;

#ifdef	UNSAFE_SIGNALS
	signal(n, catchpoll);
#endif

	for  (;;)  {
		FD_ZERO(&ready);
		for  (cnt = 0;  cnt < api_max;  cnt++)  {
			if  ((fd = apilist[cnt].prodfd) >= 0)  {
				FD_SET(fd, &ready);
				if  (fd > highfd)
					highfd = fd;
			}
		}
		nret = select(highfd+1, &ready, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0);
		if  (nret <= 0)
			break;
		for  (cnt = 0;  nret > 0  &&  cnt <= (unsigned) highfd;  cnt++)
			if  (FD_ISSET(cnt, &ready))  {
				procpoll(cnt);
				nret--;
			}
	}
}

static	int	setmon(struct api_fd *fdp)
{
	int	sockfd, pportnum, ret;
	const	char	*serv = API_MON_SERVICE;
	struct	servent	*sp;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	z;
#endif
	int	protonum;
	struct	protoent	*pp;
	struct	sockaddr_in	sin;
	struct	api_msg	msg;

	if  (!(pp = getprotobyname("udp")) && !(pp = getprotobyname("UDP")))  {
		endprotoent();
		return  GSPOOL_NODEFAULT_SERVICE;
	}
	protonum = pp->p_proto;
	endprotoent();
	if  (!(sp = getservbyname(serv, "udp")))
		sp = getservbyname(serv, "UDP");
	if  (!sp)  {
		endservent();
		return  GSPOOL_NODEFAULT_SERVICE;
	}
	pportnum = sp->s_port;
	endservent();
	msg.code = API_REQPROD;
	if  ((ret = gspool_wmsg(fdp, &msg)))
		return  ret;
	sin.sin_family = AF_INET;
	sin.sin_port = (SHORT) pportnum;
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;

#ifdef	STRUCT_SIG
	z.sighandler_el = catchpoll;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGIO, &z, (struct sigstruct_name *) 0);
#else
	signal(SIGIO, catchpoll);
#endif
	if  ((sockfd = socket(PF_INET, SOCK_DGRAM, protonum)) < 0)
		return  GSPOOL_NOSOCKET;
	if  (bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(sockfd);
		return  GSPOOL_NOBIND;
	}
	fdp->prodfd = sockfd;
	if  (fcntl(sockfd, F_SETOWN, getpid()) < 0  ||
	     fcntl(sockfd, F_SETFL, FASYNC) < 0)
		return  GSPOOL_NOSOCKET;
	return  GSPOOL_OK;
}

static	void	unsetmon(struct api_fd *fdp)
{
	struct	api_msg	msg;
	if  (fdp->prodfd < 0)
		return;
	close(fdp->prodfd);
	fdp->prodfd = -1;
	msg.code = API_UNREQPROD;
	gspool_wmsg(fdp, &msg);
}

int	gspool_jobmon(const int fd, void (*fn)(const int))
{
	struct	api_fd	*fdp = gspool_look_fd(fd);

	if  (!fdp)
		return  GSPOOL_INVALID_FD;

	if  (fn)  {
		int	ret;
		if  (fdp->prodfd < 0  &&  (ret = setmon(fdp)) != 0)
			return  ret;
	}
	else  if  (fdp->jobfn && !fdp->ptrfn)
		unsetmon(fdp);
	fdp->jobfn = fn;
	return  GSPOOL_OK;
}

int	gspool_ptrmon(const int fd, void (*fn)(const int))
{
	struct	api_fd	*fdp = gspool_look_fd(fd);

	if  (!fdp)
		return  GSPOOL_INVALID_FD;

	if  (fn)  {
		int	ret;
		if  (fdp->prodfd < 0  &&  (ret = setmon(fdp)) != 0)
			return  ret;
	}
	else  if  (fdp->ptrfn && !fdp->jobfn)
		unsetmon(fdp);
	fdp->ptrfn = fn;
	return  GSPOOL_OK;
}
#endif /* Possible to get SIGIO working */

int	gspool_close(const int fd)
{
	struct	api_fd	*fdp = gspool_look_fd(fd);
	struct	api_msg	outmsg;

	if  (!fdp)
		return  GSPOOL_INVALID_FD;
#if	defined(FASYNC) && defined(F_SETOWN)
	if  (fdp->prodfd >= 0)  {
		unsetmon(fdp);
		fdp->jobfn = (void (*)()) 0;
		fdp->ptrfn = (void (*)()) 0;
	}
#endif
	outmsg.code = API_SIGNOFF;
	gspool_wmsg(fdp, &outmsg);
	close(fdp->sockfd);
	fdp->sockfd = -1;
	if  (fdp->bufmax != 0)  {
		fdp->bufmax = 0;
		free(fdp->buff);
		fdp->buff = (char *) 0;
	}
	return  GSPOOL_OK;
}
