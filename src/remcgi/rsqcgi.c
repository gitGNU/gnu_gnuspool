/* rsqcgi.c -- remote CGI list jobs

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
#include "displayopt.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "listperms.h"
#include "rcgilib.h"

int	nopage = -1,
	headerflag = -1;

char	*formatstring;
char	sdefaultfmt[] = "LN Lu Lh Lf LQ RS Rc Rp LP",
	npdefaultfmt[] = "LN Lu Lh Lf LL RK Rc Rp LP";

char	*realuname;
int			gspool_fd;
struct	apispdet	mypriv;

char	bigbuff[200];

static	char	*localptr;

int		Njobs, Nptrs;
struct	apispq	*job_list;
slotno_t	*jslot_list;
struct	ptr_with_slot	*ptr_sl_list;

/* For when we run out of memory.....  */

void  nomem()
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

void  fmt_setup()
{
	if  (!formatstring)  {
		if  (!(formatstring = html_inistr("format", (char *) 0)))
			formatstring = nopage? npdefaultfmt: sdefaultfmt;
	}
}

extern char *hex_disp(const classcode_t, const int);
typedef	unsigned	fmt_t;
#define	LOTSANDLOTS	99999999L	/* Maximum page number */
#define	INLINE_SQLIST
#define	spq		apispq
#define	spq_job		apispq_job
#define	spq_netid	apispq_netid
#define	spq_orighost	apispq_orighost
#define	spq_rslot	apispq_rslot
#define	spq_time	apispq_time
#define	spq_proptime	apispq_proptime
#define	spq_starttime	apispq_starttime
#define	spq_hold	apispq_hold
#define	spq_nptimeout	apispq_nptimeout
#define	spq_ptimeout	apispq_ptimeout
#define	spq_size	apispq_size
#define	spq_posn	apispq_posn
#define	spq_pagec	apispq_pagec
#define	spq_npages	apispq_npages
#define	spq_cps		apispq_cps
#define	spq_pri		apispq_pri
#define	spq_wpri	apispq_wpri
#define	spq_jflags	apispq_jflags
#define	SPQ_NOH		APISPQ_NOH
#define	SPQ_WRT		APISPQ_WRT
#define	SPQ_MAIL	APISPQ_MAIL
#define	SPQ_RETN	APISPQ_RETN
#define	SPQ_ODDP	APISPQ_ODDP
#define	SPQ_EVENP	APISPQ_EVENP
#define	SPQ_REVOE	APISPQ_REVOE
#define	SPQ_MATTN	APISPQ_MATTN
#define	SPQ_WATTN	APISPQ_WATTN
#define	SPQ_LOCALONLY	APISPQ_LOCALONLY
#define	SPQ_CLIENTJOB	APISPQ_CLIENTJOB
#define	SPQ_ROAMUSER	APISPQ_ROAMUSER
#define	spq_sflags	apispq_sflags
#define	SPQ_ASSIGN	APISPQ_ASSIGN
#define	SPQ_WARNED	APISPQ_WARNED
#define	SPQ_PROPOSED	APISPQ_PROPOSED
#define	SPQ_ABORTJ	APISPQ_ABORTJ
#define	spq_dflags	apispq_dflags
#define	SPQ_PQ		APISPQ_PQ
#define	SPQ_PRINTED	APISPQ_PRINTED
#define	SPQ_STARTED	APISPQ_STARTED
#define	SPQ_PAGEFILE	APISPQ_PAGEFILE
#define	SPQ_ERRLIMIT	APISPQ_ERRLIMIT
#define	SPQ_PGLIMIT	APISPQ_PGLIMIT
#define	spq_extrn	apispq_extrn
#define	spq_pglim	apispq_pglim
#define	spq_class	apispq_class
#define	spq_pslot	apispq_pslot
#define	spq_start	apispq_start
#define	spq_end		apispq_end
#define	spq_haltat	apispq_haltat
#define	spq_uid		apispq_uid
#define	spq_uname	apispq_uname
#define	spq_puname	apispq_puname
#define	spq_file	apispq_file
#define	spq_form	apispq_form
#define	spq_ptr		apispq_ptr
#define	spq_flags	apispq_flags
#define	SPP_NULL	API_PRNULL

