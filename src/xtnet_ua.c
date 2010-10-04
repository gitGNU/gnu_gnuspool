/* xtnet_ua.c -- xtnetserv user access routines
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
#ifdef	NETWORK_VERSION
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include "incl_sig.h"
#include <errno.h>
#include "errnums.h"
#include "incl_net.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "spuser.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "xfershm.h"
#include "ecodes.h"
#include "client_if.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xtnet_ext.h"
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

extern unsigned  calcnhash(const netid_t); /* Defined in look_host.c */

struct	pend_job *add_pend(const netid_t whofrom)
{
	int	cnt;

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		if  (!pend_list[cnt].out_f)  {
			pend_list[cnt].clientfrom = whofrom;
			pend_list[cnt].prodsent = 0;
			return  &pend_list[cnt];
		}
	return  (struct pend_job *) 0;
}

struct	pend_job *find_pend(const netid_t whofrom)
{
	int	cnt;

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		if  (pend_list[cnt].clientfrom == whofrom)
			return  &pend_list[cnt];
	return  (struct pend_job *) 0;
}

struct	pend_job *find_j_by_jno(const jobno_t jobno)
{
	int	cnt;

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		if  (pend_list[cnt].out_f  &&  pend_list[cnt].jobn == jobno)
			return  &pend_list[cnt];
	return  (struct pend_job *) 0;
}

/* Clean up after decaying job.  */

void  abort_job(struct pend_job *pj)
{
	if  (!pj  ||  !pj->out_f)
		return;
	fclose(pj->out_f);
	pj->out_f = (FILE *) 0;
	if  (pj->delim)  {
		free(pj->delim);
		pj->delim = (char *) 0;
	}
	unlink(pj->tmpfl);
	unlink(pj->pgfl);
	pj->clientfrom = 0;
}

/* Pack up a spdet structure */

void  spdet_pack(struct spdet *to, struct spdet *from)
{
	to->spu_isvalid = from->spu_isvalid;
	strncpy(to->spu_form, from->spu_form, MAXFORM);
	strncpy(to->spu_formallow, from->spu_formallow, ALLOWFORMSIZE);
	strncpy(to->spu_ptr, from->spu_ptr, PTRNAMESIZE);
	strncpy(to->spu_ptrallow, from->spu_ptrallow, JPTRNAMESIZE);
	to->spu_form[MAXFORM] = '\0';
	to->spu_formallow[ALLOWFORMSIZE] = '\0';
	to->spu_ptr[PTRNAMESIZE] = '\0';
	to->spu_ptrallow[JPTRNAMESIZE] = '\0';
	to->spu_user = htonl((ULONG) from->spu_user);
	to->spu_minp = from->spu_minp;
	to->spu_maxp = from->spu_maxp;
	to->spu_defp = from->spu_defp;
	to->spu_cps = from->spu_cps;
	to->spu_flgs = htonl(from->spu_flgs);
	to->spu_class = htonl(from->spu_class);
}

/* Truncate file if neccessary.
   The file is truncated with a truncate call if needed,
   otherwise we have to do it by hand.  */

static int  dotrunc(struct pend_job *pj, const LONG size)
{
#ifdef	HAVE_FTRUNCATE
	if  (ftruncate(fileno(pj->out_f), size) < 0)
		return  1;
	pj->jobout.spq_size = size;
#else
	char	*tnam = mkspid("TMP", pj->jobout.spq_job);
	int	ch, fd;
	LONG	cnt = 0;
	FILE	*tf;
	if  ((fd = open(tnam, O_RDWR|O_CREAT|O_TRUNC|O_EXCL, 0400)) < 0  ||  !(tf = fdopen(fd, "w+")))
		return  1;
	rewind(pj->out_f);
	while  ((ch = getc(pj->out_f)) != EOF  &&  cnt < size)  {
		putc(ch, tf);
		cnt++;
	}
	pj->jobout.spq_size = cnt;
	fclose(pj->out_f);
	pj->out_f = tf;
	rewind(tf);
	if  (unlink(pj->tmpfl) < 0  ||  link(tnam, pj->tmpfl) < 0  ||  unlink(tnam) < 0)
		return  1;
#endif
	return  0;
}

/* At end of UDP or API job, scan for pages
   Return 0 (or XTNR_WARN_LIMIT) if ok otherwise error code.  */

