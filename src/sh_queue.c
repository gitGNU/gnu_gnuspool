/* sh_queue.c -- spool scheduling mostly jobs and queue handling

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
#include "errnums.h"
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

void  notify(struct spq *, struct spptr *, const int, const jobno_t, const int);
void  job_broadcast(Hashspq *, const int);
void  job_sendupdate(struct spq *, struct spq *, const int);
void  job_message(const netid_t, struct spq *, const int, const ULONG, const ULONG);
void  ptr_sendupdate(struct spptr *, struct spptr *, const int);
void  ptr_message(struct spptr *, const int);
void  ptr_broadcast(Hashspptr *, const int);
void  nowaiting(struct spptr *);
void  pmsend(struct spptr *, struct spr_req *, const int);
void  prabort(struct spptr *);
void  prjab(struct sp_omsg *);
void  report(const int);
void  nfreport(const int);
int  gshmchan(struct spshm_info *, const int);

extern  uid_t   Daemuid;
extern  float   pri_decrement;

int     jfilefd;

extern  int     qchanges;

extern  USHORT  vportnum;

char    jfile[] = JFILE;

#define CMODE           0666
#define IPC_MODE        0600
#define INITNJOBS       INITJALLOC
#define INCNJOBS        (INITJALLOC/2)

#ifndef USING_FLOCK
static  struct  sembuf  jw[2] = {{      JQ_READING,     0,      0 },
                                 {      JQ_FIDDLE,      1,      0 }},
                        ju[1] = {{      JQ_FIDDLE,      -1,     0 }};
#endif

extern  int     Network_ok;
extern  PIDTYPE Netm_pid;

#ifdef  OS_FREEBSD
#define _SC_PAGE_SIZE _SC_PAGESIZE
#endif

#ifdef  USING_FLOCK
void  setjhold(const int typ)
{
        struct  flock   lck;
        lck.l_type = typ;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Job_seg.iinf.mmfd, F_SETLKW, &lck) >= 0)
                        return;
#else
                if  (fcntl(Job_seg.iinf.lockfd, F_SETLKW, &lck) >= 0)
                        return;
#endif
                if  (errno != EINTR)
                        report($E{Lock error});
        }
}

static void  jobs_lock()
{
        setjhold(F_WRLCK);
}

static void  jobs_unlock()
{
#ifdef  USING_MMAP
        msync(Job_seg.iinf.seg, Job_seg.iinf.segsize, MS_SYNC|MS_INVALIDATE);
        msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
        setjhold(F_UNLCK);
}

#else
static void  jobs_lock()
{
        for  (;;)  {
                if  (semop(Sem_chan, jw, 2) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                report($E{Semaphore error probably undo});
        }
}

static void  jobs_unlock()
{
#ifdef  USING_MMAP
        msync(Job_seg.iinf.seg, Job_seg.iinf.segsize, MS_SYNC|MS_INVALIDATE);
        msync(Job_seg.dinf.seg, Job_seg.dinf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
        for  (;;)  {
                if  (semop(Sem_chan, ju, 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                report($E{Semaphore error probably undo});
        }
}
#endif

inline SHORT  round_short(float flt)
{
        if  (flt < 0.0)  {
                flt -= .5;
                if  (flt < -32768.0)
                        return  -32768;
        }
        else  {
                flt += .5;
                if  (flt > 32767.0)
                        return  32767;
        }
        return  (SHORT) flt;
}

static void  free_rest(const LONG jind, const LONG njbs)
{
        Hashspq         *jj;
        LONG            cnt;

        BLOCK_ZERO(&Job_seg.jlist[jind], (njbs - jind) * sizeof(Hashspq));

        /* We do this loop backwards to that the new ones go on the end of the free chain.  */

        jj = &Job_seg.jlist[njbs];
        cnt = njbs;

        while  (cnt > jind)  {
                jj--;
                jj->j.spq_pslot = -1;
                jj->nxt_jno_hash = jj->prv_jno_hash = jj->nxt_jid_hash = jj->prv_jid_hash = jj->q_prv = SHM_HASHEND;
                jj->q_nxt = Job_seg.dptr->js_freech;
                Job_seg.dptr->js_freech = --cnt;
        }
}

static void  val_dir(char *d)
{
        extern  char    *spdir;
        struct  stat    sbuf;

        disp_str = spdir;
        disp_str2 = d;

        if  (stat(d, &sbuf) < 0)
                report(d[0] == '.'?
                       $E{cannot stat spool directory}:
                       $E{cannot stat spool sub directory});

        if  (sbuf.st_uid != Daemuid)  {
                disp_arg[7] = Daemuid;
                disp_arg[8] = sbuf.st_uid;
                report(d[0] == '.'?
                       $E{spool directory not owned by spooler}:
                       $E{subdirectory not owned by spooler});
        }

        if  ((sbuf.st_mode & 0700) != 0700)  {
                disp_arg[8] = sbuf.st_mode & 0777;
                report(d[0] == '.'?
                       $E{spool directory invalid permissions}:
                       $E{subdirectory invalid permissions});
        }
}

/* Validate the ownership etc of the spool directory(ies)
   This is only a very basic check. */

void  valspdir()
{
        extern  int     spid_subdirs;
        int     cnt;

        val_dir(".");
        mkspid("", 1);          /* To force interpretation of $SPOOLSUBDS */

        if  (spid_subdirs <= 0  ||  spid_subdirs > 999)
                return;

        for  (cnt = 0;  cnt < spid_subdirs;  cnt++)  {
                char    buf[4];
#ifdef  CHARSPRINTF
                val_dir(sprintf(buf, "%.3d", cnt));
#else
                sprintf(buf, "%.3d", cnt);
                val_dir(buf);
#endif
        }
}

