/* sqcgi.c -- list jobs CGI program

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
#include "cgifndjb.h"
#include "listperms.h"
#include "xfershm.h"

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

int	nopage = -1,
	headerflag = -1;

char	*formatstring;

int	jno_width  =  0;

char	sdefaultfmt[] = "LN Lu Lh Lf LQ RS Rc Rp LP",
	npdefaultfmt[] = "LN Lu Lh Lf LL RK Rc Rp LP";

struct	spdet	*mypriv;
char	*Realuname;

char	bigbuff[200];

char	*Curr_pwd;

static	char	*localptr;

#define	IPC_MODE	0600

int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

static void	shm_setup(void)
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
		html_disperror($E{Cannot open pshm});
		exit(E_PRINQ);
	}
}

void	fmt_setup(void)
{
	if  (!formatstring)  {
		if  (!(formatstring = html_inistr("format", (char *) 0)))
			formatstring = nopage? npdefaultfmt: sdefaultfmt;
	}
}

typedef	unsigned	fmt_t;
#define	INLINE_SQLIST

#include "inline/jfmt_wattn.c"
#include "inline/jfmt_class.c"
#include "inline/jfmt_ppf.c"
#include "inline/jfmt_hold.c"
#include "inline/jfmt_sizek.c"
#include "inline/jfmt_krchd.c"
#include "inline/jfmt_jobno.c"
#include "inline/jfmt_oddev.c"
#include "inline/jfmt_ptr.c"
#include "inline/jfmt_pgrch.c"
#include "inline/jfmt_range.c"
#include "inline/jfmt_szpgs.c"
#include "inline/jfmt_nptim.c"
#include "inline/jfmt_user.c"
#include "inline/jfmt_puser.c"
#include "inline/jfmt_stime.c"
#include "inline/jfmt_mattn.c"
#include "inline/jfmt_cps.c"
#include "inline/jfmt_form.c"
#include "inline/jfmt_title.c"
#include "inline/jfmt_loco.c"
#include "inline/jfmt_mail.c"
#include "inline/jfmt_prio.c"
#include "inline/jfmt_retn.c"
#include "inline/jfmt_supph.c"
#include "inline/jfmt_ptime.c"
#include "inline/jfmt_write.c"
#include "inline/jfmt_origh.c"
#include "inline/jfmt_delim.c"

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct	formatdef  {
	SHORT	statecode;	/* Code number for heading if applicable */
	char	alch;		/* Default align char */
	char	*msg;		/* Heading */
	unsigned	(*fmt_fn)(const struct spq *, const int);
};

#define	NULLCP	(char *) 0