int  scan_job(struct pend_job *pj)
{
	char	*rcp;
	int	ch, rec_cnt, pgfid = -1, retcode = XTNQ_OK;
	ULONG	klim = 0xffffffffL;
	LONG	plim = 0x7fffffffL, onpage, char_count = 0;
	char	*rcdend;

	pj->jobout.spq_npages = 0;
	onpage = 0;
	rewind(pj->out_f);

	if  (pj->jobout.spq_pglim)  {
		if  (pj->jobout.spq_dflags & SPQ_PGLIMIT)
			plim = pj->jobout.spq_pglim;
		else
			klim = (ULONG) pj->jobout.spq_pglim << 10;
	}

	if  ((ULONG) pj->jobout.spq_size > klim)  {
		if  (pj->jobout.spq_dflags & SPQ_ERRLIMIT  ||  dotrunc(pj, (LONG) klim))
			return  XTNR_PAST_LIMIT;
		retcode = XTNR_WARN_LIMIT;
	}

	if  (!pj->delim  ||  (pj->pageout.deliml == 1  &&  pj->pageout.delimnum == 1 &&  pj->delim[0] == '\f'))  {
		pj->jobout.spq_dflags &= SPQ_ERRLIMIT | SPQ_PGLIMIT; /* I didnt mean a ~ here */

		while  (char_count < pj->jobout.spq_size  &&  (ch = getc(pj->out_f)) != EOF)  {
			char_count++;
			onpage++;
			if  (ch == '\f')  {
				onpage = 0;
				if  (++pj->jobout.spq_npages > plim)  {
					if  (pj->jobout.spq_dflags & SPQ_ERRLIMIT || dotrunc(pj, char_count))
						return  XTNR_PAST_LIMIT;
					retcode = XTNR_WARN_LIMIT;
					break;
				}
			}
		}
		if  (char_count == 0)
			return  XTNR_EMPTYFILE;

		/* If we hit EOF before we expected then the file
		   system must be full */

		if  (char_count != pj->jobout.spq_size)
			return  XTNR_FILE_FULL;
		if  (onpage)
			pj->jobout.spq_npages++;
		return  retcode;
	}

	pj->jobout.spq_dflags |= SPQ_PAGEFILE;

	if  ((pgfid = open(pj->pgfl, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0)
		return  XTNR_CC_PAGEFILE;

#ifdef	RUN_AS_ROOT
#ifdef	HAVE_FCHOWN
	if  (Daemuid != ROOTID)
		fchown(pgfid, Daemuid, getegid());
#else
	if  (Daemuid != ROOTID)
		chown(pj->pgfl, Daemuid, getegid());
#endif
#endif
	pj->pageout.lastpage = 0;	/* Fix this later perhaps */
	Ignored_error = write(pgfid, (char *) &pj->pageout, sizeof(pj->pageout));
	Ignored_error = write(pgfid, pj->delim, (unsigned) pj->pageout.deliml);

	rcp = pj->delim;
	rcdend = pj->delim + pj->pageout.deliml;
	onpage = 0;
	rec_cnt = 0;

	while  (char_count < pj->jobout.spq_size  &&  (ch = getc(pj->out_f)) != EOF)  {
		char_count++;
		onpage++;
		if  (ch == *rcp)  {
			if  (++rcp >= rcdend)  {
				if  (++rec_cnt >= pj->pageout.delimnum)  {
					if  (write(pgfid, (char *) &char_count, sizeof(LONG)) != sizeof(LONG))
						return  XTNR_FILE_FULL;
					onpage = 0;
					rec_cnt = 0;
					if  (++pj->jobout.spq_npages > plim)  {
						if  (pj->jobout.spq_dflags & SPQ_ERRLIMIT  ||  dotrunc(pj, char_count))
							return  XTNR_PAST_LIMIT;
						retcode = XTNR_WARN_LIMIT;
						break;
					}
				}
				rcp = pj->delim;
			}
		}
		else  if  (rcp > pj->delim)  {
			char	*pp, *prcp, *prevpl;
			prevpl = --rcp;	/*  Last one matched  */
			for  (;  rcp > pj->delim;  rcp--)  {
				if  (*rcp != ch)
					continue;
				pp = prevpl;
				prcp = rcp - 1;
				for  (;  prcp >= pj->delim;  pp--, prcp--)
					if  (*pp != *prcp)
						goto  rej;
				rcp++;
				break;
			rej:	;
			}
		}
	}

	if  (char_count == 0)  {
		close(pgfid);
		return  XTNR_EMPTYFILE;
	}

	/* If we hit EOF before we expected then the file system must
	   be full */

	if  (char_count != pj->jobout.spq_size)
		return  XTNR_FILE_FULL;

	/* Store the offset of the end of the file */

	if  (write(pgfid, (char *) &pj->jobout.spq_size, sizeof(LONG)) != sizeof(LONG))
		return  XTNR_FILE_FULL;

	/* Remember how big the last page was */

	if  (onpage > 0)  {
		pj->jobout.spq_npages++;
		if  ((pj->pageout.lastpage = pj->pageout.delimnum - rec_cnt) > 0)  {
			lseek(pgfid, 0L, 0);
			Ignored_error = write(pgfid, (char *) &pj->pageout, sizeof(pj->pageout));
		}
	}

	if  (close(pgfid) < 0)
		return  XTNR_FILE_FULL;
	return  retcode;
}

void  udp_job_process(const netid_t whofrom, char *pmsg, int datalength, struct sockaddr_in *sinp)
{
	int	ret = XTNQ_OK, tries;
	struct	hhash	*frp;
	struct	pend_job	*pj;
	char	reply[1];

	if  (!(frp = find_remote(whofrom)))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Unknown client");
		reply[0] = XTNR_UNKNOWN_CLIENT;
		goto  sendrep;
	}
	if  (frp->rem.ht_flags & HT_DOS  &&  frp->flags != UAL_OK)  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op_name(whofrom, "Not logged in", frp->actname);
		reply[0] = (char) frp->flags;
		goto  sendrep;
	}

	pj = find_pend(whofrom);

	switch  (pmsg[0])  {
	default:
		reply[0] = SV_CL_UNKNOWNC;
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Unknown cmd");
		goto  sendrep;

	case  CL_SV_STARTJOB:

		/* Start of new job, delete any pending one. add new one */

		abort_job(pj);
		pj = add_pend(whofrom);
		if  (!pj)  {
			reply[0] = XTNR_NOMEM_QF;
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Nomem queue file");
			goto  sendrep;
		}
		pj->timeout = frp->timeout;
		pj->lastaction = time((time_t *) 0);

		/* Generate job number and output file vaguely from netid etc.  */

		pj->jobn = (ULONG) (ntohl(whofrom) + pj->lastaction) % JOB_MOD;
		pj->out_f = goutfile(&pj->jobn, pj->tmpfl, pj->pgfl, 1);

		/* Step over to job descriptor and unpack */

		pmsg += sizeof(LONG);
		datalength -= sizeof(LONG);
		if  (datalength < sizeof(struct spq))  {
			abort_job(pj);
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "addjob buffer err");
			reply[0] = SV_CL_BADPROTO;
			goto  sendrep;
		}
		unpack_job(&pj->jobout, (struct spq *) pmsg);
		pj->jobout.spq_orighost = whofrom;

		/* Check job details */

		convert_username(frp, &pj->jobout);
		if  ((ret = validate_job(&pj->jobout)) != 0)  {
			abort_job(pj);
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op_name(whofrom, "Invalid job", frp->actname);
			reply[0] = (char) ret;
			goto  sendrep;
		}

		/* Read optional page descriptor */

		pmsg += sizeof(struct spq);
		datalength -= sizeof(struct spq);
		pj->penddelim = 0;
		if  (pj->jobout.spq_dflags & SPQ_PAGEFILE)  {
			if  (datalength < sizeof(struct pages))  {
				abort_job(pj);
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op(whofrom, "proto err pagefile");
				reply[0] = SV_CL_BADPROTO;
				goto  sendrep;
			}
			BLOCK_COPY(&pj->pageout, pmsg, sizeof(struct pages));
			pmsg += sizeof(struct pages);
			datalength -= sizeof(struct pages);
#ifndef	WORDS_BIGENDIAN
			pj->pageout.delimnum = ntohl((ULONG) pj->pageout.delimnum);
			pj->pageout.deliml = ntohl((ULONG) pj->pageout.deliml);
#endif

			/* Read page delimiter */

			if  (pj->pageout.deliml <= 0)  {
				abort_job(pj);
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op(whofrom, "delim error");
				reply[0] = XTNR_BAD_PF;
				goto  sendrep;
			}
			if  (!(pj->delim = malloc((unsigned) pj->pageout.deliml)))  {
				abort_job(pj);
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op(whofrom, "nomem for delim");
				reply[0] = XTNR_NOMEM_PF;
				goto  sendrep;
			}
			if  (pj->pageout.deliml <= datalength)
				BLOCK_COPY(pj->delim, pmsg, pj->pageout.deliml);
			else  {
				pj->penddelim = pj->pageout.deliml - datalength;
				BLOCK_COPY(pj->delim, pmsg, datalength);
			}
		}
		pj->jobout.spq_job = pj->jobn;
		pj->jobout.spq_size = 0;
		pj->jobout.spq_time = (LONG) pj->lastaction;
		goto  sendok;

	case  CL_SV_CONTDELIM:

		if  (!pj)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "no pending job");
			reply[0] = SV_CL_UNKNOWNJ;
			goto  sendrep;
		}

		pj->lastaction = time((time_t *) 0);

		/* Might be more of the page delimiter */

		if  (pj->penddelim != 0)  {
			if  (pj->penddelim <= (unsigned) datalength)  {
				BLOCK_COPY(pj->delim + (unsigned) pj->pageout.deliml - pj->penddelim, pmsg, pj->penddelim);
				pj->penddelim = 0;
			}
			else  {
				BLOCK_COPY(pj->delim + (unsigned) pj->pageout.deliml - pj->penddelim, pmsg, datalength);
				pj->penddelim -= datalength;
			}
			goto  sendok;
		}
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "unknown job (delim)");
		reply[0] = SV_CL_UNKNOWNJ;
		goto  sendrep;

	case  CL_SV_JOBDATA:

		if  (!pj)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "unknown job (job data)");
			reply[0] = SV_CL_UNKNOWNJ;
			goto  sendrep;
		}

		pj->lastaction = time((time_t *) 0);

		while  (--datalength > 0)  {
			pmsg++;
			if  (putc(*(unsigned char *)pmsg, pj->out_f) == EOF)
				goto  bad_put;
			pj->jobout.spq_size++;
		}
		if  (fflush(pj->out_f) != EOF)
			goto  sendok;
	bad_put:
		abort_job(pj);
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "file system full");
		reply[0] = XTNR_FILE_FULL;
		goto  sendrep;

	case  CL_SV_ENDJOB:
		if  (!pj)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "unknown job (end job)");
			reply[0] = SV_CL_UNKNOWNJ;
			goto  sendrep;
		}
		if  ((ret = scan_job(pj)) != XTNQ_OK  &&  ret != XTNR_WARN_LIMIT)  {
			abort_job(pj);
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "overlarge job");
			reply[0] = (char) ret;
			goto  sendrep;
		}

		/* Copy job to request buffer
		   (NB Child TCP processes initialise their copy of SPQ).  */

		for  (tries = 0;  tries < MAXTRIES;  tries++)  {
			if  (wjmsg(&sp_req, &pj->jobout) == 0)
				goto  sentok;
			sleep(TRYTIME);
		}
		abort_job(pj);
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "msg q full");
		reply[0] = XTNR_QFULL;
		goto  sendrep;

	sentok:
		fclose(pj->out_f);
		pj->out_f = (FILE *) 0;
		if  (pj->delim)  {
			free(pj->delim);
			pj->delim = (char *) 0;
		}
		pj->clientfrom = 0;
		goto  sendok;

	case  CL_SV_HANGON:
		if  (!pj)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "unknown job (hangon)");
			reply[0] = SV_CL_UNKNOWNJ;
			goto  sendrep;
		}
		pj->lastaction = time((time_t *) 0);
		pj->prodsent = 0;
		goto  sendok;
	}

 sendok:
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op_name(whofrom, "Job OK", frp->actname);
	reply[0] = ret;
 sendrep:

	sendto(uasock, reply, sizeof(reply), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

static	void  udp_send_vec(char *vec, const int size, struct sockaddr_in *sinp)
{
	sendto(uasock, vec, size, 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Similar routine for when we are sending from scratch */

static void  udp_send_to(char *vec, const int size, const netid_t whoto)
{
	int	sockfd, tries;
	struct	sockaddr_in	to_sin, cli_addr;

	BLOCK_ZERO(&to_sin, sizeof(to_sin));
	to_sin.sin_family = AF_INET;
	to_sin.sin_port = uaportnum;
	to_sin.sin_addr.s_addr = whoto;

	BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cli_addr.sin_port = 0;

	/* We don't really need the cli_addr but we are obliged to
	   bind something.  The remote uses our standard port.  */

	for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
		if  ((sockfd = socket(AF_INET, SOCK_DGRAM, udpproto)) < 0)
			return;
		if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
			close(sockfd);
			return;
		}
		if  (sendto(sockfd, vec, size, 0, (struct sockaddr *) &to_sin, sizeof(to_sin)) >= 0)  {
			close(sockfd);
			return;
		}
		close(sockfd);
	}
}

void  udp_send_ulist(const netid_t whofrom, struct sockaddr_in *sinp)
{
	char	**ul;
	int	rp = 0;
	struct	hhash	*frp;
	char	reply[CL_SV_BUFFSIZE];

	if  ((frp = find_remote(whofrom))  &&  (!(frp->rem.ht_flags & HT_DOS) || frp->flags == UAL_OK)  &&  (ul = gen_ulist((char *) 0, 0)))  {
		char	**up;
		for  (up = ul;  *up;  up++)  {
			int	lng = strlen(*up) + 1;
			if  (lng + rp > CL_SV_BUFFSIZE)  {
				udp_send_vec(reply, rp, sinp);
				rp = 0;
			}
			strcpy(&reply[rp], *up);
			rp += lng;
			free(*up);
		}
		if  (rp > 0)
			udp_send_vec(reply, rp, sinp);
		free((char *) ul);
	}

	/* Mark end with a null */

	reply[0] = '\0';
	udp_send_vec(reply, 1, sinp);
}

int  checkpw(const char *name, const char *passwd)
{
	static	char	*sppwnam;
	int		ipfd[2], opfd[2];
	char		rbuf[1];
	PIDTYPE		pid;

	if  (!sppwnam)
		sppwnam = envprocess(SPPWPROG);

	/* Don't bother with error messages, just say no.  */

	if  (pipe(ipfd) < 0)
		return  0;
	if  (pipe(opfd) < 0)  {
		close(ipfd[0]);
		close(ipfd[1]);
		return  0;
	}

	if  ((pid = fork()) == 0)  {
		close(opfd[1]);
		close(ipfd[0]);
		if  (opfd[0] != 0)  {
			close(0);
			Ignored_error = dup(opfd[0]);
			close(opfd[0]);
		}
		if  (ipfd[1] != 1)  {
			close(1);
			Ignored_error = dup(ipfd[1]);
			close(ipfd[1]);
		}
		execl(sppwnam, sppwnam, name, (char *) 0);
		exit(255);
	}
	close(opfd[0]);
	close(ipfd[1]);
	if  (pid < 0)  {
		close(ipfd[0]);
		close(opfd[1]);
		return  0;
	}
	Ignored_error = write(opfd[1], passwd, strlen(passwd));
	rbuf[0] = '\n';
	Ignored_error = write(opfd[1], rbuf, sizeof(rbuf));
	close(opfd[1]);
	if  (read(ipfd[0], rbuf, sizeof(rbuf)) != sizeof(rbuf))  {
		close(ipfd[0]);
		return  0;
	}
	close(ipfd[0]);
	return  rbuf[0] == '0'? 1: 0;
}

/* Tell scheduler about new user.  */

void  tell_sched_roam(const netid_t netid, const char *unam)
{
	struct	spr_req	nmsg;
	BLOCK_ZERO(&nmsg, sizeof(nmsg));
	nmsg.spr_mtype = MT_SCHED;
	nmsg.spr_un.n.spr_act = SON_ROAMUSER;
	nmsg.spr_un.n.spr_pid = getpid();
	nmsg.spr_un.n.spr_n.hostid = netid;
	strncpy(nmsg.spr_un.n.spr_n.hostname, unam, HOSTNSIZE);
	msgsnd(Ctrl_chan, (struct msgbuf *) &nmsg, sizeof(struct sp_nmsg), 0); /* Wait until it goes */
}

/* Tell other xtnetservs about new user.  */

void  tell_friends(struct hhash *frp)
{
	unsigned	cnt;
	struct	ua_pal  palsmsg;

	BLOCK_ZERO(&palsmsg, sizeof(palsmsg));
	palsmsg.uap_op = SV_SV_LOGGEDU;
	palsmsg.uap_netid = frp->rem.hostid;
	strncpy(palsmsg.uap_name, frp->actname, UIDSIZE);
	strncpy(palsmsg.uap_wname, frp->dosname, UIDSIZE);

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		struct	hhash	*hp;
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)
			if  ((hp->rem.ht_flags & (HT_DOS|HT_TRUSTED)) == HT_TRUSTED)  {
				if  (tracing & TRACE_SYSOP)
					client_trace_op_name(hp->rem.hostid, "tell friends", frp->actname);
				udp_send_to((char *) &palsmsg, sizeof(palsmsg), hp->rem.hostid);
			}
	}

	tell_sched_roam(frp->rem.hostid, frp->actname);
}

