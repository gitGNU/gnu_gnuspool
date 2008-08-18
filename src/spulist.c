/* spulist.c -- list users and permissions

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
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>

#include "defaults.h"
#include "files.h"
#include "spuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "helpargs.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#ifdef	SHAREDLIBS
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"
#include "displayopt.h"
#endif

FILE	*Cfile;

char	*Curr_pwd;
uid_t	Realuid,
	Effuid,
	Daemuid;

#define	SORT_NONE	0
#define	SORT_USER	1

extern	char	freeze_wanted;
char	freeze_cd, freeze_hd, headerflag;
static	char	alphsort = SORT_NONE, defline = 1, ulines = 1;

extern	struct	sphdr	Spuhdr;

static	char	*defaultname, *allname, *licu, *unlicu, *privnames[NUM_PRIVS];
static	ULONG	privbits[] = {
PV_ADMIN,	PV_SSTOP,	PV_FORMS,	PV_OTHERP,
PV_CPRIO,	PV_OTHERJ,	PV_PRINQ,	PV_HALTGO,
PV_ANYPRIO,	PV_CDEFLT,	PV_ADDDEL,	PV_COVER,
PV_UNQUEUE,	PV_VOTHERJ,	PV_REMOTEJ,	PV_REMOTEP,
PV_ACCESSOK,	PV_FREEZEOK
};

char		*formatstring;

char	sdefaultfmt[] = "%u %d %l %m %n %c %p";

char	bigbuff[80];

#ifdef	SHAREDLIBS
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
DEF_DISPOPTS;
#endif

int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));
int	spitoption(const int, const int, FILE *, const int, const int);

/* For benefit of library routines */

void	nomem(void)
{
	print_error($E{NO MEMORY});
	exit(E_NOMEM);
}

typedef	unsigned	fmt_t;

static fmt_t	fmt_form(struct spdet *up, const int fwidth)
{
	return  (fmt_t) strlen(strcpy(bigbuff, up? up->spu_form: Spuhdr.sph_form));
}

static fmt_t	fmt_printer(struct spdet *up, const int fwidth)
{
	return  (fmt_t) strlen(strcpy(bigbuff, up? up->spu_ptr: Spuhdr.sph_ptr));
}

static fmt_t	fmt_formallow(struct spdet *up, const int fwidth)
{
	return  (fmt_t) strlen(strcpy(bigbuff, up? up->spu_formallow: Spuhdr.sph_formallow));
}

static fmt_t	fmt_ptrallow(struct spdet *up, const int fwidth)
{
	return  (fmt_t) strlen(strcpy(bigbuff, up? up->spu_ptrallow: Spuhdr.sph_ptrallow));
}

static fmt_t	fmt_class(struct spdet *up, const int fwidth)
{
	return  (fmt_t) strlen(strcpy(bigbuff, hex_disp(up? up->spu_class: Spuhdr.sph_class, 0)));
}

static fmt_t	fmt_defpri(struct spdet *up, const int fwidth)
{
#ifdef	CHARSPRINTF
	sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->spu_defp: (unsigned) Spuhdr.sph_defp);
	return  (fmt_t) strlen(bigbuff);
#else
	return	(fmt_t) sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->spu_defp: (unsigned) Spuhdr.sph_defp);
#endif
}

static fmt_t	fmt_minpri(struct spdet *up, const int fwidth)
{
#ifdef	CHARSPRINTF
	sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->spu_minp: (unsigned) Spuhdr.sph_minp);
	return  (fmt_t) strlen(bigbuff);
#else
	return	(fmt_t) sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->spu_minp: (unsigned) Spuhdr.sph_minp);
#endif
}

static fmt_t	fmt_maxpri(struct spdet *up, const int fwidth)
{
#ifdef	CHARSPRINTF
	sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->spu_maxp: (unsigned) Spuhdr.sph_maxp);
	return  (fmt_t) strlen(bigbuff);
#else
	return	(fmt_t) sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->spu_maxp: (unsigned) Spuhdr.sph_maxp);
#endif
}

static fmt_t	fmt_copies(struct spdet *up, const int fwidth)
{
#ifdef	CHARSPRINTF
	sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->spu_cps: (unsigned) Spuhdr.sph_cps);
	return  (fmt_t) strlen(bigbuff);
#else
	return	(fmt_t) sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->spu_cps: (unsigned) Spuhdr.sph_cps);
#endif
}

static fmt_t	fmt_priv(struct spdet *up, const int fwidth)
{
	ULONG	priv = up? up->spu_flgs: Spuhdr.sph_flgs;

	if  ((priv & ALLPRIVS) == ALLPRIVS)
		return  (fmt_t) strlen(strcpy(bigbuff, allname));
	else  {
		char	*nxt = bigbuff;
		unsigned  pn, totl = 0, lng;
		for  (pn = 0;  pn < NUM_PRIVS;  pn++)
			if  (priv & privbits[pn])  {
				lng = strlen(strcpy(nxt, privnames[pn]));
				nxt += lng;
				*nxt++ = ' ';
				totl += lng + 1;
			}
		if  (totl == 0)
			return  0;
		*--nxt = '\0';
		return  totl - 1;
	}
}

