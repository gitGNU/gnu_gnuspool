/* extdefs.c -- external message handling routines

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

/* This was written together with xtlpd but never fully used.
   Maybe it should go. */


#include "config.h"
#include "defaults.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "files.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "extdefs.h"

#define EXTHASHMOD      17

/* Structure used to hash host ids and aliases.  */

struct  extdef  {
        struct  extdef  *hh_next,       /* Hash by name chain */
                        *hn_next;       /* Hash by number chain */
        char    *name;
        char    *mailprog;
        char    *wrtprog;
        USHORT  extnum;
};

static  char    done_file = 0;
static  struct  extdef  *hhashtab[EXTHASHMOD], *nhashtab[EXTHASHMOD];

static unsigned  calcnhash(const int num)
{
        int     i;
        unsigned  result = 0, orig = (USHORT) num;

        for  (i = 0;  i < 16;  i += 4)
                result ^= orig >> i;

        return  result % EXTHASHMOD;
}

static  unsigned  calchhash(const char *hostid)
{
        unsigned  result = 0;
        while  (*hostid)
                result = (result << 1) ^ *hostid++;
        return  result % EXTHASHMOD;
}

static void  procfile()
{
        FILE    *fp;
        int     num, fsize, ch;
        unsigned        hashval;
        struct  extdef  *ep;
        char    extname[40], mailname[80], wrtname[80];

        done_file = 1;
        if  (!(fp = fopen(EXTERNSPOOL, "r")))
                return;
        for  (;;)  {
                ch = getc(fp);
                if  (ch == EOF)
                        break;

                /* Read in leading number */

                if  (!isdigit(ch))  {
                skipn:
                        while  (ch != '\n'  &&  ch != EOF)
                                ch = getc(fp);
                        continue;
                }
                num = ch - '0';
                for  (;;)  {
                        ch = getc(fp);
                        if  (!isdigit(ch))
                                break;
                        num = num * 10 + ch - '0';
                }

                /* Insist number in range.  */

                if  (num <= 0  ||  num > 65535  ||  (ch != ' '  &&  ch != '\t'))
                        goto  skipn;

                do  ch = getc(fp);
                while  (ch == ' ' ||  ch == '\t');

                if  (!isgraph(ch))
                        goto  skipn;

                /* Read in file name */

                fsize = 0;
                do  {
                        if  (fsize < sizeof(extname)-1)
                                extname[fsize++] = ch;
                        ch = getc(fp);
                }  while  (isgraph(ch));
                extname[fsize] = '\0';

                while  (ch == ' ' || ch == '\t')
                        ch = getc(fp);

                mailname[0] = wrtname[0] = '\0';
                if  (isgraph(ch))  {
                        fsize = 0;
                        do  {
                                if  (fsize < sizeof(mailname)-1)
                                        mailname[fsize++] = ch;
                                ch = getc(fp);
                        }  while  (isgraph(ch));
                        mailname[fsize] = '\0';
                        if  (fsize == 1  &&  mailname[0] == '-')
                                mailname[0] = '\0';

                        while  (ch == ' ' || ch == '\t')
                                ch = getc(fp);

                        if  (isgraph(ch))  {
                                fsize = 0;
                                do  {
                                        if  (fsize < sizeof(wrtname)-1)
                                                wrtname[fsize++] = ch;
                                        ch = getc(fp);
                                }  while  (isgraph(ch));
                                wrtname[fsize] = '\0';
                                if  (fsize == 1  &&  wrtname[0] == '-')
                                        wrtname[0] = '\0';
                        }
                }

                if  (!(ep = (struct extdef *) malloc(sizeof(struct extdef))))
                        nomem();

                ep->name = stracpy(extname);
                ep->mailprog = mailname[0]? stracpy(mailname): (char *) 0;
                ep->wrtprog = wrtname[0]? stracpy(wrtname): (char *) 0;
                ep->extnum = num;

                hashval = calchhash(extname);
                ep->hh_next = hhashtab[hashval];
                hhashtab[hashval] = ep;
                hashval = calcnhash(num);
                ep->hn_next = nhashtab[hashval];
                nhashtab[hashval] = ep;
        }

        fclose(fp);
}

int  ext_nametonum(const char *name)
{
        struct  extdef  *ep;

        if  (!done_file)
                procfile();
        for  (ep = hhashtab[calchhash(name)];  ep;  ep = ep->hh_next)
                if  (strcmp(name, ep->name) == 0)
                        return  (int) ep->extnum;
        return  -1;
}

static struct extdef *lookup_extnum(const int num)
{
        struct  extdef  *ep;

        if  (!done_file)
                procfile();
        for  (ep = nhashtab[calcnhash(num)];  ep;  ep = ep->hn_next)
                if  ((int) ep->extnum == num)
                        return  ep;
        return  (struct extdef *) 0;
}

char    *ext_numtoname(const int num)
{
        struct  extdef  *ep;

        if  (num < 0)
                return  (char *) 0;
        ep = lookup_extnum(num);
        return  ep? ep->name: (char *) 0;
}

char    *ext_mail(const int num)
{
        struct  extdef  *ep = lookup_extnum(num);
        return  ep? ep->mailprog: (char *) 0;
}

char    *ext_wrt(const int num)
{
        struct  extdef  *ep = lookup_extnum(num);
        return  ep? ep->wrtprog: (char *) 0;
}
