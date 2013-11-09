/* sendudp.c -- send out job using UDP

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#include <ctype.h>
#include "defaults.h"
#include "files.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "incl_unix.h"
#include "serv_if.h"

static  int     had_alarm,
                verbose;

#define EXIT_OFFLINE    1
#define EXIT_DEVERROR   2
#define EXIT_USAGE      3
#define EXIT_SYSERROR   4

static  int sleep_time = 10;

static  struct  sockaddr_in     serv_addr,      /* That's me */
                                cli_addr;       /* That's him */

static  char    *clihost;

static RETSIGTYPE  catchalarm(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(SIGALRM, catchalarm);
#endif
        had_alarm++;
}

static void  xmit(int sock, char *buff, int nbytes)
{
        char    rbuf[1];
        SOCKLEN_T               repl = sizeof(struct sockaddr_in);
        struct  sockaddr_in     reply_addr;

        if  (sendto(sock, buff, nbytes, 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) != nbytes)  {
                fprintf(stderr, "Send failure host %s\n", clihost);
                exit(EXIT_SYSERROR);
        }
        if  (verbose)
                fprintf(stderr, "Sent %d bytes to %s", nbytes, clihost);
        alarm(sleep_time);
        if  (recvfrom(sock, rbuf, sizeof(rbuf), 0, (struct sockaddr *) &reply_addr, &repl) != sizeof(rbuf))  {
                if  (verbose)
                        putc('\n', stderr);

                fprintf(stderr, "Receive failure host %s\n", clihost);
                exit(EXIT_SYSERROR);
        }
        alarm(0);

        switch  (rbuf[0])  {
        case  CL_SV_OK:
                if  (verbose)
                        fprintf(stderr, "\tReceived OK\n");
                return;
        case  CL_SV_OFFLINE:
                if  (verbose)
                        fprintf(stderr, "\tDevice Offline\n");
                exit(EXIT_OFFLINE);
        case  CL_SV_ERROR:
                if  (verbose)
                        fprintf(stderr, "\tDevice Error\n");
                exit(EXIT_DEVERROR);
        default:
                if  (verbose)
                        fprintf(stderr, "\tSystem error\n");
                exit(EXIT_SYSERROR);
        }
}

MAINFN_TYPE  main(int argc, char **argv)
{
        int     ch, nbytes, sock;
        char    *progname = argv[0];
        LONG    hostid;
        char    buff[SV_CL_BUFFSIZE];
        extern  char *optarg;
        extern  int optind;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zca;
        zca.sighandler_el = catchalarm;
        sigmask_clear(zca);
        zca.sigflags_el = SIGVEC_INTFLAG;
#endif

        versionprint(argv, "$Revision: 1.9 $", 1);

        while  ((ch = getopt(argc, argv, "vs:")) != EOF)
                switch(ch)      {
                case 'v':
                        verbose++;
                        break;
                case 's':
                        sleep_time = atoi(optarg);
                        break;
                }

        argc -= optind;
        argv += optind;

        if  (argc != 2)  {
                fprintf(stderr, "Usage: %s host port\n", progname);
                exit(EXIT_USAGE);
        }

        clihost = argv[0];
#ifdef  STRUCT_SIG
        sigact_routine(SIGALRM, &zca, (struct sigstruct_name *) 0);
#else
        signal(SIGALRM, catchalarm);
#endif

        /* Decipher host name in 123.45.67.89 format or as host name */

        if  (isdigit(clihost[0]))  {
#ifdef  DGAVIION
                struct  in_addr  ina_str;
                ina_str = inet_addr(clihost);
                hostid = ina_str.s_addr;
#else
                hostid = inet_addr(clihost);
#endif
                if  (hostid == -1L)  {
                        fprintf(stderr, "Invalid internet address %s\n", clihost);
                        return  EXIT_USAGE;
                }
        }
        else  {
                struct  hostent *hp;
                if  (!(hp = gethostbyname(clihost)))  {
                        fprintf(stderr, "Unknown host %s\n", clihost);
                        exit(EXIT_USAGE);
                }
                hostid = * (LONG *) hp->h_addr;
        }

        BLOCK_ZERO(&serv_addr, sizeof(serv_addr));
        BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        cli_addr.sin_addr.s_addr = hostid;
        serv_addr.sin_family = cli_addr.sin_family = AF_INET;

        /* Decipher port number/name and plug in */

        if  (isdigit(argv[1][0]))
                cli_addr.sin_port = htons((USHORT) atoi(argv[1]));
        else  {
                struct  servent *sp;
                if  (!(sp = getservbyname(argv[1], "udp")))  {
                        fprintf(stderr, "Unknown service %s\n", argv[1]);
                        exit(EXIT_USAGE);
                }
                cli_addr.sin_port = sp->s_port;
        }

        if  ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)  {
                fprintf(stderr, "Cannot create socket\n");
                exit(EXIT_SYSERROR);
        }

        if  (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  {
                perror("bind");
                fprintf(stderr, "Bind fail\n");
                exit(EXIT_SYSERROR);
        }

        buff[0] = (char) SV_CL_DATA;
        nbytes = 1;
        while  ((ch = getchar()) != EOF)  {
                buff[nbytes++] = ch;
                if  (nbytes >= SV_CL_BUFFSIZE)  {
                        xmit(sock, buff, nbytes);
                        nbytes = 1;
                }
        }
        if  (nbytes > 1)
                xmit(sock, buff, nbytes);
        close(sock);
        return  0;
}

