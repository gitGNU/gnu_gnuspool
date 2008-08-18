/* sqccgi.c -- job operations CGI program

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

int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;

struct	xfershm		*Xfer_shmp;

uid_t	Daemuid,
	Realuid,
	Effuid;

DEF_DISPOPTS;
FILE	*Cfile;

char	*Realuname,
	*Curr_pwd;

struct	spdet	*mypriv;

/* Keep library happy */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

struct	argop  {
	const	char	*name;		/* Name of parameter case insens */
	int	(*arg_fn)(struct spq *, const struct argop *);
	short	typ;			/* Type of parameter */
#define	AO_BOOL		0
#define	AO_UCHAR	1
#define	AO_USHORT	2
#define	AO_ULONG	3
#define	AO_TIME		4
#define	AO_STRING	5
#define	AO_USER		6
	unsigned  char	off;			/* Turn off */
	unsigned  char	had;
	union  {
		USHORT		ao_boolbit;
		unsigned  char	ao_uchar;
		USHORT		ao_ushort;
		ULONG		ao_ulong;
		char		*ao_string;
		int_ugid_t	ao_user;
		time_t		ao_time;
	}  ao_un;
	struct  argop	*next;
};

int	arg_sp(struct spq *jp, const struct argop *ao)
{
	jp->spq_start = ao->ao_un.ao_ulong != 0? ao->ao_un.ao_ulong - 1: 0;
	return  1;
}

int	arg_ep(struct spq *jp, const struct argop *ao)
{
	jp->spq_end = ao->ao_un.ao_ulong != 0? ao->ao_un.ao_ulong - 1: LOTSANDLOTS;
	return  1;
}

int	arg_hatp(struct spq *jp, const struct argop *ao)
{
	jp->spq_haltat = ao->ao_un.ao_ulong != 0? ao->ao_un.ao_ulong - 1: 0;
	return  1;
}

int	arg_cps(struct spq *jp, const struct argop *ao)
{
	unsigned  nn = ao->ao_un.ao_uchar;
	if  (!(mypriv->spu_flgs & PV_ANYPRIO)  &&  nn > (unsigned) mypriv->spu_cps)
		return  0;
	jp->spq_cps = nn;
	return  1;
}

int	arg_pri(struct spq *jp, const struct argop *ao)
{
	unsigned  nn = ao->ao_un.ao_uchar;

	if  (!(mypriv->spu_flgs & PV_CPRIO)  ||
	     (!(mypriv->spu_flgs & PV_ANYPRIO)  &&
	      (nn < (unsigned) mypriv->spu_minp || nn > (unsigned) mypriv->spu_maxp)))
		return  0;
	jp->spq_pri = nn;
	return  1;
}

int	arg_jflags(struct spq *jp, const struct argop *ao)
{
	USHORT	bit = ao->ao_un.ao_boolbit;
	if  (ao->off)
		jp->spq_jflags &= ~bit;
	else
		jp->spq_jflags |= bit;
	return  1;
}

int	arg_dflags(struct spq *jp, const struct argop *ao)
{
	USHORT	bit = ao->ao_un.ao_boolbit;
	if  (ao->off)
		jp->spq_dflags &= ~bit;
	else
		jp->spq_dflags |= bit;
	return  1;
}

int	arg_class(struct spq *jp, const struct argop *ao)
{
	ULONG	rcl = ao->ao_un.ao_ulong;
	if  (!(mypriv->spu_flgs & PV_COVER))
		rcl &= mypriv->spu_class;
	if  (rcl == 0)
		return  0;
	jp->spq_class = rcl;
	return  1;
}

int	arg_hdr(struct spq *jp, const struct argop *ao)
{
	strncpy(jp->spq_file, ao->ao_un.ao_string, MAXTITLE);
	return  1;
}

int	arg_flags(struct spq *jp, const struct argop *ao)
{
	strncpy(jp->spq_flags, ao->ao_un.ao_string, MAXFLAGS);
	return  1;
}

int	arg_form(struct spq *jp, const struct argop *ao)
{
	if  (!((mypriv->spu_flgs & PV_FORMS) || qmatch(mypriv->spu_formallow, ao->ao_un.ao_string)))
		return  0;
	strncpy(jp->spq_form, ao->ao_un.ao_string, MAXFORM);
	return  1;
}

int	arg_ptr(struct spq *jp, const struct argop *ao)
{
	if  (!((mypriv->spu_flgs & PV_OTHERP)  ||  issubset(mypriv->spu_ptrallow, ao->ao_un.ao_string)))
		return  0;
	strncpy(jp->spq_ptr, ao->ao_un.ao_string, JPTRNAMESIZE);
	return  1;
}

int	arg_hold(struct spq *jp, const struct argop *ao)
{
	time_t	ht = ao->ao_un.ao_time, now = time((time_t *) 0);
	if  (ht < now)
		return  0;
	jp->spq_hold = (LONG) ht;
	return  1;
}

