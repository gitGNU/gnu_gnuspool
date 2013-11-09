/* getsnmp.c -- get an SNMP variable

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
#include <math.h>
#include "defaults.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "asn.h"
#include "snmp.h"
#include "xtlhpdefs.h"
#include "files.h"

int     debug = 0;
double  udptimeout = 1.0;
int     snmpsock;
struct  sockaddr_in  snmp_serv, snmp_cli;

extern void  snmp_xmit(char *, int);
extern unsigned  snmp_recv(char *, int);
extern int  snmp_wait();
extern int  ParseMacroFile(const char *);
extern void  init_define(const char *, const char *);
extern char *expand(const char *);

netid_t  get_hostorip(const char *name)
{
        netid_t hid;

        if  (isdigit(name[0]))  {
#ifdef  DGAVIION
                struct  in_addr  ina_str;
                ina_str = inet_addr(name);
                hid = ina_str.s_addr;
#else
                hid = inet_addr(name);
#endif
                if  (hid == -1L)  {
                        fprintf(stderr, "Invalid internet address %s\n", name);
                        exit(EXIT_USAGE);
                }
        }
        else  {
                struct  hostent  *host;
                if  (!(host = gethostbyname(name)))  {
                        fprintf(stderr, "unknown host %s\n", name);
                        exit(EXIT_USAGE);
                }
                hid = * (netid_t *) host->h_addr;
        }
        return  hid;
}

int  get_portornum(const char *name)
{
        struct  servent  *sp;

        if  (isdigit(name[0]))
                return  htons(atoi(name));

        if  (!(sp = getservbyname(name, "udp")))  {
                fprintf(stderr, "Cannot find snmp service %s\n", name);
                exit(EXIT_SNMPERROR);
        }
        return  sp->s_port;
}

void  init_snmp_socket(const netid_t hostid, const netid_t myhostid, int portnum)
{
        snmp_cli.sin_addr.s_addr = myhostid? myhostid: htonl(INADDR_ANY);
        snmp_serv.sin_addr.s_addr = hostid;
        snmp_serv.sin_family = snmp_cli.sin_family = AF_INET;
        snmp_serv.sin_port = portnum;

        if  ((snmpsock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)  {
                fprintf(stderr, "Cannot create socket\n");
                exit(EXIT_SYSERROR);
        }

        if  (bind(snmpsock, (struct sockaddr *) &snmp_cli, sizeof(snmp_cli)) < 0)  {
                perror("Bind fail");
                exit(EXIT_SYSERROR);
        }
}

MAINFN_TYPE  main(int argc, char **argv)
{
        int     c, res;
        unsigned        outlen, inlen, gnext = 0, prinid = 0;
        char    *hostname = (char *) 0, *myhostname = (char *) 0, *snmp_port = "snmp", *progname;
        char    *configname = (char *) 0;
        char    *variable, *expanded;
        char    *community = "public";
        netid_t hostid = 0, myhostid = 0;
        struct  snmp_result     last_result;
        asn_octet       *coded;
        asn_octet       inbuf[200];
        extern  char    *optarg;
        extern  int     optind;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();

        while  ((c = getopt(argc, argv, "h:H:p:T:C:d:f:NI")) != EOF)  {
                switch  (c)  {
                default:
                        fprintf(stderr, "Invalid option %c to %s\n", c, progname);
                        return  EXIT_USAGE;
                case  'h':      hostname = optarg;      continue;
                case  'H':      myhostname = optarg;    continue;
                case  'p':      snmp_port = optarg;     continue;
                case  'C':      community = optarg;     continue;
                case  'd':      debug = atoi(optarg);   continue;
                case  'T':      udptimeout = atof(optarg);      continue;
                case  'f':      configname = optarg;    continue;
                case  'N':      gnext = 1;              continue;
                case  'I':      prinid = 1;             continue;
                }
        }

        variable = argv[optind];

        if  (!variable)  {
                fprintf(stderr, "Usage: %s [options] variable\n", progname);
                return  EXIT_USAGE;
        }

        if  (!hostname)  {
                fprintf(stderr, "%s: No host name supplied\n", progname);
                return  EXIT_USAGE;
        }

        init_define(HOSTNAME_NAME, hostname);
        init_define(PORTNAME_NAME, snmp_port);

        /* Get config file, not fatal error if we can't find it.  */

        if  (configname)  {
                if  (!ParseMacroFile(configname)  &&  debug > 0)
                        fprintf(stderr, "Cannot open config file %s\n", configname);
        }
        else  {
                char    *progdir = envprocess(IDATADIR);
                char    *defcname = DEFAULT_CONFIGNAME;
                char    *filep = malloc((unsigned) (strlen(progdir) + strlen(defcname) + 2));
                if  (!filep)
                        nomem();
                sprintf(filep, "%s/%s", progdir, defcname);
                if  (!ParseMacroFile(filep)  &&  debug > 0)
                        fprintf(stderr, "Cannot open default config file %s\n", filep);
                free(progdir);
                free(filep);
        }

        hostid = get_hostorip(hostname);
        if  (myhostname)
                myhostid = get_hostorip(myhostname);

        init_snmp_socket(hostid, myhostid, get_portornum(snmp_port));
        expanded = expand(variable);
        coded = gen_snmp_get(community, expanded, &outlen, gnext);
        free(expanded);
        if  (debug > 3)  {
                fprintf(stderr, "Outgoing SNMP request...\n");
                prinbuf(coded, outlen);
        }
        snmp_xmit((char *) coded, (int) outlen);
        if  (outlen != 0)
                free((char *) coded);

        if  (!snmp_wait())  {
                if  (debug > 1)
                        fprintf(stderr, "SNMP timeout\n");
                return  EXIT_OFFLINE;
        }

        inlen = snmp_recv((char *) inbuf, sizeof(inbuf));
        if  (debug > 3)  {
                fprintf(stderr, "Incoming SNMP request...\n");
                prinbuf(inbuf, inlen);
        }

        if  ((res = snmp_parse_result(inbuf, inlen, &last_result)) != RES_OK)  {
                if  (res == RES_UNDEF)  {
                        if  (debug > 0)
                                fprintf(stderr, "(Null)\n");
                        return  EXIT_NULL;
                }
                if  (debug > 1)
                        fprintf(stderr, "Parse problem SNMP request\n");
                return  EXIT_SNMPERROR;
        }

        if  (prinid && last_result.res_id_string)
                printf("%s=", last_result.res_id_string);

        switch  (last_result.res_type)  {
        case  RES_TYPE_STRING:
                printf("%s\n", last_result.res_un.res_string);
                break;
        case  RES_TYPE_SIGNED:
                printf("%ld\n", last_result.res_un.res_signed);
                break;
        case  RES_TYPE_UNSIGNED:
                printf("%lu\n", last_result.res_un.res_unsigned);
                break;
        }

        return  0;
}
