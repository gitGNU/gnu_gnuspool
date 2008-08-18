/* spd.c -- despooler process

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

#define	VOIDUSED	3
#include "config.h"
#include <stdio.h>
#include "incl_sig.h"
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#ifdef	USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifdef	HAVE_TERMIO_H
#include <termio.h>
#else
#include <sgtty.h>
#endif
#include <errno.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <pwd.h>
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
#include "files.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "initp.h"
#include "ecodes.h"
#include "ipcstuff.h"
#define	UCONST
#include "q_shm.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#ifdef	SHAREDLIBS
#include "xfershm.h"
#include "displayopt.h"
#endif

#define	INCPAGES	10	/* Incremental size */

#define	C_MASK	0177

int	closedev(void);
void	dobanner(struct spq *);
int	doportsu(void);
void	exec_prep(const int, const int);
int	exec_wait(void);
int	execorsend(char *, char *, unsigned, const ULONG, const int);
void	path_execute(char *, char *, const int);
void	filtopen(void);
void	fpush(const int);
void	init_bigletter(void);
void	outerr(const int);
void	nfreport(const int);
void	pchar(const int);
void	pflush(void);
void	report(const int);
int	filtclose(const int);
int	rinitfile(void);
int	opendev(void);
int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
extern RETSIGTYPE	catchoff(int);	/*  Function for catching SIGHUPS with  */
FILE *	getjobfile(struct spq *, const int);

extern	char	**environ;

extern	int	pfile,
		rfid,
		lfid,
		outb_ptr;

int	Ctrl_chan;

LONG	Num_sent,
	Pages_done,
	My_msgid;

#if  	defined(RUN_AS_ROOT) || defined(SHAREDLIBS)
uid_t	Daemuid;
#endif
#ifdef	SHAREDLIBS
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
uid_t	Realuid, Effuid;
struct	xfershm		*Xfer_shmp;
DEF_DISPOPTS;
#endif

/* These bits all relate to shared memory ops.  */

int	Ptrslot;		/*  This is index of Pptr in Job_seg.plist  */
union	{			/*  Dummy ones to save testing  */
	struct	jshm_hdr  jh;
	struct	pshm_hdr  ph;
	struct	spq	jq;
	struct	spptr	pq;
}  dummy;
struct	spq	*Cjob = &dummy.jq;	/*  Current job */
struct	spptr	*Pptr = &dummy.pq;	/*  Current ptr descr */
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
char	*formp, *prtp;
extern	PIDTYPE	childproc;
extern	struct	initpkt	in_params;

char	*initstr;		/*  Setup string  */
char	*haltstr;		/*  Halt string */
char	*dsstr;			/*  Doc start string */
char	*destr;			/*  Doc end string */
char	*bdsstr;		/*  Banner doc start string */
char	*bdestr;		/*  Banner doc end string */
char	*ssstr;			/*  Suff start string */
char	*sestr;			/*  Suff end string */
char	*psstr;			/*  Page start string */
char	*pestr;			/*  Page end string */
char	*abortstr;		/*  Abort string */
char	*restartstr;		/*  Restart string */
char	*filternam;		/*  Filter file name */
char	*netfilter;		/*  Network filter name */
char	*portsu;		/*  Port setup string */
char	*bannprog;		/*  Banner program */
char	*setupnam;		/*  Setup file name  */
char	*sttystring;		/*  Explicit stty string */
char	*devnam;		/*  Device name  */
char	*rdelim;		/*  Record delimiter */
char	*pathn;			/*  Full path name of device  */

/* Things to stick in the environment.  */

#define	HNSIZE	15		/* Max length of host name */

char	Ev_spptr[PTRNAMESIZE+1+9]	= "SPOOLPTR=";
char	Ev_sph[MAXTITLE+1+9]		= "SPOOLHDR=none";
char	Ev_spu[5+1+10]			= "SPOOLUSER=0";
char	Ev_spjun[10+1+12]		= "SPOOLJUNAME=";
char	Ev_sppun[10+1+12]		= "SPOOLPUNAME=";
char	Ev_spf[MAXFORM+1+10]		= "SPOOLFORM=none";
char	Ev_spfl[MAXFLAGS+1+11]		= "SPOOLFLAGS=";
char	Ev_sppg[10+1+10]		= "SPOOLPAGE=";
char	Ev_spjob[9+1+10]		= "SPOOLJOB=";
char	Ev_sphost[10+1+HNSIZE]		= "SPOOLHOST=";
char	Ev_sprange[10+1+10+11]		= "SPOOLRANGE=";
char	Ev_spoe[1+1+8]			= "SPOOLOE=";
char	Ev_spcps[9+1+3]			= "SPOOLCPS=";

char	*Ev_spd, *prtnam = Ev_spptr+9;
char	*Ep_sph = Ev_sph+9, *Ep_spu = Ev_spu+10;
char	*Ep_spjun = Ev_spjun+12, *Ep_sppun = Ev_sppun+12;
char	*formnam = Ev_spf+10;
char	*Ep_spfl = Ev_spfl+11, *Ep_sppg = Ev_sppg+10;
char	*Ep_spjob = Ev_spjob + 9, *Ep_sphost = Ev_sphost + 10;
char	*Ep_range = Ev_sprange+11, *Ep_spoe = Ev_spoe+8, *Ep_spcps = Ev_spcps + 9;

FILE	*Cfile, *ffile;
struct	pages	pfe;

char	*ptdir,
	*daeminit,
	*shellname;

char	jerrf[NAMESIZE+1];	/*  Error messages  */

jmp_buf	stopj, restj;

#define	PAGE_FF		0
#define	PAGE_SETUP	1

char		job_pagestat;
ULONG	lastslot = (ULONG) ~0;
jobno_t		lastjob  = -1;

struct	spr_req	rq, reply;

#define	OREQ	rq.spr_un.o
#define	CRESP	reply.spr_un.c.spr_c

extern	int	errfd;

unsigned	pagenums;
LONG		*pageoffsets;

/* Send a message back to the scheduler */

void	sendrep(const int code)
{
	reply.spr_un.c.spr_act = (USHORT) code;
	msgsnd(Ctrl_chan, (struct msgbuf *) &reply, sizeof(struct sp_cmsg), 0);
}

void	set_pstate(const unsigned code)
{
	Pptr->spp_state = code;
	Ptr_seg.dptr->ps_serial++;
}

void	set_pstate_exit(const unsigned code)
{
	set_pstate(code);
	Pptr->spp_dflags = 0;		/*  In case left around */
	Cjob->spq_dflags &= ~SPQ_PQ;
	Job_seg.dptr->js_serial++;
	sendrep(SPD_DFIN);
	exit(0);
}

void	set_pstate_notify(const unsigned code)
{
	set_pstate(code);
	sendrep(SPD_SCH);
}

void	seterrorstate(const char *msg)
{
	if  (msg)
		strncpy(Pptr->spp_feedback, msg, PFEEDBACK);
	set_pstate_exit(SPP_ERROR);
}

void	setofflinestate(void)
{
	set_pstate_exit(SPP_OFFLINE);
}

/* Open job segments. */

