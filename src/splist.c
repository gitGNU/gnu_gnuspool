/* splist.c -- list printers

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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
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
#include "xfershm.h"

int	spitoption(const int, const int, FILE *, const int, const int);
int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

DEF_DISPOPTS;
extern	char	freeze_wanted;
char	freeze_cd, freeze_hd, headerflag;
char	*Curr_pwd;

FILE	*Cfile;

#define	IPC_MODE	0600

int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;

char		*formatstring;

char	sdefaultfmt[] = "%p %d %f %s %j %u";

uid_t	Daemuid,
	Realuid,
	Effuid;

struct	ptrswanted	{
	netid_t		host;
	char		ptrname[PTRNAMESIZE+1];
}  *wanted_list;

unsigned	nwanted;

char	bigbuff[80];

struct	spdet	*mypriv;

/* Vector of states - assumed given prompt codes consecutively */

char	*statenames[SPP_NSTATES];

struct	xfershm		*Xfer_shmp;

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

static void	getwanted(char ** argv)
{
	char	**ap;
	struct	ptrswanted  *wp;
	unsigned	actw = 0;

	for  (ap = argv;  *ap;  ap++)
		nwanted++;

	/* There will be at least one, see call */

	if  (!(wanted_list = (struct ptrswanted *) malloc(nwanted * sizeof(struct ptrswanted))))
		nomem();

	wp = wanted_list;

	for  (ap = argv;  *ap;  ap++)  {
		char	*arg = *ap, *cp;
		if  ((cp = strchr(arg, ':')))  {
			*cp = '\0';
			if  ((wp->host = look_hostname(arg)) == 0L)  {
				disp_str = arg;
				print_error($E{Unknown job number});
				*cp = ':';
				continue;
			}
			if  (wp->host == myhostid)
				wp->host = 0L;
			*cp = ':';
			strncpy(wp->ptrname, cp+1, PTRNAMESIZE);
		}
		else  {
			wp->host = 0L;
			strncpy(wp->ptrname, arg, PTRNAMESIZE);
		}
		wp->ptrname[PTRNAMESIZE] = '\0';
		actw++;
		wp++;
	}

	if  (actw == 0)  {
		print_error($E{No arguments to process});
		exit(E_USAGE);
	}
	nwanted = actw;
}

static int	iswanted(const struct spptr * pp)
{
	unsigned	cnt;

	for  (cnt = 0;  cnt < nwanted;  cnt++)
		if  (wanted_list[cnt].host == pp->spp_netid  && strcmp(wanted_list[cnt].ptrname, pp->spp_ptr) == 0)
			return  1;
	return  0;
}

/* Open print file and get state names.  */

void	openpfile(void)
{
	int	i;

	if  (!ptrshminit(0))  {
		print_error($E{Cannot open pshm});
		exit(E_PRINQ);
	}

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
	char	*msg;		/* Heading */
	unsigned	(*fmt_fn)(const struct spptr *, const int);
};

#define	NULLCP	(char *) 0

struct	formatdef
	lowertab[] = { /* a-z */
	{	$P{Printer title}+'a'-1,	NULLCP,	fmt_ab		},	/* a */
	{	0,				NULLCP,	0		},	/* b */
	{	$P{Printer title}+'c'-1,	NULLCP,	fmt_class	},	/* c */
	{	$P{Printer title}+'d'-1,	NULLCP,	fmt_device	},	/* d */
	{	$P{Printer title}+'e'-1,	NULLCP,	fmt_comment	},	/* e */
	{	$P{Printer title}+'f'-1,	NULLCP,	fmt_form	},	/* f */
	{	0,				NULLCP,	0		},	/* g */
	{	$P{Printer title}+'h'-1,	NULLCP,	fmt_heoj	},	/* h */
	{	$P{Printer title}+'i'-1,	NULLCP,	fmt_pid		},	/* i */
	{	$P{Printer title}+'j'-1,	NULLCP,	fmt_jobno	},	/* j */
	{	0,				NULLCP,	0		},	/* k */
	{	$P{Printer title}+'l'-1,	NULLCP,	fmt_localonly	},	/* l */
	{	$P{Printer title}+'m'-1,	NULLCP,	fmt_message	},	/* m */
	{	$P{Printer title}+'n'-1,	NULLCP,	fmt_na		},	/* n */
	{	0,				NULLCP,	0		},	/* o */
	{	$P{Printer title}+'p'-1,	NULLCP, fmt_printer	},	/* p */
	{	0,				NULLCP,	0		},	/* q */
	{	0,				NULLCP, 0		},	/* r */
	{	$P{Printer title}+'s'-1,	NULLCP, fmt_state	},	/* s */
	{	$P{Printer title}+'t'-1,	NULLCP, fmt_ostate	},	/* t */
	{	$P{Printer title}+'u'-1,	NULLCP, fmt_user	},	/* u */
	{	0,				NULLCP,	0		},	/* v */
	{	0,				NULLCP,	0		},	/* w */
	{	0,				NULLCP,	0		},	/* x */
	{	$P{Printer title}+'y'-1,	NULLCP,	fmt_minsize	},	/* y */
	{	$P{Printer title}+'z'-1,	NULLCP,	fmt_maxsize	}	/* z */
};

