/* xtlhp.c -- SNMP controlled printer driver

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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "incl_net.h"
#include "incl_unix.h"
#include "incl_sig.h"
#include "defaults.h"
#include "files.h"
#include "xtlhpdefs.h"

extern int	ParseMacroFile(const char *);
extern int	exec_script(struct command *);
extern void	init_define(const char *, const char *);
extern int	yyparse(void);

struct	command	*Control_list;
int	debug;
FILE	*Cfile;

char	*community = "public";
double	udptimeout = 1.0;
struct	sockaddr_in  snmp_serv, snmp_cli;
int	sock_fd, snmpsock, snmp_next;

extern	FILE	*yyin;

/* Flush routine for when asked from script.  */

void	do_flush(void)
{
	static	char	esc_seq[] = "\033E";

	/* We don't use sock_write or we could end up calling
	   ourselves recursively.  */

	if  (sock_fd >= 0)
		write(sock_fd, esc_seq, sizeof(esc_seq) - 1);
}

RETSIGTYPE	catchsig(int sig)
{
#ifdef	UNSAFE_SIGNALS
	signal(sig, SIG_IGN);
#endif
	do_flush();
	exit(0);
}

void	sock_write(char *buffer, int nbytes)
{
	int	outbytes, ecode;

	while  (nbytes > 0)  {
		outbytes = write(sock_fd, buffer, nbytes);
		if  (outbytes < 0)  {
			if  ((ecode = exec_script(Control_list)) >= 0)
				exit(ecode);
			close(sock_fd);
			exit(EXIT_DEVERROR);
		}
		buffer += outbytes;
		nbytes -= outbytes;
	}
}

netid_t  get_hostorip(const char *name)
{
	netid_t	hid;
	
	if  (isdigit(name[0]))  {
#ifdef	DGAVIION
		struct	in_addr  ina_str;
		ina_str = inet_addr(name);
		hid = ina_str.s_addr;
#else
		hid = inet_addr(name);
#endif
		if  (hid == -1L)  {
			fprintf(stderr, "Invalid internet address %s\n", name);
			exit(EXIT_USAGE);
		}
	}
	else  {
		struct	hostent  *host;
		if  (!(host = gethostbyname(name)))  {
			fprintf(stderr, "unknown host %s\n", name);
			exit(EXIT_USAGE);
		}
		hid = * (netid_t *) host->h_addr;
	}
	return  hid;
}

int	get_portornum(const char *name)
{
	struct	servent	 *sp;

	if  (isdigit(name[0]))
		return  htons(atoi(name));

	if  (!(sp = getservbyname(name, "udp")))  {
		fprintf(stderr, "Cannot find service %s\n", name);
		exit(EXIT_SNMPERROR);
	}
	return  sp->s_port;
}

void  sock_init(const netid_t hostid, const netid_t myhostid, const int telnetport, const int snmpport)
{
	struct	sockaddr_in  tnsin;
	struct	sockaddr_in  clisin;

	BLOCK_ZERO(&tnsin, sizeof(tnsin));
	BLOCK_ZERO(&clisin, sizeof(clisin));
	tnsin.sin_family = clisin.sin_family = snmp_serv.sin_family = snmp_cli.sin_family = AF_INET;
	tnsin.sin_port = (SHORT) telnetport;
	snmp_serv.sin_port = (SHORT) snmpport;
	tnsin.sin_addr.s_addr = snmp_serv.sin_addr.s_addr = hostid;
	clisin.sin_addr.s_addr = snmp_cli.sin_addr.s_addr = myhostid;

	/* Connect to telnet socket */
	
	if  ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
		perror("Telnet socket create");
		exit(EXIT_DEVERROR);
	}
	if  (myhostid  &&  bind(sock_fd, (struct sockaddr *) &clisin, sizeof(clisin)) < 0)  {
		perror("Telnet socket bind");
		exit(EXIT_DEVERROR);
	}
	if  (connect(sock_fd, (struct sockaddr *) &tnsin, sizeof(tnsin)) < 0)  {
		perror("Telnet socket connect");
		exit(EXIT_DEVERROR);
	}

	/* Set up snmp socket */

	if  ((snmpsock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)  {
		perror("Snmp socket create");
		exit(EXIT_SYSERROR);
	}
	if  (bind(snmpsock, (struct sockaddr *) &snmp_cli, sizeof(snmp_cli)) < 0)  {
		perror("Snmp socket bind");
		exit(EXIT_SYSERROR);
	}
}
	
