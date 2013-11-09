/* spccgi.c -- CGI program for ops on printers

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
#include "xfershm.h"
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

#define	IPC_MODE	0600

char	*Realuname,
	*Curr_pwd;

struct	spdet	*mypriv;

/* Keep library happy */

void  nomem()
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

struct	argop  {
	const	char	*name;		/* Name of parameter case insens */
	int 	(*arg_fn)(struct spptr *, const struct argop *);
	short	typ;			/* Type of parameter */
#define	AO_BOOL		0
#define	AO_ULONG	1
#define	AO_STRING	2

	unsigned  char	off;			/* Turn off */
	unsigned  char	had;
	union  {
		USHORT		ao_boolbit;
		ULONG		ao_ulong;
		char		*ao_string;
	}  ao_un;
	struct  argop	*next;
};

int  arg_netflags(struct spptr *pp, const struct argop *ao)
{
	USHORT	bit = ao->ao_un.ao_boolbit;
	if  (ao->off)
		pp->spp_netflags &= ~bit;
	else
		pp->spp_netflags |= bit;
	return  1;
}

int  arg_class(struct spptr *pp, const struct argop *ao)
{
	ULONG	rcl = ao->ao_un.ao_ulong;
	if  (!(mypriv->spu_flgs & PV_COVER))
		rcl &= mypriv->spu_class;
	if  (rcl == 0)
		return  0;
	pp->spp_class = rcl;
	return  1;
}

int  arg_form(struct spptr *pp, const struct argop *ao)
{
	strncpy(pp->spp_form, ao->ao_un.ao_string, MAXFORM);
	return  1;
}

int  arg_dev(struct spptr *pp, const struct argop *ao)
{
	strncpy(pp->spp_dev, ao->ao_un.ao_string, LINESIZE);
	return  1;
}

int  arg_descr(struct spptr *pp, const struct argop *ao)
{
	strncpy(pp->spp_comment, ao->ao_un.ao_string, COMMENTSIZE);
	return  1;
}

struct	argop  aolist[] =  {
	{	"form",	arg_form,	AO_STRING	},
	{	"dev",	arg_dev,	AO_STRING	},
	{	"descr",arg_descr,	AO_STRING	},
	{	"network",arg_netflags,	AO_BOOL,	0,0,	{ (USHORT) SPP_LOCALNET }	},
	{	"loco",	arg_netflags,	AO_BOOL,	0,0,	{ (USHORT) SPP_LOCALONLY }	},
	{	"class",arg_class,	AO_ULONG	}
};

struct	argop	*aochain;

struct	actop  {
	char	*name;
	USHORT	proc_running;		/* 1 If to be applied to running processes */
	USHORT	act_code;
}  actlist[] =  {
	{	"start",	0,	SO_PGO	},
	{	"heoj",		1,	SO_PHLT	},
	{	"halt",		1,	SO_PSTP	},
	{	"alok",		1,	SO_OYES	},
	{	"alnok",	1,	SO_ONO	},
	{	"int",		1,	SO_INTER	},
	{	"pab",		1,	SO_PJAB	},
	{	"prst",		1,	SO_RSP	}
};


void  list_op(char *arg, char *cp)
{
	int	cnt;

	*cp = '\0';

	for  (cnt = 0;  cnt < sizeof(aolist) / sizeof(struct argop);  cnt++)  {
		struct  argop  *aop = &aolist[cnt];
		if  (ncstrcmp(aop->name, arg) == 0)  {
			*cp++ = '=';
			switch  (aop->typ)  {
			case  AO_BOOL:
				switch  (*cp)  {
				case  'y':case  'Y':
				case  't':case  'T':
					aop->off = 0;
					break;
				case  'n':case  'N':
				case  'f':case  'F':
					aop->off = 255;
					break;
				default:
					goto  badarg;
				}
				break;
			case  AO_ULONG:
				if  (!isdigit(*cp))
					goto  badarg;
				aop->ao_un.ao_ulong = strtoul(cp, (char **) 0, 0);
				break;
			case  AO_STRING:
				aop->ao_un.ao_string = cp;
				break;
			}
			if  (!aop->had)  {
				aop->had = 1;
				aop->next = aochain;
				aochain = aop;
			}
			return;
		}
	}

	*cp++ = '=';
 badarg:
	if  (html_out_cparam_file("badcarg", 1, arg))
		exit(E_USAGE);
	html_error(arg);
	exit(E_SETUP);
}

