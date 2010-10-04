/* sd_fctrl.c -- spd file control optionsx

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
#include <sys/types.h>
#include <sys/ipc.h>
#ifdef	USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_sig.h"
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef	HAVE_TERMIO_H
#include <termio.h>
#else
#include <sgtty.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include "errnums.h"
#include "initp.h"
#include "ecodes.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#define	UCONST
#include "q_shm.h"
#include "incl_unix.h"
#include "incl_ugid.h"

#define	HANGTIME	100	/* Suitably long */

extern	void	exec_prep(const int, const int);
extern  int	exec_wait();
extern	void	nfreport(const int);
extern	void	report(const int);
extern	FILE	*net_feed(const int, const netid_t, const slotno_t, const jobno_t);
extern	void	path_execute(char *, char *, const int);
extern	void	holdorignore(const int);
extern	void	seterrorstate(const char *);
extern	void	setofflinestate();
extern	RETSIGTYPE  catchoff(int);
#ifndef	UNSAFE_SIGNALS
void  unhold(const int);
#endif
extern	void	set_signal(const int, RETSIGTYPE (*)(int));

extern	char	jerrf[];

int	pfile = -1,
	errfd = -1,
	isfifo;
PIDTYPE	filtpid = -1,
	netfiltpid = -1;

extern	FILE	*ffile;

char	*outbuf;

extern	char	*pathn,
	*prtnam,
	*filternam,
	*netfilter,
	*portsu,
	*sttystring,
	*shellname;

int	outb_ptr;

extern	PIDTYPE	childproc;
extern	struct	initpkt	in_params;
extern	LONG	Num_sent;		/*  Number of chars xmted */

extern	struct	spr_req	rq;
extern	struct	spq	*Cjob;
extern	struct	spptr	*Pptr;		/*  Current ptr descr */

static	int	hadalarm, filt_stop;

RETSIGTYPE  do_filt_stop(int n)
{
#ifdef	UNSAFE_SIGNALS
	signal(n, do_filt_stop);
#endif
	filt_stop++;
}

RETSIGTYPE  catch_alsig(int n)
{
#ifdef	UNSAFE_SIGNALS
	signal(SIGALRM, catch_alsig);
#endif
	hadalarm++;
}

/* Close network filter. Check for exit codes.
   Return 1 or 0 in the specific case where we get an abort message */

