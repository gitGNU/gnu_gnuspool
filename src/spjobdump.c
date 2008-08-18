/* spjobdump.c -- dump out job into command and job files

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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <ctype.h>
#include <errno.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <sys/shm.h>
#include "errnums.h"
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "pages.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "files.h"
#include "helpargs.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#ifdef	SHAREDLIBS
#include "xfershm.h"
#include "displayopt.h"
#endif

#define	IPC_MODE	0

FILE	*Cfile;

int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
#ifdef	SHAREDLIBS
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
DEF_DISPOPTS;
#endif

char		nodelete;
LONG		Jobnum;
netid_t	netid;
char		*Dirname;
char		*Xfile, *Jfile;

uid_t	Realuid, Effuid, Daemuid;

int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
FILE 	*net_feed(const int, const netid_t, const slotno_t, const jobno_t);
HelpargRef	helpargs(const Argdefault *, const int, const int);

static	void	spitstring(const int arg, FILE * xfl, const int term)
{
	int	v = arg - $A{spr explain};

	if  (optvec[v].isplus)
		fprintf(xfl, " +%s ", optvec[v].aun.string);
	else  if  (optvec[v].aun.letter == 0)
		fprintf(xfl, " +missing-arg-code-%d ", arg);
	else
		fprintf(xfl, " -%c ", optvec[v].aun.letter);
	if  (term)
		fputs("\\\n", xfl);
}

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory");
	exit(E_NOMEM);
}

/* Reread job file if necessary.  */

void	rerjobfile(void)
{
#ifdef	USING_MMAP
	if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)
#else
	if  (Job_seg.dinf.base != Job_seg.dptr->js_did)
#endif
	{
		jobshm_lock();
		jobgrown();
		jobshm_unlock();
	}
}

void  dumphdrs(const struct spq *jp, FILE *xfl, char *delim, struct pages *pt)
{
	fputs("gspl-pr", xfl);
	spitstring($A{spr priority}, xfl, 0);
	fprintf(xfl, "%d \\\n", jp->spq_pri);

	spitstring($A{spr copies}, xfl, 0);
	fprintf(xfl, "%d \\\n", jp->spq_cps);

	spitstring(jp->spq_jflags & SPQ_RETN? $A{spr retain}: $A{spr no retain}, xfl, 1);

	if  (time((time_t *) 0) < (time_t) jp->spq_hold)  {
		time_t  ht = jp->spq_hold;
		struct	tm	*tp;
		tp = localtime(&ht);
		spitstring($A{spr delay until}, xfl, 0);
		fprintf(xfl, "%.2d/%.2d/%.2d,%.2d:%.2d:%.2d \\\n",
			       tp->tm_year % 100,
			       tp->tm_mon + 1,
			       tp->tm_mday,
			       tp->tm_hour,
			       tp->tm_min,
			       tp->tm_sec);
	}

	spitstring($A{spr printed timeout}, xfl, 0);
	fprintf(xfl, "%d \\\n", jp->spq_ptimeout);
	spitstring($A{spr not printed timeout}, xfl, 0);
	fprintf(xfl, "%d \\\n", jp->spq_nptimeout);
	if  (!(jp->spq_jflags & (SPQ_WRT|SPQ_MAIL)))
		spitstring($A{spr no messages}, xfl, 1);
	if  (jp->spq_jflags & SPQ_WRT)
		spitstring($A{spr write message}, xfl, 1);
	if  (jp->spq_jflags & SPQ_MAIL)
		spitstring($A{spr mail message}, xfl, 1);
	if  (!(jp->spq_jflags & (SPQ_WATTN|SPQ_MATTN)))
		spitstring($A{spr no attention}, xfl, 1);
	if  (jp->spq_jflags & SPQ_WATTN)
		spitstring($A{spr write attention}, xfl, 1);
	if  (jp->spq_jflags & SPQ_MATTN)
		spitstring($A{spr mail attention}, xfl, 1);
	spitstring(jp->spq_jflags & SPQ_LOCALONLY? $A{spr local only}: $A{spr network wide}, xfl, 1);

	spitstring(jp->spq_jflags & SPQ_NOH? $A{spr no banner}: $A{spr banner}, xfl, 1);

	if  (delim)  {
		int	ii;

		spitstring($A{spr delimiter number}, xfl, 0);
		fprintf(xfl, "%ld\\\n", (long) pt->delimnum);
		spitstring($A{spr delimiter}, xfl, 0);
		fputs("\'", xfl);

		for  (ii = 0;  ii < pt->deliml;  ii++)  {
			int	ch = delim[ii] & 255;
			if  (!isascii(ch))
				fprintf(xfl, "\\x%.2x", ch);
			else  if  (iscntrl(ch))  {
				switch  (ch)  {
				case  033:
					fputs("\\e", xfl);
					break;
				case  ('h' & 0x1f):
					fputs("\\b", xfl);
					break;
				case  '\r':
					fputs("\\r", xfl);
					break;
				case  '\n':
					fputs("\\n", xfl);
					break;
				case  '\f':
					fputs("\\f", xfl);
					break;
				case  '\t':
					fputs("\\t", xfl);
					break;
				case  '\v':
					fputs("\\v", xfl);
					break;
				default:
					fprintf(xfl, "^%c", ch | 0x40);
					break;
				}
			}
			else  {
				switch  (ch)  {
				case  '\\':
				case  '^':
					putc(ch, xfl);
				default:
					putc(ch, xfl);
					break;
				case  '\'':
				case  '\"':
					putc('\\', xfl);
					putc(ch, xfl);
					break;
				}
			}
		}
		fputs("\' \\\n", xfl);
	}
	if  (jp->spq_jflags & (SPQ_ODDP|SPQ_EVENP))  {
		spitstring($A{spr odd even}, xfl, 0);
		fprintf(xfl, "%c\\\n",
			       jp->spq_jflags & SPQ_ODDP? (jp->spq_jflags & SPQ_REVOE? 'A': 'O'):
			       (jp->spq_jflags & SPQ_REVOE? 'B': 'E'));
	}
	if  (jp->spq_start > 0  ||  jp->spq_end <= LOTSANDLOTS)  {
		spitstring($A{spr page range}, xfl, 0);
		if  (jp->spq_start > 0)
			fprintf(xfl, "%ld", jp->spq_start+1L);
		putc('-', xfl);
		if  (jp->spq_end <= LOTSANDLOTS)
			fprintf(xfl, "%ld", jp->spq_end+1L);
		fputs(" \\\n", xfl);
	}
	if  (jp->spq_flags[0])  {
		spitstring($A{spr post proc flags}, xfl, 0);
		fprintf(xfl, "\'%s\' \\\n", jp->spq_flags);
	}
	if  (jp->spq_file[0])  {
		spitstring($A{spr header}, xfl, 0);
		fprintf(xfl, "\'%s\' \\\n", jp->spq_file);
	}
	if  (jp->spq_ptr[0])  {
		spitstring($A{spr printer}, xfl, 0);
		fprintf(xfl, "%s \\\n", jp->spq_ptr);
	}
	if  (strcmp(jp->spq_puname, jp->spq_uname) != 0)  {
		spitstring($A{spr post user}, xfl, 0);
		fprintf(xfl, "%s \\\n", jp->spq_puname);
	}
	spitstring($A{spr classcode}, xfl, 0);
	fprintf(xfl, "%s \\\n", hex_disp(jp->spq_class, 0));
	spitstring($A{spr formtype}, xfl, 0);
	fprintf(xfl, "%s \\\n", jp->spq_form);
}