/* Check password status in login/enquiries on roaming users, and "tell friends" */

static void  set_pwstatus(struct ua_login *inmsg, struct hhash *frp, struct cluhash *cp)
{
	if  (cp->rem.ht_flags & HT_PWCHECK  ||  (cp->machname  &&  ncstrcmp(cp->machname, inmsg->ual_machname) != 0))  {
		frp->flags = UAL_NOK;
		if  (inmsg->ual_op != UAL_LOGIN)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op_name(frp->rem.hostid, "No pw", frp->actname);
			return;
		}
		if  (!checkpw(frp->actname, inmsg->ual_passwd))  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op_name(frp->rem.hostid, "Bad pw", frp->actname);
			frp->flags = UAL_INVP;
			return;
		}
	}
	frp->flags = UAL_OK;
	tell_friends(frp);
}

/* Make name lower case - assumed at most UIDSIZE long, but we don't trust it.  */

static char *lcify(const char *orig)
{
	static	char	result[UIDSIZE+1];
	int	cnt = 0;

	while  (*orig  &&  cnt < UIDSIZE)
		result[cnt++] = tolower(*orig++);
	result[cnt] = '\0';
	return  result;
}

static struct cluhash *look_clu(const char *winname)
{
	struct	cluhash	*cp;
	unsigned  hval = calc_clu_hash(winname);

