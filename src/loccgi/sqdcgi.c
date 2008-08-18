/* sqdcgi.c -- delete jobs CGI program

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
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <errno.h>
#include <setjmp.h>
#include "incl_sig.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "pages.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "listperms.h"
#include "cgifndjb.h"
#include "shutilmsg.h"
#ifdef	SHAREDLIBS
#include "xfershm.h"
#endif

uid_t	Daemuid,
	Realuid,
	Effuid;

DEF_DISPOPTS;

struct	spdet	*mypriv;

char	*Realuname;

FILE	*Cfile;

#define	IPC_MODE	0600

int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

#ifdef	SHAREDLIBS
struct	xfershm		*Xfer_shmp;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

void	perform_delete(char ** args)
{
	char	*arg;
	struct	spr_req	oreq;

	oreq.spr_mtype = MT_SCHED;
	oreq.spr_un.o.spr_act = SO_AB;
	oreq.spr_un.o.spr_pid = getpid();
	oreq.spr_un.o.spr_arg1 = Realuid;
	oreq.spr_un.o.spr_arg2 = 0;
	oreq.spr_un.o.spr_seq = 0;
	oreq.spr_un.o.spr_netid = 0;

	for  (;  (arg = *args);  args++)  {
		struct	jobswanted	jw;
		const	Hashspq		*hjp;
		const	struct	spq	*jp;

		if  (decode_jnum(arg, &jw))  {
			html_out_or_err("sbadargs", 1);
			exit(E_USAGE);
		}

		if  (!(hjp = find_job(&jw)))  {
			html_out_cparam_file("jobgone", 1, arg);
			exit(E_NOJOB);
		}

		jp = jw.jp;
		if  ((!(mypriv->spu_flgs & PV_OTHERJ)  &&  strcmp(Realuname, jp->spq_uname) != 0) ||
		     (jp->spq_netid  && !(mypriv->spu_flgs & PV_REMOTEJ)))  {
			html_out_cparam_file("nopriv", 1, arg);
			exit(E_NOPRIV);
		}

		oreq.spr_un.o.spr_jobno = jp->spq_job;
		oreq.spr_un.o.spr_jpslot = hjp - Job_seg.jlist;

		if  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)  {
			html_disperror(errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error});
			exit(E_SETUP);
		}
		waitsig();
	}
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char	**newargs;
	int	ec;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.1 $", 0);
	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();
	html_openini();
	newargs = cgi_arginterp(argc, argv, 1); /* Side effect of cgi_arginterp is to set Realuid */
	Effuid = geteuid();
	INIT_DAEMUID;

	Cfile = open_cfile(MISC_UCONFIG, "rest.help");

	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	Realuname = prin_uname(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;

	/* Now we want to be Daemuid throughout if possible.  */

#if  	defined(OS_BSDI) || defined(OS_FREEBSD)
	seteuid(Daemuid);
#else
	setuid(Daemuid);
#endif

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		html_disperror($E{Spooler not running});
		return  E_NOTRUN;
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		html_disperror($E{Cannot open semaphore});
		return  E_SETUP;
	}
#endif

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	if  (!jobshminit(0))  {
		html_disperror($E{Cannot open jshm});
		return  E_JOBQ;
	}
	readjoblist(1);
	if  ((ec = msg_log(SO_MON, 1)) != 0)  {
		html_disperror(ec);
		return  E_SETUP;
	}
	perform_delete(newargs);
	if  ((ec = msg_log(SO_DMON, 0)) != 0)  {
		html_disperror(ec);
		return  E_SETUP;
	}
	html_out_or_err("delok", 1);
	return  0;
}
