/* Xt_open.c -- open API connection

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
#include <io.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <winsock.h>
#include <stdlib.h>
#include "xtapi.h"
#include "xtapi_in.h"

#define	MAXFDS	10

static	unsigned	api_max;
static	struct	api_fd	apilist[MAXFDS];

int		xtapi_dataerror;

int		xt_write(const SOCKET fd, char *buff, unsigned size)
{
	int	obytes;
	while  (size != 0)  {
		if  ((obytes = send(fd, buff, size, 0)) < 0)  {
			int	num = WSAGetLastError();
			return  XT_BADWRITE;
		}
		size -= obytes;
		buff += obytes;
	}
	return  0;
}

int		xt_read(const SOCKET fd, char *buff, unsigned size)
{
	int	ibytes;
	while  (size != 0)  {
		if  ((ibytes = recv(fd, buff, size, 0)) < 0)  {
			int	num = WSAGetLastError();
			return  XT_BADREAD;                        
		}
		size -= ibytes;
		buff += ibytes;
	}
	return  0;
}

int		xt_wmsg(const struct api_fd *fdp, struct api_msg *msg)
{
	return  xt_write(fdp->sockfd, (char *) msg, sizeof(struct api_msg));
}

int		xt_rmsg(const struct api_fd *fdp, struct api_msg *msg)
{
	return  xt_read(fdp->sockfd, (char *) msg, sizeof(struct api_msg));
}

struct	api_fd *xt_look_fd(const int fd)
{
	struct	api_fd	*result;

	if  (fd < 0  ||  (unsigned) fd >= api_max)
		return  (struct api_fd *) 0;
	result = &apilist[fd];
	if  (result->sockfd == INVALID_SOCKET)
		return  (struct api_fd *) 0;
	return  result;
}

static	int	getservice(const char *servname, const int proto, const int def_serv)
{
	struct	servent	*sp;
	char	inifilename[_MAX_PATH];
	
	if  (proto == IPPROTO_TCP)  {
		if  (!(sp = getservbyname(servname, "tcp")))
			sp = getservbyname(servname, "TCP");
	}
	else  if  (!(sp = getservbyname(servname, "udp")))
		sp = getservbyname(servname, "UDP");
	
	if  (sp)
		return  ntohs(sp->s_port);
	
	GetProfileString(XI_SOFTWARE, PRODNAME, DEFBASED, inifilename, sizeof(inifilename));
	strcat(inifilename, "\\");
    strcat(inifilename, INIFILE);
    return  GetPrivateProfileInt(proto == IPPROTO_TCP? "TCP Ports": "UDP Ports", "API", def_serv, inifilename);
}

static	int		open_common(const char *hostname, const char *servname, const char *username, const classcode_t classcode)
{
	int		portnum;
	SOCKET	sock;
	unsigned	result;
	struct	api_fd	*ret_fd;
	netid_t	hostid;
	struct	hostent	*hp;
	struct	sockaddr_in	sin;
	WSADATA wd;
	WORD    vr = 0x0101;
	
	WSAStartup(vr, &wd);
    if  (!(hp = gethostbyname(hostname)))
		return  XT_INVALID_HOSTNAME;

	hostid = * (netid_t *) hp->h_addr;
	portnum = getservice(servname? servname: DEFAULT_SERVICE, IPPROTO_TCP, DEF_TCPAPIPORTNUM);

	sin.sin_family = AF_INET;
	sin.sin_port = htons((short)portnum);
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = hostid;

	if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return  XT_NOSOCKET;

	if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		closesocket(sock);
		return  XT_NOCONNECT;
	}

	/*
	 *	Allocate ourselves a "file descriptor" and stuff our stuff in it.
	 */

	for  (result = 0;  result < api_max;  result++)
		if  (apilist[result].sockfd == INVALID_SOCKET)
			goto  found;
	if  (++api_max >= MAXFDS)
		return  XT_NOMEM;
 found:
	ret_fd = &apilist[result];
	ret_fd->portnum = (SHORT) portnum;
	ret_fd->sockfd = sock;
	ret_fd->prodfd = INVALID_SOCKET;
	ret_fd->classcode = classcode;
	ret_fd->hostid = hostid;
	ret_fd->jserial = 0;
	ret_fd->pserial = 0;
	ret_fd->bufmax = 0;
	ret_fd->buff = (char *) 0;
	strncpy(ret_fd->username, username, UIDSIZE);
	ret_fd->username[UIDSIZE] = '\0';
	return  (int) result;
}

int	xt_open(const char *hostname, const char *servname, const char *username, const classcode_t classcode)
{
	int		ret, result;
	struct	api_fd	*ret_fd;
	struct	api_msg	outmsg;
	
	if  ((result = open_common(hostname, servname, username, classcode)) < 0)
		return  result;
	ret_fd = &apilist[result];

	outmsg.code = API_SIGNON;
	strcpy(outmsg.un.signon.username, ret_fd->username);	/* ret_fd 'cous it's truncated */
	outmsg.un.signon.classcode = htonl(classcode);
	if  ((ret = xt_wmsg(ret_fd, &outmsg)) || (ret = xt_rmsg(ret_fd, &outmsg)))
		goto  errret;

	ret = (short) ntohs(outmsg.retcode);

	if  (ret != XT_OK)
		goto  errret;

	ret_fd->classcode = ntohl(outmsg.un.r_signon.classcode);
	return  result;

errret:
	xt_close(result);
	return  ret;
}