void	dumpjob(const struct spq *jp)
{
	FILE	*ifl, *xfl, *jfl;
	unsigned	oldumask;
	int		ch;
#ifndef	HAVE_SETEUID
	uid_t		gid = getgid();
#endif
	char		*delim;
	unsigned	pagenums = 0;
	LONG		*pageoffsets = (LONG *) 0;
	struct	pages	pfe;

	if  (netid)
		ifl = net_feed(FEED_NPSP, netid, jp->spq_rslot, Jobnum);
	else
		ifl = fopen(mkspid(SPNAM, Jobnum), "r");

	if  (ifl == (FILE *) 0)
		exit(E_JDFNFND);

	rdpgfile(jp, &pfe, &delim, &pagenums, &pageoffsets);

#ifdef	HAVE_SETEUID
	seteuid(Realuid);
	if  (chdir(Dirname) < 0)
		exit(E_JDNOCHDIR);
	if  ((xfl = fopen(Xfile, "w")) == (FILE *) 0)
		exit(E_JDFNOCR);
	oldumask = umask(0);
	if  ((jfl = fopen(Jfile, "w")) == (FILE *) 0)
		exit(E_JDFNOCR);
	chmod(Xfile, (int) (0777 &~oldumask));
	seteuid(Daemuid);
#else
#ifdef	ID_SWAP
#if	defined(NHONSUID) || defined(DEBUG)
	if  (Daemuid != ROOTID  &&  Realuid != ROOTID  &&  Effuid != ROOTID)  {
#else
	if  (Daemuid != ROOTID  &&  Realuid != ROOTID)  {
#endif
		setuid(Realuid);
		if  (chdir(Dirname) < 0)
			exit(E_JDNOCHDIR);
		if  ((xfl = fopen(Xfile, "w")) == (FILE *) 0)
			exit(E_JDFNOCR);
		oldumask = umask(0);
		if  ((jfl = fopen(Jfile, "w")) == (FILE *) 0)
			exit(E_JDFNOCR);
		chmod(Xfile, (int) (0777 &~oldumask));
		setuid(Daemuid);
	}
	else  {
#endif	/* ID_SWAP */
		if  (chdir(Dirname) < 0)
			exit(E_JDNOCHDIR);
		if  ((xfl = fopen(Xfile, "w")) == (FILE *) 0)
			exit(E_JDFNOCR);
		oldumask = umask(0);
		if  ((jfl = fopen(Jfile, "w")) == (FILE *) 0)
			exit(E_JDFNOCR);
		chmod(Xfile, (int) (0777 &~oldumask));
#if	defined(HAVE_FCHOWN) && !defined(M88000)
		fchown(fileno(xfl), Realuid, gid);
		fchown(fileno(jfl), Realuid, gid);
#else
		chown(Xfile, Realuid, gid);
		chown(Jfile, Realuid, gid);
#endif
#ifdef	ID_SWAP
	}
#endif
#endif /* !HAVE_SETEUID */

	dumphdrs(jp, xfl, delim, &pfe);

	/* Fix for GTK so we can have job and command files in different places.
	   If abs path name don't put directory in front */

	if  (Jfile[0] == '/')
		fprintf(xfl, "%s\n", Jfile);
	else
		fprintf(xfl, "%s/%s\n", Dirname, Jfile);
	fclose(xfl);
	while  ((ch = getc(ifl)) != EOF)
		putc(ch, jfl);
	fclose(ifl);
	fclose(jfl);
}

void	deljob(const Hashspq *jp)
{
	struct	spr_req	oreq;
	if  (nodelete)
		return;
	oreq.spr_mtype = MT_SCHED;
	oreq.spr_un.o.spr_pid = getpid();
	oreq.spr_un.o.spr_act = SO_AB;
	oreq.spr_un.o.spr_netid = 0;
	oreq.spr_un.o.spr_jpslot = jp - Job_seg.jlist;
	oreq.spr_un.o.spr_jobno = jp->j.spq_job;
	oreq.spr_un.o.spr_arg1 = 0;
	oreq.spr_un.o.spr_arg2 = 0;
	msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), 0);
}

