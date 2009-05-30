/* sqdel.c -- delete/unqueue spool jobs

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
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <errno.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "pages.h"
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"
#include "cgifndjb.h"
#include "shutilmsg.h"
#include "xfershm.h"

#define	MAXLONG	0x7fffffffL	/*  Change this?  */

#define	HTIME	5		/* Forge prompt if one doesn't come */

int	spitoption(const int, const int, FILE *, const int, const int);
int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

extern	char	freeze_wanted;
char	freeze_cd,
	freeze_hd,
	dset,			/* Directory set (for freeze options) */
	unqueue,		/* Unqueue job */
	nodel;			/* Do not delete */

DEF_DISPOPTS;

char	*Curr_pwd,		/* PWD on entry */
	*Olddir,		/* Directory to send to */
	*jobprefix,		/* Prefix for job file */
	*cmdprefix;		/* Prefix for command file */

FILE	*Cfile;

#define	IPC_MODE	0600

#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;

char	*Realuname;

struct	spdet	*mypriv;

int	exit_code,
	force;

struct	xfershm		*Xfer_shmp;

/* Keep library happy */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

/* "Read" job file.  */

void	rjobfile(void)
{
	jobshm_lock();
#ifdef	USING_MMAP
	if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)
#else
	if  (Job_seg.dinf.base != Job_seg.dptr->js_did)
#endif
		jobgrown();
	Job_seg.Last_ser = Job_seg.dptr->js_serial;
	jobshm_unlock();
}

/* Run job dump program if possible */

static	void	dounqueue(const struct spq *jp)
{
	PIDTYPE	pid;
	int	ac;
	char	*argv[7], *udprog, jnobuf[HOSTNSIZE+10], cprefbuf[30], jprefbuf[30];

	if  ((pid = fork()))  {
		int	status;

		if  (pid < 0)  {
			print_error($E{Unqueue no fork});
			return;
		}
#ifdef	HAVE_WAITPID
		while  (waitpid(pid, &status, 0) < 0)
			;
#else
		while  (wait(&status) != pid)
			;
#endif
		if  (status == 0)	/* All ok */
			return;
		if  (status & 0xff)  {
			disp_arg[9] = status & 0xff;
			print_error($E{Unqueue program fault});
			return;
		}
		status = (status >> 8) & 0xff;
		disp_arg[0] = jp->spq_job;
		disp_str = (char *) jp->spq_file;
		switch  (status)  {
		case  E_SETUP:
			print_error($E{Cannot find unqueue});
			return;
		default:
			disp_arg[1] = status;
			print_error($E{Unqueue misc error});
			return;
		case  E_JDFNFND:
			print_error($E{Unqueue spool not found});
			return;
		case  E_JDNOCHDIR:
			disp_str2 = Olddir;
			print_error($E{Unqueue dir not found});
			return;
		case  E_JDFNOCR:
			disp_str2 = Olddir;
			print_error($E{Unqueue no create});
			return;
		}
	}

	/* Child process */

	udprog = envprocess(DUMPJOB);
	setuid(Realuid);
	chdir(Curr_pwd);	/* So that it picks up config file correctly */
	if  (jp->spq_netid)
		sprintf(jnobuf, "%s:%ld", look_host(jp->spq_netid), (long) jp->spq_job);
	else
		sprintf(jnobuf, "%ld", (long) jp->spq_job);
	sprintf(cprefbuf, "%s%.6ld", cmdprefix, (long) jp->spq_job);
	sprintf(jprefbuf, "%s%.6ld", jobprefix, (long) jp->spq_job);
	if  (!(argv[0] = strrchr(udprog, '/')))
		argv[0] = udprog;
	else
		argv[0]++;
	ac = 0;
	if  (nodel)
		argv[++ac] = "-n";
	argv[++ac] = jnobuf;
	argv[++ac] = Olddir;
	argv[++ac] = cprefbuf;
	argv[++ac] = jprefbuf;
	argv[++ac] = (char *) 0;
	execv(udprog, argv);
	exit(E_SETUP);
}

/* This is the main processing routine.  */

void	process(char **joblist)
{
	char	*jobc;
	struct	spr_req	oreq;

	oreq.spr_mtype = MT_SCHED;
	oreq.spr_un.o.spr_act = SO_AB;
	oreq.spr_un.o.spr_pid = getpid();
	oreq.spr_un.o.spr_arg1 = Realuid;
	oreq.spr_un.o.spr_arg2 = 0;
	oreq.spr_un.o.spr_seq = 0;
	oreq.spr_un.o.spr_netid = 0;

	while  ((jobc = *joblist++))  {
		const  Hashspq		*hjp;
		const  struct  spq	*jp;
		int			ret;
		struct	jobswanted	jw;

		disp_str = jobc;	/* What we're wingeing about in case of any error */

		if  ((ret = decode_jnum(jobc, &jw)) != 0)  {
			print_error(ret);
			exit_code = E_NOJOB;
			continue;
		}

		if  (jw.host  &&  !(mypriv->spu_flgs & PV_REMOTEJ))  {
			print_error($E{sqdel no remote job priv});
			exit_code = E_NOPRIV;
			continue;
		}

		if  (!(hjp = find_job(&jw)))  {
			print_error($E{Unknown job number});
			exit_code = E_NOJOB;
			continue;
		}
		jp = &hjp->j;

		if  (!(mypriv->spu_flgs & PV_OTHERJ)  &&  strcmp(Realuname, jp->spq_uname) != 0)  {
			print_error($E{Chngdel not yours});
			exit_code = E_NOPRIV;
			continue;
		}

		if  (unqueue)  {
			dounqueue(jp);
			continue;
		}

		if  (!(force  || (jp->spq_dflags & SPQ_PRINTED)))  {
			print_error($E{sqdel not printed});
			exit_code = E_FALSE;
			continue;
		}

		oreq.spr_un.o.spr_jobno = jp->spq_job;
		oreq.spr_un.o.spr_jpslot = hjp - Job_seg.jlist;

		if  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)  {
			print_error(errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error});
			exit(E_SETUP);
		}
		waitsig();
	}
}

