/* rsp_net.c -- rspr comms module

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
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "defaults.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "pages.h"
#include "files.h"
#include "errnums.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "client_if.h"
#include "services.h"

/* Currently we use the same name for the TCP and UDP ports */

const	char	Sname[] = GSNETSERV_PORT;
#define	TSname	Sname

static	int	udpsock = -1, tcpsocket = -1;
static	struct	sockaddr_in	serv_addr, cli_addr;

#define	INITENV	30
#define	INCENV	20

#define	RTIMEOUT	5

int	sock_read(const int sock, char * buffer, int nbytes)
{
	while  (nbytes > 0)  {
		int	rbytes = read(sock, buffer, nbytes);
		if  (rbytes <= 0)
			return  0;
		buffer += rbytes;
		nbytes -= rbytes;
	}
	return  1;
}

int	sock_write(const int sock, const char * buffer, int nbytes)
{
	while  (nbytes > 0)  {
		int	rbytes = write(sock, buffer, nbytes);
		if  (rbytes < 0)
			return  0;
		buffer += rbytes;
		nbytes -= rbytes;
	}
	return  1;
}

static	int	initsock(const netid_t hostid)
{
	int	sockfd;
	SHORT	portnum;
	SHORT	udpproto;
	struct	servent	*sp;
	struct	protoent  *pp;
	char	*udp_protoname;

	if  (!((pp = getprotobyname("udp"))  || (pp = getprotobyname("UDP"))))  {
		print_error($E{No UDP protocol});
		exit(E_NETERR);
	}
	udp_protoname = pp->p_name;
	udpproto = pp->p_proto;
	endprotoent();

	/* Get port number for this caper */

	if  (!(sp = getservbyname(Sname, udp_protoname)))  {
		disp_str = Sname;
		print_error($E{No UDP service});
		endservent();
		exit(E_NETERR);
	}
	portnum = sp->s_port;
	endservent();

	BLOCK_ZERO(&serv_addr, sizeof(serv_addr));
	BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
	serv_addr.sin_family = cli_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = hostid;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = portnum;
	cli_addr.sin_port = 0;

	/* Save now in case of error.  */

	disp_arg[0] = ntohs(portnum);
	disp_arg[1] = hostid;

	if  ((sockfd = socket(AF_INET, SOCK_DGRAM, udpproto)) < 0)  {
		print_error($E{Cannot create UDP socket});
		exit(E_NETERR);
	}
	if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
		print_error($E{Cannot bind UDP socket});
		close(sockfd);
		exit(E_NETERR);
	}
	return  sockfd;
}

/* Unpack spuser from networked version. */

static	void	unpack_spuser(struct spdet *dest, const struct spdet *src)
{
	strncpy(dest->spu_form, src->spu_form, MAXFORM);
	strncpy(dest->spu_formallow, src->spu_formallow, ALLOWFORMSIZE);
	strncpy(dest->spu_ptr, src->spu_ptr, PTRNAMESIZE);
	strncpy(dest->spu_ptrallow, src->spu_ptrallow, JPTRNAMESIZE);
	dest->spu_isvalid = src->spu_isvalid;
	dest->spu_minp = src->spu_minp;
	dest->spu_maxp = src->spu_maxp;
	dest->spu_defp = src->spu_defp;
	dest->spu_cps  = src->spu_cps;
	dest->spu_flgs = ntohl(src->spu_flgs);
	dest->spu_class = ntohl(src->spu_class);
	dest->spu_user = ntohl((ULONG) src->spu_user);
}

static	RETSIGTYPE	asig(int n)
{
	return;			/* Don't do anything just return setting EINTR */
}

