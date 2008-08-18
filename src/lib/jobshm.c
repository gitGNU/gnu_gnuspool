/* jobshm.c -- set up access to job shared memory

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
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "incl_unix.h"
#include "ecodes.h"
#include "errnums.h"

/* All this code attempts to cope with 4 cases depending on whether we
   are using file locking for locking or semaphores (USING_FLOCK) and
   whether we are using memory-mapped files or shared memory (USING_MMAP) */

#ifdef	USING_FLOCK
static void	setjhold(const int typ)
{
	struct	flock	lck;
	lck.l_type = typ;
	lck.l_whence = 0;	/* I.e. SEEK_SET */
	lck.l_start = 0;
	lck.l_len = 0;
	for  (;;)  {
#ifdef	USING_MMAP
		if  (fcntl(Job_seg.iinf.mmfd, F_SETLKW, &lck) >= 0)
			return;
#else
		if  (fcntl(Job_seg.iinf.lockfd, F_SETLKW, &lck) >= 0)
			return;
#endif
		if  (errno != EINTR)
			print_error($E{Lock error});
	}
}

void	jobshm_lock(void)
{
	setjhold(F_RDLCK);
}

void	jobshm_unlock(void)
{
	setjhold(F_UNLCK);
}

#else
extern	int	Sem_chan;

static	struct	sembuf	jr[2] =	{{	JQ_FIDDLE,	0,	0	},
				{	JQ_READING,	1,	SEM_UNDO}},
			ju[1] = {{	JQ_READING,	-1,	SEM_UNDO}};

void	jobshm_lock(void)
{
	for  (;;)  {
		if  (semop(Sem_chan, &jr[0], 2) >= 0)
			return;
		if  (errno == EINTR)
			continue;
		print_error($E{Semaphore error probably undo});
		exit(E_JOBQ);
	}
}

void	jobshm_unlock(void)
{
	for  (;;)  {
		if  (semop(Sem_chan, &ju[0], 1) >= 0)
			return;
		if  (errno == EINTR)
			continue;
		print_error($E{Semaphore error probably undo});
		exit(E_JOBQ);
	}
}
#endif

#ifdef	USING_MMAP
static int	open_mmff(struct spshm_info * sinf, char * mmfname, const int insdir)
{
	if  (insdir)
		sinf->mmfd = open(mmfname, O_RDONLY);
	else  {
		char  *fname = mkspdirfile(mmfname);
		sinf->mmfd = open(fname, O_RDONLY);
		free(fname);
	}
	if  (sinf->mmfd < 0)
		return  0;
	fcntl(sinf->mmfd, F_SETFD, 1);
	return  1;
}
#endif

