/* ptrshm.c -- set up printer shared memoryxb

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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef	USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <stdio.h>
#include <errno.h>
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "incl_unix.h"
#include "ecodes.h"
#include "errnums.h"
#include "files.h"

/* Note that we define this here and not in any of the main programs any more
   to minimise unresolved externals in the shared libraries */

struct	pshm_info	Ptr_seg;

/* All this code attempts to cope with 4 cases depending on whether we
   are using file locking for locking or semaphores (USING_FLOCK) and
   whether we are using memory-mapped files or shared memory (USING_MMAP) */

#ifdef	USING_FLOCK
static void  setphold(const int typ)
{
	struct	flock	lck;
	lck.l_type = typ;
	lck.l_whence = 0;	/* I.e. SEEK_SET */
	lck.l_start = 0;
	lck.l_len = 0;
	for  (;;)  {
#ifdef	USING_MMAP
		if  (fcntl(Ptr_seg.inf.mmfd, F_SETLKW, &lck) >= 0)
			return;
#else
		if  (fcntl(Ptr_seg.inf.lockfd, F_SETLKW, &lck) >= 0)
			return;
#endif
		if  (errno != EINTR)
			print_error($E{Lock error});
	}
}

void  ptrshm_lock()
{
	setphold(F_RDLCK);
}

void  ptrshm_unlock()
{
	setphold(F_UNLCK);
}
#else
static	struct	sembuf	pr[2] = {{	PQ_FIDDLE,	0,	0	},
				 {	PQ_READING,	1,	SEM_UNDO}},
			pu[1] = {{	PQ_READING,	-1,	SEM_UNDO}};

void  ptrshm_lock()
{
	for  (;;)  {
		if  (semop(Sem_chan, &pr[0], 2) >= 0)
			return;
		if  (errno == EINTR)
			continue;
		print_error($E{Semaphore error probably undo});
		exit(E_PRINQ);
	}
}

void  ptrshm_unlock()
{
	for  (;;)  {
		if  (semop(Sem_chan, &pu[0], 1) >= 0)
			return;
		if  (errno == EINTR)
			continue;
		print_error($E{Semaphore error probably undo});
		exit(E_PRINQ);
	}
}
#endif

int  ptrshminit(const int insdir)
{
#ifdef  USING_MMAP
	if  (insdir)
		Ptr_seg.inf.mmfd = open(PMMAP_FILE, O_RDONLY);
	else  {
		char  *fname = mkspdirfile(PMMAP_FILE);
		Ptr_seg.inf.mmfd = open(fname, O_RDONLY);
		free(fname);
	}
	if  (Ptr_seg.inf.mmfd < 0)
		return  0;

	fcntl(Ptr_seg.inf.mmfd, F_SETFD, 1);
	Ptr_seg.inf.segsize = Ptr_seg.inf.reqsize = lseek(Ptr_seg.inf.mmfd, 0L, 2);
	if  ((Ptr_seg.inf.seg = mmap(0, Ptr_seg.inf.segsize, PROT_READ, MAP_SHARED, Ptr_seg.inf.mmfd, 0)) == MAP_FAILED)  {
		close(Ptr_seg.inf.mmfd);
		return  0;
	}
#else
#ifdef	USING_FLOCK
	if  (insdir)
		Ptr_seg.inf.lockfd = open(PLOCK_FILE, O_RDWR);
	else  {
		char	*fname = mkspdirfile(PLOCK_FILE);
		Ptr_seg.inf.lockfd = open(fname, O_RDWR);
		free(fname);
	}
	if  (Ptr_seg.inf.lockfd < 0)
		return  0;
	fcntl(Ptr_seg.inf.lockfd, F_SETFD, 1);
#endif
	if  (!Job_seg.dptr)	/* Protect against wrong order of call */
		return  0;
	Ptr_seg.inf.base = Job_seg.dptr->js_psegid;
	if  ((Ptr_seg.inf.chan = shmget((key_t) Ptr_seg.inf.base, 0, 0)) < 0)
		return  0;
	if  ((Ptr_seg.inf.seg = shmat(Ptr_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (const char *) -1)
		return  0;
	Ptr_seg.inf.segsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Ptr_seg.Nptrs * sizeof(Hashspptr);
#endif

	Ptr_seg.Last_ser = 0;
	Ptr_seg.dptr = (const struct pshm_hdr *) Ptr_seg.inf.seg;
	Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;
	Ptr_seg.hashp_pid = (const LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
	Ptr_seg.plist = (const Hashspptr *) ((const char *) Ptr_seg.hashp_pid + SHM_PHASHMOD * sizeof(LONG));
	if  ((Ptr_seg.pp_ptrs = (const Hashspptr **) malloc((Ptr_seg.Nptrs + 1) * sizeof(const Hashspptr *))) == (const Hashspptr **) 0)
		nomem();
	return  1;
}

void  ptrgrown()
{
#ifdef	USING_MMAP
	if  (Ptr_seg.inf.segsize == Job_seg.dptr->js_psegid)
		return;

	munmap((void *) Ptr_seg.inf.seg, Ptr_seg.inf.segsize);
	Ptr_seg.inf.segsize = Job_seg.dptr->js_psegid;
	if  ((Ptr_seg.inf.seg = mmap(0, Ptr_seg.inf.segsize, PROT_READ, MAP_SHARED, Ptr_seg.inf.mmfd, 0)) == MAP_FAILED)
		nomem();
#else
	if  (Ptr_seg.inf.base == Job_seg.dptr->js_psegid)
		return;

	Ptr_seg.inf.base = Job_seg.dptr->js_psegid;

	shmdt((char *) Ptr_seg.inf.seg);	/*  Lose old one  */

	if  ((Ptr_seg.inf.chan = shmget((key_t) Ptr_seg.inf.base, 0, 0)) <= 0)
		nomem();
	if  ((Ptr_seg.inf.seg = shmat(Ptr_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (const char *) -1)
		nomem();
#endif
	Ptr_seg.dptr = (const struct pshm_hdr *) Ptr_seg.inf.seg;
	Ptr_seg.Last_ser = 0;
	Ptr_seg.hashp_pid = (const LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
	Ptr_seg.plist = (const Hashspptr *) ((const char *) Ptr_seg.hashp_pid + SHM_PHASHMOD * sizeof(LONG));
	if  (Ptr_seg.Nptrs != Ptr_seg.dptr->ps_maxptrs)  {
		Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;

#ifndef	USING_MMAP
		Ptr_seg.inf.segsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Ptr_seg.Nptrs * sizeof(Hashspptr);
#endif
		free((char *) Ptr_seg.pp_ptrs);
		if  ((Ptr_seg.pp_ptrs = (const Hashspptr **) malloc((Ptr_seg.Nptrs + 1) * sizeof(const Hashspptr *))) == (const Hashspptr **) 0)
			nomem();
	}
}
