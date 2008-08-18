/* sqlist.c -- shell command to list jobs

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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
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
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"
#include "cgifndjb.h"
#ifdef	SHAREDLIBS
#include "xfershm.h"
#endif

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

uid_t	Daemuid,
	Realuid,
	Effuid;

DEF_DISPOPTS;

int		nopage,
		headerflag;

unsigned	jno_width;

char		*formatstring;

char	sdefaultfmt[] = "%N %u %h %f %Q %S %c %p %P",
	npdefaultfmt[] = "%N %u %h %f %L %K %c %p %P";

struct	spdet	*mypriv;

char	*Realuname;

struct  jobswanted  *wanted_list;

unsigned	nwanted;
char	*pagearg;

char	bigbuff[80];

extern	char	freeze_wanted;
char	freeze_cd, freeze_hd;
char	Viewj = 0;
char	*Curr_pwd;

static	char	*localptr;

FILE	*Cfile;

#define	IPC_MODE	0600

int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;

#ifdef	SHAREDLIBS
struct	xfershm		*Xfer_shmp;
#endif

int	spitoption(const int, const int, FILE *, const int, const int);
int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
FILE	*net_feed(const int, const netid_t, const slotno_t, const jobno_t);

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

static void	getwanted(char ** argv)
{
	char	**ap;
	struct	jobswanted  *wp;
	unsigned	actw = 0;

	for  (ap = argv;  *ap;  ap++)
		nwanted++;

	/* There will be at least one, see call */

	if  (!(wanted_list = (struct jobswanted *) malloc(nwanted * sizeof(struct jobswanted))))
		nomem();

	wp = wanted_list;

	for  (ap = argv;  *ap;  ap++)  {
		int	retc = decode_jnum(*ap, wp);
		if  (retc != 0)  {
			print_error(retc);
			continue;
		}
		actw++;
		wp++;
	}

	if  (actw == 0)  {
		print_error($E{No arguments to process});
		exit(E_USAGE);
	}
	nwanted = actw;
}

static int	iswanted(const struct spq * jp)
{
	unsigned	cnt;

	for  (cnt = 0;  cnt < nwanted;  cnt++)
		if  (wanted_list[cnt].jno == jp->spq_job  &&  wanted_list[cnt].host == jp->spq_netid)
			return  1;
	return  0;
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
	char	*msg;		/* Heading */
	unsigned	(*fmt_fn)(const struct spq *, const int);
};

#define	NULLCP	(char *) 0

