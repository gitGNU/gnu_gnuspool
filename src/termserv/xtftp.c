/* xtftp.c -- FTP printer driver

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
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef	HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "defaults.h"
#include "incl_net.h"
#include "incl_unix.h"
#include "incl_sig.h"
#include "ecodes.h"
#include "files.h"

char	debug = 0;
char	*progname;
int	timeout = 750, main_timeout = 30000;
netid_t	myhostid;
SHORT	myport;
int	csock;

#define	IBUF_LNG	200

void  pushout(int sock, char *buf, int olen)
{
	while  (olen > 0)  {
		int	nb = write(sock, buf, olen);
		if  (nb < 0)  {
			fprintf(stderr, "%s: Socket write error\n", progname);
			exit(60);
		}
		buf += nb;
		olen -= nb;
	}
}

int  fill_buf(int sock, char *buf)
{
	int	ilng = 0;
	do  {
		int  il = read(sock, buf, 1);
		if  (il <= 0)  {
			fprintf(stderr, "%s: Socket read error\n", progname);
			exit(61);
		}
		ilng++;
	}  while  (*buf++ != '\n'  &&  ilng < IBUF_LNG);

	if  (ilng < IBUF_LNG)
		*buf = '\0';
	return  ilng;
}

void  getcode(int *code, char *buf)
{
	if  (isdigit(buf[0])  &&  isdigit(buf[1]) &&  isdigit(buf[2]) && isspace(buf[3]))
		*code = atoi(buf) / 100;
}

int  getresp(int sock, int ismulti)
{
	int	ilng, lastcode = 0;
	fd_set	ready;
	struct	timeval	tvs;
	char	ibuf[IBUF_LNG];

	FD_ZERO(&ready);
	FD_SET(sock, &ready);
	/* Treat timeout as milliseconds */
	tvs.tv_sec = main_timeout / 1000;
	tvs.tv_usec = (main_timeout % 1000) * 1000;

	if  (debug)
		fprintf(stderr, "Getting response...");

	if  (select(sock+1, &ready, (fd_set *) 0, (fd_set *) 0, &tvs) > 0)  {
		ilng = fill_buf(sock, ibuf);
		if  (debug)  {
			fputs("\n\t", stderr);
			fwrite(ibuf, 1, ilng, stderr);
		}
		getcode(&lastcode, ibuf);
	}
	else  {
		fprintf(stderr, "No response from server\n");
		exit(62);
	}

	if  (!ismulti)  {
		if  (debug  &&  lastcode != 0)
			fprintf(stderr, "Returning code %d\n", lastcode);
		return  lastcode;
	}

	for  (;;)  {

		tvs.tv_sec = timeout / 1000; /* Linux resets it */
		tvs.tv_usec = (timeout % 1000) * 1000;

		if  (select(sock+1, &ready, (fd_set *) 0, (fd_set *) 0, &tvs) <= 0)  {
			if  (debug  &&  lastcode != 0)
				fprintf(stderr, "Returning code %d\n", lastcode);
			return  lastcode;
		}

		ilng = fill_buf(sock, ibuf);

		if  (debug)  {
			putc('\t', stderr);
			fwrite(ibuf, 1, ilng, stderr);
		}

		getcode(&lastcode, ibuf);
	}
}

int  command(int sock, char *a1, char *a2, int multi)
{
	char	*bp;
	char	obuf[100];

	bp = obuf;
	strcpy(bp, a1);
	bp += strlen(a1);
	if  (a2)  {
		int	lng = strlen(a2);
		if  (bp - obuf + lng >= sizeof(obuf))  {
			fprintf(stderr, "%s: Arg %s too long\n", progname, a2);
			exit(100);
		}
		*bp++ = ' ';
		strcpy(bp, a2);
		bp += lng;
	}
	if  (debug)  {
		*bp = '\0';
		fprintf(stderr, "Sending command: %s\n", obuf);
	}

	*bp++ = '\r';
	*bp++ = '\n';
	pushout(sock, obuf, bp - obuf);
	if  (strcmp(a1, "QUIT") == 0)
		return  0;
	return  getresp(sock, multi);
}

