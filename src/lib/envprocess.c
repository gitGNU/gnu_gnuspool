/* envprocess.c -- expand strings with environment vars in

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
#include <stdio.h>
#include <ctype.h>
#include "defaults.h"
#include "files.h"
#include "incl_unix.h"
#include "errnums.h"

#ifndef MALLINC
#define MALLINC 64
#endif

extern	char	**environ;

/* Getenv where we are looking for a non-null-terminated string */

static char *sgetenv(const char *sstart, const char *send)
{
	char	**envp;
	int	l = send - sstart;

	for  (envp = environ;  *envp;  envp++)
		if  (strncmp(*envp, sstart, l) == 0  &&  (*envp)[l] == '=')
			return  &(*envp)[l+1];
	return  (char *) 0;
}

/* Initialise environment variables not otherwise defined from Master Config file.  */

void  init_mcfile()
{
	FILE	*inf;
	char	*inl, *cp, *ep, **newenv, **envp, **nep;
	static	char	alloc_env = 0;

	if  ((inf = fopen(MASTER_CONFIG, "r")) == (FILE *) 0)
		return;

 cont:
	while  ((inl = strread(inf, "\n")))  {
		for  (cp = inl;  isspace(*cp);  cp++)
			;
		if  (*cp == '#' || (ep = strchr(cp, '=')) == (char *) 0)  {
			free(inl);
			continue;
		}

		/* Look in environment.  Make sure we include the
		   = otherwise we'll get prefixes caught */

		for  (envp = environ;  *envp;  envp++)
			if  (strncmp(*envp, cp, (ep - cp) + 1) == 0)  {
				free(inl);
				goto  cont;
			}

		/* Add to environment, adding 2 for null and new entry.  */

		if  ((newenv = (char **) malloc((unsigned) ((envp - environ) + 2) * sizeof(char *))) == (char **) 0)
			nomem();
		nep = newenv;
		envp = environ;

		/* Copy existing entries */

		while  (*envp)
			*nep++ = *envp++;

		/* Copy in new, stick null on end */

		*nep++ = stracpy(cp);
		*nep = (char *) 0;

		/* If allocated before free it.  Assign new environment */

		if  (alloc_env)
			free((char *) environ);
		environ = newenv;
		alloc_env++;
		free(inl);
	}

	fclose(inf);
}

char	*envprocess(const char *inp_string)
{
	const	char	*inp;
	char	*outp;
	int	i;
	const	char	*es, *ee, *rs;
	char	*result, *val;
	int	outlen;

	inp = inp_string;
	outlen = strlen(inp_string) + MALLINC;
	if  ((outp = result = (char *) malloc((unsigned) outlen)) == (char *) 0)
		nomem();
	while  (*inp != '\0')  {
		if  (*inp == '$')  {

			/* Environment variable.  */

			if  (*++inp == '$')
				goto  is_ch;

			es = inp;
			rs = (const char *) 0;

			if  (*inp == '{')  {
				inp++;
				es++;
				while  (*inp != '-' && *inp != '}' && *inp != '\0')
					inp++;
				ee = inp;
				if  (*inp == '-')  {
					rs = ++inp;
					while (*inp != '}' && *inp != '\0')
						inp++;
				}
			}
			else  {
				while (isalnum(*inp) || *inp == '_')
					inp++;
				ee = inp;
			}
			if  (ee <= es)  {	/* Invalid var */
				free((char *) result);
				return  (char *) 0;
			}

			/* If no such var, ignore or insert replacement */

			if  ((val = sgetenv(es, ee)))  {
				int	lng = strlen(val);

				i = outp - result;
				if  (i + lng >= outlen) {
					do  outlen += MALLINC;
					while  (outlen <= i + lng);
					if  ((result = (char *) realloc(result, (unsigned) outlen)) == (char *) 0)
						nomem();
					outp = result + i;
				}
				strcpy(outp, val);
				outp += lng;
			}
			else  if  (rs)  {
				int	lng = inp - rs;
				i = outp - result;
				if  (lng > 0)  {
					if  (i + lng >= outlen)  {
						do  outlen += MALLINC;
						while  (outlen <= i + lng);
						if  ((result = (char *) realloc(result, (unsigned) outlen)) == (char *) 0)
							nomem();
						outp = result + i;
					}
					strncpy(outp, rs, lng);
					outp += lng;
				}
			}
			else  if  (ee == es + 1  &&  *es == '0')  {
				int	lng = strlen(progname);
				i = outp - result;
				if  (i + lng >= outlen) {
					do  outlen += MALLINC;
					while  (outlen <= i + lng);
					if  ((result = (char *) realloc(result, (unsigned) outlen)) == (char *) 0)
						nomem();
					outp = result + i;
				}
				strcpy(outp, progname);
				outp += lng;
			}

			if  (*inp == '}')
				inp++;
			continue;
		}
is_ch:
		*outp++ = *inp++;
		i = outp - result;
		if  (i >= outlen)  {
			do  outlen += MALLINC;
			while  (i >= outlen);
			if  ((result = (char *) realloc(result, (unsigned) outlen)) == (char *) 0)
				nomem();
			outp = result + i;
		}
	}

	*outp = '\0';
	return  result;
}

/* Get an absolute path name to the given file in the batch spool directory */

char	*mkspdirfile(const char *bfile)
{
	char	*spdir = envprocess(SPDIR);
	char	*res = malloc((unsigned) (strlen(spdir) + strlen(bfile) + 2));
	if  (!res)
		nomem();
	sprintf(res, "%s/%s", spdir, bfile);
	free(spdir);
	return  res;
}