int  closenetfilt()
{
#ifndef	HAVE_WAITPID
	PIDTYPE	wpid;
#endif
	int	status, stage = -1;

	set_signal(SIGALRM, catch_alsig);
	set_signal(SIGPIPE, SIG_IGN);
	filt_stop = 0;
	set_signal(DAEMSTOP, do_filt_stop);

	/* We can close the pipe to the network filter because
	   that itself (we think!) won't hang, it's the network
	   filter which does. */

	close(pfile);
	pfile = -1;

	/* We understand "canhang" to mean that the network filter process
	   may become immortal.... */

	hadalarm = 0;
	alarm(in_params.pi_ca);

	/* Now wait for the network filter process to finish. */

#ifdef	HAVE_WAITPID
	while  (waitpid(netfiltpid, &status, 0) < 0)
#else
	while  ((wpid = wait(&status)) != netfiltpid)  if  (wpid < 0)
#endif
	{
		if  (errno != EINTR)  {
			disp_str = netfilter;
			disp_arg[0] = netfiltpid;
			nfreport($E{Lost track network filter});
			set_signal(SIGALRM, catchoff);
			seterrorstate(gprompt($P{Lost track network filter}));
		}

		/* If it was an abort, apply boot */

		if  (filt_stop > 0)  {
			kill(-netfiltpid, filt_stop > 1? SIGKILL: SIGTERM);
			continue;
		}

		/* If if wasn't an alarm which incremented hadalarm, forget it */

		if  (!hadalarm)
			continue;

		hadalarm = 0;

		/* As the first stage, if we've had an alarm, just send the first
		   kill signal and wait again. Kills send to process group */

		if  (stage < 0)  {
			kill(-netfiltpid, in_params.pi_clsig);
			alarm(in_params.pi_ca);
			stage = 0;
			continue;
		}

		/* Second stage, give it the hobnail treatment and try again.
		   Also do this if we don't believe it can hang */

		if  (stage == 0  ||  !(in_params.pi_flags & PI_CANHANG))  {
			kill(-netfiltpid, SIGKILL);
			alarm(in_params.pi_ca);
			stage = 1;
			continue;
		}

		/* We think the thing has hung. */

		disp_str = netfilter;
		disp_arg[0] = netfiltpid;
		nfreport($E{spd network filter hung});
		set_signal(SIGALRM, catchoff);
		seterrorstate(gprompt($P{spd network filter hung}));
	}

	/* Restore alarm */

	alarm(0);
	set_signal(SIGALRM, catchoff);
	set_signal(DAEMSTOP, SIG_IGN);

	/* Check status for signals/error */

	if  (status & 255)  {	/* Had signal */
		int	wsig = status & 127;
		unsigned  whatsig = 1 << wsig;

		if  ((wsig == SIGTERM || wsig == SIGKILL)  &&  filt_stop > 0)
			return  0;

		if  (in_params.pi_offlsig & whatsig)
			setofflinestate();

		if  (in_params.pi_errsig & whatsig)  {

			/* Report an error state */

			disp_str = netfilter;
			disp_arg[0] = wsig;

			if  (status & 128)  {
				nfreport($E{Network filter core dumped});
				seterrorstate(gprompt($P{Network filter core dumped}));
			}
			else  {
				nfreport($E{Network filter crashed});
				seterrorstate(gprompt($P{Network filter crashed}));
			}
		}
	}
	else  {
		unsigned  whatexit = status >> 8;
		unsigned  word = whatexit >> 5;
		unsigned  bit = 1 << (whatexit & 31);

		if  (in_params.pi_offlexit[word] & bit)
			setofflinestate();

		if  (in_params.pi_errexit[word] & bit)  {

			/* Report sets error state we don't bother with
			   displaying it as we rely on the thing to feed back errors if
			   reqd */

			disp_str = netfilter;
			disp_arg[0] = whatexit;
			report($E{Network filter error halted});
		}
	}

	/* Normal shutdown case.
	   Apply post-close wait in the particular case where we are doing reopens and
	   we are not shutting down. */

	if  ((in_params.pi_flags & PI_REOPEN)  &&  in_params.pi_postclsl  &&  Pptr  &&  Pptr->spp_state != SPP_SHUTD)
		sleep(in_params.pi_postclsl);

	return  1;
}

/* Close device (or network filter).
   Return 1 if OK 0 if we had an interrupt on a network filter */

int  closedev()
{
	if  (pfile < 0)		/* Not open forget it */
		return  1;

	if  (netfilter)
		return  closenetfilt();

	/* If the device can hang up the whole machine, get a child
	   process to close it. With a bit of luck if immortality
	   strikes just that will be immortal.  */

	if  (in_params.pi_flags & PI_CANHANG)  {
		PIDTYPE	wpid;
		int	status;

		/* Need to set signal before we fork to be sure of
		   detaching the segments before getting killed.  */

#ifdef	UNSAFE_SIGNALS
		RETSIGTYPE	(*oldalarm)() = signal(SIGALRM, SIG_IGN);
#else
		holdorignore(SIGALRM);
#endif
		if  ((wpid = fork()) == 0)  {
			PIDTYPE	gpid;

			/* If we are doing this in a "reopen",
			   we have to avoid leaving a "zombie".  */

			if  (in_params.pi_flags & PI_REOPEN)  {
#ifdef	SETPGRP_VOID
				setpgrp(); /* So we can kill grandchild */
#else
				setpgrp(0, getpid());
#endif
				/* Main path exits when it's made a grandchild
				   process */

				while  ((gpid = fork()) < 0)
					sleep(in_params.pi_ca);
				if  (gpid != 0)
					exit(0);
			}

			/* If this (grand) child process is hanging
			   make sure it leaves the shm segs alone
			   while it does nothing useful.  */

#ifdef	USING_MMAP
			if  (Ptr_seg.inf.seg)  {
				munmap(Ptr_seg.inf.seg, Ptr_seg.inf.segsize);
				close(Ptr_seg.inf.mmfd);
			}
			if  (Job_seg.dinf.seg){
				munmap(Job_seg.dinf.seg, Job_seg.dinf.segsize);
				close(Job_seg.dinf.mmfd);
			}
			if  (Job_seg.iinf.seg){
				munmap(Job_seg.iinf.seg, Job_seg.iinf.segsize);
				close(Job_seg.iinf.mmfd);
			}
#else
			if  (Ptr_seg.inf.seg)
				shmdt(Ptr_seg.inf.seg);
			if  (Job_seg.dinf.seg)
				shmdt(Job_seg.dinf.seg);
			if  (Job_seg.iinf.seg)
				shmdt(Job_seg.iinf.seg);
#endif
			set_signal(SIGALRM, SIG_DFL);
			sleep(in_params.pi_ca);
			close(pfile);		/* This might hang */
			exit(0);
		}
#ifdef	UNSAFE_SIGNALS
		signal(SIGALRM, oldalarm);
#else
		unhold(SIGALRM);
#endif
		/* Of course we might not be able to fork */

		if  (wpid < 0)
			report($E{Internal cannot fork});

		close(pfile);			/* Hopefully this WONT hang */

		/* Try to put it out of its misery
		   This may make the exit in the kill hang */

		if  (in_params.pi_flags & PI_REOPEN)  {
#ifdef	HAVE_WAITPID
			kill(-wpid, SIGALRM); /* Try to kill grandchild */
			while  (waitpid(wpid, &status, 0) < 0  &&  errno == EINTR)
				;
#else
			PIDTYPE	npid;
			kill(-wpid, SIGALRM); /* Try to kill grandchild */
			while  ((npid = wait(&status)) != wpid  &&  (npid >= 0 || errno == EINTR))
				;
#endif
		}
		else
			kill(wpid, SIGALRM);
	}
	else
		close(pfile);
	pfile = -1;
	return  1;
}