void  apply_ops(char *arg)
{
	const	Hashspptr	*hpp;
	const	struct  spptr	*pp;
	struct	argop		*aop;
	int			ret;
	struct	ptrswanted	pw;
	struct	spr_req		preq;
	struct	spptr		SPPTR;

	if  (!decode_pname(arg, &pw))  {
		if  (html_out_cparam_file("badcarg", 1, arg))
			exit(E_USAGE);
		html_error(arg);
		exit(E_SETUP);
	}
	if  (!(hpp = find_ptr(&pw)))  {
		html_out_cparam_file("ptrgone", 1, arg);
		exit(E_NOJOB);
	}
	pp = pw.pp;
	if  ((mypriv->spu_flgs & (PV_PRINQ|PV_HALTGO)) != (PV_PRINQ|PV_HALTGO) ||
	     (pp->spp_netid  && !(mypriv->spu_flgs & PV_REMOTEP)))  {
		html_out_cparam_file("nopriv", 1, arg);
		exit(E_NOPRIV);
	}

	if  (!aochain)		/* Nothing to do how boring */
		return;

	preq.spr_mtype = MT_SCHED;
	if  (pp->spp_state >= SPP_PROC)  {
		html_out_or_err("badstate", 1);
		exit(E_USAGE);
	}

	preq.spr_un.p.spr_pid = getpid();
	preq.spr_un.p.spr_act = SP_CHGP;
	preq.spr_un.p.spr_pslot = hpp - Ptr_seg.plist;
	preq.spr_un.p.spr_seq = 0;
	preq.spr_un.p.spr_netid = 0;
	SPPTR = *pp;		/* Want a read-write version to pass */
	for  (aop = aochain;  aop;  aop = aop->next)
		if  (!(*aop->arg_fn)(&SPPTR, aop))  {
			html_out_or_err("badargs", 1);
			exit(E_USAGE);
		}
	if  ((ret = wpmsg(&preq, &SPPTR)))  {
		html_disperror(ret);
		exit(E_SETUP);
	}
	waitsig();
}

void  apply_action(struct actop *aop, char *arg)
{
	const	Hashspptr	*hpp;
	const	struct  spptr	*pp;
	struct	ptrswanted	pw;
	struct	spr_req		preq;
	int	blkcount = MSGQ_BLOCKS;

	if  (!decode_pname(arg, &pw))  {
		if  (html_out_cparam_file("badcarg", 1, arg))
			exit(E_USAGE);
		html_error(arg);
		exit(E_SETUP);
	}
	if  (!(hpp = find_ptr(&pw)))  {
		html_out_cparam_file("ptrgone", 1, arg);
		exit(E_NOJOB);
	}
	pp = pw.pp;
	if  ((mypriv->spu_flgs & (PV_PRINQ|PV_HALTGO)) != (PV_PRINQ|PV_HALTGO) ||
	     (pp->spp_netid  && !(mypriv->spu_flgs & PV_REMOTEP)))  {
		html_out_cparam_file("nopriv", 1, arg);
		exit(E_NOPRIV);
	}

	preq.spr_mtype = MT_SCHED;
	if  (pp->spp_state >= SPP_PROC)  {
		if  (!aop->proc_running)
			goto  badstate;
	}
	else  if  (aop->proc_running)
		goto  badstate;

	preq.spr_un.o.spr_pid = getpid();
	preq.spr_un.o.spr_jpslot = hpp - Ptr_seg.plist;
	preq.spr_un.o.spr_act = aop->act_code;
	preq.spr_un.o.spr_arg1 = Realuid;
	preq.spr_un.o.spr_arg2 = 0;
	preq.spr_un.o.spr_jobno = 0;
	preq.spr_un.o.spr_seq = 0;
	preq.spr_un.o.spr_netid = 0;

	while  (msgsnd(Ctrl_chan, (struct msgbuf *) &preq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)  {
		if  (errno != EAGAIN)  {
			html_disperror($E{IPC msg q error});
			exit(E_SETUP);
		}
		blkcount--;
		if  (blkcount <= 0)  {
			html_disperror($E{IPC msg q full});
			exit(E_SETUP);
		}
		sleep(MSGQ_BLOCKWAIT);
	}
	return;

badstate:
	html_out_or_err("badstate", 1);
	exit(E_USAGE);
}

void  perform_update(char **args)
{
	char	**ap = args, *arg = *ap;

	if  (!arg)
		return;

	if  (!strchr(arg, '='))  {
		int	cnt;
		for  (cnt = 0;  cnt < sizeof(actlist)/sizeof(struct actop);  cnt++)  {
			if  (ncstrcmp(actlist[cnt].name, arg) == 0)  {
				for  (ap++;  (arg = *ap);  ap++)
					apply_action(&actlist[cnt],  arg);
				return;
			}
		}
		/* Drop through... */
	}

	for  (;  (arg = *ap);  ap++)  {
		char	*cp = strchr(arg, '=');
		if  (cp)
			list_op(arg, cp);
		else
			apply_ops(arg);
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

	versionprint(argv, "$Revision: 1.9 $", 0);

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
		return E_NOTRUN;
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

	if  ((ec = init_xfershm(0)))  {
		html_disperror(ec);
		return  E_SETUP;
	}
	if  (!jobshminit(0))  {
		html_disperror($E{Cannot open jshm});
		return  E_JOBQ;
	}
	if  (!ptrshminit(0))  {
		html_disperror($E{Cannot open pshm});
		return  E_PRINQ;
	}

	if  ((ec = msg_log(SO_MON, 1)) != 0)  {
		html_disperror(ec);
		return  E_SETUP;
	}
	perform_update(newargs);
	if  ((ec = msg_log(SO_DMON, 0)) != 0)  {
		html_disperror(ec);
		return  E_SETUP;
	}
	html_out_or_err("chngok", 1);
	return  0;
}
