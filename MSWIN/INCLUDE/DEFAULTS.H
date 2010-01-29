/* defaults.h -- defaults and constants

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

#define	NAMESIZE	14		/* File name size allowing for restricted names */
#define	MAXTITLE	30		/* Size of header */
#define	LINESIZE	29		/* Size of dev field */
#define MAXFLAGS	62		// Size of P/P flags buffer
#define	UIDSIZE		11		/* Size of UID field */
#define	HOSTNSIZE	14		/* Host name size */
#define	PFEEDBACK	39		/* Feedback field on printer */

#define	MAXFORM		34		// Size of form field INCREASED
#define	PTRNAMESIZE	28		// Printer name size
#define	JPTRNAMESIZE	(PTRNAMESIZE*2+2)	// Printer name size in job
#define	COMMENTSIZE	41		// Comment size on printer
#define	ALLOWFORMSIZE	63

//	Timeout intervals and such

#define	QREWRITE	300		// Time to write queues to file
#define	QNPTIMEOUT	(7*24)	// One week if not printed
#define	QPTIMEOUT	24		// One day if printed
#define	NETTICKLE	1000	// Keep networks alive
#define	LICWARN		14		// Days to warn about licence

//      Other things mostly to do with forms

#define	DEF_CHARGE	1000	// I.e. charges divided by 1000
#define	DEF_OBUF	1024	// Output buffer size
#define	DEF_OPEN	30		// Timeout before we give up
#define	DEF_OFFLINE	30		// Timeout for writing

#define	DEF_WIDTH	80		// Page width (for banners)
#define	DEF_RCOUNT	1		// No of delimiters
#define	DEF_DELIM	'\f'	// Default delimiter
#define	DEFAULT		"default"	// Default form type
#define	DEF_SUFCHARS	".-"	// Default suffix chars

//	Timezone - change this as needed (mostly needed for DOS version).

#define DEFAULT_TZ	"TZ=GMT0BST"

//      Display bits for spq

#define DEFAULT_PLINES  3               // Size of printer list 
#define MAX_PLINES      15              // Display maximum 

#define POLLMAX         90              // Maximum polling interval 
#define POLLMIN         2               // Minimum polling interval 
#define DEFAULT_REFRESH 10              // Default initial

typedef long    jobno_t;
typedef long    netid_t;
typedef long    slotno_t;               // May be -ve
typedef long    int_pid_t;
typedef	long	int_ugid_t;       
typedef unsigned long classcode_t;

//      Fields in hosts lines

#define HOSTF_HNAME     0
#define HOSTF_ALIAS     1
#define HOSTF_FLAGS     2
#define HOSTF_TIMEOUT   3
