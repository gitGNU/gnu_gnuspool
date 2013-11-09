/* xfershm.c -- manage shared memory used for buffering messages

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
#include <sys/ipc.h>
#include <sys/msg.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#include <errno.h>
#include <stdio.h>
#include "defaults.h"
#include "ipcstuff.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "incl_unix.h"
#include "ecodes.h"
#include "errnums.h"
#include "files.h"

/* Note that we define this here and not in any of the main programs any more
   to minimise unresolved externals in the shared libraries */

struct  xfershm *Xfer_shmp;

#ifdef  USING_MMAP
int     Xfermmfd;
#define SHMLOCK_FD      Xfermmfd
#else  /* Using shm */
#ifdef  USING_FLOCK
int     Xfershm_lockfd;
#define SHMLOCK_FD      Xfershm_lockfd
#endif /* USING_FLOCK */
#endif /* !USING_MMAP */

#ifdef  USING_FLOCK

void  lockxbuf()
{
        struct  flock   lck;
        lck.l_type = F_WRLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        for  (;;)  {
                if  (fcntl(SHMLOCK_FD, F_SETLKW, &lck) >= 0)
                        return;
                if  (errno != EINTR)  {
                        print_error($E{Lock error});
                        exit(E_NOMEM);
                }
        }
}

void  unlockxbuf()
{
        struct  flock   lck;
        lck.l_type = F_UNLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        for  (;;)  {
                if  (fcntl(SHMLOCK_FD, F_SETLKW, &lck) >= 0)
                        return;
                if  (errno != EINTR)  {
                        print_error($E{Unlock error});
                        exit(E_NOMEM);
                }
        }
}

#else

/* Likewise define this here */

int     Sem_chan = -1;

static  struct  sembuf  xlw = { XT_LOCK, -1, SEM_UNDO },
                        xulw ={ XT_LOCK, 1, SEM_UNDO };

