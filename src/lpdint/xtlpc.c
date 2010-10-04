/* xtlpc.c -- driver to output using LPD protocol

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
#include <errno.h>
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "defaults.h"
#include "files.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "incl_sig.h"
#include "lpctypes.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"

#define	PIDFILE	".xtlpcpid"

static	char	*myhostname;
netid_t	myhostid;
static	int	nonull;

int		loops = 3, unresport = 0;
int		had_alarm = 0;
unsigned	loopwait = 1;
float		lingertime = 0;
unsigned	input_timeout = 5, output_timeout = 5, send_retries = 0;

extern	uid_t	Realuid, Effuid, Daemuid;

extern int  parsecf(FILE *);
extern int  evalcond(struct condition *);

RETSIGTYPE  cleanupfiles(int signum)
{
	struct	varname	*vp;

	vp = lookuphash("CFILE");
	if  (vp->vn_value  && vp->vn_value[0])
		unlink(vp->vn_value);
	vp = lookuphash("DFILE");
	if  (vp->vn_value  && vp->vn_value[0])
		unlink(vp->vn_value);
	unlink(PIDFILE);
	if  (signum != 0)
		exit(200);
}

RETSIGTYPE  alarmhandler(int signum)
{
#ifdef	UNSAFE_SIGNALS
	signal(signum, alarmhandler);
#endif
	had_alarm++;
}

static netid_t  host_by_nameoraddr(char *str)
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

static void  envassign(char *name, char *defvalue)
{
	struct	varname	*vp;

	vp = lookuphash(name);
	if  (vp->vn_value)  {
		if  (vp->vn_value[0])
			return;
		free(vp->vn_value);
	}
	vp->vn_value = stracpy(defvalue); /* Must be free-able */
}

static void  assignenvs(char *sendhost)
{
	extern	char	**environ;
	char	**ep;
	struct	hostent	*hp;
	char	myhost[256];

	for  (ep = environ;  *ep;  ep++)  {
		char	*ec, *eqp;
		struct	varname	*vp;
		ec = stracpy(*ep);
		if  ((eqp = strchr(ec, '=')))  {
			*eqp = '\0';
			vp = lookuphash(ec);
			if  (vp->vn_value)
				free(vp->vn_value);
			vp->vn_value = stracpy(eqp+1);
		}
		free(ec);
	}

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
			fprintf(stderr, "Who am I?\n");
			exit(50);
		}
		myhostid = *(netid_t *) hp->h_addr;
	}
	myhostname = stracpy(myhost);
}

static void  reset_envirvar(char *name, char *value)
{
	struct	varname	*vp = lookuphash(name);
	if  (vp->vn_value)
		free(vp->vn_value);
	vp->vn_value = stracpy(value);
}

FILE *generate_cdfname(int startseq, char *vname, char **oldval)
{
	struct	varname	*vp = lookuphash(vname);
	int	ch, cnt;
	FILE	*result;

	if  (!vp->vn_value || vp->vn_value[0] == '\0')  {
		fprintf(stderr, "%s not defined in control file\n", vname);
		exit(10);
	}
	if  ((int) strlen(vp->vn_value) > 70 || strchr(vp->vn_value, '/'))  {
		fprintf(stderr, "Invalid value \'%s\' for %s\n", vp->vn_value, vname);
		exit(11);
	}
	for  (ch = 'A';  ch <= 'Z';  ch++)  {
		for  (cnt = startseq;  cnt < 1000;  cnt++)  {
			int	fd;
			char	outname[160];
			sprintf(outname, vp->vn_value, ch, cnt);
			if  ((fd = open(outname, O_CREAT|O_EXCL|O_WRONLY, 0666)) < 0)  {
				if  (errno == EACCES)  {
					fprintf(stderr, "No write access to spool directory\n");
					exit(12);
				}
				continue;
			}
			if  (!(result = fdopen(fd, "w")))  {
				fprintf(stderr, "Trouble opening %s\n", outname);
				exit(13);
			}
			*oldval = vp->vn_value;
			vp->vn_value = stracpy(outname);
			return  result;
		}
		startseq = 1;
	}
	fprintf(stderr, "Unable to allocate a file name for %s\n", vname);
	exit(14);
}

void  restore_cdfname(char *vname, char *oldval)
{
	struct  varname  *vp = lookuphash(vname);
	if  (vp->vn_value)  {
		unlink(vp->vn_value);
		free(vp->vn_value);
	}
	vp->vn_value = oldval;
}

