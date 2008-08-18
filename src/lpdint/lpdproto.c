/* lpdproto.c -- LPD protocol emulation for xtlpd

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
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include "incl_unix.h"
#include "lpdtypes.h"

#define	PR_CHECK	'\1'
#define	PR_RECEIVE	'\2'
#define	PR_SHORTDISP	'\3'
#define	PR_LONGDISP	'\4'
#define	PR_REMOVE	'\5'

#define	RECV_CLEANUP	'\1'
#define	RECV_CFILE	'\2'
#define	RECV_DFILE	'\3'

#define	LINBUF_SIZE	150
#define	XBUFSIZE	1024
#define	FILEN_SIZE	40

char	cfilename[FILEN_SIZE],
	dfilename[FILEN_SIZE];

extern	int	debug_level;

extern void	printfiles(char *);

/* Hideous and revolting algorithm copied from lpd.  */

static void	cleanupfiles(void)
{
	if  (cfilename[0])  {
		if  (debug_level > 1)
			fprintf(stderr, "Unlink cfile \'%s\'\n", cfilename);
		tf_unlink(cfilename, 1);
		cfilename[0] = '\0';
	}
	if  (dfilename[0])  {
		do  {
			do  {
				if  (debug_level > 1)
					fprintf(stderr, "Unlink dfile \'%s\'\n", dfilename);
				tf_unlink(dfilename, 1);
			}  while    (dfilename[2]-- > 'A');
			dfilename[2] = 'z';
		}  while  (dfilename[0]-- > 'd');
		dfilename[0] = '\0';
	}
}

static void	acknowledge(int sockfd)
{
	static	char	ackcode = '\0';
	write(sockfd, &ackcode, sizeof(ackcode));
}

static void	recvabort(int sockfd, char *message)
{
	static	char	ecode = '\1';
	cleanupfiles();
	write(sockfd, &ecode, sizeof(ecode));
	fprintf(stderr, "%s\n", message);
	exit(100);
}

static int	readsockline(int sockfd, char *inbuf)
{
	char	inch;
	int	inb, cnt = 0;

	for  (;;)  {
		if  ((inb = read(sockfd, &inch, sizeof(inch))) <= 0)  {
			if  (inb < 0)
				recvabort(sockfd, "Lost connection");
			return  0;
		}
		if  (inch == '\n')
			break;
		if  (cnt < LINBUF_SIZE - 1)
			inbuf[cnt++] = inch;
	}
	inbuf[cnt] = '\0';
	return  1;
}

static int	readfile(int sockfd, char * filename, int size)
{
	int	outfd, bytesleft = size;
	char	buf[XBUFSIZE];

	if  ((outfd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0660)) < 0)  {
		perror(filename);
		recvabort(sockfd, "Cannot create incoming file");
		return  0;
	}

	acknowledge(sockfd);

	while  (bytesleft > 0)  {
		int	amt = XBUFSIZE, amtleft, inb;
		char	*bp = buf;
		if  (amt > bytesleft)
			amt = bytesleft;
		amtleft = amt;
		do  {
			inb = read(sockfd, bp, amtleft);
			if  (inb <= 0)
				recvabort(sockfd, "Connection abort in readfile");
			amtleft -= inb;
			bp += inb;
		}  while  (amtleft > 0);
		if  (write(outfd, buf, amt) != amt)  {
			close(outfd);
			recvabort(sockfd, "Write failure (disc full?) in readfile\n");
		}
		bytesleft -= amt;
	}
	if  (close(outfd) < 0)  {
		perror("Close");
		recvabort(sockfd, "Close failure (disc full?) in readfile\n");
		return  0;
	}
	if  (read(sockfd, buf, 1) <= 0)
		recvabort(sockfd, "Connection abort(2) in readfile");
	if  (buf[0])  {
		tf_unlink(filename, 0);
		return  0;
	}
	acknowledge(sockfd);
	return  1;
}

static void	recvjob(int sockfd)
{
	char	*cp, *sp;
	int	size;
	char	linbuf[LINBUF_SIZE];

	acknowledge(sockfd);

	for  (;;)  {
		if  (!readsockline(sockfd, linbuf))
			return;
		cp = linbuf;
		if  (debug_level > 1)
			fprintf(stderr, "Had a job line %d: %s\n", *cp, cp+1);
		switch  (*cp++)  {
		case  RECV_CLEANUP:
			cleanupfiles();
			continue;
		case  RECV_CFILE:
			size = 0;
			while  (isdigit(*cp))
				size = size * 10 + *cp++ - '0';
			while  (isspace(*cp))
				cp++;
			strncpy(cfilename, cp, FILEN_SIZE-1);
			if  (!readfile(sockfd, cfilename, size))
				cleanupfiles();
			continue;
		case  RECV_DFILE:
			size = 0;
			while  (isdigit(*cp))
				size = size * 10 + *cp++ - '0';
			while  (isspace(*cp))
				cp++;
			if  ((sp = strrchr(cp, '/')))
				cp = sp + 1;
			strncpy(dfilename, cp, FILEN_SIZE-1);
			readfile(sockfd, dfilename, size);
			continue;
		}
		recvabort(sockfd, "Protocol failure in recvjob");
	}
}