/* Catch offline type messages (from SIGHUP and SIGALRM).  */

RETSIGTYPE  catchoff(int n)
{
	set_signal(SIGHUP, SIG_IGN);
	set_signal(SIGALRM, SIG_IGN);
	set_signal(DAEMSTOP, SIG_IGN);
	set_signal(DAEMRST, SIG_IGN);
	closedev();			/* This is now "hang-proof" (heard that before!) */
	setofflinestate();
}

/* Fire off a process (if possible) to remember standard error from
   our printer when as terminal server */

static	int  savefeedback()
{
	PIDTYPE	fpid;
	int	pfs[2];
	FILE	*infile;
	char	in_line[80];

	if  (pipe(pfs) < 0)	/* Just use stderr if no pipe */
		return  0;
	if  (!(infile = fdopen(pfs[0], "r")))  {
		close(pfs[0]);
		close(pfs[1]);
		return  0;
	}
	fpid = fork();
	if  (fpid < 0)  {	/* Just use stderr instead life is complicated */
		fclose(infile);
		close(pfs[1]);
		return  0;
	}
	if  (fpid != 0)  {	/* Main path - net filter process */
		close(2);
		if  (dup(pfs[1]) < 0)  {
			kill(fpid, SIGKILL);
			close(pfs[0]);
			close(pfs[1]);
			return  0;
		}
		fclose(infile);
		close(pfs[1]);
		return  1;
	}

	/* We are now in the process which reads the std error */

	close(pfile);
	close(pfs[1]);
	set_signal(DAEMSTOP, SIG_DFL);
	set_signal(SIGPIPE, SIG_DFL);
	set_signal(SIGHUP, SIG_DFL);
	set_signal(SIGALRM, SIG_DFL);
	set_signal(DAEMRST, SIG_IGN);

	while  (fgets(in_line, sizeof(in_line), infile))  {
		int	l = strlen(in_line) - 1;
		if  (in_line[l] == '\n')
			in_line[l] = '\0';
		if  (in_params.pi_flags & PI_LOGERROR)  {
			disp_str = pathn;
			disp_str2 = prtnam;
			nfreport($E{Log error header});
			disp_str = in_line;
			nfreport($E{Log error report});
		}
		if  (in_params.pi_flags & PI_FBERROR)
			strncpy(Pptr->spp_feedback, in_line, PFEEDBACK);
	}
	exit(0);
}

/* Open device.  */