/* Create/open job file. Read it into memory if it exists. */

void  createjfile(int jsize)
{
#if     defined(USING_MMAP) || !defined(NO_ROUND_PAGES)
        LONG    pagesize = sysconf(_SC_PAGE_SIZE);
#endif
        Hashspq         *hjj;
        struct  spq     *jj;
        int             Maxjobs;
        unsigned        hashval;
        LONG            nxtj, prevind;
        struct  stat    sbuf;
#ifdef  USING_MMAP
        char            byte = 0;
#endif

        if  ((jfilefd = open(jfile, O_RDWR|O_CREAT, CMODE)) < 0)
                report($E{Cannot create jfile});

#ifdef  RUN_AS_ROOT
#if     defined(HAVE_FCHOWN) && !defined(M88000)
        if  (Daemuid)
                fchown(jfilefd, Daemuid, getegid());
#else
        if  (Daemuid)
                chown(jfile, Daemuid, getegid());
#endif
#endif
        fcntl(jfilefd, F_SETFD, 1);

        if  (jsize <= 0)
                jsize = INITNJOBS;

        /* Initialise shared memory segment to size of file, rounding up. */

#ifndef USING_FLOCK
        /* If we're using semaphores lock it now as semaphore created.
           If we're using file locking we'll have to wait until the file descr is open.
           I don't think the race condition matters as no one will try to lock it
           before they've tried and to access the shared memory segment or mmap file */
        jobs_lock();
#endif
        fstat(jfilefd, &sbuf);
        Maxjobs = sbuf.st_size / sizeof(struct spq);
        if  (Maxjobs < jsize)
                Maxjobs = jsize;

        Job_seg.iinf.segsize = Job_seg.iinf.reqsize = sizeof(struct jshm_hdr) + 2 * SHM_JHASHMOD * sizeof(LONG);
#if     defined(USING_MMAP) || !defined(NO_ROUND_PAGES)
        Job_seg.iinf.segsize = (Job_seg.iinf.segsize + pagesize - 1) & ~(pagesize-1);
#endif

        /* Allocate job info segment, containing number of jobs and hash tables.
           This has been split off from the job data to try to avoid funny problems
           on machines with suspicious shared memory code. */

#ifdef  USING_MMAP
        if  ((Job_seg.iinf.mmfd = open(JIMMAP_FILE, O_CREAT|O_RDWR|O_EXCL, IPC_MODE)) < 0)
                report($E{Cannot create jinfo shm});
#ifdef  RUN_AS_ROOT
#ifdef  HAVE_FCHOWN
        if  (Daemuid)
                fchown(Job_seg.iinf.mmfd, Daemuid, getgid());
#else
        if  (Daemuid)
                chown(JIMMAP_FILE, Daemuid, getgid());
#endif
#endif
        fcntl(Job_seg.iinf.mmfd, F_SETFD, 1);
        jobs_lock();            /* If we are using mmap we use that fd so it's OK */

        /* Write a byte to the last byte of the file */
        lseek(Job_seg.iinf.mmfd, (long) (Job_seg.iinf.segsize - 1), 0);
        if  (write(Job_seg.iinf.mmfd, &byte, 1) != 1)
                report($E{Cannot create jinfo shm});
        if  ((Job_seg.iinf.seg = mmap(0, Job_seg.iinf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, Job_seg.iinf.mmfd, 0)) == MAP_FAILED)
                report($E{Cannot attach jinfo shm});

#else  /* Not using mmap */

#ifdef  USING_FLOCK
        if  ((Job_seg.iinf.lockfd = open(JLOCK_FILE, O_CREAT|O_RDWR|O_TRUNC, IPC_MODE)) < 0)
                report($E{Cannot create jinfo shm});
#ifdef  HAVE_FCHOWN
        if  (Daemuid)
                Ignored_error = fchown(Job_seg.iinf.lockfd, Daemuid, getgid());
#else
        if  (Daemuid)
                Ignored_error = chown(JLOCK_FILE, Daemuid, getgid());
#endif
        fcntl(Job_seg.iinf.lockfd, F_SETFD, 1);
        jobs_lock();
#endif /* USING_FLOCK */

        Job_seg.iinf.base = JSHMID;
        Job_seg.dinf.base = SHMID + JSHMOFF; /* Better include this here for later */
        if  ((Job_seg.iinf.chan = shmget(JSHMID, Job_seg.iinf.segsize, IPC_MODE|IPC_CREAT|IPC_EXCL)) < 0)
                report($E{Cannot create jinfo shm});
        if  ((Job_seg.iinf.seg = shmat(Job_seg.iinf.chan, (char *) 0, 0)) == (char *) -1)
                report($E{Cannot attach jinfo shm});
#endif /* SHM -v- MMAP */
        Job_seg.dptr = (struct jshm_hdr *) Job_seg.iinf.seg;
        Job_seg.hashp_jno = (LONG *) (Job_seg.iinf.seg + sizeof(struct jshm_hdr));
        Job_seg.hashp_jid = (LONG *) ((char *) Job_seg.hashp_jno + SHM_JHASHMOD * sizeof(LONG));

        /* Repeat allocation for job data segment */

        Job_seg.dinf.reqsize = Maxjobs * sizeof(Hashspq);
        if  (!gshmchan(&Job_seg.dinf, JSHMOFF))
                report($E{Create jshm error});

        /* If we got more than we asked for, adjust Maxjobs accordingly. */

        Maxjobs = Job_seg.dinf.segsize / sizeof(Hashspq);
        Job_seg.jlist = (Hashspq *) Job_seg.dinf.seg;

        /* Initialise hash tables,
           Initialise pointers to head and tail of queue and free chain. */

        for  (nxtj = 0;  nxtj < SHM_JHASHMOD;  nxtj++)
                Job_seg.hashp_jno[nxtj] = Job_seg.hashp_jid[nxtj] = SHM_HASHEND;

        Job_seg.dptr->js_q_head = Job_seg.dptr->js_q_tail = Job_seg.dptr->js_freech = SHM_HASHEND;

        Job_seg.Njobs = 0;
        hjj = &Job_seg.jlist[0];
        jj = &hjj->j;

        /* Read in the job queue file. */

        while  (read(jfilefd, (char *) jj, sizeof(struct spq)) == sizeof(struct spq))  {
                if  (jj->spq_job == 0)
                        continue;

                /* Clear in case we stopped in a hurry. */

                jj->spq_dflags &= ~SPQ_PQ;
                jj->spq_sflags = 0;             /* Turns off SPQ_WARNED bit also */
                jj->spq_pslot = -1;
                jj->spq_netid = 0L;

                /* Now insert into hash tables for job number and jid, and put on Q */

                jj->spq_rslot = Job_seg.Njobs;
                hjj->prv_jno_hash = hjj->prv_jid_hash = hjj->q_nxt = SHM_HASHEND;

                /* Put on job number hash chain */
                hashval = jno_jhash(jj->spq_job);
                prevind = hjj->nxt_jno_hash = Job_seg.hashp_jno[hashval];
                Job_seg.hashp_jno[hashval] = Job_seg.Njobs;
                if  (prevind >= 0L)
                        Job_seg.jlist[prevind].prv_jno_hash = Job_seg.Njobs;

                /* Put on job ident hash chain */
                hashval = jid_hash(0L, Job_seg.Njobs);
                prevind = hjj->nxt_jid_hash = Job_seg.hashp_jid[hashval];
                Job_seg.hashp_jid[hashval] = Job_seg.Njobs;
                if  (prevind >= 0L)
                        Job_seg.jlist[prevind].prv_jid_hash = Job_seg.Njobs;

                /* Put on queue */
                nxtj = hjj->q_prv = Job_seg.dptr->js_q_tail;
                Job_seg.dptr->js_q_tail = Job_seg.Njobs;
                if  (nxtj < 0L)
                        Job_seg.dptr->js_q_head = Job_seg.Njobs;
                else
                        Job_seg.jlist[nxtj].q_nxt = Job_seg.Njobs;

                Job_seg.Njobs++;
                hjj++;
                jj = &hjj->j;
        }

        free_rest((LONG) Job_seg.Njobs, (LONG) Maxjobs);

        /* Set up header structure. */

#ifdef  USING_MMAP
        Job_seg.dptr->js_did = Job_seg.dinf.segsize;
#else
        Job_seg.dptr->js_did = Job_seg.dinf.base;
#endif
        Job_seg.dptr->js_psegid = 0;
        Job_seg.dptr->js_info = 0;
        Job_seg.dptr->js_njobs = Job_seg.Njobs;
        Job_seg.dptr->js_maxjobs = Maxjobs;

        /* The following statement OR the one like it in attach_hosts() won't do
           anything useful depending upon which is called first see main().
           Put in both places so that we can shuffle the call order to get the best effect. */

        Job_seg.dptr->js_viewport = vportnum;
        Job_seg.dptr->js_lastwrite = sbuf.st_mtime;
        Job_seg.dptr->js_serial = 1;
        jobs_unlock();
}

