/* sh_netfeed.c -- spshed pass over spool files

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
#ifdef	NETWORK_VERSION
#include "incl_sig.h"
#include <sys/types.h>
#include <sys/ipc.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef	USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "ipcstuff.h"
#define	UCONST
#include "q_shm.h"
#include "files.h"
#include "errnums.h"
#include "incl_unix.h"

#define	INITPAGES	20
#define	INCPAGES	10

unsigned	pagenums;
LONG		*pageoffsets;

#ifdef	USING_FLOCK
extern void	setjhold(const int);
extern void	setphold(const int);

#define	JLOCK		setjhold(F_RDLCK)
#define	JUNLOCK		setjhold(F_UNLCK)
#define	PTRS_LOCK	setphold(F_RDLCK)
#define	PTRS_UNLOCK	setphold(F_UNLCK)

#else
static	struct	sembuf	rjr[2]	= {{ JQ_FIDDLE,		0,	0 },
				   { JQ_READING,	1,	0 }},
			rju[1]  = {{ JQ_READING,	-1,	0 }},
			rpr[2]	= {{ PQ_FIDDLE,		0,	0 },
				   { PQ_READING,	1,	0 }},
			rpu[1]  = {{ PQ_READING,	-1,	0 }};

#define	SEM_OP(buf, num)	while  (semop(Sem_chan, buf, num) < 0  &&  errno == EINTR)
#define	JLOCK		SEM_OP(rjr, 2)
#define	JUNLOCK		SEM_OP(rju, 1)
#define	PTRS_LOCK	SEM_OP(rpr, 2)
#define	PTRS_UNLOCK	SEM_OP(rpu, 1)
#endif

extern	SHORT	viewsock;

void	report(const int);
void	nfreport(const int);

/* Pack up a job ready for its journey into the unknown... */

void	job_pack(struct spq *to, struct spq *from)
{
	to->spq_job = htonl((ULONG) from->spq_job);
	to->spq_netid = 0L;
	to->spq_orighost = from->spq_orighost == 0? myhostid: from->spq_orighost;
	to->spq_rslot = htonl((ULONG) from->spq_rslot);
	to->spq_time = htonl((ULONG) from->spq_time);
	to->spq_proptime = 0L;
	to->spq_starttime = htonl((ULONG) from->spq_starttime);
	to->spq_hold = htonl((ULONG) from->spq_hold);
	to->spq_nptimeout = htons(from->spq_nptimeout);
	to->spq_ptimeout = htons(from->spq_ptimeout);
	to->spq_size = htonl((ULONG) from->spq_size);
	to->spq_posn = htonl((ULONG) from->spq_posn);
	to->spq_pagec = htonl((ULONG) from->spq_pagec);
	to->spq_npages = htonl((ULONG) from->spq_npages);

	to->spq_cps = from->spq_cps;
	to->spq_pri = from->spq_pri;
	to->spq_wpri = htons((USHORT) from->spq_wpri);

	to->spq_jflags = htons(from->spq_jflags);
	to->spq_sflags = from->spq_sflags;
	to->spq_dflags = from->spq_dflags;

	to->spq_extrn = htons(from->spq_extrn);
	to->spq_pglim = 0;			/* Not used any more */

	to->spq_class = htonl(from->spq_class);

	to->spq_pslot = htonl(-1L);

	to->spq_start = htonl((ULONG) from->spq_start);
	to->spq_end = htonl((ULONG) from->spq_end);
	to->spq_haltat = htonl((ULONG) from->spq_haltat);
	to->spq_uid = htonl(from->spq_uid);

	strncpy(to->spq_uname, from->spq_uname, UIDSIZE+1);
	strncpy(to->spq_puname, from->spq_puname, UIDSIZE+1);
	strncpy(to->spq_file, from->spq_file, MAXTITLE+1);
	strncpy(to->spq_form, from->spq_form, MAXFORM+1);
	strncpy(to->spq_ptr, from->spq_ptr, JPTRNAMESIZE+1);
	strncpy(to->spq_flags, from->spq_flags, MAXFLAGS+1);
}

/* Unpack a job and see what British Hairyways has broken this time */

