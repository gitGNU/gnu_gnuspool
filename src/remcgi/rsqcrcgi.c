/* rsqcrcgi.c -- remote CGI create jobs

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

#define	MAXLONG	0x7fffffffL	/*  Change this?  */
#define	QNPTIMEOUT	(7*24)		/* One week if not printed */
#define	QPTIMEOUT	24		/* One day if printed */
#define	DEF_SUFCHARS	".-"		/* Default suffix chars */

int	gspool_fd;
char	*realuname;
struct	apispdet	mypriv;
struct	apispq	SPQ;
char	wotform[MAXFORM+1];
char	*page_delim;
int	delimlen, delimnum;
int			Njobs, Nptrs;
struct	apispq		*job_list;
slotno_t		*jslot_list;
struct	ptr_with_slot	*ptr_sl_list;

extern char *hex_disp(const classcode_t, const int);

/* Keep library happy */

void  nomem()
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

static void  init_jobdefaults()
{
	/* Assumes SPQ initialised to zero of course */
	strcpy(wotform, mypriv.spu_form);
	strcpy(SPQ.apispq_ptr, mypriv.spu_ptr);
	SPQ.apispq_pri = mypriv.spu_defp;
	SPQ.apispq_class = mypriv.spu_class;
	SPQ.apispq_uid = Realuid;
	strncpy(SPQ.apispq_uname, realuname, UIDSIZE);
	strcpy(SPQ.apispq_puname, SPQ.apispq_uname);
	SPQ.apispq_cps = 1;
	SPQ.apispq_end = MAXLONG - 1L;
	SPQ.apispq_nptimeout = QNPTIMEOUT;
	SPQ.apispq_ptimeout = QPTIMEOUT;
}

/* Interpret argument if any as being a "template job" job number */

void  interp_args(char **args)
{
	char	*arg = *args;
	int	ret;
	struct	jobswanted  jw;
	struct	apispq	oldjob;

	if  (!arg)		/* No job to look at */
		return;

	if  ((ret = decode_jnum(arg, &jw)) != 0)  {
		if  (html_out_cparam_file("badcarg", 1, arg))
			exit(E_USAGE);
		html_error(arg);
		exit(E_SETUP);
	}
	if  ((ret = gspool_jobfind(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.jno, jw.host, &jw.slot, &oldjob)) < 0)  {
		if  (ret == GSPOOL_UNKNOWN_JOB)
			html_out_cparam_file("jobgone", 1, arg);
		else
			html_disperror($E{Base for API errors} + ret);
		exit(E_NOJOB);
	}

	/* Copy in parameters for the job */

	SPQ.apispq_hold = oldjob.apispq_hold;
	SPQ.apispq_nptimeout = oldjob.apispq_nptimeout;
	SPQ.apispq_ptimeout = oldjob.apispq_ptimeout;
	SPQ.apispq_cps = oldjob.apispq_cps;
	SPQ.apispq_pri = oldjob.apispq_pri;
	SPQ.apispq_jflags = oldjob.apispq_jflags & (APISPQ_NOH|APISPQ_WRT|APISPQ_MAIL|APISPQ_RETN|
					   APISPQ_ODDP|APISPQ_EVENP|APISPQ_REVOE|APISPQ_MATTN|
					   APISPQ_WATTN|APISPQ_LOCALONLY);

	if  ((mypriv.spu_flgs & PV_COVER))
		SPQ.apispq_class = oldjob.apispq_class;
	else  if  ((SPQ.apispq_class = oldjob.apispq_class & mypriv.spu_class) == 0)
		SPQ.apispq_class = mypriv.spu_class;

	SPQ.apispq_start = oldjob.apispq_start;
	SPQ.apispq_end = oldjob.apispq_end;

	if  (strcmp(oldjob.apispq_uname, oldjob.apispq_puname) != 0)
		strcpy(SPQ.apispq_puname, oldjob.apispq_puname);

	strcpy(SPQ.apispq_file, oldjob.apispq_file);
	if  ((mypriv.spu_flgs & PV_FORMS) || qmatch(mypriv.spu_formallow, oldjob.apispq_form))
		strcpy(wotform, oldjob.apispq_form);
	if  ((mypriv.spu_flgs & PV_OTHERP)  ||  issubset(mypriv.spu_ptrallow, oldjob.apispq_ptr))
		strcpy(SPQ.apispq_ptr, oldjob.apispq_ptr);
	strcpy(SPQ.apispq_flags, oldjob.apispq_flags);

	/* Grab page delimiter if applicable */
	if  (oldjob.apispq_dflags & APISPQ_PAGEFILE)  {
		FILE	*pfl = gspool_jobpbrk(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.slot);
		if  (pfl)  {
			struct	apipages  page_fe;
			if  (fread((char *) &page_fe, sizeof(page_fe), 1, pfl) == 1)  {
				delimnum = page_fe.delimnum;
				delimlen = page_fe.deliml;
				if  (!(page_delim = malloc((unsigned) delimlen)))
					html_nomem();
				fread(page_delim, sizeof(char), delimlen, pfl);
				while  (getc(pfl) != EOF)
					;
				fclose(pfl);
			}
		}
	}
}

