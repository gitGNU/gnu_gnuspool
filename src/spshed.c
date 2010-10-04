/* spshed.c -- spool scheduling main module

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
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef	USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#include <limits.h>
#include "errnums.h"
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "files.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "xfershm.h"
#define	UCONST
#include "q_shm.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifdef	OS_FREEBSD
#define _SC_PAGE_SIZE _SC_PAGESIZE
#endif

#ifndef	USING_FLOCK
#if (defined(OS_LINUX) || defined(OS_BSDI)) && !defined(_SEM_SEMUN_UNDEFINED)
#define	my_semun	semun
#else

/* Define a union as defined in semctl(2) to pass the 4th argument of
   semctl. On most machines it is good enough to pass the value
   but on the SPARC and PYRAMID unions are passed differently
   even if all the possible values would fit in an int.  */

union	my_semun	{
	int	val;
	struct	semid_ds	*buf;
	ushort	*array;
};
#endif
#endif /* Using semaphores not FLOCK */

#define	C_MASK		0077		/*  Umask value  */
#define	IPC_MODE	0600

extern  void  lockxbuf();
extern  void  unlockxbuf();

extern  void  addoper(struct sp_omsg *);
extern  void  addptr(struct sp_xpmsg *, struct spptr *);
extern  void  chjob(struct sp_xjmsg *, struct spq *);
extern  void  chgptr(struct sp_xpmsg *, struct spptr *);
extern  void  conn_attach(struct remote *);
extern  void  deloper(struct sp_omsg *);
extern  void  delptr(struct sp_omsg *);
extern  void  doabort(struct sp_omsg *);
extern  void  enqueue(struct sp_xjmsg *, struct spq *);
extern  void  gop(struct sp_omsg *);
extern  void  halt(struct sp_omsg *);
extern  void  haltall();
extern  void  heoj(struct sp_omsg *);
extern  void  init_mwenv(int);
extern  void  interrupt(struct sp_omsg *);
extern  void  killops();
extern  void  msgptr(struct sp_omsg *);
extern  void  nfreport(const int);
extern  void  createjfile(int);
extern  void  createpfile(int);
extern  void  prdfin(struct sp_cmsg *);
extern  void  prdone(struct sp_cmsg *, const unsigned);
extern  void  prjab(struct sp_omsg *);
extern  void  proper(struct sp_cmsg *);
extern  void  restart(struct sp_omsg *);
extern  void  rewrjq();
extern  void  rewrpq();
extern  void  shut_host(const netid_t);
extern  void  tellopers();
extern  void  valspdir();
#ifdef	NETWORK_VERSION
extern  void  attach_hosts();
extern  void  clearhost(const netid_t);
extern  void  confirm_print(struct sp_omsg *);
extern  void  endsync(const netid_t);
extern  void  jpassign(struct sp_omsg *);
extern  void  locpassign(struct sp_omsg *);
extern  struct remote *alloc_roam(const netid_t, const char *);
extern  void  net_initsync();
extern  void  net_xmit(const netid_t, const int, const LONG);
extern  void  netmonitor();
extern  void  netshut();
extern  void  netsync();
extern  void  newhost();
extern  int  rattach(struct remote *);
extern  void  remchjob(struct sp_xjmsg *, struct spq *);
extern  void  remdequeue(struct sp_omsg *);
extern  void  rem_notify(struct sp_omsg *);
extern  void  rempropose(struct sp_omsg *);
extern  void  sendsync(const netid_t);
extern  void  unassign_job(struct sp_xjmsg *, struct spq *);
extern  void  unassign_remjob(struct sp_xjmsg *, struct spq *);
extern  void  unassign_ptr(struct sp_xpmsg *, struct spptr *);
#endif
extern  void  report(const int);

extern  unsigned  qpurge();
extern  unsigned  selectj();

#ifdef	NETWORK_VERSION
extern  unsigned  conn_process(const int, struct sp_nmsg *);
extern  unsigned  nettickle();

extern  struct remote	*find_connected(const netid_t);
extern  struct remote	*find_probe(const netid_t);
#endif

float	pri_decrement = 1.0;	/* Decrement in priority assignment */

/* If we are using memory-mapped files, we use the memory-map file id
   Xfermmfd as the file id to do locking if we're using file locking.
   Otherwise we use "Sem_chan" to talk to semaphores.
   If we are using shared memory we use Xfershm_lockfd to do locking
   if we are using file locking, otherwise Sem_chan for semaphores */

