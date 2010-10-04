/* rsqdcgi.c -- remove CGI delete jobs

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

int	gspool_fd;
char	*realuname;
struct	apispdet	mypriv;
int			Njobs, Nptrs;
struct	apispq		*job_list;
slotno_t		*jslot_list;
struct	ptr_with_slot	*ptr_sl_list;

/* For when we run out of memory.....  */

void  nomem()
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

void  perform_delete(char **args)
{
	char	*arg;

	for  (;  (arg = *args);  args++)  {
		int			ret;
		struct	jobswanted	jw;
		struct	apispq		job;

		if  (decode_jnum(arg, &jw))  {
			html_out_or_err("sbadargs", 1);
			exit(E_USAGE);
		}

		if  (gspool_jobfind(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.jno, jw.host, &jw.slot, &job) < 0)  {
			html_out_cparam_file("jobgone", 1, arg);
			exit(E_NOJOB);
		}

		if  ((!(mypriv.spu_flgs & PV_OTHERJ)  &&
		      strcmp(realuname, job.apispq_uname) != 0) ||
		     (job.apispq_netid != dest_hostid && !(mypriv.spu_flgs & PV_REMOTEJ)))  {
			html_out_cparam_file("nopriv", 1, arg);
			exit(E_NOPRIV);
		}

		if  ((ret = gspool_jobdel(gspool_fd, GSPOOL_FLAG_IGNORESEQ, jw.slot)) < 0)  {
			html_disperror($E{Base for API errors} + ret);
			exit(E_NOPRIV);
		}
	}
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
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
	perform_delete(newargs);
	html_out_or_err("delok", 1);
	return  0;
}