RETSIGTYPE  catchabort(int n)
{
#ifdef	UNSAFE_SIGNALS
	signal(n, SIG_IGN);
#endif
	command(csock, "QUIT", (char *) 0, 0);
	exit(E_SIGNAL);
}

void  portcommand(int sock)
{
	int	cnt;
	char	*thing;
	unsigned  char	*cp;
	char	xbuf[40];

	cp = (unsigned char *) &myhostid;
	thing = xbuf;
	for  (cnt = 0;  cnt < sizeof(myhostid);  cnt++)
		thing += sprintf(thing, "%u,", *cp++);
	cp = (unsigned char *) &myport;
	sprintf(thing, "%u,%u", cp[0], cp[1]);
	if  (command(sock, "PORT", xbuf, 0) != 2)  {
		fprintf(stderr, "Port command failed\n");
		exit(71);
	}
}

SHORT  gserv(char *name)
{
	struct  servent	*sp;

	if  (isdigit(name[0]))
		return  htons((SHORT) atoi(name));

	if  (!(sp = getservbyname(name, "tcp"))  &&  !(sp = getservbyname(name, "TCP")))  {
		fprintf(stderr, "%s: Unknown service %s\n", progname, name);
		exit(50);
	}
	return  sp->s_port;
}

netid_t  ghost(char *name)
{
	if  (isdigit(name[0]))  {
		netid_t	hostid;
#ifdef	DGAVIION
		struct	in_addr  ina_str;
		ina_str = inet_addr(name);
		hostid = ina_str.s_addr;
#else
		hostid = inet_addr(name);
#endif
		if  (hostid == -1L)  {
			fprintf(stderr, "%s: Invalid internet address %s\n", progname, name);
			exit(51);
		}
		return  hostid;
	}
	else  {
		struct	hostent  *host;
		if  (!(host = gethostbyname(name)))  {
			fprintf(stderr, "%s: unknown host %s\n", progname, name);
			exit(52);
		}
		return  * (netid_t *) host->h_addr;
	}
}

void  getmyhostid()
{
	char	myname[256];

	myhostid = 0L;
	myname[sizeof(myname) - 1] = '\0';
	gethostname(myname, sizeof(myname) - 1);
	myhostid = ghost(myname);
}