/* Display contents of printer list.  */

void	pdisplay(void)
{
	int	pcnt;
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
	if  (pieces &&  !(lengths = (unsigned *) malloc(pieces * sizeof(unsigned))))
		nomem();
	for  (pc = 0;  pc < pieces;  pc++)
		lengths[pc] = 0;

	/* First scan to get width of each format */

	for  (pcnt = 0;  pcnt < Ptr_seg.nptrs;  pcnt++)  {
		const struct spptr *pp = &Ptr_seg.pp_ptrs[pcnt]->p;
		if  (pp->spp_state == SPP_NULL  ||  pp->spp_state >= SPP_NSTATES)
			continue;
		if  (nwanted != 0  &&  !iswanted(pp))
			continue;
		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
					lng = (lowertab[*fp - 'a'].fmt_fn)(pp, 0);
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
				do  fp++;  while  (lengths[pc] == 0  &&  *fp == ' ');
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

	for  (pcnt = 0;  pcnt < Ptr_seg.nptrs;  pcnt++)  {
		const struct spptr *pp = &Ptr_seg.pp_ptrs[pcnt]->p;
		if  (pp->spp_state == SPP_NULL  ||  pp->spp_state >= SPP_NSTATES)
			continue;
		if  (nwanted != 0  &&  !iswanted(pp))
			continue;
		fp = formatstring;
		pc = 0;
		while  (*fp)  {
			if  (*fp == '%')  {
				if  (!*++fp)
					break;
				bigbuff[0] = '\0'; /* Zap last thing */
				if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
					lng = (lowertab[*fp - 'a'].fmt_fn)(pp, (int) lengths[pc]);
				else
					goto  putit;
				fputs(bigbuff, stdout);
				if  (pc != pieces - 1)
					while  (lng < lengths[pc])  {
						putchar(' ');
						lng++;
					}
				do  fp++;  while  (lengths[pc] == 0  &&  *fp == ' ');
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

OPTION(o_explain)
{
	print_error($E{splist options});
	exit(0);
}

#include "inline/o_justq.c"
#include "inline/o_classc.c"
#include "inline/o_dloco.c"
#include "inline/o_format.c"
#include "inline/o_psort.c"
#include "inline/o_freeze.c"

/* Defaults and proc table for arg interp.  */

static	const	Argdefault	Adefs[] = {
	{  '?', $A{splist explain}	},
	{  'q', $A{splist list ptr}	},
	{  'C', $A{splist class}	},
	{  'l', $A{splist local}	},
	{  'r', $A{splist remotes}	},
	{  'F', $A{splist format}	},
	{  'D', $A{splist default format}},
	{  'H', $A{splist header}	},
	{  'N', $A{splist no header}	},
	{  'U', $A{splist unsorted}	},
	{  'S', $A{splist sorted}	},
	{ 0, 0 }
};

optparam  optprocs[] = {
o_explain,	o_justq,	o_classcode,	o_localonly,
o_nolocalonly,	o_formatstr,	o_formatdflt,	o_header,
o_noheader,	o_punsorted,	o_psorted,
o_freezecd,	o_freezehd
};

void	spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;
	fprintf(dest, "%s", name);
	cancont = spitoption(headerflag? $A{splist header}: $A{splist no header}, $A{splist explain}, dest, '=', cancont);
	cancont = spitoption(Displayopts.opt_sortptrs != SORTP_NONE? $A{splist sorted}: $A{splist unsorted}, $A{splist explain}, dest, ' ', cancont);
	if  (formatstring)  {
		spitoption($A{splist format}, $A{splist explain}, dest, ' ', 0);
		fprintf(dest, " \"%s\"", formatstring);
		cancont = 0;
	}
	else
		cancont = spitoption($A{splist default format}, $A{splist explain}, dest, ' ', cancont);

	spitoption(Displayopts.opt_localonly != NRESTR_NONE? $A{splist local}: $A{splist remotes}, $A{splist explain}, dest, ' ', cancont);
	spitoption($A{splist list ptr}, $A{splist explain}, dest, ' ', 0);
	fprintf(dest, Displayopts.opt_restrp? " \'%s\'": " -", Displayopts.opt_restrp);
	spitoption($A{splist class}, $A{splist explain}, dest, ' ', 0);
	fprintf(dest, " %s\n", hex_disp(Displayopts.opt_classcode, 0));
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
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
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	hash_hostfile();
	argv = optprocess(argv, Adefs, optprocs, $A{splist explain}, $A{splist freeze home}, 0);

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

	if  (!jobshminit(0))  {
		print_error($E{Cannot open jshm});
		exit(E_JOBQ);
	}

	openpfile();
	if  (argv[0])
		getwanted(argv);
	if  (!formatstring)
		formatstring = sdefaultfmt;
	readptrlist(0);
	pdisplay();
	return  0;
}
