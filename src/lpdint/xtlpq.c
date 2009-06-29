/* xtlpq.c -- program(s) invoked by sp.lpq sp.lprm

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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <errno.h>
#include <pwd.h>
#include "defaults.h"
#include "files.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"

#define	DEFAULT_PORTNAME	"printer"
#define	PR_SHORTDISP	'\3'
#define	PR_LONGDISP	'\4'
#define	PR_REMOVE	'\5'

netid_t	myhostid;

extern	uid_t	Realuid, Effuid, Daemuid;

void	nomem(void)
{
	fprintf(stderr, "Out of memory\n");
	exit(E_NOMEM);
}

static netid_t	host_by_nameoraddr(char *str)
{
	if  (isdigit(str[0]))  {
		netid_t  ina;
#ifdef	DGAVIION
		struct	in_addr	ina_str;
		ina_str = inet_addr(str);
		ina = ina_str.s_addr;
#else
		ina = inet_addr(str);
#endif
		if  (ina == -1L)
			return  (netid_t) 0;
		return  ina;
	}
	else  {
		struct  hostent  *hp = gethostbyname(str);
		return  hp? *(netid_t *) hp->h_addr: (netid_t) 0;
	}
}

static void	assignhost(char *sendhost)
{
	struct	hostent	*hp;
	char	myhost[256];

	if  (sendhost)  {
		if  (!(myhostid = host_by_nameoraddr(sendhost)))  {
			fprintf(stderr, "Invalid host name %s\n", sendhost);
			exit(51);
		}
		if  (isdigit(sendhost[0]) &&  (hp = gethostbyaddr((char *) &myhostid, sizeof(myhostid), AF_INET)))
			strncpy(myhost, hp->h_name, sizeof(myhost)-1);
		else
			strncpy(myhost, sendhost, sizeof(myhost)-1);
		myhost[sizeof(myhost) - 1] = '\0';
	}
	else  {
		gethostname(myhost, sizeof(myhost));
		if  (!(hp = gethostbyname(myhost)))  {
			fprintf(stderr, "Who am I? Please specify my host name with -S\n");
			exit(50);
		}
		myhostid = *(netid_t *) hp->h_addr;
	}
}

/* Set up network stuff */

static int	init_network(netid_t hostid, char *portnamenum)
{
	int	qsock;
	struct	protoent  *pp;
	char	*tcp_protoname;
	short	qportnum, resport;
	struct	sockaddr_in	server, client;
	short	tcpproto;

	/* Get TCP protocol name */

	if  (!((pp = getprotobyname("tcp"))  || (pp = getprotobyname("TCP"))))  {
		fprintf(stderr, "Cannot find tcp proto - aborting\n");
		exit(31);
	}
	tcp_protoname = pp->p_name;
	tcpproto = pp->p_proto;
	if  (isdigit(portnamenum[0]))
		qportnum = htons(atoi(portnamenum));
	else  {
		struct	servent	*sp;
		if  (!(sp = getservbyname(portnamenum, tcp_protoname)))  {
			fprintf(stderr, "Cannot find port %s - aborting\n", portnamenum);
			exit(32);
		}
		qportnum = sp->s_port;
		endservent();
	}

	endprotoent();

	resport = IPPORT_RESERVED - 4;
	BLOCK_ZERO(&server, sizeof(server));
	BLOCK_ZERO(&client, sizeof(client));
	server.sin_family = client.sin_family = AF_INET;
	server.sin_port = qportnum;
	client.sin_port = htons(resport);
	server.sin_addr.s_addr = hostid;
	client.sin_addr.s_addr = myhostid;

	if  ((qsock = socket(PF_INET, SOCK_STREAM, tcpproto)) < 0)  {
		fprintf(stderr, "Cannot open socket\n");
		exit(33);
	}
	while   (bind(qsock, (struct sockaddr *) &client, sizeof(client)) < 0)  {
		if  (errno == EACCES)  {
			fprintf(stderr, "No privilege to bind port\n");
			exit(34);
		}
		if  (--resport <= 0)  {
			fprintf(stderr, "Run out of space to bind ports\n");
			exit(37);
		}
		client.sin_port = htons(resport);
	}
	if  (connect(qsock, (struct sockaddr *) &server, sizeof(server)) < 0)  {
		fprintf(stderr, "Cannot connect socket\n");
		close(qsock);
		exit(36);
	}
	return  qsock;
}