void  initsockets(netid_t hostid, SHORT portnum, int *csock, int *dsock)
{
	int	cs, ds;
	SOCKLEN_T	sfrom;
	struct	sockaddr_in	csin, dsin, from;
#ifdef	SO_REUSEADDR
	int	on = 1;
#endif
	BLOCK_ZERO(&csin, sizeof(csin));
	BLOCK_ZERO(&dsin, sizeof(dsin));
	csin.sin_family = dsin.sin_family = AF_INET;
	csin.sin_addr.s_addr = hostid;
	dsin.sin_addr.s_addr = INADDR_ANY;
	csin.sin_port = portnum;
	dsin.sin_port = 0;

	if  ((cs = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
		fprintf(stderr, "%s: Cannot create control socket\n", progname);
		exit(41);
	}

	if  (connect(cs, (struct sockaddr *) &csin, sizeof(csin)) < 0)  {
		fprintf(stderr, "%s: Cannot connect control socket\n", progname);
		exit(42);
	}

	if  ((ds = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
		fprintf(stderr, "%s: Cannot create data socket\n", progname);
		exit(44);
	}
#ifdef	SO_REUSEADDR
	setsockopt(ds, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
	if  (bind(ds, (struct sockaddr *) &dsin, sizeof(dsin)) < 0)  {
		fprintf(stderr, "%s: Cannot bind data socket\n", progname);
		exit(45);
	}

	if  (listen(ds, 1) < 0)  {
		fprintf(stderr, "%s: Cannot listen on data socket\n", progname);
		exit(46);
	}

	sfrom = sizeof(from);
	if  (getsockname(ds, (struct sockaddr *) &from, &sfrom) < 0)  {
		fprintf(stderr, "%s: Cannot get sock name\n", progname);
		exit(47);
	}

	myport = from.sin_port;
	*csock = cs;
	*dsock = ds;
}

int  accept_data(int dsock)
{
	struct	sockaddr_in	from;
	SOCKLEN_T	fromlen = sizeof(from);
	return  accept(dsock, (struct sockaddr *) &from, &fromlen);
}

int  xmit_stdin(int dsock, char *outfile)
{
	int	cnt, ssock;
	char	xbuf[1024];

	if  ((cnt = fread(xbuf, 1, sizeof(xbuf), stdin)) <= 0)
		return  0;	/* Nothing to send */

	if  (command(csock, "STOR", outfile, 1) > 2)  {
		fprintf(stderr, "Store command failed\n");
		exit(71);
	}
	ssock = accept_data(dsock);

	if  (ssock < 0)  {
		fprintf(stderr, "%s: Cannot do accept\n", progname);
		exit(18);
	}

	do  {
		pushout(ssock, xbuf, cnt);
		cnt = fread(xbuf, 1, sizeof(xbuf), stdin);
	}  while  (cnt > 0);

	close(ssock);
	return  1;
}

MAINFN_TYPE  main(int argc, char **argv)
{
	char	*hostname = (char *) 0, *cport = (char *) 0;
	char	*username = (char *) 0, *password = (char *) 0;
	char	*outfile = (char *) 0, *infile = (char *) 0;
	char	*directory = (char *) 0;
	int	textmode = 0, c;
	int	dsock;
	extern	char	*optarg;
	char	outname[20];
#ifdef	STRUCT_SIG
	struct	sigstruct_name	zs;
#endif

	versionprint(argv, "$Revision: 1.1 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	while  ((c = getopt(argc, argv, "h:H:p:P:du:w:o:i:tbA:T:R:D:")) != EOF)
		switch  (c)  {
		default:
			fprintf(stderr, "%s: invalid option\n", progname);
			return  1;
		case  'h':case  'H':
			hostname = optarg;
			continue;
		case  'p':case  'P':
			cport = optarg;
			continue;
		case  'd':
			debug++;
			continue;
		case  'u':
			username = optarg;
			continue;
		case  'w':
			password = optarg;
			continue;
		case  'o':
			outfile = optarg;
			continue;
		case  'i':
			infile = optarg;
			continue;
		case  't':
			textmode = 1;
			continue;
		case  'b':
			textmode = 0;
			continue;
		case  'D':
			directory = optarg;
			continue;
		case  'T':
			timeout = atoi(optarg);
			continue;
		case  'R':
			main_timeout = atoi(optarg);
			continue;
		case  'A':
			myhostid = ghost(optarg);
			continue;
		}

	if  (!hostname)  {
		fprintf(stderr, "%s: You must give a host name with -h\n", progname);
		return  2;
	}

	if  (myhostid  ==  0L)
		getmyhostid();

	if  (!cport)
		cport = "ftp";

	if  (!outfile)  {
		sprintf(outname, "XTFTP.%.10d", getpid());
		outfile = outname;
	}

	if  (infile  &&  !freopen(infile, "r", stdin))  {
		fprintf(stderr, "%s: Cannot open input file %s\n", progname, infile);
		return  3;
	}

	initsockets(ghost(hostname), gserv(cport), &csock, &dsock);

#ifdef	STRUCT_SIG
	zs.sighandler_el = catchabort;
	sigmask_clear(zs);
	zs.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGTERM, &zs, (struct sigstruct_name *) 0);
#else
	signal(SIGTERM, catchabort);
#endif

	if  (getresp(csock, 1) != 2)  {
		fprintf(stderr, "Unexpected response from server\n");
		return  48;
	}

	if  (username)  {
		int  code = command(csock, "USER", username, 0);
		if  (code  ==  3)  {
			if  (!password)  {
				fprintf(stderr, "You must give a password for user %s\n", username);
				return  4;
			}
			code = command(csock, "PASS", password, 0);
			if  (code != 2)  {
				fprintf(stderr, "%s: Password not accepted\n", progname);
				return  5;
			}
		}
	}

	if  (directory  &&  command(csock, "CWD", directory, 0) != 2)  {
		fprintf(stderr, "%s: cannot select directory %s\n", progname, directory);
		return  7;
	}

	portcommand(csock);
	command(csock, "TYPE", textmode? "A": "I", 0);
	if  (xmit_stdin(dsock, outfile)  &&  getresp(csock, 0) != 2)  {
		fprintf(stderr, "Transmit failure\n");
		return  6;
	}
	command(csock, "QUIT", (char *) 0, 0);
	return  0;
}
