/* dosspwrite.c -- Send completion messages to MSWIN clients

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
#ifdef	NETWORK_VERSION
#include <stdio.h>
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include "defaults.h"
#include "files.h"
#include "errnums.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "incl_sig.h"
#include "spuser.h"
#include "serv_if.h"
#include "client_if.h"
#include "cfile.h"
#include "services.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"



#define	RTIMEOUT	10	/* 10 seconds to hear back from xtnetserv */

SHORT	uaportnum;

extern	uid_t	Realuid, Effuid, Daemuid;

/* For benefit of library routines */

void  nomem()
{
	exit(255);
}

static	int	udpsend(const netid_t hostid, char *msg, const int msglen)
{
	int	sock;
	struct	sockaddr_in	serv_addr;	/* That's me */
	struct	sockaddr_in	cli_addr;	/* That's him */

	BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
	cli_addr.sin_addr.s_addr = hostid;
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = uaportnum;

	BLOCK_ZERO(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_family = AF_INET;

	if  ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		return  0;
	if  (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  {
		close(sock);
		return  0;
	}
	sendto(sock, msg, msglen, 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
	close(sock);
	return  1;
}

static	RETSIGTYPE  asig(int n)
{
	return;			/* Don't do anything just return setting EINTR */
}

static int  roamsend(char *username, char *msg, const int msglen)
{
	int			sockfd;
	int			cnt, doneok = 0;
	struct	hostent		*hp;
	struct	sockaddr_in	xt_addr, my_addr;
	struct	ua_pal		xt_enq;
	struct	ua_asku_rep	xt_rep;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	za;
#endif
	SOCKLEN_T		repl = sizeof(struct sockaddr_in);
	struct	sockaddr_in	reply_addr;

	/* First we chat to our friendly local xtnetserv, and see
	   where it thinks we're logged in.  */

	BLOCK_ZERO(&xt_addr, sizeof(xt_addr));
	BLOCK_ZERO(&my_addr, sizeof(my_addr));
	xt_addr.sin_family = my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	xt_addr.sin_port = uaportnum;

	/* These manouevres are to talk to myself.
	   I think that "localhost" is supposed to be defined
	   but I'm not quite sure. */

	if  (!(hp = gethostbyname("localhost")))  {
		char	myname[256];
		myname[sizeof(myname) - 1] = '\0';
		gethostname(myname, sizeof(myname) - 1);
		if  (!(hp = gethostbyname(myname)))
			return  0;
	}
	xt_addr.sin_addr.s_addr = *(netid_t *) hp->h_addr;

	BLOCK_ZERO(&xt_enq, sizeof(xt_enq));
	xt_enq.uap_op = SV_SV_ASKU;
	strncpy(xt_enq.uap_name, username, UIDSIZE);

	/* Send our enquiry to xtnetserv */

	if  ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return  0;
	if  (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0)  {
		close(sockfd);
		return  0;
	}
	if  (sendto(sockfd, (char *) &xt_enq, sizeof(xt_enq), 0, (struct sockaddr *) &xt_addr, sizeof(xt_addr)) < 0)  {
		close(sockfd);
		return  0;
	}

	/* Wrap alarm handler round recvfrom call in case it isn't running. */

#ifdef	STRUCT_SIG
	za.sighandler_el = asig;
	sigmask_clear(za);
	za.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGALRM, &za, (struct sigstruct_name *) 0);
#else
	signal(SIGALRM, asig);
#endif
	alarm(RTIMEOUT);

	/* Get reply, saving result in cnt */

	cnt = recvfrom(sockfd, (char *) &xt_rep, sizeof(xt_rep), 0, (struct sockaddr *) &reply_addr, &repl);

	/* Unset alarm and mask alarm signal */

	alarm(0);
#ifdef	STRUCT_SIG
	za.sighandler_el = SIG_IGN;
	sigact_routine(SIGALRM, &za, (struct sigstruct_name *) 0);
#else
	signal(SIGALRM, SIG_IGN);
#endif

	/* Close socket - abort if receive failed. */

	close(sockfd);
	if  (cnt <= 0)
		return  0;

	/* Now try sending message to every geyser. */

	for  (cnt = (int) ntohs(xt_rep.uau_n) - 1;  cnt >= 0;  cnt--)
		if  (udpsend(xt_rep.uau_ips[cnt], msg, msglen))
			doneok++;

	return  doneok;
}

/* If we fail to get through, reroute back to despatch program and do an "ordinary" write. */

static void  back_despatch(char *msg)
{
	char	*dispatch = envprocess(MSGDISPATCH);
	char	*cmdline = malloc((unsigned) (strlen(dispatch) + 20));
	FILE	*outp;

	if  (!cmdline)
		nomem();
	sprintf(cmdline, "%s -wx", dispatch);
	if  ((outp = popen(cmdline, "w")))  {
		fputs(msg, outp);
		pclose(outp);
	}
	exit(0);
}

#endif /* Network version */

/* Ye olde main routine. */

MAINFN_TYPE  main(int argc, char **argv)
{
#ifdef	NETWORK_VERSION
	char	*hostname, *username, *portname = GSNETSERV_PORT;
	char	*arg, **wp = (char **) 0;
	int	roamuser = 0, obp = 0, ch;
	char	obuf[SV_CL_MSGBUFF];

	versionprint(argv, "$Revision: 1.2 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	Realuid = getuid();
	Effuid = geteuid();
	init_mcfile();

	/* Slurp message into buffer */

	while  ((ch = getchar()) != EOF)
		if  (obp < sizeof(obuf)-1)
			obuf[obp++] = (char) ch;
	if  (obp <= 0)
		obuf[obp++] = '?';
	obuf[obp] = '\0';

	/* Decode arguments, we retain the old way of doing it for a while.  */

	arg = *++argv;

	if  (!arg)
		return  E_USAGE;

	if  (*arg == '-')  {
		do  {
			arg++;
			while  (*arg)  {
				switch  (*arg++)  {
				case  'r':  roamuser = 1;	continue;
				case  'u':  wp = &username;	break;
				case  'h':  wp = &hostname;	break;
				case  'p':  wp = &portname;	break;
				}
				if  (*arg)
					*wp = arg;
				else  if  (!*++argv)
					return  E_USAGE;
				else
					*wp = *argv;
				break;
			}
		}  while  ((arg = *++argv) && *arg == '-');
	}
	else  if  (argc == 2)  {
		username = arg;
		roamuser = 1;
	}
	else  if  (argc == 3)  {
		hostname = arg;
		username = *++argv;
	}
	else
		return  E_USAGE;

	if  ((Cfile = open_icfile()) == (FILE *) 0)
		exit(E_NOCONFIG);

	/* Figure out port number. This is used to talk to both xtnetserv
	   for roaming users, or Windows clients. */

	if  (isdigit(portname[0]))
		uaportnum = htons((SHORT) atoi(portname));
	else  {
		struct  servent	*sp;
		if  (!(sp = getservbyname(portname, "udp"))  &&  !(sp = getservbyname(portname, "UDP")))
			back_despatch(obuf); /* Doesn't return */
		uaportnum = sp->s_port;
	}

	if  (roamuser)  {
		if  (roamsend(username, obuf, obp))
			return  0;
	}
	else  {
		netid_t	hostid;
		if  (isdigit(hostname[0]))  {
#ifdef	DGAVIION
			struct	in_addr  ina_str;
			ina_str = inet_addr(hostname);
			hostid = ina_str.s_addr;
#else
			hostid = inet_addr(hostname);
#endif
			if  (hostid == -1L)
				back_despatch(obuf);
		}
		else  {
			struct	hostent	*hp;
			if  (!(hp = gethostbyname(hostname)))
				back_despatch(obuf);
			hostid = * (LONG *) hp->h_addr;
		}

		if  (udpsend(hostid, obuf, obp))
			return  0;
	}
	back_despatch(obuf);
#endif /* NETWORK_VERSION */
	return  0;		/* Not really reached */
}