int	arg_npt(struct spq *jp, const struct argop *ao)
{
	USHORT	us = ao->ao_un.ao_ushort;
	if  (us == 0)
		return  0;
	jp->spq_nptimeout = us;
	return  1;
}

int	arg_pt(struct spq *jp, const struct argop *ao)
{
	USHORT	us = ao->ao_un.ao_ushort;
	if  (us == 0)
		return  0;
	jp->spq_ptimeout = us;
	return  1;
}

int	arg_puser(struct spq *jp, const struct argop *ao)
{
	strncpy(jp->spq_puname, prin_uname(ao->ao_un.ao_user), UIDSIZE);
	return  1;
}

struct	argop  aolist[] =  {
	{	"sp",	arg_sp,		AO_ULONG	},
	{	"ep",	arg_ep,		AO_ULONG	},
	{	"hatp",	arg_hatp,	AO_ULONG	},
	{	"cps",	arg_cps,	AO_UCHAR	},
	{	"pri",	arg_pri,	AO_UCHAR	},
	{	"noh",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_NOH }	},
	{	"hdr",	arg_hdr,	AO_STRING	},
	{	"form",	arg_form,	AO_STRING	},
	{	"ptr",	arg_ptr,	AO_STRING	},
	{	"npto",	arg_npt,	AO_USHORT	},
	{	"pto",	arg_pt,		AO_USHORT	},
	{	"wrt",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_WRT }	},
	{	"mail",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_MAIL }	},
	{	"retn",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_RETN }	},
	{	"oddp",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_ODDP }	},
	{	"evenp",arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_EVENP }	},
	{	"revoe",arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_REVOE }	},
	{	"mattn",arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_MATTN }	},
	{	"wattn",arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_WATTN }	},
	{	"loco",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_LOCALONLY }	},
	{	"printed",arg_dflags,	AO_BOOL,	0,0,	{ (USHORT) SPQ_PRINTED  }	},
	{	"class",arg_class,	AO_ULONG	},
	{	"hold", arg_hold,	AO_TIME		},
	{	"puser",arg_puser,	AO_USER	},
	{	"flags",arg_flags,	AO_STRING	}
};

struct	argop	*aochain;

void	list_op(char *arg, char * cp)
{
	int	cnt;
 
	*cp = '\0';

	for  (cnt = 0;  cnt < sizeof(aolist) / sizeof(struct argop);  cnt++)  {
		struct  argop  *aop = &aolist[cnt];
		unsigned  long	res;
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
			case  AO_UCHAR:
				if  (!isdigit(*cp))
					goto  badarg;
				res = strtoul(cp, (char **) 0, 0);
				if  (res > 255)
					goto  badarg;
				aop->ao_un.ao_uchar = res;
				break;
			case  AO_USHORT:
				if  (!isdigit(*cp))
					goto  badarg;
				res = strtoul(cp, (char **) 0, 0);
				if  (res > 0xffff)
					goto  badarg;
				aop->ao_un.ao_ushort = res;
				break;
			case  AO_ULONG:
				if  (!isdigit(*cp))
					goto  badarg;
				aop->ao_un.ao_ulong = strtoul(cp, (char **) 0, 0);
				break;
			case  AO_TIME:
				aop->ao_un.ao_time = strtol(cp, (char **) 0, 0);
				break;
			case  AO_STRING:
				aop->ao_un.ao_string = cp;
				break;
			case  AO_USER:
				if  ((aop->ao_un.ao_user = lookup_uname(cp)) == UNKNOWN_UID)
					aop->ao_un.ao_user = Realuid;
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

void	apply_ops(char *arg)
{
	const	Hashspq		*hjp;
	const	struct  spq	*jp;
	struct	argop		*aop;
	int			ret;
	struct	jobswanted	jw;
	struct	spr_req		jreq;
	struct	spq		SPQ;

	if  (decode_jnum(arg, &jw))  {
		if  (html_out_cparam_file("badcarg", 1, arg))
			exit(E_USAGE);
		html_error(arg);
		exit(E_SETUP);
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

	if  (!aochain)		/* Nothing to do how boring */
		return;

	jreq.spr_mtype = MT_SCHED;
	jreq.spr_un.j.spr_act = SJ_CHNG;
	jreq.spr_un.j.spr_pid = getpid();
	jreq.spr_un.j.spr_jslot = hjp - Job_seg.jlist;
	jreq.spr_un.j.spr_seq = 0;
	jreq.spr_un.j.spr_netid = 0;
	SPQ = *jp;
	for  (aop = aochain;  aop;  aop = aop->next)
		if  (!(*aop->arg_fn)(&SPQ, aop))  {
			html_out_or_err("badargs", 1);
			exit(E_USAGE);
		}
	if  ((ret = wjmsg(&jreq, &SPQ)))  {
		html_disperror(ret);
		exit(E_SETUP);
	}
	waitsig();
}

void	perform_update(char **args)
{
	char	**ap, *arg;

	for  (ap = args;  (arg = *ap);  ap++)  {
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
	readjoblist(1);
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
