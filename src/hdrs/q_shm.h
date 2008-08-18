/* q_shm.h -- shared memory data and structures

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

struct	jshm_hdr	{
	unsigned	js_did;		/* Id of job segment or size if mm-file */
	unsigned	js_psegid;	/* Id of ptr segment or size if mm=file */
	unsigned	js_info;	/* Info to pass to user */
	unsigned	js_njobs;	/* Number of jobs in queue  */
	unsigned	js_maxjobs;	/* Max number provided for */
	int		js_viewport;	/* Port number for viewing remote jobs */
	time_t		js_lastwrite;	/* Last written out to file */
	ULONG		js_serial;	/* Incremented every time written */
	LONG		js_freech;	/* Free chain */
	LONG		js_q_head,	/* Head of job queue */
			js_q_tail;	/* Tail of job queue */
};

struct	pshm_hdr	{
	unsigned	ps_nptrs;	/* Number of printers  */
	unsigned	ps_maxptrs;	/* Max number  */
	time_t		ps_lastwrite;	/* Last written out to file */
	ULONG		ps_serial;	/* Incremented every time written */
	LONG		ps_freech;	/* Free chain */
	LONG		ps_l_head,	/* Head of ptr list */
			ps_l_tail;	/* Tail of ptr list */
};

typedef	struct  {
	LONG	q_nxt, q_prv;		/* Job queue location */
	LONG	nxt_jno_hash,		/* Next on hash chain by job number */
		prv_jno_hash,		/* Previous on hash chain by job number */
		nxt_jid_hash,		/* Next on hash chain by jident */
		prv_jid_hash;		/* Previous on hash chain by jident */
	struct	spq	j;		/* The job */
}  Hashspq;

typedef	struct  {
	LONG	l_nxt, l_prv;		/* Ptr list location */
	LONG	nxt_pid_hash,		/* Next on hash chain by pident */
		prv_pid_hash;		/* Previous on hash chain by pident */
	struct	spptr	p;
}  Hashspptr;

/* UCONST is defined as const on user programs which attach the segments read-only,
   otherwise as the null string (some CCs can't manage const anyhow) */

#ifndef	UCONST
#define	UCONST	const
#endif

struct	spshm_info  {
#ifdef	USING_MMAP
	int		mmfd;		/* File descriptor */
#else
#ifdef	USING_FLOCK
	int		lockfd;		/* Lock file fd distinct from mm fd if we're using shm */
#endif
	int		base,		/* Key base for allocating shared memory segs */
			chan;		/* Current id */
#endif
	UCONST	char	*seg;		/* Current base address from shmat */
	ULONG		segsize; 	/* Size of segment in bytes */
	ULONG		reqsize;	/* Requested segment size */
};

struct	jshm_info  {
	struct	spshm_info	iinf;	/* As above for info segment */
	struct	spshm_info	dinf;	/* As above for data segment */
	UCONST	struct	jshm_hdr *dptr;	/* Pointer to jshm_hdr (pointer into segment) */
	UCONST	LONG	*hashp_jno;	/* Hash by job numbers */
	UCONST	LONG	*hashp_jid;	/* Hash by job idents */
	UCONST	Hashspq	*jlist;		/* Vector of job pointers */
	UCONST	Hashspq	**jj_ptrs;	/* Sorted list to actual jobs */
	unsigned	njobs,		/* Number we can see */
			Njobs;		/* Maximum number */
	ULONG		Last_ser;	/* Serial of last version/changes */
};

struct	pshm_info  {
	struct	spshm_info	inf;	/* As above */
	UCONST	struct	pshm_hdr *dptr;	/* Pointer to pshm_hdr (pointer into segment) */
	UCONST	LONG	*hashp_pid;	/* Hash by ptr ident */
	UCONST	Hashspptr  *plist;	/* Vector of ptr pointers */
	UCONST	Hashspptr  **pp_ptrs;	/* Sorted list to actual ptrs */
	unsigned	nptrs,		/* Printers we are looking at */
			Nptrs,		/* Maximum possible printers */
			npprocesses;	/* Number with active processes */
	ULONG		Last_ser;	/* Serial of last version/changes */
};

#define	SHM_JHASHMOD	397		/* Prime number extracted from atmosphere */
#define	SHM_PHASHMOD	59		/* Prime number extracted from atmosphere */
#define	SHM_HASHEND	(-1L)		/* Denotes end of hash */

extern	struct	jshm_info	Job_seg;
extern	struct	pshm_info	Ptr_seg;

#define	jno_jhash(jno)	(((unsigned) (jno)) % SHM_JHASHMOD)
#define	jid_hash(nid,slot)	(((((unsigned) (nid)) >> 16) ^ ((unsigned) (nid)) ^ (((unsigned) (slot)) >> 4) ^ ((unsigned) (slot))) % SHM_JHASHMOD)
#define	pid_hash(nid,slot)	(((((unsigned) (nid)) >> 16) ^ ((unsigned) (nid)) ^ (((unsigned) (slot)) >> 4) ^ ((unsigned) (slot))) % SHM_PHASHMOD)

extern void	jobshm_lock(void);
extern void	jobshm_unlock(void);
extern void	ptrshm_lock(void);
extern void	ptrshm_unlock(void);
extern int	jobshminit(const int);
extern int	ptrshminit(const int);
extern void	jobgrown(void);
extern void	ptrgrown(void);