int	xt_login(const char *hostname, const char *servname, const char *username, char *passwd, const classcode_t classcode)
{
	int		ret, result;
	struct	api_fd	*ret_fd;
	struct	api_msg	outmsg;
	char	pwbuf[API_PASSWDSIZE+1];

	/*
	 *	Before we do anything, copy argument password to buffer and zap argument
	 *	to make it harder to find password on stack.
	 */

	memset(pwbuf, '\0', sizeof(pwbuf));
	if  (passwd)  {
		int	 cnt = 0;
		while  (*passwd  &&  cnt < API_PASSWDSIZE)  {
			pwbuf[cnt++] = *passwd;
			*passwd++ = '\0';
		}
	}

	if  ((result = open_common(hostname, servname, username, classcode)) < 0)
		return  result;
	ret_fd = &apilist[result];

	outmsg.code = API_LOGIN;
	strcpy(outmsg.un.signon.username, ret_fd->username);	/* ret_fd 'cous it's truncated */
	outmsg.un.signon.classcode = htonl(classcode);
	if  ((ret = xt_wmsg(ret_fd, &outmsg))  ||  (ret = xt_rmsg(ret_fd, &outmsg)))
		goto  errret;

	ret = (short) ntohs(outmsg.retcode);
	
	if  (ret == XT_NO_PASSWD)  {
		if  ((ret = xt_write(ret_fd->sockfd, pwbuf, sizeof(pwbuf))))
			goto  errret;
		
		if  ((ret = xt_rmsg(ret_fd, &outmsg)))
			goto  errret;

		ret = (short) ntohs(outmsg.retcode);

		if  (ret != XT_OK)
			goto  errret;
	}
	else  if  (ret != XT_OK)
		goto  errret;

	if  ((ret = xt_rmsg(ret_fd, &outmsg)))
		goto  errret;

	ret = (short) ntohs(outmsg.retcode);
	if  (ret != XT_OK)
		goto  errret;

	ret_fd->classcode = ntohl(outmsg.un.r_signon.classcode);
	return  result;

errret:
	xt_close(result);
	return  ret;
}

int	xt_procmon(const int fd)
{
	struct	api_fd	*fdp;
	struct	api_msg	imsg;

	if  (!(fdp = xt_look_fd(fd)))
		return  0;

	if  (recvfrom(fdp->prodfd, (char *) &imsg, sizeof(imsg), 0, (struct sockaddr *) 0, (int *) 0) < 0)
		return  0;

	switch  (imsg.code)  {
	default:
		return  0;
	case  API_JOBPROD:
		fdp->jserial = ntohl(imsg.un.r_reader.seq);
		return	XTWINAPI_JOBPROD;
	case  API_PTRPROD:
		fdp->pserial = ntohl(imsg.un.r_reader.seq);
		return	XTWINAPI_PTRPROD;
	}
}

int	xt_setmon(const int fd, HWND hWnd, UINT wMsg)
{
	int		pportnum, ret;
	SOCKET	sockfd;
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	sockaddr_in	sin;
	struct	api_msg	msg;

	if  (!fdp)
		return  XT_INVALID_FD;

	pportnum = getservice(MON_SERVICE, IPPROTO_UDP, DEF_UDPAPIPORTNUM);
	msg.code = API_REQPROD;
	if  ((ret = xt_wmsg(fdp, &msg)))
		return  ret;
		
	sin.sin_family = AF_INET;
	sin.sin_port = htons((short)pportnum);
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;

	if  ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return  XT_NOSOCKET;                      
		
	if  (bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		closesocket(sockfd);
		return  XT_NOBIND;
	}
	if  (WSAAsyncSelect(sockfd, hWnd, wMsg, FD_READ) != 0)  {
		closesocket(sockfd);
		return  XT_NOSOCKET;
	}
	fdp->prodfd = sockfd;
	return  XT_OK;
}

void	xt_unsetmon(const int fd)
{
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg	msg;
	if  (!fdp  ||  fdp->prodfd == INVALID_SOCKET)
		return;
	closesocket(fdp->prodfd);
	fdp->prodfd = INVALID_SOCKET;
	msg.code = API_UNREQPROD;
	xt_wmsg(fdp, &msg);
}

int	xt_close(const int fd)
{
	struct	api_fd	*fdp = xt_look_fd(fd);
	struct	api_msg	outmsg;

	if  (!fdp)
		return  XT_INVALID_FD;

	if  (fdp->prodfd != INVALID_SOCKET)
		xt_unsetmon(fd);
	outmsg.code = API_SIGNOFF;
	xt_wmsg(fdp, &outmsg);
	closesocket(fdp->sockfd);
	fdp->sockfd = INVALID_SOCKET;
	if  (fdp->bufmax != 0)  {
		fdp->bufmax = 0;
		free(fdp->buff);
		fdp->buff = (char *) 0;
	}
	WSACleanup();
	return  XT_OK;
}
