/* ripc.c -- check/report errors in shared memory etc

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
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#ifdef	HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef	AF_INET
#define	AF_INET	2
#endif
#include "defaults.h"
#include "network.h"
#include "files.h"
#include "spq.h"
#include "ipcstuff.h"
#define	UCONST
#ifdef	USING_MMAP
#undef	USING_MMAP
#endif
#define	mmfd	base
#include "q_shm.h"
#include "xfershm.h"
#include "incl_unix.h"
#include "ecodes.h"

#define	DEFAULT_NWORDS	4
#define	MAX_NWORDS	8

int			daemtime;
unsigned		blksize = DEFAULT_NWORDS;		/* Bumped up to bytes later */
char			npchar = '.', dumphex, dumpok, dumpall, psok, notifok, dipc, showfree, nofgrep;
char			*psarg;
char			*spooldir = "/usr/spool/spd";

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

#if	(defined(OS_LINUX) || defined(OS_BSDI)) && !defined(_SEM_SEMUN_UNDEFINED)
#define	my_semun	semun
#else

/* Define a union as defined in semctl(2) to pass the 4th argument of
   semctl. On most machines it is good enough to pass the value
   but on the SPARC and PYRAMID unions are passed differently
   even if all the possible values would fit in an int.  */

union	my_semun	{
	int	val;
	struct	semid_ds	*buf;
	ushort			*array;
};
#endif

void	nomem()
{
	fprintf(stderr, "Out of memory\n");
	exit(E_NOMEM);
}

#ifdef	HAVE_SYS_MMAN_H
char *mmfile_name(const char *name)
{
	static	char	fname[PATH_MAX];

	/* Paranoia about buffer overflow */
	if  (strlen(spooldir) + strlen(name) + 2 > PATH_MAX)
		return  "none";
	strcpy(fname, spooldir);
	strcat(fname, "/");
	strcat(fname, name);
	return  fname;
}

/* Locate a memory-mapped file in its hole and return an fd to it
   and stat-ify it into sb */

int  open_mmfile(const char *name, struct stat *sb)
{
	int	result;
	if  ((result = open(mmfile_name(name), O_RDONLY)) < 0)
		return  -1;
	fstat(result, sb);
	return  result;
}
#endif

void  segdump(unsigned char *shp, unsigned segsz)
{
	unsigned  addr = 0, had = 0, hadsame = 0, left = segsz;
	unsigned  char  lastbuf[MAX_NWORDS * sizeof(ULONG)];

	while  (left > 0)  {
		unsigned  pce = left > blksize? blksize: left;
		unsigned  cnt;

		if  (had  &&  pce == blksize  &&  memcmp(shp, lastbuf, blksize) == 0)  {
			if  (!hadsame)  {
				hadsame++;
				printf("%.6x -- same ---\n", addr);
			}
		}
		else  {
			had++;
			hadsame = 0;
			memcpy(lastbuf, shp, blksize);
			printf("%.6x ", addr);
			for  (cnt = 0;  cnt < pce;  cnt++)  {
				printf("%.2x", shp[cnt]);
				if  ((cnt & (sizeof(ULONG)-1)) == sizeof(ULONG)-1)
					putchar(' ');
			}
			if  (pce < blksize)  {
				unsigned  col;
				for  (col = pce * 2 + pce/sizeof(ULONG);  col < blksize/sizeof(ULONG) * (2*sizeof(ULONG)+1);  col++)
					putchar(' ');
			}
			for  (cnt = 0;  cnt < pce;  cnt++)  {
				putchar(isprint(shp[cnt])? shp[cnt]: npchar);
				if  ((cnt & (sizeof(ULONG)-1)) == (sizeof(ULONG)-1)  &&  cnt != pce-1)
					putchar(' ');
			}
			putchar('\n');
		}

		addr += pce;
		left -= pce;
		shp += pce;
	}
	printf("%.6x --- end ---\n", addr);
}

void  dump_lock(const int fd, const char *msg)
{
#ifdef	OS_LINUX
	FILE	*ifl;
	char	fname[80];
#endif
	struct	flock	lck;
	lck.l_type = F_WRLCK;
	lck.l_whence = 0;
	lck.l_start = 0;
	lck.l_len = 0;
	if  (fcntl(fd, F_GETLK, &lck) < 0  ||  lck.l_type == F_UNLCK)
		return;
	printf("%s segment is locked by process id %d\n", msg, lck.l_pid);
#ifdef	OS_LINUX
	sprintf(fname, "/proc/%d/cmdline", lck.l_pid);
	if  ((ifl = fopen(fname, "r")))  {
		int	ch;
		printf("\tCmd for pid %d:\n\t", lck.l_pid);
		while  ((ch = getc(ifl)) != EOF)  {
			if  (!isprint(ch))
				ch = ' ';
			putchar(ch);
		}
		fclose(ifl);
		putchar('\n');
	}
#endif
}

void  dump_mode(unsigned mode)
{
	printf("%c%c-", mode & 4? 'r': '-', mode & 2? 'w': '-');
}

char *pr_uname(uid_t uid)
{
	static	char	buf[16];
	struct	passwd	*pw;

	if  ((pw = getpwuid(uid)))
		return  pw->pw_name;
	sprintf(buf, "U%ld", (long) uid);
	return  buf;
}

char *pr_gname(gid_t gid)
{
	static	char	buf[16];
	struct	group	*pw;

	if  ((pw = getgrgid(gid)))
		return  pw->gr_name;
	sprintf(buf, "G%ld", (long) gid);
	return  buf;
}

char *lookhost(netid_t hid)
{
	static	char	buf[100];
	struct	hostent	*hp;

	if  (hid == 0L)
		return  "";
	if  (!(hp = gethostbyaddr((char *) &hid, sizeof(hid), AF_INET)))  {
		struct	in_addr	addr;
		addr.s_addr = hid;
		sprintf(buf, "%s:", inet_ntoa(addr));
	}
	else
		sprintf(buf, "%s:", hp->h_name);
	return  buf;
}

void  dump_ipcperm(struct ipc_perm *perm)
{
	printf("Owner: %s/%s\t", pr_uname(perm->uid), pr_gname(perm->gid));
	printf("Creator: %s/%s\tMode: ", pr_uname(perm->cuid), pr_gname(perm->cgid));
	dump_mode(perm->mode >> 6);
	dump_mode(perm->mode >> 3);
	dump_mode(perm->mode);
	putchar('\n');
}

void  dotime(time_t arg, char *descr)
{
	struct	tm	*tp = localtime(&arg);
	static  char	*wdays[] =  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"  };

	printf("%s:\t%s %.2d/%.2d/%.4d at %.2d:%.2d:%.2d\n", descr,
	       wdays[tp->tm_wday], tp->tm_mday, tp->tm_mon+1, tp->tm_year + 1900,
	       tp->tm_hour, tp->tm_min, tp->tm_sec);
}

void  dops(const long lpid1, const long lpid2)
{
	char	obuf[100];

	if  (!psarg)
		return;
	if  (lpid1 == 0L && lpid2 == 0L)  {
		sprintf(obuf, "ps %s", psarg);
		printf("Memory mapped files so no last attaching process\n");
	}
	else  {
		if  (lpid1 == getpid() || lpid2 == getpid())  {
			printf("Sorry pid field overwritten by self (%ld/%ld)\n", lpid1, lpid2);
			return;
		}
		printf("Last attaching processes were id %ld and %ld, output of ps follows\n\n", lpid1, lpid2);
		if  (nofgrep)
			sprintf(obuf, "ps %s", psarg);
		else  if  (lpid1 == 0L)
			sprintf(obuf, "ps %s | fgrep \' %ld \'", psarg, lpid2);
		else  if  (lpid2 == 0L)
			sprintf(obuf, "ps %s | fgrep \' %ld \'", psarg, lpid1);
		else
			sprintf(obuf, "ps %s | fgrep \' %ld \n %ld \'", psarg, lpid1, lpid2);
	}
	fflush(stdout);
	Ignored_error = system(obuf);
}