int	jobshminit(const int insdir)
{
#ifdef	USING_MMAP
	if  (!open_mmff(&Job_seg.iinf, JIMMAP_FILE, insdir))
		return  0;
	Job_seg.iinf.segsize = Job_seg.iinf.reqsize = lseek(Job_seg.iinf.mmfd, 0L, 2);
	if  ((Job_seg.iinf.seg = mmap(0, Job_seg.iinf.segsize, PROT_READ, MAP_SHARED, Job_seg.iinf.mmfd, 0)) == MAP_FAILED)  {
		close(Job_seg.iinf.mmfd);
		return  0;
	}
#else
#ifdef	USING_FLOCK
	if  (insdir)
		Job_seg.iinf.lockfd = open(JLOCK_FILE, O_RDWR);
	else  {
		char	*fname = mkspdirfile(JLOCK_FILE);
		Job_seg.iinf.lockfd = open(fname, O_RDWR);
		free(fname);
	}
	if  (Job_seg.iinf.lockfd < 0)
		return  0;
	fcntl(Job_seg.iinf.lockfd, F_SETFD, 1);
#endif
	/* Get details of info seg */
	Job_seg.iinf.base = JSHMID;
	Job_seg.iinf.segsize = Job_seg.iinf.reqsize = sizeof(struct jshm_hdr) + 2 * SHM_JHASHMOD * sizeof(LONG);
	if  ((Job_seg.iinf.chan = shmget(JSHMID, 0, 0)) < 0)
		return  0;
	if  ((Job_seg.iinf.seg = shmat(Job_seg.iinf.chan, (char *) 0, SHM_RDONLY)) == (const char *) -1)
		return  0;
#endif

	Job_seg.dptr = (const struct jshm_hdr *) Job_seg.iinf.seg;
	Job_seg.hashp_jno = (const LONG *) (Job_seg.iinf.seg + sizeof(struct jshm_hdr));
	Job_seg.hashp_jid = (const LONG *) ((const char *) Job_seg.hashp_jno + SHM_JHASHMOD * sizeof(LONG));

	/* Get details of data seg
	   We don't bother about locks on that. */

#ifdef	USING_MMAP
	if  (!open_mmff(&Job_seg.dinf, JDMMAP_FILE, insdir))
		return  0;
	Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.dptr->js_did;
	Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
	if  ((Job_seg.dinf.seg = mmap(0, Job_seg.dinf.segsize, PROT_READ, MAP_SHARED, Job_seg.dinf.mmfd, 0)) == MAP_FAILED)  {
		close(Job_seg.dinf.mmfd);
		return  0;
	}
	Job_seg.jlist = (const Hashspq *) Job_seg.dinf.seg;
#else
	/* Get details of data seg */
	Job_seg.dinf.base = Job_seg.dptr->js_did;
	Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
	Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.Njobs * sizeof(Hashspq);
	if  ((Job_seg.dinf.chan = shmget((key_t) Job_seg.dinf.base, 0, 0)) < 0)
		return  0;
	if  ((Job_seg.dinf.seg = shmat(Job_seg.dinf.chan, (char *) 0, SHM_RDONLY)) == (const char *) -1)
		return  0;
#endif

	Job_seg.jlist = (const Hashspq *) Job_seg.dinf.seg;
	Job_seg.Last_ser = 0;

	if  ((Job_seg.jj_ptrs = (const Hashspq **) malloc((Job_seg.Njobs + 1) * sizeof(const Hashspq *))) == (const Hashspq **) 0)
		nomem();
	return  1;
}

void	jobgrown(void)
{
#ifdef	USING_MMAP
	if  (Job_seg.dinf.segsize == Job_seg.dptr->js_did)
		return;
	munmap((void *) Job_seg.dinf.seg, Job_seg.dinf.segsize);
	Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.dptr->js_did;
#else
	if  (Job_seg.dinf.base == Job_seg.dptr->js_did)
		return;
	Job_seg.dinf.base = Job_seg.dptr->js_did;
#endif

	if  (Job_seg.Njobs != Job_seg.dptr->js_maxjobs)  {
		Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
#ifndef	USING_MMAP
		Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.Njobs * sizeof(Hashspq);
#endif
		free((char *) Job_seg.jj_ptrs);
		if  ((Job_seg.jj_ptrs = (const Hashspq **) malloc((Job_seg.Njobs + 1) * sizeof(const Hashspq *))) == (const Hashspq **) 0)
			nomem();
	}

#ifdef	USING_MMAP
	if  ((Job_seg.dinf.seg = mmap(0, lseek(Job_seg.dinf.mmfd, 0L, 2), PROT_READ, MAP_SHARED, Job_seg.dinf.mmfd, 0)) == MAP_FAILED)
		nomem();
#else
	shmdt((char *) Job_seg.dinf.seg);	/*  Lose old one  */
	if  ((Job_seg.dinf.chan = shmget((key_t) Job_seg.dinf.base, 0, 0)) <= 0)
		nomem();
	if  ((Job_seg.dinf.seg = shmat(Job_seg.dinf.chan, (char *) 0, SHM_RDONLY)) == (const char *) -1)
		nomem();
#endif

	Job_seg.Last_ser = 0;
	Job_seg.jlist = (const Hashspq *) Job_seg.dinf.seg;
}
