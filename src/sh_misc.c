/* sh_misc.c -- spshed misc routines

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
#include "incl_sig.h"
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "errnums.h"
#include "defaults.h"
#include "network.h"
#include "spq.h"
#define	UCONST
#include "q_shm.h"
#include "spuser.h"
#include "files.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "notify.h"

extern	void	job_sendnote(struct spq *, struct spptr *, const int, const jobno_t, const int);
extern	void	net_xmit(const netid_t, const int, const LONG);
extern	int	islogged(const int_ugid_t);
extern	FILE	*net_feed(const int, const netid_t, const slotno_t, const jobno_t);
extern	void	do_exit(const int);

extern	char	**environ;
extern	char	*msgdisp, *ptrmsg;
extern	uid_t	Daemuid;

extern	int	Ctrl_chan;
#ifndef	USING_FLOCK
extern	int	Sem_chan;
#endif

#ifdef	BUGGY_SIGCLD
int	nchild;
#endif

static	FILE	*rpfile;

/* Open report file if possible and write message to it.  */

void  nfreport(const int msgno)
{
	int	fid;
	time_t	tim;
	struct  tm	*tp;
	int	saverrno = errno;
	int	mon, mday;

	if  (rpfile == (FILE *) 0)  {
		fid = open(REPFILE, O_WRONLY|O_APPEND|O_CREAT, 0666);
		if  (fid < 0)
			return;

		/* Force owned by spooler */

		if  (Daemuid)
#if	defined(HAVE_FCHOWN) && !defined(M88000)
			Ignored_error = fchown(fid, Daemuid, getegid());
#else
			Ignored_error = chown(REPFILE, Daemuid, getegid());
#endif

		fcntl(fid, F_SETFD, 1);
		rpfile = fdopen(fid, "a");
		if  (rpfile == (FILE *) 0)  {
			close(fid);
			return;
		}
	}

	time(&tim);
	tp = localtime(&tim);
	mon = tp->tm_mon + 1;
	mday = tp->tm_mday;

	/* Keep those dyslexic pirates at SCH happy by swapping round
	   days and months if > 4 hours West */

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

/* Report - generate fatal error message.  */

void  report(const int msgno)
{
	nfreport(msgno);
	do_exit(E_SHEDERR);
}

/* Things to purge from the environment after wrapping SPOOL and =
   round each.  */

const	char	*purgenv[] = {
	"PTR", "HDR", "USER", "JUNAME", "PUNAME",
	"FORM","FLAGS","JOB","HOST","RANGE", "OE"};

char	**newenviron, **endnewenv;
static	int	oldumask;

/* Initialise environment for notify-type commands by squeezing out
   any existing environment variables looking like the ones we set up.  */

void  init_mwenv(int oldu)
{
	char	**ep, **nep;

	/* Allow space for all the new ones plus null ont' end Count
	   existing ones assuming we find none.  */

	unsigned  ecount = sizeof(purgenv)/sizeof(purgenv[0]) + 1;
	for  (ep = environ;  *ep;  ep++)
		ecount++;
	if  (!(newenviron = (char **) malloc(ecount * sizeof(char *))))
		nomem();

	for  (ep = environ, nep = newenviron;  *ep;  ep++)  {
		int	lcnt;
		for  (lcnt = 0;  lcnt < sizeof(purgenv)/sizeof(purgenv[0]);  lcnt++)  {
			unsigned  lng;
			char	ebuf[30];
#ifdef	CHARSPRINTF
			sprintf(ebuf, "SPOOL%s=", purgenv[lcnt]);
			lng = strlen(ebuf);
#else
			lng = sprintf(ebuf, "SPOOL%s=", purgenv[lcnt]);
#endif
			if  (strncmp(*ep, ebuf, lng) == 0)
				goto  missit;
		}
		*nep++ = *ep;
	missit:
		;
	}
	endnewenv = nep;
	oldumask = oldu;
}

/* Invoke message dispatch command.  */

void  rmsg(cmd_type cmd, int msgcode, struct spq *jp, struct spptr *pp, const netid_t netid, const jobno_t jerrf, const int past)
{
	char	**nep = endnewenv;
	char	**ap, *cp;
	FILE	*po, *erfl;
	int	pfds[2];
	PIDTYPE	pid;
	char	ebuf[100];
	char	*arglist[7];

	/* Insert the new values of the environment variables.  */

	sprintf(ebuf, "SPOOLJOB=%ld", (long) jp->spq_job);
	*nep++ = stracpy(ebuf);
	if  (jp->spq_file[0])  {
		sprintf(ebuf, "SPOOLHDR=%s", jp->spq_file);
		*nep++ = stracpy(ebuf);
	}
	sprintf(ebuf, "SPOOLUSER=%ld", (long) jp->spq_uid);
	*nep++ = stracpy(ebuf);
	sprintf(ebuf, "SPOOLJUNAME=%s", jp->spq_uname);
	*nep++ = stracpy(ebuf);
	sprintf(ebuf, "SPOOLPUNAME=%s", jp->spq_puname);
	*nep++ = stracpy(ebuf);
	sprintf(ebuf, "SPOOLFORM=%s", jp->spq_form);
	*nep++ = stracpy(ebuf);
	sprintf(ebuf, "SPOOLFLAGS=%s", jp->spq_flags);
	*nep++ = stracpy(ebuf);

	/* Only add printer and host name if relevant.  */

	if  (pp)  {
		sprintf(ebuf, "SPOOLPTR=%s", pp->spp_ptr);
		*nep++ = stracpy(ebuf);
	}
	if  (netid)  {
		sprintf(ebuf, "SPOOLHOST=%s", look_host(netid));
		*nep++ = stracpy(ebuf);
	}

	/* Only insert ranges if they say something.  */

	if  (jp->spq_start != 0  ||  jp->spq_end <= LOTSANDLOTS)  {
		if  (jp->spq_start != 0)  {
			if  (jp->spq_end <= LOTSANDLOTS)
				sprintf(ebuf, "SPOOLRANGE=%ld-%ld", jp->spq_start+1L, jp->spq_end+1L);
			else
				sprintf(ebuf, "SPOOLRANGE=%ld-", jp->spq_start+1L);
		}
		else
			sprintf(ebuf, "SPOOLRANGE=-%ld", jp->spq_end+1L);
		*nep++ = stracpy(ebuf);
	}

	/* Ditto odd/even, assuming that spd swapped them if SPQ_REVOE set.  */

	if  (jp->spq_jflags & (SPQ_ODDP|SPQ_EVENP))  {
		unsigned  flags = jp->spq_jflags;
		if  (flags & SPQ_REVOE)			/* Setting will have been reversed by spd */
			flags ^= SPQ_ODDP|SPQ_EVENP;
		sprintf(ebuf, "SPOOLOE=%c", flags & SPQ_ODDP? '2': '1');
		*nep++ = stracpy(ebuf);
	}

	/* Put null marker on the end.  */

	*nep = (char *) 0;

	/* And now do the business.  If we don't have a file to set, just do exec.  */

	ap = arglist;
	if  ((cp = strrchr(msgdisp, '/')))
		cp++;
	else
		cp = msgdisp;
	*ap++ = cp;
	cp = ebuf;

	/* Generate arg string -[mwd][p][f] plus -e n for externals */

	*cp++ = '-';
	switch  (cmd)  {
	default:
	case  NOTIFY_MAIL:
		*cp++ = 'm';
		break;
	case  NOTIFY_WRITE:
		*cp++ = 'w';
		break;
	case  NOTIFY_DOSWRITE:
		*cp++ = 'd';	/* Redundantly added for externals */
		break;
	}
	if  (past)
		*cp++ = 'p';
	if  (jerrf != 0)
		*cp++ = 'f';
	*cp = '\0';
	*ap++ = stracpy(ebuf);

	if  (jp->spq_extrn != 0)  {
		*ap++ = "-e";
		sprintf(ebuf, "%u", (unsigned) jp->spq_extrn);
		*ap++ = stracpy(ebuf);
	}

	/* Next arg (2) is message code.
	   NB Assume we don't need "ebuf" again!!!! */

	sprintf(ebuf, "%d", msgcode);
	*ap++ = ebuf;

	/* For DOS machines, append host name arg 3.  */

	if  ((cmd == NOTIFY_DOSWRITE  &&  !(jp->spq_jflags & SPQ_ROAMUSER))  ||  jp->spq_extrn != 0)
		*ap++ = look_host(jp->spq_orighost);

	/* Null on end (4).  */

	*ap = (char *) 0;

	/* If no file to send, just exec without worrying about file.  */

	if  (jerrf == 0)  {
		if  (fork() != 0)
			return;
		execve(msgdisp, arglist, newenviron);
		exit(255);
	}

	/* File to send.
	   Hand-crafted "popen" as too many unix libraries have
	   broken popen which falls over if f.d.s 0 - 2 aren't
	   what they expect. */

	if  (pipe(pfds) < 0  || (pid = fork()) < 0)
		return;

	if  (pid == 0)  {		/*  Child process grandchild of spshed */
		close(pfds[1]);	/*  Write side  */
		if  (pfds[0] != 0)  {
			close(0);
			Ignored_error = dup(pfds[0]);
			close(pfds[0]);
		}
		execve(msgdisp, arglist, newenviron);
		exit(255);
	}

	/* This is the parent process (the fork was in "notify" or "rem_notify").

		spshed
		+------>following code
			+------>code just above

	*/

	close(pfds[0]);			/*  Read side  */
	if  ((po = fdopen(pfds[1], "w")) == (FILE *) 0)  {
		kill(pid, SIGKILL);
		return;
	}

	/* Get error log file, either from network or from local spool directory.  */

	if  (netid != 0)
		erfl = net_feed(FEED_ER, netid, jp->spq_rslot, jerrf);
	else
		erfl = fopen(mkspid(ERNAM, jerrf), "r");

	/* Pump out file */

	if  (erfl != (FILE *) 0)  {
		int  ch;
		while  ((ch = getc(erfl)) != EOF)
			putc(ch, po);
		fclose(erfl);
	}
	fclose(po);
}

/* Fork dealing with child processes and nasty things in the environment */

static	int  mw_fork()
{
#ifndef	BUGGY_SIGCLD
#ifdef	STRUCT_SIG
	struct	sigstruct_name  zc;
#endif
	if  (fork() != 0)
		return  1;
#ifdef	STRUCT_SIG
	zc.sighandler_el = SIG_DFL;
	sigmask_clear(zc);
	zc.sigflags_el = 0;
	sigact_routine(SIGCLD, &zc, (struct sigstruct_name *) 0);
#else
	signal(SIGCLD, SIG_DFL);
#endif
#else
	/* Do everything within a child process to avoid holding up
	   the scheduler.  First wait for other processes though.  */

	if  (nchild > 0)  {
		nchild = 0;
		while  (wait(0) >= 0)
			;
	}

	if  (fork() != 0)  {
		nchild++;
		return  1;
	}
#endif

#ifdef	RUN_AS_ROOT
	if  (Daemuid)
		setuid(Daemuid);
#endif
	umask(oldumask);
	return  0;
}

void  notify(struct spq *jp, struct spptr *pp, const int msgcode, const jobno_t jerrf, const int past)
{
	int	wa = jp->spq_jflags & SPQ_WATTN, wm = jp->spq_jflags & SPQ_WRT;
	struct	spq	jcopy;

#ifdef	NETWORK_VERSION
	if  (jp->spq_netid)  {
		if  (!past || wm || jp->spq_jflags & SPQ_MAIL || jerrf != 0)
			job_sendnote(jp, pp, msgcode, jerrf, past);
		return;
	}
#endif

	/* If user is using spq, turn off write type messages.  Also
	   do it if we don't know which host he might be at.  */

	if  ((wa || wm) && islogged(jp->spq_uid))
		wa = wm = 0;

	if  (past && !wm && !(jp->spq_jflags & SPQ_MAIL) && jerrf == 0)
		return;

	/* Copy job because it can get mangled after we fork We assume
	   that printers won't disappear in the time - they have
	   to be halted first!  */

	jcopy = *jp;
	jp = &jcopy;

	if  (mw_fork())
		return;

	if  (past)  {
		if  (wm)
			rmsg(jp->spq_jflags & SPQ_CLIENTJOB? NOTIFY_DOSWRITE: NOTIFY_WRITE, msgcode, jp, pp, 0L, jerrf, past);
		if  (jp->spq_jflags & SPQ_MAIL || jerrf != 0)
			rmsg(NOTIFY_MAIL, msgcode, jp, pp, 0L, jerrf, past);
		if  (jerrf != 0)
			unlink(mkspid(ERNAM, jerrf));
	}
	else  {
		if  (wa)
			rmsg(jp->spq_jflags & SPQ_CLIENTJOB? NOTIFY_DOSWRITE: NOTIFY_WRITE, msgcode, jp, pp, 0L, jerrf, past);
		if  (jp->spq_jflags & SPQ_MATTN)
			rmsg(NOTIFY_MAIL, msgcode, jp, pp, 0L, jerrf, past);
	}
	exit(0);
}

#ifdef	NETWORK_VERSION
/* Remote version of above */

void  rem_notify(struct sp_omsg *rq)
{
	struct	spq	*jp;
	struct	spptr	*pp;
	int		wa, wm, msgcode = (int) rq->spr_arg1;
	slotno_t	pslot;
	struct	spq	jcopy;

	jp = &Job_seg.jlist[rq->spr_jpslot].j;
	pslot = (slotno_t) rq->spr_arg2;
	pp = pslot < 0? (struct spptr *) 0: &Ptr_seg.plist[pslot].p;

	wa = jp->spq_jflags & SPQ_WATTN;
	wm = jp->spq_jflags & SPQ_WRT;

	/* If user is using spq, turn off write type messages.
	   Note that this is this machine's job hence ok to use
	   spq_uid */

	if  ((wa || wm) && islogged(jp->spq_uid))  {
		wa = 0;
		wm = 0;
	}

	if  (rq->spr_act == SO_PNOTIFY && !wm && !(jp->spq_jflags & SPQ_MAIL) && rq->spr_jobno == 0)
		return;

	/* Copy job because it can get mangled after we fork As before
	   we assume different of printers.  */

	jcopy = *jp;
	jp = &jcopy;

	if  (mw_fork())
		return;

	if  (rq->spr_act == SO_PNOTIFY)  {
		if  (wm)
			rmsg(jp->spq_jflags & SPQ_CLIENTJOB? NOTIFY_DOSWRITE: NOTIFY_WRITE,
			     msgcode, jp, pp, rq->spr_netid, rq->spr_jobno, PAST_TENSE);
		if  (jp->spq_jflags & SPQ_MAIL || rq->spr_jobno != 0)
			rmsg(NOTIFY_MAIL, msgcode, jp, pp, rq->spr_netid, rq->spr_jobno, PAST_TENSE);
		if  (rq->spr_jobno != 0)
			net_xmit(rq->spr_netid, SN_DELERR, rq->spr_jobno);
	}
	else  {
		if  (wa)
			rmsg(jp->spq_jflags & SPQ_CLIENTJOB? NOTIFY_DOSWRITE: NOTIFY_WRITE,
			     msgcode, jp, pp, rq->spr_netid, rq->spr_jobno, PRESENT_TENSE);
		if  (jp->spq_jflags & SPQ_MATTN)
			rmsg(NOTIFY_MAIL, msgcode, jp, pp, rq->spr_netid, rq->spr_jobno, PRESENT_TENSE);
	}
	exit(0);
}
#endif

/* Notify printers in funny states if required.
   We only do this with local printers */

void  ptrnotify(struct spptr *pp)
{
	if  (ptrmsg  &&  *ptrmsg)  {
		char	*cp;
		struct	spptr	copyp;
		copyp = *pp;	/* Might get mangled after we fork */
		if  (mw_fork())
			return;
		if  ((cp = strrchr(ptrmsg, '/')))
			cp++;
		else
			cp = ptrmsg;
		execlp(ptrmsg, cp, copyp.spp_ptr, copyp.spp_dev, copyp.spp_comment, (char *) 0);
		exit(255);
	}
}