struct	formatdef
	uppertab[] = { /* A-Z */
	{	$P{job fmt title}+'A'-1, 'L',	NULLCP,	fmt_wattn	},	/* A */
	{	0,			 'L',	NULLCP,	0		},	/* B */
	{	$P{job fmt title}+'C'-1, 'L',	NULLCP,	fmt_class	},	/* C */
	{	$P{job fmt title}+'D'-1, 'L',	NULLCP,	fmt_delim	},	/* D */
	{	0,			 'L',	NULLCP,	0		},	/* E */
	{	$P{job fmt title}+'F'-1, 'L',	NULLCP,	fmt_ppflags	},	/* F */
	{	$P{job fmt title}+'G'-1, 'L',	NULLCP,	fmt_hat		},	/* G */
	{	$P{job fmt title}+'H'-1, 'L',	NULLCP, fmt_hold	},	/* H */
	{	0,			 'L',	NULLCP, 0		},	/* I */
	{	0,			 'L',	NULLCP,	0		},	/* J */
	{	$P{job fmt title}+'K'-1, 'R',	NULLCP,	fmt_sizek	},	/* K */
	{	$P{job fmt title}+'L'-1, 'L',	NULLCP, fmt_kreached	},	/* L */
	{	0,			 'L',	NULLCP, 0		},	/* M */
	{	$P{job fmt title}+'N'-1, 'L',	NULLCP, fmt_jobno	},	/* N */
	{	$P{job fmt title}+'O'-1, 'L',	NULLCP,	fmt_oddeven	},	/* O */
	{	$P{job fmt title}+'P'-1, 'L',	NULLCP, fmt_printer	},	/* P */
	{	$P{job fmt title}+'Q'-1, 'L',	NULLCP,	fmt_pgreached	},	/* Q */
	{	$P{job fmt title}+'R'-1, 'L',	NULLCP, fmt_range	},	/* R */
	{	$P{job fmt title}+'S'-1, 'R',	NULLCP, fmt_szpages	},	/* S */
	{	$P{job fmt title}+'T'-1, 'L',	NULLCP, fmt_nptime	},	/* T */
	{	$P{job fmt title}+'U'-1, 'L',	NULLCP, fmt_puser	},	/* U */
	{	0,			 'L',	NULLCP,	0		},	/* V */
	{	$P{job fmt title}+'W'-1, 'L',	NULLCP,	fmt_stime	},	/* W */
	{	0,			 'L',	NULLCP, 0		},	/* X */
	{	0,			 'L',	NULLCP,	0		},	/* Y */
	{	0,			 'L',	NULLCP,	0		}	/* Z */
},
	lowertab[] = { /* a-z */
	{	$P{job fmt title}+'a'-1, 'L',	NULLCP,	fmt_mattn	},	/* a */
	{	0,			 'L',	NULLCP,	0		},	/* b */
	{	$P{job fmt title}+'c'-1, 'R',	NULLCP,	fmt_cps		},	/* c */
	{	$P{job fmt title}+'d'-1, 'L',	NULLCP,	fmt_delimnum	},	/* d */
	{	0,			 'L',	NULLCP,	0		},	/* e */
	{	$P{job fmt title}+'f'-1, 'L',	NULLCP,	fmt_form	},	/* f */
	{	0,			 'L',	NULLCP,	0		},	/* g */
	{	$P{job fmt title}+'h'-1, 'L',	NULLCP,	fmt_title	},	/* h */
	{	0,			 'L',	NULLCP,	0		},	/* i */
	{	0,			 'L',	NULLCP,	0		},	/* j */
	{	0,			 'L',	NULLCP,	0		},	/* k */
	{	$P{job fmt title}+'l'-1, 'L',	NULLCP,	fmt_localonly	},	/* l */
	{	$P{job fmt title}+'m'-1, 'L',	NULLCP,	fmt_mail	},	/* m */
	{	0,			 'L',	NULLCP,	0		},	/* n */
	{	$P{job fmt title}+'o'-1, 'L',	NULLCP,	fmt_orighost	},	/* o */
	{	$P{job fmt title}+'p'-1, 'R',	NULLCP, fmt_prio	},	/* p */
	{	$P{job fmt title}+'q'-1, 'L',	NULLCP,	fmt_retain	},	/* q */
	{	0,			 'L',	NULLCP, 0		},	/* r */
	{	$P{job fmt title}+'s'-1, 'L',	NULLCP, fmt_supph	},	/* s */
	{	$P{job fmt title}+'t'-1, 'L',	NULLCP, fmt_ptime	},	/* t */
	{	$P{job fmt title}+'u'-1, 'L',	NULLCP, fmt_user	},	/* u */
	{	0,			 'L',	NULLCP,	0		},	/* v */
	{	$P{job fmt title}+'w'-1, 'L',	NULLCP,	fmt_write	},	/* w */
	{	0,			 'L',	NULLCP,	0		},	/* x */
	{	0,			 'L',	NULLCP,	0		},	/* y */
	{	0,			 'L',	NULLCP,	0		}	/* z */
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
		if  (!isalpha(fmch))
			break;
		fd = isupper(fmch)? &uppertab[fmch - 'A']: &lowertab[fmch - 'a'];
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

/* Display contents of job file.  */

void	jdisplay(void)
{
	int	fcnt, jcnt;
	unsigned	pflgs = 0;

	/* <TABLE> included in pre-list file so as to possibly include formatting elements */

	find_commonest(formatstring);

	/* Possibly insert header */

	if  (headerflag)  {
		/* Possibly insert first header showing options */
		if  (Displayopts.opt_restrp || Displayopts.opt_restru ||
		     Displayopts.opt_localonly == NRESTR_LOCALONLY ||
		     Displayopts.opt_jprindisp != JRESTR_ALL)  {
			startrow();
			printf("<th colspan=%d align=center>", ncolfmts+1);
			fputs(gprompt($P{sqcgi restr start}), stdout);
			html_fldprint(gprompt($P{sqcgi restr view}));
			if  (Displayopts.opt_restrp)  {
				html_fldprint(gprompt($P{sqcgi restr printers}));
				html_fldprint(Displayopts.opt_restrp);
				if  (Displayopts.opt_jinclude != JINCL_NONULL)
					html_fldprint(gprompt($P{sqcgi restr null}));
			}
			if  (Displayopts.opt_restru)  {
				html_fldprint(gprompt($P{sqcgi restr users}));
				html_fldprint(Displayopts.opt_restru);
			}
			if  (Displayopts.opt_jprindisp != JRESTR_ALL)
				html_fldprint(gprompt(Displayopts.opt_jprindisp == JRESTR_PRINT?
						      $P{sqcgi restr p}: $P{sqcgi restr unp}));
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

	/* Final run-through to output stuff */

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		const struct spq *jp = &Job_seg.jj_ptrs[jcnt]->j;
		unsigned  hval = 0;
		if  (jp->spq_job == 0)
			break;

		startrow();
		startcell('d', commonest_align->str);
		fmt_jobno(jp, 0);
		if  (strcmp(Realuname, jp->spq_uname) == 0)
			hval |= LV_MINEORVIEW|LV_DELETEABLE|LV_CHANGEABLE;
		else  {
			if  (mypriv->spu_flgs & PV_OTHERJ)
				hval |= LV_DELETEABLE|LV_CHANGEABLE;
			if  (mypriv->spu_flgs & PV_VOTHERJ)
				hval |= LV_MINEORVIEW;
		}
		if  (mypriv->spu_flgs & PV_REMOTEJ  ||  jp->spq_netid == 0L)
			hval |= LV_LOCORRVIEW;

		if  (jp->spq_npages > 1)
			hval |= LV_PAGES;

		if  (jp->spq_dflags & SPQ_PRINTED)
			hval |= LV_WARNOFF;

		printf("<input type=checkbox name=jobs value=\"%s,%u\"></td>", bigbuff, hval);

		for  (fcnt = 0;  fcnt < ncolfmts;  fcnt++)  {
			startcell('d', cflist[fcnt].alstr);
			bigbuff[0] = '\0';
			(cflist[fcnt].form->fmt_fn)(jp, 0);
			html_fldprint(bigbuff);
			fputs("</td>", stdout);
		}
		fputs("</tr>\n", stdout);
	}

	pflgs |= GLV_ANYCHANGES;
	if  (mypriv->spu_flgs & PV_FORMS)
		pflgs |= GLV_FORMCHANGE;
	if  (mypriv->spu_flgs & PV_OTHERP)
		pflgs |= GLV_PTRCHANGE;
	if  (mypriv->spu_flgs & PV_CPRIO)
		pflgs |= GLV_PRIOCHANGE;
	if  (mypriv->spu_flgs & PV_ANYPRIO)
		pflgs |= GLV_ANYPRIO;
	if  (mypriv->spu_flgs & PV_COVER)
		pflgs |= GLV_COVER;
	if  (mypriv->spu_flgs & PV_ACCESSOK)
		pflgs |= GLV_ACCESSF;
	if  (mypriv->spu_flgs & PV_FREEZEOK)
		pflgs |= GLV_FREEZEF;
	printf("</table>\n<input type=hidden name=privs value=%u>\n", pflgs);
	printf("<input type=hidden name=prio value=\"%d,%d,%d\">\n",
	       mypriv->spu_defp, mypriv->spu_minp, mypriv->spu_maxp);
	printf("<input type=hidden name=copies value=%d>\n", mypriv->spu_cps);
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
	for  (fp = &uppertab[0],  lett = 'A';  fp < &uppertab[26];  lett++, fp++)
		if  (fp->statecode != 0)  {
			msg = gprompt(fp->statecode + 500);
			printf("list_format_code(\'%c\', \'%c\', \"%s\");\n", lett, fp->alch, msg);
			free(msg);
		}
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

int	set_user(char *arg)
{
	Displayopts.opt_restru = *arg? arg: (char *) 0;
	return  1;
}

int	set_incnull(char *arg)
{
	switch  (tolower(*arg))  {
	case  'a':
		Displayopts.opt_jinclude = JINCL_ALL;
		return  1;
	case  'y':
		Displayopts.opt_jinclude = JINCL_NULL;
		return  1;
	case  'n':
		Displayopts.opt_jinclude = JINCL_NONULL;
		return  1;
	default:
		return  0;
	}
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

int	set_prininc(char *arg)
{
	switch  (tolower(*arg))  {
	case  'n':
		Displayopts.opt_jprindisp = JRESTR_ALL;
		return  1;
	case  'u':
		Displayopts.opt_jprindisp = JRESTR_UNPRINT;
		return  1;
	case  'p':
		Displayopts.opt_jprindisp = JRESTR_PRINT;
		return  1;
	default:
		return  0;
	}
}

int	set_pagecnt(char *arg)
{
	switch  (tolower(*arg))  {
	case  'y':case '1':
		nopage = 0;
		return  1;
	case  'n':case '0':
		nopage = 1;
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
	{	"user",		AIF_ARG,	set_user	},
	{	"incnull",	AIF_ARG,	set_incnull	},
	{	"localonly",	AIF_ARG,	set_loco	},
	{	"prininc",	AIF_ARG,	set_prininc	},
	{	"pagecnt",	AIF_ARG,	set_pagecnt	},
	{	"listprfm",	AIF_ARGLIST  },
	{	"listopts",	AIF_NOARG,	perf_optselect	},
	{	"listformat",	AIF_NOARG,	perf_listformat	}
};

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

/* Invoke a call to the JavaScript function "jsfunc" with parameters
   taken from the job "jobnum" or defaults if null
   Title, suppress, Copies, Pri, form, printer, form list, printer list */

void	perf_listprfm(char *tmpl, char *jsfunc, char *jobnum)
{
	int	def_copies = 1, def_pri = mypriv->spu_defp, def_supp = 0;
	char	*def_ptr = mypriv->spu_ptr, *def_form = mypriv->spu_form, *def_tit = "";
	int	jcnt;
	struct	strvec	pv, fv;

	shm_setup();
	readjoblist(1);
	readptrlist(1);

	/* Reset defaults from job if we can find it */

	if  (jobnum)  {
		struct	jobswanted	jw;
		if  (decode_jnum(jobnum, &jw) == 0  &&  find_job(&jw))  {
			def_supp = jw.jp->spq_jflags & SPQ_NOH;
			def_copies = jw.jp->spq_cps;
			def_pri = jw.jp->spq_pri;
			def_ptr = stracpy(jw.jp->spq_ptr);
			def_form = stracpy(jw.jp->spq_form);
			def_tit = stracpy(jw.jp->spq_file);
		}
	}

	strvec_init(&pv);
	strvec_init(&fv);
	strvec_add(&pv, "");
	if  (mypriv->spu_ptr[0])
		strvec_add(&pv, mypriv->spu_ptr);
	strvec_add(&fv, mypriv->spu_form);

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		const struct spq *jp = &Job_seg.jj_ptrs[jcnt]->j;
		if  (jp->spq_ptr[0])
			strvec_add(&pv, jp->spq_ptr);
		strvec_add(&fv, jp->spq_form);
	}
	for  (jcnt = 0;  jcnt < Ptr_seg.nptrs;  jcnt++)  {
		const struct spptr *pp = &Ptr_seg.pp_ptrs[jcnt]->p;
		strvec_add(&pv, pp->spp_ptr);
		strvec_add(&fv, pp->spp_form);
	}
	strvec_sort(&pv);
	strvec_sort(&fv);

	/* OK now generate the stuff */

	html_output_file(tmpl, 1);
	printf("%s(\"%s\", %s, %d, %d, \"%s\", \"%s\", ",
	       jsfunc,
	       escquot(def_tit),
	       def_supp? "true": "false",
	       def_copies, def_pri, def_form, def_ptr);
	print_strvec(&fv);
	putchar(',');
	print_strvec(&pv);
	fputs(");\n", stdout);
	html_output_file("prfm_postamble", 0);
	exit(0);
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char	*spdir, **newargs;
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
	Realuname = prin_uname(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;

	/* Now we want to be Daemuid throughout if possible.  */

#if  	defined(OS_BSDI) || defined(OS_FREEBSD)
	seteuid(Daemuid);
#else
	setuid(Daemuid);
#endif
	hash_hostfile();
	interp_args(newargs);

	spdir = envprocess(SPDIR);
	if  (chdir(spdir) < 0)  {
		html_disperror($E{Cannot chdir});
		exit(E_NOCHDIR);
	}
	shm_setup();
	if  (nopage < 0)
		nopage = html_inibool("nopages", 0);
	if  (headerflag < 0)
		headerflag = html_inibool("headers", 0);
	fmt_setup();
	localptr = gprompt($P{Spq local printer});
	html_output_file("list_preamble", 1);
	readptrlist(1);
	readjoblist(0);
	jdisplay();
	jobshm_unlock();
	html_output_file("list_postamble", 0);
	return  0;
}
