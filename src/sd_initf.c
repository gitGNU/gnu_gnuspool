/* sd_initf.c -- spd get parameters via spdinit

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
#include "incl_sig.h"
#include <sys/types.h>
#include <errno.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef	HAVE_TERMIO_H
#include <termio.h>
#else
#include <sgtty.h>
#endif
#include <sys/stat.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <setjmp.h>
#include "errnums.h"
#include "initp.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#define	UCONST
#include "q_shm.h"
#include "ecodes.h"
#include "incl_unix.h"

void	pflush(void);
void	pout(char *, int);
int	opendev(void);
RETSIGTYPE	stopit(int);
void	seterrorstate(const char *);
void	holdorignore(const int);

extern	void	set_signal(const int, RETSIGTYPE (*)(int));

int	rfid = -1,
	lfid = -1;
PIDTYPE	childproc;

extern	FILE	*ffile;

extern	int	pfile;		/*  Now use direct writes  */
extern	PIDTYPE	filtpid;

extern	char	*shellname,
		*ptdir,
		*daeminit;

extern	char	*devnam, *prtnam;	/*  Device and printer names  */

struct	initpkt	in_params;

extern	char	*setupnam,	/*  Setup file name  */
		*sttystring,	/*  Explicit stty string */
		*filternam,	/*  Filter  */
		*netfilter,	/*  Network filter */
		*bannprog,	/*  Banner program */
		*portsu,	/*  Port set up */
		*initstr,	/*  Initialisation string */
		*haltstr,	/*  Halt string */
		*dsstr,		/*  Doc start string */
		*destr,		/*  Doc end string */
		*bdsstr,	/*  Banner doc start string */
		*bdestr,	/*  Banner doc end string */
		*ssstr,		/*  Suffix start string  */
		*sestr,		/*  Suffix end string  */
		*psstr,		/*  Page start string */
		*pestr,		/*  Page end string */
		*abortstr,	/*  Abort string */
		*restartstr,	/*  Restart string */
		*pathn,		/*  Full path name for device */
		*rdelim,	/*  Record delimiter  */
		*formnam,	/*  Current form name  */
		*outbuf;	/*  Output buffer  */

extern	struct	spptr  *Pptr;
extern	struct	spr_req	reply;
#define	CRESP	reply.spr_un.c.spr_c

/* Open report file if possible write message to it.  */

