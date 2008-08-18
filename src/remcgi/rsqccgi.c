/* rsqccgi.c -- remote CGI commands

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

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "gspool.h"
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <errno.h>
#include "network.h"
#include "ecodes.h"
#include "errnums.h"
#include "files.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "rcgilib.h"

#define	LOTSANDLOTS	99999999L	/* Maximum page number */

uid_t	Daemuid,
	Realuid,
	Effuid;

FILE	*Cfile;

int	gspool_fd;
char	*realuname;
struct	apispdet	mypriv;
int			Njobs, Nptrs;
struct	apispq		*job_list;
slotno_t		*jslot_list;
struct	ptr_with_slot	*ptr_sl_list;

/* Keep library happy */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

struct	argop  {
	const	char	*name;		/* Name of parameter case insens */
	int	(*arg_fn)(struct apispq *, const struct argop *);
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

int	arg_sp(struct apispq *jp, const struct argop *ao)
{
	jp->apispq_start = ao->ao_un.ao_ulong != 0? ao->ao_un.ao_ulong - 1: 0;
	return  1;
}

int	arg_ep(struct apispq *jp, const struct argop *ao)
{
	jp->apispq_end = ao->ao_un.ao_ulong != 0? ao->ao_un.ao_ulong - 1: LOTSANDLOTS;
	return  1;
}

int	arg_hatp(struct apispq *jp, const struct argop *ao)
{
	jp->apispq_haltat = ao->ao_un.ao_ulong != 0? ao->ao_un.ao_ulong - 1: 0;
	return  1;
}

int	arg_cps(struct apispq *jp, const struct argop *ao)
{
	unsigned  nn = ao->ao_un.ao_uchar;
	if  (!(mypriv.spu_flgs & PV_ANYPRIO)  &&  nn > (unsigned) mypriv.spu_cps)
		return  0;
	jp->apispq_cps = nn;
	return  1;
}

int	arg_pri(struct apispq *jp, const struct argop *ao)
{
	unsigned  nn = ao->ao_un.ao_uchar;

	if  (!(mypriv.spu_flgs & PV_CPRIO)  ||
	     (!(mypriv.spu_flgs & PV_ANYPRIO)  &&
	      (nn < (unsigned) mypriv.spu_minp || nn > (unsigned) mypriv.spu_maxp)))
		return  0;
	jp->apispq_pri = nn;
	return  1;
}

int	arg_jflags(struct apispq *jp, const struct argop *ao)
{
	USHORT	bit = ao->ao_un.ao_boolbit;
	if  (ao->off)
		jp->apispq_jflags &= ~bit;
	else
		jp->apispq_jflags |= bit;
	return  1;
}

int	arg_dflags(struct apispq *jp, const struct argop *ao)
{
	USHORT	bit = ao->ao_un.ao_boolbit;
	if  (ao->off)
		jp->apispq_dflags &= ~bit;
	else
		jp->apispq_dflags |= bit;
	return  1;
}

int	arg_class(struct apispq *jp, const struct argop *ao)
{
	ULONG	rcl = ao->ao_un.ao_ulong;
	if  (!(mypriv.spu_flgs & PV_COVER))
		rcl &= mypriv.spu_class;
	if  (rcl == 0)
		return  0;
	jp->apispq_class = rcl;
	return  1;
}

int	arg_hdr(struct apispq *jp, const struct argop *ao)
{
	strncpy(jp->apispq_file, ao->ao_un.ao_string, MAXTITLE);
	return  1;
}

int	arg_flags(struct apispq *jp, const struct argop *ao)
{
	strncpy(jp->apispq_flags, ao->ao_un.ao_string, MAXFLAGS);
	return  1;
}

int	arg_form(struct apispq *jp, const struct argop *ao)
{
	if  (!((mypriv.spu_flgs & PV_FORMS) || qmatch(mypriv.spu_formallow, ao->ao_un.ao_string)))
		return  0;
	strncpy(jp->apispq_form, ao->ao_un.ao_string, MAXFORM);
	return  1;
}

int	arg_ptr(struct apispq *jp, const struct argop *ao)
{
	if  (!((mypriv.spu_flgs & PV_OTHERP)  ||  issubset(mypriv.spu_ptrallow, ao->ao_un.ao_string)))
		return  0;
	strncpy(jp->apispq_ptr, ao->ao_un.ao_string, JPTRNAMESIZE);
	return  1;
}

int	arg_hold(struct apispq *jp, const struct argop *ao)
{
	time_t	ht = ao->ao_un.ao_time, now = time((time_t *) 0);
	if  (ht < now)
		return  0;
	jp->apispq_hold = (LONG) ht;
	return  1;
}

int	arg_npt(struct apispq *jp, const struct argop *ao)
{
	USHORT	us = ao->ao_un.ao_ushort;
	if  (us == 0)
		return  0;
	jp->apispq_nptimeout = us;
	return  1;
}

int	arg_pt(struct apispq *jp, const struct argop *ao)
{
	USHORT	us = ao->ao_un.ao_ushort;
	if  (us == 0)
		return  0;
	jp->apispq_ptimeout = us;
	return  1;
}

int	arg_puser(struct apispq *jp, const struct argop *ao)
{
	strncpy(jp->apispq_puname, prin_uname(ao->ao_un.ao_user), UIDSIZE);
	return  1;
}

struct	argop  aolist[] =  {
	{	"sp",	arg_sp,		AO_ULONG	},
	{	"ep",	arg_ep,		AO_ULONG	},
	{	"hatp",	arg_hatp,	AO_ULONG	},
	{	"cps",	arg_cps,	AO_UCHAR	},
	{	"pri",	arg_pri,	AO_UCHAR	},
	{	"noh",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_NOH }	},
	{	"hdr",	arg_hdr,	AO_STRING	},
	{	"form",	arg_form,	AO_STRING	},
	{	"ptr",	arg_ptr,	AO_STRING	},
	{	"npto",	arg_npt,	AO_USHORT	},
	{	"pto",	arg_pt,		AO_USHORT	},
	{	"wrt",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_WRT }	},
	{	"mail",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_MAIL }	},
	{	"retn",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_RETN }	},
	{	"oddp",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_ODDP }	},
	{	"evenp",arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_EVENP }	},
	{	"revoe",arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_REVOE }	},
	{	"mattn",arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_MATTN }	},
	{	"wattn",arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_WATTN }	},
	{	"loco",	arg_jflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_LOCALONLY }	},
	{	"printed",arg_dflags,	AO_BOOL,	0,0,	{ (USHORT) APISPQ_PRINTED  }	},
	{	"class",arg_class,	AO_ULONG	},
	{	"hold", arg_hold,	AO_TIME		},
	{	"puser",arg_puser,	AO_USER	},
	{	"flags",arg_flags,	AO_STRING	}
};