OPTION(o_explain)
{
	print_error($E{sqdel options});
	exit(0);
}

OPTION(o_noforce)
{
	force = 0;
	return  OPTRESULT_OK;
}

OPTION(o_force)
{
	force = 1;
	return  OPTRESULT_OK;
}

OPTION(o_nounqueue)
{
	unqueue = 0;
	return  OPTRESULT_OK;
}

OPTION(o_unqueue)
{
	unqueue = 1;
	return  OPTRESULT_OK;
}

OPTION(o_nodel)
{
	nodel = 1;
	return  OPTRESULT_OK;
}

OPTION(o_del)
{
	nodel = 0;
	return  OPTRESULT_OK;
}

OPTION(o_jobprefix)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	free(jobprefix);
	jobprefix = stracpy(arg);
	return  OPTRESULT_ARG_OK;
}

OPTION(o_cmdprefix)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	free(cmdprefix);
	cmdprefix = stracpy(arg);
	return  OPTRESULT_ARG_OK;
}

OPTION(o_directory)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	Olddir = stracpy(arg);
	dset = 1;
	return	OPTRESULT_ARG_OK;
}

#include "inline/o_classc.c"
#include "inline/o_freeze.c"

/* Defaults and proc table for arg interp.  */

static	const	Argdefault	Adefs[] = {
	{  '?', $A{sqdel explain}	},
	{  'n', $A{sqdel keep unp}	},
	{  'N', $A{sqdel keep unp}	},
	{  'y', $A{sqdel del unp}	},
	{  'Y', $A{sqdel del unp}	},
	{  'C', $A{sqdel classcode}	},
	{  'u', $A{sqdel unqueue}	},
	{  'e', $A{sqdel no unqueue}	},
	{  'k', $A{sqdel no delete}	},
	{  'd', $A{sqdel delete}	},
	{  'J', $A{sqdel job pref}	},
	{  'S', $A{sqdel cmd pref}	},
	{  'D', $A{sqdel directory}	},
	{ 0, 0 }
};

optparam  optprocs[] = {
o_explain,	o_noforce,	o_force,	o_classcode,
o_unqueue,	o_nounqueue,	o_nodel,	o_del,
o_jobprefix,	o_cmdprefix,	o_directory,
o_freezecd,	o_freezehd
};

void	spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;
	char	*fmt = " %s";
	fprintf(dest, "%s", name);
	cancont = spitoption(force? $A{sqdel del unp}: $A{sqdel keep unp}, $A{sqdel explain}, dest, '=', 0);
	cancont = spitoption(nodel? $A{sqdel no delete}: $A{sqdel delete}, $A{sqdel explain}, dest, ' ', cancont);
	cancont = spitoption(unqueue? $A{sqdel unqueue}: $A{sqdel no unqueue}, $A{sqdel explain}, dest, ' ', cancont);
	spitoption($A{sqdel classcode}, $A{sqdel explain}, dest, ' ', 0);
	fprintf(dest, fmt, hex_disp(Displayopts.opt_classcode, 0));
	spitoption($A{sqdel job pref}, $A{sqdel explain}, dest, ' ', 0);
	fprintf(dest, fmt, jobprefix);
	spitoption($A{sqdel cmd pref}, $A{sqdel explain}, dest, ' ', 0);
	fprintf(dest, fmt, cmdprefix);
	if  (dset)  {
		spitoption($A{sqdel directory}, $A{sqdel explain}, dest, ' ', 0);
		fprintf(dest, fmt, Olddir);
	}
	putc('\n', dest);
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	int	ec;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.2 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");

	jobprefix = gprompt($P{sqdel default job prefix});
	cmdprefix = gprompt($P{sqdel default cmd prefix});

	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	if  ((mypriv->spu_flgs & (PV_OTHERJ|PV_VOTHERJ)) != (PV_OTHERJ|PV_VOTHERJ))
		Realuname = prin_uname(Realuid);
	SWAP_TO(Realuid);
	argv = optprocess(argv, Adefs, optprocs, $A{sqdel explain}, $A{sqdel freeze home}, 0);
	if  (unqueue)  {
		if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
			Curr_pwd = runpwd();
		if  (!Olddir)
			Olddir = Curr_pwd;
	}
	SWAP_TO(Daemuid);

#define	FREEZE_EXIT
#include "inline/freezecode.c"

	/* Winge now if no unqueue priv as we might as well let the
	   guy bung things in the .gnuspool file if he wants.  */

	if  (unqueue  &&  !(mypriv->spu_class & PV_UNQUEUE))  {
		print_error($E{sqdel cannot unqueue});
		return  E_NOPRIV;
	}

	if  (argv[0] == (char *) 0)  {
		print_error($E{sqdel no jobs});
		return  E_USAGE;
	}

	/* Grab message id */

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		print_error($E{Spooler not running});
		return  E_NOTRUN;
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		return  E_SETUP;
	}
#endif

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	if  (!jobshminit(0))  {
		print_error($E{Cannot open jshm});
		return  E_JOBQ;
	}
	if  ((ec = msg_log(SO_MON, 1)) != 0)  {
		print_error(ec);
		return  E_SETUP;
	}
	rjobfile();
	process(&argv[0]);
	if  ((ec = msg_log(SO_DMON, 0)) != 0)  {
		print_error(ec);
		return  E_SETUP;
	}
	return  exit_code;
}