#ifdef	USING_MMAP
extern	int	Xfermmfd;	/* FD for mm segment and possibly locking decl in xfershm.c */
#else /* Using shared memory */
int	Xfershmchan;		/* ID of shared memory segment */
#endif
#ifdef	USING_FLOCK
extern	int	Xfershm_lockfd;	/* Declared properly in xfershm.c */
#endif /* Using flock */

#ifdef	NETWORK_VERSION
extern	PIDTYPE	Netm_pid;
PIDTYPE	Xtns_pid;
#endif

extern	struct	xfershm		*Xfer_shmp;

/* Semaphore structures.  */

int	qchanges;
time_t	suspend_sched;

extern	int	Netsync_req;

#define	INITJBUF	50

char	*daem,
	*msgdisp,
	*ptrmsg;

char	*spdir;

time_t	hadalarm;

int	Network_ok;		/* Set if skipping networking */

void  do_exit(const int n)
{
#ifndef	USING_FLOCK
	union	my_semun	uun;
#endif
#ifdef	USING_MMAP
	munmap(Ptr_seg.inf.seg, Ptr_seg.inf.segsize);
	munmap(Job_seg.dinf.seg, Job_seg.dinf.segsize);
	munmap(Job_seg.iinf.seg, Job_seg.iinf.segsize);
#else
	shmdt(Ptr_seg.inf.seg);
	shmdt(Job_seg.dinf.seg);
	shmdt(Job_seg.iinf.seg);
#endif
	msgctl(Ctrl_chan, IPC_RMID, (struct msqid_ds *) 0);
#ifndef	USING_FLOCK
	uun.val = 0;
	semctl(Sem_chan, 0, IPC_RMID, uun);
#endif
#ifdef	USING_MMAP
	close(Job_seg.iinf.mmfd);
	close(Job_seg.dinf.mmfd);
	close(Ptr_seg.inf.mmfd);
	close(Xfermmfd);
	unlink(JIMMAP_FILE);
	unlink(JDMMAP_FILE);
	unlink(PMMAP_FILE);
	unlink(XFMMAP_FILE);
#else
	shmctl(Job_seg.iinf.chan, IPC_RMID, (struct shmid_ds *) 0);
	shmctl(Job_seg.dinf.chan, IPC_RMID, (struct shmid_ds *) 0);
	shmctl(Ptr_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
	shmctl(Xfershmchan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef	USING_FLOCK
	close(Job_seg.iinf.lockfd);
	close(Ptr_seg.inf.lockfd);
	close(Xfershm_lockfd);
	unlink(JLOCK_FILE);
	unlink(PLOCK_FILE);
	unlink(XLOCK_FILE);
#endif /* USING_FLOCK  */
#endif /* Shm -v- MMAP */
#ifdef	NETWORK_VERSION
	if  (Netm_pid > 0)
		kill(Netm_pid, SIGKILL);
#endif
	exit(n);
}

void  nomem()
{
	report($E{NO MEMORY});
	do_exit(E_NOMEM);
}

/* Synchronise files.  */

void  syncfls()
{
	if  (Ptr_seg.Last_ser)  {
		rewrpq();
		Ptr_seg.Last_ser = 0;
	}
	if  (Job_seg.Last_ser)  {
		rewrjq();
		Job_seg.Last_ser = 0;
	}
}

/* Try to exit gracefully and quickly....  */

RETSIGTYPE  niceend(int signum)
{
	if  (signum < NSIG)  {
#ifdef	UNSAFE_SIGNALS
		signal(signum, SIG_IGN);
#endif
		signum = signum == SIGTERM? $E{Sched killed}: $E{Program fault halt};
	}

	nfreport(signum);
	haltall();
#ifdef	NETWORK_VERSION
	netshut();
	if  (Xtns_pid)
		kill(-Xtns_pid, SIGTERM);
#endif
	syncfls();
	killops();
	do_exit(0);
}

/* Open IPC channel.  Done whilst we are still super-user as we use it
   as a lock file and we want to attach hosts etc as super-user.  */

void  openrfile()
{
	char	*xfers;
#ifndef	USING_FLOCK
	int	i;
	union	my_semun	uun;
	ushort	array[SEMNUMS];
	struct	semid_ds  sb;
#endif
	struct	msqid_ds  mb;
#ifdef	USING_MMAP
	LONG	pagesize = sysconf(_SC_PAGE_SIZE);
	ULONG	rqsize = (sizeof(struct xfershm) + pagesize - 1) & ~(pagesize-1);
	char	byte = 0;
#endif

	if  ((Ctrl_chan = msgget(MSGID, IPC_MODE|IPC_CREAT|IPC_EXCL)) < 0)  {
		if  (errno == EEXIST)
			exit(0);
		report($E{Cannot create message queue});
	}

	if  (Daemuid)  {
		if  (msgctl(Ctrl_chan, IPC_STAT, &mb) < 0)
			report($E{Cannot reset message queue});

		mb.msg_perm.uid = Daemuid;
		mb.msg_perm.gid = getgid();
		if  (msgctl(Ctrl_chan, IPC_SET, &mb) < 0)
			report($E{Cannot reset message queue});
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE|IPC_CREAT)) < 0)
		report($E{Semaphore create error});

	if  (Daemuid)  {
		uun.buf = &sb;
		if  (semctl(Sem_chan, 0, IPC_STAT, uun) < 0)
			report($E{Semaphore reset error});

		sb.sem_perm.uid = Daemuid;
		sb.sem_perm.gid = getgid();

		if  (semctl(Sem_chan, 0, IPC_SET, uun) < 0)
			report($E{Semaphore reset error});
	}

	for  (i = 0;  i < SEMNUMS;  i++)
		array[i] = 0;
	array[XT_LOCK] = 1;

	uun.array = array;

	if  (semctl(Sem_chan, 0, SETALL, uun) < 0)
		report($E{Semaphore reset error});
#endif

#ifdef	USING_MMAP
	if  ((Xfermmfd = open(XFMMAP_FILE, O_RDWR|O_TRUNC|O_CREAT, IPC_MODE)) < 0)
		report($E{Cannot create xfer shm});
#ifdef	HAVE_FCHOWN
	if  (Daemuid)
		Ignored_error = fchown(Xfermmfd, Daemuid, getgid());
#else
	if  (Daemuid)
		Ignored_error = chown(XFMMAP_FILE, Daemuid, getgid());
#endif
	fcntl(Xfermmfd, F_SETFD, 1);
	lseek(Xfermmfd, (long) (rqsize - 1), 0);
	if  (write(Xfermmfd, &byte, 1) != 1)
		report($E{Cannot create xfer shm});
	if  ((xfers = mmap(0, rqsize, PROT_READ|PROT_WRITE, MAP_SHARED, Xfermmfd, 0)) == MAP_FAILED)
		report($E{Cannot attach xfer shm});
#else  /* Using shared memory - but we might still be doing file locking */
	if  ((Xfershmchan = shmget(XSHMID, sizeof(struct xfershm), IPC_MODE|IPC_CREAT|IPC_EXCL)) < 0)
		report($E{Cannot create xfer shm});
	if  (Daemuid)  {
		struct	shmid_ds	shb;
		if  (shmctl(Xfershmchan, IPC_STAT, &shb) < 0)
			report($E{Shm reset error});
		shb.shm_perm.uid = Daemuid;
		shb.shm_perm.gid = getgid();
		if  (shmctl(Xfershmchan, IPC_SET, &shb) < 0)
			report($E{Shm reset error});
	}
	if  ((xfers = shmat(Xfershmchan, (char *) 0, 0)) == (char *) -1)
		report($E{Cannot attach xfer shm});
#ifdef	USING_FLOCK
	/* Case where we're using file locking for locking but
	   using shared memory - Create lock file */
	if  ((Xfershm_lockfd = open(XLOCK_FILE, O_RDWR|O_CREAT|O_TRUNC, IPC_MODE)) < 0)
		report($E{Semaphore create error});
#ifdef	HAVE_FCHOWN
	if  (Daemuid)
		fchown(Xfershm_lockfd, Daemuid, getgid());
#else
	if  (Daemuid)
		chown(XLOCK_FILE, Daemuid, getgid());
#endif /* HAVE_CHOWN */
	fcntl(Xfershm_lockfd, F_SETFD, 1);
#endif /* USING_FLOCK */
#endif /* End of worrying about shared memory -v- memory mapping */
	Xfer_shmp = (struct xfershm *) xfers;
	Xfer_shmp->xf_nonq = Xfer_shmp->xf_head = Xfer_shmp->xf_tail = 0;
#ifndef	USING_FLOCK
	set_xfer_server();
#endif
#ifdef	USING_MMAP
	msync(xfers, rqsize, MS_ASYNC|MS_INVALIDATE);
#endif
}