void	initjseg(void)
{
#ifdef	USING_MMAP
	char	*fname = mkspdirfile(JIMMAP_FILE);
	void	*segp;
	int	fd = open(fname, O_RDWR);
	free(fname);
	if  (fd < 0)
		report($E{spd no jshm});
	Job_seg.iinf.mmfd = fd;
	fcntl(fd, F_SETFD, 1);
	Job_seg.iinf.segsize = Job_seg.iinf.reqsize = lseek(fd, 0L, 2);
	if  ((segp = mmap(0, Job_seg.iinf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
		report($E{spd no jshm});
	Job_seg.iinf.seg = segp;
#else
	/* Get details of info seg */
	Job_seg.iinf.base = JSHMID;
	Job_seg.iinf.segsize = sizeof(struct jshm_hdr) + 2 * SHM_JHASHMOD * sizeof(LONG);
	if  ((Job_seg.iinf.chan = shmget(JSHMID, 0, 0)) < 0  ||
	     (Job_seg.iinf.seg = shmat(Job_seg.iinf.chan, (char *) 0, 0)) == (char *) -1)
		report($E{spd no jshm});
#endif
	Job_seg.dptr = (struct jshm_hdr *) Job_seg.iinf.seg;
	Job_seg.hashp_jno = (LONG *) (Job_seg.iinf.seg + sizeof(struct jshm_hdr));
	Job_seg.hashp_jid = (LONG *) ((char *) Job_seg.hashp_jno + SHM_JHASHMOD * sizeof(LONG));

#ifdef	USING_MMAP
	Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.dptr->js_did;
	Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
	fname = mkspdirfile(JDMMAP_FILE);
	fd = open(fname, O_RDWR);
	free(fname);
	if  (fd < 0)
		report($E{spd no jshm});
	Job_seg.dinf.mmfd = fd;
	fcntl(fd, F_SETFD, 1);
	if  ((segp = mmap(0, Job_seg.dinf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
		report($E{spd no jshm});
	Job_seg.dinf.seg = segp;
#else
	/* Get details of data seg */
	Job_seg.dinf.base = Job_seg.dptr->js_did;
	Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
	Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.Njobs * sizeof(Hashspq);
	if  ((Job_seg.dinf.chan = shmget((key_t) Job_seg.dinf.base, 0, 0)) < 0  ||
	     (Job_seg.dinf.seg = shmat(Job_seg.dinf.chan, (char *) 0, 0)) == (char *) -1)
		report($E{spd no jshm});
#endif
	Job_seg.jlist = (Hashspq *) Job_seg.dinf.seg;
	Job_seg.Last_ser = 0;
}

/* The scheduler sends messages if it has to reallocate the shared
   memory segment used to hold the printer list or job queue. */

void	remap(void)
{
#ifdef	USING_MMAP

	/* We don't actually need to do this for printers because "our" onee
	   won't have moved. But let's be consistent. */

	if  (Ptr_seg.inf.segsize != Job_seg.dptr->js_psegid)  {

		char	Oldstate = Pptr->spp_state;
		unsigned  char	Oldflags = Pptr->spp_dflags;

		munmap(Ptr_seg.inf.seg, Ptr_seg.inf.segsize);
		Ptr_seg.inf.segsize = Ptr_seg.inf.reqsize = Job_seg.dptr->js_psegid;
		if  ((Ptr_seg.inf.seg = mmap(0, Ptr_seg.inf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, Ptr_seg.inf.mmfd, 0)) == MAP_FAILED)
			report($E{spd no pshm});

		Ptr_seg.dptr = (struct pshm_hdr *) Ptr_seg.inf.seg;
		Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;
		Ptr_seg.hashp_pid = (LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
		Ptr_seg.plist = (Hashspptr *) ((char *) Ptr_seg.hashp_pid + SHM_PHASHMOD*sizeof(LONG));
		Pptr = &Ptr_seg.plist[Ptrslot].p;
		Pptr->spp_state = Oldstate;
		Pptr->spp_dflags = Oldflags;
		devnam = Pptr->spp_dev;
		prtp = Pptr->spp_ptr;
		formp = Pptr->spp_form;
	}

	if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)  {
		munmap(Job_seg.dinf.seg, Job_seg.dinf.segsize);
		Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.dptr->js_did;
		Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
		if  ((Job_seg.dinf.seg = mmap(0, Job_seg.dinf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, Job_seg.dinf.mmfd, 0)) == MAP_FAILED)
			report($E{spd no jshm});
		Job_seg.jlist = (Hashspq *) Job_seg.dinf.seg;
		Cjob = (Pptr->spp_jslot >= 0)? &Job_seg.jlist[Pptr->spp_jslot].j : &dummy.jq;
	}
#else
	if  (Ptr_seg.inf.base != Job_seg.dptr->js_psegid)  {

		char	Oldstate = Pptr->spp_state;
		unsigned  char	Oldflags = Pptr->spp_dflags;

		Ptr_seg.inf.base = Job_seg.dptr->js_psegid;
		shmdt((char *) Ptr_seg.inf.seg);	/*  Lose old one  */

		if  ((Ptr_seg.inf.chan = shmget((key_t) Ptr_seg.inf.base, 0, 0)) <= 0  ||
		     (Ptr_seg.inf.seg = shmat(Ptr_seg.inf.chan, (char *) 0, 0)) == (char *) -1)
			report($E{spd no pshm});

		Ptr_seg.dptr = (struct pshm_hdr *) Ptr_seg.inf.seg;
		Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;
		Ptr_seg.inf.segsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Ptr_seg.Nptrs * sizeof(Hashspptr);
		Ptr_seg.hashp_pid = (LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
		Ptr_seg.plist = (Hashspptr *) ((char *) Ptr_seg.hashp_pid + SHM_PHASHMOD*sizeof(LONG));
		Pptr = &Ptr_seg.plist[Ptrslot].p;
		Pptr->spp_state = Oldstate;
		Pptr->spp_dflags = Oldflags;
		devnam = Pptr->spp_dev;
		prtp = Pptr->spp_ptr;
		formp = Pptr->spp_form;
	}

	if  (Job_seg.dinf.base != Job_seg.dptr->js_did)  {
		Job_seg.dinf.base = Job_seg.dptr->js_did;
		Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
		Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.Njobs * sizeof(Hashspq);
		shmdt((char *) Job_seg.dinf.seg);	/*  Lose old one  */
		if  ((Job_seg.dinf.chan = shmget((key_t) Job_seg.dinf.base, 0, 0)) <= 0  ||
		     (Job_seg.dinf.seg = shmat(Job_seg.dinf.chan, (char *) 0, 0)) == (char *) -1)
			report($E{spd no jshm});
		Job_seg.jlist = (Hashspq *) Job_seg.dinf.seg;
		Cjob = (Pptr->spp_jslot >= 0)? &Job_seg.jlist[Pptr->spp_jslot].j : &dummy.jq;
	}
#endif
}

/* Receive a message from the scheduler */

void	getreq(struct spr_req *buf)
{
	do  {
		if  (msgrcv(Ctrl_chan,
			    (struct msgbuf *) buf,
			    sizeof(struct spr_req) - sizeof(long), /* I did mean lower-case "long" */
			    My_msgid, 0) >= 0)
			return;
	}  while  (errno == EINTR);

	if  (errno == EIDRM)
		set_pstate_exit(SPP_HALT);
	if  (buf == &rq)
		report($E{spd bad packet});
	report($E{spd bad swait});
}

/* Catch restart messages.  */

RETSIGTYPE	catchrst(int n)
{
#ifdef	HAVE_SIGACTION
#ifndef	SA_NODEFER
	sigset_t	nset;
	sigemptyset(&nset);
	sigaddset(&nset, n);
	sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *) 0);
#endif
#elif	defined(STRUCT_SIG)
	sigsetmask(sigsetmask(~0) & ~sigmask(n));
#elif	defined(HAVE_SIGSET)
	sigrelse(n);
#elif	defined(UNSAFE_SIGNALS)
	signal(n, SIG_IGN);
#endif
	longjmp(restj, 1);
}

/* Catch abort messages.  */

RETSIGTYPE	stopit(int n)
{
#ifdef	HAVE_SIGACTION
#ifndef	SA_NODEFER
	sigset_t	nset;
	sigemptyset(&nset);
	sigaddset(&nset, n);
	sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *) 0);
#endif
#elif	defined(STRUCT_SIG)
	sigsetmask(sigsetmask(~0) & ~sigmask(n));
#elif	defined(HAVE_SIGSET)
	sigrelse(n);
#elif	defined(UNSAFE_SIGNALS)
	signal(n, SIG_IGN);
#endif
	Pptr->spp_dflags |= SPP_HADAB;
	longjmp(stopj, 1);
}

/* Hold or ignore signal */

void	holdorignore(const int signum)
{
#ifdef	HAVE_SIGACTION
	sigset_t	nset;
	sigemptyset(&nset);
	sigaddset(&nset, signum);
	sigprocmask(SIG_BLOCK, &nset, (sigset_t *) 0);
#elif	defined(STRUCT_SIG)
	sigsetmask(sigsetmask(~0) | sigmask(signum));
#elif	defined(HAVE_SIGSET)
	sighold(signum);
#else
	signal(signum, SIG_IGN);
#endif
}

#ifndef	UNSAFE_SIGNALS
void	unhold(const int signum)
{
#ifdef	HAVE_SIGACTION
	sigset_t	nset;
	sigemptyset(&nset);
	sigaddset(&nset, signum);
	sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *) 0);
#elif	defined(STRUCT_SIG)
	sigsetmask(sigsetmask(~0) & ~sigmask(signum));
#elif	defined(HAVE_SIGSET)
	sigrelse(signum);
#endif
}
#endif

void	set_signal(const int signum, RETSIGTYPE (*val)(int))
{
#ifdef	STRUCT_SIG
	struct	sigstruct_name  zs;
#ifdef	HAVE_SIGACTION
	sigset_t	nset;
	sigemptyset(&nset);
	sigaddset(&nset, signum);
#endif
	zs.sighandler_el = val;
	sigmask_clear(zs);
	zs.sigflags_el = SIGVEC_INTFLAG | SIGACT_INTSELF;
	sigact_routine(signum, &zs, (struct sigstruct_name *) 0);
#ifdef	HAVE_SIGACTION
	sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *) 0);