static int udp_enquire(char *outmsg, const int outlen, char *inmsg, const int inlen)
{
#ifdef	STRUCT_SIG
	struct	sigstruct_name	za;
#endif
	int	inbytes;
	SOCKLEN_T		repl = sizeof(struct sockaddr_in);
	struct	sockaddr_in	reply_addr;
	if  (sendto(udpsock, outmsg, outlen, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  {
		disp_str = look_host(serv_addr.sin_addr.s_addr);
		print_error($E{Cannot send on UDP socket});
		exit(E_NETERR);
	}
#ifdef	STRUCT_SIG
	za.sighandler_el = asig;
	sigmask_clear(za);
	za.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGALRM, &za, (struct sigstruct_name *) 0);
#else
	signal(SIGALRM, asig);
#endif
	alarm(RTIMEOUT);
	if  ((inbytes = recvfrom(udpsock, inmsg, inlen, 0, (struct sockaddr *) &reply_addr, &repl)) <= 0)  {
		disp_str = look_host(serv_addr.sin_addr.s_addr);
		print_error($E{Cannot receive on UDP socket});
		exit(E_NETERR);
	}
	alarm(0);
#ifdef	STRUCT_SIG
	za.sighandler_el = SIG_IGN;
	sigact_routine(SIGALRM, &za, (struct sigstruct_name *) 0);
#else
	signal(SIGALRM, SIG_IGN);
#endif
	return  inbytes;
}

/* Get details of account for given user */

struct	spdet *remgetspuser(const netid_t hostid, char *realuname)
{
	char	enq[UIDSIZE + 3];
	struct	ua_reply	resp;
	static	struct  spdet	result;

	if  (udpsock < 0)
		udpsock = initsock(hostid);
	BLOCK_ZERO(enq, sizeof(enq));
	enq[0] = CL_SV_UENQUIRY;
	strncpy(&enq[1], realuname, UIDSIZE);
	udp_enquire((char *) enq, sizeof(enq), (char *) &resp, sizeof(resp));
	unpack_spuser(&result, &resp.ua_perm);
	if  (result.spu_isvalid != SPU_VALID)  {
		disp_str = realuname;
		disp_str2 = look_host(hostid);
		if  (result.spu_isvalid == SPU_INVALID)  {
			print_error($E{User invalid at remote});
			exit(E_UNOTSETUP);
		}
	}
	return	&result;
}

static	void	packjob(struct spq *dest, const struct spq *src)
{
	dest->spq_job = htonl((ULONG) src->spq_job);
	dest->spq_netid = 0L;
	dest->spq_orighost = 0L;
	dest->spq_rslot = 0L;
	dest->spq_time = htonl((ULONG) src->spq_time);
	dest->spq_proptime = 0L;
	dest->spq_starttime = 0L;
	dest->spq_hold = htonl((ULONG) src->spq_hold);
	dest->spq_nptimeout = htons(src->spq_nptimeout);
	dest->spq_ptimeout = htons(src->spq_ptimeout);
	dest->spq_size = htonl((ULONG) src->spq_size);
	dest->spq_posn = htonl((ULONG) src->spq_posn);
	dest->spq_pagec = htonl((ULONG) src->spq_pagec);
	dest->spq_npages = htonl((ULONG) src->spq_npages);

	dest->spq_cps = src->spq_cps;
	dest->spq_pri = src->spq_pri;
	dest->spq_wpri = htons((USHORT) src->spq_wpri);

	dest->spq_jflags = htons(src->spq_jflags);
	dest->spq_sflags = src->spq_sflags;
	dest->spq_dflags = src->spq_dflags;

	dest->spq_extrn = htons(src->spq_extrn);
	dest->spq_pglim = htons(src->spq_pglim);

	dest->spq_class = htonl(src->spq_class);

	dest->spq_pslot = htonl(-1L);

	dest->spq_start = htonl((ULONG) src->spq_start);
	dest->spq_end = htonl((ULONG) src->spq_end);
	dest->spq_haltat = htonl((ULONG) src->spq_haltat);

	dest->spq_uid = htonl(src->spq_uid);

	strncpy(dest->spq_uname, src->spq_uname, UIDSIZE+1);
	strncpy(dest->spq_puname, src->spq_puname, UIDSIZE+1);
	strncpy(dest->spq_file, src->spq_file, MAXTITLE+1);
	strncpy(dest->spq_form, src->spq_form, MAXFORM+1);
	strncpy(dest->spq_ptr, src->spq_ptr, JPTRNAMESIZE+1);
	strncpy(dest->spq_flags, src->spq_flags, MAXFLAGS+1);
}

int	inittcp(const netid_t hostid)
{
	int			sock;
	SHORT			tcpportnum;
	struct	sockaddr_in	sin;
	struct	servent		*sp;
	struct	protoent	*pp;
	char			*tcp_protoname;

	if  (!((pp = getprotobyname("tcp"))  || (pp = getprotobyname("TCP"))))  {
		print_error($E{No TCP protocol});
		exit(E_NETERR);
	}
	tcp_protoname = pp->p_name;
	endprotoent();

	/* Get port number for this caper */

	if  (!(sp = getservbyname(TSname, tcp_protoname)))  {
		disp_str = TSname;
		print_error($E{No TCP service});
		endservent();
		exit(E_NETERR);
	}
	tcpportnum = sp->s_port;
	endservent();

	if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return  -1;

	/* Set up bits and pieces. */

	sin.sin_family = AF_INET;
	sin.sin_port = tcpportnum;
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = hostid;

	if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(sock);
		return  -1;
	}

	return  sock;
}

