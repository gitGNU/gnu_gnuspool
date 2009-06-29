/* spcgi.c -- CGI program for printer listing

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
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <sys/shm.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "listperms.h"
#include "cgifndjb.h"
#include "xfershm.h"

int	headerflag = -1;
char	*Curr_pwd;

#define	IPC_MODE	0600

char	*formatstring;
char	sdefaultfmt[] = "Lp Ld Lf Ls Lj Lu";

char	bigbuff[200];

struct	spdet	*mypriv;
char	*Realuname;

/* Vector of states - assumed given prompt codes consecutively */

char	*statenames[SPP_NSTATES];

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

void	shm_setup(void)
{
	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		html_disperror($E{Spooler not running});
		exit(E_NOTRUN);
	}

#ifndef	USING_FLOCK
	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		html_disperror($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif

	if  (!jobshminit(0))  {
		html_disperror($E{Cannot open jshm});
		exit(E_JOBQ);
	}

	if  (!ptrshminit(0))  {
		print_error($E{Cannot open pshm});
		exit(E_PRINQ);
	}
}

void	fmt_setup(void)
{
	if  (!formatstring  &&  !(formatstring = html_inistr("format", (char *) 0)))
		formatstring = sdefaultfmt;
}

/* Get state names.  */

void	openpfile(void)
{
	int	i;

	/* Read state names */

	for  (i = 0;  i < SPP_NSTATES;  i++)
		statenames[i] = gprompt($P{Printer status null}+i);
}

typedef	unsigned	fmt_t;
#define	INLINE_SPLIST
#include "inline/pfmt_ab.c"
#include "inline/pfmt_class.c"
#include "inline/pfmt_dev.c"
#include "inline/pfmt_heoj.c"
#include "inline/pfmt_pid.c"
#include "inline/pfmt_jobno.c"
#include "inline/pfmt_loco.c"
#include "inline/pfmt_msg.c"
#include "inline/pfmt_na.c"
#include "inline/pfmt_ptr.c"
#include "inline/pfmt_form.c"
#include "inline/pfmt_state.c"
#include "inline/pfmt_ostat.c"
#include "inline/pfmt_user.c"
#include "inline/pfmt_minsz.c"
#include "inline/pfmt_maxsz.c"

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct	formatdef  {
	SHORT	statecode;	/* Code number for heading if applicable */
	char	alch;		/* Default align char */
	char	*msg;		/* Heading */
	unsigned	(*fmt_fn)(const struct spptr *, const int);
};

#define	NULLCP	(char *) 0

struct	formatdef
	lowertab[] = { /* a-z */
	{	$P{Printer title}+'a'-1, 'L',	NULLCP,	fmt_ab		},	/* a */
	{	0,			 'L',	NULLCP,	0		},	/* b */
	{	$P{Printer title}+'c'-1, 'L',	NULLCP,	fmt_class	},	/* c */
	{	$P{Printer title}+'d'-1, 'L',	NULLCP,	fmt_device	},	/* d */
	{	$P{Printer title}+'e'-1, 'L',	NULLCP,	fmt_comment	},	/* e */
	{	$P{Printer title}+'f'-1, 'L',	NULLCP,	fmt_form	},	/* f */
	{	0,			 'L',	NULLCP,	0		},	/* g */
	{	$P{Printer title}+'h'-1, 'L',	NULLCP,	fmt_heoj	},	/* h */
	{	$P{Printer title}+'i'-1, 'L',	NULLCP,	fmt_pid		},	/* i */
	{	$P{Printer title}+'j'-1, 'L',	NULLCP,	fmt_jobno	},	/* j */
	{	0,			 'L',	NULLCP,	0		},	/* k */
	{	$P{Printer title}+'l'-1, 'L',	NULLCP,	fmt_localonly	},	/* l */
	{	$P{Printer title}+'m'-1, 'L',	NULLCP,	fmt_message	},	/* m */
	{	$P{Printer title}+'n'-1, 'L',	NULLCP,	fmt_na		},	/* n */
	{	0,			 'L',	NULLCP,	0		},	/* o */
	{	$P{Printer title}+'p'-1, 'L',	NULLCP, fmt_printer	},	/* p */
	{	0,			 'L',	NULLCP,	0		},	/* q */
	{	0,			 'L',	NULLCP, 0		},	/* r */
	{	$P{Printer title}+'s'-1, 'L',	NULLCP, fmt_state	},	/* s */
	{	$P{Printer title}+'t'-1, 'L',	NULLCP, fmt_ostate	},	/* t */
	{	$P{Printer title}+'u'-1, 'L',	NULLCP, fmt_user	},	/* u */
	{	0,			 'L',	NULLCP,	0		},	/* v */
	{	0,			 'L',	NULLCP,	0		},	/* w */
	{	0,			 'L',	NULLCP,	0		},	/* x */
	{	$P{Printer title}+'y'-1, 'L',	NULLCP,	fmt_minsize	},	/* y */
	{	$P{Printer title}+'z'-1, 'L',	NULLCP,	fmt_maxsize	}	/* z */
};