void	unpack_job(struct spq *to, struct spq *from)
{
	to->spq_job = ntohl((ULONG) from->spq_job);
	to->spq_netid = 0L;
	to->spq_orighost = from->spq_orighost == myhostid? 0: from->spq_orighost;
	to->spq_rslot = ntohl((ULONG) from->spq_rslot);
	to->spq_time = ntohl((ULONG) from->spq_time);
	to->spq_proptime = 0L;
	to->spq_starttime = ntohl((ULONG) from->spq_starttime);
	to->spq_hold = ntohl((ULONG) from->spq_hold);
	to->spq_nptimeout = ntohs(from->spq_nptimeout);
	to->spq_ptimeout = ntohs(from->spq_ptimeout);
	to->spq_size = ntohl((ULONG) from->spq_size);
	to->spq_posn = ntohl((ULONG) from->spq_posn);
	to->spq_pagec = ntohl((ULONG) from->spq_pagec);
	to->spq_npages = ntohl((ULONG) from->spq_npages);

	to->spq_cps = from->spq_cps;
	to->spq_pri = from->spq_pri;
	to->spq_wpri = ntohs((USHORT) from->spq_wpri);
	to->spq_jflags = ntohs(from->spq_jflags);
	to->spq_sflags = from->spq_sflags;
	to->spq_dflags = from->spq_dflags;

	to->spq_extrn = ntohs((USHORT) from->spq_extrn);
	to->spq_pglim = 0;	/* Not used any more */

	to->spq_class = ntohl(from->spq_class);

	to->spq_pslot = ntohl(-1L);

	to->spq_start = ntohl((ULONG) from->spq_start);
	to->spq_end = ntohl((ULONG) from->spq_end);
	to->spq_haltat = ntohl((ULONG) from->spq_haltat);

	to->spq_uid = ntohl(from->spq_uid);

	strncpy(to->spq_uname, from->spq_uname, UIDSIZE+1);
	strncpy(to->spq_puname, from->spq_puname, UIDSIZE+1);
	strncpy(to->spq_file, from->spq_file, MAXTITLE+1);
	strncpy(to->spq_form, from->spq_form, MAXFORM+1);
	strncpy(to->spq_ptr, from->spq_ptr, JPTRNAMESIZE+1);
	strncpy(to->spq_flags, from->spq_flags, MAXFLAGS+1);
}

/* Ditto for printers */

void	ptr_pack(struct spptr *to, struct spptr *from)
{
	to->spp_netid = from->spp_netid;
	to->spp_rslot = htonl((ULONG) from->spp_rslot);

	to->spp_pid = htonl((ULONG) from->spp_pid);
	to->spp_job = htonl((ULONG) from->spp_job);
	to->spp_rjhostid = from->spp_rjhostid == 0? myhostid: from->spp_rjhostid;
	to->spp_rjslot = htonl(from->spp_rjslot);
	to->spp_jslot = htonl((ULONG) from->spp_jslot);

	to->spp_state = from->spp_state;
	to->spp_sflags = from->spp_sflags;
	to->spp_dflags = from->spp_dflags;
	to->spp_netflags = from->spp_netflags;

	to->spp_class = htonl(from->spp_class);
	to->spp_minsize = htonl((ULONG) from->spp_minsize);
	to->spp_maxsize = htonl((ULONG) from->spp_maxsize);

	to->spp_extrn = htons(from->spp_extrn);
	to->spp_resvd = 0;

	strncpy(to->spp_dev, from->spp_dev, LINESIZE+1);
	strncpy(to->spp_form, from->spp_form, MAXFORM+1);
	strncpy(to->spp_ptr, from->spp_ptr, PTRNAMESIZE+1);
	strncpy(to->spp_feedback, from->spp_feedback, PFEEDBACK+1);
	strncpy(to->spp_comment, from->spp_comment, COMMENTSIZE+1);
}

/* And unpack again... */

