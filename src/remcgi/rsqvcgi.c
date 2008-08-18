/* rsqvcgi.c -- remote CGI view jobs

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

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

#define	PAGE_LUMP	100

void	perform_view(char *jnum, char *pnum)
{
	int			myjob, haspgfile = 0;
	unsigned		wpage = 0;
	LONG			*pageoffs = (LONG *) 0;
	FILE			*ifl;
	struct	jobswanted	jw;
	struct	apispq		job;

	if  (!jnum  ||  decode_jnum(jnum, &jw))  {
		html_out_or_err("sbadargs", 1);
		exit(0);
	}
	if  (gspool_jobfind(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.jno, jw.host, &jw.slot, &job) < 0)  {
	jobgone:
		html_out_cparam_file("jobgone", 1, jnum);
		exit(0);
	}
	myjob = strcmp(realuname, job.apispq_uname) == 0;

	if  (!((mypriv.spu_flgs & PV_VOTHERJ)  ||  myjob)  ||
	     (job.apispq_netid != dest_hostid  &&  !(mypriv.spu_flgs & PV_REMOTEJ)))  {
		html_out_cparam_file("nopriv", 1, jnum);
		exit(0);
	}

	/* If file is not paged, wpage will be left at 0 */

	if  (job.apispq_npages > 1  &&  pnum  &&  isdigit(pnum[0]))  {

		/* Set up wpage in case we don't have a page file */

		wpage = strtoul(pnum, (char **) 0, 0);
		if  (wpage == 0)
			wpage = 1;
		else  if  (wpage > job.apispq_npages)
			wpage = job.apispq_npages;

		if  ((ifl = gspool_jobpbrk(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.slot)))  {
			int	cnt, nitems;
			unsigned	npc = 0, nalloc = 0;
			struct	apipages	pfe;
			if  (fread((char *) &pfe, sizeof(pfe), 1, ifl) != 1)  {
				fclose(ifl);
				goto  nopf;
			}
			/* Skim over delimiter */
			for  (cnt = 0;  cnt < pfe.deliml;  cnt++)
				if  (getc(ifl) == EOF)
					break;
			do  {
				if  (npc >= nalloc)  {
					nalloc += PAGE_LUMP;
					if  (npc != 0)
						pageoffs = (LONG *) realloc((char *) pageoffs, nalloc * sizeof(LONG));
					else
						pageoffs = (LONG *) malloc(nalloc * sizeof(LONG));
					if  (!pageoffs)
						html_nomem();
				}
				nitems = fread((char *) &pageoffs[npc], sizeof(LONG), PAGE_LUMP, ifl);
				npc += nitems;
			}  while  (nitems > 0);

			fclose(ifl);
			if  (npc < wpage)  {
				wpage = 0; /* Huh? */
				goto  nopf;
			}
			haspgfile = 1;
		}
	}

 nopf:
	if  (!(ifl = gspool_jobdata(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.slot)))
		goto  jobgone;

	html_out_or_err("viewstart", 1);

	if  (wpage != 0)  {
		int	ch = EOF;
		unsigned  current_page = 1;

		fputs("<SCRIPT LANGUAGE=\"JavaScript\">\n", stdout);
		printf("page_viewheader(\"%s\", \"%s\", %u, %lu, %ld, %ld, %ld, %d);\n",
		       jnum, job.apispq_file, wpage, (unsigned long) job.apispq_npages,
		       (long) (job.apispq_start+1), (long)(job.apispq_end+1), (long) (job.apispq_haltat? job.apispq_haltat+1: 0),
		       (mypriv.spu_flgs & PV_OTHERJ) || myjob);
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

		/* Soak up rest of file */
		while  (ch != EOF)
			ch = getc(ifl);
	}
	else  {
		int	ch;
		fputs("<SCRIPT LANGUAGE=\"JavaScript\">\n", stdout);
		printf("viewheader(\"%s\", \"%s\", %d);\n",
		       jnum, job.apispq_file, (mypriv.spu_flgs & PV_OTHERJ) || myjob);
		fputs("</SCRIPT>\n<PRE>", stdout);
		while  ((ch = getc(ifl)) != EOF)
			html_pre_putchar(ch);
	}
	fclose(ifl);
	fputs("</PRE>\n", stdout);
	html_out_or_err("viewend", 0);
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
	perform_view(newargs[0], newargs[1]);
	return  0;
}