struct	argop	*aochain;

void	list_op(char *arg, char *cp)
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

void	apply_ops(char * arg)
{
	int			ret;
	struct	argop		*aop;
	struct	jobswanted	jw;
	struct	apispq		job;

	if  (decode_jnum(arg, &jw))  {
		if  (html_out_cparam_file("badcarg", 1, arg))
			exit(E_USAGE);
		html_error(arg);
		exit(E_SETUP);
	}
	if  ((ret = gspool_jobfind(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.jno, jw.host, &jw.slot, &job)) < 0)  {
		if  (ret == GSPOOL_UNKNOWN_JOB)
			html_out_cparam_file("jobgone", 1, arg);
		else
			html_disperror($E{Base for API errors} + ret);
		exit(E_NOJOB);
	}
	if  ((!(mypriv.spu_flgs & PV_OTHERJ)  &&  strcmp(realuname, job.apispq_uname) != 0) ||
	     (job.apispq_netid != dest_hostid  && !(mypriv.spu_flgs & PV_REMOTEJ)))  {
		html_out_cparam_file("nopriv", 1, arg);
		exit(E_NOPRIV);
	}

	if  (!aochain)		/* Nothing to do how boring */
		return;

	for  (aop = aochain;  aop;  aop = aop->next)
		if  (!(*aop->arg_fn)(&job, aop))  {
			html_out_or_err("badargs", 1);
			exit(E_USAGE);
		}

	if  ((ret = gspool_jobupd(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.slot, &job)) < 0)  {
		html_disperror($E{Base for API errors} + ret);
		exit(E_NOPRIV);
	}
}

void	perform_update(char ** args)
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
	int_ugid_t	chku;

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();
	html_openini();
	hash_hostfile();
	Effuid = geteuid();
	if  ((chku = lookup_uname(SPUNAME)) == UNKNOWN_UID)
		Daemuid = ROOTID;
	else
		Daemuid = chku;
	newargs = cgi_arginterp(argc, argv, CGI_AI_REMHOST|CGI_AI_SUBSID);
	/* Side effect of cgi_arginterp is to set Realuid */
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");
	realuname = prin_uname(Realuid);
	setgid(getgid());
	setuid(Realuid);
	api_open(realuname);
	perform_update(newargs);
	html_out_or_err("chngok", 1);
	return  0;
}