char	had_hdr = 0, had_pri = 0, had_cps = 0;
char	*buff_filename;

static void  arg_jobdata(char *arg)
{
	if  (!had_hdr)
		strncpy(SPQ.apispq_file, arg, MAXTITLE);
}

static void  arg_hdr(char *arg)
{
	strncpy(SPQ.apispq_file, arg, MAXTITLE);
	if  (arg[0])		/* Only if it isn't blank */
		had_hdr = 1;
}

static void  arg_sp(char *arg)
{
	ULONG  v = strtoul(arg, (char **) 0, 0);
	SPQ.apispq_start = v != 0? v - 1: 0;
}

static void  arg_ep(char *arg)
{
	ULONG  v = strtoul(arg, (char **) 0, 0);
	SPQ.apispq_end = v != 0? v - 1: MAXLONG-1;
}

static void  arg_cps(char *arg)
{
	int	c = atoi(arg);

	if  (had_cps)
	    SPQ.apispq_cps = (unsigned char) (c + SPQ.apispq_cps);
	else  {
	    SPQ.apispq_cps = (unsigned char) c;
	    had_cps = 1;
	}
}

static void  arg_pri(char *arg)
{
	int	n = atoi(arg);
	if  (had_pri)
	    SPQ.apispq_pri = (unsigned char) (n + SPQ.apispq_pri);
	else  {
	    SPQ.apispq_pri = (unsigned char) n;
	    had_pri = 1;
	}
}

static void  arg_form(char *arg)
{
	strncpy(wotform, arg, MAXFORM);
}

static void  arg_ptr(char *arg)
{
	strncpy(SPQ.apispq_ptr, arg, JPTRNAMESIZE);
}

static void  arg_npt(char *arg)
{
	SPQ.apispq_nptimeout = (USHORT) atoi(arg);
}

static void  arg_pt(char *arg)
{
	SPQ.apispq_ptimeout = (USHORT) atoi(arg);
}

static void  arg_jflags(char *arg, const unsigned flag)
{
	switch  (*arg)  {
	case  'y':case  'Y':
	case  't':case  'T':
	case  '1':
		SPQ.apispq_jflags |= flag;
		break;
	case  'n':case  'N':
	case  'f':case  'F':
	case  '0':
		SPQ.apispq_jflags &= ~flag;
		break;
	}
}

static void  arg_jflags_noh(char *arg)
{
	arg_jflags(arg, APISPQ_NOH);
}

static void  arg_jflags_wrt(char *arg)
{
	arg_jflags(arg, APISPQ_WRT);
}

static void  arg_jflags_mail(char *arg)
{
	arg_jflags(arg, APISPQ_MAIL);
}

static void  arg_jflags_retn(char *arg)
{
	arg_jflags(arg, APISPQ_RETN);
}

static void  arg_jflags_oddp(char *arg)
{
	arg_jflags(arg, APISPQ_ODDP);
}

static void  arg_jflags_evenp(char *arg)
{
	arg_jflags(arg, APISPQ_EVENP);
}

static void  arg_jflags_revoe(char *arg)
{
	arg_jflags(arg, APISPQ_REVOE);
}