/* Get a shared memory id, and increment key.  */

int  gshmchan(struct spshm_info *infp, const int off)
{
#ifdef	USING_MMAP
	LONG	pagesize = sysconf(_SC_PAGE_SIZE);
	ULONG	rqsize = (infp->reqsize + pagesize - 1) & ~(pagesize-1);
	char	byte = 0;

	if  (infp->segsize > 0) 	/* Existing segment we are growing */
		munmap(infp->seg, infp->segsize);
	else  {
		char	*fname = off == JSHMOFF? JDMMAP_FILE: PMMAP_FILE;
		if  ((infp->mmfd = open(fname, O_CREAT|O_RDWR|O_EXCL, IPC_MODE)) < 0)
			goto  fail;
#ifdef	RUN_AS_ROOT
#ifdef	HAVE_FCHOWN
		if  (Daemuid)
			fchown(infp->mmfd, Daemuid, getgid());
#else
		if  (Daemuid)
			chown(fname, Daemuid, getgid());
#endif
#endif
		fcntl(infp->mmfd, F_SETFD, 1);
	}

	/* Write a byte to the last byte of the file */

	lseek(infp->mmfd, (long) (rqsize - 1), 0);
	if  (write(infp->mmfd, &byte, 1) == 1)  {

		/* Map what we can give up if we are stuck */

		for  (;  rqsize > infp->segsize;  rqsize >>= 1)  {
			void  *segp = mmap(0, rqsize, PROT_READ|PROT_WRITE, MAP_SHARED, infp->mmfd, 0);
			if  (segp != MAP_FAILED)  {
				infp->seg = segp;
				infp->segsize = rqsize;
				return  1;
			}
		}
	}

 fail:
	disp_arg[9] = off;
	report($E{Shm create error});
	return  0;		/* To keep C compilers happy */

#else  /* SHM case */
#ifndef	NO_ROUND_PAGES
	LONG	pagesize = sysconf(_SC_PAGE_SIZE);
#endif
	int	result, bomb = MAXSHMS / SHMINC;
	void	*segp;
	struct	shmid_ds	shb;

#ifdef	NO_ROUND_PAGES
	infp->segsize = infp->reqsize;
#else
	infp->segsize = (infp->reqsize + pagesize - 1) & ~(pagesize - 1);
#endif

	for  (;;)  {

		if  (infp->base >= SHMID + MAXSHMS)
			infp->base = SHMID + off;

		if  ((result = shmget(infp->base, infp->segsize, IPC_MODE|IPC_CREAT|IPC_EXCL)) >= 0)
			break;

		if  (errno == EINVAL)  { /* Halve until it fits */
#ifdef	NO_ROUND_PAGES
			infp->segsize >>= 1;
#else
			infp->segsize = ((infp->segsize >> 1) + pagesize - 1) & ~(pagesize - 1);
#endif
			if  (infp->segsize > 0)
				continue;
		}

		if  (errno != EEXIST  ||  --bomb <= 0)  {
			disp_arg[9] = infp->base;
			report($E{Shm create error});
		}

		infp->base += SHMINC;
	}
	infp->chan = result;

	/* Find out what size we really got....  */

	if  (shmctl(result, IPC_STAT, &shb) < 0)
		report($E{Shm reset error});
	infp->segsize = shb.shm_segsz;

#ifdef	RUN_AS_ROOT
	if  (Daemuid)  {
		struct	shmid_ds	shb;
		shb.shm_perm.uid = Daemuid;
		shb.shm_perm.gid = getgid();

		if  (shmctl(result, IPC_SET, &shb) < 0)
			report($E{Shm reset error});
	}
#endif

	/* Now attach segment */

	if  ((segp = shmat(result, 0, 0)) == (void *) -1)
		return  0;

	infp->seg = segp;
	return  1;
#endif
}

