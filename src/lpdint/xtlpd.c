/* xtlpd.c -- emulate lpd server

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "defaults.h"
#include "files.h"
#include "incl_net.h"
#include "incl_unix.h"
#include "lpdtypes.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"

short   qsock;                  /* TCP Socket for accepting queued jobs on */

int     debug_level = 0;

extern  uid_t   Realuid, Effuid, Daemuid;

extern int  parsecf(FILE *);
extern void  lassign(struct varname *, const char *);
extern void  process(const int);

/* Set up network stuff */

static  int     init_network()
{
        struct  varname *vp;
        short   qportnum;
        struct  sockaddr_in     sin;
#ifdef  SO_REUSEADDR
        int     on = 1;
#endif

        vp = lookuphash(PORTNAME);
        if  (!vp->vn_value)  {
                fprintf(stderr, "No port name/number defined in control file - aborting\n");
                return  0;
        }

        if  (isdigit(vp->vn_value[0]))
                qportnum = htons(atoi(vp->vn_value));
        else  {
                struct  servent *sp;
                if  (!(sp = getservbyname(vp->vn_value, "tcp")))  {
                        fprintf(stderr, "Cannot find port %s - aborting\n", vp->vn_value);
                        return  0;
                }
                qportnum = sp->s_port;
                endservent();
        }

        BLOCK_ZERO(&sin, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = qportnum;
        sin.sin_addr.s_addr = INADDR_ANY;

        if  ((qsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)  {
                fprintf(stderr, "Failed to open tcp socket\n");
                return  0;
        }
#ifdef  SO_REUSEADDR
        setsockopt(qsock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
        if  (bind(qsock, (struct sockaddr *) &sin, sizeof(sin)) < 0  ||  listen(qsock, 5) < 0)  {
                close(qsock);
                fprintf(stderr, "Failed to bind tcp port\n");
                return  0;
        }
        return  1;
}

static int  tcp_accept(const int msock, netid_t *whofrom)
{
        int     sock;
        SOCKLEN_T       sinl;
        struct  sockaddr_in  sin;

        sinl = sizeof(sin);
        if  ((sock = accept(msock, (struct sockaddr *) &sin, &sinl)) < 0)
                return  -1;
        *whofrom = sin.sin_addr.s_addr;
        return  sock;
}

MAINFN_TYPE  main(int argc, char **argv)
{
        FILE    *wotfile;
        int     newsock;
        netid_t nid;
        struct  varname *vp, *vhost;
        struct  hostent *hp;

        versionprint(argv, "$Revision: 1.9 $", 1);

        Realuid = getuid();
        setuid(ROOTID); /* Should be set-uid root, need this for su commands in scripts */

        if  (argc != 2)  {
                fprintf(stderr, "Usage: %s ctrlfile\n", argv[0]);
                return  1;
        }

        if  (!(wotfile = fopen(argv[1], "r")))  {
                fprintf(stderr, "%s: cannot open %s\n", argv[0], argv[1]);
                return  2;
        }

        if  (parsecf(wotfile))  {
                fprintf(stderr, "Aborting due to control file errors\n");
                return  3;
        }
        fclose(wotfile);

        /* Set up system variables */

        vp = lookuphash(SPOOLDIR);
        if  (!vp->vn_value)  {
                fprintf(stderr, "No spool directory defined in control file\n");
                return  4;
        }
        if  (chdir(vp->vn_value) < 0)  {
                fprintf(stderr, "Cannot open spool directory %s\n", vp->vn_value);
                return  5;
        }

        if  (!init_network())
                return  7;

        /* If logfile exists, switch stderr to it.  */

        vp = lookuphash(LOGFILE);
        if  (vp->vn_value)  {
                int     fd = open(vp->vn_value, O_CREAT|O_WRONLY|O_APPEND, 0644);
                if  (fd < 0)  {
                        fprintf(stderr, "Unable to create logfile %s\n", vp->vn_value);
                        return  6;
                }
                fflush(stderr);
                close(2);
                fcntl(fd, F_DUPFD, 2);
                close(fd);
        }

        if  (fork() != 0)
                return  0;
#ifdef  SETPGRP_VOID
        setpgrp();
#else
        setpgrp(0, getpid());
#endif
        vp = lookuphash(DEBUG_VAR);
        if  (vp->vn_value)
                debug_level = atoi(vp->vn_value);

        vhost = lookuphash(HOSTNAME);

        for  (;;)  {
                struct  in_addr naddr;
                newsock = tcp_accept(qsock, &nid);
                if  (newsock < 0)  {
                        fprintf(stderr, "TCP accept failure\n");
                        exit(200);
                }
                hp = gethostbyaddr((char *) &nid, sizeof(nid), AF_INET);
                naddr.s_addr = nid;
                lassign(vhost, hp? hp->h_name: inet_ntoa(naddr));
                if  (debug_level > 0)
                        fprintf(stderr, "Connection from %s\n", vhost->vn_value);
                if  (fork() != 0)  {
                        wait((int *) 0);
                        close(newsock);
                        continue;
                }
                if  (fork() != 0)
                        exit(0);
                close(qsock);
                process(newsock);
                exit(0);
        }

        return  0;
}