void  shmhdr(long key, int chan, struct shmid_ds *sp, char *descr)
{
	printf("Shared memory for %s, key %lx, id %d\n", descr, key, chan);
	dump_ipcperm(&sp->shm_perm);
	printf("Size is %d, last op pid %ld, creator pid %ld, attached %lu\n",
		      (int) sp->shm_segsz, (long) sp->shm_lpid, (long) sp->shm_cpid,
		      (unsigned long) sp->shm_nattch);
	dotime(sp->shm_atime, "Last attached");
	dotime(sp->shm_dtime, "Last detached");
	dotime(sp->shm_ctime, "Last changed");
}

#ifdef	HAVE_SYS_MMAN_H
void  mmaphdr(struct stat *sp, const char *descr)
{
	printf("Memory mapped file for %s\n", descr);
	printf("Owner: %s/%s\tMode:", pr_uname(sp->st_uid), pr_gname(sp->st_gid));
	putchar(sp->st_mode & (1 << 8)? 'r': '-');
	putchar(sp->st_mode & (1 << 7)? 'w': '-');
	putchar(sp->st_mode & (1 << 6)? (sp->st_mode & (1 << 11)? 's': 'x'): (sp->st_mode & (1 << 11)? 'S': '-'));
	putchar(sp->st_mode & (1 << 5)? 'r': '-');
	putchar(sp->st_mode & (1 << 4)? 'w': '-');
	putchar(sp->st_mode & (1 << 3)? (sp->st_mode & (1 << 10)? 's': 'x'): (sp->st_mode & (1 << 10)? 'S': '-'));
	putchar(sp->st_mode & (1 << 2)? 'r': '-');
	putchar(sp->st_mode & (1 << 1)? 'w': '-');
	putchar(sp->st_mode & (1 << 0)? (sp->st_mode & (1 << 9)? 't': 'x'): (sp->st_mode & (1 << 9)? 'T': '-'));
	putchar('\n');
	dotime(sp->st_atime, "Last accessed");
	dotime(sp->st_mtime, "Last modified");
	dotime(sp->st_ctime, "Last changed");
}
#endif

void  dumpjob(struct spq *jp)
{
	printf("%s%ld\t", lookhost(jp->spq_netid), (long) jp->spq_job);
	if  (jp->spq_jflags & SPQ_NOH)
		printf(" NOH");
	if  (jp->spq_jflags & SPQ_WRT)
		printf(" WRT");
	if  (jp->spq_jflags & SPQ_MAIL)
		printf(" ML");
	if  (jp->spq_jflags & SPQ_RETN)
		printf(" RET");
	if  (jp->spq_jflags & SPQ_ODDP)
		printf(" ODD");
	if  (jp->spq_jflags & SPQ_EVENP)
		printf(" EVEN");
	if  (jp->spq_jflags & SPQ_REVOE)
		printf(" REV");
	if  (jp->spq_jflags & SPQ_MATTN)
		printf(" MA");
	if  (jp->spq_jflags & SPQ_WATTN)
		printf(" WA");
	if  (jp->spq_jflags & SPQ_LOCALONLY)
		printf(" LO");
	if  (jp->spq_sflags & SPQ_ASSIGN)
		printf(" ASS");
	if  (jp->spq_sflags & SPQ_WARNED)
		printf(" WARN");
	if  (jp->spq_sflags & SPQ_PROPOSED)
		printf(" PROP");
	if  (jp->spq_sflags & SPQ_ABORTJ)
		printf(" ABT");
	if  (jp->spq_dflags & SPQ_PQ)
		printf(" PQ");
	if  (jp->spq_dflags & SPQ_PRINTED)
		printf(" PTD");
	if  (jp->spq_dflags & SPQ_STARTED)
		printf(" STD");
	if  (jp->spq_dflags & SPQ_PAGEFILE)
		printf(" PF");
	if  (jp->spq_dflags & SPQ_ERRLIMIT)
		printf(" EL");
	if  (jp->spq_dflags & SPQ_PGLIMIT)
		printf(" PGL");
	putchar('\n');
}

void  dumpptr(struct spptr *pp)
{
	static	char	*snames[] =  { "null", "offl", "err", "hlt", "init", "idle", "shd", "prt", "a/w"  };
	printf("%s%s\t%s\t%s\t", lookhost(pp->spp_netid), pp->spp_ptr, pp->spp_dev,
	       pp->spp_state < SPP_NSTATES? snames[(int) pp->spp_state]: "???");
	if  (pp->spp_sflags & SPP_SELECT)
		printf(" SEL");
	if  (pp->spp_sflags & SPP_INTER)
		printf(" INT");
	if  (pp->spp_sflags & SPP_HEOJ)
		printf(" HE");
	if  (pp->spp_sflags & SPP_PROPOSED)
		printf(" PROP[%s%ld]", lookhost(pp->spp_rjhostid), (long) pp->spp_job);
	if  (pp->spp_dflags & SPP_HADAB)
		printf(" HA");
	if  (pp->spp_dflags & SPP_REQALIGN)
		printf(" REQA");
	if  (pp->spp_netflags & SPP_LOCALNET)
		printf(" LNET");
	if  (pp->spp_netflags & SPP_LOCALONLY)
		printf(" LOCO");
	putchar('\n');
}

struct	jbitmaps  {
	unsigned  msize;
	ULONG	*jbitmap,	/* Jobs */
		*fbitmap,	/* Free chain */
		*hbitmap;	/* Job numbers */
};

int  jbitmap_init(struct jbitmaps *bms)
{
	bms->msize = ((Job_seg.dptr->js_maxjobs + 31) >> 5) << 2;
	bms->jbitmap = (ULONG *) malloc(bms->msize);
	bms->fbitmap = (ULONG *) malloc(bms->msize);
	bms->hbitmap = (ULONG *) malloc(bms->msize);
	return  bms->jbitmap  &&  bms->fbitmap  &&  bms->hbitmap;
}

void  jbitmap_free(struct jbitmaps *bms)
{
	free((char *) bms->hbitmap);
	free((char *) bms->fbitmap);
	free((char *) bms->jbitmap);
}

int  jbitmap_check(struct jbitmaps *bms)
{
	if  (bms->msize == ((Job_seg.dptr->js_maxjobs + 31) >> 5) << 2)
		return  1;
	jbitmap_free(bms);
	return  jbitmap_init(bms);
}

void  jobseg_hinit1()
{
	Job_seg.hashp_jno = (LONG *) (Job_seg.iinf.seg + sizeof(struct jshm_hdr));
	Job_seg.hashp_jid = (LONG *) ((char *) Job_seg.hashp_jno + SHM_JHASHMOD * sizeof(LONG));
}