void  op_process(const int bytes, struct sp_omsg *req)
{
	if  (bytes != sizeof(struct sp_omsg))  {
		disp_arg[9] = bytes;
		report($E{Sched op message length error});
	}

	switch	(req->spr_act)  {
	default:
		disp_arg[9] = req->spr_act;
		report($E{Sched op message error});

	case  SO_AB:
	case  SO_ABNN:
		doabort(req);
		break;

#ifdef	NETWORK_VERSION
	case  SO_DEQUEUED:
		remdequeue(req);
		break;
#endif

	case  SO_MON:
		addoper(req);
		break;

	case  SO_DMON:
		deloper(req);
		break;

	case  SO_RSP:
		restart(req);
		break;

	case  SO_PHLT:
		heoj(req);
		break;

	case  SO_PSTP:
		halt(req);
		break;

	case  SO_PGO:
		gop(req);
		break;

	case  SO_DELP:
		delptr(req);
		break;

	case  SO_SSTOP:
		niceend((int) req->spr_arg1);
		break;

	case  SO_SUSPSCHED:
		suspend_sched = time((time_t *) 0) + req->spr_arg1;
		break;

	case  SO_UNSUSPEND:
		suspend_sched = 0;
		break;

	case  SO_OYES:
	case  SO_ONO:
		msgptr(req);
		break;

	case  SO_INTER:
		interrupt(req);
		break;

	case  SO_PJAB:
		prjab(req);
		break;

#ifdef	NETWORK_VERSION

	case  SN_NEWHOST:
		newhost();
		netmonitor();
		break;

	case  SN_SHUTHOST:
		clearhost(req->spr_netid);

	case  SN_ABORTHOST:
		netmonitor();
		break;

	case  SN_REQSYNC:
		sendsync(req->spr_netid);
		net_xmit(req->spr_netid, SN_ENDSYNC, 0L);
		break;

	case  SN_ENDSYNC:
		endsync(req->spr_netid);
		break;

	case  SO_NOTIFY:
	case  SO_PNOTIFY:
		rem_notify(req);
		break;

	case  SO_PROPOSE:
		rempropose(req);
		break;

	case  SO_PROP_OK:
	case  SO_PROP_NOK:
	case  SO_PROP_DEL:
		confirm_print(req);
		break;

	case  SO_ASSIGN:
		jpassign(req);
		break;

	case  SO_LOCASSIGN:
		locpassign(req);
		break;
#endif
	}
}