void	unpack_ptr(struct spptr *to, struct spptr *from)
{
	to->spp_netid = from->spp_netid;
	to->spp_rslot = ntohl((ULONG) from->spp_rslot);

	to->spp_pid = ntohl((ULONG) from->spp_pid);
	to->spp_job = ntohl((ULONG) from->spp_job);
	to->spp_rjhostid = from->spp_rjhostid == myhostid? 0: from->spp_rjhostid;
	to->spp_rjslot = ntohl((ULONG) from->spp_rjslot);
	to->spp_jslot = ntohl((ULONG) from->spp_jslot);

	to->spp_state = from->spp_state;
	to->spp_sflags = from->spp_sflags;
	to->spp_dflags = from->spp_dflags;
	to->spp_netflags = from->spp_netflags;

	to->spp_class = ntohl(from->spp_class);

	to->spp_minsize = ntohl((ULONG) from->spp_minsize);
	to->spp_maxsize = ntohl((ULONG) from->spp_maxsize);

	to->spp_extrn = ntohs(from->spp_extrn);
	to->spp_resvd = 0;

	strncpy(to->spp_dev, from->spp_dev, LINESIZE+1);
	strncpy(to->spp_form, from->spp_form, MAXFORM+1);
	strncpy(to->spp_ptr, from->spp_ptr, PTRNAMESIZE+1);
	strncpy(to->spp_feedback, from->spp_feedback, PFEEDBACK+1);
	strncpy(to->spp_comment, from->spp_comment, COMMENTSIZE+1);
}

/* If the jobs segment has moved, find where it went. */

void	check_jmoved(void)
{
#ifdef	USING_MMAP
	if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)  {
		munmap(Job_seg.dinf.seg, Job_seg.dinf.segsize);
		Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.dptr->js_did;
		Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
		if  ((Job_seg.dinf.seg = mmap(0, Job_seg.dinf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, Job_seg.dinf.mmfd, 0)) == MAP_FAILED)
			report($E{Network process jshm error});
		Job_seg.jlist = (Hashspq *) Job_seg.dinf.seg;
	}
#else
	if  (Job_seg.dinf.base != Job_seg.dptr->js_did)  {
		shmdt((char *) Job_seg.dinf.seg);	/*  Lose old one  */
		Job_seg.dinf.base = Job_seg.dptr->js_did;
		Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
		Job_seg.dinf.segsize = Job_seg.dinf.reqsize = Job_seg.Njobs * sizeof(Hashspq);
		if  ((Job_seg.dinf.chan = shmget((key_t) Job_seg.dinf.base, 0, 0)) <= 0  ||
		     (Job_seg.dinf.seg = shmat(Job_seg.dinf.chan, (char *) 0, 0)) == (char *) -1)
			report($E{Network process jshm error});
		Job_seg.jlist = (Hashspq *) Job_seg.dinf.seg;
	}
#endif
}

void	check_pmoved(void)
{
#ifdef	USING_MMAP
	if  (Job_seg.dptr->js_psegid != Ptr_seg.inf.segsize)
#else
	if  (Job_seg.dptr->js_psegid != Ptr_seg.inf.base)
#endif
	{
#ifdef	USING_MMAP
		munmap(Ptr_seg.inf.seg, Ptr_seg.inf.segsize);
		Ptr_seg.inf.segsize = Ptr_seg.inf.reqsize = Job_seg.dptr->js_psegid;
		if  ((Ptr_seg.inf.seg = mmap(0, Ptr_seg.inf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, Ptr_seg.inf.mmfd, 0)) == MAP_FAILED)
#else
		shmdt(Ptr_seg.inf.seg);
		Ptr_seg.inf.base = Job_seg.dptr->js_psegid;
		if  ((Ptr_seg.inf.chan = shmget((key_t) Ptr_seg.inf.base, 0, 0)) < 0  ||
		     (Ptr_seg.inf.seg = shmat(Ptr_seg.inf.chan, (char *) 0, 0)) == (char *) -1)
#endif
			report($E{Network process pshm error});
		Ptr_seg.dptr = (struct pshm_hdr *) Ptr_seg.inf.seg;
		Ptr_seg.Nptrs = Ptr_seg.dptr->ps_maxptrs;
		Ptr_seg.inf.segsize = Ptr_seg.inf.reqsize = sizeof(struct pshm_hdr) + SHM_PHASHMOD * sizeof(LONG) + Ptr_seg.Nptrs * sizeof(Hashspptr);
		Ptr_seg.hashp_pid = (LONG *) (Ptr_seg.inf.seg + sizeof(struct pshm_hdr));
		Ptr_seg.plist = (Hashspptr *) ((char *) Ptr_seg.hashp_pid + SHM_PHASHMOD*sizeof(LONG));
	}
}