	for  (cp = cluhashtab[hval];  cp;  cp = cp->next)
		if  (ncstrcmp(cp->rem.hostname, winname) == 0  &&  (lookup_uname(cp->rem.hostname) != UNKNOWN_UID  ||  lookup_uname(cp->rem.alias) != UNKNOWN_UID))
			return  cp;
	for  (cp = cluhashtab[hval];  cp;  cp = cp->alias_next)
		if  (ncstrcmp(cp->rem.alias, winname) == 0  &&  (lookup_uname(cp->rem.hostname) != UNKNOWN_UID  ||  lookup_uname(cp->rem.alias) != UNKNOWN_UID))
			return  cp;
	/*  If a default user is supplied, use that */
	winname = XTDEFNAME;
	for  (cp = cluhashtab[calc_clu_hash(winname)];  cp;  cp = cp->next)
		if  (ncstrcmp(cp->rem.hostname, winname) == 0  &&  lookup_uname(cp->rem.alias) != UNKNOWN_UID)
			return  cp;
	return  (struct cluhash *) 0;
}

struct cluhash *update_roam_name(struct hhash *frp, const char *name)
{
	struct	cluhash	*cp = look_clu(name);

	if  (cp)  {
		free(frp->dosname);
		frp->dosname = stracpy(name);
		free(frp->actname);
		if  (lookup_uname(cp->rem.hostname) == UNKNOWN_UID)
			frp->actname = stracpy(cp->rem.alias);
		else
			frp->actname = stracpy(cp->rem.hostname);
		frp->rem.ht_flags = cp->rem.ht_flags;
		frp->timeout = cp->rem.ht_timeout;
		frp->lastaction = time((time_t *) 0);
	}
	return  cp;
}