static struct joborptr *fetch_ptr()
{
	struct	joborptr  *result;
	lockxbuf();
	if  (Xfer_shmp->xf_nonq == 0)
		report($E{No messages on queue});
	result = &Xfer_shmp->xf_queue[Xfer_shmp->xf_head];
	Xfer_shmp->xf_head = (Xfer_shmp->xf_head + 1) % (TRANSBUF_NUM + 1);
	Xfer_shmp->xf_nonq--;
#ifdef	USING_MMAP
	msync((char *) Xfer_shmp, sizeof(struct xfershm), MS_ASYNC|MS_INVALIDATE);
#endif
	return  result;
}

void  pr_process(const int bytes, struct sp_xpmsg *req)
{
	struct	joborptr  *pp;
	struct	spptr	inptr;

	if  (bytes != sizeof(struct sp_xpmsg))  {
		disp_arg[9] = bytes;
		report($E{Sched ptr message length error});
	}

	pp = fetch_ptr();
	if  (pp->jorp_sender != req->spr_pid)  {
		nfreport($E{Buffer sequence error printer});
		unlockxbuf();
		return;
	}
	BLOCK_COPY(&inptr, &pp->jorp_un.p, sizeof(struct spptr));
	unlockxbuf();

	switch	(req->spr_act)  {
	default:
		disp_arg[9] = req->spr_act;
		report($E{Sched ptr message error});

	case  SP_ADDP:
		addptr(req, &inptr);
		break;

	case  SP_CHGP:
	case  SP_CHANGEDPTR:
		chgptr(req, &inptr);
		break;

#ifdef	NETWORK_VERSION
	case  SP_PUNASSIGNED:
		unassign_ptr(req, &inptr);
		break;
#endif
	}
}

void  ch_process(const int bytes, struct sp_cmsg *req)
{
	if  (bytes != sizeof(struct sp_cmsg))  {
		disp_arg[9] = bytes;
		report($E{Sched charge message length error});
	}

	switch	(req->spr_act)  {
	default:
		disp_arg[9] = req->spr_act;
		report($E{Sched charge message error});

	case  SPD_DONE:
	case  SPD_DAB:
	case  SPD_DERR:
		prdone(req, req->spr_act);
		break;

	case  SPD_DFIN:
		prdfin(req);
		break;

	case  SPD_SCH:
		proper(req);
		break;

	case  SPD_CHARGE:
		break;
	}
}

void  jb_process(const int bytes, struct sp_xjmsg *req)
{
	struct	joborptr  *pp;
	struct	spq	injob;

	if  (bytes < sizeof(struct sp_xjmsg))  {
		disp_arg[9] = bytes;
		report($E{Sched job message length error});
	}

	pp = fetch_ptr();
	BLOCK_COPY(&injob, &pp->jorp_un.q, sizeof(struct spq));
	if  (pp->jorp_sender != req->spr_pid)  {
		nfreport($E{Buffer sequence error job});
		unlockxbuf();
		return;
	}
	unlockxbuf();

	switch	(req->spr_act)  {
	default:
		disp_arg[9] = req->spr_act;
		report($E{Sched job message error});

	case  SJ_ENQ:
		enqueue(req, &injob);
		break;

	case  SJ_CHNG:
		chjob(req, &injob);
		break;

#ifdef	NETWORK_VERSION
	case  SJ_CHANGEDJOB:
		remchjob(req, &injob);
		break;

	case  SJ_JUNASSIGN:
		unassign_job(req, &injob);
		break;

	case  SJ_JUNASSIGNED:
		unassign_remjob(req, &injob);
		break;
#endif
	}
}