/* Find local job slot corresponding to remote job */

int  find_jslot(const jobno_t jobno, const netid_t hostid, const slotno_t slotn, const char *place, const unsigned op)
{
	LONG	jind;
	JLOCK;
	check_jmoved();
	jind = Job_seg.hashp_jid[jid_hash(hostid, slotn)];
	while  (jind >= 0L)  {
		Hashspq  *hjp = &Job_seg.jlist[jind];
		struct  spq  *jp = &hjp->j;
		if  (jp->spq_job == jobno  &&  jp->spq_netid == hostid  &&  jp->spq_rslot == slotn)  {
			JUNLOCK;
			return  jind;
		}
		jind = hjp->nxt_jid_hash;
	}
	JUNLOCK;
	disp_str = look_host(hostid);
	disp_str2 = place;
	disp_arg[0] = jobno;
	disp_arg[1] = slotn;
	disp_arg[2] = op;
	nfreport($E{Lost track of job});
	return  -1;
}

/* Ditto for printers */

slotno_t	find_pslot(const netid_t hostid, const slotno_t slotn)
{
	LONG  pind;

	PTRS_LOCK;
	check_pmoved();
	pind = Ptr_seg.hashp_pid[pid_hash(hostid, slotn)];
	while  (pind >= 0L)  {
		Hashspptr  *hcp = &Ptr_seg.plist[pind];
		struct  spptr	*cp = &hcp->p;
		if  (cp->spp_state != SPP_NULL  &&  cp->spp_netid == hostid  &&  cp->spp_rslot == slotn)  {
			PTRS_UNLOCK;
			return  (slotno_t) pind;
		}
		pind = hcp->nxt_pid_hash;
	}
	PTRS_UNLOCK;
	disp_str = look_host(hostid);
	disp_arg[1] = slotn;
	nfreport($E{Lost track of printer});
	return  -1;
}

/* Check that reference to job is valid and at the same time
   check that the job segment hasn't been grown (remember that this
   is run by processes forked off from the main spshed one). */

Hashspq  *ver_job(const slotno_t jslot, const jobno_t jobno)
{
	Hashspq	*result;
	check_jmoved();
	if  (jslot >= Job_seg.dptr->js_maxjobs)
		return  (Hashspq *) 0;
	result = &Job_seg.jlist[jslot];
	if  (result->j.spq_job != jobno  ||  result->j.spq_netid)
		return  (Hashspq *) 0;
	return  result;
}

/* Same sort of caper for printers */

Hashspptr *ver_ptr(const slotno_t pslot)
{
	Hashspptr	*result;
	check_pmoved();
	if  (pslot >= Ptr_seg.dptr->ps_maxptrs)
		return  (Hashspptr *) 0;
	result = &Ptr_seg.plist[pslot];
	if  (result->p.spp_state == SPP_NULL  ||  result->p.spp_netid != 0L)
		return  (Hashspptr *) 0;
	return  result;
}

Hashspptr *ver_remptr(const slotno_t pslot, const netid_t netid)
{
	slotno_t	slotn;
	if  ((slotn = find_pslot(netid, pslot)) < 0)
		return  (Hashspptr *) 0;
	return  &Ptr_seg.plist[slotn];
}

/* Read and attempt to check validity of page file 0 not ok 1 ok */

int	rdpagefile(struct spq *jp)
{
	int		pgfid, rlng;
	char		*fdelim;
	struct	pages	pfe;

	if  ((pgfid = open(mkspid(PFNAM, jp->spq_job), O_RDONLY)) < 0)
		return  0;

	if  (read(pgfid, (char *) &pfe, sizeof(struct pages)) != sizeof(struct pages))  {
		close(pgfid);
		return  0;
	}

	if  ((fdelim = (char *) malloc((unsigned) pfe.deliml)) == (char *) 0)  {
		close(pgfid);
		return  0;
	}

	/* Read delimiter itself */

	if  (read(pgfid, fdelim, (unsigned) pfe.deliml) != pfe.deliml)
		goto  badfile;

	/* Slurp up vector of page offsets */

	if  (pagenums < jp->spq_npages + 1)  {
		pagenums = jp->spq_npages + 1;
		if  ((pageoffsets = (LONG *) malloc(pagenums * sizeof(LONG))) == (LONG *) 0)
			goto  badfile;
	}

	rlng = jp->spq_npages * sizeof(LONG);

	if  (read(pgfid, (char *) &pageoffsets[1], (unsigned) rlng) != rlng)
		goto  badfile;

	pageoffsets[0] = 0L;
	free(fdelim);
	close(pgfid);
	return  1;

 badfile:
	free(fdelim);
	close(pgfid);
	return  0;
}