static char *conv_line(char *str, int *length)
{
	char	*result = malloc((unsigned) (strlen(str) + 10));
	char	*cp, *rp;

	if  (!result)
		nomem();
	rp = result;
	cp = str;

	while  (*cp)  {
		if  (*cp == '^')  {
			if  (*++cp == '^')
				*rp++ = '^';
			else
				*rp++ = *cp++ & 0x1f;
		}
		else  if  (*cp == '%')  {
			char	*np;
			struct	stat	sbuf;
			char	nbuf[20];
			do  cp++;
			while  (*cp == ' ');
			if  (!(np = strchr(cp, ' ')))
				continue;
			*np = '\0';
			if  (stat(cp, &sbuf) < 0)  {
				*np = ' ';
				continue;
			}
			*np = ' ';
			cp = np;
			sprintf(nbuf, "%ld", nonull && sbuf.st_size == 0? 1: sbuf.st_size);
			np = nbuf;
			do  *rp++ = *np++;
			while  (*np);
		}
		else
			*rp++ = *cp++;
	}

	*rp = '\0';
	*length = rp - result;
	return  result;
}

static int  getseq()
{
	int	fd, result;
	char	buf[100];

	if  ((fd = open(".seq", O_RDWR)) < 0)  {
		if  ((fd = open(".seq", O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0)  {
			fprintf(stderr, "Cannot create sequence file\n");
			exit(20);
		}
		result = 1;
	}
	else  {
		int	inb = read(fd, buf, sizeof(buf));
		if  (inb <= 1)
			result = 1;
		result = atoi(buf) + 1;
		lseek(fd, 0L, 0);
	}
	sprintf(buf, "%d\n", result);
	Ignored_error = write(fd, buf, strlen(buf));
	close(fd);
	return  result;
}

static void  savepid()
{
	int	fd;
	char	buf[40];

	if  ((fd = open(PIDFILE, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0)
		return;
	sprintf(buf, "%d\n", getpid());
	Ignored_error = write(fd, buf, strlen(buf));
	close(fd);
}

/* Set up network stuff */

static int  init_network(netid_t hostid)
{
	int	qsock;
	struct	protoent  *pp;
	struct	varname	*vp;
	char	*tcp_protoname;
	short	qportnum, resport;
	struct	sockaddr_in	server, client;
	short	tcpproto;

	/* First get port name/number */

	vp = lookuphash("PORTNAME");
	if  (!vp->vn_value  ||  !vp->vn_value[0])  {
		fprintf(stderr, "No port name/number defined in control file - aborting\n");
		exit(30);
	}

	/* Get TCP protocol name */

	if  (!((pp = getprotobyname("tcp"))  || (pp = getprotobyname("TCP"))))  {
		fprintf(stderr, "Cannot find tcp proto - aborting\n");
		exit(31);
	}
	tcp_protoname = pp->p_name;
	tcpproto = pp->p_proto;
	if  (isdigit(vp->vn_value[0]))
		qportnum = htons(atoi(vp->vn_value));
	else  {
		struct	servent	*sp;
		if  (!(sp = getservbyname(vp->vn_value, tcp_protoname)))  {
			fprintf(stderr, "Cannot find port %s - aborting\n", vp->vn_value);
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
	if  (!unresport)  {
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
	}

	do  {
		if  (connect(qsock, (struct sockaddr *) &server, sizeof(server)) >= 0)
			goto  doneconn;
		sleep(loopwait);
	}  while  (--loops > 0);

	fprintf(stderr, "Cannot connect socket\n");
	close(qsock);
	exit(36);

 doneconn:

	if  (lingertime != 0.0)  {
		struct	linger	soko;
		soko.l_linger = (int) (lingertime * 100.0 + .5);
		soko.l_onoff = 1;
		setsockopt(qsock, SOL_SOCKET, SO_LINGER, &soko, sizeof(soko));
	}

	return  qsock;
}

int  pushout(int sockfd, char *buf, int buflen)
{
	int	outb;

	while  (buflen > 0)  {
		alarm(output_timeout);
		if  ((outb = write(sockfd, buf, buflen)) < 0)  {
			if  (errno == EINTR)  {
				fprintf(stderr, "Output timeout\n");
				return  1;
			}
			fprintf(stderr, "Lost connection to remote\n");
			cleanupfiles(0);
			exit(40);
		}
		alarm(0);
		buflen -= outb;
		buf += outb;
	}
	return  0;
}

int  pushfile(int sockfd, char *filename)
{
	int	infd, inb;
	long	had = 0;
	char	outbuf[1024];

	if  ((infd = open(filename, O_RDONLY)) < 0)  {
		fprintf(stderr, "Lost file %s??\n", filename);
		cleanupfiles(0);
		exit(41);
	}
	while  ((inb = read(infd, outbuf, sizeof(outbuf))) > 0)  {
		had += inb;
		if  (pushout(sockfd, outbuf, inb))
			return  1;
	}
	if  (nonull  &&  had <= 0)  {
		outbuf[0] = '\0';
		if  (pushout(sockfd, outbuf, 1))
			return  1;
	}
	close(infd);
	return  0;
}

int  pullin(int sockfd, char *cbuf, int cbuflen)
{
	char	inbuf;

	while  (cbuflen > 0)  {
		int	nb;
		alarm(input_timeout);
		if  ((nb = read(sockfd, &inbuf, sizeof(inbuf))) <= 0)  {
			if  (nb == 0)
				fprintf(stderr, "EOF\n");
			else  {
				if  (errno == EINTR)  {
					fprintf(stderr, "Input timeout\n");
					return  1;
				}
				perror("Ack read");
			}
			fprintf(stderr, "Trouble reading ack packet\n");
			exit(42);
		}
		alarm(0);
		if  (inbuf != *cbuf)  {
			fprintf(stderr, "Error returned from remote - %d\n", inbuf);
			exit(43);
		}
		cbuflen--;
		cbuf++;
	}
	return  0;
}

MAINFN_TYPE  main(int argc, char **argv)
{
	struct	ctrltype	*cp;
	int	ch, sequence, sock, pass1 = 0;
	char	*ctrlfile = (char *) 0;
	char	*outhost = (char *) 0;
	char	*sendhost = (char *) 0;
	char	*fifoname = (char *) 0;
	char	*printername = (char *) 0;
	char	*save_cfile = (char *) 0,
		*save_dfile = (char *) 0;
	netid_t	outhostid;
	FILE	*wotfile, *cfile, *dfile, *ff = (FILE *) 0;
	struct	varname	*vp;
	extern	int	optind;
	extern	char	*optarg;
#ifdef	STRUCT_SIG
	struct	sigstruct_name  zc;
#endif
	Realuid = getuid();

	versionprint(argv, "$Revision: 1.2 $", 1);

	while  ((ch = getopt(argc, argv, "f:H:S:NF:P:l:L:s:UI:O:R:")) != EOF)
		switch  (ch)  {
		case  '?':
			fprintf(stderr, "Usage: %s -f Ctrlfile -H Outhost\n", argv[0]);
			return  1;
		case  'f':
			ctrlfile = optarg;
			break;
		case  'F':
			fifoname = optarg;
			break;
		case  'H':
			outhost = optarg;
			break;
		case  'S':
			sendhost = optarg;
			break;
		case  'P':
			printername = optarg;
			break;
		case  'N':
			nonull++;
			break;
		case  'l':
			loops = atoi(optarg);
			break;
		case  'L':
			loopwait = (unsigned) atoi(optarg);
			break;
		case  's':
			lingertime = atof(optarg);
			break;
		case  'U':
			unresport = 1;
			break;
		case  'I':
			input_timeout = (unsigned) atoi(optarg);
			break;
		case  'O':
			output_timeout = (unsigned) atoi(optarg);
			break;
		case  'R':
			send_retries = (unsigned) atoi(optarg);
			break;
		}

	if  (!ctrlfile)  {
		fprintf(stderr, "%s: You didn\'t give a control file\n", argv[0]);
		return  2;
	}
	if  (!outhost)  {
		fprintf(stderr, "%s: You didn\'t give a destination host\n", argv[0]);
		return  3;
	}

	if  (argv[optind])  {
		if  (fifoname)  {
			fprintf(stderr, "%s: You cannot specify a FIFO and an input file\n", argv[0]);
			return  17;
		}
		if  (!freopen(argv[optind], "r", stdin))  {
			fprintf(stderr, "Cannot reopen stdin\n");
			return  4;
		}
	}

	if  (!(outhostid = host_by_nameoraddr(outhost)))  {
		fprintf(stderr, "%s: Unknown host name %s\n", argv[0], outhost);
		return  5;
	}

	if  (!(wotfile = fopen(ctrlfile, "r")))  {
		fprintf(stderr, "%s: cannot open %s\n", argv[0], ctrlfile);
		return  6;
	}

	assignenvs(sendhost);

	/* Assign parameters if not defined by Xi-Text in the environment */

	envassign("SPOOLPTR", "lp");
	envassign("SPOOLHDR", "stdin");
	envassign("SPOOLUSER", "0");
	envassign("SPOOLJUNAME", "root");
	envassign("SPOOLPUNAME", "root");
	envassign("SPOOLFORM", "standard");
	envassign("SPOOLJOB", "0");
	envassign("SPOOLHOST", myhostname);
	envassign("SPOOLCPS", "1");

	/* Set up my names. Ignore the environment */

	reset_envirvar("MYHOST", myhostname);
	reset_envirvar("DESTHOST", outhost);

	/* Possibly override printer name */

	if  (printername)
		reset_envirvar("SPOOLPTR", printername);

	if  (parsecf(wotfile))  {
		fprintf(stderr, "Aborting due to control file errors\n");
		return  7;
	}
	fclose(wotfile);

	vp = lookuphash("XTLPCSPOOL");
	if  (!vp->vn_value)  {
		fprintf(stderr, "No client spool directory defined\n");
		return  8;
	}

	if  (chdir(vp->vn_value) < 0)  {
		fprintf(stderr, "Cannot select spool directory\n");
		return  9;
	}

	/* Look for fifo now if applicable after we've changed dir.  */

	if  (fifoname)  {
		struct	stat	sbuf;
		if  (stat(fifoname, &sbuf) < 0)  {
			fprintf(stderr, "%s: cannot find FIFO %s\n", argv[0], fifoname);
			return  15;
		}
		if  ((sbuf.st_mode & S_IFMT) != S_IFIFO)  {
			fprintf(stderr, "%s: %s is not a FIFO\n", argv[0], fifoname);
			return  16;
		}
		/* Turn ourself into a child process */
		if  (fork() != 0)
			return  0;
	}
	savepid();

#ifdef	STRUCT_SIG
	zc.sighandler_el = cleanupfiles;
	sigmask_clear(zc);
	zc.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGTERM, &zc, (struct sigstruct_name *) 0);
	sigact_routine(SIGINT, &zc, (struct sigstruct_name *) 0);
	sigact_routine(SIGQUIT, &zc, (struct sigstruct_name *) 0);
	sigact_routine(SIGHUP, &zc, (struct sigstruct_name *) 0);
	zc.sighandler_el = alarmhandler;
	sigact_routine(SIGALRM, &zc, (struct sigstruct_name *) 0);
#else
	signal(SIGTERM, cleanupfiles);
	signal(SIGINT, cleanupfiles);
	signal(SIGQUIT, cleanupfiles);
	signal(SIGHUP, cleanupfiles);
	signal(SIGALRM, cleanupfiles);
#endif

	sock = init_network(outhostid);

	for  (;;)  {

		sequence = getseq();
		cfile = generate_cdfname(sequence, "CFILE", &save_cfile);
		dfile = generate_cdfname(sequence, "DFILE", &save_dfile);

		/* Copy stdin, or from FIFO to dfile */

		if  (fifoname)  {
			if  (!(ff = fopen(fifoname, "r")))  {
				cleanupfiles(0);
				return  0;
			}
			while  ((ch = getc(ff)) != EOF)
				putc(ch, dfile);
		}
		else  while  ((ch = getchar()) != EOF)
			putc(ch, dfile);

		fclose(dfile);

		for  (cp = card_list;  cp;  cp = cp->ctrl_next)  {
			char	*str;
			int	repnum = 1, cnt;
			struct	condition  *cc = cp->ctrl_cond;

			if  (cc  &&  cc->cond_type != COND_MULT  &&  !evalcond(cc))
				continue;

			str = expandvars(cp->ctrl_string);

			if  (cc  &&  cc->cond_type == COND_MULT  &&  cc->cond_var  &&  cc->cond_var->vn_value)
				repnum = atoi(cc->cond_var->vn_value);

			for  (cnt = 0;  cnt < repnum;  cnt++)
				fprintf(cfile, "%s\n", str);
			free(str);
		}

		fclose(cfile);

	restart:

		for  (cp = proto_list;  cp;  cp = cp->ctrl_next)  {
			char	*str = expandvars(cp->ctrl_string), *nstr;
			int	linelen;

			switch  (cp->ctrl_action)  {
			default:
				fprintf(stderr, "Unknown action???\n");
				break;

			case  CT_SENDLINEONCE:
				if  (pass1)
					break;
			case  CT_SENDLINE:
				nstr = conv_line(str, &linelen);
				if  (pushout(sock, nstr, linelen))
					goto  io_error;
				free(nstr);
			case  CT_NOP:
				break;

			case  CT_RECVLINEONCE:
				if  (pass1)
					break;
			case  CT_RECVLINE:
				nstr = conv_line(str, &linelen);
				if  (pullin(sock, nstr, linelen))
					goto  io_error;
				free(nstr);
				break;

			case  CT_SENDFILE:
				if  (!pushfile(sock, str))
					break;
			io_error:
				if  (send_retries == 0)  {
					fprintf(stderr, "Aborting due to network I/O timeouts\n");
					return  201;
				}
				send_retries--;
				fprintf(stderr, "Retrying....\n");
				close(sock);
				pass1 = 0; /* For FIFO case */
				sock = init_network(outhostid);
				goto  restart;
			}
			free(str);
		}

		if  (!fifoname)
			break;

		pass1++;

		/* Restore the CFILE/DFILE variables to their original state */

		restore_cdfname("DFILE", save_dfile);
		restore_cdfname("CFILE", save_cfile);
		if  (ff)
			fclose(ff);
	}
	cleanupfiles(0);

	return  0;
}
