/* lpstatc.c -- pretend to be UNIX "lpstat"

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
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"

/* Semaphore structures.  */

struct	spdet	*mypriv;

char	*format;

#define	IPC_MODE	0600

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

/* Display contents of job file.  */

void	jdisplay(void)
{
	int	jcnt;
	const  struct  spq  *jp;
	time_t	st;
	struct	tm	*tp;
	char	jobnbuf[30];
	static	char	months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		jp = &Job_seg.jj_ptrs[jcnt]->j;
		if  (jp->spq_job == 0)
			break;

		if  (jp->spq_netid)
			sprintf(jobnbuf, "%s:%ld", look_host(jp->spq_netid), (long) jp->spq_job);
		else
			sprintf(jobnbuf, "%ld", (long) jp->spq_job);
		st = jp->spq_time;
		tp = localtime(&st);
		printf(format, jobnbuf, jp->spq_uname, jp->spq_size, &months[tp->tm_mon*3], tp->tm_mday, tp->tm_hour, tp->tm_min);
		putchar('\n');
	}
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	int	ch;
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
	SWAP_TO(Realuid);

	Displayopts.opt_classcode = mypriv->spu_class;

	while  ((ch = getopt(argc, argv, "a:c:f:l:o:p:D:rRsS:tu:v:")) != EOF)
		switch  (ch)  {
		default:
			return  E_USAGE;
		case  'a':case 'c':case 'f':case 'l':case 'o':
		case  'p':case 'D':case 'r':case 'R':case 's':
		case  'S':case 't':case 'u':case 'v':
			fprintf(stderr, "Sorry - '-%c' option not supported yet\n", ch);
			break;
		}

	if  (!(format = getenv("LPSTATFMT")))
	     format = "%-24s%-8s%14d   %3.3s %.2d %.2d:%.2d";

	/* Now we want to be Daemuid throughout if possible.  */

	setuid(Daemuid);

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
	readjoblist(0);
	jdisplay();
	return  0;
}