/*      Rewrite job file.
        If the number of jobs has shrunk since the last time,
        squash it up (by recreating the file, as vanilla UNIX doesn't have a
        "truncate file" syscall [why not?]). */

void  rewrjq()
{
#ifndef  HAVE_FTRUNCATE
        int     fjobs;
#endif
        int     nwritten;
        LONG    jind;
        struct  flock   wlock;

#ifdef   HAVE_FTRUNCATE
        lseek(jfilefd, 0L, 2);
#else
        fjobs = lseek(jfilefd, 0L, 2) / sizeof(struct spq);
#endif
        nwritten = Job_seg.dptr->js_njobs;

        /* Cream out non-local jobs  */

        for  (jind = Job_seg.dptr->js_q_head;  jind >= 0L;  jind = Job_seg.jlist[jind].q_nxt)
                if  (Job_seg.jlist[jind].j.spq_netid)
                        nwritten--;

        wlock.l_type = F_WRLCK;
        wlock.l_whence = 0;
        wlock.l_start = 0L;
        wlock.l_len = 0L;

#ifdef  HAVE_FTRUNCATE
        while  (fcntl(jfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        report($E{Panic couldnt lock job file});
        }
        Ignored_error = ftruncate(jfilefd, 0L);
        lseek(jfilefd, 0L, 0);
#else
        if  (nwritten < fjobs)  {
                close(jfilefd);
                unlink(jfile);
                if  ((jfilefd = open(jfile, O_RDWR|O_CREAT, CMODE)) < 0)
                        report($E{Cannot create jfile});
#ifdef  RUN_AS_ROOT
                if  (Daemuid)
#if     defined(HAVE_FCHOWN) && !defined(M88000)
                        fchown(jfilefd, Daemuid, getegid());
#else
                        chown(jfile, Daemuid, getegid());
#endif
#endif
        }
        else
                lseek(jfilefd, 0L, 0);
        /* Note that there is a race if there's no ftruncate */
        while  (fcntl(jfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        report($E{Panic couldnt lock job file});
        }
#endif /* HAVE_FTRUNCATE */

        jind = Job_seg.dptr->js_q_head;
        while  (jind >= 0L)  {
                Hashspq *jp = &Job_seg.jlist[jind];
                if  (jp->j.spq_netid == 0)
                        Ignored_error = write(jfilefd, (char *) &jp->j, sizeof(struct spq));
                jind = jp->q_nxt;
        }

        time(&Job_seg.dptr->js_lastwrite);

        wlock.l_type = F_UNLCK;
        while  (fcntl(jfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        report($E{Panic couldnt lock job file});
        }
}

/* Reallocate job shared segment if it grows too big.  We do this by
   allocating a new shared memory segment, putting the key field in the job info
   segment - variation from before */

void  growjseg()
{
        unsigned        Oldmaxj = Job_seg.dptr->js_maxjobs;
        unsigned        Maxjobs = Oldmaxj + INCNJOBS;
        Hashspq         *newlist;
        LONG            pind;
#ifdef  USING_MMAP
        void            *newseg;
#else
        struct  spshm_info      new_info;
#endif
        struct  spr_req rq;

        if  ((Job_seg.Njobs = Job_seg.dptr->js_njobs) > Oldmaxj)  {
                disp_arg[0] = Job_seg.Njobs;
                disp_arg[1] = Oldmaxj;
                report($E{Job information segment corrupted});
        }

#ifdef  USING_MMAP

        Job_seg.dinf.reqsize = Maxjobs * sizeof(Hashspq);
        gshmchan(&Job_seg.dinf, JSHMOFF); /* Panics if it can't grow */
        newseg = Job_seg.dinf.seg;
        Maxjobs = Job_seg.dinf.segsize / sizeof(Hashspq);

        /* We still have the same physical file so we don't need to copy.
           However it might have wound up at a different virt address */

        newlist = (Hashspq *) newseg;
        Job_seg.dptr->js_did = Job_seg.dinf.segsize;
#else

        new_info = Job_seg.dinf;
        new_info.base += SHMINC;
        new_info.reqsize = Maxjobs * sizeof(Hashspq);
        if  (!gshmchan(&new_info, JSHMOFF))
                report($E{Create jshm error});

        /* Copy over stuff from old segment. */
        newlist = (Hashspq *) new_info.seg;
        BLOCK_COPY(newlist, Job_seg.jlist, sizeof(Hashspq) * Oldmaxj);

        /* Detach and remove old segment. */
        shmdt(Job_seg.dinf.seg);
        shmctl(Job_seg.dinf.chan, IPC_RMID, (struct shmid_ds *) 0);
        Job_seg.dptr->js_did = Job_seg.dinf.base;
#endif
        Job_seg.dptr->js_maxjobs = Maxjobs;
        Job_seg.jlist = newlist;
        free_rest((LONG) Oldmaxj, (LONG) Maxjobs);

        /* Give (my) printers a friendly poke.
           Network processes will notice the moved data segment
           (At least I hope they will) */

        rq.spr_mtype = MT_SCHED;
        rq.spr_un.o.spr_act = SP_REMAP;

        pind = Ptr_seg.dptr->ps_l_head;
        while  (pind >= 0L)  {
                Hashspptr  *pp = &Ptr_seg.plist[pind];
                if  (pp->p.spp_state >= SPP_PROC  &&  pp->p.spp_netid == 0)
                        pmsend(&pp->p, &rq, sizeof(struct sp_omsg));
                pind = pp->l_nxt;
        }
        if  (Netm_pid > 0)
                kill(Netm_pid, QRFRESH);
}

static void  takeoff_q(const unsigned jind)
{
        LONG  nxtind = Job_seg.jlist[jind].q_nxt;
        LONG  prvind = Job_seg.jlist[jind].q_prv;

        if  (nxtind < 0L)
                Job_seg.dptr->js_q_tail = prvind;
        else
                Job_seg.jlist[nxtind].q_prv = prvind;
        if  (prvind < 0L)
                Job_seg.dptr->js_q_head = nxtind;
        else
                Job_seg.jlist[prvind].q_nxt = nxtind;
}

static void  dequeue_nolock(const LONG jind)
{
        Hashspq *jhp = &Job_seg.jlist[jind];
        LONG    prevind, nextind;

        /* Delete from job number chain. */

        prevind = jhp->prv_jno_hash;
        nextind = jhp->nxt_jno_hash;
        if  (prevind < 0L)
                Job_seg.hashp_jno[jno_jhash(jhp->j.spq_job)] = nextind;
        else
                Job_seg.jlist[prevind].nxt_jno_hash = nextind;

        if  (nextind >= 0L)
                Job_seg.jlist[nextind].prv_jno_hash = prevind;

        /* Delete from job ident chain. */

        prevind = jhp->prv_jid_hash;
        nextind = jhp->nxt_jid_hash;

        if  (prevind < 0L)
                Job_seg.hashp_jid[jid_hash(jhp->j.spq_netid, jhp->j.spq_rslot)] = nextind;
        else
                Job_seg.jlist[prevind].nxt_jid_hash = nextind;

        if  (nextind >= 0L)
                Job_seg.jlist[nextind].prv_jid_hash = prevind;

        /* Delete from queue and add to free chain. */

        jhp->j.spq_job = 0;
        jhp->j.spq_netid = 0;
        jhp->j.spq_pslot = -1;
        takeoff_q(jind);

        jhp->q_prv = SHM_HASHEND;
        jhp->q_nxt = Job_seg.dptr->js_freech;
        Job_seg.dptr->js_freech = jind;
        Job_seg.dptr->js_njobs--;
        Job_seg.dptr->js_serial++;
        Job_seg.Last_ser++;
        qchanges++;
}

static void  puton_q(const unsigned jind, const LONG after)
{
        LONG    nxt;

        Job_seg.jlist[jind].q_prv = after;

        if  (after < 0L)  {
                /* At beginning of q. */
                nxt = Job_seg.dptr->js_q_head;
                Job_seg.dptr->js_q_head = (LONG) jind;
        }
        else  {
                nxt = Job_seg.jlist[after].q_nxt;
                Job_seg.jlist[after].q_nxt = (LONG) jind;
        }

        Job_seg.jlist[jind].q_nxt = nxt;
        if  (nxt < 0L)
                Job_seg.dptr->js_q_tail = (LONG) jind;
        else
                Job_seg.jlist[nxt].q_prv = (LONG) jind;
}

/* Remove records of remote jobs when a remote machine kicks the bucket.
   This MUST be called before net_pclear as we "unassign" jobs which were being printed. */

void  net_jclear(const netid_t netid)
{
        LONG    jind;

        jobs_lock();
        jind = Job_seg.dptr->js_q_head;
        while  (jind >= 0L)  {
                Hashspq  *hjp = &Job_seg.jlist[jind];
                struct  spq  *jp = &hjp->j;
                LONG  nxtind = hjp->q_nxt;              /* Might get mangled by takeoff_q */

                if  (jp->spq_netid == netid)
                        dequeue_nolock(jind);
                else  if  (jp->spq_netid == 0)  {
                        if  (jp->spq_sflags & SPQ_PROPOSED  &&  jp->spq_pslot == netid)  {
                                jp->spq_sflags &= SPQ_WARNED; /* Didn't mean ~ there */
                                jp->spq_pslot = -1;
                        }
                        else  if  (jp->spq_pslot >= 0  &&  jp->spq_pslot < Ptr_seg.dptr->ps_maxptrs)  {
                                /* See if we think that the dying machine was printing it. */
                                struct spptr *pp = &Ptr_seg.plist[jp->spq_pslot].p;
                                if  (pp->spp_state != SPP_NULL  &&  pp->spp_netid == netid)  {
                                        jp->spq_sflags &= SPQ_WARNED; /* No ~ meant there either */
                                        jp->spq_dflags = 0;
                                        jp->spq_pslot = -1;
                                }
                        }
                }
                jind = nxtind;
        }
        jobs_unlock();
}

/* Get printer corresponding to job being printed */

struct  spptr *printing(struct spq *jp)
{
        struct  spptr   *cp;

        if  (jp->spq_pslot < 0  ||  jp->spq_pslot >= Ptr_seg.dptr->ps_maxptrs)
                return  (struct spptr *) 0;
        cp = &Ptr_seg.plist[jp->spq_pslot].p;
        if  (cp->spp_state == SPP_NULL)
                return  (struct spptr *) 0;
        return  cp;
}

/* Find local slot corresponding to given remote slot */

slotno_t  find_rslot(const netid_t netid, const slotno_t rslot)
{
        LONG  jind = Job_seg.hashp_jid[jid_hash(netid, rslot)];

        while  (jind >= 0L)  {
                Hashspq *hjp = &Job_seg.jlist[jind];
                if  (hjp->j.spq_netid == netid  &&  hjp->j.spq_rslot == rslot)
                        return  jind;
                jind = hjp->nxt_jid_hash;
        }
        disp_str = look_host(netid);
        disp_arg[1] = rslot;
        nfreport($E{Lost track of job});
        return  -1;
}

/* Enqueue request on spool queue.
   Non-local jobs have non-zero rq->spr_netid */

void  enqueue(struct sp_xjmsg *rq, struct spq *inj)
{
        unsigned        hashval;
        LONG            jind, nxtind, lastind;
        int             hadl = 0;
        Hashspq         *jhp;
        struct  spq     *dest;
        float           fwpri;

        jobs_lock();

        /* Grow job segment if needed. */

        if  (Job_seg.dptr->js_njobs >= Job_seg.dptr->js_maxjobs)
                growjseg();

        /* Take it off the free chain. */

        jind = Job_seg.dptr->js_freech;
        jhp = &Job_seg.jlist[jind];
        dest = &jhp->j;
        Job_seg.dptr->js_freech = jhp->q_nxt;

        /* Stuff it in and insert default stuff. */

        *dest = *inj;
        dest->spq_sflags = 0;
        dest->spq_dflags &= SPQ_PRINTED|SPQ_PAGEFILE;

        /*      Force on local only bit if skipping networking. */

        if  (!Network_ok)
                dest->spq_jflags |= SPQ_LOCALONLY;

        /* Set spq_rslot to self on local jobs for benefit of hash tables */
        dest->spq_pslot = -1;
        if  ((dest->spq_netid = rq->spr_netid) == 0)
                dest->spq_rslot = jind;

        /* Stuff it into the hash chains.  */

        hashval = jno_jhash(dest->spq_job);
        nxtind = Job_seg.hashp_jno[hashval];
        jhp->nxt_jno_hash = nxtind;
        jhp->prv_jno_hash = SHM_HASHEND;
        if  (nxtind >= 0L)
                Job_seg.jlist[nxtind].prv_jno_hash = jind;
        Job_seg.hashp_jno[hashval] = jind;

        hashval = jid_hash(dest->spq_netid, dest->spq_rslot);
        nxtind = Job_seg.hashp_jid[hashval];
        jhp->nxt_jid_hash = nxtind;
        jhp->prv_jid_hash = SHM_HASHEND;
        if  (nxtind >= 0L)
                Job_seg.jlist[nxtind].prv_jid_hash = jind;
        Job_seg.hashp_jid[hashval] = jind;

        fwpri = (float) dest->spq_pri;
        lastind = Job_seg.dptr->js_q_tail;

        if  (dest->spq_netid)  {

                /* For non-local jobs take job submission time into account...
                   This is not terribly accurate... */

                while  (lastind >= 0L)  {
                        Hashspq *lhp = &Job_seg.jlist[lastind];
                        if  ((float) lhp->j.spq_pri >= fwpri)
                                break;
                        if  (lhp->j.spq_time < dest->spq_time)
                                fwpri -= pri_decrement;
                        lastind = lhp->q_prv;
                        hadl++;
                }
        }
        else  {
                while  (lastind >= 0L)  {
                        Hashspq *lhp = &Job_seg.jlist[lastind];
                        if  ((float) lhp->j.spq_pri >= fwpri)
                                break;
                        fwpri -= pri_decrement;
                        lastind = lhp->q_prv;
                        hadl++;
                }
        }

        /* Insert current job.  */

        dest->spq_wpri = round_short(fwpri);
        puton_q(jind, lastind);
        Job_seg.dptr->js_njobs++;
        Job_seg.dptr->js_serial++;
        Job_seg.Last_ser++;
        qchanges++;
        if  (dest->spq_netid == 0  &&  (dest->spq_jflags & SPQ_LOCALONLY) == 0)
                job_broadcast(jhp, SJ_ENQ);
        jobs_unlock();
}

/* Copy "minor" details of jobs which don't cause too much difficulty
   if the job is already printing. */

static  void  copy_minor(struct spq *dest, struct spq *newj)
{
        dest->spq_cps = newj->spq_cps;
        dest->spq_jflags &= ~(SPQ_WRT|SPQ_MAIL|SPQ_RETN|SPQ_MATTN|SPQ_WATTN);
        dest->spq_jflags |= newj->spq_jflags & (SPQ_WRT|SPQ_MAIL|SPQ_RETN|SPQ_MATTN|SPQ_WATTN);
        dest->spq_nptimeout = newj->spq_nptimeout;
        dest->spq_ptimeout = newj->spq_ptimeout;
}

/* Copy more substantial bits of jobs we want to change */

static  void  copy_major(const LONG jind, struct spq *dest, struct spq *newj)
{
        int     pdiff;

        strncpy(dest->spq_file, newj->spq_file, sizeof(dest->spq_file));
        strncpy(dest->spq_form, newj->spq_form, sizeof(dest->spq_form));
        strncpy(dest->spq_ptr, newj->spq_ptr, sizeof(dest->spq_ptr));
        strncpy(dest->spq_flags, newj->spq_flags, sizeof(dest->spq_flags));
        strncpy(dest->spq_puname, newj->spq_puname, UIDSIZE);

        dest->spq_hold = newj->spq_hold;
        dest->spq_jflags &= ~(SPQ_NOH|SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);
        dest->spq_jflags |= newj->spq_jflags & (SPQ_NOH|SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);

        dest->spq_dflags &= ~SPQ_PRINTED;
        dest->spq_dflags |= newj->spq_dflags & SPQ_PRINTED;
        dest->spq_start = newj->spq_start;
        dest->spq_end = newj->spq_end;
        dest->spq_haltat = newj->spq_haltat;
        dest->spq_npages = newj->spq_npages;
        dest->spq_class = newj->spq_class;

        /* Now worry about priority.  Set pdiff to increase in priority. */

        if  ((pdiff = (int) newj->spq_pri - (int) dest->spq_pri) != 0)  {
                float   fwpri = (float) (pdiff + dest->spq_wpri);

                dest->spq_pri = newj->spq_pri;

                if  (pdiff > 0)  {      /*  Going up ....  */
                        LONG  pind = Job_seg.jlist[jind].q_prv;
                        while  (pind >= 0L)  {
                                if  ((float) Job_seg.jlist[pind].j.spq_pri >= fwpri)
                                        break;
                                fwpri -= pri_decrement;
                                pind = Job_seg.jlist[pind].q_prv;
                        }
                        takeoff_q(jind);
                        puton_q(jind, pind);
                }
                else  {         /* Going down ...... */
                        LONG  cind = jind, nind;
                        while  ((nind = Job_seg.jlist[cind].q_nxt) >= 0L)  {
                                if  ((float) Job_seg.jlist[nind].j.spq_pri <= fwpri)
                                        break;
                                fwpri += pri_decrement;
                                cind = nind;
                        }
                        if  (cind != jind)  {
                                takeoff_q(jind);
                                puton_q(jind, cind);
                        }
                }
                dest->spq_wpri = round_short(fwpri);
        }
}

/* Change details of job in queue.
   This is the case where we have various details to change, possibly
   passing on some info elsewhere. */

void  chjob(struct sp_xjmsg *rq, struct spq *newj)
{
        Hashspq         *hjp;
        struct  spq     *dest;

        if  (rq->spr_jslot >= Job_seg.dptr->js_maxjobs)
                return;
        hjp = &Job_seg.jlist[rq->spr_jslot];
        dest = &hjp->j;

        /* If it belongs to some other machine, then pass on
           details and let it worry about it. */

        if  (dest->spq_netid)  {
                job_sendupdate(dest, newj, SJ_CHNG);
                return;
        }

        copy_minor(dest, newj); /* Things that don't cause chaos */
        Job_seg.Last_ser++;
        qchanges++;
        Job_seg.dptr->js_serial++;

        if  (printing(dest))  {
                if  (!(dest->spq_jflags & SPQ_LOCALONLY))
                        job_broadcast(hjp, SJ_CHANGEDJOB);
                return;
        }

        jobs_lock();
        copy_major(rq->spr_jslot, dest, newj);

        /* Force on local only bit if skipping networking. */

        if  (!Network_ok)
                dest->spq_jflags |= SPQ_LOCALONLY;

        /* Now if the job is to have the local only bit toggled, then
           it should appear or disappear on remote machines. */

        if  ((dest->spq_jflags & SPQ_LOCALONLY) != (newj->spq_jflags & SPQ_LOCALONLY))  {
                if  (dest->spq_jflags & SPQ_LOCALONLY)  {
                        dest->spq_jflags &= ~SPQ_LOCALONLY;
                        job_broadcast(hjp, SJ_ENQ);
                }
                else  {
                        dest->spq_jflags |= SPQ_LOCALONLY;
                        job_broadcast(hjp, SO_DEQUEUED);
                }
        }
        else  if  (!(dest->spq_jflags & SPQ_LOCALONLY))
                job_broadcast(hjp, SJ_CHANGEDJOB);

        jobs_unlock();
}

/* Note that a remote job has been changed. */

void  remchjob(struct sp_xjmsg *rq, struct spq *newj)
{
        struct  spq     *dest;
        slotno_t        slot;

        if  ((slot = find_rslot(rq->spr_netid, rq->spr_jslot)) < 0)
                return;
        dest = &Job_seg.jlist[slot].j;
        copy_minor(dest, newj);
        Job_seg.Last_ser++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        if  (printing(dest))
                return;
        jobs_lock();
        copy_major(slot, dest, newj);
        jobs_unlock();
}

/* Update remote details of job when the job on the remote machine is
   printed by a local-only printer on the (same) remote machine. */

void  locpassign(struct sp_omsg *rq)
{
        struct  spq  *dest;
        slotno_t        slot;

        if  ((slot = find_rslot(rq->spr_netid, rq->spr_jpslot)) < 0)
                return;
        dest = &Job_seg.jlist[slot].j;
        dest->spq_sflags |= SPQ_ASSIGN;
        dest->spq_pslot = -1;   /* Belt & braces */
        Job_seg.dptr->js_serial++;
        /* Won't bother with Job_seg.Last_ser++ etc as it has a negligible effect
           in this instance */
}

/* Note assignment of job to printer
   It might be one of "my" jobs. However it will always be
   someone else's printer. */

void  jpassign(struct sp_omsg *rq)
{
        struct  spq     *jp;
        struct  spptr   *pp;
        slotno_t        slot = rq->spr_arg1;

        /* If the job is non-local, then it will have the network id in spr_arg2 */

        if  (rq->spr_arg2)  {
                slot = find_rslot((netid_t) rq->spr_arg2, slot);
                if  (slot < 0  ||  rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs)
                        return;
        }
        else  if  (rq->spr_jpslot >= Ptr_seg.dptr->ps_maxptrs  ||  rq->spr_arg1 >= Job_seg.dptr->js_maxjobs)
                return;

        pp = &Ptr_seg.plist[rq->spr_jpslot].p;
        jp = &Job_seg.jlist[slot].j;

        /* Check parameters are what we expect. */

        if  (pp->spp_state == SPP_NULL  ||  pp->spp_netid == 0  ||  jp->spq_job != rq->spr_jobno)
                return;

        /* Ok I think we're ready.... */

        jp->spq_sflags = SPQ_ASSIGN;
        jp->spq_dflags |= SPQ_PQ;
        jp->spq_pslot = rq->spr_jpslot;
        pp->spp_jslot = slot;
        pp->spp_job = jp->spq_job;
        pp->spp_rjhostid = jp->spq_netid;
        pp->spp_rjslot = jp->spq_rslot;
        pp->spp_state = SPP_RUN;
        Job_seg.Last_ser++;
        Ptr_seg.Last_ser++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        Ptr_seg.dptr->ps_serial++;
}

/* When printer finishes with job, then we break the links.
 This routine is only called from the machine the printer is on. */

void  unassign(Hashspq *hjp, Hashspptr *hcp)
{
        struct  spq     *jp = &hjp->j;
        struct  spptr   *cp = &hcp->p;

        /* First break the printer list and broadcast details
           to the waiting world */

        cp->spp_rjslot = cp->spp_jslot = -1;
        cp->spp_job = 0;
        cp->spp_rjhostid = 0;
        if  (!(cp->spp_netflags & SPP_LOCALONLY))
                ptr_broadcast(hcp, SP_PUNASSIGNED);

        /* If it was a remote job, then call the following routine
           on the other machine. */

        if  (jp->spq_netid)
                job_sendupdate(jp, jp, SJ_JUNASSIGN);
        else  {
                jp->spq_pslot = -1;
                jp->spq_sflags &= ~SPQ_ASSIGN;
                if  (!(jp->spq_jflags & SPQ_LOCALONLY))
                        job_broadcast(hjp, SJ_JUNASSIGNED);
        }
}

/* Routine called when a job on this machine stops being printed
   by a remote machine - the copies may get reset. */

void  unassign_job(struct sp_xjmsg *rq, struct spq *newj)
{
        Hashspq         *hjp;
        struct  spq     *dest;

        if  (rq->spr_jslot >= Job_seg.dptr->js_maxjobs)
                return;
        hjp = &Job_seg.jlist[rq->spr_jslot];
        dest = &hjp->j;
        dest->spq_cps = newj->spq_cps;
        dest->spq_jflags = newj->spq_jflags;
        dest->spq_dflags = newj->spq_dflags;
        dest->spq_sflags &= SPQ_WARNED; /* Didn't mean ~ there */
        dest->spq_haltat = newj->spq_haltat;
        dest->spq_pslot = -1;
        if  (dest->spq_cps == 0)
                dest->spq_time = time((time_t *) 0); /* Timeouts on my time */
        job_broadcast(hjp, SJ_JUNASSIGNED);
        Job_seg.Last_ser++;
        qchanges++;
        Job_seg.dptr->js_serial++;
}

/* Note that a remote job has been unassigned.
   Called from "job_broadcast" in previous routine. */

void  unassign_remjob(struct sp_xjmsg *rq, struct spq *newj)
{
        struct  spq     *dest;
        slotno_t  slot;

        if  ((slot = find_rslot(rq->spr_netid, rq->spr_jslot)) < 0)
                return;

        dest = &Job_seg.jlist[slot].j;
        dest->spq_cps = newj->spq_cps;
        dest->spq_jflags = newj->spq_jflags;
        dest->spq_dflags = newj->spq_dflags;
        dest->spq_sflags &= SPQ_WARNED; /* Didn't mean ~ there */
        dest->spq_haltat = newj->spq_haltat;
        dest->spq_pslot = -1;
        Job_seg.Last_ser++;
        qchanges++;
        Job_seg.dptr->js_serial++;
}

/* Remove job from queue.  Remove files if relevant. */

void  dequeue(const LONG jind)
{
        struct  spq     *jp = &Job_seg.jlist[jind].j;

        jobs_lock();
        if  (jp->spq_netid == 0)  {
                unlink(mkspid(SPNAM, jp->spq_job));
                unlink(mkspid(PFNAM, jp->spq_job));
        }
        dequeue_nolock(jind);
        jobs_unlock();
}

/* Delete a job by whatever means */

void  delete_job(Hashspq *hjp)
{
        if  (hjp->j.spq_netid)
                job_message(hjp->j.spq_netid, &hjp->j, SO_ABNN, 0L, 0L);
        else  {
                if  (!(hjp->j.spq_jflags & SPQ_LOCALONLY))
                        job_broadcast(hjp, SO_DEQUEUED);
                dequeue(hjp - Job_seg.jlist);
        }
}

/* Abort job, removing from queue. */

void  doabort(struct sp_omsg *rq)
{
        slotno_t  slotn = rq->spr_jpslot;
        Hashspq  *hjp;
        struct  spq     *jp;

        if  (slotn >= Job_seg.dptr->js_maxjobs)
                return;
        hjp = &Job_seg.jlist[slotn];
        jp = &hjp->j;

        if  (jp->spq_job == 0L) /* Already deleted (user sitting on A key in spq) */
                return;

        if  (printing(jp))  {
                rq->spr_jpslot = jp->spq_pslot;
                prjab(rq);
        }
        else  {
                if  (rq->spr_act != SO_ABNN)
                        notify(jp, (struct spptr *) 0, $E{Job deleted msg}, (jobno_t) 0, PAST_TENSE);
                delete_job(hjp);
        }
}

/* Note removal of non-local job */

void  remdequeue(struct sp_omsg *rq)
{
        slotno_t        slot;
        struct  spq     *jp;
        struct  spptr   *cp;

        if  ((slot = find_rslot(rq->spr_netid, rq->spr_jpslot)) < 0)
                return;
        jp = &Job_seg.jlist[slot].j;

        if  ((cp = printing(jp))  &&  cp->spp_netid == 0)  {
                rq->spr_jpslot = jp->spq_pslot;
                prjab(rq);
        }
        dequeue(slot);
}

/* Remove dead wood from job queue. */

#define HOURS   3600L

unsigned  qpurge()
{
        time_t  now = time((time_t *) 0);
        unsigned  at = 0;
        LONG  jind = Job_seg.dptr->js_q_head;

        while  (jind >= 0L)  {

                Hashspq *hjp = &Job_seg.jlist[jind];
                struct  spq  *jp = &hjp->j;
                time_t  twhen;
                LONG  cind = jind;

                jind = hjp->q_nxt;

                /* Forget other machine's jobs. The other machine can worry about them.  */

                if  (jp->spq_netid)
                        continue;

                twhen = (jp->spq_dflags & SPQ_PRINTED? jp->spq_ptimeout: jp->spq_nptimeout) * HOURS + (time_t) jp->spq_time;

                if  (twhen <= now)  {
                        if  (!printing(jp))  {

                                /* Insist on telling someone.  NB local jobs only - remember?  */

                                if  (!(jp->spq_jflags & (SPQ_MAIL|SPQ_WRT)))
                                        jp->spq_jflags |= SPQ_MAIL;
                                notify(jp, (struct spptr *) 0, $E{Job autodel msg}, (jobno_t) 0, PAST_TENSE);
                                if  (!(jp->spq_jflags & SPQ_LOCALONLY))
                                        job_broadcast(hjp, SO_DEQUEUED);
                                dequeue(cind);
                                continue;
                        }
                }
                else  if  (at == 0 || at > twhen - now)
                        at = twhen - now;
        }
        return  at;
}
