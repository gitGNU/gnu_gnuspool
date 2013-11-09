/* xtelnet.c -- reverse telnet printer dreiver

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
#include "incl_net.h"
#include "incl_unix.h"
#include "defaults.h"
#include "files.h"

#define EXIT_OFFLINE    1
#define EXIT_DEVERROR   2
#define EXIT_USAGE      3
#define EXIT_SYSERROR   4

MAINFN_TYPE  main(int argc, char **argv)
{
        int     c, inp, portnum, sfd, debug = 0, loops = 3;
        unsigned        loopwait = 1, endsleep = 0;
        float           lingertime = 0;
        char    *hostname = (char *) 0, *port_text = (char *) 0, *progname;
        LONG    hostid;
        struct  sockaddr_in     sin;
        extern  char    *optarg;
        char    buffer[256];

        versionprint(argv, "$Revision: 1.9 $", 1);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        while  ((c = getopt(argc, argv, "h:H:p:P:dDl:L:t:s:")) != EOF)  {
                switch  (c)  {
                default:
                        fprintf(stderr, "Invalid option to %s\n", progname);
                        return  EXIT_USAGE;
                case  'h':
                case  'H':
                        hostname = optarg;
                        continue;
                case  'p':
                case  'P':
                        port_text = optarg;
                        continue;
                case  'd':
                case  'D':
                        debug = 1;
                        continue;
                case  'l':
                        loops = atoi(optarg);
                        continue;
                case  'L':
                        loopwait = (unsigned) atoi(optarg);
                        continue;
                case  't':
                        endsleep = (unsigned) atoi(optarg);
                        continue;
                case  's':
                        lingertime = atof(optarg);
                        continue;
                }
        }
        if  (!hostname)  {
                fprintf(stderr, "%s: No host name supplied\n", progname);
                return  EXIT_USAGE;
        }
        if  (!port_text)  {
                fprintf(stderr, "%s: No port number supplied\n", progname);
                return  EXIT_USAGE;
        }

        /* Decipher host name in 123.45.67.89 format or as host name */

        if  (isdigit(hostname[0]))  {
#ifdef  DGAVIION
                struct  in_addr  ina_str;
                ina_str = inet_addr(hostname);
                hostid = ina_str.s_addr;
#else
                hostid = inet_addr(hostname);
#endif
                if  (hostid == -1L)  {
                        fprintf(stderr, "%s: Invalid internet address %s\n", progname, hostname);
                        return  EXIT_USAGE;
                }
        }
        else  {
                struct  hostent  *host;
                if  (!(host = gethostbyname(hostname)))  {
                        fprintf(stderr, "%s: unknown host %s\n", progname, hostname);
                        return  EXIT_SYSERROR;
                }
                hostid = * (LONG *) host->h_addr;
        }

        /* The same for port names/numbers */

        if  (isdigit(port_text[0]))
             portnum = htons((SHORT) atoi(port_text));
        else  {
                struct  servent *sp;
                if  (!(sp = getservbyname(port_text, "tcp"))  &&  !(sp = getservbyname(port_text, "TCP")))  {
                        fprintf(stderr, "%s: Unknown service %s\n", progname, port_text);
                        return  EXIT_USAGE;
                }
                portnum = sp->s_port;
        }

        sin.sin_family = AF_INET;
        sin.sin_port = (SHORT)portnum;
        BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
        sin.sin_addr.s_addr = hostid;

        sfd = socket(AF_INET, SOCK_STREAM, 0);
        if  (sfd < 0)   {
                fprintf(stderr, "%s: Cannot create socket\n", progname);
                return  EXIT_SYSERROR;
        }
        do  {
                if  (connect(sfd, (struct sockaddr *) &sin, sizeof(sin)) >= 0)
                        goto  doneconn;
                sleep(loopwait);
        }  while  (--loops > 0);
        fprintf(stderr, "%s: Cannot connect\n", progname);
        return EXIT_DEVERROR;
doneconn:
        if  (lingertime != 0.0)  {
                struct  linger  soko;
                soko.l_linger = (int) (lingertime * 100.0 + .5);
                soko.l_onoff = 1;
                setsockopt(sfd, SOL_SOCKET, SO_LINGER, &soko, sizeof(soko));
        }

        /* Do the business....  */

        inp = 0;
        while  ((c = getchar()) != EOF)  {
                buffer[inp++] = (char)c;
                if  (inp >= sizeof(buffer))  {
                        if  (write(sfd, buffer, (unsigned int)inp) != inp)  {
                                fprintf(stderr, "%s: Send failure\n", progname);
                                return  EXIT_DEVERROR;
                        }
                        if  (debug)
                                fprintf(stderr, "%s: Sent %d bytes ok\n", progname, inp);
                        inp = 0;
                }
        }
        if  (inp >= 0)  {
                if  (write(sfd, buffer, (unsigned int)inp) != inp)  {
                        fprintf(stderr, "%s: Send failure\n", progname);
                        return  EXIT_DEVERROR;
                }
                if  (debug)
                        fprintf(stderr, "%s: Sent %d bytes ok\n", progname, inp);
                inp = 0;
        }
        close(sfd);
        if  (endsleep)
                sleep(endsleep);
        return  0;
}