void	print_hdrfmt(struct formatdef * fp)
{
	if  (!fp->fmt_fn)
		return;
	if  (!fp->msg)
		fp->msg = gprompt(fp->statecode);
	html_fldprint(fp->msg);
}

struct	altype  {
	char	*str;		/* Align string for html */
	char	ch;		/* Align char */
}  altypes[] = {
	{	"left",	'L' },
	{	"right",	'R' },
	{	"center",	'C' }
};

#define	NALIGNTYPES (sizeof(altypes) / sizeof(struct altype))

struct	altype	*commonest_align = &altypes[0];

struct altype *	lookup_align(const int alch)
{
	int	cnt;
	for  (cnt = 0;  cnt < NALIGNTYPES;  cnt++)
		if  (altypes[cnt].ch == alch)
			return  &altypes[cnt];
	return  commonest_align;
}

struct	colfmt  {
	struct	formatdef  *form;
	char	*alstr;
};

struct	colfmt	*cflist;
int	ncolfmts, maxcolfmts;

#define	INITCF	10
#define	INCCF	5

void	find_commonest(char * fp)
{
	int	rvec[NALIGNTYPES];
	int	cnt, mx = 0, ind = 0, fmch;
	struct	altype	*whichalign;
	struct	formatdef  *fd;

	for  (cnt = 0;  cnt < NALIGNTYPES;  cnt++)
		rvec[cnt] = 0;

	if  (!(cflist = (struct colfmt *) malloc(INITCF * sizeof(struct colfmt))))
		nomem();
	maxcolfmts = INITCF;

	while  (*fp)  {
		while  (*fp  &&  !isalpha(*fp))
			fp++;
		if  (!*fp)
			break;
		whichalign = lookup_align(*fp++);
		rvec[whichalign - &altypes[0]]++;
		fmch = *fp++;
		if  (!islower(fmch))
			break;
		fd = &lowertab[fmch - 'a'];
		if  (!fd->fmt_fn)
			continue;
		if  (ncolfmts >= maxcolfmts)  {
			maxcolfmts += INCCF;
			cflist = (struct colfmt *) realloc((char *) cflist, (unsigned) (maxcolfmts * sizeof(struct colfmt)));
			if  (!cflist)
				nomem();
		}
		cflist[ncolfmts].form = fd;
		cflist[ncolfmts].alstr = whichalign->str;
		ncolfmts++;
	}

	for  (cnt = 0;  cnt < NALIGNTYPES;  cnt++)
		if  (mx < rvec[cnt])  {
			mx = rvec[cnt];
			ind = cnt;
		}
	commonest_align = &altypes[ind];

	for  (cnt = 0;  cnt < ncolfmts;  cnt++)
		if  (cflist[cnt].alstr == commonest_align->str)
			cflist[cnt].alstr = (char *) 0;
}

void	startrow(void)
{
	printf("<tr align=%s>\n", commonest_align->str);
}

void	startcell(const int celltype, const char *str)
{
	if  (str)
		printf("<t%c align=%s>", celltype, str);
	else
		printf("<t%c>", celltype);
}

/* Display contents of printer list.  */

