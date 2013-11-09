/* sh_plist.c -- spool scheduling mostly printer handling

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef  USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#include <limits.h>
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "files.h"
#include "ipcstuff.h"
#define UCONST
#include "q_shm.h"
#include "incl_unix.h"
#include "notify.h"
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

#define INITNPTRS       INITPALLOC
#define INCNPTRS        (INITPALLOC/2)
#define CMODE           0666
#define IPC_MODE        0600

extern  void  delete_job(Hashspq *);
extern  void  dequeue(const int);
extern  void  nfreport(const int);
extern  void  notify(struct spq *, struct spptr *, const int, const jobno_t, const int);
extern  void  job_broadcast(Hashspq *, const int);
extern  void  job_message(const netid_t, struct spq *, const int, const ULONG, const ULONG);
extern  void  ptr_broadcast(Hashspptr *, const int);
extern  void  ptr_sendupdate(struct spptr *, struct spptr *, const int);
extern  void  ptr_message(struct spptr *, const int);
extern  void  ptr_assxmit(Hashspq *, Hashspptr *);
extern  void  report(const int);
extern  void  unassign(Hashspq *, Hashspptr *);

extern  int  gshmchan(struct spshm_info *, const int);
extern  slotno_t  find_rslot(const netid_t, const slotno_t);

extern  struct spptr    *printing(struct spq *);
void  ptrnotify(struct spptr *);

extern  uid_t   Daemuid;
extern  int     Ctrl_chan;
#ifndef USING_FLOCK
extern  int     Sem_chan;
#endif

extern  int     Network_ok;
extern  PIDTYPE Netm_pid;

#ifndef USING_FLOCK
static  struct  sembuf  pw[2] = {{      PQ_READING,     0,      0 },
                                {       PQ_FIDDLE,      1,      0 }},
                        pu[1] = {{      PQ_FIDDLE,      -1,     0 }};
#endif

char    pfile[] = PFILE;
int     pfilefd;

extern  int     qchanges;
extern  time_t  suspend_sched;

#ifdef  BUGGY_SIGCLD
int     nchild;
#endif

extern  char    *daem;

static  char    Sufchars[] = DEF_SUFCHARS;

#ifdef  USING_FLOCK

void  setphold(const int typ)
{
        struct  flock   lck;
        lck.l_type = typ;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Ptr_seg.inf.mmfd, F_SETLKW, &lck) >= 0)
                        return;
#else
                if  (fcntl(Ptr_seg.inf.lockfd, F_SETLKW, &lck) >= 0)
                        return;
#endif
                if  (errno != EINTR)
                        report($E{Lock error});
        }
}

static void  ptrs_lock()
{
        setphold(F_WRLCK);
}

static void  ptrs_unlock()
{
#ifdef  USING_MMAP
        msync(Ptr_seg.inf.seg, Ptr_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
        setphold(F_UNLCK);
}

#else

static void  ptrs_lock()
{
        for  (;;)  {
                if  (semop(Sem_chan, pw, 2) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                report($E{Semaphore error probably undo});
        }
}

static void  ptrs_unlock()
{
#ifdef  USING_MMAP
        msync(Ptr_seg.inf.seg, Ptr_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
        for  (;;)  {
                if  (semop(Sem_chan, pu, 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                report($E{Semaphore error probably undo});
        }
}
#endif

static void  free_rest(const LONG pind, const LONG nptrs)
{
        Hashspptr       *pp;
        LONG            cnt;

        BLOCK_ZERO(&Ptr_seg.plist[pind], (nptrs - pind) * sizeof(Hashspptr));

        /* We do this loop backwards to that the new ones go on the end of the free chain. */

        pp = &Ptr_seg.plist[nptrs];
        cnt = nptrs;

        while  (cnt > pind)  {
                pp--;
                pp->p.spp_rjslot = pp->p.spp_jslot = -1;
                pp->l_prv = pp->nxt_pid_hash = pp->prv_pid_hash = SHM_HASHEND;
                pp->l_nxt = Ptr_seg.dptr->ps_freech;
                Ptr_seg.dptr->ps_freech = --cnt;
        }
}

/* Create/open print file. Read it into memory if it exists.  */