struct cluhash *new_roam_name(const netid_t whofrom, struct hhash **frpp, const char *name)
{
	struct	cluhash  *cp;

	if  ((cp = look_clu(name)))  {

		unsigned  nhval;
		struct	hhash	*frp;

		/* Allocate and put a new structure on host hash list. */

		if  (!(frp = (struct hhash *) malloc(sizeof(struct hhash))))
			nomem();

		BLOCK_ZERO(frp, sizeof(struct hhash));
		nhval = calcnhash(whofrom);
		frp->hn_next = nhashtab[nhval];
		nhashtab[nhval] = frp;
		frp->rem = cp->rem;
		frp->rem.hostid = whofrom;
		frp->timeout = frp->rem.ht_timeout;
		frp->lastaction = time((time_t *) 0);
		frp->dosname = stracpy(name);
		if  (lookup_uname(cp->rem.hostname) == UNKNOWN_UID)
			frp->actname = stracpy(cp->rem.alias);
		else
			frp->actname = stracpy(cp->rem.hostname);
		*frpp = frp;
	}
	return  cp;
}

int  update_nonroam_name(struct hhash *frp, const char *name)
{
	free(frp->actname);
	frp->actname = stracpy(name);
	if  (lookup_uname(frp->actname) == UNKNOWN_UID)  {
		if  (lookup_uname(lcify(name)) == UNKNOWN_UID)  {
			frp->flags = UAL_INVU;
			return  0;
		}
		free(frp->actname);
		frp->actname = stracpy(lcify(name));
	}
	frp->lastaction = time((time_t *) 0);
	return  1;
}

static void  do_logout(struct hhash *frp)
{
	if  (frp->rem.ht_flags & HT_ROAMUSER)  {

		struct  hhash  **hpp, *hp;

		/* Roaming user, get rid of record from hash chain.
		   We "cannot" fall off the bottom of this loop,
		   but we write it securely don't we...  */

		for  (hpp = &nhashtab[calcnhash(frp->rem.hostid)]; (hp = *hpp); hpp = &hp->hn_next)
			if  (hp == frp)  {
				*hpp = hp->hn_next;
				free(frp->actname);
				free(frp->dosname);
				free((char *) frp);
				break;
			}
	}
	else  {

		/* Non roaming-user case, disregard it unless we have
		   a non-standard login, whereupon we fix it back
		   to standard login.  */

		if  (strcmp(frp->actname, frp->dosname) != 0)  {
			free(frp->actname);
			frp->actname = stracpy(frp->dosname);
			frp->flags = frp->rem.ht_flags & HT_PWCHECK? UAL_NOK: UAL_OK;
		}

		/* Worry about that again tomorrow unless we hear back */
		frp->lastaction = time((time_t *) 0) + 3600L * 24;
	}
}