void	nfreport(const int msgno)
{
	int	fid;
	time_t	tim;
	struct  tm	*tp;
	static	FILE	*rpfile = (FILE *) 0;
	int	saverrno = errno;
	int	mday, mon;

	if  (!rpfile)  {
		if  ((fid = open(REPFILE, O_WRONLY|O_APPEND|O_CREAT, 0666)) < 0)
			return;
		if  ((rpfile = fdopen(fid, "a")) == (FILE *) 0)
			return;
	}
	time(&tim);
	tp = localtime(&tim);
	mon = tp->tm_mon + 1;
	mday = tp->tm_mday;

	/* Keep those dyslexic pirates in the US happy by swapping
	   round days and months if >= 4 hours West */

#ifdef	HAVE_TM_ZONE
	if  (tp->tm_gmtoff <= -4 * 60 * 60)  {
#else
	if  (timezone >= 4 * 60 * 60)  {
#endif
		mday = mon;
		mon = tp->tm_mday;
	}

	fprintf(rpfile, "%.2d:%.2d:%.2d %.2d/%.2d - %s\n==============\n",
		tp->tm_hour, tp->tm_min, tp->tm_sec, mday, mon, progname);
	errno = saverrno;
	fprint_error(rpfile, msgno);
	fflush(rpfile);
}

void	report(const int n)
{
	nfreport(n);
	seterrorstate((const char *) 0);
}

void	nomem(void)
{
	report($E{NO MEMORY});
}

int	exec_stop;

RETSIGTYPE	do_exec_stop(int n)
{
#ifdef UNSAFE_SIGNALS
	signal(n, SIG_IGN);
#endif
	exec_stop++;
}

int	exec_wait(void)
{
	int	status;
#ifndef	HAVE_WAITPID
	PIDTYPE	wpid;
#endif
	exec_stop = 0;
	set_signal(DAEMSTOP, do_exec_stop);
#ifdef	HAVE_WAITPID
	while  (waitpid(childproc, &status, 0) < 0  &&  errno == EINTR)
		if  (exec_stop > 0)
			kill(-childproc, exec_stop > 1? SIGKILL: SIGTERM);
#else
	while  ((wpid = wait(&status)) != pid)  {
		if  (wpid >= 0)
			continue;
		if  (errno != EINTR)
			break;
		if  (exec_stop > 0)
			kill(-childproc, exec_stop > 1? SIGKILL: SIGTERM);
	}
#endif
	holdorignore(DAEMSTOP);
	childproc = 0;
	return  status;
}

void	exec_prep(const int ofd, const int ofd2)
{
	close(1);
	close(2);
	dup(ofd);
	dup(ofd2);
	close(ofd);
	close(ofd2);
#ifdef	SETPGRP_VOID
	setpgrp();
#else
	setpgrp(0, getpid());
#endif
	set_signal(SIGPIPE, SIG_IGN);
	set_signal(DAEMSTOP, SIG_DFL);
	set_signal(DAEMRST, SIG_DFL);
}

/* Either write setup/halt/docstart/docend string to device (excepting
   final null char), or exec a process to do it.
   Return 1 if OK.
   Return 0 if not finished, possibly because of interrupt */

int  execorsend(char *name0, char *str, unsigned lng, const ULONG eflag, const int restsig)
{
	int	ofid = pfile, ret;

	if  (!str  ||  lng == 0)
		return  1;

	/* In case running under filter (applies to page strings) */

	if  (filtpid > 0)  {
		if  (!eflag)  {
			fwrite(str, 1, lng-1, ffile);
			return  1;
		}
		fflush(ffile);
		ofid = fileno(ffile);
	}
	else  {
		if  (pfile < 0)  {
			if  (!opendev())
				seterrorstate((const char *) 0);
			ofid = pfile;
		}
		else
			pflush();

		if  (!eflag)  {
			pout(str, (int) lng - 1);
			return  1;
		}
	}

	if  ((childproc = fork()) == 0)  {
		exec_prep(ofid, ofid);
		execl(shellname, name0, "-c", str, (const char *) 0);
		exit(255);
	}
	if  (childproc < 0)
		report($E{Internal cannot fork});

	ret = exec_wait() == 0  ||  exec_stop <= 0;
	if  (restsig)
		set_signal(DAEMSTOP, stopit);
	return  ret;
}

/* This is a routine to directly execute things or run the shall as
   required This may return to face the music if it can't find
   the proggie.  Assumed always to be run from a forked-off
   process so we don't worry about memory leaks and
   what-have-you.  Thinks: sometime we might predigest the PATH
   and possibly the actual stuff.  */

#define	MAX_EARGS	80

void	path_execute(char *name0, char *lin, const int directex)
{
	if  (directex)  {
		int	argc = 1;
		char	**ap, *cmd, *resb, *narg;
		char	*argv[MAX_EARGS]; /* Tough if not enough but we do check.... */

		/* Expand any environment vars in the command line.
		   Discard leading spaces */

		if  (strchr(lin, '$'))
			lin = envprocess(lin);

		while  (isspace(*lin))
			lin++;

		/* This is probably an overkill.  */

		if  (!(resb = malloc((unsigned) (2 * strlen(lin) + 1))))
			nomem();
		narg = resb;

		ap = &argv[1];	/* Come back to arg0 later */
		cmd = lin;
		while  (*lin  &&  !isspace(*lin))
			lin++;

		while  (*lin)  {

			*lin++ = '\0';			/* Terminates last thing */

			while  (isspace(*lin))
				lin++;

			if  (!*lin)			/* Might terminate after lots of spaces */
				break;

			while  (*lin  &&  !isspace(*lin))  {
				if  (*lin == '\''  || *lin == '\"')  {
					int	quote = *lin++;

					while  (*lin  &&  *lin != quote)
						*narg++ = *lin++;

					if  (*lin)
						lin++;
				}
				else
					*narg++ = *lin++;
			}
			*narg++ = '\0';

			if  (argc < MAX_EARGS-1)  {
				argc++;
				*ap++ = resb;
			}

			resb = narg;
		}

		*ap = (char *) 0;

		/* Set arg 0 to be the basename of whatever we do.  */

		if  ((narg = strrchr(cmd, '/')))
			argv[0] = narg+1;
		else
			argv[0] = cmd;

		if  (cmd[0] != '/')  {
			char	*patha = envprocess("$PATH:"), *pathb = envprocess(IPROGDIR), *path;
			char	*cp, *resbuf;

			if  (!(path = malloc((unsigned) (strlen(patha) + strlen(pathb) + 1))))
				nomem();

			strcpy(path, patha);
			strcat(path, pathb);
			free(patha);
			free(pathb);

			if  (!(resbuf = malloc((unsigned) (1 + strlen(path) + strlen(cmd)))))
				nomem();

			while  ((cp = strchr(path, ':')))  {
				*cp = '\0';
				if  (path[0] == '/')  {
					sprintf(resbuf, "%s/%s", path, cmd);
					execv(resbuf, argv);
				}
				path = cp + 1;
			}
			if  (path[0] == '/')  {
				sprintf(resbuf, "%s/%s", path, cmd);
				execv(resbuf, argv);
			}
		}
		else
			execv(cmd, argv);
	}
	else
		execl(shellname, name0, "-c", lin, (char *) 0);
}

/* Read parameters from setup file by invoking 'spdinit' process.
   Return 1 - ok 0 - error (reported to report file).  */

int	rinitfile(void)
{
	int	status;
	PIDTYPE	pid;
#ifndef	HAVE_WAITPID
	PIDTYPE	wpid;
#endif
	int	pfs[2];
	char	*rfile = (char *) 0, *lfile = (char *) 0;

	/* Clear any gunge left over from last time.  (rfile and lfile are local vars).  */

#define	CLEAR_FILE(X)	if (X >= 0) { close(X); X = -1; }
#define	CLEAR_STR(X)	if (X != (char *) 0) { free(X); X = (char *) 0; }

	CLEAR_FILE(rfid);
	CLEAR_FILE(lfid);

	CLEAR_STR(setupnam);
	CLEAR_STR(sttystring);
	CLEAR_STR(filternam);
	CLEAR_STR(netfilter);
	CLEAR_STR(portsu);
	CLEAR_STR(bannprog);
	CLEAR_STR(haltstr);
	CLEAR_STR(dsstr);
	CLEAR_STR(destr);
	CLEAR_STR(bdsstr);
	CLEAR_STR(bdestr);
	CLEAR_STR(ssstr);
	CLEAR_STR(sestr);
	CLEAR_STR(psstr);
	CLEAR_STR(pestr);
	CLEAR_STR(abortstr);
	CLEAR_STR(restartstr);
	CLEAR_STR(rdelim);
	CLEAR_STR(outbuf);

	if  (pipe(pfs) < 0)  {
		nfreport($E{Internal cannot create pipe});
		return  0;
	}

	if  ((pid = fork()) == 0)  {
		close(pfs[0]);	/*  Read side  */
		close(1);
		dup(pfs[1]);	/*  Should be 1 now */
		close(pfs[1]);
		set_signal(SIGPIPE, SIG_IGN);
		set_signal(SIGTERM, SIG_IGN);
		execl(daeminit, "SPDI", prtnam, formnam, (const char *) 0);
		nfreport($E{No spdinit});
		exit(255);
	}
	if  (pid < 0)  {
		nfreport($E{Internal cannot fork});
		return  0;
	}
	close(pfs[1]);

	/* Read header */

	if  (read(pfs[0], (char *) &in_params, sizeof(in_params)) != sizeof(in_params))  {
		nfreport($E{Spdinit bad packet});
		return  0;
	}

	/* Initialise charge/retention details from setup file.  */

	CRESP.spc_cpc = in_params.pi_charge;
	reply.spr_un.c.spr_flags = 0;
	if  (in_params.pi_flags & PI_RETAIN)
		reply.spr_un.c.spr_flags |= SPF_RETAIN;
	if  (in_params.pi_flags & PI_NOCOPIES)
		reply.spr_un.c.spr_flags |= SPF_NOCOPIES;

	/* Allocate output buffer */

	if  ((outbuf = (char *) malloc(in_params.pi_obuf)) == (char *) 0)
		nomem();

#define ERR_NUM $E{Spdinit bad packet}

#define	RD_STR(L, S)	if  (in_params.L)  {\
	if  ((S = (char *) malloc(in_params.L)) == (char *) 0)\
		nomem();\
	if  (read(pfs[0], S, (unsigned) in_params.L) != in_params.L)  {\
		nfreport(ERR_NUM);\
		return  0;\
	}}

	RD_STR(	pi_setup,	initstr)
	RD_STR(	pi_halt,	haltstr)
	RD_STR(	pi_docstart,	dsstr)
	RD_STR(	pi_docend,	destr)
	RD_STR(	pi_bdocstart,	bdsstr)
	RD_STR(	pi_bdocend,	bdestr)
	RD_STR(	pi_sufstart,	ssstr)
	RD_STR(	pi_sufend,	sestr)
	RD_STR(	pi_pagestart,	psstr)
	RD_STR(	pi_pageend,	pestr)
	RD_STR(	pi_abort,	abortstr)
	RD_STR(	pi_restart,	restartstr)
	RD_STR(	pi_align,	setupnam)
	RD_STR(	pi_filter,	filternam)
	RD_STR(	pi_rfile,	rfile)
	RD_STR(	pi_rcstring,	rdelim)
	RD_STR(	pi_logfile,	lfile)
	RD_STR( pi_portsu,	portsu)
	RD_STR( pi_bannprog,	bannprog)
	RD_STR( pi_netfilt,	netfilter)
	RD_STR( pi_sttystring,	sttystring)

	close(pfs[0]);
#ifdef	HAVE_WAITPID
	while  (waitpid(pid, &status, 0) < 0  &&  errno == EINTR)
		;
#else
	while  ((wpid = wait(&status)) != pid  &&  (wpid >= 0 || errno == EINTR))
		;
#endif
	if  ((status & 255) != 0)  {
		nfreport($E{Spdinit error});
		return  0;
	}

	/* If network device absolutely insist on network filter */

	disp_str = prtnam;
	disp_str2 = pathn;

	if  (Pptr->spp_netflags & SPP_LOCALNET)  {
		if  (!netfilter)  {
			nfreport($E{Network device no network command});
			return  0;
		}
	}
	else  if  (netfilter)  {
		nfreport($E{Network device no network command});
		return  0;
	}

	/* If non-zero exit code, assume that the error has already been reported.  */

	if  (status != 0)
		return  0;

	/* Open report file if any.  */

	if  (rfile != (char *) 0)  {
		rfid = open(rfile, O_RDWR|O_CREAT, 0644);
		free(rfile);
	}

	/* Open log file if any.  */

	if  (lfile != (char *) 0)  {
		lfid = open(lfile, O_WRONLY|O_CREAT|O_APPEND, 0644);
		free(lfile);
	}
	return  1;
}