struct	formatdef
	uppertab[] = { /* A-Z */
	{	$P{job fmt title}+'A'-1,	NULLCP,	fmt_wattn	},	/* A */
	{	0,				NULLCP,	0		},	/* B */
	{	$P{job fmt title}+'C'-1,	NULLCP,	fmt_class	},	/* C */
	{	$P{job fmt title}+'D'-1,	NULLCP,	fmt_delim	},	/* D */
	{	0,				NULLCP,	0		},	/* E */
	{	$P{job fmt title}+'F'-1,	NULLCP,	fmt_ppflags	},	/* F */
	{	$P{job fmt title}+'G'-1,	NULLCP,	fmt_hat		},	/* G */
	{	$P{job fmt title}+'H'-1,	NULLCP, fmt_hold	},	/* H */
	{	0,				NULLCP, 0		},	/* I */
	{	0,				NULLCP,	0		},	/* J */
	{	$P{job fmt title}+'K'-1,	NULLCP,	fmt_sizek	},	/* K */
	{	$P{job fmt title}+'L'-1,	NULLCP, fmt_kreached	},	/* L */
	{	0,				NULLCP, 0		},	/* M */
	{	$P{job fmt title}+'N'-1,	NULLCP, fmt_jobno	},	/* N */
	{	$P{job fmt title}+'O'-1,	NULLCP,	fmt_oddeven	},	/* O */
	{	$P{job fmt title}+'P'-1,	NULLCP, fmt_printer	},	/* P */
	{	$P{job fmt title}+'Q'-1,	NULLCP,	fmt_pgreached	},	/* Q */
	{	$P{job fmt title}+'R'-1,	NULLCP, fmt_range	},	/* R */
	{	$P{job fmt title}+'S'-1,	NULLCP, fmt_szpages	},	/* S */
	{	$P{job fmt title}+'T'-1,	NULLCP, fmt_nptime	},	/* T */
	{	$P{job fmt title}+'U'-1,	NULLCP, fmt_puser	},	/* U */
	{	0,				NULLCP,	0		},	/* V */
	{	$P{job fmt title}+'W'-1,	NULLCP,	fmt_stime	},	/* W */
	{	0,				NULLCP, 0		},	/* X */
	{	0,				NULLCP,	0		},	/* Y */
	{	0,				NULLCP,	0		}	/* Z */
},
	lowertab[] = { /* a-z */
	{	$P{job fmt title}+'a'-1,	NULLCP,	fmt_mattn	},	/* a */
	{	0,				NULLCP,	0		},	/* b */
	{	$P{job fmt title}+'c'-1,	NULLCP,	fmt_cps		},	/* c */
	{	$P{job fmt title}+'d'-1,	NULLCP,	fmt_delimnum	},	/* d */
	{	0,				NULLCP,	0		},	/* e */
	{	$P{job fmt title}+'f'-1,	NULLCP,	fmt_form	},	/* f */
	{	0,				NULLCP,	0		},	/* g */
	{	$P{job fmt title}+'h'-1,	NULLCP,	fmt_title	},	/* h */
	{	0,				NULLCP,	0		},	/* i */
	{	0,				NULLCP,	0		},	/* j */
	{	0,				NULLCP,	0		},	/* k */
	{	$P{job fmt title}+'l'-1,	NULLCP,	fmt_localonly	},	/* l */
	{	$P{job fmt title}+'m'-1,	NULLCP,	fmt_mail	},	/* m */
	{	0,				NULLCP,	0		},	/* n */
	{	$P{job fmt title}+'o'-1,	NULLCP,	fmt_orighost	},	/* o */
	{	$P{job fmt title}+'p'-1,	NULLCP, fmt_prio	},	/* p */
	{	$P{job fmt title}+'q'-1,	NULLCP,	fmt_retain	},	/* q */
	{	0,				NULLCP, 0		},	/* r */
	{	$P{job fmt title}+'s'-1,	NULLCP, fmt_supph	},	/* s */
	{	$P{job fmt title}+'t'-1,	NULLCP, fmt_ptime	},	/* t */
	{	$P{job fmt title}+'u'-1,	NULLCP, fmt_user	},	/* u */
	{	0,				NULLCP,	0		},	/* v */
	{	$P{job fmt title}+'w'-1,	NULLCP,	fmt_write	},	/* w */
	{	0,				NULLCP,	0		},	/* x */
	{	0,				NULLCP,	0		},	/* y */
	{	0,				NULLCP,	0		}	/* z */
};

/* Display contents of job file.  */