#ifdef	NETWORK_VERSION
unsigned  conn_process(const int bytes, struct sp_nmsg *req)
{
	struct	remote	*rp;

	if  (bytes < sizeof(struct sp_nmsg))  {
		disp_arg[9] = bytes;
		report($E{Sched network message length error});
	}
	switch	(req->spr_act)  {
	default:
		disp_arg[9] = req->spr_act;
		report($E{Sched network message error});

	case  SON_CONNECT:
		if  (find_connected(req->spr_n.hostid))		/* Already speaking */
			break;
		if  (Netm_pid)
			kill(Netm_pid, NETSHUTSIG);
		if  (rattach(&req->spr_n))	/* Immediate connect - now does any needed free_probe */
			sendsync(req->spr_n.hostid);
		break;

	case  SON_DISCONNECT:
		shut_host(req->spr_n.hostid);	/* Send shutdown msg if connected */
		clearhost(req->spr_n.hostid);	/* Clearhost now includes free_probe */
		if  (Netm_pid)
			kill(Netm_pid, NETSHUTSIG);
		/* Signal handler runs aborthost which re-runs netmonitor */
		break;

	case  SON_CONNOK:	/* Sent by net monitor which does its own exit */
		if  ((rp = find_probe(req->spr_n.hostid)))  {
			conn_attach(rp);		/* Now includes free_probe */
			sendsync(req->spr_n.hostid);
		}
		netmonitor();
		break;

	case  SON_XTNATT:

		/* Doesn't really need to be this big but "future
		   extensions".  We kill off the process if we think a previous
		   one is running */

		if  (Xtns_pid  &&  kill(Xtns_pid, 0) >= 0)
			kill(-(PIDTYPE) req->spr_pid, SIGKILL);
		else
			Xtns_pid = req->spr_pid;
		break;

	case  SON_ROAMUSER:
		alloc_roam(req->spr_n.hostid, req->spr_n.hostname);
		break;
	}

	return  nettickle();
}
#endif

/* Catch alarm signals.  */

RETSIGTYPE  acatch(int n)
{
#ifdef	UNSAFE_SIGNALS
	signal(n, acatch);
#endif
	time(&hadalarm);
}

/* This routine is the main process.  */