void  createpfile(int psize)
{
        Hashspptr       *hpp;
        struct  spptr   *pp;
        int             Maxptrs;
        unsigned        hashval;
        LONG            nxtp, prevind;
        struct  stat    sbuf;

        if  ((pfilefd = open(pfile, O_RDWR|O_CREAT, CMODE)) < 0)
                report($E{Cannot create pfile});

#ifdef  RUN_AS_ROOT
        if  (Daemuid)
#if     defined(HAVE_FCHOWN) && !defined(M88000)
                fchown(pfilefd, Daemuid, getegid());
#else
                chown(pfile, Daemuid, getegid());
#endif
#endif

        fcntl(pfilefd, F_SETFD, 1);

        if  (psize <= 0)
                psize = INITNPTRS;

        /* Initialise shared memory segment to size of file, rounding up. */

#ifndef USING_FLOCK
        /* If we're using semaphores lock it now as semaphore created.
           If we're using file locking we'll have to wait until the file descr is open.
           I don't think the race condition matters as no one will try to lock it
           before they've tried and to access the shared memory segment or mmap file */
        ptrs_lock();
#endif
        fstat(pfilefd, &sbuf);
        Maxptrs = sbuf.st_size / sizeof(struct spptr);
        if  (Maxptrs < psize)
                Maxptrs = psize;

        Ptr_seg.inf.reqsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Maxptrs * sizeof(Hashspptr);

#ifndef USING_MMAP
#ifdef  USING_FLOCK
        if  ((Ptr_seg.inf.lockfd = open(PLOCK_FILE, O_CREAT|O_RDWR|O_TRUNC, IPC_MODE)) < 0)
                report($E{Create pshm error});
#ifdef  HAVE_FCHOWN
        if  (Daemuid)
                Ignored_error = fchown(Ptr_seg.inf.lockfd, Daemuid, getgid());
#else
        if  (Daemuid)
                Ignored_error = chown(PLOCK_FILE, Daemuid, getgid());
#endif
        fcntl(Ptr_seg.inf.lockfd, F_SETFD, 1);
        ptrs_lock();
#endif /* USING_FLOCK */
        Ptr_seg.inf.base = SHMID + PSHMOFF;
#endif /* !USING_MMAP */

        if  (!gshmchan(&Ptr_seg.inf, PSHMOFF))
                report($E{Create pshm error});

#ifdef  USING_MMAP
        ptrs_lock();
        Job_seg.dptr->js_psegid = Ptr_seg.inf.segsize;
#else
        Job_seg.dptr->js_psegid = Ptr_seg.inf.base;
#endif
        /* If kernel gave us more than we asked for, adjust Maxptrs accordingly.  */

        Maxptrs = (Ptr_seg.inf.segsize - sizeof(struct pshm_hdr) - SHM_PHASHMOD * sizeof(LONG)) / sizeof(Hashspptr);

        Ptr_seg.dptr = (struct pshm_hdr *) Ptr_seg.inf.seg;
        Ptr_seg.hashp_pid = (LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
        Ptr_seg.plist = (Hashspptr *) ((char *) Ptr_seg.hashp_pid + SHM_PHASHMOD * sizeof(LONG));

        /* Initialise hash table and pointers to head and tail of list.  */

        for  (nxtp = 0;  nxtp < SHM_PHASHMOD;  nxtp++)
                Ptr_seg.hashp_pid[nxtp] = SHM_HASHEND;

        Ptr_seg.dptr->ps_l_head = Ptr_seg.dptr->ps_l_tail = Ptr_seg.dptr->ps_freech = SHM_HASHEND;

        /* Read printers into list.  */

        Ptr_seg.Nptrs = 0;
        hpp = &Ptr_seg.plist[0];
        pp = &hpp->p;

        while  (read(pfilefd, (char *) pp, sizeof(struct spptr)) == sizeof(struct spptr))  {
                if  (pp->spp_state == SPP_NULL)
                        continue;
                pp->spp_job = 0;
                pp->spp_rjhostid = 0;
                pp->spp_rjslot = pp->spp_jslot = -1;
                pp->spp_state = SPP_HALT;
                pp->spp_sflags = 0;
                pp->spp_dflags = 0;
                pp->spp_pid = 0;
                pp->spp_resvd = 0;

                /* Put on hash chain and list */

                pp->spp_rslot = Ptr_seg.Nptrs;
                hpp->prv_pid_hash = hpp->l_nxt = SHM_HASHEND;
                hashval = pid_hash(0, Ptr_seg.Nptrs);
                prevind = hpp->nxt_pid_hash = Ptr_seg.hashp_pid[hashval];
                Ptr_seg.hashp_pid[hashval] = Ptr_seg.Nptrs;
                if  (prevind >= 0)
                        Ptr_seg.plist[prevind].prv_pid_hash = (LONG) Ptr_seg.Nptrs;
                nxtp = hpp->l_prv = Ptr_seg.dptr->ps_l_tail;
                Ptr_seg.dptr->ps_l_tail = Ptr_seg.Nptrs;
                if  (nxtp < 0L)
                        Ptr_seg.dptr->ps_l_head = (LONG) Ptr_seg.Nptrs;
                else
                        Ptr_seg.plist[nxtp].l_nxt = (LONG) Ptr_seg.Nptrs;

                Ptr_seg.Nptrs++;
                hpp++;
                pp = &hpp->p;
        }

        free_rest((LONG) Ptr_seg.Nptrs, (LONG) Maxptrs);

        /* Initialise header structure. */

        Ptr_seg.dptr->ps_nptrs = Ptr_seg.Nptrs;
        Ptr_seg.dptr->ps_maxptrs = (unsigned) Maxptrs;
        Ptr_seg.dptr->ps_lastwrite = sbuf.st_mtime;
        Ptr_seg.dptr->ps_serial = 1;
        ptrs_unlock();
}

/* Rewrite printer file.
   If the number of printers has shrunk since
   the last time, squash it up (by recreating the file, as
   vanilla UNIX doesn't have a "truncate file" syscall [why
   not?]). Note that the indices of printers in the file may no
   longer correspond to those in the shm segment, but who cares,
   we aren't going to read it again before we finish. */

void  rewrpq()
{
        int     fnptrs, nptrs;
        LONG    pind;
        struct  flock   wlock;

        fnptrs = lseek(pfilefd, 0L, 2) / sizeof(struct spptr);
        nptrs = Ptr_seg.dptr->ps_nptrs;

        /* Cream out non-local printers */

        pind = Ptr_seg.dptr->ps_l_head;
        while  (pind >= 0L)  {
                Hashspptr  *pp = &Ptr_seg.plist[pind];
                if  (pp->p.spp_state != SPP_NULL  &&  pp->p.spp_netid != 0)
                        nptrs--;
                pind = pp->l_nxt;
        }

        wlock.l_type = F_WRLCK;
        wlock.l_whence = 0;
        wlock.l_start = 0L;
        wlock.l_len = 0L;

#ifdef  HAVE_FTRUNCATE
        while  (fcntl(pfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        report($E{Panic couldnt lock ptr file});
        }
        if  (nptrs < fnptrs)
                Ignored_error = ftruncate(pfilefd, 0L);
        lseek(pfilefd, 0L, 0);
#else
        if  (nptrs < fnptrs)  {

                /* Recreate file if we have less than last time */

                close(pfilefd);
                unlink(pfile);
                if  ((pfilefd = open(pfile, O_RDWR|O_CREAT, CMODE)) < 0)
                        report($E{Cannot create pfile});
#ifdef  RUN_AS_ROOT
                if  (Daemuid)
#if     defined(HAVE_FCHOWN) && !defined(M88000)
                        fchown(pfilefd, Daemuid, getegid());
#else
                        chown(pfile, Daemuid, getegid());
#endif
#endif /* RUN_AS_ROOT */
        }
        else
                lseek(pfilefd, 0L, 0);
        /* Note that there is a race if there's no ftruncate */
        while  (fcntl(pfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        report($E{Panic couldnt lock ptr file});
        }
#endif  /* HAVE_FTRUNCATE */

        pind = Ptr_seg.dptr->ps_l_head;
        while  (pind >= 0L)  {
                Hashspptr  *pp = &Ptr_seg.plist[pind];
                if  (pp->p.spp_state != SPP_NULL  &&  pp->p.spp_netid == 0)
                        Ignored_error = write(pfilefd, (char *) &pp->p, sizeof(struct spptr));
                pind = pp->l_nxt;
        }

        time(&Ptr_seg.dptr->ps_lastwrite);

        wlock.l_type = F_UNLCK;
        while  (fcntl(pfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        report($E{Panic couldnt lock ptr file});
        }
}

void  check_qclear(const LONG id)
{
        struct  spr_req buf;

        while  (msgrcv(Ctrl_chan, (struct msgbuf *) &buf, sizeof(buf)-sizeof(long), (long) id, IPC_NOWAIT) >= 0  ||  errno == EINTR)
                ;
}

/* Send IPM to printer */

void  pmsend(struct spptr *pp, struct spr_req *rq, const int bytes)
{
        if  (pp->spp_pid == 0)
                return;

        rq->spr_mtype = pp->spp_pid + MT_PMSG;

        for  (;;)  {
                if  (msgsnd(Ctrl_chan, (struct msgbuf *) rq, bytes, IPC_NOWAIT) >= 0)
                        return;

                /* Has printer died?  */

                if  (kill((PIDTYPE) pp->spp_pid, 0) < 0)
                        break;

                sleep(10);
        }

        check_qclear((LONG) (pp->spp_pid + MT_PMSG));
        pp->spp_state = SPP_HALT;
        pp->spp_sflags = 0;
        pp->spp_dflags = 0;
        pp->spp_pid = 0;
        Ptr_seg.dptr->ps_serial++;
        Ptr_seg.Last_ser++;
        qchanges++;
}

/* Reallocate printer shared segment if it grows too big.  We do this
   by allocating a new shared memory segment, and planting where
   we've gone in the old one.  */

void  growpseg()
{
        char            *newseg;
        unsigned        oldmaxp = Ptr_seg.dptr->ps_maxptrs;
        unsigned        Maxptrs = oldmaxp + INCNPTRS;
        struct  pshm_hdr*newdptr;
        LONG            *newhash;
        Hashspptr       *newlist;
        LONG            pind;
#ifndef USING_MMAP
        struct  spshm_info      new_info;
#endif
        struct  spr_req rq;

        ptrs_lock();

        Ptr_seg.Nptrs = Ptr_seg.dptr->ps_nptrs;

#ifdef  USING_MMAP
        Ptr_seg.inf.reqsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Maxptrs * sizeof(Hashspptr);
        gshmchan(&Ptr_seg.inf, PSHMOFF); /* Panics if it can't grow */
        Maxptrs = (Ptr_seg.inf.segsize - sizeof(struct pshm_hdr) - SHM_PHASHMOD * sizeof(LONG)) / sizeof(Hashspptr);
        newseg = Ptr_seg.inf.seg;
#else
        new_info = Ptr_seg.inf;
        new_info.base += SHMINC;
        new_info.reqsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Maxptrs * sizeof(Hashspptr);
        if  (!gshmchan(&new_info, PSHMOFF))
                report($E{Create pshm error});
        Maxptrs = (Ptr_seg.inf.segsize - sizeof(struct pshm_hdr) - SHM_PHASHMOD * sizeof(LONG)) / sizeof(Hashspptr);
        newseg = new_info.seg;
#endif

        newdptr = (struct pshm_hdr *) newseg;
        newhash = (LONG *) (newseg + sizeof(struct pshm_hdr));
        newlist = (Hashspptr *) ((char *) newhash + SHM_PHASHMOD * sizeof(LONG));

#ifdef  USING_MMAP
        newdptr->ps_serial++;
        Job_seg.dptr->js_psegid = Ptr_seg.inf.segsize;
#else
        /* Copy over stuff from old segment.  */

        newdptr->ps_lastwrite = Ptr_seg.dptr->ps_lastwrite;
        newdptr->ps_serial = Ptr_seg.dptr->ps_serial + 1;
        newdptr->ps_l_head = Ptr_seg.dptr->ps_l_head;
        newdptr->ps_l_tail = Ptr_seg.dptr->ps_l_tail;
        newdptr->ps_freech = Ptr_seg.dptr->ps_freech;
        BLOCK_COPY(newhash, Ptr_seg.hashp_pid, sizeof(LONG) * SHM_PHASHMOD);
        BLOCK_COPY(newlist, Ptr_seg.plist, sizeof(Hashspptr) * oldmaxp);

        /* Detach and remove old segment.  */

        shmdt(Ptr_seg.inf.seg);
        shmctl(Ptr_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
        Job_seg.dptr->js_psegid = Ptr_seg.inf.base;
        Ptr_seg.inf = new_info;
#endif

        Ptr_seg.dptr = newdptr;
        Ptr_seg.hashp_pid = newhash;
        Ptr_seg.plist = newlist;
        Ptr_seg.dptr->ps_nptrs = Ptr_seg.Nptrs;
        Ptr_seg.dptr->ps_maxptrs = (unsigned) Maxptrs;
        free_rest((LONG) oldmaxp, (LONG) Maxptrs);

        /* Prod spd processes.  */

        rq.spr_mtype = MT_SCHED;
        rq.spr_un.o.spr_act = SP_REMAP;

        for  (pind = Ptr_seg.dptr->ps_l_head;  pind >= 0L;  pind = Ptr_seg.plist[pind].l_nxt)  {
                struct  spptr  *np = &Ptr_seg.plist[pind].p;
                if  (np->spp_pid  &&  np->spp_netid == 0)
                        pmsend(np, &rq, sizeof(struct sp_omsg));
        }

        ptrs_unlock();
        if  (Netm_pid > 0)
                kill(Netm_pid, QRFRESH);
}

static void  takeoff_l(const LONG pind)
{
        struct  spptr   *cp = &Ptr_seg.plist[pind].p;
        LONG  nxtind = Ptr_seg.plist[pind].l_nxt;
        LONG  prvind = Ptr_seg.plist[pind].l_prv;
        unsigned  hashval = pid_hash(cp->spp_netid, cp->spp_rslot);

        if  (nxtind < 0L)
                Ptr_seg.dptr->ps_l_tail = prvind;
        else
                Ptr_seg.plist[nxtind].l_prv = prvind;

        if  (prvind < 0L)
                Ptr_seg.dptr->ps_l_head = nxtind;
        else  {
                Ptr_seg.plist[prvind].l_nxt = nxtind;
                Ptr_seg.plist[pind].l_prv = SHM_HASHEND;
        }

        Ptr_seg.plist[pind].l_nxt = Ptr_seg.dptr->ps_freech;
        Ptr_seg.dptr->ps_freech = pind;

        nxtind = Ptr_seg.plist[pind].nxt_pid_hash;
        prvind = Ptr_seg.plist[pind].prv_pid_hash;
        if  (prvind < 0L)
                Ptr_seg.hashp_pid[hashval] = nxtind;
        else
                Ptr_seg.plist[prvind].nxt_pid_hash = nxtind;
        if  (nxtind >= 0L)
                Ptr_seg.plist[nxtind].prv_pid_hash = prvind;

        Ptr_seg.dptr->ps_nptrs--;
        cp->spp_state = SPP_NULL;
        cp->spp_rjslot = cp->spp_jslot = -1;
        cp->spp_netid = 0;
        cp->spp_rslot = 0;
}

static void  puton_l(const LONG pind)
{
        LONG            prv;
        unsigned        hashval;
        Hashspptr       *hrs = &Ptr_seg.plist[pind];

        prv = hrs->l_prv = Ptr_seg.dptr->ps_l_tail;
        hrs->l_nxt = SHM_HASHEND;
        Ptr_seg.dptr->ps_l_tail = pind;
        if  (prv < 0L)
                Ptr_seg.dptr->ps_l_head = pind;
        else
                Ptr_seg.plist[prv].l_nxt = pind;

        /* Now put on hash chain.  */

        hashval = pid_hash(hrs->p.spp_netid, hrs->p.spp_rslot);
        hrs->nxt_pid_hash = prv = Ptr_seg.hashp_pid[hashval];
        Ptr_seg.hashp_pid[hashval] = pind;
        hrs->prv_pid_hash = SHM_HASHEND;
        if  (prv >= 0L)
                Ptr_seg.plist[prv].prv_pid_hash = pind;
        Ptr_seg.dptr->ps_nptrs++;
}

/* Remove records of remote printer when a remote machine snuffs it.
   This MUST be called after net_jclear as that routine relies on
   details of the dying printers being around.  */

void  net_pclear(const netid_t netid)
{
        int     done = 0;
        LONG    pind;

        ptrs_lock();
        pind = Ptr_seg.dptr->ps_l_head;
        while  (pind >= 0L)  {
                Hashspptr  *hcp = &Ptr_seg.plist[pind];
                struct  spptr   *cp = &hcp->p;
                LONG  nxtind = hcp->l_nxt;              /* Might get mangled by takeoff_l */
                if  (cp->spp_netid == netid)  {
                        takeoff_l(pind);
                        done++;
                }
                else  if  (cp->spp_netid == 0  &&  cp->spp_sflags & SPP_PROPOSED  &&  cp->spp_rjhostid == netid)  {

                        /* Catch half-proposals */

                        cp->spp_jslot = -1;
                        cp->spp_rjhostid = 0;
                        cp->spp_sflags = 0;
                        done++;
                }
                pind = nxtind;
        }

        if  (done)  {
                Ptr_seg.Last_ser++;
                qchanges++;
                Ptr_seg.dptr->ps_serial++;
        }
        ptrs_unlock();
}

/* Start up a printer.  This routine just initiates the process.  */

void  startp(Hashspptr *pp)
{
        PIDTYPE pid;
#ifdef  OLD_VERSION
        char    segn[10];
#endif
        char    slotn[10];      /*  Should be big enough!  */
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zc;
        zc.sighandler_el = SIG_DFL;
        sigmask_clear(zc);
        zc.sigflags_el = 0;
#endif

        while  ((pid = fork()) < 0)
                sleep(10);

        if  (pid > 0)  {        /*  Parent process  */
                pp->p.spp_pid = pid;
#ifdef  BUGGY_SIGCLD
                /* Fork again so that the scheduler is not the parent of the
                   daemon process and thus leaves a zombie process to be waited for.  */

                while  ((pid = fork()) < 0)
                        sleep(10);
                if  (pid != 0)
                        exit(0);
                nchild = 0;
#endif
                Ptr_seg.Last_ser++;
                qchanges++;
                Ptr_seg.dptr->ps_serial++;

                if  ((pp->p.spp_netflags & SPP_LOCALONLY) == 0)
                        ptr_broadcast(pp, SP_CHANGEDPTR);
                return;
        }

#ifndef BUGGY_SIGCLD
#ifdef  STRUCT_SIG
        sigact_routine(SIGCLD, &zc, (struct sigstruct_name *) 0);
#else
        signal(SIGCLD, SIG_DFL);
#endif
#endif

        /* In child process.
           Pass shm key/slot number as arguments (old version).
           Just pass slot number (new version) with ptr name to aid ps output */

#ifdef  OLD_VERSION
        sprintf(segn, "%d", Ptr_seg.inf.base);
        sprintf(slotn, "%ld", (long) (pp - Ptr_seg.plist));
        execl(daem, daem, segn, slotn, (char *) 0);
#else
        sprintf(slotn, "%ld", (long) (pp - Ptr_seg.plist));
        execl(daem, daem, pp->p.spp_ptr, slotn, (char *) 0);
#endif
        exit(255);
}

/* Abort specified printer.
   This is only applied to local printers.  */

void  prabort(struct spptr *pp)
{
        if  (pp->spp_state < SPP_PROC)
                return;

        if  (pp->spp_pid  &&  kill((PIDTYPE) pp->spp_pid, DAEMSTOP) < 0)  {
                pp->spp_state = SPP_ERROR;
                pp->spp_sflags = 0;
                pp->spp_dflags = 0;
                check_qclear((LONG) (pp->spp_pid + MT_PMSG));
                pp->spp_pid = 0;
                Ptr_seg.dptr->ps_serial++;
                Ptr_seg.Last_ser++;
                qchanges++;
        }
}

/* Tell printer to stop.  This routine is called for local printers only. */

void  finp(struct spptr *pp)
{
        struct  spr_req rq;

        if  (pp->spp_state >= SPP_PROC)  {
                rq.spr_mtype = MT_SCHED;
                rq.spr_un.o.spr_act = SP_FIN;
                pmsend(pp, &rq, sizeof(struct sp_omsg));
        }
}

/* Tell printer that 'awaiting oper' job has disappeared */

void  nowaiting(struct spptr *pp)
{
        struct  spr_req rq;

        rq.spr_mtype = MT_SCHED;
        rq.spr_un.o.spr_act = SP_PAB;
        pmsend(pp, &rq, sizeof(struct sp_omsg));
}

/* Add printer to list.
   We might be adding a local printer, or
   dealing with the addition of a non-local printer transmitted
   from some other machine.  */

void  addptr(struct sp_xpmsg *rq, struct spptr *pdet)
{
        LONG            pind;
        Hashspptr       *hrs;
        struct  spptr  *rs;

        /* See if printer allocated to that device.  If so, push off.
           Nov 2011 - ban similarly-named printers on same machine */

        pind = Ptr_seg.dptr->ps_l_head;
        while  (pind >= 0L)  {
                hrs = &Ptr_seg.plist[pind];
                rs = &hrs->p;
                if  (rs->spp_state != SPP_NULL  &&  rs->spp_netid == rq->spr_netid  &&
                     (ncstrcmp(rs->spp_ptr, pdet->spp_ptr) == 0  ||
                      ((rs->spp_netflags & SPP_LOCALNET) == 0  &&  strcmp(rs->spp_dev, pdet->spp_dev) == 0)))
                        return;
                pind = hrs->l_nxt;
        }

        if  (rq->spr_netid == 0)  {
                /* Local printers start off halted */
                pdet->spp_state = SPP_HALT;
                pdet->spp_sflags = 0;
                pdet->spp_dflags = 0;
                pdet->spp_feedback[0] = '\0';
        }

        if  (Ptr_seg.dptr->ps_nptrs >= Ptr_seg.dptr->ps_maxptrs)
                growpseg();

        ptrs_lock();
        pind = Ptr_seg.dptr->ps_freech;
        if  (pind < 0L)  {
                ptrs_unlock();
                report($E{Create pshm error});
        }
        hrs = &Ptr_seg.plist[pind];
        rs = &hrs->p;
        Ptr_seg.dptr->ps_freech = hrs->l_nxt;
        *rs = *pdet;
        if  ((rs->spp_netid = rq->spr_netid) == 0)
                rs->spp_rslot = pind;
        puton_l(pind);

        /* This next bit is not perfectly accurate but it means that
           if printer is added due to this machine being
           connected and something is being printed then it will
           display the job as being a local locally printed which
           is a fib.  I just didn't think that it was worth
           finding the job just for this case. Sorry.  */

        rs->spp_job = 0;
        rs->spp_rjhostid = 0;
        rs->spp_rjslot = rs->spp_jslot = -1;

        /* Force on local only bit if skipping networking.  */

        if  (!Network_ok)
                rs->spp_netflags |= SPP_LOCALONLY;

        rs->spp_pid = 0;
        ptrs_unlock();
        if  (rq->spr_netid == 0  &&  (rs->spp_netflags & SPP_LOCALONLY) == 0)
                ptr_broadcast(hrs, SP_ADDP);
        Ptr_seg.dptr->ps_serial++;
        Ptr_seg.Last_ser++;
        qchanges++;
}

/* Change printer - this is a command invoked by the user */

void  chgptr(struct sp_xpmsg *rq, struct spptr *newp)
{
        Hashspptr       *hcp;
        struct  spptr    *cp;
        int     newlocal, oldlocal;

        if  (rq->spr_pslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        hcp = &Ptr_seg.plist[rq->spr_pslot];
        cp = &hcp->p;

        if  (cp->spp_state == SPP_NULL)
                return;

        if  (cp->spp_netid)  {

                /* Remote printer - this might be a change notified by
                   that machine. Actually cp->spp_netid should ==
                   newp->spp_netid but we don't check.  */

                if  (rq->spr_netid)  {
                        newp->spp_rslot = cp->spp_rslot;
                        newp->spp_netid = cp->spp_netid;
                        newp->spp_jslot = cp->spp_jslot; /* Only changed by assign etc */
                        *cp = *newp;
                }
                else    {       /* Remote printer which I want to change from here.  */
                        ptr_sendupdate(cp, newp, SP_CHGP);
                        return;
                }
        }
        else  {
                if  (!(newp->spp_netflags & SPP_LOCALNET))  {
                        LONG  pind = Ptr_seg.dptr->ps_l_head;
                        while  (pind >= 0L)  {
                                struct  spptr  *rs = &Ptr_seg.plist[pind].p;
                                pind = Ptr_seg.plist[pind].l_nxt;

                                /* See if printer allocated to that device.  If so, push off.
                                   Nov 2011 - ban simiilarly-named printers on the same machine */

                                if  (rs != cp  &&  rs->spp_state != SPP_NULL  &&  rs->spp_netid == 0  &&
                                     (ncstrcmp(rs->spp_ptr, newp->spp_ptr) == 0  ||
                                     (!(rs->spp_netflags & SPP_LOCALNET)  &&  strcmp(rs->spp_dev, newp->spp_dev) == 0)))
                                        return;
                        }
                }

                /* We are talking about a local printer.  Check
                   process id number because the spd process
                   might not have set the state yet if just
                   started up.  */

                if  (cp->spp_pid)
                        return;

                /* Set these to a known value...  */

                newp->spp_job = 0;
                newp->spp_rjhostid = 0;
                newp->spp_rjslot = newp->spp_jslot = -1;
                newp->spp_state = SPP_HALT;
                newp->spp_sflags = 0;
                newp->spp_dflags = 0;
                newp->spp_pid = 0;
                newp->spp_rslot = rq->spr_pslot;
                newp->spp_netid = 0;

                /* Force on local only bit if skipping networking.  */

                if  (!Network_ok)
                        newp->spp_netflags |= SPP_LOCALONLY;

                oldlocal = cp->spp_netflags & SPP_LOCALONLY;
                newlocal = newp->spp_netflags & SPP_LOCALONLY;
                *cp = *newp;

                if  (oldlocal != newlocal)  {
                        if  (newlocal)
                                ptr_broadcast(hcp, SO_DELP);
                        else
                                ptr_broadcast(hcp, SP_ADDP);
                }
                else  if  (!newlocal)
                        ptr_broadcast(hcp, SP_CHANGEDPTR);
        }
        Ptr_seg.dptr->ps_serial++;
        Ptr_seg.Last_ser++;
        qchanges++;
}

/* Note remote printer no longer assigned */

void  unassign_ptr(struct sp_xpmsg *rq, struct spptr *newp)
{
        struct  spptr    *cp;

        if  (rq->spr_pslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        cp = &Ptr_seg.plist[rq->spr_pslot].p;
        if  (cp->spp_state == SPP_NULL  ||  cp->spp_netid == 0)
                return;
        cp->spp_rjslot = cp->spp_jslot = -1;
        cp->spp_job = 0;
        cp->spp_rjhostid = 0;
        cp->spp_state = newp->spp_state;
        cp->spp_sflags = newp->spp_sflags;
        cp->spp_dflags = newp->spp_dflags;
        Ptr_seg.dptr->ps_serial++;
        Ptr_seg.Last_ser++;
        qchanges++;
}

/* Delete printer from list.  */

void  delptr(struct sp_omsg *rq)
{
        Hashspptr  *hcp;
        struct  spptr  *cp;

        if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        hcp = &Ptr_seg.plist[rq->spr_jpslot];
        cp = &hcp->p;
        if  (cp->spp_state == SPP_NULL || cp->spp_pid != 0)
                return;
        if  (cp->spp_netid)  {
                if  (rq->spr_netid == 0)  {
                        ptr_message(cp, SO_DELP);
                        return;
                }
        }
        else  if  ((cp->spp_netflags & SPP_LOCALONLY) == 0)
                ptr_broadcast(hcp, SO_DELP);
        ptrs_lock();
        Ptr_seg.dptr->ps_serial++;
        takeoff_l(rq->spr_jpslot);
        ptrs_unlock();
        Ptr_seg.Last_ser++;
        qchanges++;
}

/* Set printer going - i.e. if halted, start up process ready for first request.  */

void  gop(struct sp_omsg *rq)
{
        Hashspptr  *hcp;
        struct  spptr   *cp;

        if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        hcp = &Ptr_seg.plist[rq->spr_jpslot];
        cp = &hcp->p;
        if  (cp->spp_state == SPP_NULL)
                return;
        if  (cp->spp_netid)
                ptr_message(cp, SO_PGO);
        else  {
                if  (cp->spp_pid)  {
                        if  (cp->spp_sflags & SPP_HEOJ)  {
                                cp->spp_sflags &= ~SPP_HEOJ;
                                Ptr_seg.Last_ser++;
                                qchanges++;
                                Ptr_seg.dptr->ps_serial++;
                                if  ((cp->spp_netflags & SPP_LOCALONLY) == 0)
                                        ptr_broadcast(hcp, SP_CHANGEDPTR);
                        }
                }
                else
                        startp(hcp);
        }
}

/* Tell printer to stop at the end of the current job.  */

void  heoj(struct sp_omsg *rq)
{
        Hashspptr  *cp;

        if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        cp = &Ptr_seg.plist[rq->spr_jpslot];
        if  (cp->p.spp_netid)  {
                ptr_message(&cp->p, SO_PHLT);
                return;
        }
        switch  (cp->p.spp_state)  {
        default:
                return;

        case  SPP_INIT:
        case  SPP_RUN:
                cp->p.spp_sflags |= SPP_HEOJ;
                Ptr_seg.Last_ser++;
                qchanges++;
                Ptr_seg.dptr->ps_serial++;
                if  ((cp->p.spp_netflags & SPP_LOCALONLY) == 0)
                        ptr_broadcast(cp, SP_CHANGEDPTR);
                break;

        case  SPP_WAIT:
        case  SPP_OPER:
                finp(&cp->p);
                break;
        }
}

/* Tell printer to halt whatever it is doing.  */

void  halt(struct sp_omsg *rq)
{
        struct  spptr  *cp;

        if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        cp = &Ptr_seg.plist[rq->spr_jpslot].p;
        if  (cp->spp_netid)  {
                ptr_message(cp, SO_PSTP);
                return;
        }

        switch  (cp->spp_state)  {
        case  SPP_INIT:
        case  SPP_RUN:
        case  SPP_SHUTD:
                cp->spp_sflags &= ~SPP_INTER;
                prabort(cp);
        case  SPP_WAIT:
        case  SPP_OPER:
                finp(cp);
                return;
        }
}

/* Halt everything.  */

void  haltall()
{
        LONG  pind = Ptr_seg.dptr->ps_l_head;

        while  (pind >= 0L)  {
                struct  spptr  *cp = &Ptr_seg.plist[pind].p;
                pind = Ptr_seg.plist[pind].l_nxt;

                if  (cp->spp_netid)
                        continue;

                switch  (cp->spp_state)  {
                default:
                        continue;
                case  SPP_INIT:
                case  SPP_RUN:
                case  SPP_SHUTD:
                        cp->spp_sflags &= ~SPP_INTER;
                        prabort(cp);
                case  SPP_WAIT:
                case  SPP_OPER:
                        finp(cp);
                        continue;
                }
        }
}

/* Restart current job. */

void  restart(struct sp_omsg *rq)
{
        struct  spptr  *cp;
        if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        cp = &Ptr_seg.plist[rq->spr_jpslot].p;
        if  (cp->spp_netid)
                ptr_message(cp, SO_RSP);
        else  if  (cp->spp_state == SPP_RUN)
                kill((PIDTYPE) cp->spp_pid, DAEMRST);
}

/* Send start/continue job message.  */

void  startj(Hashspq *jp, struct spptr *pp)
{
        struct  spr_req req;

        pp->spp_sflags = SPP_SELECT;
        req.spr_mtype = MT_SCHED;
        req.spr_un.o.spr_act = SP_REQ;
        req.spr_un.o.spr_pid = pp->spp_pid;
        req.spr_un.o.spr_seq = 0;
        req.spr_un.o.spr_netid = 0;
        req.spr_un.o.spr_jpslot = jp - Job_seg.jlist;
        req.spr_un.o.spr_jobno = jp->j.spq_job;
        req.spr_un.o.spr_arg1 = 0;
        req.spr_un.o.spr_arg2 = 0;
        pmsend(pp, &req, sizeof(struct sp_omsg));
}

/* Deal with "proposals" from remote machine to print one of "my" jobs.
   "spr_jpslot" gives the slot number of the job in my list.
   "spr_arg1" gives the slot number of the job in the remote machine's list
   and "spr_arg2" that of the printer in the remote machine's list.  */

void  rempropose(struct sp_omsg *rq)
{
        struct  spq  *jp;

        if  (rq->spr_jpslot >= Job_seg.dptr->js_maxjobs)
                return;
        jp = &Job_seg.jlist[rq->spr_jpslot].j;
        if  (jp->spq_job != rq->spr_jobno  ||  jp->spq_netid) /* Changed or not mine any more */
                job_message(rq->spr_netid, jp, SO_PROP_DEL, rq->spr_arg1, rq->spr_arg2);
        else  if  (jp->spq_sflags & SPQ_ASSIGN)
                job_message(rq->spr_netid, jp, SO_PROP_NOK, rq->spr_arg1, rq->spr_arg2);
        else  {
                jp->spq_sflags |= SPQ_ASSIGN | SPQ_PROPOSED;
                /* Put netid in pslot field in case the machine dies in the meantime.  */
                jp->spq_pslot = rq->spr_netid;
                job_message(rq->spr_netid, jp, SO_PROP_OK, rq->spr_arg1, rq->spr_arg2);
                Job_seg.dptr->js_serial++;
        }
}

static void  ptr_unpropose(struct spptr *pp)
{
        pp->spp_job = 0;
        pp->spp_rjhostid = 0;
        pp->spp_rjslot = pp->spp_jslot = -1;
        pp->spp_sflags &= ~(SPP_SELECT | SPP_PROPOSED);
        Ptr_seg.dptr->ps_serial++;
}

/* Handle confirmation of print */

void  confirm_print(struct sp_omsg *rq)
{
        slotno_t        ind;
        Hashspq         *hjp;
        Hashspptr       *hpp;
        struct  spq     *jp;
        struct  spptr   *pp;

        /* The job slot number in fact is the slot number of the stub
           entry on this machine even though the master is on the
           machine which sent this message. The slot number
           started life in spr_arg1 but got moved to spr_jpslot
           in "job_recv" in sh_network.c.  */

        if  (rq->spr_jpslot >= Job_seg.dptr->js_maxjobs)
                return;
        if  (rq->spr_arg2 >= Ptr_seg.dptr->ps_maxptrs)
                return;
        hjp = &Job_seg.jlist[rq->spr_jpslot];
        jp = &hjp->j;
        hpp = &Ptr_seg.plist[rq->spr_arg2];
        pp = &hpp->p;

        /* Cross-check, but cancel settings on printer (job might have
           disappeared altogether before we even get here). */

        if  (pp->spp_state != SPP_WAIT)
                return;
        if  ((pp->spp_sflags & (SPP_SELECT|SPP_PROPOSED)) != (SPP_SELECT|SPP_PROPOSED))
                return;
        if  (jp->spq_job != rq->spr_jobno || jp->spq_netid != rq->spr_netid ||
                        !(jp->spq_sflags & SPQ_PROPOSED))  {
                ptr_unpropose(pp);
                return;
        }

        /* Now proceed according to response Assume ok unless otherwise...  */

        switch  (rq->spr_act)  {
        default:
                jp->spq_sflags &= ~SPQ_PROPOSED;
                pp->spp_job = jp->spq_job;
                pp->spp_rjhostid = jp->spq_netid;
                pp->spp_rjslot = jp->spq_rslot;
                pp->spp_jslot = hjp - Job_seg.jlist;
                ptr_assxmit(hjp, hpp);
                startj(hjp, pp);
                Job_seg.dptr->js_serial++;
                break;

        case  SO_PROP_NOK:
                /* Knock back job flags to just "assign", so that if
                   we go back into selectj before we get some
                   other sort of message from the owning machine
                   we won't go round again.  */
                jp->spq_sflags = SPQ_ASSIGN;
                jp->spq_pslot = -1;
                Job_seg.dptr->js_serial++;
                ptr_unpropose(pp);
                break;

        case  SO_PROP_DEL:
                /* The job seems to have disappeared...  */
                if  ((ind = find_rslot(rq->spr_netid, (slotno_t) rq->spr_jpslot)) >= 0)
                        dequeue(ind);
                ptr_unpropose(pp);
                break;
        }
}

/* Proceed to assign a job (possibly a remote job, local copy) to a
   printer, which will always be a local printer.  */

void  assign(Hashspq *hjp, Hashspptr *hpp)
{
        struct  spq     *jp = &hjp->j;
        struct  spptr   *pp = &hpp->p;

#ifdef  SHED_FORMASSIGN
        /* Copy form type of job into form type of printer in case there is a suffix.  */

        strncpy(pp->spp_form, jp->spq_form, sizeof(jp->spq_form));
#endif

        /* Set assigned bit in job.  This determines that selectj won't find it again.  */

        jp->spq_sflags |= SPQ_ASSIGN;
        jp->spq_pslot = hpp - Ptr_seg.plist;
        pp->spp_job = jp->spq_job;
        pp->spp_rjhostid = jp->spq_netid;
        pp->spp_rjslot = jp->spq_rslot;         /* spq_rslot now valid for local jobs */

        /* Set these 'ere we forget (note nice poetic touch).  */

        Ptr_seg.dptr->ps_serial++;
        Job_seg.dptr->js_serial++;
        Job_seg.Last_ser++;
        qchanges++;

        if  (jp->spq_netid)  {
                pp->spp_sflags = SPP_SELECT | SPP_PROPOSED;
                jp->spq_sflags |= SPQ_PROPOSED;
                job_message(jp->spq_netid, jp, SO_PROPOSE, (ULONG) (hjp - Job_seg.jlist), (ULONG) (hpp - Ptr_seg.plist));
        }
        else  {
                pp->spp_jslot = hjp - Job_seg.jlist;
                if  (!(jp->spq_jflags & SPQ_LOCALONLY))  {
                        if  (pp->spp_netflags & SPP_LOCALONLY)
                                job_broadcast(hjp, SO_LOCASSIGN);
                        else
                                ptr_assxmit(hjp, hpp);
                }
                startj(hjp, pp);
        }
}

/* This is the routine which allocates jobs to printers.  */

unsigned  selectj()
{
        LONG            pind, jind;
        char            *dp;
        time_t          now = time((time_t *) 0);
        unsigned        wtime = 0;
        int             fl, ch;

        if  (suspend_sched > now)
                return  (unsigned) (suspend_sched - now);

        /* Run through the list of (my) printers looking for any in WAIT state.  */

        pind = Ptr_seg.dptr->ps_l_head;
        while  (pind >= 0L)  {

                Hashspptr  *hpp = &Ptr_seg.plist[pind];
                struct  spptr   *pp = &hpp->p;
                pind = hpp->l_nxt;

                if  (pp->spp_state != SPP_WAIT  ||  pp->spp_sflags & SPP_SELECT  ||  pp->spp_netid)
                        continue;

                /* Having found such a beast, look through the list of
                   jobs for one with the right paper, which
                   either specifies that printer or no printer.  */

                if  ((dp = strpbrk(pp->spp_form, Sufchars)))
                        fl = dp - pp->spp_form;
                else
                        fl = strlen(pp->spp_form);

                jind = Job_seg.dptr->js_q_head;
                while  (jind >= 0L)  {

                        Hashspq *hjp = &Job_seg.jlist[jind];
                        struct  spq     *jp = &hjp->j;

                        jind = hjp->q_nxt;

                        /* Reject job if ....

                           ...no copies are to be printed */

                        if  (jp->spq_cps == 0)
                                continue;

                        /* ...Job assigned to some printer.

                           This flag is also set if we started to assign a
                           remote job but the other end wasn't
                           interested, the assumption being that
                           that end is shortly about to tell us
                           what is really going on.  Also if
                           being aborted but not filtered through yet. */

                        if  (jp->spq_sflags & (SPQ_ASSIGN|SPQ_ABORTJ))
                                continue;

                        /* ...If holding until a given time and that time hasn't arrived yet.  */

                        if  ((time_t) jp->spq_hold > now)  {
                                time_t  p = (time_t) jp->spq_hold - now;
                                if  (wtime == 0  ||  p < wtime)
                                        wtime = p;
                                continue;
                        }

                        /* ...If outside size range */

                        if  ((ULONG)jp->spq_size < pp->spp_minsize)
                                continue;
                        if  (pp->spp_maxsize  &&  (ULONG)jp->spq_size > pp->spp_maxsize)
                                continue;

                        /* ...If printer is a local printer and job is a remote job.  */

                        if  (jp->spq_netid && (pp->spp_netflags & SPP_LOCALONLY))
                                continue;

                        /* ...If class codes don't intersect */

                        if  ((jp->spq_class & pp->spp_class) == 0)
                                continue;

                        /* ...If the form type up to the suffix doesn't match */

                        if  (ncstrncmp(jp->spq_form, pp->spp_form, fl) != 0)
                                continue;
                        ch = jp->spq_form[fl];
                        if  (ch != '\0' && !strchr(Sufchars, ch))
                                continue;

                        /* ...If a printer type is specified and doesn't match.  */

                        if  (jp->spq_ptr[0] != '\0'  &&  !qmatch(jp->spq_ptr, pp->spp_ptr))
                                continue;

                        /* We are now ready to print the job.
                           "break" because we have finished with the
                           printer in question.  */

                        assign(hjp, hpp);
                        break;
                }
        }

        /* Warn users about jobs at the top of the queue */

        jind = Job_seg.dptr->js_q_head;
        while  (jind >= 0L)  {
                Hashspq *hjp = &Job_seg.jlist[jind];
                struct  spq     *jp = &hjp->j;
                jind = hjp->q_nxt;
                if  (jp->spq_netid)
                        continue;
                if  ((time_t) jp->spq_hold > now)
                        continue;
                if  (jp->spq_dflags & (SPQ_PRINTED|SPQ_PQ))
                        break;
                if  (!(jp->spq_sflags & SPQ_WARNED))  {
                        jp->spq_sflags |= SPQ_WARNED;
                        if  (jp->spq_jflags & (SPQ_MATTN|SPQ_WATTN))
                                notify(jp, (struct spptr *) 0, $E{Job awoper msg}, (jobno_t) 0, PRESENT_TENSE);
                }
        }

        return  wtime;
}

/* Note printer job done. 'ab' is SPD_DONE SPD_DAB or SPD_DERR
   This "can only happen" for a local printer. */

void  prdone(struct sp_cmsg *rq, const unsigned ab)
{
        Hashspptr  *hcp;
        Hashspq    *hjp;
        struct  spptr   *cp;
        struct  spq  *jp;
        int     wass;

        if  (rq->spr_pslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        hcp = &Ptr_seg.plist[rq->spr_pslot];
        cp = &hcp->p;

        wass = cp->spp_sflags;
        cp->spp_sflags = 0;
        Ptr_seg.dptr->ps_serial++;
        Ptr_seg.Last_ser++;
        qchanges++;
        if  ((cp->spp_netflags & SPP_LOCALONLY) == 0)
                ptr_broadcast(hcp, SP_CHANGEDPTR);

        /* If no such job, must have been aborted.  Select new state and return.  */

        if  (cp->spp_jslot < 0  ||  cp->spp_jslot >= Job_seg.dptr->js_maxjobs)  {
                /*  Unknown job  */
                if  (ab == SPD_DERR)
                        unlink(mkspid(ERNAM, cp->spp_job));
                if  (cp->spp_sflags & SPP_HEOJ)
                        finp(cp);
                return;
        }

        hjp = &Job_seg.jlist[cp->spp_jslot];
        jp = &hjp->j;

        /* If job was aborted, don't count the copy.  */

        if  (ab == SPD_DAB)  {
                unassign(hjp, hcp);
                /* Abort flag set on job to denote that we sent the
                   abort message to the printer to say we were deleting the job.  */
                if  (jp->spq_sflags & SPQ_ABORTJ)  {
                        notify(jp, cp, $E{Job aborted msg}, jp->spq_job, PAST_TENSE);
                        delete_job(hjp);
                }
                if  (wass & SPP_HEOJ)
                        finp(cp);
                return;
        }

        /* Otherwise decrement number of copies.  */

        if  (ab == SPD_DERR)  {
                jp->spq_cps = 0;
                notify(jp, cp, $E{Job errors msg}, jp->spq_job, PAST_TENSE);
                unassign(hjp, hcp);
                if  (!(jp->spq_jflags & SPQ_RETN || rq->spr_flags & SPF_RETAIN) || jp->spq_sflags & SPQ_ABORTJ)
                        delete_job(hjp);
                else  {
                        jp->spq_time = (LONG) time((time_t *) 0);
                        jp->spq_sflags &= ~SPQ_ABORTJ;
                }
        }
        else  {
                jp->spq_cps--;
                if  (rq->spr_flags & SPF_NOCOPIES) /* Force to zero if onecopy only set */
                        jp->spq_cps = 0;
                unassign(hjp, hcp);
                if  (jp->spq_cps != 0)  {
                        if  (jp->spq_sflags & SPQ_ABORTJ)  { /* Got abort just as it finished */
                                jp->spq_cps = 0;
                                delete_job(hjp);
                        }
                }
                else  {
                        notify(jp, cp, $E{Job completed msg}, (jobno_t) 0, PAST_TENSE);
                        if  (!(jp->spq_jflags & SPQ_RETN || rq->spr_flags & SPF_RETAIN) || jp->spq_sflags & SPQ_ABORTJ)
                                delete_job(hjp);
                        else
                                jp->spq_time = (LONG) time((time_t *) 0);
                }
        }

        if  (wass & SPP_HEOJ)
                finp(cp);
}

/* Note that printer has finished.  */

void  prdfin(struct sp_cmsg *rq)
{
        Hashspptr       *hcp;
        struct  spptr  *cp;

        if  (rq->spr_pslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        hcp = &Ptr_seg.plist[rq->spr_pslot];
        cp = &hcp->p;
        cp->spp_sflags = 0;
        cp->spp_dflags = 0;
        check_qclear((LONG) (rq->spr_pid + MT_PMSG));
        cp->spp_pid = 0;

        /* If we had just marked a job as allocated to that printer, unmark it.  */

        if  (cp->spp_jslot >= 0  &&  cp->spp_jslot < Job_seg.dptr->js_maxjobs)  {
                Hashspq  *hjp = &Job_seg.jlist[cp->spp_jslot];
                hjp->j.spq_dflags &= ~SPQ_PQ;
                unassign(hjp, hcp);
        }
        Ptr_seg.Last_ser++;
        qchanges++;
        Ptr_seg.dptr->ps_serial++;
        if  (cp->spp_state < SPP_HALT)
                ptrnotify(cp);
        if  ((cp->spp_netflags & SPP_LOCALONLY) == 0)
                ptr_broadcast(hcp, SP_CHANGEDPTR);
}

/* Respond to state change message.  */

void  proper(struct sp_cmsg *rq)
{
        Hashspptr       *hcp;
        struct  spptr  *cp;

        if  (rq->spr_pslot >= Ptr_seg.dptr->ps_maxptrs)
                return;

        hcp = &Ptr_seg.plist[rq->spr_pslot];
        cp = &hcp->p;

        /* If halting at end of job, do it now.
           Otherwise ignore it if not supposed to be running.  */

        if  (cp->spp_sflags & SPP_HEOJ)  {
                finp(cp);
                return;
        }

        if  (cp->spp_state < SPP_PROC)
                return;

        /* If awaiting oper and job is no longer there, then tell printer. */

        if  (cp->spp_state == SPP_OPER)  {
                Hashspq         *hjj = &Job_seg.jlist[cp->spp_jslot];
                struct  spq     *jj = &hjj->j;
                if  (cp->spp_jslot < 0  ||  jj->spq_job != cp->spp_job)  {
                        nowaiting(cp);
                        Ptr_seg.Last_ser++;
                        qchanges++;
                        Ptr_seg.dptr->ps_serial++;
                        if  ((cp->spp_netflags & SPP_LOCALONLY) == 0)
                                ptr_broadcast(hcp, SP_CHANGEDPTR);
                        return;
                }

                if  (jj->spq_jflags & (SPQ_MATTN|SPQ_WATTN))
                        notify(jj, cp, $E{Job awoper msg}, (jobno_t) 0, PRESENT_TENSE);
        }

        Ptr_seg.Last_ser++;
        qchanges++;
        Ptr_seg.dptr->ps_serial++;
        if  ((cp->spp_netflags & SPP_LOCALONLY) == 0)
                ptr_broadcast(hcp, SP_CHANGEDPTR);
}

/* Deal with printing job being aborted */

void  prjab(struct sp_omsg *rq)
{
        Hashspptr       *hcp;
        struct  spptr  *cp;

        if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                return;

        hcp = &Ptr_seg.plist[rq->spr_jpslot];
        cp = &hcp->p;

        if  (cp->spp_netid)
                ptr_message(cp, SO_PJAB);
        else  {
                if  (cp->spp_jslot >= 0  &&  cp->spp_jslot < Job_seg.dptr->js_maxjobs)
                        Job_seg.jlist[cp->spp_jslot].j.spq_sflags |= SPQ_ABORTJ;

                if  (cp->spp_state >= SPP_PROC)  {
                        cp->spp_sflags &= SPP_HEOJ; /* Just want to keep this */
                        Ptr_seg.dptr->ps_serial++;
                        if  (cp->spp_state == SPP_OPER)
                                nowaiting(cp);
                        else  if  (cp->spp_state == SPP_RUN)
                                prabort(cp);
                        if  (cp->spp_state != SPP_SHUTD  &&  cp->spp_sflags & SPP_HEOJ)
                                finp(cp);
                }
        }
}

/* Deal with responses from operator to awaiting operator state.  */

void  msgptr(struct sp_omsg *rq)
{
        Hashspptr       *hcp;
        struct  spptr  *cp;
        struct  spr_req  prq;

        if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                return;

        hcp = &Ptr_seg.plist[rq->spr_jpslot];
        cp = &hcp->p;

        if  (cp->spp_netid)  {
                ptr_message(cp, (int) rq->spr_act);
                return;
        }

        if  (cp->spp_state != SPP_OPER)  {
                if  (cp->spp_state == SPP_WAIT)  {
                        if  (rq->spr_act == SO_ONO)
                                cp->spp_dflags |= SPP_REQALIGN;
                        else
                                cp->spp_dflags &= ~SPP_REQALIGN;
                        Ptr_seg.Last_ser++;
                        qchanges++;
                        Ptr_seg.dptr->ps_serial++;
                        if  ((cp->spp_netflags & SPP_LOCALONLY) == 0)
                                ptr_broadcast(hcp, SP_CHANGEDPTR);
                }
                return;
        }

        prq.spr_mtype = MT_SCHED;
        prq.spr_un.o.spr_act = (rq->spr_act == SO_ONO)? SP_PNO: SP_PYES;
        prq.spr_un.o.spr_pid = cp->spp_pid;
        prq.spr_un.o.spr_seq = 0;
        pmsend(cp, &prq, sizeof(struct sp_omsg));
}

/* Deal with interrupt job.  */

void  interrupt(struct sp_omsg *rq)
{
        Hashspptr       *hcp;
        struct  spptr  *cp;

        if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                return;
        hcp = &Ptr_seg.plist[rq->spr_jpslot];
        cp = &hcp->p;
        if  (cp->spp_state != SPP_RUN  ||  cp->spp_sflags & SPP_INTER)
                return;
        if  (cp->spp_netid)
                ptr_message(cp, SO_INTER);
        else  {
                cp->spp_sflags |= SPP_INTER;
                Ptr_seg.dptr->ps_serial++;
                prabort(cp);
        }
}