static void  arg_jflags_mattn(char *arg)
{
	arg_jflags(arg, APISPQ_MATTN);
}

static void  arg_jflags_wattn(char *arg)
{
	arg_jflags(arg, APISPQ_WATTN);
}

static void  arg_jflags_loco(char *arg)
{
	arg_jflags(arg, APISPQ_LOCALONLY);
}

static void  arg_class(char *arg)
{
	classcode_t	cl = strtoul(arg, (char **) 0, 0);
	if  (!(mypriv.spu_flgs & PV_COVER))
		cl &= mypriv.spu_class;
	if  (cl == 0)  {
		disp_str = arg;
		disp_str2 = hex_disp(mypriv.spu_class, 0);
		html_disperror($E{setting zero class});
		exit(E_BADCLASS);
	}
	SPQ.apispq_class = cl;
}

static void  arg_hold(char *arg)
{
	time_t	ht = atol(arg), now = time((time_t *) 0);
	if  (ht < now)
		ht = 0;
	SPQ.apispq_hold = (LONG)ht;
}

static void  arg_puser(char *arg)
{
	strncpy(SPQ.apispq_puname,
		lookup_uname(arg) == UNKNOWN_UID? realuname: arg,
		UIDSIZE);
}

static void  arg_flags(char *arg)
{
	strncpy(SPQ.apispq_flags, arg, MAXFLAGS);
}

struct	posttab  ptab[] =  {
	{	"jobfile",	arg_jobdata,	&buff_filename  },
	{	"sp",		arg_sp		},
	{	"ep",		arg_ep		},
	{	"cps",		arg_cps		},
	{	"cps_10",	arg_cps		},
	{	"cps_100",	arg_cps		},
	{	"pri",		arg_pri		},
	{	"pri_10",	arg_pri		},
	{	"pri_100",	arg_pri		},
	{	"noh",		arg_jflags_noh 	},
	{	"hdr",		arg_hdr		},
	{	"form",		arg_form	},
	{	"ptr",		arg_ptr		},
	{	"npto",		arg_npt		},
	{	"pto",		arg_pt		},
	{	"wrt",		arg_jflags_wrt	},
	{	"mail",		arg_jflags_mail	},
	{	"retn",		arg_jflags_retn	},
	{	"oddp",		arg_jflags_oddp	},
	{	"evenp",	arg_jflags_evenp},
	{	"revoe",	arg_jflags_revoe},
	{	"mattn",	arg_jflags_mattn},
	{	"wattn",	arg_jflags_wattn},
	{	"loco",		arg_jflags_loco	},
	{	"class",	arg_class	},
	{	"hold", 	arg_hold	},
	{	"puser",	arg_puser	},
	{	"flags",	arg_flags	},
	{	(char *) 0  }
};

jobno_t  perform_submit()
{
	jobno_t	jn;
	int	ch, ret;
	FILE	*inf, *outf;

	if  (!buff_filename)  {
		html_disperror($E{sqcrcgi no file supplied});
		exit(E_USAGE);
	}
	if  (!(inf = fopen(buff_filename, "r")))  {
		html_disperror($E{sqcrcgi cannot reopen temp file});
		unlink(buff_filename);
		exit(E_SETUP);
	}
	strcpy(SPQ.apispq_form, wotform);
	if  (!(outf = gspool_jobadd(gspool_fd, &SPQ, page_delim, delimlen, delimnum)))  {
		html_disperror($E{Base for API errors} + gspool_dataerror);
		unlink(buff_filename);
		exit(E_NOPRIV);
	}
	while  ((ch = getc(inf)) != EOF)
		putc(ch, outf);
	fclose(inf);
	unlink(buff_filename);
	fclose(outf);
	if  ((ret = gspool_jobres(gspool_fd, &jn)) != 0)  {
		html_disperror($E{Base for API errors} + ret);
		exit(E_NOPRIV);
	}
	return  jn;
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
	char	**newargs;
	int_ugid_t	chku;
	jobno_t	jobnum;

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
	init_jobdefaults();
	interp_args(newargs);
	html_postvalues(ptab);
	jobnum = perform_submit();
	html_out_param_file("submitok", 1, jobnum, 0L);
	return  0;
}