/* Handle logins and initial enquiries, doing as much as possible.  */

static void  udp_login(const netid_t whofrom, struct ua_login *inmsg, const int inlng, struct sockaddr_in *sinp)
{
	struct	ua_login	reply;

	BLOCK_ZERO(&reply, sizeof(reply));	/* Maybe this is paranoid but with passwords kicking about..... */

	if  (inlng != sizeof(struct ua_login))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Login struct err");
		reply.ual_op = SV_CL_BADPROTO;
		goto  sendret;
	}

	if  (inmsg->ual_op == UAL_ENQUIRE  ||  inmsg->ual_op == UAL_LOGIN)  {
		struct	hhash		*frp;
		struct	cluhash		*cp;

		if  ((frp = find_remote(whofrom)))  {
			if  (frp->rem.ht_flags & HT_ROAMUSER)  {

				/* Might have changed in the interim
				   We stored the Windows user as sent in "dosname",
				   the other way round from the non-roaming case */

				if  (inmsg->ual_op == UAL_LOGIN)  {
					if  (strcmp(frp->dosname, inmsg->ual_name) != 0)  { /* Name change */
						if  (!(cp = update_roam_name(frp, inmsg->ual_name)))  {
							if  (tracing & TRACE_CLIOPEND)
								client_trace_op_name(whofrom, "Unknown user", inmsg->ual_name);
							reply.ual_op = XTNR_NOT_USERNAME;
							goto  sendret;
						}
						set_pwstatus(inmsg, frp, cp);
					}
					else  if  (frp->flags != UAL_OK)  {
						if  (!(cp = look_clu(inmsg->ual_name)))  {
							if  (tracing & TRACE_CLIOPEND)
								client_trace_op_name(whofrom, "Unknown client user", inmsg->ual_name);
							reply.ual_op = XTNR_UNKNOWN_CLIENT;
							goto  sendret;
						}
						set_pwstatus(inmsg, frp, cp);
					}
				}
			}
			else  {

				/* If this isn't from a client, I'm confused */

				if  (!(frp->rem.ht_flags & HT_DOS))  {
					if  (tracing & TRACE_CLIOPEND)
						client_trace_op(whofrom, "Client op - not client");
					reply.ual_op = XTNR_NOT_CLIENT;
					goto  sendret;
				}

				/* Not a roaming user, but some species of dos user who must be
				   correctly specified (case insens) in the Windows user in
				   "actname", no opportunity for aliasing here as we've got a real
				   user in "dosname" */

				if  (inmsg->ual_op == UAL_LOGIN)  {
					if  (ncstrcmp(frp->actname, inmsg->ual_name) != 0)  {  /* Name change */
						if  (!update_nonroam_name(frp, inmsg->ual_name))  {
							if  (tracing & TRACE_CLIOPEND)
								client_trace_op_name(whofrom, "invalid user", inmsg->ual_name);
							frp->flags = UAL_INVU;
							goto  sendactname;
						}

						/* Change here - we no longer effectively force on password
						   check if no (dosuser) see also look_host.c */

						if  (frp->rem.ht_flags & HT_PWCHECK  ||
						     (frp->dosname[0]  &&  ncstrcmp(frp->dosname, frp->actname) != 0))  {
							frp->flags = UAL_NOK;		/* Password required */
							if  (inmsg->ual_op == UAL_LOGIN  &&  !checkpw(frp->actname, inmsg->ual_passwd))
								frp->flags = UAL_INVP;
							else
								frp->flags = UAL_OK;
						}
						else
							frp->flags = UAL_OK;
					}
				}
			}
		}
		else  {

			/* We haven't heard of this geyser before.
			   (This only applies to roaming users).  */

			if  (!(cp = new_roam_name(whofrom, &frp, inmsg->ual_name)))  {
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op_name(whofrom, "unknown client uname", inmsg->ual_name);
				reply.ual_op = XTNR_UNKNOWN_CLIENT;
				goto  sendret;
			}
			set_pwstatus(inmsg, frp, cp);
		}

	sendactname:
		strncpy(reply.ual_name, frp->actname, UIDSIZE);
		reply.ual_op = (unsigned char) frp->flags;
	}
	else  {

		struct	hhash		*frp;

		/* Logout case. */

		if  (inmsg->ual_op != UAL_LOGOUT)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Invalid proto logout");
			reply.ual_op = SV_CL_BADPROTO;
			goto  sendret;
		}

		if  (!(frp = find_remote(whofrom)))  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Unknown client");
			reply.ual_op = XTNR_UNKNOWN_CLIENT;
			goto  sendret;
		}

		reply.ual_op = UAL_OK;
		do_logout(frp);
	}

 sendret:
	udp_send_vec((char *) &reply, sizeof(reply), sinp);
}