int  opendev()
{
	if  (netfilter)  {
		int	pfs[2];

		if  (pipe(pfs) < 0)
			report($E{Internal cannot create pipe});

		netfiltpid = fork();
		if  (netfiltpid < 0)
			report($E{Internal cannot fork});

		if  (netfiltpid == 0)  {
			close(pfs[1]);
			close(0);	/*  Cfile I think  */
			Ignored_error = dup(pfs[0]);	/*  Should be 0 now */
			close(pfs[0]);
			close(1);
			open("/dev/null", O_RDWR); /* Should be fd 1 */
			if  (!(in_params.pi_flags & (PI_LOGERROR | PI_FBERROR)  &&  savefeedback()))  {
				close(2);	/* So attach /dev/null instead */
				Ignored_error = dup(1);
			}
			/* Set process group to try to insulate the main path
			   from attempts to kill me */
#ifdef	SETPGRP_VOID
			setpgrp();
#else
			setpgrp(0, getpid());
#endif
			set_signal(DAEMRST, SIG_DFL);
			set_signal(SIGPIPE, SIG_DFL);
			set_signal(SIGTERM, SIG_DFL);
			path_execute("NETFILTER", netfilter, (in_params.pi_flags & PI_EXNETFILT)? 1: 0);
			exit(255);
		}
		close(pfs[0]);	/*  Close read side  */
		pfile = pfs[1];
		/* In main process treat constipated pipe signal as offline */
		set_signal(SIGPIPE, catchoff);
	}
	else  if  (sttystring)  {
#ifdef	HAVE_WAITPID
		PIDTYPE	spid;
#else
		PIDTYPE	spid, rpid;
#endif
		int	status;

		/* Assume stty has cryptic arguments which are a
		   mystery to us.  Open in a loop - setting alarm
		   for being offline.  This doesn't work if we
		   have to pratt around with clocal as well (sigh) */

		alarm(in_params.pi_oa);
		while  ((pfile = open(pathn, O_RDWR)) < 0)  {
			if  (errno != EINTR)  {
				nfreport($E{Cannot open device file});
				return  0;
			}
		}
		alarm(0);
		spid = fork();
		if  (spid < 0)
			report($E{Internal cannot fork});
		if  (spid == 0)  {
			set_signal(DAEMRST, SIG_DFL);
			set_signal(SIGPIPE, SIG_DFL);
			set_signal(SIGTERM, SIG_DFL);
			close(0); /* If the guy doesn't like these he can > or < them*/
			close(1);
			close(2);
			Ignored_error = dup(dup(dup(pfile)));
			close(pfile);
			path_execute("STTY", sttystring, (in_params.pi_flags & PI_EXSTTY)? 1: 0);
			exit(255);
		}
		set_signal(SIGALRM, catch_alsig);
		alarm(in_params.pi_oa);
#ifdef	HAVE_WAITPID
		while  (waitpid(spid, &status, 0) < 0)
#else
		while  ((rpid = wait(&status)) != spid)  if  (rpid < 0)
#endif
		{
			kill(spid, SIGKILL);
			set_signal(SIGHUP, SIG_IGN);
			set_signal(SIGALRM, SIG_IGN);
			closedev();
			setofflinestate();
		}
		alarm(0);
		if  (status != 0)  {
			closedev();
			return  0;
		}
	}
	else  {
#ifdef	HAVE_TERMIO_H
		int  dummy = 0;
#endif
		unsigned	ftype;
		struct	stat	sbuf;

		/* In case of error....  */

		disp_str = pathn;

#ifdef	HAVE_TERMIO_H
		if  (in_params.pi_tty.c_cflag & CLOCAL)  {
			if  ((dummy = open(pathn, O_WRONLY|O_NDELAY)) < 0)  {
				nfreport($E{Cannot open device file});
				return  0;
			}
#ifdef	OS_PRIME
			/* Use "old" version of TCSETAW which avoids
			   mangling ^s/^q characters */

			if  (ioctl(dummy, oTCSETAW, &in_params.pi_tty) < 0)  {
				nfreport($E{Cannot open device file});
				return  0;
			}
#else
			if  (ioctl(dummy, TCSETAW, &in_params.pi_tty) < 0)  {
				nfreport($E{Cannot open device file});
				return  0;
			}
#endif
		}
#endif /* HAVE_TERMIO_H */

		/* Open in a loop - setting alarm for being offline.  */

		alarm(in_params.pi_oa);
		while  ((pfile = open(pathn, O_WRONLY)) < 0)  {
			if  (errno != EINTR)  {
				nfreport($E{Cannot open device file});
				return  0;
			}
		}
		alarm(0);

		/* Check that the thing really is a device - protest if it isn't.  */

		fstat(pfile, &sbuf);
		ftype = sbuf.st_mode & S_IFMT;
		if  (ftype == S_IFCHR)
			isfifo = 0;
		else  if  (ftype == S_IFIFO)  {
			isfifo = 1;
#ifndef	HAVE_TERMIO_H
			return  1;
#endif
		}
		else  {
			nfreport($E{Invalid device name});
			return  0;
		}

#ifdef	HAVE_TERMIO_H
		if  (in_params.pi_tty.c_cflag & CLOCAL)
			close(dummy);
		else  {
#ifdef	PRIME
			ioctl(pfile, oTCSETAW, &in_params.pi_tty);
#else
			ioctl(pfile, TCSETAW, &in_params.pi_tty);
#endif
		}
#else
		ioctl(pfile, TIOCSETP, &in_params.pi_tty);
#endif
	}

	return  1;
}