#include "inline/jfmt_wattn.c"
#include "inline/jfmt_class.c"
#include "inline/jfmt_ppf.c"
#include "inline/jfmt_hold.c"
#include "inline/jfmt_sizek.c"
#include "inline/jfmt_krchd.c"

static fmt_t  fmt_jobno(const struct spq *jp, const int fwidth)
{
	if  (jp->spq_netid != dest_hostid)
		sprintf(bigbuff, "%s:%ld", look_host(jp->spq_netid), (long) jp->spq_job);
	else
		sprintf(bigbuff, "%ld", (long) jp->spq_job);
	return  0;		/* Don't really worry about result */
}

#include "inline/jfmt_oddev.c"

static fmt_t  fmt_printer(const struct apispq *jp, const int fwidth)
{
	if  (jp->apispq_dflags & APISPQ_PQ)  {
		struct	apispptr  *pp = find_ptr(jp->apispq_pslot);
		if  (pp  &&  pp->apispp_state != SPP_NULL)  {
			if  (pp->apispp_netid == dest_hostid)
				sprintf(bigbuff, "%s:%s", look_host(pp->apispp_netid), pp->apispp_ptr);
			else
				strcpy(bigbuff, pp->apispp_ptr);
			return  0;
		}
		strcpy(bigbuff, localptr);
		return  0;
	}
	strcpy(bigbuff, jp->apispq_ptr);
	return  0;
}

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

static fmt_t  fmt_orighost(const struct apispq *jp, const int fwidth)
{
	if  (jp->apispq_jflags & HT_ROAMUSER)
		sprintf(bigbuff, "(%s)", jp->apispq_uname);
	strcpy(bigbuff, look_host(jp->apispq_orighost));
	return  0;
}

static int  get_delim(const struct apispq *jp, int *num, int *lng, char **delimp)
{
	/* jp is an entry in job_list the way this is used */

	FILE	*ifl = gspool_jobpbrk(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jslot_list[jp - job_list]);
	struct	apipages  fpl;
	if  (!ifl)
		return  0;
	if  (fread((char *) &fpl, sizeof(fpl), 1, ifl) != 1)
		return  0;

	*num = fpl.delimnum;
	*lng = fpl.deliml;

	if  (!(*delimp = malloc((unsigned) fpl.deliml)))
		html_nomem();
	if  (fread(*delimp, 1, fpl.deliml, ifl) != fpl.deliml)  {
		free(*delimp);
		return  0;
	}
	while  (getc(ifl) != EOF)
		;
	fclose(ifl);
	return  1;
}

static fmt_t  fmt_delimnum(const struct spq *jp, const int fwidth)
{
	char	*delim;
	int	ndelim = 1, lng;

	if  (get_delim(jp, &ndelim, &lng, &delim))
		free(delim);
	sprintf(bigbuff, "%d", ndelim);
	return  0;
}