void	lassign(struct varname *varr, const char *str)
{
	if  (varr->vn_value)
		free(varr->vn_value);
	varr->vn_value = stracpy(str);
}

void	setexport(struct varname *varr, char *str)
{
	char	*envv;
	lassign(varr, str);
	if  (!(envv = malloc((unsigned) (strlen(varr->vn_name) + strlen(str) + 2))))
		nomem();
	sprintf(envv, "%s=%s", varr->vn_name, str);
	putenv(envv);
}

static void	dodisp(const int sockfd, char *cmd)
{
	int	ch;
	char	*expv;
	FILE	*pip_in, *sock_out;

	if  (!cmd)		/* Relevant variable not defined */
		return;
	expv = expandvars(cmd);

	/* We exit after this, don't worry about freeing "expv" etc */

	if  (!(pip_in = popen(expv, "r")))
		return;
	if  (!(sock_out = fdopen(sockfd, "w")))
		return;
#ifdef	SETVBUF_REVERSED
	setvbuf(sock_out, _IOFBF, (char *) 0, BUFSIZ);
#else
	setvbuf(sock_out, (char *) 0, _IOFBF, BUFSIZ);
#endif

	while  ((ch = getc(pip_in)) != EOF)
		putc(ch, sock_out);
	pclose(pip_in);
	free(expv);
}

static void	doremove(const int sockfd)
{
	int	ch;
	struct	varname	*varrm = lookuphash(REMOVE);
	char	*expr;
	FILE	*pip_in, *sock_out;

	if  (!varrm->vn_value)
		return;
	expr = expandvars(varrm->vn_value);
	if  (!(pip_in = popen(expr, "r")))
		return;
	if  (!(sock_out = fdopen(sockfd, "w")))
		return;
#ifdef	SETVBUF_REVERSED
	setvbuf(sock_out, _IOFBF, (char *) 0, BUFSIZ);
#else
	setvbuf(sock_out, (char *) 0, _IOFBF, BUFSIZ);
#endif
	while  ((ch = getc(pip_in)) != EOF)
		putc(ch, sock_out);
	pclose(pip_in);
	free(expr);
}

void	process(const int sockfd)
{
	char	*cp, *pname;
	struct	varname	*varp, *varsl, *varll, *varcmd, *varperson;
	char	linbuf[LINBUF_SIZE];

	varp = lookuphash(PRINTER_VAR);

	for  (;;)  {
		if  (!readsockline(sockfd, linbuf))
			exit(0);
		cp = linbuf;
		if  (debug_level > 0)
			fprintf(stderr, "Had a process line: %d: %s\n", *cp, cp+1);
		switch  (*cp++)  {
		default:
			cp--;
			fprintf(stderr, "Unexpected top-level request 0x%x\n", *cp);
			continue;

		case  PR_CHECK:
			lassign(varp, cp);
			chdir(cp);
			printfiles((char *) 0);
			exit(0);

		case  PR_RECEIVE:
			lassign(varp, cp);
			chdir(cp);
			recvjob(sockfd);
			printfiles(cfilename);
			exit(0);

		case  PR_SHORTDISP:
			varsl = lookuphash(SHORT_LIST);
			varcmd = lookuphash(CMDLINE_VAR);
			pname = cp;
			do  cp++;
			while  (*cp  &&  *cp != ' ');
			*cp++ = '\0';
			setexport(varp, pname);
			setexport(varcmd, cp);
			dodisp(sockfd, varsl->vn_value);
			exit(0);

		case  PR_LONGDISP:
			varll = lookuphash(LONG_LIST);
			varcmd = lookuphash(CMDLINE_VAR);
			pname = cp;
			do  cp++;
			while  (*cp  &&  *cp != ' ');
			*cp++ = '\0';
			setexport(varp, pname);
			setexport(varcmd, cp);
			dodisp(sockfd, varll->vn_value);
			exit(0);

		case  PR_REMOVE:
			varperson = lookuphash(PERSON_VAR);
			varcmd = lookuphash(CMDLINE_VAR);
			pname = cp;
			do  cp++;
			while  (*cp  &&  *cp != ' ');
			*cp++ = '\0';
			setexport(varp, pname);
			pname = cp;
			do  cp++;
			while  (*cp  &&  *cp != ' ');
			*cp++ = '\0';
			setexport(varperson, pname);
			setexport(varcmd, cp);
			doremove(sockfd);
			exit(0);
		}
	}
}