MAINFN_TYPE	main(int argc, char **argv)
{
	int	ecode, c, inp, portnum, snmpport, snmpdebug = 0;
	char	*hostname = (char *) 0, *myhostname = (char *) 0, *port_text = "9100", *snmp_port = "snmp", *progname;
	char	*configname = (char *) 0, *ctrlname = (char *) 0, *logname = (char *) 0;
	ULONG	blksize = DEFAULT_BLKSIZE;
	ULONG	bytecount = 1;
	netid_t	hostid = 0, myhostid = 0;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	z;
#endif
	char	buffer[1024];
	extern	char	*optarg;

	versionprint(argv, "$Revision: 1.1 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	while  ((c = getopt(argc, argv, "h:H:p:P:d:D:f:c:b:l:C:T:S:N")) != EOF)  {
		char	*cp;
		switch  (c)  {
		default:
			fprintf(stderr, "Invalid option to %s\n", progname);
			return  EXIT_USAGE;
		case  'N':	snmp_next = 1;		continue;
		case  'h':	hostname = optarg;	continue;
		case  'H':	myhostname = optarg;	continue;
		case  'p':
		case  'P':	port_text = optarg;	continue;
		case  'C':	community = optarg;	continue;
		case  'T':	udptimeout = atof(optarg);	continue;
		case  'S':	snmp_port = optarg;	continue;
		case  'd':	debug = atoi(optarg);	continue;
		case  'D':	snmpdebug = 1;	debug = atoi(optarg);		continue;
		case  'f':	configname = optarg;	continue;
		case  'c':	ctrlname = optarg;	continue;
		case  'l':	logname = optarg;	continue;
		case  'b':
			blksize = atol(optarg);
			for  (cp = optarg;  isdigit(*cp);  cp++)
				;
			if  (*cp)  {
				switch  (tolower(*cp))  {
				case  'b':
					blksize <<= 9;
					break;
				case  'k':
					blksize <<= 10;
					break;
				case  'm':
					blksize <<= 20;
					break;
				case  'g':
					blksize <<= 30;
					break;
				}
			}
			if  (blksize == 0)
				blksize = DEFAULT_BLKSIZE;
			continue;
		}
	}

	/* Checks and assigns.  */

	if  (logname  &&  strcmp(logname, "-") != 0)
		freopen(logname, "a", stderr);

	if  (!hostname)  {
		fprintf(stderr, "%s: No host name supplied\n", progname);
		return  EXIT_USAGE;
	}

	hostid = get_hostorip(hostname);
	if  (myhostname)
		myhostid = get_hostorip(myhostname);
	portnum = get_portornum(port_text);
	snmpport = get_portornum(snmp_port);
	endhostent();
	endservent();

	init_define(HOSTNAME_NAME, hostname);
	init_define(PORTNAME_NAME, port_text);

	/* Get config file, not fatal error if we can't find it.  */

	if  (configname)  {
		if  (!ParseMacroFile(configname)  &&  debug > 0)
			fprintf(stderr, "Cannot open config file %s\n", configname);
	}
	else  {
		char	*progdir = envprocess(IDATADIR);
		char	*defcname = DEFAULT_CONFIGNAME;
		char	*filep = malloc((unsigned) (strlen(progdir) + strlen(defcname) + 2));
		if  (!filep)
			nomem();
		sprintf(filep, "%s/%s", progdir, defcname);
		if  (!ParseMacroFile(filep)  &&  debug > 0)
			fprintf(stderr, "Cannot open default config file %s\n", filep);
		free(progdir);
		free(filep);
	}

	/* Get control file */

	if  (ctrlname)  {
		if  (!(yyin = fopen(ctrlname, "r")))  {
			fprintf(stderr, "Cannot open control file %s\n", ctrlname);
			return  EXIT_USAGE;
		}
	}
	else  {
		char	*progdir = envprocess(IDATADIR);
		char	*defcname = DEFAULT_CTRLNAME;
		char	*filep = malloc((unsigned) (strlen(progdir) + strlen(defcname) + 2));
		if  (!filep)
			nomem();
		sprintf(filep, "%s/%s", progdir, defcname);
		if  (!(yyin = fopen(filep, "r")))  {
			fprintf(stderr, "Cannot open default control file %s\n", ctrlname);
			return  EXIT_USAGE;
		}
		free(progdir);
		free(filep);
	}
	if  (yyparse())  {
		fprintf(stderr, "Aborted due to syntax error(s)\n");
		return  EXIT_USAGE;
	}

	fclose(yyin);

	sock_init(hostid, myhostid, portnum, snmpport);

	/* If control file exits or we're just debugging the script/SNMP, then exit straightaway.  */

	if  ((ecode = exec_script(Control_list)) >= 0  ||  snmpdebug)
		return  ecode < 0? 0: ecode;

	/* If stdin has nothing in it, then don't bother to do
	   anything else, just exit.  It was a null input to
	   check what was going on REMEMBER WE READ THE FIRST
	   CHAR OF INPUT INTO "c".  */

	if  ((c = getchar()) == EOF)
		return  0;

#ifdef	STRUCT_SIG
	z.sighandler_el = catchsig;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGHUP, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGINT, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGQUIT, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGTERM, &z, (struct sigstruct_name *) 0);
#else
	signal(SIGHUP, catchsig);
	signal(SIGINT, catchsig);
	signal(SIGQUIT, catchsig);
	signal(SIGTERM, catchsig);
#endif

	/* Now do the business.  We copied the first character of the input into c.  */

	inp = 0;
	buffer[inp++] = (char) c;

	while  ((c = getchar()) != EOF)  {
		buffer[inp++] = (char) c;

		if  (inp >= sizeof(buffer))  {
			sock_write(buffer, inp);
			bytecount += inp;
			inp = 0;
			if  (bytecount >= blksize)  {
				bytecount = 0;
				if  ((ecode = exec_script(Control_list)) >= 0)  {
					close(sock_fd);
					return  ecode;
				}
			}
		}
	}

	/* Last bit */

	if  (inp >= 0)
		sock_write(buffer, inp);
	close(sock_fd);
	return  0;
}
