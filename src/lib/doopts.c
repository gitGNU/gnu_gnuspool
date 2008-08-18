/* doopts.c -- handle options and keywords

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
#include "helpargs.h"
#include "errnums.h"
#include "ecodes.h"
#include "incl_unix.h"

/* Extract and decode options from argv and the helpfile so we can configure the option names.
   This was a fun idea whilst it lasted but no one ever used the full generality.
   I think we will flip it to using getopt etc like other software in the next version */

char  **doopts(char **argv, HelpargRef Adesc, optparam *const optlist, int minstate)
{
	char	*arg;
	int		ad, rc;
	HelpargkeyRef	ap;

 nexta:
	for  (;;)  {

		/* Advance to next arg, stop search if it doesn't start with - or + */

		arg = *++argv;
		if  (!arg || (*arg != '-' && *arg != '+'))
			return	argv;

		if  (*arg == '-')  {

			/* Treat -- as alternative to + to start keywords
			   or -- on its own as end of arguments */

			if  (*++arg == '-')  {
				if  (*++arg)
					goto  keyw_arg;
				return  ++argv;
			}

			/* Past initial '-', argv still on whole argument */

			while  (*arg >= ARG_STARTV)  {
				ad = Adesc[*arg - ARG_STARTV].value;

				if  (ad == 0  ||  ad < minstate)  {
					disp_str = *argv;
					print_error($E{program arg error});
					exit(E_USAGE);
				}

				/* Each function returns OPTRESULT_ARG_OK (1) if it eats the
				   argument or OPTRESULT_OK (0) if it doesn't.
				   It returns OPTRESULT_LAST_ARG_OK (2) if it eats an argument
				   which must be the last argument and OPTRESULT_MISSARG (-1)
				   if an argument is missing. */

				if  (!*++arg)  {	/* No trailing stuff after arg letter */
					if  ((rc = (optlist[ad - minstate])(argv[1])) < OPTRESULT_OK)  {
						disp_str = *argv;
						print_error($E{program opt expects arg});
						exit(E_USAGE);
					}
					if  (rc > OPTRESULT_OK)  {	/* Eaten the next arg */
						argv++;
						if  (rc > OPTRESULT_ARG_OK)	/* Last return the following arg */
							return  ++argv;
					}
					goto  nexta;
				}

				/* Trailing stuff after arg letter, we incremented to it */

				if  ((rc = (optlist[ad - minstate])(arg)) > OPTRESULT_OK)  { /* Eaten */
					if  (rc > OPTRESULT_ARG_OK)		/* Last of its kind */
						return  ++argv;	/* Point to thing following */
					goto  nexta;
				}

			}
			continue;
		}

		arg++;		/* Increment past '+' */

	keyw_arg:

		for  (ap = Adesc[tolower(*arg) - ARG_STARTV].mult_chain;  ap;  ap = ap->next)
			if  (ncstrcmp(arg, ap->chars) == 0)
				goto  found;
		disp_str = arg;
		print_error($E{program arg bad string});
		exit(E_USAGE);

	found:
		if  ((rc = (optlist[ap->value - minstate])(argv[1])) < OPTRESULT_OK)  {
			disp_str = arg;
			print_error($E{program opt expects arg});
			exit(E_USAGE);
		}

		if  (rc > OPTRESULT_OK)  {		/* Eaten */
			argv++;
			if  (rc > OPTRESULT_ARG_OK)	/* The end */
				return  ++argv;
		}
	}
}