void	jdisplay(void)
{
	int	jcnt;
	char	*fp;
	unsigned  pieces, pc, *lengths = (unsigned *) 0;
	int	lng;

	pieces = 0;
	fp = formatstring;
	while  (*fp)  {
		if  (*fp == '%')  {
			if  (!*++fp)
				break;
			if  ((isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)  ||
			     (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn))
				pieces++;
		}
		fp++;
	}
	if  (pieces &&  !(lengths = (unsigned *) malloc(pieces * sizeof(unsigned))))
		nomem();
	for  (pc = 0;  pc < pieces;  pc++)
		lengths[pc] = 0;

	/* Initial scan to get job number width */

	jno_width = 0;

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		const  struct  spq  *jp = &Job_seg.jj_ptrs[jcnt]->j;
		char	nbuf[12];

		if  (jp->spq_job == 0)
			break;
		if  (nwanted != 0  &&  !iswanted(jp))
			continue;
#ifdef	CHARSPRINTF
		sprintf(nbuf, "%ld", (long) jp->spq_job);
		lng = strlen(nbuf);
#else
		lng = sprintf(nbuf, "%ld", (long) jp->spq_job);
#endif
		if  (lng > jno_width)
			jno_width = lng;
	}

	/* Second scan to get width of each format */

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		const  struct  spq  *jp = &Job_seg.jj_ptrs[jcnt]->j;
		if  (jp->spq_job == 0)
			break;
		if  (nwanted != 0  &&  !iswanted(jp))
			continue;
		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)
					lng = (uppertab[*fp - 'A'].fmt_fn)(jp, 0);
				else  if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
					lng = (lowertab[*fp - 'a'].fmt_fn)(jp, 0);
				else  {
					fp++;
					continue;
				}
				if  (lng > lengths[pc])
					lengths[pc] = lng;
				pc++;
			}
			fp++;
		}
	}

	/* Possibly expand columns for header */

	if  (headerflag)  {
		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)  {
					if  (!uppertab[*fp - 'A'].msg)
						uppertab[*fp - 'A'].msg = gprompt(uppertab[*fp - 'A'].statecode);
					lng = strlen(uppertab[*fp - 'A'].msg);
				}
				else  if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)  {
					if  (!lowertab[*fp - 'a'].msg)
						lowertab[*fp - 'a'].msg = gprompt(lowertab[*fp - 'a'].statecode);
					lng = strlen(lowertab[*fp - 'a'].msg);
				}
				else  {
					fp++;
					continue;
				}
				if  (lng > lengths[pc])
					lengths[pc] = lng;
				pc++;
			}
			fp++;
		}

		/* And now output it...  */

		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)  {
					fputs(uppertab[*fp - 'A'].msg, stdout);
					lng = strlen(uppertab[*fp - 'A'].msg);
				}
				else  if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)  {
					fputs(lowertab[*fp - 'a'].msg, stdout);
					lng = strlen(lowertab[*fp - 'a'].msg);
				}
				else
					goto  putit1;
				if  (pc != pieces - 1)
					while  (lng < lengths[pc])  {
						putchar(' ');
						lng++;
					}
				do  fp++;
				while  (lengths[pc] == 0  &&  *fp == ' ');
				pc++;
				continue;
			}
		putit1:
			putchar(*fp);
			fp++;
		}
		putchar('\n');
	}

	/* Final run-through to output stuff */

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		const struct spq *jp = &Job_seg.jj_ptrs[jcnt]->j;
		if  (jp->spq_job == 0)
			break;
		if  (nwanted != 0  &&  !iswanted(jp))
			continue;
		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				bigbuff[0] = '\0'; /* Zap last thing */
				if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)
					lng = (uppertab[*fp - 'A'].fmt_fn)(jp, (int) lengths[pc]);
				else  if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
					lng = (lowertab[*fp - 'a'].fmt_fn)(jp, (int) lengths[pc]);
				else
					goto  putit;
				fputs(bigbuff, stdout);
				if  (pc != pieces - 1)
					while  (lng < lengths[pc])  {
						putchar(' ');
						lng++;
					}
				do  fp++;
				while  (lengths[pc] == 0  &&  *fp == ' ');
				pc++;
				continue;
			}
		putit:
			putchar(*fp);
			fp++;
		}
		putchar('\n');
	}
}

/* See if page is in required range.
   Return -1 if no chance of being in range, might as well finish.
   Return 0 if not in range.
   Return 1 if in range. */

static int	inrange(const unsigned page)
{
	char	*cp;
	ULONG	num1, num2;
	int	lessr = 0;

	cp = pagearg;
	while  (*cp)  {
		num1 = 0;
		while  (isdigit(*cp))
			num1 = num1 * 10 + *cp++ - '0';
		if  (*cp == '-')  {
			cp++;
			if  (isdigit(*cp))  {
				num2 = 0;
				while  (isdigit(*cp))
					num2 = num2 * 10 + *cp++ - '0';
			}
			else
				num2 = LOTSANDLOTS;
		}
		else  {
			num2 = num1;
			if  (*cp == ',')
				cp++;
		}
		if  (page < num1)
			lessr++;
		else  if  (page <= num2)
			return  1;
	}
	return  lessr? 0: -1;
}

static void	viewj(const struct spq * jp)
{
	int		haspages = 0;
	char		*delim = (char *) 0;
	unsigned	pagenums = 0;
	LONG		*pageoffs = (LONG *) 0;
	FILE		*ifl;
	struct	pages	pfe;

	if  (pagearg  &&  (haspages = rdpgfile(jp, &pfe, &delim, &pagenums, &pageoffs)) < 0)
		nomem();
	if  (jp->spq_netid)
		ifl = net_feed(FEED_NPSP, jp->spq_netid, jp->spq_rslot, jp->spq_job);
	else
		ifl = fopen(mkspid(SPNAM, jp->spq_job), "r");

	if  (!ifl)  {
		disp_arg[0] = jp->spq_job;
		if  (jp->spq_netid)  {
			disp_str = look_host(jp->spq_netid);
			print_error($E{sqlist cant view remote job});
		}
		else
			print_error($E{sqlist cant view local job});
		goto  endview;
	}
	if  (pagearg)  {
		LONG	current_offset = 0L;
		unsigned  current_page = 1;
		int	rg, ch = ' ';
		if  (haspages)  {
			while  ((rg = inrange(current_page)) >= 0)  {
				if  (rg == 0)  {
					while  (current_offset < pageoffs[current_page]  &&  (ch = getc(ifl)) != EOF)
						current_offset++;
				}
				else  while  (current_offset < pageoffs[current_page]  &&  (ch = getc(ifl)) != EOF)  {
					current_offset++;
					putchar(ch);
				}
				if  (ch == EOF)
					break;
				current_page++;
			}
		}
		else  {
			while  ((rg = inrange(current_page)) >= 0)  {
				if  (rg == 0)  {
					while  ((ch = getc(ifl)) != EOF  &&  ch != '\f')
						current_offset++;
					current_offset++;		/* To include the \f */
				}
				else  {
					while  ((ch = getc(ifl)) != EOF)  {
						current_offset++;
						putchar(ch);
						if  (ch == '\f')
							break;
					}
				}
				if  (ch == EOF)
					break;
				current_page++;
			}
		}
	}
	else  {
		int	ch;
		while  ((ch = getc(ifl)) != EOF)
			putchar(ch);
	}
	fclose(ifl);
endview:
	if  (haspages)  {
		free(delim);
		free((char *) pageoffs);
	}
}