#else
	sigsetmask(sigsetmask(~0) & ~sigmask(signum));
#endif
#else  /* !STRUCT_SIG */
	signal(signum, val);
#ifdef	HAVE_SIGSET
	sigrelse(signum);
#endif /* HAVE_SIGSET */
#endif /* Type of signal */
}

/* Add character count to the record, print log file entry if needed.  */

void	addrecord(void)
{
	/* Charge extra if one copy only but the guy asked for more */

	if  (in_params.pi_flags & PI_NOCOPIES  &&  Cjob->spq_cps > 1)
		Num_sent *= (LONG) Cjob->spq_cps;

	if  (rfid >= 0)  {
		LONG	previous;

		lseek(rfid, (long) in_params.pi_offset, 0);
		if  (read(rfid, (char *)&previous, sizeof(previous)) != sizeof(previous))
			previous = 0;
		previous += Num_sent;
		lseek(rfid, (long) in_params.pi_offset, 0);
		write(rfid, (char *)&previous, sizeof(previous));
	}

	if  (lfid >= 0)  {
		time_t	finishtime = time((time_t *)0);

		/* Note that elapsed time is from job submission as I
		   thought this more useful - JMC
		   Put "starttime" in if you prefer.  */

		time_t	elapsed = finishtime - (time_t) Cjob->spq_time;
		unsigned	lng;
		int	hour, min, sec, mon, mday;
		struct	tm	*tp;
		char	buffer[200];	/*  Hope enough - groan */
		static	char	*unnamed;

		if  (!unnamed)
			unnamed = gprompt($P{spd unnamed});

		tp = localtime(&finishtime);
		hour = tp->tm_hour;
		min = tp->tm_min;
		sec = tp->tm_sec;
		finishtime = Cjob->spq_starttime; /* Needed if it's 64-bit */
		tp = localtime(&finishtime);
		mon = tp->tm_mon+1;
		mday = tp->tm_mday;

		/* Keep those dyslexic pirates at SCH happy by
		   swapping round days and months if > 4 hours West */

#ifdef	HAVE_TM_ZONE
		if  (tp->tm_gmtoff <= -4 * 60 * 60)  {
#else
		if  (timezone >= 4 * 60 * 60)  {
#endif
			mday = mon;
			mon = tp->tm_mday;
		}

		/* If this thing crosses midnight the finish date will
		   be a lie, but I really think it is acceptable.  */

#ifdef CHARSPRINTF
		sprintf(buffer,
#else
		lng = sprintf(buffer,
#endif
"%.2d/%.2d|%.2d:%.2d:%.2d|%.2d:%.2d:%.2d|%ld:%.2d:%.2d|%s|%s|%s|%ld|%s|%s|%d|%ld|%s%s%ld\n",
		mday, mon,
		tp->tm_hour, tp->tm_min, tp->tm_sec,
		hour, min, sec,
		(long) (elapsed / 3600),
		(int) ((elapsed % 3600) / 60),
		(int) (elapsed % 60),
		Cjob->spq_uname,
		Cjob->spq_file[0]? Cjob->spq_file: unnamed,
		formnam,
		(long) Num_sent,
		devnam,
		prtp,
		Cjob->spq_pri,
		(long) Pages_done,
		Cjob->spq_netid? look_host(Cjob->spq_netid): "",
		Cjob->spq_netid? ":": "",
		(long) Cjob->spq_job);
#ifdef CHARSPRINTF
		lng = strlen(buffer);
#endif
		/* The reason we don't use fprintf is because another
		   "spd" might be writing to the same file, and
		   fprintf might perform several writes causing
		   mixed output.  */

		write(lfid, buffer, lng);
	}
	CRESP.spc_chars = Num_sent;
	Num_sent = 0;
	Pages_done = 0;
}

/* This is where we initialise the printer.
   Terminate (in seterrorstate) if we fail  */

void	startup_state(void)
{
	set_pstate_notify(SPP_INIT);

	/* Read Setup file, do port setup string if applicable,
	   open device. If anything goes wrong, enter error state. */

	if  (!rinitfile()  ||  (portsu  &&  !doportsu())  ||  !opendev())
		seterrorstate((const char *) 0);

	Pptr->spp_dflags = setupnam? SPP_REQALIGN : 0;

	/* Send setup and suffix start strings.
	   If we get an interrupt, halt */

	if  (!execorsend("SETUP", initstr, in_params.pi_setup, in_params.pi_flags & PI_EX_SETUP, 0)  ||
	     !execorsend("SUFFST", ssstr, in_params.pi_sufstart, in_params.pi_flags & PI_EX_SUFST, 0))
		set_pstate_exit(SPP_HALT);

	if  (in_params.pi_flags & PI_REOPEN)
		closedev();

	set_pstate_notify(SPP_WAIT);
}

/* Throw a page or try to do what we have to do to get to the top of
   form. ***WARNING*** not very accurate in abort or restart
   cases where we don't have one off of a sequence as we don't
   know where in the last buffersworth the signal came.  */

void	pagethrow(int lcnt)
{
	int	rc, cc, nd, nc;
	char	*str;

	if  (rdelim)  {
		str = rdelim;
		nd = pfe.delimnum;
		nc = pfe.deliml;
	}
	else  {
		str = "\f";
		nd = DEF_RCOUNT;
		nc = 1;
	}
	if  (nc != 1  ||  str[0] != '\n')
		lcnt = 0;

	for  (rc = lcnt;  rc < nd;  rc++)
		for  (cc = 0;  cc < nc;  cc++)
			pchar(str[cc]);
}

/* single sheets - await operator.  Return 1 if ok 0 if halting -1 if job deleted */

int	swait(void)
{
	struct	spr_req	nreply;

	set_pstate_notify(SPP_OPER);

	for  (;;)  {
		getreq(&nreply);

		switch  (nreply.spr_un.o.spr_act)  {
		default:
			disp_arg[0] = nreply.spr_un.o.spr_act;
			report($E{spd invalid swait code});

		case  SP_REMAP:
			remap();
			continue;

		case  SP_FIN:	/*  Terminate  */
			return  0;

		case  SP_PYES:
		case  SP_PNO:
			set_pstate_notify(SPP_RUN);
			return  1;

		case  SP_PAB:
			return  -1;
		}
	}
}

/* New paper - print out setup file and await reply.
   Return 1 if all done OK, 0 if terminated, -1 if aborted */

int	pwait(void)
{
	int	ch;
	struct	spr_req	nreply;
	FILE	*setupfl = (FILE *) 0;

	if  (!setupnam)
		return  1;

 rerun_align:
	for  (;;)  {
		if  (!execorsend("DOCST", dsstr, in_params.pi_docstart, in_params.pi_flags & PI_EX_DOCST, 0))
			return  0;

		if  (in_params.pi_flags & PI_EX_ALIGN)  {

			if  (pfile < 0  &&  !opendev())
				seterrorstate((const char *) 0);

			if  ((childproc = fork()) == 0)  {
				exec_prep(pfile, pfile);
				set_signal(SIGHUP, SIG_DFL);
				set_signal(SIGALRM, SIG_DFL);
				set_signal(SIGPIPE, SIG_DFL);
				path_execute("ALIGN", setupnam, (in_params.pi_flags & PI_EXEXALIGN)? 1: 0);
				exit(255);
			}
			else  if  (exec_wait() != 0)
				return  1;
		}
		else  {
			/* Open file, skip if we can't find it */

			if  (!(setupfl = fopen(setupnam, "r")))
				return  1;

			if  (filternam)  {
				filtopen();
				while  ((ch = getc(setupfl)) != EOF)
					putc(ch, ffile);
				fclose(setupfl);
				if  (filtclose(FC_NOERR) != SPD_DONE)
					return  0;
			}
			else  {
				int	lcnt = 0;
				while  ((ch = getc(setupfl)) != EOF)  {
					if  (ch == '\n')
						lcnt++;
					pchar(ch);
				}
				fclose(setupfl);
				if  (!destr)
					pagethrow(lcnt);
				pflush();
			}
		}

		if  (!execorsend("DOCEND", destr, in_params.pi_docend, in_params.pi_flags & PI_EX_DOCEND, 0))
			return  0;

		pflush();
		set_pstate_notify(SPP_OPER);

		/* Close device to force out last characters if reopen
		   set (buggette fixette 18/7/91).  */

		if  (in_params.pi_flags & PI_REOPEN)
			closedev();

		for  (;;)  {
			getreq(&nreply);

			switch  (nreply.spr_un.o.spr_act)  {
			case  SP_REMAP:
				remap();
				continue;

			default:
				disp_arg[0] = nreply.spr_un.o.spr_act;
				report($E{spd invalid pwait code});

			case  SP_FIN:	/*  Terminate  */
				set_pstate_exit(SPP_HALT);

			case  SP_PYES:
				set_pstate(SPP_RUN);
				Pptr->spp_dflags &= ~SPP_REQALIGN;
				return  1;

			case  SP_PNO:
				goto  rerun_align;

			case  SP_PAB:
				return  -1;
			}
		}
	}
}

/* Compage page delimiters etc from file with ones read from setup file Use the latter if it exists */

int	comp_dels(const char *fdelim)
{
	if  (pfe.delimnum != in_params.pi_rcount  ||  pfe.deliml != in_params.pi_rcstring - 1)
		return  0;
	return  BLOCK_CMP(rdelim, fdelim, pfe.deliml);
}

/* Where we have a delimiter in the setup file and we don't have a
   page file or we don't believe it, regenerate page file from
   delimiters and text or go through the motions of so doing on
   remote jobs.  */

void	scanpages(struct spq *jp)
{
	char	*rcp;
	int	ch;
	FILE	*inf;
	char	*pfn;
	char	*rcdend;
	int	rec_cnt, pgfid;
	LONG	char_cnt, page_cnt, outlng;

	pfe.delimnum = in_params.pi_rcount;
	pfe.deliml = in_params.pi_rcstring - 1;
	pfe.lastpage = 0;

	/* Assume that this is always called after rdpgfile() which
	   will set up an initial vector of pageoffsets whatever
	   else happens.  */

	rcp = rdelim;
	rcdend = rdelim + in_params.pi_rcstring - 1;
	rec_cnt = 0L;
	page_cnt = 0L;
	char_cnt = 0L;

	if  ((inf = getjobfile(Cjob, FEED_NPSP)))  {
		while  ((ch = getc(inf)) != EOF)  {
			char_cnt++;
			if  (ch == *rcp)  {	/* Matches 1st char */
				if  (++rcp >= rcdend)  {
					if  (++rec_cnt >= in_params.pi_rcount)  {
						if  (++page_cnt + 1 >= pagenums)  {
							pagenums += INCPAGES;
							pageoffsets = (LONG *) realloc((char *) pageoffsets, sizeof(LONG) * pagenums);
							if  (pageoffsets == (LONG *) 0)
								nomem();
						}
						pageoffsets[page_cnt] = char_cnt;
						rec_cnt = 0L;
					}
					rcp = rdelim;
				}
			}
			else  if  (rcp > rdelim)  {
				char	*pp, *prcp, *prevpl;

				/* Back up along delimiter to where we were */

				prevpl = --rcp;	/*  Last one matched  */
				for  (;  rcp > rdelim;  rcp--)  {
					if  (*rcp != ch)
						continue;
					pp = prevpl;
					prcp = rcp - 1;
					for  (;  prcp >= rdelim;  pp--, prcp--)
						if  (*pp != *prcp)
							goto  rej;
					rcp++;
					break;
				rej:	;
				}
			}
		}
		fclose(inf);
	}

	pageoffsets[0] = 0L;
	pageoffsets[page_cnt+1] = jp->spq_size;
	outlng = (page_cnt+1) * sizeof(LONG);

	if  (pageoffsets[page_cnt] < jp->spq_size)  {
		page_cnt++;
		pfe.lastpage = in_params.pi_rcount - rec_cnt;
	}

	jp->spq_npages = Cjob->spq_npages = page_cnt;
	Job_seg.dptr->js_serial++;

	/* Generate a new file on local jobs */

	if  (!jp->spq_netid)  {
		pfn = mkspid(PFNAM, jp->spq_job);
		if  ((pgfid = open(pfn, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
			return;

		write(pgfid, (char *) &pfe, sizeof(struct pages));
		write(pgfid, rdelim, (unsigned) (in_params.pi_rcstring - 1));
		write(pgfid, (char *) &pageoffsets[1], (unsigned) outlng);
		close(pgfid);

		/* Mark job to indicate that we have assigned a page
		   file.  We don't generally broadcast it as we
		   can pick up this information as we want it.  */

		jp->spq_dflags |= SPQ_PAGEFILE;
	}
}

/* For each job classify as follows in job_pagestat

	PAGE_SETUP	details of pages in pageoffsets
			any job with non-formfeed delims
			may need to initially read in job
			if nonexistent/incompatible page file
	PAGE_FF		note formfeeds on the fly */

void	page_anal(struct spq *jp)
{
	char	*fdelim;

	/* First look and see if current form type has a specified delimiter. */

	if  (in_params.pi_rcstring)  {

		/* See if job came with a page file and use that if we think it fits.  */

		int	ret = rdpgfile(jp, &pfe, &fdelim, &pagenums, &pageoffsets);
		if  (ret < 0)
			nomem();
		if  (ret > 0)  {

			/* There was a page file.  Compare delimiter
			   we just read with the delimiter we are
			   proposing to use. If it matches,
			   believe the page file.  Otherwise scan
			   job and generate page offsets vector
			   ourselves */

			if  (!comp_dels(fdelim))
				scanpages(jp);
		}
		else
			scanpages(jp);

		job_pagestat = PAGE_SETUP;
	}
	else  {
		int	ret;

		/* We haven't got a page delimiter with the form type,
		   but one might have come with the file. If so use that. */

		if  (rdelim)  {	/* Might have some fossil left over */
			free(rdelim);
			rdelim = (char *) 0;
		}

		ret = rdpgfile(jp, &pfe, &fdelim, &pagenums, &pageoffsets);
		if  (ret < 0)
			nomem();
		if  (ret > 0)  {
			job_pagestat = PAGE_SETUP;
			rdelim = fdelim;
		}
		else  {
			job_pagestat = PAGE_FF;	/* Do it on the fly. */
			pfe.delimnum = DEF_RCOUNT;
			pfe.deliml = 1;
			pfe.lastpage = 0;
		}
	}
}

/* Seek up from from to to.  To take care of remote jobs where we
   can't seek.  Return to.  */

LONG	pfseek(FILE *ifl, LONG from, LONG to)
{
	if  (Cjob->spq_netid == 0)
		fseek(ifl, (long) to, 0);
	else  while  (from < to)  {
		getc(ifl);
		from++;
	}
	return  to;
}

/* Continue with rest of job.
   Return 1 if OK, 0 if not (because one of the page send things
   detected an abort signal */

int	contj(FILE *ifl, LONG initpage)
{
	int	ch;
	void	(*pfunc)() = pchar;
	LONG	char_cnt, endofpage;
	ULONG	page_cnt;

	/* Open filter if necessary (whoops!!!)  */

	if  (filternam)  {
		filtopen();
		pfunc = fpush;
	}

	/* Protect against stupid initial page (Do this after opening
	   filter to be consistent about whether we start it or not). */

	if  (initpage > Cjob->spq_npages)
		return  1;

	/* Cover 'insist on page 1' case.  PI_NORANGES won't be set
	   because that would cause initpage to be 0.  */

	if  (initpage > 0L  &&  in_params.pi_flags & PI_INCLP1)  {
		page_cnt = 0;
		char_cnt = pfseek(ifl, 0L, pageoffsets[0]);
		sprintf(Ep_sppg, "%lu", (unsigned long) (page_cnt+1));
		if  (!execorsend("PSTART", psstr, in_params.pi_pagestart, in_params.pi_flags & PI_EX_PAGESTART, 1))
			return  0;
		endofpage = pageoffsets[page_cnt+1];
		Cjob->spq_pagec = page_cnt;
		Job_seg.dptr->js_serial++;
		while  (char_cnt < endofpage)  {
			if  ((ch = getc(ifl)) == EOF)
				break;
			char_cnt++;
			Cjob->spq_posn = char_cnt;
			(*pfunc)(ch);
		}
		Pages_done++;
#ifdef	USING_MMAP
		msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
		if  (!execorsend("PEND", pestr, in_params.pi_pageend, in_params.pi_flags & PI_EX_PAGEEND, 1))
			return  0;
	}

	page_cnt = initpage;
	char_cnt = pfseek(ifl, 0L, pageoffsets[initpage]);

	while  (char_cnt < Cjob->spq_size  &&  (page_cnt <= Cjob->spq_end || in_params.pi_flags & PI_NORANGES))  {

		Cjob->spq_haltat = page_cnt > in_params.pi_windback? page_cnt - in_params.pi_windback: 0L;

		if  (page_cnt & 1)  {
			if  (Cjob->spq_jflags & SPQ_EVENP)  {
				++page_cnt;
				if  (page_cnt >= Cjob->spq_npages)
					return  1;
				char_cnt = pfseek(ifl, char_cnt, pageoffsets[page_cnt]);
				continue;
			}
		}
		else  if  (Cjob->spq_jflags & SPQ_ODDP)  {
			++page_cnt;
			if  (page_cnt >= Cjob->spq_npages)
				return  1;
			char_cnt = pfseek(ifl, char_cnt, pageoffsets[page_cnt]);
			continue;
		}

		sprintf(Ep_sppg, "%lu", (unsigned long) (page_cnt+1));
		if  (!execorsend("PSTART", psstr, in_params.pi_pagestart, in_params.pi_flags & PI_EX_PAGESTART, 1))
			return  0;

		endofpage = pageoffsets[page_cnt+1];
		Cjob->spq_pagec = page_cnt;
		Job_seg.dptr->js_serial++;

		while  (char_cnt < endofpage)  {
			if  ((ch = getc(ifl)) == EOF)  {
#ifdef	USING_MMAP
				msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
				return  execorsend("PEND", pestr, in_params.pi_pageend, in_params.pi_flags & PI_EX_PAGEEND, 1);
			}
			char_cnt++;
			Cjob->spq_posn = char_cnt;
			(*pfunc)(ch);
		}
		Pages_done++;
#ifdef	USING_MMAP
		msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
		if  (!execorsend("PEND", pestr, in_params.pi_pageend, in_params.pi_flags & PI_EX_PAGEEND, 1))
			return  0;
		page_cnt++;
	}

	/* End of file, stick out any missing delimiters */

	if  (char_cnt >= Cjob->spq_size  &&  !destr  &&  !filternam)  {
		int	rc, cc;
		char	*str = rdelim;
		if  (!str)
			str = "\f";
		for  (rc = 0;  rc < pfe.lastpage;  rc++)
			for  (cc = 0;  cc < pfe.deliml;  cc++)
				pchar(str[cc]);
		Pages_done++;
	}

	return  1;
}

/* Continue with rest of job in case where we are using formfeeds as
   delimiters and we haven't pre-read the job to see where they
   come, preferring to believe the count that spr made.
   Return 1 ok 0 not ok (page start/end detected abort) */

int	contj_ff(FILE *ifl, LONG initpage)
{
	int	ch;
	int	lastch = -1;
	void	(*pfunc)() = pchar;
	LONG	char_cnt;
	ULONG	page_cnt;

	if  (filternam)  {
		filtopen();
		pfunc = fpush;
	}

	char_cnt = 0;
	page_cnt = 0;

	if  (initpage > 0L  &&  in_params.pi_flags & PI_INCLP1)  {
		sprintf(Ep_sppg, "%lu", (unsigned long) (page_cnt+1));
		if  (!execorsend("PSTART", psstr, in_params.pi_pagestart, in_params.pi_flags & PI_EX_PAGESTART, 1))
			return  0;
		Cjob->spq_pagec = page_cnt;
		Job_seg.dptr->js_serial++;
		do  {		/* To end of page */
			if  ((ch = getc(ifl)) == EOF)  {
#ifdef	USING_MMAP
				msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
				if  (!execorsend("PEND", pestr, in_params.pi_pageend, in_params.pi_flags & PI_EX_PAGEEND, 1))
					return  0;
				goto  endfile;
			}
			char_cnt++;
			Cjob->spq_posn = char_cnt;
			(*pfunc)(ch);
			lastch = ch;
		}  while  (ch != DEF_DELIM);

#ifdef	USING_MMAP
		msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
		if  (!execorsend("PEND", pestr, in_params.pi_pageend, in_params.pi_flags & PI_EX_PAGEEND, 1))
			return  0;

		page_cnt++;
		Pages_done++;
	}

	while  (page_cnt < initpage)  {
		if  ((ch = getc(ifl)) == EOF)
			return  1;
		if  (ch == DEF_DELIM)
			page_cnt++;
		char_cnt++;
	}

	while  (char_cnt < Cjob->spq_size  &&  (page_cnt <= Cjob->spq_end || in_params.pi_flags & PI_NORANGES))  {

		Cjob->spq_haltat = page_cnt > in_params.pi_windback? page_cnt - in_params.pi_windback: 0L;

		if  (Cjob->spq_jflags & (SPQ_EVENP|SPQ_ODDP))  {

			/* If the tests here look wrong it is of
			   course because pages are held
			   internally as 0 to n-1 where the human
			   reader sees 1 to n */

			if  ((Cjob->spq_jflags & SPQ_EVENP  &&  page_cnt & 1)  ||
			     (Cjob->spq_jflags & SPQ_ODDP  &&  !(page_cnt & 1)))  {
			     for  (;;)  {
				     if  ((ch = getc(ifl)) == EOF)
					     goto  endfile;
				     char_cnt++;
				     if  (ch == DEF_DELIM)
					     break;
			     }
			     page_cnt++;
			     continue;	/* To main while loop */
			}
		}

		/* Update page count in environment, deal with page
		   start string/command.  */

		sprintf(Ep_sppg, "%lu", (unsigned long) (page_cnt+1));
		if  (!execorsend("PSTART", psstr, in_params.pi_pagestart, in_params.pi_flags & PI_EX_PAGESTART, 1))
			return  0;

		/* Update page count in job.  For network jobs the
		   feeder process will keep the "real" copy up to date. */

		Cjob->spq_pagec = page_cnt;
		Job_seg.dptr->js_serial++;

		do  {		/* To end of page */
			if  ((ch = getc(ifl)) == EOF)  {
#ifdef	USING_MMAP
				msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
				if  (!execorsend("PEND", pestr, in_params.pi_pageend, in_params.pi_flags & PI_EX_PAGEEND, 1))
					return  0;
				goto  endfile;
			}
			char_cnt++;
			Cjob->spq_posn = char_cnt;
			(*pfunc)(ch);
			lastch = ch;
		}  while  (ch != DEF_DELIM);

#ifdef	USING_MMAP
		msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
		if  (!execorsend("PEND", pestr, in_params.pi_pageend, in_params.pi_flags & PI_EX_PAGEEND, 1))
			return  0;
		page_cnt++;
		Pages_done++;
	}

 endfile:

	/* End of file, stick out final form feed if appropriate */

	if  (char_cnt >= Cjob->spq_size  &&  !destr  &&  !filternam  &&  lastch != DEF_DELIM)
		pchar(DEF_DELIM);

	/* If we've printed anything at all say we did a page.
	   Accuracy is a bit limited here but we do our best.  */

	if  (lastch >= 0  &&  lastch != DEF_DELIM)
		Pages_done++;
	return  1;
}

/* Process a print request */

int	proc_req(void)
{
	int	newjob = 0, r;
	LONG	initpage;
	FILE	*infile = (FILE *) 0;

	/* NB We aren't interruptable at the moment.
	   Remember not to do anything too extensive */

	set_pstate(SPP_RUN);

	if  (lastslot != OREQ.spr_jpslot  ||  lastjob != OREQ.spr_jobno)  {
		newjob++;

		/* Belt && braces stuff to ensure we don't poke into thin air.  */

		if  (Pptr->spp_jslot < 0 || Pptr->spp_jslot >= Job_seg.dptr->js_maxjobs || Job_seg.jlist[Pptr->spp_jslot].j.spq_job != OREQ.spr_jobno)  {
			disp_arg[0] = OREQ.spr_jobno;
			disp_arg[1] = Pptr->spp_jslot;
			disp_str = Pptr->spp_ptr;
			disp_str2 = Pptr->spp_dev;
			report($E{spd invalid job slot});
		}

		/* Point to current job slot in job shm */

		Cjob = &Job_seg.jlist[Pptr->spp_jslot].j;

		/* Copy bits of job into relevant environment
		   variables - the following are the invariant bits. */

		sprintf(Ep_spu, "%lu", (unsigned long) (Cjob->spq_uid));
		strcpy(Ep_spjun, Cjob->spq_uname);
		sprintf(Ep_spjob, "%ld", (long) Cjob->spq_job);
		if  (Cjob->spq_netid)
			strncpy(Ep_sphost, look_host(Cjob->spq_netid), HNSIZE-1);
		else
			Ep_sphost[0] = '\0';

		/* If not previously handled, record now as the point
		   at which we started to worry about this job.
		   Note that we don't allow users to mess around
		   with SPQ_STARTED - we do with SPQ_PRINTED
		   which was arguably a buggette.  */

		if  (!(Cjob->spq_dflags & SPQ_STARTED))  {
			Cjob->spq_starttime = time((time_t *) 0);
			Cjob->spq_dflags |= SPQ_STARTED;
		}

		/* Remember relevant bits for charging */

		CRESP.spc_user = Cjob->spq_uid;
		CRESP.spc_pri = Cjob->spq_pri;
		CRESP.spc_chars = Cjob->spq_size;

		lastslot = OREQ.spr_jpslot;
		lastjob = OREQ.spr_jobno;
	}

	/* Set up the other environment variables.  We do this every
	   time because the variables might change between copies. */

	strncpy(Ep_sph, Cjob->spq_file, MAXTITLE);
	strncpy(Ep_spfl, Cjob->spq_flags, MAXFLAGS);
	strcpy(Ep_sppun, Cjob->spq_puname);
	if  (Cjob->spq_start != 0)  {
		if  (Cjob->spq_end <= LOTSANDLOTS)
			sprintf(Ep_range, "%ld-%ld", Cjob->spq_start+1L, Cjob->spq_end+1L);
		else
			sprintf(Ep_range, "%ld-", Cjob->spq_start+1L);
	}
	else  if  (Cjob->spq_end <= LOTSANDLOTS)
		sprintf(Ep_range, "-%ld", Cjob->spq_end+1L);
	else
		*Ep_range = '\0';
	*Ep_spoe = Cjob->spq_jflags & SPQ_ODDP? '2': Cjob->spq_jflags & SPQ_EVENP? '1': '0';
	sprintf(Ep_spcps, "%u", (unsigned) Cjob->spq_cps);

	/* Has form type changed?  The scheduler worries about whether
	   the type matches up to the suffix.  */

	if  (strncmp(Cjob->spq_form, formnam, sizeof(Cjob->spq_form)) != 0)  {
		set_pstate_notify(SPP_INIT);

		/* Yes it has, send suffix end string for existing suffix
		   An interrupt here is taken as a halt */

		if  (!execorsend("SUFFEND", sestr, in_params.pi_sufend, in_params.pi_flags & PI_EX_SUFEND, 0))
			return  0;

		/* Set form type appropriately, both local copy, and copy in shared memory. */

		strncpy(formnam, Cjob->spq_form, MAXFORM+1);
		strncpy(formp, formnam, MAXFORM+1);

		/* Read setup file for new form type. */

		if  (!rinitfile())
			seterrorstate((const char *) 0);

		Pptr->spp_dflags = setupnam? SPP_REQALIGN: 0;
		if  (!execorsend("SUFFST", ssstr, in_params.pi_sufstart, in_params.pi_flags & PI_EX_SUFST, 0))
			return  0;

		set_pstate(SPP_RUN);
		newjob++;	/* Force new banner */
	}

	/* Deal with alignment page.  */

	if  (Pptr->spp_dflags & SPP_REQALIGN)  {
		newjob++;	/* Force new banner etc */
		if  ((r = pwait()) <= 0)
			return  0;
	}

	/* Printer now in running state - tell world */

	Cjob->spq_dflags |= SPQ_PQ;
	Cjob->spq_posn = 0L;
	Job_seg.dptr->js_serial++;

	sendrep(SPD_SCH);

	/* If new job or old job with new bits and pieces deal with banner etc */

	if  (newjob)  {
		page_anal(Cjob);
		dobanner(Cjob);
	}
	else  if  (in_params.pi_flags & PI_HDRPERCOPY)
		dobanner(Cjob);

	/* If single job mode, deal with that.  */

	if  (in_params.pi_flags & PI_SINGLE  &&  swait() <= 0)
		return  0;

	/* Set page number to 0 and put into environment so that we
	   have something reasonably sensible in SPOOLPAGE If
	   haltat page was set, start from there and set it to
	   zero as it only gets used once.  */

	sprintf(Ep_sppg, "%u", 0);
	if  (in_params.pi_flags & PI_NORANGES)
		initpage = 0;
	else  {
		initpage = Cjob->spq_start;
		if  (Cjob->spq_haltat)  {
			initpage = Cjob->spq_haltat;
			Cjob->spq_haltat = 0;
		}
	}

	/* Set up for abort messages so that they can "longjmp" here.  */

	if  (setjmp(stopj))  {

		holdorignore(DAEMSTOP);

		if  (infile)  {
			fclose(infile);
			infile = (FILE *) 0;
		}

		/* If we get a second or subsequent abort message
		   while we're still handling the code below,
		   then abort the closedown.  */

		if  (setjmp(stopj))  {
			set_signal(DAEMSTOP, SIG_IGN);
			filtclose(FC_NOERR|FC_ABORT|FC_KILL);
			return  0;
		}

		set_signal(DAEMSTOP, stopit);	/* Enable above code */
		addrecord();	/* Don't forget to keep log */

		if  (filternam)
			filtclose(FC_NOERR|FC_ABORT);
		else  {
			outb_ptr = 0;	/*  Zap previous contents */
			if  (abortstr)  {
				if  (!execorsend("ABORT", abortstr, in_params.pi_abort, in_params.pi_flags & PI_EX_ABORT, 0))
					return  0;
			}
			else  {
				outerr($E{aborted message});	/*  **ABORTED** message */
				if  (!destr)
					pagethrow(0);
			}
		}
		if  (!execorsend("DOCEND", destr, in_params.pi_docend, in_params.pi_flags & PI_EX_DOCEND, 0))
			return  0;
		pflush();
		set_signal(DAEMSTOP, SIG_IGN);
		return  0;
	}

	/* Repeat for restart messages */

	if  (setjmp(restj))  {
		addrecord();
		holdorignore(DAEMRST);
		if  (infile)  {
			fclose(infile);
			infile = (FILE *) 0;
		}
		Cjob->spq_posn = 0L;
		Job_seg.dptr->js_serial++;
		if  (filternam)  {
#ifdef	USING_MMAP
			msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
			if  (!execorsend("PEND", pestr, in_params.pi_pageend, in_params.pi_flags & PI_EX_PAGEEND, 0))
				return  0;
			filtclose(FC_NOERR|FC_ABORT);
			execorsend("DOCEND", destr, in_params.pi_docend, in_params.pi_flags & PI_EX_DOCEND, 0);
		}
		else  {
			outb_ptr = 0;	/* Zap previous contents */
			if  (restartstr)
				execorsend("RESTART", restartstr, in_params.pi_restart, in_params.pi_flags & PI_EX_RESTART, 0);
			else  {
				outerr($E{restarted message}); /* Restarting message */
				if  (destr)
					execorsend("DOCEND", destr, in_params.pi_docend, in_params.pi_flags & PI_EX_DOCEND, 0);
				else  {
					pagethrow(0);
					pflush();
				}
			}
		}
	}

	/* Emit document start string.  */

	execorsend("DOCST", dsstr, in_params.pi_docstart, in_params.pi_flags & PI_EX_DOCST, 0);

	/* Open job file - say done if not found, not aborted as we get into a loop otherwise.  */

	r = SPD_DONE;
	if  (!(infile = getjobfile(Cjob, FEED_SP)))
		goto  donej;

	set_signal(DAEMSTOP, stopit);	/* Enable above code for aborts */
	set_signal(DAEMRST, catchrst);	/* Enable above code for restarts */

	/* Do the business */

	r = job_pagestat == PAGE_SETUP? contj(infile, initpage) : contj_ff(infile, initpage);

	set_signal(DAEMSTOP, SIG_IGN);
	set_signal(DAEMRST, SIG_IGN);

	pflush();

	/* We now close the input file after every print. This is a
	   little more expensive, but we do it for 2 reasons:
	   1. We HAVE to for network files.  2. If the machine
	   crashes the last spool job will get left in the
	   lost+found directory; also the space is not released
	   for large jobs even if it doesn't crash.  */

	fclose(infile);
	infile = (FILE *) 0;
	if  (!r)  {
		filtclose(FC_NOERR|FC_ABORT);
		return  0;
	}
	r = filtclose(0);	/* Returns SPD_DERR if messages */
	if  (!execorsend("DOCEND", destr, in_params.pi_docend, in_params.pi_flags & PI_EX_DOCEND, 0))
		return  0;

	/* Reset "halt at" as we finished job.  Attend to odd/even page stuff */

	Cjob->spq_haltat = 0L;
	if  (Cjob->spq_jflags & SPQ_REVOE)  {
		if  (Cjob->spq_jflags & SPQ_ODDP)  {
			Cjob->spq_jflags &= ~SPQ_ODDP;
			Cjob->spq_jflags |= SPQ_EVENP;
		}
		else  {
			Cjob->spq_jflags &= ~SPQ_EVENP;
			Cjob->spq_jflags |= SPQ_ODDP;
		}
	}

	addrecord();
 donej:
	if  (in_params.pi_flags & PI_REOPEN  &&  !closedev())
		return  0;
	Cjob->spq_dflags |= SPQ_PRINTED;
	Cjob->spq_dflags &= ~SPQ_PQ;
	Job_seg.dptr->js_serial++;
	Pptr->spp_dflags = 0;	/* Make sure HADAB off */
	set_pstate(SPP_WAIT);
	sendrep(r);
	return  1;
}

/* Main printing state after getting through init stuff.  */

void	process(void)
{
	for  (;;)  {
		getreq(&rq);

		switch  (rq.spr_un.j.spr_act)  {
		case  SP_REMAP:
			remap();
			continue;

		default:
			disp_arg[0] = rq.spr_un.j.spr_act;
			report($E{spd bad request});

		case  SP_FIN:	/*  Terminate  */
			set_pstate_exit(SPP_HALT);

		case  SP_REQ:	/*  A request  */
			if  (proc_req())
				continue;
			if  (in_params.pi_flags & PI_REOPEN)
				closedev();
			Cjob->spq_dflags &= ~SPQ_PQ;
			Job_seg.dptr->js_serial++;
			Pptr->spp_dflags = 0;	/* Make sure HADAB off */
			set_pstate(SPP_WAIT);
			sendrep(SPD_DAB);
			continue;
		}
	}
}

/* Initialise environment.  */

void	envinit(void)
{
	char  **cp, **ncp;
	int	lng = 1+14;	/*  Null entry + 14 new ones  */
	char	**newenv;

	for  (cp = environ;  *cp;  cp++)
		lng++;
	if  ((newenv = (char **)malloc(lng*sizeof(char *))) == (char **) 0)
		nomem();

	/* Copy old environment, skipping matches of our friends.  */

	ncp = newenv;
	for  (cp = environ;  *cp;  cp++)  {
		if  (strncmp(*cp, "SPOOL", 5) == 0)  {
			char	*mp = *cp+5;
			if  (strncmp(mp, "HDR=", 4) == 0)
				continue;
			if  (strncmp(mp, "USER=", 5) == 0)
				continue;
			if  (strncmp(mp, "FORM=", 5) == 0)
				continue;
			if  (strncmp(mp, "FLAGS=", 6) == 0)
				continue;
			if  (strncmp(mp, "PTR=", 4) == 0)
				continue;
			if  (strncmp(mp, "JUNAME=", 7) == 0)
				continue;
			if  (strncmp(mp, "PUNAME=", 7) == 0)
				continue;
			if  (strncmp(mp, "DEV=", 4) == 0)
				continue;
			if  (strncmp(mp, "PAGE=", 5) == 0)
				continue;
			if  (strncmp(mp, "JOB=", 4) == 0)
				continue;
			if  (strncmp(mp, "HOST=", 5) == 0)
				continue;
			if  (strncmp(mp, "RANGE=", 6) == 0)
				continue;
			if  (strncmp(mp, "OE=", 3) == 0)
				continue;
			if  (strncmp(mp, "CPS=", 4) == 0)
				continue;
		}
		*ncp++ = *cp;
	}

	*ncp++ = Ev_spd;
	*ncp++ = Ev_spptr;
	*ncp++ = Ev_sph;
	*ncp++ = Ev_spu;
	*ncp++ = Ev_spjun;
	*ncp++ = Ev_sppun;
	*ncp++ = Ev_spf;
	*ncp++ = Ev_spfl;
	*ncp++ = Ev_sppg;
	*ncp++ = Ev_spjob;
	*ncp++ = Ev_sphost;
	*ncp++ = Ev_sprange;
	*ncp++ = Ev_spoe;
	*ncp++ = Ev_spcps;
	*ncp = (char *) 0;
	environ = newenv;
}

/* Ye olde main routine.
   I am now called with 2 arguments.

   1.	Printer name.
   2.	Offset in shm segment.

   Device/printer/formtype are all extracted from there. */

MAINFN_TYPE	main(int argc, char **argv)
{
#ifdef	USING_MMAP
	char	*fname;
#endif

	versionprint(argv, "$Revision: 1.1 $", 1);

	umask(C_MASK);
	init_mcfile();

	/* Ignore or hold signals until ready */

	Job_seg.dptr = &dummy.jh; 	/* Save testing in case we get "report" before we're set up */
	Ptr_seg.dptr = &dummy.ph;

	set_signal(SIGPIPE, SIG_IGN);

	set_signal(SIGHUP, SIG_IGN);
	set_signal(SIGALRM, SIG_IGN);

	set_signal(DAEMSTOP, SIG_IGN);
	set_signal(DAEMRST, SIG_IGN);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

#ifdef	CLOSE_SPD_FDS
#ifndef	DEBUG

	/* Clog up f.d.s 0, 1, 2 to stop pipes getting confused */

	close(0);
	close(1);
	close(2);
	dup(dup(open("/dev/null", O_RDWR)));
#endif
#endif
#ifdef	SETPGRP_VOID
	setpgrp();
#else
	setpgrp(0, getpid());
#endif

	if  ((Cfile = open_icfile()) == (FILE *) 0)
		exit(E_NOCONFIG);

	/* Initialise the table of big letters for banners.  */

	init_bigletter();

	ptdir = envprocess(PTDIR);
	shellname = envprocess(SHELL);

#ifdef	SHAREDLIBS
	Realuid = getuid();
	Effuid = geteuid();
#endif
#ifdef	RUN_AS_ROOT
	if  ((Daemuid = lookup_uname(SPUNAME)) == UNKNOWN_UID)
		Daemuid = ROOTID;
	setuid(Daemuid);
#endif

	if  (argc != 3)
		exit(E_USAGE);

	/* We pass the printer name as the 1st argument but we don't bother with it */

	Ptrslot = atoi(argv[2]);

	/* Grab job info segment which now gives the location of the printer seg */
	initjseg();
#ifdef	USING_MMAP
	Ptr_seg.inf.segsize = Ptr_seg.inf.reqsize = Job_seg.dptr->js_psegid;
	fname = mkspdirfile(PMMAP_FILE);
	Ptr_seg.inf.mmfd = open(fname, O_RDWR);
	free(fname);
	if  (Ptr_seg.inf.mmfd < 0  ||
	     (Ptr_seg.inf.seg = mmap(0, Ptr_seg.inf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, Ptr_seg.inf.mmfd, 0)) == MAP_FAILED)
		report($E{spd no pshm});
	fcntl(Ptr_seg.inf.mmfd, F_SETFD, 1);
	Ptr_seg.dptr = (struct pshm_hdr *) Ptr_seg.inf.seg;
	Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;
#else
	Ptr_seg.inf.base = Job_seg.dptr->js_psegid;
	if  ((Ptr_seg.inf.chan = shmget((key_t) Ptr_seg.inf.base, 0, 0)) < 0  ||
	     (Ptr_seg.inf.seg = shmat(Ptr_seg.inf.chan, (char *) 0, 0)) == (char *) -1)
		report($E{spd no pshm});
	Ptr_seg.dptr = (struct pshm_hdr *) Ptr_seg.inf.seg;
	Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;
	Ptr_seg.inf.segsize = Ptr_seg.inf.reqsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Ptr_seg.Nptrs * sizeof(Hashspptr);
#endif
	/* Set up pointers to ptr seg */

	Ptr_seg.Last_ser = 0;
	Ptr_seg.hashp_pid = (LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
	Ptr_seg.plist = (Hashspptr *) ((char *) Ptr_seg.hashp_pid + SHM_PHASHMOD * sizeof(LONG));
	if  (Ptrslot < 0  ||  Ptrslot >= Ptr_seg.dptr->ps_maxptrs)  {
		disp_str = argv[2];
		report($E{spd invalid pslot});
	}
	Pptr = &Ptr_seg.plist[Ptrslot].p;

	/* Now set device name, printer name and form type.
	   NB Device and printer names are assumed never to change. */

	devnam = Pptr->spp_dev;
	prtp = Pptr->spp_ptr;
	formp = Pptr->spp_form;

	/* Set up environment variables etc from that lot */

	strncpy(formnam, formp, MAXFORM);
	strncpy(prtnam, prtp, PTRNAMESIZE);

	/* Denote fifos from alternative directory with leading / */

	if  (Pptr->spp_netflags & SPP_LOCALNET)
		pathn = stracpy(devnam);
	else  if  (devnam[0] == '/')  {
		char	*dir = getenv(FIFO_DIR);
		if  (dir)  {
			if  ((pathn = (char *) malloc((unsigned)(strlen(devnam) + strlen(dir) + 1))) == (char *) 0)
				nomem();
			sprintf(pathn, "%s%s", dir, devnam);
		}
		else
			pathn = stracpy(devnam);
	}
	else  {
		if  ((pathn = (char *) malloc((unsigned)(strlen(devnam) + 6))) == (char *) 0)
			nomem();
		sprintf(pathn, "/dev/%s", devnam);
	}

	/* Set up env var for dev.  */

	if  ((Ev_spd = (char *) malloc((unsigned)(strlen(pathn) + 9 + 1))) == (char *) 0)
		nomem();

	sprintf(Ev_spd, "SPOOLDEV=%s", pathn);
	daeminit = envprocess(DAEMINIT);

	envinit();

	/* Grab IPM channel, set up reply buffer.  */

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)
		report($E{spd no msg q});

	My_msgid = MT_PMSG + (reply.spr_un.c.spr_pid = getpid());
	reply.spr_un.c.spr_netid = 0;
	reply.spr_un.c.spr_pslot = Ptrslot;
	reply.spr_mtype = MT_SCHED;

	/* This is where we do the actual work!  Set a longjmp for a
	   drastic file error to terminate gracefully.  Next
	   enter startup state - if ok go on to process jobs
	   finally shut down. */

	set_signal(SIGALRM, catchoff);
	set_signal(SIGHUP, catchoff);
	startup_state();
	process();

	/* And now close down tidily. */

	set_signal(SIGHUP, SIG_IGN);
	set_signal(DAEMSTOP, SIG_IGN);
	set_signal(DAEMRST, SIG_IGN);
	set_signal(SIGPIPE, SIG_IGN);
	set_pstate_notify(SPP_SHUTD);
	filtclose(FC_NOERR|FC_ABORT);
	if  (execorsend("SUFFEND", sestr, in_params.pi_sufend, in_params.pi_flags & PI_EX_SUFEND, 0)  &&
	     execorsend("HALT", haltstr, in_params.pi_halt, in_params.pi_flags & PI_EX_HALT, 0))
		pflush();	/* Don't flush if sufend or halt got interrupted */
	closedev();
	set_pstate_exit(SPP_HALT);/* Doesn't return but */
	return  0;		/* Keep C compilers quiet */
}
