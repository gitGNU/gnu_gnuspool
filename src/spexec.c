/* spexec.c -- kludge program to run things under the identity of the original owner

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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include "incl_unix.h"
#include "defaults.h"
#include "files.h"
#include "ecodes.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"
#include "displayopt.h"


/* This is a helper program for macros in user interface stuff so
   that macro commands get run under the identity of the user who
   invoked the original UI command.

   It has to be run set-uid "root" so that setuid(x) eliminates
   all traces of any set-user stuff.

   Originally it just did setuid(getuid()) but that doesn't work
   for GTK stuff which won't work set-user. To fool GTK you have
   to do setreuid to spooler losing the real uid. So what we do
   in the GTK programs is check that $HOME is owned by the real
   user and we set-user to the owner of the HOME directory here.

   I think this is secure but if it isn't please let me know
   ASAP!!! (and suggest how to fix) */

FILE	*Cfile;
uid_t	Realuid, Effuid, Daemuid;
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
DEF_DISPOPTS;

void	nomem()
{
	exit(E_NOMEM);
}


/* Rewrite the first argument of the argument list so that
   things like spchange -c3 work in macros. (Otherwise the
   macro command tries to run "spchange -c3" as a command name). */


char **rewrite(char **argv)
{
	unsigned  nargs = 1;	/* Including null */
	char	**ap, *sp, *np;
	char	**result, **rp;

	/* If it doesn't have spaces in, nothing to do */

	if  (!strchr(argv[0], ' '))
		return  argv;

	/* Count the number of args */

	for  (ap = argv;  *ap;  ap++)
		nargs++;

	/* Assume increase in number of args is same as number of spaces
	   this may be an overestimate */

	for  (sp = argv[0];  (np = strchr(sp, ' '));  sp = np + 1)
		nargs++;

	result = (char **) malloc(nargs * sizeof(char *));
	if  (!result)
		exit(E_NOMEM);

	rp = result;

	for  (sp = argv[0];  (np = strchr(sp, ' '));  sp = np)  {
		*np = '\0';	/* Assume doesn't matter if we clobber it */
		*rp++ = sp;
		do  np++;
		while  (*np == ' ');
	}

	/* Add trailing stuff */

	if  (*sp)
		*rp++ = sp;

	/* Add rest of args */

	for  (ap = argv + 1;  *ap;  ap++)
		*rp++ = *ap;
	*rp = (char *) 0;

	return  result;
}

MAINFN_TYPE  main(int argc, char **argv)
{
	struct  passwd  *pw = getpwnam(SPUNAME);
	uid_t  duid = ROOTID, myuid = getuid();
	char	**newargv;

	versionprint(argv, "$Revision: 1.2 $", 1);

	/* Get spooler uid */

	if  (pw)
		duid = pw->pw_uid;
	endpwent();

	/* If the real user id is "spooler" this is either because we have invoked the
	   original ui program as spooler or because we have invoked a GTK program which
	   switched the real uid to spooler. In such cases we fish the uid out of the
	   home directory and use that.

	   Refuse to work if no home directory or it looks strange */

	if  (myuid == duid)  {
		char	*homed = getenv("HOME");
		struct  stat  sbuf;

		if  (!homed  ||  stat(homed, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)
			exit(E_SETUP);

		myuid = sbuf.st_uid;
	}

	/* Switch completely to the user id and do the bizniz. */

	setuid(myuid);
	newargv = rewrite(argv);
	execvp(newargv[0], newargv);
	exit(E_SPEXEC2);
}
