/* files.h -- file names and locations

   Copyright 2009 Free Software Foundation, Inc.

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

#define	XI_SOFTWARE	"FSF Inc"
#define	PRODNAME	"GNUspool"
#define	DEFBASED	"\\GNUSPOOL"

#define	SPUNAME		"gnuspool"
#define	HOSTFILE	"hosts"
#define	INIFILE		"gnuspool.ini"

#define	CONNSERVNAME	"gnuspool"
#define	PROBESERVNAME	"gnuspool"
#define	FEEDSERVNAME	"gnuspool-feeder"
#define	QSERVNAME	"gnuspool-netsrv"
#define	UASERVNAME	"gnuspool-netsrv"
                                
//  Default port numbers
                                
#define	DEF_CONNPORTNUM		48100
#define	DEF_PROBEPORTNUM	48100
#define	DEF_FEEDPORTNUM		48101
#define	DEF_SERVPORTNUM		48102	// Default port number for server
#define	DEF_INPORTNUM		48102	// Default port number for local printers
#define	DEF_TCPAPIPORTNUM	48103	// Default port number for API TCP
#define	DEF_UDPAPIPORTNUM	48103	// Default port number for API UDP
