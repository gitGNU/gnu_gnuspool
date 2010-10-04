/* gtksave.c -- Save options in ~/.gnuspool for GTK progs

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
#include "defaults.h"
#include "incl_unix.h"
#include "files.h"
#include "ecodes.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"

#define	BUFFSIZE	256

extern	uid_t	Realuid, Effuid, Daemuid;

void	nomem()
{
	fprintf(stderr, "Out of memory\n");
	exit(E_NOMEM);
}

/* Field names are alternate arguments */

int  isfld(char *buff, char **argv)
{
	char	*ep = strchr(buff, '=');
	char	**ap;
	unsigned  lng = ep - buff;

	if  (!ep)
		return  0;

	for  (ap = argv + 1;  *ap;  ap += 2)  {
		if  (lng != strlen(*ap))
			continue;
		if  (strncmp(buff, *ap, lng) == 0)
			return  1;
	}
	return  0;
}

/*  Arguments are:
    1. Encoded options XSPQDISPOPT
    2. Encoded class code XSPQDISPCC
    3. Users to limit display to XSPQDISPUSER
    4. Printers to limit display to XSPQDISPPTR
    5. Job titles to limit display to XSPQDISPTIT
    6. Fields for job view XSPQJOBFLD
    7. Fields for job view XSPQPTRFLD */

MAINFN_TYPE  main(int argc, char **argv)
{
	char	*homed;
	int	oldumask, cnt;
	FILE	*xtfile;

	versionprint(argv, "$Revision: 1.2 $", 1);

	/* If we haven't got the right arguments then just quit.
	   This is only meant to be run by xspq.
	   Maybe one day we'll have a more sophisticated routine. */

	if  ((argc & 1) == 0)
		return  E_USAGE;

	if  (!(homed = getenv("HOME")))  {
		struct  passwd  *pw = getpwuid(getuid());
		if  (!pw)
			return  E_SETUP;
		homed = pw->pw_dir;
	}

	if  (chdir(homed) < 0)
		return  E_SETUP;

	/* Set umask so anyone can read the file (home dir mush be at least 0111). */

	oldumask = umask(0);
	umask(oldumask & ~0444);

	if  ((xtfile = fopen(USER_CONFIG, "r")))  {
		FILE  *tmpf = tmpfile();
		char	buffer[BUFFSIZE];

		while  (fgets(buffer, BUFFSIZE, xtfile))  {
			if  (!isfld(buffer, argv))
				fputs(buffer, tmpf);
		}
		rewind(tmpf);
		fclose(xtfile);
		if  (!(xtfile = fopen(USER_CONFIG, "w")))
			return  E_NOPRIV;
		while  (fgets(buffer, BUFFSIZE, tmpf))
			fputs(buffer, xtfile);
	}
	else  if  (!(xtfile = fopen(USER_CONFIG, "w")))
		return  E_NOPRIV;

	/* Now stick the new stuff on the end of the file */

	for  (cnt = 1;  cnt < argc;  cnt += 2)  {
		char	*fld = argv[cnt];
		char	*val = argv[cnt+1];
		if  (strcmp(val, "-") != 0)
			fprintf(xtfile, "%s=%s\n", fld, val);
	}

	return  0;
}