void	pushout(int sockfd, char *buf, int buflen)
{
	int	outb;

	while  (buflen > 0)  {
		if  ((outb = write(sockfd, buf, buflen)) < 0)  {
			fprintf(stderr, "Lost connection to remote\n");
			exit(40);
		}
		buflen -= outb;
		buf += outb;
	}
}

MAINFN_TYPE	main(int argc, char **argv)
{
#ifdef XTLPRM
	extern	int	optind;
	char	*username = (char *) 0, *cp;
#else
	int	longlist = 0;
#endif
	int	sock, lng, ch;
	char	*portname = (char *) 0;
	char	*printername = (char *) 0;
	char	*outhost = (char *) 0;
	char	*sendhost = (char *) 0;
	netid_t	outhostid;
	extern	char	*optarg;
	char	outline[300];

	versionprint(argv, "$Revision: 1.2 $", 1);

	Realuid = getuid();

#ifdef XTLPRM
	while  ((ch = getopt(argc, argv, "p:P:H:S:u:")) != EOF)
#else
	while  ((ch = getopt(argc, argv, "p:P:H:S:l")) != EOF)
#endif
		switch  (ch)  {
		case  '?':
#ifdef XTLPRM
			fprintf(stderr, "Usage: %s -H Outhost -P outport -p printer -u user\n", argv[0]);
#else
			fprintf(stderr, "Usage: %s -H Outhost -P outport -p printer -l\n", argv[0]);
#endif
			return  1;
		case  'P':
			portname = optarg;
			break;
		case  'H':
			outhost = optarg;
			break;
		case  'p':
			printername = optarg;
			break;
		case  'S':
			sendhost = optarg;
			break;
#ifdef XTLPRM
		case  'u':
			username = optarg;
			break;
#else
		case  'l':
			longlist++;
			break;
#endif
		}

	if  (!printername)  {
		fprintf(stderr, "%s: You didn\'t give a printer name\n", argv[0]);
		return  2;
	}
	if  (!outhost)  {
		fprintf(stderr, "%s: You didn\'t give a destination host\n", argv[0]);
		return  3;
	}
	if  (!portname)
		portname = DEFAULT_PORTNAME;
#ifdef XTLPRM
	if  (!username)  {
		struct	passwd	*pw = getpwuid(getuid());
		username = stracpy(pw? pw->pw_name: "root");
	}
#endif
	if  (!(outhostid = host_by_nameoraddr(outhost)))  {
		fprintf(stderr, "%s: Unknown host name %s\n", argv[0], outhost);
		return  5;
	}

	assignhost(sendhost);
	sock = init_network(outhostid, portname);

#ifdef XTLPRM
#ifdef CHARSPRINTF
	sprintf(outline, "%c%s %s", PR_REMOVE, printername, username);
	lng = strlen(outline);
#else
	lng = sprintf(outline, "%c%s %s", PR_REMOVE, printername, username);
#endif
	cp = &outline[lng];
	while  (argv[optind] && (cp - outline) < sizeof(outline) - 20) {
		*cp++ = ' ';
		strcpy(cp, argv[optind]);
		cp += strlen(cp);
		optind++;
	}
	*cp++ = '\n';
	*cp = '\0';
	pushout(sock, outline, cp - outline);
#else	/* Not XTLPRM */
#ifdef CHARSPRINTF
	sprintf(outline, "%c%s\n", longlist? PR_LONGDISP: PR_SHORTDISP, printername);
	lng = strlen(outline);
#else
	lng = sprintf(outline, "%c%s\n", longlist? PR_LONGDISP: PR_SHORTDISP, printername);
#endif
	pushout(sock, outline, lng);
#endif	/* Not XTLPRM */

	while  ((lng = read(sock, outline, sizeof(outline))) > 0)
		fwrite(outline, 1, lng, stdout);
	return  0;
}
