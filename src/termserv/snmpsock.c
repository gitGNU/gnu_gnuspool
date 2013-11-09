/* snmpsock.c -- socket handling for SNMP

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
#ifdef  OS_FREEBSD
#include <sys/time.h>
#endif
#include <math.h>
#include "xtlhpdefs.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"

extern  int     snmpsock;
extern  double  udptimeout;
extern  struct  sockaddr_in  snmp_serv, snmp_cli;

void  snmp_xmit(char *buff, int nbytes)
{
        if  (sendto(snmpsock, buff, nbytes, 0, (struct sockaddr *) &snmp_serv, sizeof(snmp_serv)) != nbytes)  {
                perror("SNMP Send fail");
                exit(EXIT_SYSERROR);
        }
}

unsigned  snmp_recv(char *buff, int nbytes)
{
        int                     nbs;
        SOCKLEN_T               repl = sizeof(struct sockaddr_in);
        struct  sockaddr_in     reply_addr;

        if  ((nbs = recvfrom(snmpsock, buff, nbytes, 0, (struct sockaddr *) &reply_addr, &repl)) < 0)  {
                perror("SNMP Receive fail");
                exit(EXIT_SYSERROR);
        }
        return  nbs;
}

int  snmp_wait()
{
        fd_set  sel;
        struct  timeval  tv;
        double  ipart, fpart;
        FD_ZERO(&sel);
        FD_SET(snmpsock, &sel);
        fpart = modf(udptimeout, &ipart);
        tv.tv_sec = (long) ipart;
        tv.tv_usec = (long) (fpart * 1000000.0);
        return  select(snmpsock+1, &sel, 0, 0, &tv) > 0;
}