static fmt_t  fmt_delim(const struct spq *jp, const int fwidth)
{
	char	*delim;
	int	ndelim = 1, lng;
	char	*outp = bigbuff;

	if  (get_delim(jp, &ndelim, &lng, &delim))  {
		int	ii;
		for  (ii = 0;  ii < lng;  ii++)  {
			int	ch = delim[ii] & 255;
			if  (!isascii(ch))
				sprintf(outp, "\\x%.2x", ch);
			else  if  (iscntrl(ch))  {
				switch  (ch)  {
				case  033:
					strcpy(outp, "\\e");
					break;
				case  ('h' & 0x1f):
					strcpy(outp, "\\b");
					break;
				case  '\r':
					strcpy(outp, "\\r");
					break;
				case  '\n':
					strcpy(outp, "\\n");
					break;
				case  '\f':
					strcpy(outp, "\\f");
					break;
				case  '\t':
					strcpy(outp, "\\t");
					break;
				case  '\v':
					strcpy(outp, "\\v");
					break;
				default:
					sprintf(outp, "^%c", ch | 0x40);
					break;
				}
			}
			else  {
				if  (ch == '\\'  ||  ch == '^')
					*outp++ = ch;
				*outp++ = ch;
				continue;
			}
			outp += strlen(outp);
		}
		free((char *) delim);
		*outp = '\0';
	}
	else
		strcpy(bigbuff, "\\f");
	return  0;
}

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct	formatdef  {
	SHORT	statecode;	/* Code number for heading if applicable */
	char	alch;		/* Default align char */
	char	*msg;		/* Heading */
	unsigned  (*fmt_fn)(const struct apispq *, const int);
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

void	print_hdrfmt(struct formatdef *fp)
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

struct	altype	*lookup_align(const int alch)
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

void  find_commonest(char *fp)
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

void  startrow()
{
	printf("<tr align=%s>\n", commonest_align->str);
}

void  startcell(const int celltype, const char *str)
{
	if  (str)
		printf("<t%c align=%s>", celltype, str);
	else
		printf("<t%c>", celltype);
}

/* Display contents of job file.  */

void  jdisplay()
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

	for  (jcnt = 0;  jcnt < Njobs;  jcnt++)  {
		const struct apispq *jp = &job_list[jcnt];
		unsigned  hval = 0;

		/* Prune out stuff which doesn't belong */

		if  ((jp->apispq_class & Displayopts.opt_classcode) == 0)
			continue;

		if  (Displayopts.opt_jprindisp != JRESTR_ALL)  {
			if  (Displayopts.opt_jprindisp == JRESTR_UNPRINT)  {
				if  (jp->apispq_dflags & SPQ_PRINTED)
					continue;
			}
			else  if  (!(jp->apispq_dflags & SPQ_PRINTED))
				continue;
		}
		if  (Displayopts.opt_restru  &&  !qmatch(Displayopts.opt_restru, jp->apispq_uname))
			continue;

		if  (Displayopts.opt_restrp)  {
			/* Gyrations as jp is read-only (and issubset modifies args) */
			char	pb[JPTRNAMESIZE+1];
			switch  (Displayopts.opt_jinclude)  {
			case  JINCL_NONULL:
				if  (!jp->spq_ptr[0])
					continue;
				goto  chkmtch;
			case  JINCL_NULL:
				if  (!jp->spq_ptr[0])
					break;
			chkmtch:
				strncpy(pb, jp->spq_ptr, JPTRNAMESIZE);
				pb[JPTRNAMESIZE] = '\0';
				if  (!issubset(Displayopts.opt_restrp, pb))
					continue;
			default:
				break;
			}
		}

		startrow();
		startcell('d', commonest_align->str);
		fmt_jobno(jp, 0);
		if  (strcmp(realuname, jp->apispq_uname) == 0)
			hval |= LV_MINEORVIEW|LV_DELETEABLE|LV_CHANGEABLE;
		else  {
			if  (mypriv.spu_flgs & PV_OTHERJ)
				hval |= LV_DELETEABLE|LV_CHANGEABLE;
			if  (mypriv.spu_flgs & PV_VOTHERJ)
				hval |= LV_MINEORVIEW;
		}
		if  (mypriv.spu_flgs & PV_REMOTEJ  ||  jp->spq_netid == dest_hostid)
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
	if  (mypriv.spu_flgs & PV_FORMS)
		pflgs |= GLV_FORMCHANGE;
	if  (mypriv.spu_flgs & PV_OTHERP)
		pflgs |= GLV_PTRCHANGE;
	if  (mypriv.spu_flgs & PV_CPRIO)
		pflgs |= GLV_PRIOCHANGE;
	if  (mypriv.spu_flgs & PV_ANYPRIO)
		pflgs |= GLV_ANYPRIO;
	if  (mypriv.spu_flgs & PV_COVER)
		pflgs |= GLV_COVER;
	if  (mypriv.spu_flgs & PV_ACCESSOK)
		pflgs |= GLV_ACCESSF;
	if  (mypriv.spu_flgs & PV_FREEZEOK)
		pflgs |= GLV_FREEZEF;
	printf("</table>\n<input type=hidden name=privs value=%u>\n", pflgs);
	printf("<input type=hidden name=prio value=\"%d,%d,%d\">\n",
	       mypriv.spu_defp, mypriv.spu_minp, mypriv.spu_maxp);
	printf("<input type=hidden name=copies value=%d>\n", mypriv.spu_cps);
	printf("<input type=hidden name=classcode value=%lu>\n", (unsigned long) mypriv.spu_class);
}

struct	arginterp  {
	char	*argname;
	unsigned  short  flags;
#define	AIF_NOARG	0
#define	AIF_ARG		1
#define	AIF_ARGLIST	2
	int  (*arg_fn)(char *);
};

int  perf_listformat(char *notused)
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

extern int  perf_optselect(char *);

int  set_queue(char *arg)
{
	Displayopts.opt_restrp = *arg? arg: (char *) 0;
	return  1;
}

int  set_user(char *arg)
{
	Displayopts.opt_restru = *arg? arg: (char *) 0;
	return  1;
}

int  set_incnull(char *arg)
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

int  set_loco(char *arg)
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

int  set_prininc(char *arg)
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

int  set_pagecnt(char *arg)
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

int  set_header(char *arg)
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

int  set_format(char *arg)
{
	formatstring = arg;
	return  1;
}

extern void  perf_listprfm(char *, char *, char *);

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

int  perf_optselect(char *notused)
{
	html_out_param_file("setopts", 1, 0, html_cookexpiry());
	exit(0);
}

void  interp_args(char **args)
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
   Copies, form, printer, form list, printer list */

void  perf_listprfm(char *tmpl, char *jsfunc, char *jobnum)
{
	int	def_copies = 1, def_pri, def_supp = 0;
	char	*def_ptr, *def_form, *def_tit = "";
	int	jcnt;
	struct	strvec	pv, fv;

	/* Reset defaults from job if we can find it */

	api_open(realuname);
	def_pri = mypriv.spu_defp;
	def_ptr = mypriv.spu_ptr;
	def_form = mypriv.spu_form;
	Displayopts.opt_classcode = mypriv.spu_class;
	read_jobqueue(0);
	api_readptrs(0);

	if  (jobnum)  {
		struct	jobswanted	jw;
		struct	apispq		job;
		if  (decode_jnum(jobnum, &jw) == 0  &&
		     gspool_jobfind(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.jno, jw.host, &jw.slot, &job) >= 0)  {
			def_supp = job.apispq_jflags & APISPQ_NOH;
			def_copies = job.apispq_cps;
			def_pri = job.apispq_pri;
			def_ptr = stracpy(job.apispq_ptr);
			def_form = stracpy(job.apispq_form);
			def_tit = stracpy(job.apispq_file);
		}
	}

	strvec_init(&pv);
	strvec_init(&fv);
	strvec_add(&pv, "");
	if  (mypriv.spu_ptr[0])
		strvec_add(&pv, mypriv.spu_ptr);
	strvec_add(&fv, mypriv.spu_form);

	for  (jcnt = 0;  jcnt < Njobs;  jcnt++)  {
		const struct apispq *jp = &job_list[jcnt];
		if  (jp->apispq_ptr[0])
			strvec_add(&pv, jp->apispq_ptr);
		strvec_add(&fv, jp->apispq_form);
	}
	for  (jcnt = 0;  jcnt < Nptrs;  jcnt++)  {
		const struct apispptr *pp = &ptr_sl_list[jcnt].ptr;
		strvec_add(&pv, pp->apispp_ptr);
		strvec_add(&fv, pp->apispp_form);
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

MAINFN_TYPE  main(int argc, char **argv)
{
	char	**newargs;
	unsigned	Jaccess_flags = 0;
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
	newargs = cgi_arginterp(argc, argv, CGI_AI_REMHOST);
	/* Side effect of cgi_arginterp is to set Realuid */
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");
	realuname = prin_uname(Realuid);
	setgid(getgid());
	setuid(Realuid);
	interp_args(newargs);
	if  (Displayopts.opt_localonly == NRESTR_LOCALONLY)
		Jaccess_flags |= GSPOOL_FLAG_LOCALONLY;
	api_open(realuname);
	Displayopts.opt_classcode = mypriv.spu_class;
	read_jobqueue(Jaccess_flags);
	api_readptrs(0);
	if  (nopage < 0)
		nopage = html_inibool("nopages", 0);
	if  (headerflag < 0)
		headerflag = html_inibool("headers", 0);
	fmt_setup();
	localptr = gprompt($P{Spq local printer});
	html_output_file("list_preamble", 1);
	jdisplay();
	html_output_file("list_postamble", 0);
	return  0;
}