void	pdisplay(void)
{
	int	fcnt, pcnt;
	unsigned   pflgs = 0, lpperm = 0, rpperm = 0;

	/* <TABLE> included in pre-list file so as to possibly include formatting elements */

	find_commonest(formatstring);

	/* Possibly insert header */

	if  (headerflag)  {
		/* Possibly insert first header showing options */
		if  (Displayopts.opt_restrp || Displayopts.opt_localonly == NRESTR_LOCALONLY)  {
			startrow();
			printf("<th colspan=%d align=center>", ncolfmts+1);
			fputs(gprompt($P{sqcgi restr start}), stdout);
			html_fldprint(gprompt($P{sqcgi restr view}));
			if  (Displayopts.opt_restrp)  {
				html_fldprint(gprompt($P{sqcgi restr printers}));
				html_fldprint(Displayopts.opt_restrp);
			}
			if  (Displayopts.opt_localonly == NRESTR_LOCALONLY)
				html_fldprint(gprompt($P{sqcgi restr loco}));
			fputs(gprompt($P{sqcgi restr end}), stdout);
			fputs("</th></tr>\n", stdout);
		}
		startrow();
		startcell('h', commonest_align->str); /* Blank space in place of checkbox */
		fputs("</th>", stdout);
		for  (fcnt = 0;  fcnt < ncolfmts;  fcnt++)  {
			startcell('h', cflist[fcnt].alstr);
			print_hdrfmt(cflist[fcnt].form);
			fputs("</th>", stdout);
		}
		fputs("</tr>\n", stdout);
	}

	/* Set up default permissions which for printers apply
	   throughout. */

	if  (mypriv->spu_flgs & PV_PRINQ)  {
		pflgs |= GLV_ANYCHANGES;
		lpperm |= LV_MINEORVIEW|LV_LOCORRVIEW|LV_CHANGEABLE;
		if  (mypriv->spu_flgs & PV_HALTGO)
			lpperm |= LV_KILLABLE;
		if  (mypriv->spu_flgs & PV_ADDDEL)
			lpperm |= LV_DELETEABLE;
		if  (mypriv->spu_flgs & PV_REMOTEP)
			rpperm = lpperm;
		if  (mypriv->spu_flgs & PV_COVER)
			pflgs |= GLV_COVER;
		if  (mypriv->spu_flgs & PV_ACCESSOK)
			pflgs |= GLV_ACCESSF;
		if  (mypriv->spu_flgs & PV_FREEZEOK)
			pflgs |= GLV_FREEZEF;
	}

	/* Final run-through to output stuff */

	for  (pcnt = 0;  pcnt < Ptr_seg.nptrs;  pcnt++)  {
		const struct spptr *pp = &Ptr_seg.pp_ptrs[pcnt]->p;
		unsigned  flgs;
		if  (pp->spp_state == SPP_NULL  ||  pp->spp_state >= SPP_NSTATES)
			continue;

		flgs = pp->spp_netid == 0L? lpperm: rpperm;
		if  (pp->spp_state >= SPP_PROC)
			flgs |= LV_PROCESS;

		startrow();
		startcell('d', commonest_align->str);
		fmt_printer(pp, 0);
		printf("<input type=checkbox name=ptrs value=\"%s,%u\"></td>", bigbuff, flgs);

		for  (fcnt = 0;  fcnt < ncolfmts;  fcnt++)  {
			startcell('d', cflist[fcnt].alstr);
			bigbuff[0] = '\0';
			(cflist[fcnt].form->fmt_fn)(pp, 0);
			html_fldprint(bigbuff);
			fputs("</td>", stdout);
		}
		fputs("</tr>\n", stdout);
	}

	printf("</table>\n<input type=hidden name=privs value=%u>\n", pflgs);
	printf("<input type=hidden name=classcode value=%lu>\n", (unsigned long) mypriv->spu_class);
}

struct	arginterp  {
	char	*argname;
	unsigned  short  flags;
#define	AIF_NOARG	0
#define	AIF_ARG		1
#define	AIF_ARGLIST	2
	int	(*arg_fn)(char *);
};

int	perf_listformat(char *notused)
{
	struct	formatdef   *fp;
	int	lett;
	char	*msg;

	html_out_or_err("listfmt_pre", 1);
	for  (fp = &lowertab[0],  lett = 'a';  fp < &lowertab[26];  lett++, fp++)
		if  (fp->statecode != 0)  {
			msg = gprompt(fp->statecode + 500);
			printf("list_format_code(\'%c\', \'%c\', \"%s\");\n", lett, fp->alch, msg);
			free(msg);
		}

	fmt_setup();
	printf("existing_formats(\"%s\");\n", formatstring);
	html_out_param_file("listfmt_post", 0, 0, html_cookexpiry());
	exit(0);
}

extern int	perf_optselect(char *);

int	set_queue(char *arg)
{
	Displayopts.opt_restrp = *arg? arg: (char *) 0;
	return  1;
}

int	set_loco(char *arg)
{
	switch  (tolower(*arg))  {
	case  'l':case 'y':case '1':
		Displayopts.opt_localonly = NRESTR_LOCALONLY;
		return  1;
	case  'n':case '0':
		Displayopts.opt_localonly = NRESTR_NONE;
		return  1;
	default:
		return  0;
	}
}

int	set_header(char *arg)
{
	switch  (tolower(*arg))  {
	case  'y':case '1':
		headerflag = 1;
		return  1;
	case  'n':case '0':
		headerflag = 0;
		return  1;
	default:
		return  0;
	}
}