void  process()
{
	struct	spr_req	inreq;
	int	bytes;
	unsigned	selt, put, reft = 0, alt;
#ifdef	NETWORK_VERSION
	unsigned	nett = 0;

	if  (Netsync_req > 0)
		netsync();
#endif

	selt = selectj();

	for  (;;)  {
		put = 0;
		if  (Job_seg.dptr->js_njobs != 0)  {
			put = qpurge();
			if  (qchanges)
				goto  opend;
		}

		/* Set alarm time 'alt' for smallest of:
		   Time to select held jobs (selt)
		   Time to purge old jobs (put)
		   Time to write out job/printer files (reft) */

#ifdef	NETWORK_VERSION
		if  (selt + put + reft + nett == 0)
			alt = 0;
		else  {
			alt = 0x7fff;
			if  (selt)
				alt = selt;
			if  (put  &&  put < alt)
				alt = put;
			if  (reft  &&  reft < alt)
				alt = reft;
			if  (nett  &&  nett < alt)
				alt = nett;
		}
#else
		if  (selt + put + reft == 0)
			alt = 0;
		else  {
			alt = 0x7fff;
			if  (selt)
				alt = selt;
			if  (put  &&  put < alt)
				alt = put;
			if  (reft  &&  reft < alt)
				alt = reft;
		}
#endif
		alarm(alt);

		if  ((bytes = msgrcv(Ctrl_chan,
				     (struct msgbuf *) &inreq,
				     sizeof(inreq) - sizeof(long), /* I do mean lower-case "long" */
				     MT_SCHED,
				     0)) < 0)  {

			/* If we get an EINTR, assume that it was an
			   alarm call.  */

			if  (errno != EINTR)
				report($E{Network IPC send fail});
			selt = selectj();
			put = qpurge();
#ifdef	NETWORK_VERSION
			nett = nettickle();
#endif
			goto  pend;
		}

		switch  (inreq.spr_un.o.spr_act)  {
		case  SO_AB:
		case  SO_ABNN:
		case  SO_DEQUEUED:
		case  SO_MON:
		case  SO_DMON:
		case  SO_RSP:
		case  SO_PHLT:
		case  SO_PSTP:
		case  SO_PGO:
		case  SO_DELP:
		case  SO_SSTOP:
		case  SO_OYES:
		case  SO_ONO:
		case  SO_INTER:
		case  SO_PJAB:
		case  SN_NEWHOST:
		case  SN_SHUTHOST:
		case  SN_ABORTHOST:
		case  SN_REQSYNC:
		case  SN_ENDSYNC:
		case  SO_NOTIFY:
		case  SO_PNOTIFY:
		case  SO_PROPOSE:
		case  SO_PROP_OK:
		case  SO_PROP_NOK:
		case  SO_PROP_DEL:
		case  SO_ASSIGN:
		case  SO_LOCASSIGN:
		case  SO_SUSPSCHED:
		case  SO_UNSUSPEND:
			op_process(bytes, &inreq.spr_un.o);
			selt = selectj();
			break;
		case  SP_ADDP:
		case  SP_CHGP:
		case  SP_CHANGEDPTR:
		case  SP_PUNASSIGNED:
			pr_process(bytes, &inreq.spr_un.p);
			selt = 0;
			break;
		case  SJ_ENQ:
		case  SJ_CHNG:
		case  SJ_CHANGEDJOB:
		case  SJ_JUNASSIGN:
		case  SJ_JUNASSIGNED:
			jb_process(bytes, &inreq.spr_un.j);
			selt = selectj();
			break;
		case  SPD_DONE:
		case  SPD_DAB:
		case  SPD_DERR:
		case  SPD_DFIN:
		case  SPD_SCH:
		case  SPD_CHARGE:
			ch_process(bytes, &inreq.spr_un.c);
			selt = selectj();
			break;
#ifdef	NETWORK_VERSION
		case  SON_CONNECT:
		case  SON_DISCONNECT:
		case  SON_CONNOK:
		case  SON_XTNATT:
		case  SON_ROAMUSER:
			nett = conn_process(bytes, &inreq.spr_un.n);
			selt = selectj();
			break;
#endif
		case  SOU_PWCHANGED:
#ifdef	NETWORK_VERSION
			if  (Netm_pid)
				kill(Netm_pid, NETSHUTSIG);
			if  (Xtns_pid)
				kill(-Xtns_pid, SIGHUP);
#endif
			break;
		default:
			disp_arg[9] = inreq.spr_un.o.spr_act;
			report($E{Sched invalid message});
		}

		/* Advise operators.  */

	opend:
		if  (qchanges)
			tellopers();

		/* Update files now or when due.  */
	pend:
		if  (Job_seg.Last_ser || Ptr_seg.Last_ser)  {
			time_t	now = time((time_t *) 0);
			time_t	jtim = now - Job_seg.dptr->js_lastwrite;
			time_t	ptim = now - Ptr_seg.dptr->ps_lastwrite;

			reft = QREWRITE;
			if  (jtim >= QREWRITE  || ptim >= QREWRITE)  {
				syncfls();
				reft = 0;
			}
			else  {
				if  (Job_seg.Last_ser)
					reft -= jtim;
				if  (Ptr_seg.Last_ser  &&  QREWRITE - ptim < reft)
					reft = QREWRITE - ptim;
			}
		}
#ifdef	NETWORK_VERSION
		if  (Netsync_req > 0)
			netsync();
#endif
	}
}

/* Ye olde main routine.
   Take initial job and printer allocations as parameters */