/* Output a bufferful, and check for offline indications.  */

void  pout(char *buf, int size)
{
	int	nbytes;

	if  (pfile < 0  &&  !opendev())
		seterrorstate((const char *) 0);

	alarm(in_params.pi_offa);

	do  {
		if  ((nbytes = write(pfile, buf, (unsigned) size)) < 0)  {
			if  (errno != EINTR)
				report($E{Device output error});
		}
		else  {
			buf += nbytes;
			size -= nbytes;
		}
	}  while  (size > 0);

	alarm(0);
}

void  stuffch(const int ch)
{
	outbuf[outb_ptr] = (char) ch;
	if  (++outb_ptr >= in_params.pi_obuf)  {
		pout(outbuf, (int) in_params.pi_obuf);
		outb_ptr = 0;
		Job_seg.dptr->js_serial++;
	}
}

void  pchar(const int ch)
{
	if  (ch == '\n'  &&  in_params.pi_flags2 & PI_ADDCR)
		stuffch('\r');
	stuffch(ch);
	Num_sent++;
}

void  pflush()
{
	if  (outb_ptr > 0)  {
		pout(outbuf, outb_ptr);
		outb_ptr = 0;
	}
	if  (isfifo  ||  netfilter)
		return;
#ifdef	TURN_XOFF
#ifdef	HAVE_TERMIO_H
	ioctl(pfile, TCSBRK, 1);
#else
#ifdef TIOCSTART
	ioctl(pfile, TIOCSTART, 0);
#else
	ioctl(pfile, TIOCFLUSH, 0);
#endif
#endif
#endif
}

/* Open filter process.  */

void  filtopen()
{
	int	pfs[2];

	/* Make sure printer is open first (bug fix 18/7/91) */

	if  (pfile < 0  &&  !opendev())
		seterrorstate((const char *) 0);

	if  (pipe(pfs) < 0)
		report($E{Internal cannot create pipe});

	if  (errfd < 0)  {
		sprintf(jerrf, "SPDE%.6lu", (unsigned long) getpid());
		if  ((errfd = open(jerrf, O_RDWR|O_TRUNC|O_CREAT|O_APPEND, 0644)) < 0)
			report($E{Cannot create job error file});
	}

	filtpid = fork();
	if  (filtpid < 0)
		report($E{Internal cannot fork});

	if  (filtpid == 0)  {
		close(pfs[1]);
		close(0);	/*  Cfile I think  */
		Ignored_error = dup(pfs[0]);	/*  Should be 0 now */
		close(pfs[0]);
		exec_prep(pfile, errfd); /* Sets setpgrp */
		set_signal(SIGPIPE, SIG_DFL);
		path_execute("FILTER", filternam, (in_params.pi_flags & PI_EXFILTPROG)? 1: 0);
		exit(255);
	}
	close(pfs[0]);	/*  Close read side  */
	ffile = fdopen(pfs[1], "w");
	if  (ffile == (FILE *) 0)
		report($E{Cannot fdopen pipe});
}

