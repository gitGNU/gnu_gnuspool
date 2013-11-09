/* spcharge.c -- charges program

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
#include "defaults.h"
#include "files.h"
#include "ecodes.h"
#include "errnums.h"
#include "helpargs.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"

/* This is needed by the standard error handling stuff in the library */

void    nomem()
{
        print_error($E{NO MEMORY});
        exit(E_NOMEM);
}

MAINFN_TYPE  main(int argc, char **argv)
{
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        versionprint(argv, "$Revision: 1.9 $", 0);

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
        print_error($E{spcharge options});
        return E_USAGE;
}