void  lockxbuf()
{
        for  (;;)  {
                if  (semop(Sem_chan, &xlw, 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_SETUP);
        }
}

void  unlockxbuf()
{
        for  (;;)  {
                if  (semop(Sem_chan, &xulw, 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_SETUP);
        }
}

/* Turn off SEM_UNDO for server processes */

void  set_xfer_server()
{
        xlw.sem_flg = xulw.sem_flg = 0;
}
#endif

int  init_xfershm(const int insdir)
{
        char    *xret;

#ifdef  USING_MMAP
        if  (insdir)
                Xfermmfd = open(XFMMAP_FILE, O_RDWR);
        else  {
                char  *fname = mkspdirfile(XFMMAP_FILE);
                Xfermmfd = open(fname, O_RDWR);
                free(fname);
        }
        if  (Xfermmfd < 0)
                return  $E{Cannot open buffer shm};

        fcntl(Xfermmfd, F_SETFD, 1);
        if  ((xret = mmap(0, lseek(Xfermmfd, 0L, 2), PROT_READ|PROT_WRITE, MAP_SHARED, Xfermmfd, 0)) == MAP_FAILED)
                return  $E{Cannot attach buffer shm};
#else
        int     xfer_chan;
#ifdef  USING_FLOCK
        if  (insdir)
                Xfershm_lockfd = open(XLOCK_FILE, O_RDWR);
        else  {
                char  *fname = mkspdirfile(XLOCK_FILE);
                Xfershm_lockfd = open(fname, O_RDWR);
                free(fname);
        }
        if  (Xfershm_lockfd < 0)
                return  $E{Cannot open buffer shm};
        fcntl(Xfershm_lockfd, F_SETFD, 1);
#endif
        if  ((xfer_chan = shmget((key_t) XSHMID, 0, 0)) < 0)
                return  $E{Cannot open buffer shm};

        if  ((xret = shmat(xfer_chan, (char *) 0, 0)) == (char *) -1)
                return  $E{Cannot attach buffer shm};

#endif
        Xfer_shmp = (struct xfershm *) xret;
        return  0;
}

static struct joborptr *getptr()
{
        struct  joborptr        *result = &Xfer_shmp->xf_queue[Xfer_shmp->xf_tail];
        Xfer_shmp->xf_tail = (Xfer_shmp->xf_tail + 1) % (TRANSBUF_NUM + 1);
        Xfer_shmp->xf_nonq++;
        return  result;
}

int  wjmsg(struct spr_req *req, struct spq *job)
{
        int     ret, save_errno;
        int     blkcount = MSGQ_BLOCKS;

        if  (Xfer_shmp->xf_nonq >= TRANSBUF_NUM + 1)
                return  $EH{Transfer buffer full up};

        /* We try very hard here to avoid races and suchwhat.  On the
           client side only one user will "win" in the next
           statement and should get its message through before
           unlocking.

           On the scheduler side we read the message first and then
           pick up the buffer which it will only do after it has been
           unlocked after the message is sent.

           The "only thing that can go wrong" is for the client to be
           killed between sending the message and writing the buffer,
           but that should be covered by the client's process id being
           checked.  */

        lockxbuf();

        if  (Xfer_shmp->xf_nonq >= TRANSBUF_NUM + 1)  {

                /* Check again in case it filled up whilst we were
                   waiting for the lock to happen.  */

                unlockxbuf();
                return  $EH{Transfer buffer full up};
        }

        /* Send the message, ignoring interrupt errors.  */

        while  ((ret = msgsnd(Ctrl_chan, (struct msgbuf *) req, sizeof(struct sp_xjmsg), IPC_NOWAIT)) < 0)  {
                if  (errno == EINTR)
                        continue;
                if  (errno != EAGAIN)
                        break;
                blkcount--;
                if  (blkcount <= 0)
                        break;
                sleep(MSGQ_BLOCKWAIT);
        }

        /* If all ok, get pointer, copy across buffer, unlock the
           thing and go home.  */

        if  (ret >= 0)  {
                struct  joborptr  *jp = getptr();
                jp->jorp_sender = req->spr_un.j.spr_pid;
                BLOCK_COPY(&jp->jorp_un.q, job, sizeof(struct spq));
#ifdef  USING_MMAP
                msync((char *) Xfer_shmp, sizeof(struct xfershm), MS_ASYNC|MS_INVALIDATE);
#endif
                unlockxbuf();
                return  0;
        }

        save_errno = errno;     /* Might get mangled by unlockxbuf */
        unlockxbuf();
        errno = save_errno;
        return  errno == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error};
}

int  wpmsg(struct spr_req *req, struct spptr *ptr)
{
        int     ret, save_errno;
        int     blkcount = MSGQ_BLOCKS;

        if  (Xfer_shmp->xf_nonq >= TRANSBUF_NUM + 1)
                return  $EH{Transfer buffer full up};

        lockxbuf();

        if  (Xfer_shmp->xf_nonq >= TRANSBUF_NUM + 1)  {
                unlockxbuf();
                return  $EH{Transfer buffer full up};
        }

        /* Send the message, ignoring interrupt errors.  */

        while  ((ret = msgsnd(Ctrl_chan, (struct msgbuf *) req, sizeof(struct sp_xpmsg), IPC_NOWAIT)) < 0)  {
                if  (errno == EINTR)
                        continue;
                if  (errno != EAGAIN)
                        break;
                blkcount--;
                if  (blkcount <= 0)
                        break;
                sleep(MSGQ_BLOCKWAIT);
        }

        if  (ret >= 0)  {
                struct  joborptr  *jp = getptr();
                jp->jorp_sender = req->spr_un.p.spr_pid;
                BLOCK_COPY(&jp->jorp_un.p, ptr, sizeof(struct spptr));
                unlockxbuf();
                return  0;
        }

        save_errno = errno;     /* Might get mangled by unlockxbuf */
        unlockxbuf();
        errno = save_errno;
        return  errno == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error};
}