static fmt_t	fmt_user(struct spdet *up, const int fwidth)
{
	return  (fmt_t) strlen(strcpy(bigbuff, up? prin_uname((uid_t) up->spu_user) : defaultname));
}

static fmt_t	fmt_licu(struct spdet *up, const int fwidth)
{
	return  (fmt_t) (up? strlen(strcpy(bigbuff, up->spu_isvalid == SPU_VALID? licu: unlicu)) : 0);
}

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct	formatdef  {
	SHORT	statecode;	/* Code number for heading if applicable */
	char	*msg;		/* Heading */
	unsigned	(*fmt_fn)(struct spdet *, const int);
};

#define	NULLCP	(char *) 0

struct	formatdef
	lowertab[] = { /* a-z */
	{	$P{Spulist title}+'a'-1,	NULLCP,	fmt_formallow	},	/* a */
	{	$P{Spulist title}+'b'-1,	NULLCP,	fmt_ptrallow	},	/* b */
	{	$P{Spulist title}+'c'-1,	NULLCP,	fmt_class	},	/* c */
	{	$P{Spulist title}+'d'-1,	NULLCP,	fmt_defpri	},	/* d */
	{	0,				NULLCP,	0		},	/* e */
	{	$P{Spulist title}+'f'-1,	NULLCP,	fmt_form	},	/* f */
	{	0,				NULLCP,	0		},	/* g */
	{	0,				NULLCP,	0		},	/* h */
	{	0,				NULLCP,	0		},	/* i */
	{	0,				NULLCP,	0		},	/* j */
	{	0,				NULLCP,	0		},	/* k */
	{	$P{Spulist title}+'l'-1,	NULLCP,	fmt_minpri	},	/* l */
	{	$P{Spulist title}+'m'-1,	NULLCP,	fmt_maxpri	},	/* m */
	{	$P{Spulist title}+'n'-1,	NULLCP,	fmt_copies	},	/* n */
	{	$P{Spulist title}+'o'-1,	NULLCP,	fmt_printer	},	/* o */
	{	$P{Spulist title}+'p'-1,	NULLCP, fmt_priv	},	/* p */
	{	0,				NULLCP,	0		},	/* q */
	{	0,				NULLCP, 0		},	/* r */
	{	0,				NULLCP, 0		},	/* s */
	{	0,				NULLCP, 0		},	/* t */
	{	$P{Spulist title}+'u'-1,	NULLCP, fmt_user	},	/* u */
	{	$P{Spulist title}+'v'-1,	NULLCP,	fmt_licu	},	/* v */
	{	0,				NULLCP,	0		},	/* w */
	{	0,				NULLCP,	0		},	/* x */
	{	0,				NULLCP,	0		},	/* y */
	{	0,				NULLCP,	0		}	/* z */
};

/* Display contents of user list.  */