int  remenqueue(const netid_t host, const struct spq *jp, const struct pages *pfe, const char *delim, FILE *inf, jobno_t *jn)
{
	int			ch, outbytes, hadwarn = 0;
	unsigned		sequence = 1;
	LONG	char_count;
	struct	client_if	result;
	struct	tcp_data	outb;

	if  (tcpsocket < 0  &&  (tcpsocket = inittcp(host)) < 0)
		return  $E{Cannot create TCP socket};

	/* Splurge out job */

	outb.tcp_code = TCP_STARTJOB;
	outb.tcp_seq = 0;
	outb.tcp_size = 0;
	packjob((struct spq *) outb.tcp_buff, jp);
	if  (!sock_write(tcpsocket, (char *) &outb, sizeof(outb)))
		return  $E{write failure remote job};

	/* If page delimiters, splurge that out */

	if  (jp->spq_dflags & SPQ_PAGEFILE)  {
		struct	pages	*outp = (struct pages *) outb.tcp_buff;
		outb.tcp_code = TCP_PAGESPEC;
		BLOCK_ZERO(outp, sizeof(struct pages));
		outp->delimnum = htonl((ULONG) pfe->delimnum);
		outp->deliml = ntohl((ULONG) pfe->deliml);
		if  (!sock_write(tcpsocket, (char *) &outb, sizeof(outb)))
			return  $E{write failure remote job};
		if  (!sock_write(tcpsocket, delim, (unsigned) pfe->deliml))
			return  $E{write failure remote job};
	}

	outbytes = 0;
	char_count = 0L;
	outb.tcp_code = TCP_DATA;
	while  (char_count < jp->spq_size)  {
		if  ((ch = getc(inf)) == EOF)  {
			if  (!hadwarn)  {
				print_error($E{remote job file truncated});
				hadwarn = 1;
			}
			ch = ' ';
		}
		outb.tcp_buff[outbytes++] = (char) ch;
		if  (outbytes >= CL_SV_BUFFSIZE)  {
			outb.tcp_seq = (unsigned char) sequence;
			outb.tcp_size = htons((unsigned short) outbytes);
			sequence++;
			if  (!sock_write(tcpsocket, (char *) &outb, sizeof(outb)))
				return  $E{write failure remote job};
			outbytes = 0;
		}
		char_count++;
	}
	if  (outbytes > 0)  {
		outb.tcp_seq = (unsigned char) sequence;
		outb.tcp_size = htons((unsigned short) outbytes);
		if  (!sock_write(tcpsocket, (char *) &outb, sizeof(outb)))
		     return  $E{write failure remote job};
	}
	outb.tcp_code = TCP_ENDJOB;
	outb.tcp_seq = 0;
	outb.tcp_size = 0;
	if  (!sock_write(tcpsocket, (char *) &outb, sizeof(outb)))
		return  $E{write failure remote job};

	/* Now see what the other end had to say about it */

	if  (!sock_read(tcpsocket, (char *) &result, sizeof(result)))
		return  $E{write failure remote job};

	if  (result.flag == XTNQ_OK)  {
		*jn = ntohl((ULONG) result.jobnum);
		return  0;
	}

	switch  (result.flag)  {
	default:
	case  XTNR_LOST_SYNC:		return  $E{remote job lost sync};
	case  XTNR_LOST_SEQ:		return  $E{remote job lost seq};
	case  XTNR_UNKNOWN_CLIENT:	return  $E{remote job unknown client};
	case  XTNR_ZERO_CLASS:		return	$E{remote job invalid class code};
	case  XTNR_BAD_PRIORITY:	return  $E{remote job bad priority};
	case  XTNR_BAD_COPIES:		return  $E{remote job bad copies};
	case  XTNR_BAD_FORM:		return  $E{remote job bad form};
	case  XTNR_BAD_PTR:		return  $E{remote job bad ptr};
	case  XTNR_BAD_PF:		return  $E{remote job bad page file};
	case  XTNR_NOMEM_PF:		return  $E{remote job nomem pagefile};
	case  XTNR_CC_PAGEFILE:		return  $E{remote job cannot create pagefile};
	case  XTNR_EMPTYFILE:		return  $E{remote job file empty};
	case  XTNR_FILE_FULL:		return  $E{remote job file full};
	case  XTNR_QFULL:		return  $E{remote job message queue full};
	case  XTNR_PAST_LIMIT:		return  $E{remote job past limit};
	case  XTNR_WARN_LIMIT:
		*jn = ntohl((ULONG) result.jobnum);
		disp_arg[0] = *jn;
		print_error($E{remote job truncated});
		return  0;
	}
}

void	remgoodbye(void)
{
	struct	tcp_data	outb;
	if  (tcpsocket < 0)
		return;
	BLOCK_ZERO(&outb, sizeof(outb));
	outb.tcp_code = TCP_CLOSESOCK;
	sock_write(tcpsocket, (char *) &outb, sizeof(outb));
	close(tcpsocket);
	tcpsocket = -1;
}
