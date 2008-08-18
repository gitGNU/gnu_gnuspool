/* sqvcgi.c -- CGI program to view jobs

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
#include "listperms.h"
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

struct	spdet	*mypriv;
char	*Realuname;

FILE	*Cfile;

#define	IPC_MODE	0600

int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
#ifdef	SHAREDLIBS
struct	xfershm		*Xfer_shmp;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;

int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
FILE	*net_feed(const int, const netid_t, const slotno_t, const jobno_t);

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

void	perform_view(char *jnum, char *pnum)
{
	struct	jobswanted	jw;
	const  struct	spq	*jp;
	int		haspgfile = 0, myjob;
	char		*delim = (char *) 0;
	unsigned	pagenums = 0, wpage = 0;
	LONG		*pageoffs = (LONG *) 0;
	FILE		*ifl;
	struct	pages	pfe;

	if  (!jnum  ||  decode_jnum(jnum, &jw))  {
		html_out_or_err("sbadargs", 1);
		exit(0);
	}

	if  (!find_job(&jw))  {
	jgone:
		html_out_cparam_file("jobgone", 1, jnum);
		exit(0);
	}

	jp = jw.jp;
	myjob = strcmp(Realuname, jp->spq_uname) == 0;

	if  (!((mypriv->spu_flgs & PV_VOTHERJ)  ||  myjob)  ||
	     (jp->spq_netid  &&  !(mypriv->spu_flgs & PV_REMOTEJ)))  {
		html_out_cparam_file("nopriv", 1, jnum);
		exit(0);
	}

	/* If file is not paged, wpage will be left at 0 */

	if  (jp->spq_npages > 1  &&  pnum  &&  isdigit(pnum[0])  &&
	     (haspgfile = rdpgfile(jp, &pfe, &delim, &pagenums, &pageoffs)) >= 0)  {
		wpage = strtoul(pnum, (char **) 0, 0);
		if  (wpage == 0)
			wpage = 1;
		else  if  (wpage > jp->spq_npages)
			wpage = jp->spq_npages;
	}

	if  (jp->spq_netid)
		ifl = net_feed(FEED_NPSP, jp->spq_netid, jp->spq_rslot, jp->spq_job);
	else
		ifl = fopen(mkspid(SPNAM, jp->spq_job), "r");

	if  (!ifl)
		goto  jgone;

	html_out_or_err("viewstart", 1);

	if  (wpage != 0)  {
		int	ch = ' '; /* Initialise to avoid it being EOF */
		unsigned  current_page = 1;

		fputs("<SCRIPT LANGUAGE=\"JavaScript\">\n", stdout);
		printf("page_viewheader(\"%s\", \"%s\", %u, %lu, %ld, %ld, %ld, %d);\n",
		       jnum, jp->spq_file, wpage, (long) jp->spq_npages,
		       (long) (jp->spq_start+1), (long) (jp->spq_end+1), (long) (jp->spq_haltat? jp->spq_haltat+1: 0),
		       (mypriv->spu_flgs & PV_OTHERJ) || myjob);
		fputs("</SCRIPT>\n<PRE>\n", stdout);

		/* Find start of required page */

		if  (haspgfile)  {

			/* Case where delimiter is not a formfeed */

			LONG	current_offset = 0L;

			while  (current_page < wpage)  {
				while  (current_offset < pageoffs[current_page]  &&  (ch = getc(ifl)) != EOF)
					current_offset++;
				current_page++;
			}

			while  (current_offset < pageoffs[current_page])  {
				if  ((ch = getc(ifl)) == EOF)
					break;
				html_pre_putchar(ch);
				current_offset++;
			}
		}
		else  {
			/* Delimiter is a formfeed.
			   Read through to start of page and then spit out
			   the page */

			while  (current_page < wpage)  {
				while  ((ch = getc(ifl)) != '\f'  &&  ch != EOF)
					;
				current_page++;
			}
			while  ((ch = getc(ifl)) != '\f'  &&  ch != EOF)
				html_pre_putchar(ch);
		}

		/* Soak up rest of job if on network.
		   Seems to benefit network if all slurped up */

		if  (jp->spq_netid)
			while  (ch != EOF)
				ch = getc(ifl);
	}
	else  {
		int	ch;
		fputs("<SCRIPT LANGUAGE=\"JavaScript\">\n", stdout);
		printf("viewheader(\"%s\", \"%s\", %d);\n",
		       jnum, jp->spq_file, (mypriv->spu_flgs & PV_OTHERJ) || myjob);
		fputs("</SCRIPT>\n<PRE>", stdout);
		while  ((ch = getc(ifl)) != EOF)
			html_pre_putchar(ch);
	}
	fputs("</PRE>\n", stdout);
	html_out_or_err("viewend", 0);
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char	*spdir, **newargs;
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
	hash_hostfile();

	/* Now we want to be Daemuid throughout if possible.  */

#if  	defined(OS_BSDI) || defined(OS_FREEBSD)
	seteuid(Daemuid);
#else
	setuid(Daemuid);
#endif

	spdir = envprocess(SPDIR);
	if  (chdir(spdir) < 0)  {
		html_disperror($E{Cannot chdir});
		exit(E_NOCHDIR);
	}

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		html_disperror($E{Spooler not running});
		exit(E_NOTRUN);
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		html_disperror($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	if  (!jobshminit(1))  {
		html_disperror($E{Cannot open jshm});
		exit(E_JOBQ);
	}
	if  (!ptrshminit(1))  {
		html_disperror($E{Cannot open pshm});
		exit(E_PRINQ);
	}
	readjoblist(1);
	readptrlist(1);

	perform_view(newargs[0], newargs[1]);
	return  0;
}