void	check_viewj(const struct spq * jp)
{
	if  (!(mypriv->spu_flgs & PV_VOTHERJ)  &&  strcmp(Realuname, jp->spq_uname) != 0)  {
		disp_arg[0] = jp->spq_job;
		disp_str2 = (char *) jp->spq_uname;
		if  (jp->spq_netid)  {
			disp_str = look_host(jp->spq_netid);
			print_error($E{sqlist no view permission remote});
		}
		else
			print_error($E{sqlist no view permission local});
		return;
	}
	if  (jp->spq_netid  &&  !(mypriv->spu_flgs & PV_REMOTEJ))  {
		print_error($E{sqlist no remote job priv});
		return;
	}
	viewj(jp);
}

static void	doview(void)
{
	unsigned  jcnt;

	if  (nwanted != 0)  {
		for  (jcnt = 0;  jcnt < nwanted;  jcnt++)  {
			const  Hashspq *hjp = find_job(&wanted_list[jcnt]);
			if  (hjp)
				check_viewj(&hjp->j);
		}
	}
	else  {
		for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
			const  struct  spq  *jp = &Job_seg.jj_ptrs[jcnt]->j;
			if  (jp->spq_job == 0)
				continue;
			check_viewj(jp);
		}
	}
}

OPTION(o_explain)
{
	print_error($E{sqlist options});
	exit(0);
}

#include "inline/o_dpage.c"
#include "inline/o_justq.c"
#include "inline/o_justu.c"
#include "inline/o_allj.c"
#include "inline/o_classc.c"
#include "inline/o_dloco.c"
#include "inline/o_format.c"
#include "inline/o_freeze.c"
#include "inline/o_jinclall.c"

OPTION(o_viewjob)
{
	Viewj = 1;
	return  OPTRESULT_OK;
}

OPTION(o_noviewjob)
{
	Viewj = 0;
	return  OPTRESULT_OK;
}

OPTION(o_viewpages)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (pagearg)
		free(pagearg);
	pagearg = arg[0] && (arg[0] != '-' || arg[1])? stracpy(arg): (char *) 0;
	return  OPTRESULT_ARG_OK;
}

/* Defaults and proc table for arg interp.  */

const	Argdefault	Adefs[] = {
  {  '?', $A{sqlist explain}	},
  {  'q', $A{sqlist list only}	},
  {  'u', $A{sqlist just user}	},
  {  't', $A{sqlist just title}	},
  {  'y', $A{sqlist unprinted jobs}	},
  {  'p', $A{sqlist printed jobs}	},
  {  'Y', $A{sqlist all jobs}	},
  {  'C', $A{sqlist classcode}	},
  {  'e', $A{sqlist no page counts}	},
  {  'E', $A{sqlist page counts}	},
  {  'l', $A{sqlist local}	},
  {  'r', $A{sqlist remotes}	},
  {  'F', $A{sqlist format}	},
  {  'D', $A{sqlist def format}	},
  {  'H', $A{sqlist header}	},
  {  'N', $A{sqlist no header}	},
  {  'z', $A{sqlist include null}	},
  {  'Z', $A{sqlist no include null}	},
  {  'I', $A{sqlist include all}	},
  {  'V', $A{sqlist view job}  },
  {  'n', $A{sqlist no view job}  },
  {  'P', $A{sqlist view pages}  },
  {  0, 0 }
};

optparam  optprocs[] = {
o_explain,	o_justq,	o_justu,	o_justt,
o_justnp,	o_justp,	o_allj,		o_classcode,
o_nopage,	o_page,		o_localonly,	o_nolocalonly,
o_formatstr,	o_formatdflt,	o_header,	o_noheader,
o_jinclnull,	o_jinclnonull,	o_jinclall,
o_viewjob,	o_noviewjob,	o_viewpages,
o_freezecd,	o_freezehd
};