static void  note_roamer(const netid_t whofrom, struct ua_pal *inmsg, const int inlng)
{
	struct	hhash	*sender, *client;

	/* Ignore message if invalid length, unknown host or not
	   trusted host wot's sending it.  */

	if  (inlng != sizeof(struct ua_pal)  ||  !((sender = find_remote(whofrom))  &&  sender->rem.ht_flags & HT_TRUSTED))
		return;

	if  ((client = find_remote(inmsg->uap_netid)))  {

		/* We've met this machine before, but possibly with a
		   different user. */

		if  (!(client->rem.ht_flags & HT_ROAMUSER))	/* Huh??? */
			return;
		if  (strcmp(client->dosname, inmsg->uap_wname) == 0)
			return;
		free(client->actname);
		free(client->dosname);
		client->actname = stracpy(inmsg->uap_name);
		client->dosname = stracpy(inmsg->uap_wname);
		if  (tracing & TRACE_SYSOP)
			client_trace_op_name(whofrom, "Note roam user", client->actname);
		client->lastaction = time((time_t *) 0);
	}
	else  {
		/* Ignore it unless we know about the windows user */

		if  (!new_roam_name(inmsg->uap_netid, &client, inmsg->uap_wname))
			return;
		client->flags = UAL_OK;
		if  (tracing & TRACE_SYSOP)
			client_trace_op_name(whofrom, "Note new roam user", inmsg->uap_wname);
	}

	tell_sched_roam(inmsg->uap_netid, client->actname);
}

/* This is mainly for dosspwrite */

static void  answer_asku(const netid_t whofrom, struct ua_pal *inmsg, const int inlng, struct sockaddr_in *sinp)
{
	int	nu = 0, cnt;
	struct	hhash	*hp;
	struct	ua_asku_rep	reply;

	BLOCK_ZERO(&reply, sizeof(reply));

	if  (inlng != sizeof(struct ua_pal)  ||  (whofrom != myhostid && whofrom != localhostid  &&  !((hp = find_remote(whofrom))  &&  hp->rem.ht_flags & HT_TRUSTED)))
		goto  dun;

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)  {
			if  (!(hp->rem.ht_flags & HT_DOS))
				continue;
			if  (hp->flags != UAL_OK)
				continue;
			if  (hp->rem.ht_flags & HT_ROAMUSER)  {
				if  (strcmp(hp->actname, inmsg->uap_name) != 0)
					continue;
			}
			else  if  (strcmp(hp->dosname, inmsg->uap_name) != 0)
				continue;
			reply.uau_ips[nu++] = hp->rem.hostid;
			if  (nu >= UAU_MAXU)
				goto  dun;
		}
	}
 dun:
	reply.uau_n = htons(nu);
	udp_send_vec((char *) &reply, sizeof(reply), sinp);
	if  (tracing & TRACE_SYSOP)
		client_trace_op_name(whofrom, "asku", inmsg->uap_name);
}

static void  answer_askall(const netid_t whofrom, struct ua_pal *inmsg, const int inlng)
{
	int	cnt;
	struct	hhash	*hp;
	struct	ua_pal	reply;

	BLOCK_ZERO(&reply, sizeof(reply));
	reply.uap_op = SV_SV_LOGGEDU;

	if  (inlng != sizeof(struct ua_pal)  ||
	     (whofrom != myhostid && whofrom != localhostid  &&
	      !((hp = find_remote(whofrom))  &&  hp->rem.ht_flags & HT_TRUSTED)))
		return;

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)  {
			if  (!(hp->rem.ht_flags & HT_ROAMUSER))
				continue;
			if  (hp->flags != UAL_OK)
				continue;
			reply.uap_netid = hp->rem.hostid;
			strncpy(reply.uap_name, hp->actname, UIDSIZE);
			strncpy(reply.uap_wname, hp->dosname, UIDSIZE);
			udp_send_to((char *) &reply, sizeof(reply), whofrom);
		}
	}
	if  (tracing & TRACE_SYSOP)
		client_trace_op(whofrom, "askall done");
}

void  send_askall()
{
	int	cnt;
	struct	ua_pal	msg;

	BLOCK_ZERO(&msg, sizeof(msg));
	msg.uap_op = SV_SV_ASKALL;

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		struct	hhash	*hp;
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)
			if  ((hp->rem.ht_flags & (HT_DOS|HT_TRUSTED)) == HT_TRUSTED)
				udp_send_to((char *) &msg, sizeof(msg), hp->rem.hostid);
	}
}

/* Respond to keep alive messages - we rely on "find_remote" updating
   the last access time.  */

static void  tickle(const netid_t whofrom, struct sockaddr_in *sinp)
{
	struct	hhash	*frp = find_remote(whofrom);
	char	repl = XTNQ_OK;
	if  (frp)  {
		udp_send_vec(&repl, sizeof(repl), sinp);
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "tickle");
	}
}