#include "inline/spr_adefs.c"

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char		*spdir, *colp;
	LONG		jind;
	const	Hashspq	*jp;
	HelpargRef	helpa;
	struct	spdet	*mypriv;
	char	*Realuname = (char *) 0;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.1 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

#ifndef	DEBUG
	freopen("/dev/null", "w", stderr);
#endif

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");
	helpa = helpargs(Adefs, $A{spr explain}, $A{spr wait time});
	makeoptvec(helpa, $A{spr explain}, $A{spr wait time});

	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	if  (!(mypriv = getspuentry(Realuid)))
		exit(E_UNOTSETUP);

	/* This is only called internally, so don't bother with messages.  */

	if  (argc != 5)  {
		if  (argc != 6  ||  strcmp(argv[1], "-n") != 0)
			exit(E_USAGE);
		nodelete = 1;
		argv++;
	}

	/* Unless we definitely have access to other user's jobs,
	   we need to grab the real user name for comparison.  */

	if  ((mypriv->spu_flgs & (PV_OTHERJ|PV_VOTHERJ)) != (PV_OTHERJ|PV_VOTHERJ))
		Realuname = prin_uname(Realuid);

	if  ((colp = strchr(argv[1], ':')))  {
		*colp = '\0';
		if  ((netid = look_hostname(argv[1])) == 0L)
			exit(E_USAGE);
		Jobnum = atol(colp+1);
	}
	else
		Jobnum = atol(argv[1]);
	Dirname = argv[2];
	Xfile = argv[3];
	Jfile = argv[4];

	spdir = envprocess(SPDIR);
	if  (chdir(spdir) < 0)
		exit(E_NOCHDIR);
	free(spdir);

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)
		exit(E_NOTRUN);

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif

	if  (!jobshminit(1))
		exit(E_JOBQ);

	rerjobfile();

	jobshm_lock();
	jind = Job_seg.hashp_jno[jno_jhash(Jobnum)];
	while  (jind >= 0L)  {
		jp = &Job_seg.jlist[jind];
		if  (jp->j.spq_job == Jobnum  &&  jp->j.spq_netid == netid)
			goto  gotit;
		jind = jp->nxt_jno_hash;
	}
	/* NB assumed locking sets SEM_UNDO */
	exit(E_JDJNFND);

 gotit:
	jobshm_unlock();
	if  ((mypriv->spu_flgs & (PV_OTHERJ|PV_VOTHERJ)) != (PV_OTHERJ|PV_VOTHERJ)  &&
	     strcmp(Realuname, jp->j.spq_uname) != 0  &&
	     (!(mypriv->spu_flgs & PV_VOTHERJ)  ||  !(nodelete  ||  mypriv->spu_flgs & PV_OTHERJ)))
		exit(E_NOPRIV);
	dumpjob(&jp->j);
	deljob(jp);
	return  0;
}