int	set_sorted(char *arg)
{
	switch  (tolower(*arg))  {
	case  'y':case '1':
		Displayopts.opt_sortptrs = SORTP_BYNAME;
		return  1;
	case  'n':case '0':
		Displayopts.opt_sortptrs = SORTP_NONE;
		return  1;
	default:
		return  0;
	}
}

int	set_format(char *arg)
{
	formatstring = arg;
	return  1;
}

extern void	perf_listprfm(char *, char *, char *);

struct	arginterp  argtypes[] =  {
	{	"format",	AIF_ARG,	set_format	},
	{	"header",	AIF_ARG,	set_header	},
	{	"queue",	AIF_ARG,	set_queue	},
	{	"localonly",	AIF_ARG,	set_loco	},
	{	"sorted",	AIF_ARG,	set_sorted	},
	{	"listprfm",	AIF_ARGLIST  },
	{	"listopts",	AIF_NOARG,	perf_optselect	},
	{	"listformat",	AIF_NOARG,	perf_listformat	}
};

/* Invoke a call to the JavaScript function "jsfunc" with parameters
   taken from the printer "ptrnam"
   Printer, Form, device, description, possible forms */

void	perf_listprfm(char *tmpl, char *jsfunc, char *ptrnam)
{
	char	*pname, *pform, *pdev, *pdescr;
	int	jcnt;
	struct	ptrswanted	pw;
	struct	strvec	fv;

	shm_setup();
	readptrlist(1);
	readjoblist(1);

	if  (decode_pname(ptrnam, &pw)  &&  find_ptr(&pw))  {
		const	struct  spptr	*pp = pw.pp;
		pname = stracpy(pp->spp_ptr);
		pform = stracpy(pp->spp_form);
		pdev = stracpy(pp->spp_dev);
		pdescr = stracpy(pp->spp_comment);
	}
	else
		pname = pform = pdev = pdescr = "";

	strvec_init(&fv);
	strvec_add(&fv, mypriv->spu_form);

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)
		strvec_add(&fv, Job_seg.jj_ptrs[jcnt]->j.spq_form);
	for  (jcnt = 0;  jcnt < Ptr_seg.nptrs;  jcnt++)
		strvec_add(&fv, Ptr_seg.pp_ptrs[jcnt]->p.spp_form);
	strvec_sort(&fv);

	/* OK now generate the stuff */

	html_output_file(tmpl, 1);
	printf("%s(\"%s\", \"%s\", \"%s\", \"%s\", ",
	       jsfunc,
	       escquot(pname),
	       escquot(pform),
	       escquot(pdev),
	       escquot(pdescr));
	print_strvec(&fv);
	fputs(");\n", stdout);
	html_output_file("prfm_postamble", 0);
	exit(0);
}

int	perf_optselect(char *notused)
{
	html_out_param_file("setopts", 1, 0, html_cookexpiry());
	exit(0);
}

void	interp_args(char **args)
{
	char	**ap, *arg, *cp = (char *) 0;
	int	cnt;

	for  (ap = args;  (arg = *ap);  ap++)  {
		if  ((cp = strchr(arg, ':')) || (cp = strchr(arg, '=')))
			*cp++ = '\0';
		for  (cnt = 0;  cnt < sizeof(argtypes)/sizeof(struct arginterp);  cnt++)  {
			if  (ncstrcmp(arg, argtypes[cnt].argname) == 0)  {
			    if  (argtypes[cnt].flags == AIF_ARGLIST)  {
				if  (!ap[1] || !ap[2])
				    goto  badarg;
				perf_listprfm(ap[1], ap[2], ap[3]);
			    }
			    if  ((cp  &&  argtypes[cnt].flags == AIF_NOARG)  ||
				 (!cp  &&  argtypes[cnt].flags == AIF_ARG)  ||
				 !(argtypes[cnt].arg_fn)(cp))
				goto  badarg;
			    goto  ok;
			}
		}
	badarg:
		html_out_or_err("badargs", 1);
		exit(E_USAGE);
	ok:
		;
	}
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char	**newargs;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.2 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();
	html_openini();
	newargs = cgi_arginterp(argc, argv, 0); /* Side effect of cgi_arginterp is to set Realuid */
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	hash_hostfile();

	/* Now we want to be Daemuid throughout if possible.  */

#if  	defined(OS_BSDI) || defined(OS_FREEBSD)
	seteuid(Daemuid);
#else
	setuid(Daemuid);
#endif

	interp_args(newargs);
	shm_setup();
	openpfile();
	if  (headerflag < 0)
		headerflag = html_inibool("headers", 0);
	fmt_setup();
	html_output_file("list_preamble", 1);
	readptrlist(0);
	pdisplay();
	ptrshm_unlock();
	html_output_file("list_postamble", 0);
	return  0;
}