void  process_ua()
{
	uid_t	Realuid;	/* NOT a global */
	int	datalength;
	LONG	ret;
	int_ugid_t	uret;
	netid_t	whofrom;
	struct	hhash	*frp;
	char	*luser;
	struct	spdet	*spuser;
	struct	ua_reply  rep;
	LONG	pmsgl[CL_SV_BUFFSIZE/sizeof(LONG)]; /* Force to long */
	char	*pmsg = (char *) pmsgl;
	struct	sockaddr_in	*sinp;
#ifdef	STRUCT_SIG
	struct	sigstruct_name  zch;
#endif
	struct	sockaddr_in	sin;
	SOCKLEN_T		sinl = sizeof(sin);
	sinp = &sin;

#ifdef	STRUCT_SIG
	zch.sighandler_el = SIG_IGN;
	sigmask_clear(zch);
	zch.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(QRFRESH, &zch, (struct sigstruct_name *) 0);
#else
	signal(QRFRESH, SIG_IGN);
#endif
	if  ((datalength = recvfrom(uasock, pmsg, sizeof(pmsgl), 0, (struct sockaddr *) sinp, &sinl)) < 0)
		return;
	whofrom = sin.sin_addr.s_addr;

	/* Deal with job data etc above */

	switch  (pmsg[0])  {
	case  CL_SV_UENQUIRY:
		break;
	case  CL_SV_ULIST:
		udp_send_ulist(whofrom, sinp);
		return;
	case  UAL_ENQUIRE:	/* This is done first */
	case  UAL_LOGIN:
	case  UAL_LOGOUT:	/* Not used now */
	case  UAL_OK:		/* Actually these are redundant */
	case  UAL_NOK:
		udp_login(whofrom, (struct ua_login *) pmsg, datalength, sinp);
		return;
	default:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "data");
		udp_job_process(whofrom, pmsg, datalength, sinp);
		return;
	case  SV_SV_LOGGEDU:
		if  (tracing & TRACE_SYSOP)
			client_trace_op(whofrom, "noteroam");
		note_roamer(whofrom, (struct ua_pal *) pmsg, datalength);
		return;
	case  SV_SV_ASKU:
		if  (tracing & TRACE_SYSOP)
			client_trace_op(whofrom, "asku");
		answer_asku(whofrom, (struct ua_pal *) pmsg, datalength, sinp);
		return;
	case  SV_SV_ASKALL:
		if  (tracing & TRACE_SYSOP)
			client_trace_op(whofrom, "askall");
		answer_askall(whofrom, (struct ua_pal *) pmsg, datalength);
		return;
	case  CL_SV_KEEPALIVE:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "keepalive");
		tickle(whofrom, sinp);
		return;
	}

	/* This is the original style of enquiry.
	   We just received a single byte and we pass back user name and
	   permissions.  In the new regime we kick off with
	   UAL_ENQUIRE (with Win user name and machine) and
	   possibly have a conversation about passwords and so
	   forth before we get here.  */

	if  (!(frp = find_remote(whofrom)))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "init-unknown client");
		ret = XTNR_UNKNOWN_CLIENT;
		goto  badret;
	}
	if  (frp->rem.ht_flags & HT_DOS)  {
		if  (frp->flags != UAL_OK)  {
			ret = frp->flags;
			goto  badret;
		}
		luser = frp->actname;	/* Use actname as nonroam case puts this in as well (and its here if name changed) */
	}
	else	/* Unix enquirers pack the user id on the end */
		luser = &pmsg[1];

	uret = lookup_uname(luser);
	Realuid = uret == UNKNOWN_UID?  Daemuid: uret;		/* NB Not a global Realuid! */

	spuser = getspuentry(Realuid);

	/* Pack together our answer and launch it out again.  */

	strcpy(rep.ua_uname, prin_uname(Realuid));
	spdet_pack(&rep.ua_perm, spuser);
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op_name(whofrom, "init-OK", luser);

 xmit:

	sendto(uasock, (char *) &rep, sizeof(rep), 0, (struct sockaddr *) &sin, sizeof(sin));
	return;

 badret:

	/* Bad return - we return the errors in the user id field of
	   the permissions structure, which is otherwise useless.  */

	BLOCK_ZERO(rep.ua_uname, sizeof(rep.ua_uname));
	rep.ua_perm.spu_isvalid = 0;
	rep.ua_perm.spu_user = htonl(ret);
	goto  xmit;
}

/* Tell the client we think it should wake up */

void  send_prod(struct pend_job *pj)
{
	char	prodit = SV_CL_TOENQ;
	udp_send_to(&prodit, sizeof(prodit), pj->clientfrom);
	if  (tracing & TRACE_SYSOP)
		client_trace_op(pj->clientfrom, "Prod");
}

/* See which UDP ports seem to have dried up
   Return time of next alarm.  */

unsigned  process_alarm()
{
	time_t	now = time((time_t *) 0);
	unsigned  mintime = 0, nexttime;
	int	prodtime, killtime, cnt;

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		if  (pend_list[cnt].clientfrom != 0L)  {
			struct	pend_job  *pj = &pend_list[cnt];
			prodtime = pj->lastaction + pj->timeout - now;
			killtime = prodtime + pj->timeout;
			if  (killtime < 0)  {
				abort_job(pj); /* Must have died */
				continue;
			}
			if  (prodtime <= 0)  {
				send_prod(pj);
				nexttime = killtime;
			}
			else
				nexttime = prodtime;
			if  (mintime != 0  &&  (mintime == 0 || mintime > nexttime))
				mintime = nexttime;
		}

	/* Apply timeouts to stale connections.  */

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		struct	hhash	*hp;
	redohash:		/* Hash chain gets mangled by do_logout */
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)
			if  (hp->rem.ht_flags & HT_DOS)  {
				long	tdiff = (long) (hp->lastaction + hp->timeout) - now;
				if  (tdiff <= 0)  {
					if  (tracing & TRACE_SYSOP)
						client_trace_op_name(hp->rem.hostid, "Force logout", hp->actname);
					do_logout(hp);
					goto  redohash;
				}
				else  if  (mintime == 0 || mintime > (unsigned) tdiff)
					mintime = (unsigned) tdiff;
			}
	}
	return  mintime;
}

#else	/* NETWORK_VERSION */

void	foo2()
{
	return;
}

#endif /* !NETWORK_VERSION */