/* Use form feed */

int	scan_ffs(FILE *infile, struct spq *jp)
{
	int	ch;
	LONG		page_cnt;

	pagenums = INITPAGES;
	if  ((pageoffsets = (LONG *) malloc(pagenums * sizeof(LONG))) == (LONG *) 0)
		return  0;

	page_cnt = 0L;

	while  ((ch = getc(infile)) != EOF)  {
		if  (ch == DEF_DELIM)  {
			if  (++page_cnt + 1 >= pagenums)  {
				pagenums += INCPAGES;
				pageoffsets = (LONG *) realloc((char *) pageoffsets, sizeof(LONG) * pagenums);
				if  (pageoffsets == (LONG *) 0)
					return  0;
			}
			pageoffsets[page_cnt] = ftell(infile);
		}
	}

	pageoffsets[0] = 0L;
	pageoffsets[page_cnt+1] = jp->spq_size;
	if  (pageoffsets[page_cnt] < jp->spq_size)
		page_cnt++;
	jp->spq_npages = page_cnt;
	Job_seg.dptr->js_serial++;
	return  1;
}

/* Send a file down a socket and keep track of the pages */

void	sp_feed(const slotno_t jslot, const jobno_t jobno, const int sock)
{
	int	ch;
	Hashspq		*hjp;
	struct	spq	*jp;
	FILE	*infile, *outfile;
	LONG	page_cnt, char_cnt, endofpage;

	if  (!(hjp = ver_job(jslot, jobno)))
		return;
	jp = &hjp->j;

	if  ((infile = fopen(mkspid(SPNAM, jobno), "r")) == (FILE *) 0)
		return;
	if  ((outfile = fdopen(sock, "w")) == (FILE *) 0)
		return;
#ifdef	SETVBUF_REVERSED
	setvbuf(outfile, _IOFBF, (char *) 0, BUFSIZ);
#else
	setvbuf(outfile, (char *) 0, _IOFBF, BUFSIZ);
#endif

	jp->spq_dflags |= SPQ_PQ;

	if  (!rdpagefile(jp))  {
		if  (!scan_ffs(infile, jp))  {
			jp->spq_npages = 1;
			Job_seg.dptr->js_serial++;
			pageoffsets = (LONG *) malloc(2 * sizeof(LONG));
			if  (!pageoffsets)  {
				jp->spq_dflags &= ~SPQ_PQ;
				return;
			}
			pageoffsets[0] = 0l;
			pageoffsets[1] = jp->spq_size;
		}
		rewind(infile);
	}

	char_cnt = 0L;
	page_cnt = 0L;

	while  (char_cnt < jp->spq_size)  {
		endofpage = pageoffsets[page_cnt+1];
		jp->spq_pagec = page_cnt;
		Job_seg.dptr->js_serial++;

		while  (char_cnt < endofpage)  {
			if  ((ch = getc(infile)) == EOF)
				goto  badfile;
			char_cnt++;
			jp->spq_posn = char_cnt;
			Job_seg.dptr->js_serial++;
			putc(ch, outfile);
		}
		page_cnt++;
	}
	jp->spq_dflags |= SPQ_PRINTED;
 badfile:
	jp->spq_dflags &= ~SPQ_PQ;
	fclose(infile);
	fclose(outfile);
	free((char *) pageoffsets);
	return;
}

static	void	sockwrite(const int sock, char * buff, int bytes)
{
	while  (bytes > 0)  {
		int	obytes = write(sock, buff, bytes);
		if  (obytes < 0)
			return;
		buff += obytes;
		bytes -= obytes;
	}
}