void	udisplay(struct spdet *ul, const unsigned nu)
{
	struct  spdet	*up;
	struct  spdet  *ep = &ul[nu];
	char	*fp;
	unsigned  pieces, pc, *lengths = (unsigned *) 0;
	int	lng;

	pieces = 0;
	fp = formatstring;
	while  (*fp)  {
		if  (*fp == '%')  {
			if  (!*++fp)
				break;
			if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
				pieces++;
		}
		fp++;
	}
	if  (pieces  &&  !(lengths = (unsigned *) malloc(pieces * sizeof(unsigned))))
		nomem();
	for  (pc = 0;  pc < pieces;  pc++)
		lengths[pc] = 0;

	/* First scan to get width of each format */

	if  (defline)  {
		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
					lng = (lowertab[*fp - 'a'].fmt_fn)((struct spdet *) 0, 0);
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
	if  (ulines)  {
		for  (up = ul;  up < ep;  up++)  {
			fp = formatstring;
			pc = 0;
			while  (*fp)  {
				if  (*fp == '%')  {
					if  (!*++fp)
						break;
					if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
						lng = (lowertab[*fp - 'a'].fmt_fn)(up, 0);
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
	}

	/* Possibly expand columns for header */

	if  (headerflag)  {
		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)  {
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
				if  (!(islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn))
					goto  putit1;
				fputs(lowertab[*fp - 'a'].msg, stdout);
				lng = strlen(lowertab[*fp - 'a'].msg);
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

	if  (defline)  {
		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				bigbuff[0] = '\0'; /* Zap last thing */
				if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
					lng = (lowertab[*fp - 'a'].fmt_fn)((struct spdet *) 0, (int) lengths[pc]);
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

	if  (ulines)  {
		for  (up = ul;  up < ep;  up++)  {
			fp = formatstring;
			pc = 0;
			while  (*fp)  {
				if  (*fp == '%')  {
					if  (!*++fp)
						break;
					bigbuff[0] = '\0'; /* Zap last thing */
					if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
						lng = (lowertab[*fp - 'a'].fmt_fn)(up, (int) lengths[pc]);
					else
						goto  putit2;
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
			putit2:
				putchar(*fp);
				fp++;
			}
			putchar('\n');
		}
	}
}

OPTION(o_explain)
{
	print_error($E{spulist options});
	exit(0);
}

#include "inline/o_usort.c"
#include "inline/o_format.c"
#include "inline/o_freeze.c"

OPTION(o_defline)
{
	defline = 1;
	return  OPTRESULT_OK;
}

OPTION(o_nodefline)
{
	defline = 0;
	return  OPTRESULT_OK;
}

OPTION(o_ulines)
{
	ulines = 1;
	return  OPTRESULT_OK;
}

OPTION(o_noulines)
{
	ulines = 0;
	return  OPTRESULT_OK;
}

/* Defaults and proc table for arg interp.  */

static	const	Argdefault  Adefs[] = {
	{  '?', $A{spulist explain}	},
	{  'u', $A{spulist sort user}	},
	{  'n', $A{spulist sort uid}	},
	{  'd', $A{spulist default line}},
	{  's', $A{spulist no default line}},
	{  'U', $A{spulist user lines}},
	{  'S', $A{spulist no user lines}},
	{  'F', $A{spulist format}	},
	{  'D', $A{spulist default format}},
	{  'H', $A{spulist header}	},
	{  'N', $A{spulist no header}	},
	{ 0, 0 }
};

optparam  optprocs[] = {
o_explain,	o_usort,	o_nsort,	o_defline,
o_nodefline,	o_ulines,	o_noulines,	o_formatstr,
o_formatdflt,	o_header,	o_noheader,
o_freezecd,	o_freezehd
};

void	spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;
	fprintf(dest, "%s", name);
	cancont = spitoption(alphsort == SORT_USER?
			  $A{spulist sort user}:
			  $A{spulist sort uid},
			  $A{spulist explain}, dest, '=', cancont);
	cancont = spitoption(headerflag? $A{spulist header}:
			     $A{spulist no header},
			     $A{spulist explain}, dest, ' ', cancont);
	cancont = spitoption(defline? $A{spulist default line}:
			     $A{spulist no default line},
			     $A{spulist explain}, dest, ' ', cancont);
	cancont = spitoption(ulines? $A{spulist user lines}:
			     $A{spulist no user lines},
			     $A{spulist explain}, dest, ' ', cancont);
	if  (formatstring)  {
		spitoption($A{spulist format}, $A{spulist explain}, dest, ' ', 0);
		fprintf(dest, " \"%s\"", formatstring);
		cancont = 0;
	}
	else
		cancont = spitoption($A{spulist default format}, $A{spulist explain}, dest, ' ', cancont);
	putc('\n', dest);
}

int	sort_u(struct spdet *a, struct spdet *b)
{
	return  strcmp(prin_uname((uid_t) a->spu_user), prin_uname((uid_t) b->spu_user));
}

MAINFN_TYPE	main(int argc, char **argv)
{
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif
	struct	spdet	*mypriv, *ulist = (struct spdet *) 0;
	unsigned	Nu = 0, pn;

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
	argv = optprocess(argv, Adefs, optprocs, $A{spulist explain}, $A{spulist freeze home}, 0);
	SWAP_TO(Daemuid);

	if  (!(defline || ulines))  {
		print_error($E{spulist nothing to do});
		exit(E_USAGE);
	}
	defaultname = gprompt($P{Spulist default name});
	allname = gprompt($P{Spulist all name});
	licu = gprompt($P{Spulist lic user});
	unlicu = gprompt($P{Spulist unlic user});

	for  (pn = 0;  pn < NUM_PRIVS;  pn++)
		privnames[pn] = gprompt($P{Priv adm} + pn);

	if  (!(mypriv = getspuentry(Realuid)))  {
		print_error($E{Not registered yet});
		exit(E_UNOTSETUP);
	}
#include "inline/freezecode.c"
	if  (freeze_wanted)
		exit(0);
	if  (!(mypriv->spu_flgs & PV_ADMIN))  {
		print_error($E{shell no admin file priv});
		exit(E_NOPRIV);
	}
	if  (spu_needs_rebuild)
		print_error($E{Spufile needs rebuild});
	if  (ulines)  {
		ulist = getspulist(&Nu);
		if  (alphsort == SORT_USER)
			qsort(QSORTP1 ulist, Nu, sizeof(struct spdet), QSORTP4 sort_u);
	}
	if  (!formatstring)
		formatstring = sdefaultfmt;
	udisplay(ulist, Nu);
	exit(0);
}