MAINFN_TYPE  main(int argc, char **argv)
{
#ifndef	DEBUG
	PIDTYPE	pid;
#endif
	int	jsize = 0, psize = 0;
	int_ugid_t	chku;
	char		*sn;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	zign;
#endif

	versionprint(argv, "$Revision: 1.2 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	/* New feature - initialise size of job/printer lists from
	   arguments for benefit of machines which choke over allocating
	   additional shared memory.  */

	if  (argc > 1)  {
		jsize = atoi(argv[1]);
		if  (argc > 2)  {
			psize = atoi(argv[2]);
			if  (argc > 3)
				pri_decrement = (float) atof(argv[3]);
		}
	}

	init_mcfile();

	/* Turn off networking if SKIP_NETWORK set */

	Network_ok = 1;
	if  ((sn = getenv("SKIP_NETWORK"))  &&  (sn[0] == '1' || sn[0] == 'y' || sn[0] == 'Y'))
		Network_ok = 0;

	/* As parent process, close files etc and generate IPC message
	   This is because if the scheduler is invoked by 'spr', the
	   latter does not want to restart until the IPC has been
	   created.  */

	if  ((Cfile = open_icfile()) == (FILE *) 0)
		exit(E_NOCONFIG);

	rpwfile();		/* Insist on this in any case */

	Realuid = getuid();
	Effuid = geteuid();
	if  ((chku = lookup_uname(SPUNAME)) == UNKNOWN_UID)
		Daemuid = ROOTID;
	else
		Daemuid = chku;

#ifdef	STRUCT_SIG
	zign.sighandler_el = SIG_IGN;
	sigmask_clear(zign);
	zign.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGHUP, &zign, (struct sigstruct_name *) 0);
	sigact_routine(SIGINT, &zign, (struct sigstruct_name *) 0);
	sigact_routine(SIGQUIT, &zign, (struct sigstruct_name *) 0);
	sigact_routine(SIGTERM, &zign, (struct sigstruct_name *) 0);
#else
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
#endif

#ifdef	SETPGRP_VOID
#ifndef	M88000
	/* On the M88000 this causes problems when spd calls setpgrp too.  */
	setpgrp();
#endif	/* !M88000 */
#else
	setpgrp(0, getpid());
#endif

	spdir = envprocess(SPDIR);
	daem = envprocess(DAEM);
	msgdisp = envprocess(MSGDISPATCH);
	ptrmsg = getenv(PTRMSGS);
	init_mwenv(umask(C_MASK));

	if  (chdir(spdir) < 0)
		report($E{Cannot chdir});

	valspdir();

	openrfile();

	/* Revert to spooler user */

#ifdef	SCO_SECURITY
#ifdef	RUN_AS_ROOT
	setluid(ROOTID);
	setuid(ROOTID);
#else
	setluid(Daemuid);
	setuid(Daemuid);
#endif
#else
#ifdef	RUN_AS_ROOT
	setuid(ROOTID);
#else
	setuid(Daemuid);
#endif
#endif

#ifndef	BUGGY_SIGCLD
#ifdef	STRUCT_SIG
	zign.sighandler_el = SIG_IGN;
#ifdef	SA_NOCLDWAIT
	zign.sigflags_el |= SA_NOCLDWAIT;
#endif
	sigact_routine(SIGCLD, &zign, (struct sigstruct_name *) 0);
#else
	signal(SIGCLD, SIG_IGN);
#endif
#endif

	/* Ignore PIPE errors in case daemon processes quit.
	   (Signal is #defined as sigset on suitable systems).  */

#ifdef	STRUCT_SIG
	zign.sighandler_el = SIG_IGN;
	sigact_routine(SIGPIPE, &zign, (struct sigstruct_name *) 0);
	zign.sighandler_el = acatch;
	sigact_routine(SIGALRM, &zign, (struct sigstruct_name *) 0);
#else
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, acatch);
#endif

	/* Open job and printer files.  */

	createjfile(jsize);
	createpfile(psize);

	/* Fork off to leave an orphaned child process.
	   Return E_FALSE if non-networked so spstart won't try
	   to start xtnetserv */

#ifndef	DEBUG
	pid = fork();
	if  (pid > 0)		/*  Main path  */
		exit(Network_ok? E_TRUE: E_FALSE);

	if  (pid < 0)
		report($E{Internal cannot fork});
#endif

#ifdef	NETWORK_VERSION

	/* Attach to other people (Note order change from 18.1 as spq
	crashes if attach_hosts takes a long time and the msg queue is
	set up but not the shm segments).  Send details of my jobs to
	other people Start up net monitor process */

	if  (Network_ok)  {
		attach_hosts();
		net_initsync();
		netmonitor();
	}
#endif
	/* Provide for SIGTERM terminating nicely Also provide for core dumps etc.  */

#ifdef	STRUCT_SIG
	zign.sighandler_el = niceend;
	sigact_routine(SIGTERM, &zign, (struct sigstruct_name *) 0);
#ifndef	DEBUG
	sigact_routine(SIGBUS, &zign, (struct sigstruct_name *) 0);
	sigact_routine(SIGSEGV, &zign, (struct sigstruct_name *) 0);
	sigact_routine(SIGILL, &zign, (struct sigstruct_name *) 0);
#ifdef	SIGSYS
	sigact_routine(SIGSYS, &zign, (struct sigstruct_name *) 0);
#endif /* SIGSYS */
#endif /* !DEBUG */
#else  /* !STRUCT_SIG */
	signal(SIGTERM, niceend);
#ifndef	DEBUG
	signal(SIGBUS, niceend);
	signal(SIGSEGV, niceend);
	signal(SIGILL, niceend);
#ifdef	SIGSYS
	signal(SIGSYS, niceend);
#endif
#endif
#endif

#ifdef	DONT_DEFINE_THIS
	$E{Sched normal stop};
#endif
	nfreport($E{Sched started});
	process();		/* Never returns  */
	return  0;		/* But compiler doesn't know that */
}