#ifndef	WORDS_BIGENDIAN
/* Send a page file down the socket in cases where we have to transmogrify into net byte order. */
void	feed_pages(const jobno_t jobno, const int sock)
{
	int	ffd, lng, bytes;
	LONG	*ep;
	struct	pages	pfe;
	union	{
		char	buffer[128];
		LONG	lbuffer[128/sizeof(LONG)];
	}  un;

	if  ((ffd = open(mkspid(PFNAM, jobno), O_RDONLY)) < 0)
		return;

	if  (read(ffd, (char *) &pfe, sizeof(pfe)) != sizeof(pfe))  {
		close(ffd);
		return;
	}

	lng = pfe.deliml;
	pfe.delimnum = htonl((ULONG) pfe.delimnum);
	pfe.deliml = htonl((ULONG) pfe.deliml);
	pfe.lastpage = htonl((ULONG) pfe.lastpage);
	sockwrite(sock, (char *) &pfe, sizeof(pfe));

	/* Send delimiter down socket */

	while  (lng > 0)  {
		bytes = lng > sizeof(un.buffer)? sizeof(un.buffer): lng;
		read(ffd, un.buffer, (unsigned) bytes);
		sockwrite(sock, un.buffer, bytes);
		lng -= bytes;
	}

	/* Transmogrify buffersworth of page offsets */

	while  ((bytes = read(ffd, un.buffer, sizeof(un.buffer))) > 0)  {
		ep = &un.lbuffer[bytes/sizeof(LONG)];
		while  (--ep >= un.lbuffer)
			*ep = htonl((ULONG) *ep);
		sockwrite(sock, un.buffer, bytes);
	}

	close(ffd);
}
#endif

/* Send a file down a socket without worrying about pages */

void	feed_pr(const jobno_t jobno, const char *prefix, const int sock)
{
	int	ffd, bytes;
	char	buffer[1024];

	if  ((ffd = open(mkspid(prefix, jobno), O_RDONLY)) >= 0)  {
		while  ((bytes = read(ffd, buffer, sizeof(buffer))) > 0)
			sockwrite(sock, buffer, bytes);
		close(ffd);
	}
}

/* Process request from another process to read job/error/page file */

void	feed_req(void)
{
	PIDTYPE	pid;
	int	sock;
	slotno_t	jslot;
	jobno_t		jobno;
	struct	feeder	rq;
	SOCKLEN_T	sinl;
	struct	sockaddr  sin;

	sinl = sizeof(sin);
	if  ((sock = accept(viewsock, &sin, &sinl)) < 0)
		return;

	if  (read(sock, (char *) &rq, sizeof(rq)) != sizeof(rq))  {
		close(sock);
		return;
	}

	if  ((pid = fork()) < 0)
		report($E{Internal cannot fork});

#ifndef	BUGGY_SIGCLD
	if  (pid != 0)  {
		close(sock);
		return;
	}
#else
	/* Make the process the grandchild so we don't have to worry about waiting for it later. */

	if  (pid != 0)  {
#ifdef	HAVE_WAITPID
		while  (waitpid(pid, (int *) 0, 0) < 0  &&  errno == EINTR)
			;
#else
		PIDTYPE	wpid;
		while  ((wpid = wait((int *) 0)) != pid  &&  (wpid >= 0 || errno == EINTR))
			;
#endif
		close(sock);
		return;
	}
	if  (fork() != 0)
		exit(0);
#endif

	/* We are now a separate process */

	jobno = ntohl((ULONG) rq.jobno);
	jslot = ntohl((ULONG) rq.jobslot);

	switch  (rq.fdtype)  {
	default:
		sp_feed(jslot, jobno, sock);
		exit(0);

	case  FEED_NPSP:
		feed_pr(jobno, SPNAM, sock);
		exit(0);

	case  FEED_ER:
		feed_pr(jobno, ERNAM, sock);
		exit(0);

	case  FEED_PF:
#ifdef	WORDS_BIGENDIAN
		feed_pr(jobno, PFNAM, sock);
#else
		feed_pages(jobno, sock);
#endif
		exit(0);
	}
}

#else	/* !NETWORK_VERSION */

/* This "routine" isn't strictly necessary but some C compilers winge
   if they are given a .c file with no code so here is some.... */

void	feed_req()
{
	return;
}

#endif	/* !NETWORK_VERSION */
