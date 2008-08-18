/* sizebodge.c -- bodges for versions of Xwin managers which can't get sizes right

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
#include <fcntl.h>
#include <errno.h>
#undef	CONST			/* Because CONST is redefined by some peoples' Intrinsic.h */
#include <Xm/DialogS.h>
#include "config.h"		/* Because CONST is undefed by some people's Intrinsic.h */
#include "files.h"
#include "errnums.h"
#include "unixdefs.h"

static	FILE	*bodgefile;

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

void	bodge_open(void)
{
	char	*hm;
	char	Path[PATH_MAX];

	if  ((bodgefile = fopen(".xisizes", "r")))
		return;
	if  (!(hm = getenv("HOME")))
		return;
	(void) sprintf(Path, "%s/.xisizes", hm);
	if  ((bodgefile = fopen(Path, "r")))
		return;
	hm = envprocess(CFILEDIR);
	(void) sprintf(Path, "%sxisizes", hm);
	free(hm);
	bodgefile = fopen(Path, "r");
}

void	bodge_size(Widget w, CONST char *dlgname)
{
	int	ch, hadeof = 0;
	Dimension	width, height;
	CONST	char	*cp;

	if  (!bodgefile)
		return;

	for  (;;)  {
		ch = getc(bodgefile);
		if  (ch == EOF)  {
			if  (hadeof)
				return;
			(void) fseek(bodgefile, 0L, 0);
			hadeof++;
			continue;
		}

		/*
		 *	Expect programname:dlgname: width , height
		 */

		cp = progname;
		if  (tolower(ch) != tolower(*cp))  {
		skiprest:
			while  (ch != '\n'  &&  ch != EOF)
				ch = getc(bodgefile);
			continue;
		}
		cp++;
		while  (*cp)  {
			ch = getc(bodgefile);
			if  (tolower(ch) != tolower(*cp))
				goto  skiprest;
			cp++;
		}
		ch = getc(bodgefile);
		if  (ch != ':')
			goto  skiprest;

		/*
		 *	Read pgm name, get dlg name
		 */

		cp = dlgname;
		do  {
			ch = getc(bodgefile);
			if  (ch != *cp)
				goto  skiprest;
			cp++;
		}  while  (*cp);
		ch = getc(bodgefile);
		if  (ch != ':')
			goto  skiprest;

		/*
		 *	Skip over any space and read width
		 */

		do  ch = getc(bodgefile);
		while  (isspace(ch));
		width = 0;
		while  (isdigit(ch))  {
			width = width * 10 + ch - '0';
			ch = getc(bodgefile);
		}
		while  (isspace(ch))
			ch = getc(bodgefile);

		/*
		 *	Read , and repeat for height, reject trailing garbage.
		 */

		if  (ch != ',')
			goto  skiprest;
		do  ch = getc(bodgefile);
		while  (isspace(ch));
		height = 0;
		while  (isdigit(ch))  {
			height = height * 10 + ch - '0';
			ch = getc(bodgefile);
		}
		while  (ch == ' '  ||  ch == '\t')
			ch = getc(bodgefile);
		if  (ch != '\n')
			goto  skiprest;
		XtVaSetValues(w, XmNwidth, width, XmNheight, height, NULL);
	}
}