void	spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;
	fprintf(dest, "%s", name);
	cancont = spitoption(Displayopts.opt_localonly != NRESTR_NONE? $A{sqlist local}: $A{sqlist remotes}, $A{sqlist explain}, dest, '=', cancont);
	cancont = spitoption(nopage? $A{sqlist no page counts}: $A{sqlist page counts}, $A{sqlist explain}, dest, ' ', cancont);
	cancont = spitoption(headerflag? $A{sqlist header}: $A{sqlist no header}, $A{sqlist explain}, dest, ' ', cancont);
	cancont = spitoption(Displayopts.opt_jprindisp == JRESTR_PRINT? $A{sqlist printed jobs}:
			     Displayopts.opt_jprindisp == JRESTR_UNPRINT? $A{sqlist unprinted jobs}:
			     $A{sqlist all jobs}, $A{sqlist explain}, dest, ' ', cancont);
	cancont = spitoption(Displayopts.opt_jinclude == JINCL_ALL? $A{sqlist include all}:
			     Displayopts.opt_jinclude == JINCL_NULL? $A{sqlist include null}:
			     $A{sqlist no include null}, $A{sqlist explain}, dest, ' ', cancont);
	cancont = spitoption(Viewj? $A{sqlist view job}: $A{sqlist no view job}, $A{sqlist explain}, dest, ' ', cancont);
	if  (formatstring)  {
		spitoption($A{sqlist format}, $A{sqlist explain}, dest, ' ', 0);
		fprintf(dest, " \"%s\"", formatstring);
	}
	else
		spitoption($A{sqlist def format}, $A{sqlist explain}, dest, ' ', cancont);
	spitoption($A{sqlist list only}, $A{sqlist explain}, dest, ' ', 0);
	fprintf(dest, Displayopts.opt_restrp? " \'%s\'": " -", Displayopts.opt_restrp);
	spitoption($A{sqlist just user}, $A{sqlist explain}, dest, ' ', 0);
	fprintf(dest, Displayopts.opt_restru? " \'%s\'": " -", Displayopts.opt_restru);
	spitoption($A{sqlist just title}, $A{sqlist explain}, dest, ' ', 0);
	fprintf(dest, Displayopts.opt_restrt? " \'%s\'": " -", Displayopts.opt_restrt);
	spitoption($A{sqlist classcode}, $A{sqlist explain}, dest, ' ', 0);
	fprintf(dest, " %s", hex_disp(Displayopts.opt_classcode, 0));
	spitoption($A{sqlist view pages}, $A{sqlist explain}, dest, ' ', 0);
	if  (pagearg)
		fprintf(dest, " \'%s\'\n", pagearg);
	else
		fprintf(dest, " -\n");
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char	*spdir;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;

	Cfile = open_cfile(MISC_UCONFIG, "rest.help");

	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	if  ((mypriv->spu_flgs & (PV_OTHERJ|PV_VOTHERJ)) != (PV_OTHERJ|PV_VOTHERJ))
		Realuname = prin_uname(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	hash_hostfile();
	argv = optprocess(argv, Adefs, optprocs, $A{sqlist explain}, $A{sqlist freeze home}, 0);

	/* Now we want to be Daemuid throughout if possible.  */

#if  	defined(OS_BSDI) || defined(OS_FREEBSD)
	seteuid(Daemuid);
#else
	setuid(Daemuid);
#endif

#include "inline/freezecode.c"

	if  (freeze_wanted)
		exit(0);

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		print_error($E{Spooler not running});
		exit(E_NOTRUN);
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	if  (!jobshminit(0))  {
		print_error($E{Cannot open jshm});
		exit(E_JOBQ);
	}
	if  (!ptrshminit(0))  {
		print_error($E{Cannot open pshm});
		exit(E_PRINQ);
	}
	if  (argv[0])
		getwanted(argv);
	if  (!formatstring)
		formatstring = nopage? npdefaultfmt: sdefaultfmt;
	localptr = gprompt($P{Spq local printer});
	spdir = envprocess(SPDIR);
	if  (chdir(spdir) < 0)  {
		print_error($E{Cannot chdir});
		exit(E_NOCHDIR);
	}
	if  (Viewj)  {
		readjoblist(1);
		doview();
	}
	else  {
		readptrlist(1);
		readjoblist(0);
		jdisplay();
	}
	return  0;
}