void  fpush(const int ch)
{
	if  (ffile)  {
		if  (Num_sent % in_params.pi_obuf == 0)
			Job_seg.dptr->js_serial++;
		Num_sent++;
		putc(ch, ffile);
	}
}

/* Close down pipe.  Return SPD_DONE if no errors, SPD_DERR if error
   file has stuff in it.  Don't bother with error file if
   FC_NOERR set.  If FC_AB is set kill the process group.
   If FC_KILL is set kill it with SIGKILL */

int  filtclose(const int fcflags)
{
#ifndef	HAVE_WAITPID
	PIDTYPE	pid;
#endif
	int	status;
	struct	stat	sbuf;
	FILE	*ssfile;

	if  (filtpid <= 0)		/*  Wasn't running  */
		return  SPD_DONE;

	/* Set signal handling according to what we are doing.
	   Unless killing off, we watch for further kills */

	if  (fcflags & FC_KILL)  {
		holdorignore(SIGTERM); /* No point */
		kill(-filtpid, SIGKILL);
	}
	else  {
		filt_stop = 0;
		set_signal(DAEMSTOP, do_filt_stop);
		if  (fcflags & FC_ABORT)
			kill(-filtpid, SIGTERM);
	}

	if  (ffile)  {
		fclose(ffile);
		ffile = (FILE *) 0;
	}
	status = 255 << 8;	/* In case wait fails */

	/* We wait for the filter process to finish.
	   If we get an interrupt, check to see if we had
	   a stop signal and send SIGTERM, unless we've already
	   had SIGTERM or we've had more than one signal, in
	   which case send SIGKILL */

#ifdef	HAVE_WAITPID
	while  (waitpid(filtpid, &status, 0) < 0  &&  errno == EINTR)
		if  (filt_stop > 0)
			kill(-filtpid, (filt_stop > 1  ||  fcflags & FC_ABORT)? SIGKILL: SIGTERM);
#else
	while  ((pid = wait(&status)) != filtpid)  {
		if  (pid >= 0)
			continue;
		if  (errno != EINTR)
			break;
		if  (filt_stop > 0)
			kill(-filtpid, (filt_stop > 1  ||  fcflags & FC_ABORT)? SIGKILL: SIGTERM);
	}
#endif
	filtpid = -1;
	set_signal(DAEMSTOP, SIG_IGN);

	/* If nothing written to error file, leave it open for next
	   time - saves opening and closing them all the time.  */

	if  (status != 0 && (ssfile = fdopen(errfd, "a")))  {
		disp_str = filternam;
		disp_arg[0] = status & 255;
		disp_arg[1] = (status >> 8) & 255;
		fprint_error(ssfile, $E{Filter process gave error});
		fclose(ssfile);
		errfd = -1;
	}
	else  {
		if  (fstat(errfd, &sbuf) < 0 || sbuf.st_size <= 0)
			return  SPD_DONE;
		close(errfd);
		errfd = -1;
	}

	/* Rename error file, unless not bothering.  There is a slight
	   (I hope) risk that the thing already exists, but we'll
	   take it...  */

	if  (!(fcflags & FC_NOERR))
		Ignored_error = link(jerrf, mkspid(ERNAM, Cjob->spq_job));

	unlink(jerrf);
	return  SPD_DERR;
}

/* Port set up before we first open.  */

int  doportsu()
{
	if  ((childproc = fork()) == 0)  {
		/* We think that we have already connected fds 0 to 2
		   to /dev/null */
#ifdef	SETPGRP_VOID
		setpgrp();
#else
		setpgrp(0, getpid());
#endif
		set_signal(DAEMSTOP, SIG_DFL);
		set_signal(DAEMRST, SIG_DFL);
		path_execute("PORTSU", portsu, (in_params.pi_flags & PI_EXPORTSU)? 1: 0);
		exit(255);
	}
	if  (childproc < 0)
		return  0;
	return  exec_wait() == 0;
}

/* Open job file */

FILE *getjobfile(struct spq *jp, const int feedtype)
{
	if  (jp->spq_netid)
		return  net_feed(feedtype, jp->spq_netid, jp->spq_rslot, jp->spq_job);
	return  fopen(mkspid(SPNAM, jp->spq_job), "r");
}
