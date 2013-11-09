/* prin_gname.c -- for hashing up group names/ids

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
#include <grp.h>
#include "defaults.h"
#include "incl_unix.h"
#include "incl_ugid.h"

extern  struct  group  *my_getgrent();
extern  void  my_endgrent();

/* Structure used to hash group ids.  */

struct  ghash   {
        struct  ghash   *grph_next, *grpu_next;
        int_ugid_t      grph_gid;
        char    grph_name[1];
};

#define HASHMOD 37

static  int     doneit;
static  struct  ghash   *ghash[HASHMOD];
static  struct  ghash   *gnhash[HASHMOD];

/* Read group file to build up hash table of group ids.
   This is done once only at the start of the program.  */

void  rgrpfile()
{
        struct  group  *ugrp;
        struct  ghash  *hp, **hpp, **hnpp;
        char    *pn;
        unsigned  sum;

        while  ((ugrp = my_getgrent()))  {
                pn = ugrp->gr_name;
                sum = 0;
                while  (*pn)
                        sum += *pn++;

                for  (hpp = &ghash[(ULONG)ugrp->gr_gid % HASHMOD]; (hp = *hpp); hpp = &hp->grph_next)
                        ;

                hnpp = &gnhash[sum % HASHMOD];
                if  ((hp = (struct ghash *) malloc(sizeof(struct ghash) + strlen(ugrp->gr_name))) == (struct ghash *) 0)
                        nomem();
                hp->grph_gid = ugrp->gr_gid;
                strcpy(hp->grph_name, ugrp->gr_name);
                hp->grph_next = *hpp;
                hp->grpu_next = *hnpp;
                *hpp = hp;
                *hnpp = hp;
        }
        my_endgrent();
        doneit = 1;
}

/* Given a group id, return a group name.  */

char *prin_gname(const gid_t gid)
{
        struct  ghash  *hp;

        if  (!doneit)
                rgrpfile();
        hp = ghash[(ULONG) gid % HASHMOD];

        while  (hp)  {
                if  (gid == hp->grph_gid)
                        return  hp->grph_name;
                hp = hp->grph_next;
        }
        return  "???";
}

/* Do the opposite (long because Amdahl uids are unsigned) */

int_ugid_t  lookup_gname(const char *name)
{
        const   char    *cp;
        unsigned  sum = 0;
        struct  ghash  *hp;

        if  (!doneit)
                rgrpfile();
        cp = name;
        while  (*cp)
                sum += *cp++;
        hp = gnhash[sum % HASHMOD];
        while  (hp)  {
                if  (strcmp(name, hp->grph_name) == 0)
                        return  hp->grph_gid;
                hp = hp->grpu_next;
        }
        return  -1;
}