void  del_jshm(const int typ1, const int typ2)
{
#ifdef	HAVE_SYS_MMAN_H
	if  (typ1 > 0)
#endif
		shmctl(Job_seg.iinf.chan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef	HAVE_SYS_MMAN_H
	else  if  (typ1 < 0)  {
		munmap(Job_seg.iinf.seg, Job_seg.iinf.segsize);
		close(Job_seg.iinf.mmfd);
		unlink(mmfile_name(JIMMAP_FILE));
	}
#endif
#ifdef	HAVE_SYS_MMAN_H
	if  (typ2 > 0)
#endif
		shmctl(Job_seg.dinf.chan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef	HAVE_SYS_MMAN_H
	else  if  (typ2 < 0)  {
		munmap(Job_seg.dinf.seg, Job_seg.iinf.segsize);
		close(Job_seg.dinf.mmfd);
		unlink(mmfile_name(JDMMAP_FILE));
	}
#endif
}

int  jobshminit1(struct shmid_ds *sp, struct stat *fsb)
{
#ifdef	HAVE_SYS_MMAN_H

	/* First try to find a memory-mapped file*/

	int	i = open_mmfile(JIMMAP_FILE, fsb);

	if  (i >= 0)  {
		void	*buffer;
		Job_seg.iinf.mmfd = i;
		Job_seg.iinf.reqsize = Job_seg.iinf.segsize = fsb->st_size;
		if  ((buffer = mmap(0, fsb->st_size, PROT_READ, MAP_SHARED, i, 0)) == MAP_FAILED)  {
			fprintf(stderr, "Found job file %s but could not attach it\n", JIMMAP_FILE);
			close(i);
			return  0;
		}
		Job_seg.iinf.seg = buffer;
		Job_seg.dptr = (struct jshm_hdr *) buffer;
		jobseg_hinit1();
		return  -1;
	}
#endif
	Job_seg.iinf.base = JSHMID;
	Job_seg.iinf.segsize = Job_seg.iinf.reqsize = sizeof(struct jshm_hdr) + 2 * SHM_JHASHMOD * sizeof(LONG);
	if  ((Job_seg.iinf.chan = shmget(JSHMID, 0, 0)) < 0)
		return  0;
	if  (shmctl(Job_seg.iinf.chan, IPC_STAT, sp) < 0)
		return  0;
	if  ((Job_seg.iinf.seg = shmat(Job_seg.iinf.chan, (char *) 0, 0)) == (char *) -1)
		return  0;
	Job_seg.dptr = (struct jshm_hdr *) Job_seg.iinf.seg;
	Ptr_seg.inf.base = Job_seg.dptr->js_psegid; /* Set for ptrshminit */
	jobseg_hinit1();
	return  1;
}

int  jobshminit2(struct shmid_ds *sp, struct stat *fsb)
{
#ifdef	HAVE_SYS_MMAN_H

	/* First try to find a memory-mapped file*/

	int	i = open_mmfile(JDMMAP_FILE, fsb);

	if  (i >= 0)  {
		void	*buffer;
		Job_seg.dinf.mmfd = i;
		Job_seg.dinf.reqsize = Job_seg.dinf.segsize = fsb->st_size;
		if  ((buffer = mmap(0, fsb->st_size, PROT_READ, MAP_SHARED, i, 0)) == MAP_FAILED)  {
			fprintf(stderr, "Found job file %s but could not attach it\n", JDMMAP_FILE);
			close(i);
			return  0;
		}
		Job_seg.dinf.seg = buffer;
		Job_seg.jlist = (Hashspq *) buffer;
		return  -1;
	}
#endif
	Job_seg.dinf.base = Job_seg.dptr->js_did;
	Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
	Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.Njobs * sizeof(Hashspq);
	if  ((Job_seg.dinf.chan = shmget((key_t) Job_seg.dinf.base, 0, 0)) < 0)
		return  0;
	if  (shmctl(Job_seg.dinf.chan, IPC_STAT, sp) < 0)
		return  0;
	if  ((Job_seg.dinf.seg = shmat(Job_seg.dinf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)
		return  0;
	Job_seg.jlist = (Hashspq *) Job_seg.dinf.seg;
	return  1;
}

inline int  bitmap_isset(ULONG *bm, const unsigned n)
{
	return  bm[n >> 5] & (1 << (n & 31));
}

inline void  bitmap_set(ULONG *bm, const unsigned n)
{
	bm[n >> 5] |= 1 << (n & 31);
}

/* Decide if the job segment is mangled.
   Return 1 if it is but don't print anything
   Note that there is a possibility of a race condition if the
   job shm is being updated but we daren't use the semaphore in case
   we miss whatever is doing the dirty. */

int  check_jmangled(struct jbitmaps *bms)
{
	unsigned  cnt;
	LONG	jind, prevind;

	if  (Job_seg.dptr->js_njobs > Job_seg.dptr->js_maxjobs)
		return  1;

	BLOCK_ZERO(bms->jbitmap, bms->msize);
	BLOCK_ZERO(bms->fbitmap, bms->msize);
	BLOCK_ZERO(bms->hbitmap, bms->msize);

	/* Check the job queue is OK with forward and back links */

	jind = Job_seg.dptr->js_q_head;
	prevind = SHM_HASHEND;

	while  (jind != SHM_HASHEND)  {
		Hashspq	*hjp = &Job_seg.jlist[jind];

		if  (jind >= Job_seg.dptr->js_maxjobs)
			return	1;

		if  (bitmap_isset(bms->jbitmap, jind))
			return  1;

		bitmap_set(bms->jbitmap, jind);

		if  (hjp->j.spq_netid == 0  &&  jind != hjp->j.spq_rslot)
			return  1;

		if  (prevind != hjp->q_prv)
			return  1;

		prevind = jind;
		jind = hjp->q_nxt;
	}

	/* Check the free chain is OK */

	jind = Job_seg.dptr->js_freech;
	while  (jind != SHM_HASHEND)  {
		Hashspq	*hjp = &Job_seg.jlist[jind];

		if  (jind >= Job_seg.dptr->js_maxjobs)
			return  1;

		if  (bitmap_isset(bms->fbitmap, jind))
			return  1;

		bitmap_set(bms->fbitmap, jind);
		if  (bitmap_isset(bms->jbitmap, jind))
			return  1;

		if  (hjp->q_prv != SHM_HASHEND)
			return	1;

		jind = hjp->q_nxt;
	}

	/* Look for "orphaned" jobs */

	for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
		if  (!bitmap_isset(bms->jbitmap, cnt)  &&  !bitmap_isset(bms->fbitmap, cnt))
			return  1;

	/* Check the job number hash */

	for  (cnt = 0;  cnt < SHM_JHASHMOD;  cnt++)  {
		int	ccnt = 0;
		jind = Job_seg.hashp_jno[cnt];

		if  (jind == SHM_HASHEND)
			continue;

		if  (jind >= Job_seg.dptr->js_maxjobs)
			return  1;

		prevind = SHM_HASHEND;

		do  {
			Hashspq	*hjp = &Job_seg.jlist[jind];

			if  (jno_jhash(hjp->j.spq_job) != cnt)
				return  1;

			if  (bitmap_isset(bms->hbitmap, jind))
				return  1;

			if  (!bitmap_isset(bms->jbitmap, jind))
				return  1;

			if  (bitmap_isset(bms->fbitmap, jind))
				return  1;

			bitmap_set(bms->hbitmap, jind);

			if  (prevind != hjp->prv_jno_hash)
				return  1;

			ccnt++;
			prevind = jind;
			jind = hjp->nxt_jno_hash;
		}  while  (jind != SHM_HASHEND);
	}

	/* Check for inconsistencies */

	for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
		if  (bitmap_isset(bms->jbitmap, cnt)  &&  !bitmap_isset(bms->hbitmap, cnt))
			return  1;

	/* Check Ident hash chain */

	BLOCK_ZERO(bms->hbitmap, bms->msize);

	for  (cnt = 0;  cnt < SHM_JHASHMOD;  cnt++)  {
		int	ccnt = 0;
		jind = Job_seg.hashp_jid[cnt];

		if  (jind == SHM_HASHEND)
			continue;

		if  (jind >= Job_seg.dptr->js_maxjobs)
			return  1;

		prevind = SHM_HASHEND;

		do  {
			Hashspq	*hjp = &Job_seg.jlist[jind];

			if  (jid_hash(hjp->j.spq_netid,hjp->j.spq_rslot) != cnt)
				return  1;

			if  (bitmap_isset(bms->hbitmap, jind))
				return  1;

			if  (!bitmap_isset(bms->jbitmap, jind))
				return  1;

			if  (bitmap_isset(bms->fbitmap, jind))
				return  1;

			bitmap_set(bms->hbitmap, jind);

			if  (prevind != hjp->prv_jid_hash)
				return  1;

			ccnt++;
			prevind = jind;
			jind = hjp->nxt_jid_hash;
		}  while  (jind != SHM_HASHEND);
	}

	for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
		if  (bitmap_isset(bms->jbitmap, cnt)  &&  !bitmap_isset(bms->hbitmap, cnt))
			return  1;

	return  0;
}

void  dump_jobshm()
{
	unsigned		cnt, errs = 0;
	LONG			jind, prevind;
	int			typ1 = 0, typ2 = 0;
	struct	shmid_ds	isbuf, dsbuf;
	struct	stat		ifsbuf, dfsbuf;
	struct	jbitmaps	jbm;

	if  ((typ1 = jobshminit1(&isbuf, &ifsbuf)) == 0)
		printf("Cannot open job info shm\n");
	else  if  ((typ2 = jobshminit2(&dsbuf, &dfsbuf)) == 0)
		printf("Cannot open job data shm\n");

	if  (typ1 == 0  ||  typ2 == 0)  {
		printf("Aborting job dump as segment(s) missing\n");
		if  (dipc)
			del_jshm(typ1, typ2);
		return;
	}
	if  (typ1 < 0)
		dump_lock(Job_seg.iinf.mmfd, "Job info");
	if  (typ2 < 0)
		dump_lock(Job_seg.dinf.mmfd, "Job data");

	if  (!jbitmap_init(&jbm))  {
		fprintf(stderr, "Sorry - cannot allocate bitmaps for jobs\n");
		if  (dipc)
			del_jshm(typ1, typ2);
		return;
	}

	if  (daemtime > 0)  {
		for  (;;)  {
			if  (check_jmangled(&jbm))  {
				dotime(time((time_t *) 0), "JOB SEGMENT CORRUPTED!!!");
				dops(typ1 < 0? 0L: (long) isbuf.shm_lpid, typ2 < 0? 0L: (long) dsbuf.shm_lpid);
				break;
			}

			if  (!notifok)  {
				dotime(time((time_t *) 0), "Jseg OK");
				if  (psok)
					dops(typ1 < 0? 0L: (long) isbuf.shm_lpid, typ2 < 0? 0L: (long) dsbuf.shm_lpid);
				if  (dumpok)  {
					segdump((unsigned char *) Job_seg.iinf.seg, Job_seg.iinf.segsize);
					segdump((unsigned char *) Job_seg.dinf.seg, Job_seg.dinf.segsize);
				}
			}

			sleep((unsigned) daemtime);

#ifdef	HAVE_SYS_MMAN_H
			if  (typ1 > 0)
#endif
				shmctl(Job_seg.iinf.chan, IPC_STAT, &isbuf);
#ifdef	HAVE_SYS_MMAN_H
			if  (typ2 > 0)
#endif
				shmctl(Job_seg.dinf.chan, IPC_STAT, &dsbuf);

#ifdef	HAVE_SYS_MMAN_H
			if  (typ2 < 0)  {
				if  (Job_seg.dinf.segsize == Job_seg.dptr->js_did)
					continue;
				munmap(Job_seg.dinf.seg, Job_seg.dinf.segsize);
				Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.dptr->js_did;
				Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
				if  ((Job_seg.dinf.seg = mmap(0, Job_seg.dinf.segsize, PROT_READ, MAP_SHARED, Job_seg.dinf.mmfd, 0)) == MAP_FAILED)  {
					printf("Job segment disappeared\n");
					if  (dipc)
						del_jshm(typ1, typ2);
					jbitmap_free(&jbm);
					return;
				}
			}
			else  {
#endif
				if  (Job_seg.dinf.base == Job_seg.dptr->js_did)
					continue;
				shmdt((char *) Job_seg.dinf.seg);	/*  Lose old one  */
				Job_seg.dinf.base = Job_seg.dptr->js_did;
				Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
				Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.Njobs * sizeof(Hashspq);
				if  ((Job_seg.dinf.chan = shmget((key_t) Job_seg.dinf.base, 0, 0)) <= 0  ||
				     (Job_seg.dinf.seg = shmat(Job_seg.dinf.chan, (char *) 0, 0)) == (char *) -1)  {
					printf("Job segment disappeared\n");
					if  (dipc)
						del_jshm(typ1, typ2);
					jbitmap_free(&jbm);
					return;
				}
#ifdef	HAVE_SYS_MMAN_H
			}
#endif
			Job_seg.jlist = (Hashspq *) Job_seg.dinf.seg;

			if  (!jbitmap_check(&jbm))  {
				fprintf(stderr, "Sorry - cannot re-allocate bitmaps for jobs\n");
				if  (dipc)
					del_jshm(typ1, typ2);
				return;
			}
		}
	}

#ifdef	HAVE_SYS_MMAN_H
	if  (typ1 < 0)  {
		if  (daemtime > 0)
			fstat(Job_seg.iinf.mmfd, &ifsbuf); /* Needs refresh */
		mmaphdr(&ifsbuf, "Job info mmap file");
	}
	else
#endif
		shmhdr(Job_seg.iinf.base, Job_seg.iinf.chan, &isbuf, "Job info shared memory");
#ifdef	HAVE_SYS_MMAN_H
	if  (typ2 < 0)  {
		if  (daemtime > 0)
			fstat(Job_seg.dinf.mmfd, &dfsbuf); /* Needs refresh */
		mmaphdr(&dfsbuf, "Job data mmap file");
	}
	else
#endif
		shmhdr(Job_seg.dinf.base, Job_seg.dinf.chan, &dsbuf, "Job data shared memory");

	printf("Header info:\n\nnjobs:\t%u\tmaxjobs:\t%u\n", Job_seg.dptr->js_njobs, Job_seg.dptr->js_maxjobs);
	dotime(Job_seg.dptr->js_lastwrite, "Last write");
	printf("Free chain %ld\n", (long) Job_seg.dptr->js_freech);
	printf("Serial:\t%lu\n\nQueue: Head %ld Tail %ld\n", (unsigned long) Job_seg.dptr->js_serial, (long) Job_seg.dptr->js_q_head, (long) Job_seg.dptr->js_q_tail);

	if  (dumpall)  {
		for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)  {
			Hashspq	*hjp = &Job_seg.jlist[cnt];
			printf("\n%u:\tNxt %ld Prv %ld\n", cnt, (long) hjp->q_nxt, (long) hjp->q_prv);
			dumpjob(&hjp->j);
		}
		putchar('\n');
	}

	printf("Following queue\n");
	BLOCK_ZERO(jbm.jbitmap, jbm.msize);
	BLOCK_ZERO(jbm.fbitmap, jbm.msize);
	BLOCK_ZERO(jbm.hbitmap, jbm.msize);

	jind = Job_seg.dptr->js_q_head;
	prevind = SHM_HASHEND;

	while  (jind != SHM_HASHEND)  {
		Hashspq	*hjp = &Job_seg.jlist[jind];

		if  (jind >= Job_seg.dptr->js_maxjobs)  {
			printf("****Off end of job list, job slot %ld\n", (long) jind);
			goto  baderr;
		}

		if  (bitmap_isset(jbm.jbitmap, jind))  {
			printf("****Loop in job list, job slot %ld\n", (long) jind);
			goto  baderr;
		}
		bitmap_set(jbm.jbitmap, jind);

		printf("%ld:\t%s%ld\n", (long) jind, lookhost(hjp->j.spq_netid), (long) hjp->j.spq_job);

		if  (hjp->j.spq_netid == 0  &&  jind != hjp->j.spq_rslot)  {
			printf("****rslot is %ld not %ld\n", (long) hjp->j.spq_rslot, (long) jind);
			errs++;
		}

		if  (prevind != hjp->q_prv)  {
			printf("****Previous pointer points to %ld and not %ld as expected\n", (long) hjp->q_prv, (long) prevind);
			errs++;
		}

		prevind = jind;
		jind = hjp->q_nxt;
	}

	printf("Following free chain\n");

	jind = Job_seg.dptr->js_freech;
	cnt = 0;
	while  (jind != SHM_HASHEND)  {
		Hashspq	*hjp = &Job_seg.jlist[jind];

		if  (jind >= Job_seg.dptr->js_maxjobs)  {
			printf("****Off end of job list in free chain, job slot %ld\n", (long) jind);
			goto  baderr;
		}

		if  (bitmap_isset(jbm.fbitmap, jind))  {
			printf("****Loop in free chain, job slot %ld\n", (long) jind);
			goto  baderr;
		}
		bitmap_set(jbm.fbitmap, jind);
		if  (bitmap_isset(jbm.jbitmap, jind))  {
			printf("****Free chain contains job %ld on job list\n", (long) jind);
			errs++;
		}

		if  (hjp->q_prv != SHM_HASHEND)  {
			printf("****Unexpected prev entry %ld in free chain job %ld\n", (long) hjp->q_prv, (long) jind);
			errs++;
		}

		if  (showfree)  {
			printf(" %6ld", (long) jind);
			cnt += 7;
			if  (cnt > 72)  {
				putchar('\n');
				cnt = 0;
			}
		}
		jind = hjp->q_nxt;
	}
	if  (cnt > 0)
		putchar('\n');

	for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
		if  (!bitmap_isset(jbm.jbitmap, cnt)  &&  !bitmap_isset(jbm.fbitmap, cnt))  {
			printf("****Job %u is orphaned\n", cnt);
			errs++;
		}

	printf("\nFollowing job number hash\n");

	for  (cnt = 0;  cnt < SHM_JHASHMOD;  cnt++)  {
		int	ccnt = 0;
		jind = Job_seg.hashp_jno[cnt];

		if  (jind == SHM_HASHEND)
			continue;

		if  (jind >= Job_seg.dptr->js_maxjobs)  {
			printf("****Off end of job list in job number hash, job slot %ld\n", (long) jind);
			goto  baderr;
		}

		printf("Jno_hash %u:\n", cnt);
		prevind = SHM_HASHEND;

		do  {
			Hashspq	*hjp = &Job_seg.jlist[jind];

			if  (jno_jhash(hjp->j.spq_job) != cnt)  {
				printf("****Incorrect hash value %u on chain for %u\n", jno_jhash(hjp->j.spq_job), cnt);
				errs++;
			}

			if  (bitmap_isset(jbm.hbitmap, jind))  {
				printf("****duplicated job %ld job number %ld on job number hash\n", (long) jind, (long) hjp->j.spq_job);
				goto  baderr;
			}

			if  (!bitmap_isset(jbm.jbitmap, jind))  {
				printf("****job %ld number %ld on job number hash but not job list\n", (long) jind, (long) hjp->j.spq_job);
				errs++;
			}
			if  (bitmap_isset(jbm.fbitmap, jind))  {
				printf("****job %ld number %ld on job number hash and free list\n", (long) jind, (long) hjp->j.spq_job);
				errs++;
			}

			bitmap_set(jbm.hbitmap, jind);

			printf("\t(%u)\t%s%ld\n", ccnt, lookhost(hjp->j.spq_netid), (long) hjp->j.spq_job);

			if  (prevind != hjp->prv_jno_hash)  {
				printf("****Previous pointer points to %ld and not %ld as expected\n", (long) hjp->prv_jno_hash, (long) prevind);
				errs++;
			}

			ccnt++;
			prevind = jind;
			jind = hjp->nxt_jno_hash;
		}  while  (jind >= 0L);
	}

	for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
		if  (bitmap_isset(jbm.jbitmap, cnt)  &&  !bitmap_isset(jbm.hbitmap, cnt))  {
			printf("****Job slot %u missing from job number hash\n", cnt);
			errs++;
		}

	printf("\nFollowing jident hash\n");
	BLOCK_ZERO(jbm.hbitmap, jbm.msize);

	for  (cnt = 0;  cnt < SHM_JHASHMOD;  cnt++)  {
		int	ccnt = 0;
		jind = Job_seg.hashp_jid[cnt];

		if  (jind == SHM_HASHEND)
			continue;

		if  (jind >= Job_seg.dptr->js_maxjobs)  {
			printf("****Off end of job list in jident hash, job slot %ld\n", (long) jind);
			goto  baderr;
		}

		printf("Jid_hash %u:\n", cnt);
		prevind = SHM_HASHEND;

		do  {
			Hashspq	*hjp = &Job_seg.jlist[jind];

			if  (jid_hash(hjp->j.spq_netid,hjp->j.spq_rslot) != cnt)  {
				printf("****Incorrect hash value %u on chain for %u\n", jid_hash(hjp->j.spq_netid,hjp->j.spq_rslot), cnt);
				errs++;
			}

			if  (bitmap_isset(jbm.hbitmap, jind))  {
				printf("****duplicated job %ld job number %ld on jident hash\n", (long) jind, (long) hjp->j.spq_job);
				goto  baderr;
			}

			if  (!bitmap_isset(jbm.jbitmap, jind))  {
				printf("****job %ld number %ld on jident hash but not job list\n", (long) jind, (long) hjp->j.spq_job);
				errs++;
			}
			if  (bitmap_isset(jbm.fbitmap, jind))  {
				printf("****job %ld number %ld on jident hash and free list\n", (long) jind, (long) hjp->j.spq_job);
				errs++;
			}

			bitmap_set(jbm.hbitmap, jind);

			printf("\t(%u)\t%s%ld\n", ccnt, lookhost(hjp->j.spq_netid), (long) hjp->j.spq_job);

			if  (prevind != hjp->prv_jid_hash)  {
				printf("****Previous pointer points to %ld and not %ld as expected\n", (long) hjp->prv_jid_hash, (long) prevind);
				errs++;
			}

			ccnt++;
			prevind = jind;
			jind = hjp->nxt_jid_hash;
		}  while  (jind != SHM_HASHEND);
	}

	for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
		if  (bitmap_isset(jbm.jbitmap, cnt)  &&  !bitmap_isset(jbm.hbitmap, cnt))  {
			printf("****Job slot %u missing from jident hash\n", cnt);
			errs++;
		}

	if  (errs > 0)
		printf("********Found %u errors in job shm********\n", errs);

 baderr:
	jbitmap_free(&jbm);
	if  (dumphex)  {
		segdump((unsigned char *) Job_seg.iinf.seg, Job_seg.iinf.segsize);
		segdump((unsigned char *) Job_seg.dinf.seg, Job_seg.dinf.segsize);
	}
	if  (dipc)
		del_jshm(typ1, typ2);
}

int  ptrshminit1(struct shmid_ds *sp, struct stat *fsb)
{
#ifdef	HAVE_SYS_MMAN_H

	/* First try to find a memory-mapped file*/

	int  i = open_mmfile(PMMAP_FILE, fsb);

	if  (i >= 0)  {
		void	*buffer;
		Ptr_seg.inf.mmfd = i;
		Ptr_seg.inf.reqsize = Ptr_seg.inf.segsize = fsb->st_size;
		if  ((buffer = mmap(0, fsb->st_size, PROT_READ, MAP_SHARED, i, 0)) == MAP_FAILED)  {
			fprintf(stderr, "Found ptr file %s but could not attach it\n", PMMAP_FILE);
			close(i);
			return  0;
		}
		Ptr_seg.inf.seg = buffer;
		Ptr_seg.dptr = (struct pshm_hdr *) buffer;
		Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;
		Ptr_seg.Last_ser = 0;
		Ptr_seg.hashp_pid = (LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
		Ptr_seg.plist = (Hashspptr *) ((char *) Ptr_seg.hashp_pid + SHM_PHASHMOD * sizeof(LONG));
		return  -1;
	}
#endif
	/* We set "base" in jobshminit1 (perhaps) */
	if  (Ptr_seg.inf.base == 0)  {
		int	i;

		for  (i = 0;  i < MAXSHMS;  i += SHMINC)  {
			Ptr_seg.inf.base = SHMID + i + PSHMOFF;
			if  ((Ptr_seg.inf.chan = shmget((key_t) Ptr_seg.inf.base, 0, 0)) < 0  ||
			     shmctl(Ptr_seg.inf.chan, IPC_STAT, sp) < 0)
				continue;
			if  ((Ptr_seg.inf.seg = shmat(Ptr_seg.inf.chan, (char *) 0, SHM_RDONLY)) != (char *) -1)
				break;
		}
		return  0;
	}
	else  {
		if  ((Ptr_seg.inf.chan = shmget((key_t) Ptr_seg.inf.base, 0, 0)) < 0)
			return  0;
		if  (shmctl(Ptr_seg.inf.chan, IPC_STAT, sp) < 0)
			return  0;
		if  ((Ptr_seg.inf.seg = shmat(Ptr_seg.inf.chan, (char *) 0, 0)) == (char *) -1)
			return  0;
	}
	Ptr_seg.dptr = (struct pshm_hdr *) Ptr_seg.inf.seg;
	Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;
	Ptr_seg.inf.segsize = Ptr_seg.inf.reqsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Ptr_seg.Nptrs * sizeof(Hashspptr);
	Ptr_seg.Last_ser = 0;
	Ptr_seg.hashp_pid = (LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
	Ptr_seg.plist = (Hashspptr *) ((char *) Ptr_seg.hashp_pid + SHM_PHASHMOD * sizeof(LONG));
	return  1;
}

void  del_pshm(const int typ)
{
#ifdef	HAVE_SYS_MMAN_H
	if  (typ > 0)
#endif
		shmctl(Ptr_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef	HAVE_SYS_MMAN_H
	else  {
		munmap(Ptr_seg.inf.seg, Ptr_seg.inf.segsize);
		close(Ptr_seg.inf.mmfd);
		unlink(mmfile_name(PMMAP_FILE));
	}
#endif
}

void  dump_ptrshm()
{
	int		typ;
	unsigned	msize, cnt, errs = 0;
	LONG		pind, prevind;
	ULONG		*pbitmap, *fbitmap, *hbitmap;
	struct	shmid_ds	sbuf;
	struct	stat		fsbuf;

	if  ((typ = ptrshminit1(&sbuf, &fsbuf)) == 0)  {
		printf("Cannot open ptr shm\n");
		return;
	}

#ifdef	HAVE_SYS_MMAN_H
	if  (typ < 0)  {
		dump_lock(Ptr_seg.inf.mmfd, "Printer");
		mmaphdr(&fsbuf, "Ptr mmap file");
	}
	else
#endif
		shmhdr(Ptr_seg.inf.base, Ptr_seg.inf.chan, &sbuf, "Printer shared memory");

	printf("Header info:\n\nnptrs:\t%u\tmaxptrs:\t%u\n", Ptr_seg.dptr->ps_nptrs, Ptr_seg.dptr->ps_maxptrs);
	dotime(Ptr_seg.dptr->ps_lastwrite, "Last write");
	printf("Free chain %ld\n", (long) Ptr_seg.dptr->ps_freech);
	printf("Serial:\t%lu\n\nList: Head %ld Tail %ld\n\n", (unsigned long) Ptr_seg.dptr->ps_serial, (long) Ptr_seg.dptr->ps_l_head, (long) Ptr_seg.dptr->ps_l_tail);

	if  (dumpall)  {
		for  (cnt = 0;  cnt < Ptr_seg.dptr->ps_maxptrs;  cnt++)  {
			Hashspptr *hpp = &Ptr_seg.plist[cnt];
			printf("\n%u:\tNxt %ld Prv %ld\n", cnt, (long) hpp->l_nxt, (long) hpp->l_prv);
			if  (hpp->p.spp_state == SPP_NULL)
				printf("NULL\n");
			else
				dumpptr(&hpp->p);
		}
		putchar('\n');
	}

	msize = ((Ptr_seg.dptr->ps_maxptrs + 31) >> 5) << 2;
	pbitmap = (ULONG *) malloc(msize);
	fbitmap = (ULONG *) malloc(msize);
	hbitmap = (ULONG *) malloc(msize);

	if  (!(pbitmap  &&  fbitmap  &&  hbitmap))  {
		fprintf(stderr, "Sorry - cannot allocate bitmaps for ptrs\n");
		if  (dipc)
			del_pshm(typ);
		return;
	}

	printf("Following list\n");
	BLOCK_ZERO(pbitmap, msize);

	pind = Ptr_seg.dptr->ps_l_head;
	prevind = SHM_HASHEND;

	while  (pind >= 0L)  {
		Hashspptr	*hpp = &Ptr_seg.plist[pind];

		if  (pind >= Ptr_seg.dptr->ps_maxptrs)  {
			printf("****Off end of ptr list, ptr slot %ld\n", (long) pind);
			goto  baderr;
		}

		if  (bitmap_isset(pbitmap, pind))  {
			printf("****Loop in ptr list, ptr slot %ld\n", (long) pind);
			goto  baderr;
		}
		bitmap_set(pbitmap, pind);

		printf("%ld:\t%s%s\n", (long) pind, lookhost(hpp->p.spp_netid), hpp->p.spp_ptr);

		if  (hpp->p.spp_netid == 0  &&  pind != hpp->p.spp_rslot)  {
			printf("rslot is %ld not %ld\n", (long) hpp->p.spp_rslot, (long) pind);
			errs++;
		}

		if  (prevind != hpp->l_prv)  {
			printf("Previous pointer points to %ld and not %ld as expected\n", (long) hpp->l_prv, (long) prevind);
			errs++;
		}

		prevind = pind;
		pind = hpp->l_nxt;
	}

	printf("Following free chain\n");
	BLOCK_ZERO(fbitmap, msize);

	pind = Ptr_seg.dptr->ps_freech;
	cnt = 0;
	while  (pind >= 0L)  {
		Hashspptr	*hpp = &Ptr_seg.plist[pind];

		if  (pind >= Ptr_seg.dptr->ps_maxptrs)  {
			printf("****Off end of ptr list in free chain, ptr number %ld\n", (long) pind);
			goto  baderr;
		}

		if  (bitmap_isset(fbitmap, pind))  {
			printf("****Loop in free chain, ptr number %ld\n", (long) pind);
			goto  baderr;
		}
		bitmap_set(fbitmap, pind);
		if  (bitmap_isset(pbitmap, pind))  {
			printf("****Free chain contains ptr %ld on ptr list\n", (long) pind);
			errs++;
		}

		if  (hpp->l_prv >= 0L)  {
			printf("****Unexpected prev entry %ld in free chain ptr %ld\n", (long) hpp->l_prv, (long) pind);
			errs++;
		}

		if  (showfree)  {
			printf(" %6ld", (long) pind);
			cnt += 7;
			if  (cnt > 72)  {
				putchar('\n');
				cnt = 0;
			}
		}
		pind = hpp->l_nxt;
	}
	if  (cnt > 0)
		putchar('\n');

	for  (cnt = 0;  cnt < Ptr_seg.dptr->ps_maxptrs;  cnt++)
		if  (!bitmap_isset(pbitmap, cnt)  &&  !bitmap_isset(fbitmap, cnt))  {
			printf("****Ptr %u is orphaned\n", cnt);
			errs++;
		}

	printf("\nFollowing pident hash\n");
	BLOCK_ZERO(hbitmap, msize);

	for  (cnt = 0;  cnt < SHM_PHASHMOD;  cnt++)  {
		int	ccnt = 0;
		pind = Ptr_seg.hashp_pid[cnt];

		if  (pind < 0L)
			continue;

		if  (pind >= Ptr_seg.dptr->ps_maxptrs)  {
			printf("****Off end of ptr list, ptr slot %ld\n", (long) pind);
			goto  baderr;
		}

		printf("Pid_hash %u:\n", cnt);
		prevind = SHM_HASHEND;

		do  {
			Hashspptr  *hpp = &Ptr_seg.plist[pind];

			if  (pid_hash(hpp->p.spp_netid, hpp->p.spp_rslot) != cnt)  {
				printf("****Incorrect hash value %u on chain for %u\n", pid_hash(hpp->p.spp_netid, hpp->p.spp_rslot), cnt);
				errs++;
			}

			if  (bitmap_isset(hbitmap, pind))  {
				printf("****duplicated ptr slot %ld ptr %s on pident hash\n", (long) pind, hpp->p.spp_ptr);
				goto  baderr;
			}

			if  (!bitmap_isset(pbitmap, pind))  {
				printf("****ptrslot %ld %s on pident hash but not ptr list\n", (long) pind, hpp->p.spp_ptr);
				errs++;
			}
			if  (bitmap_isset(fbitmap, pind))  {
				printf("****ptrslot %ld %s on pident hash and free list\n", (long) pind, hpp->p.spp_ptr);
				errs++;
			}

			bitmap_set(hbitmap, pind);

			printf("\t(%u)\t%s%s\n", ccnt, lookhost(hpp->p.spp_netid), hpp->p.spp_ptr);

			if  (prevind != hpp->prv_pid_hash)  {
				printf("****Previous pointer points to %ld and not %ld as expected\n", (long) hpp->prv_pid_hash, (long) prevind);
				errs++;
			}

			ccnt++;
			prevind = pind;
			pind = hpp->nxt_pid_hash;
		}  while  (pind >= 0L);
	}

	for  (cnt = 0;  cnt < Ptr_seg.dptr->ps_maxptrs;  cnt++)
		if  (bitmap_isset(pbitmap, cnt)  &&  !bitmap_isset(hbitmap, cnt))  {
			printf("****Ptr slot %u missing from pident hash\n", cnt);
			errs++;
		}

	if  (errs > 0)
		printf("********Found %u errors in ptr shm********\n", errs);

 baderr:
	free((char *) hbitmap);
	free((char *) fbitmap);
	free((char *) pbitmap);

	if  (dumphex)
		segdump((unsigned char *) Ptr_seg.inf.seg, Ptr_seg.inf.segsize);
	if  (dipc)
		del_pshm(typ);
}

void  dump_xfershm()
{
	int			xfer_chan;
	unsigned		segsize = 0;
	unsigned		cnt, pos;
	char			*xret;
	struct	xfershm		*Xfer_shmp;
	struct	shmid_ds	sbuf;
#ifdef	HAVE_SYS_MMAN_H
	int			typ = 0;
	struct	stat		fsbuf;


	if  ((xfer_chan = open_mmfile(XFMMAP_FILE, &fsbuf)) >= 0)  {
		segsize = fsbuf.st_size;
		if  ((xret = mmap(0, segsize, PROT_READ, MAP_SHARED, xfer_chan, 0)) == MAP_FAILED)  {
			fprintf(stderr, "Found xfer buf file %s but could not attach it\n", XFMMAP_FILE);
			close(xfer_chan);
			return;
		}
		mmaphdr(&fsbuf, "Xfer buffer mmap");
		dump_lock(xfer_chan, "Xfer buffer");
		typ = -1;
	}
	else   {
#endif
		if  ((xfer_chan = shmget((key_t) XSHMID, 0, 0)) < 0  ||
		     shmctl(xfer_chan, IPC_STAT, &sbuf) < 0 ||
		     (xret = shmat(xfer_chan, (char *) 0, 0)) == (char *) -1)  {
			printf("Could not open xfer shm\n\n");
			return;
		}
		segsize = sbuf.shm_segsz;
		shmhdr(XSHMID, xfer_chan, &sbuf, "Xfer buffer shm");
#ifdef	HAVE_SYS_MMAN_H
		typ = 1;
	}
#endif

	Xfer_shmp = (struct xfershm *) xret;
	printf("Xfer number on queue:\t%u\tHead:\t%u\tTail:\t%u\n",
	       Xfer_shmp->xf_nonq, Xfer_shmp->xf_head, Xfer_shmp->xf_tail);
	pos = Xfer_shmp->xf_head;
	for  (cnt = 0;  cnt < Xfer_shmp->xf_nonq;  cnt++)  {
		printf("Queue:\t%u\tPos:\t%u\tPid:\t%ld\n", cnt, pos, (long) Xfer_shmp->xf_queue[pos].jorp_sender);
		pos = (pos + 1) % (TRANSBUF_NUM + 1);
	}
	putchar('\n');
	if  (dumphex)
		segdump((unsigned char *) xret, segsize);
	if  (dipc)  {
#ifdef	HAVE_SYS_MMAN_H
		if  (typ >= 0)
#endif
			shmctl(xfer_chan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef	HAVE_SYS_MMAN_H
		else  {
			munmap(xret, segsize);
			close(xfer_chan);
			unlink(mmfile_name(XFMMAP_FILE));
		}
#endif
	}
}

void  dump_sem(int semid, int semnum, char *descr)
{
	union	my_semun	zz;
	zz.val = 0;
	printf("%s:\t%d\t%d\t%d\t%d\n",
	       descr,
	       semctl(semid, semnum, GETVAL, zz),
	       semctl(semid, semnum, GETNCNT, zz),
	       semctl(semid, semnum, GETZCNT, zz),
	       semctl(semid, semnum, GETPID, zz));
}

void  dump_sema4()
{
	int			semid;
	struct	semid_ds	sbuf;
	union	my_semun	zz;

	zz.buf = &sbuf;

	if  ((semid = semget(SEMID, SEMNUMS, 0600)) < 0)  {
		printf("Cannot open semaphore (%d)\n", errno);
		return;
	}

	if  (semctl(semid, 0, IPC_STAT, zz) < 0)  {
		printf("Cannot stat semaphore (%d)\n", errno);
		return;
	}

	if  (sbuf.sem_nsems != SEMNUMS)  {
		printf("Confused about number of semaphores expected %d found %ld\n", SEMNUMS, (long) sbuf.sem_nsems);
		return;
	}

	printf("Semaphore 0x%.8x id %d\n", SEMID, semid);
	dump_ipcperm(&sbuf.sem_perm);
	dotime(sbuf.sem_otime, "Op time");
	dotime(sbuf.sem_ctime, "Change time");

	printf("Sem:\t\t\tNcnt\tZcnt\tPid\n");
	dump_sem(semid, JQ_FIDDLE, "Job Fiddle");
	dump_sem(semid, JQ_READING, "Job Read");
	dump_sem(semid, PQ_FIDDLE, "Ptr Fiddle");
	dump_sem(semid, PQ_READING, "Ptr Read");
	dump_sem(semid, XT_LOCK, "Xfer Lock");
	putchar('\n');

	if  (dipc)
		semctl(semid, 0, IPC_RMID, zz);
}

void  dump_q(int rq)
{
	int			msgid;
	struct	msqid_ds	sbuf;

	if  ((msgid = msgget(MSGID, 0)) < 0)  {
		printf("No message queue\n");
		return;
	}

	if  (msgctl(msgid, IPC_STAT, &sbuf) < 0)  {
		printf("Cannot find message state\n");
		return;
	}

	printf("Message queue key 0x%.8x id %d\n", MSGID, msgid);
	dump_ipcperm(&sbuf.msg_perm);
	printf("Number of bytes %lu messages %lu max bytes %lu\n",
	       (unsigned long) sbuf.msg_cbytes, (unsigned long) sbuf.msg_qnum, (unsigned long) sbuf.msg_qbytes);
	printf("Last sender %ld receiver %ld\n", (long) sbuf.msg_lspid, (long) sbuf.msg_lrpid);
	dotime(sbuf.msg_stime, "Send time");
	dotime(sbuf.msg_rtime, "Receive time");
	dotime(sbuf.msg_ctime, "Change time");

	if  (rq)  {
		int	bytes;
		struct	spr_req	*sr;
		char	buf[2048+sizeof(long)];

		if  ((bytes = msgrcv(msgid, (struct msgbuf *) buf, 2048, 0, IPC_NOWAIT)) < 0)
			printf("Nothing readable\n");
		else  {
			unsigned  count = 1;
			do  {
				printf("Message %d, %d bytes:", count, bytes);
				sr = (struct spr_req *) buf;
				if  (sr->spr_mtype == MT_SCHED)
					printf(" TO SCHEDULER\t");
				else
					printf(" To spd, pid=%ld\t", (long) (sr->spr_mtype - MT_PMSG));
				switch  (sr->spr_un.o.spr_act)  {
				default:
					printf(" Unknown message type %d\n", sr->spr_un.o.spr_act);
					break;
				case  SJ_ENQ:
					printf("Queue job\n");
					break;
				case  SJ_CHNG:
					printf("Change job\n");
					break;
				case  SJ_CHANGEDJOB:
					printf("Remote job change\n");
					break;
				case  SJ_JUNASSIGN:
					printf("Job Unassign\n");
					break;
				case  SJ_JUNASSIGNED:
					printf("Remote job unassigned\n");
					break;
				case  SP_ADDP:
					printf("Add ptr\n");
					break;
				case  SP_CHGP:
					printf("Change ptr\n");
					break;
				case  SP_CHANGEDPTR:
					printf("Remote ptr changed\n");
					break;
				case  SP_PUNASSIGNED:
					printf("Ptr unassigned\n");
					break;
				case  SP_REQ:
					printf("Request to ptr\n");
					break;
				case  SP_FIN:
					printf("Spd terminated\n");
					break;
				case  SP_PAB:
					printf("Abort spd\n");
					break;
				case  SP_PYES:
					printf("A/w yes\n");
					break;
				case  SP_PNO:
					printf("A/w no\n");
					break;
				case  SP_REMAP:
					printf("Remap world\n");
					break;
				case  SPD_DONE:
					printf("Completed job\n");
					break;
				case  SPD_DAB:
					printf("Done abort\n");
					break;
				case  SPD_DERR:
					printf("Done error\n");
					break;
				case  SPD_DFIN:
					printf("Spd finished\n");
					break;
				case  SPD_SCH:
					printf("Spd state change\n");
					break;
				case  SPD_CHARGE:
					printf("Charge record\n");
					break;
				case  SO_AB:
					printf("Abort job\n");
					break;
				case  SO_ABNN:
					printf("Abort no notify\n");
					break;
				case  SO_DEQUEUED:
					printf("Broadcast delete\n");
					break;
				case  SO_MON:
					printf("Mon on\n");
					break;
				case  SO_DMON:
					printf("Mon off\n");
					break;
				case  SO_RSP:
					printf("Restart printer\n");
					break;
				case  SO_PHLT:
					printf("Halt EOJ\n");
					break;
				case  SO_PSTP:
					printf("Halt now\n");
					break;
				case  SO_PGO:
					printf("Start printer\n");
					break;
				case  SO_DELP:
					printf("Delete printer\n");
					break;
				case  SO_SSTOP:
					printf("Stop scheduler\n");
					break;
				case  SO_OYES:
					printf("Send A/w yes\n");
					break;
				case  SO_ONO:
					printf("Send A/w no\n");
					break;
				case  SO_INTER:
					printf("Interrupt ptr\n");
					break;
				case  SO_PJAB:
					printf("Ptr abort\n");
					break;
				case  SO_NOTIFY:
					printf("Notify remote\n");
					break;
				case  SO_PNOTIFY:
					printf("Notify remote past\n");
					break;
				case  SO_PROPOSE:
					printf("Propose\n");
					break;
				case  SO_PROP_OK:
					printf("OK Prop\n");
					break;
				case  SO_PROP_NOK:
					printf("Not OK prop\n");
					break;
				case  SO_PROP_DEL:
					printf("Deleted prop\n");
					break;
				case  SO_ASSIGN:
					printf("Assign job\n");
					break;
				case  SO_LOCASSIGN:
					printf("Local assign\n");
					break;
				case  SN_NEWHOST:
					printf("New Host\n");
					break;
				case  SN_SHUTHOST:
					printf("Shutdown host\n");
					break;
				case  SN_ABORTHOST:
					printf("Abort Host\n");
					break;
				case  SN_DELERR:
					printf("Delete error file\n");
					break;
				case  SN_REQSYNC:
					printf("Request syn\n");
					break;
				case  SN_ENDSYNC:
					printf("End sync\n");
					break;
				case  SN_TICKLE:
					printf("Tickle\n");
					break;
				case  SON_CONNECT:
					printf("Connect\n");
					break;
				case  SON_DISCONNECT:
					printf("Disconnect\n");
					break;
				case  SON_CONNOK:
					printf("Connect OK\n");
					break;
				case  SON_XTNATT:
					printf("Attach xtnetserv\n");
					break;
				case  SOU_PWCHANGED:
					printf("Password changed\n");
					break;
				}
				count++;
			}  while  ((bytes = msgrcv(msgid, (struct msgbuf *) buf, 2048, 0, IPC_NOWAIT)) >= 0);
		}
	}
	if  (dipc)
		msgctl(msgid, IPC_RMID, (struct msqid_ds *) 0);
}

MAINFN_TYPE  main(int argc, char **argv)
{
	int	c, hadsd = 0;
	int	rq = 0;
	extern	char	*optarg;

	versionprint(argv, "$Revision: 1.1 $", 0);

	while  ((c = getopt(argc, argv, "rdFAD:P:o:nxabB:N:S:G")) != EOF)
		switch  (c)  {
		default:
		usage:
			fprintf(stderr, "Usage: %s [-r] [-d] [-F] [-A] [-x] [-a] [-b] [-B n] [-N ch] [-S dir ] [-D secs] [-P psarg] [-G ] [-n] [-o outfile]\n", argv[0]);
			return  1;
		case  'r':
			rq++;
			continue;
		case  'd':
			dipc = 1;
			continue;
		case  'F':
			showfree = 1;
			continue;
		case  'A':
			dumpall = 1;
			continue;
		case  'S':
			hadsd++;
			spooldir = optarg;
			continue;
		case  'D':
			daemtime = atoi(optarg);
			continue;
		case  'P':
			psarg = optarg;
			continue;
		case  'G':
			nofgrep = 1;
			continue;
		case  'o':
			if  (!freopen(optarg, "a", stdout))  {
				fprintf(stderr, "Cannot output to %s\n", optarg);
				return  2;
			}
			continue;
		case  'n':
			notifok = 1;
			continue;
		case  'x':
			dumphex = 1;
			continue;
		case  'a':
			dumpok = 1;
			continue;
		case  'b':
			psok = 1;
			continue;
		case  'B':
			blksize = atoi(optarg);
			if  (blksize == 0  ||  blksize > MAX_NWORDS)
				goto  usage;
			continue;
		case  'N':
			npchar = optarg[0];
			continue;
		}

	blksize *= sizeof(ULONG);

#ifdef	HAVE_SYS_MMAN_H

	/* Quick and dirty interpretation of /etc/gnuspool-config
	   We need this for memory-mapped files */

	if  (!hadsd)  {
		FILE  *cf = fopen(MASTER_CONFIG, "r");
		if  (cf)  {
			char	inbuf[200];
			while  (fgets(inbuf, 200, cf))  {
				if  (strncmp(inbuf, "SPOOLDIR", 8) == 0  &&  (inbuf[8] == '=' || inbuf[8] == ':'))  {
					int  ln = strlen(inbuf) - 1;
					char	*nd;
					if  (inbuf[ln] == '\n')
						inbuf[ln--] = '\0';
					if  (inbuf[9] != '/')
						break;
					if  (!(nd = malloc((unsigned) ln - 8)))  {
						fprintf(stderr, "Out of memory for directory\n");
						return  255;
					}
					strcpy(nd, &inbuf[9]);
					spooldir = nd;
					break;
				}
			}
			fclose(cf);
		}
	}
#endif

	dump_jobshm();
	dump_ptrshm();
	dump_xfershm();

	dump_sema4();

	dump_q(rq);

	return  0;
}
