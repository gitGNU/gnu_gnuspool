/* defaults.h -- various defaults and sizes

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

#define NAMESIZE        14              /* File name size allowing for restricted names */
#define MAXTITLE        30              /* Size of header */
#define PTRNAMESIZE     28              /* Printer name size  */
#define JPTRNAMESIZE    (PTRNAMESIZE*2+2)/* Printer name size in job  */
#define LINESIZE        29              /* Size of dev field */
#define MAXFORM         34              /* Size of form field INCREASED */
#define MAXFLAGS        62              /* Size of P/P flags buffer */
#define UIDSIZE         11              /* Size of UID field */
#define WUIDSIZE        23              /* Windows UID size max (same for now) */
#define HOSTNSIZE       14              /* Host name size */
#define PFEEDBACK       39              /* Feedback field on printer */
#define COMMENTSIZE     41              /* Comment size on printer */
#define ALLOWFORMSIZE   63

#define DEF_LUMPSIZE    50              /* Number of jobs/printers to send at once on startup */
#define DEF_LUMPWAIT    2               /* Time to wait in each case */
#define DEF_CLOSEDELAY  1               /* Time to wait for other end to digest msg before close */

#define LOTSANDLOTS     99999999L       /* Maximum page number */

/* Timeout intervals and such */

#define QREWRITE        300             /* Time to write queues to file */
#define QNPTIMEOUT      (7*24)          /* One week if not printed */
#define QPTIMEOUT       24              /* One day if printed */
#define NETTICKLE       1000            /* Keep networks alive */
#define LICWARN         14              /* Days to warn about licence */

/* Other things mostly to do with forms */

#define DEF_CHARGE      1000            /* I.e. charges divided by 1000 */
#define DEF_OBUF        1024            /* Output buffer size */
#define DEF_OPEN        30              /* Timeout before we give up */
#define DEF_CLOSE       100             /* Timout on close */
#define DEF_OFFLINE     30              /* Timeout for writing */

#define DEF_WIDTH       80              /* Page width (for banners) */
#define DEF_RCOUNT      1               /* No of delimiters */
#define DEF_DELIM       '\f'            /* Default delimiter */
#define DEF_SUFCHARS    ".-"            /* Default suffix chars */

#define RECURSE_MAX     10              /* Maximum level of recursive expansion of $-style constructs */

/* Timezone - change this as needed (mostly needed for DOS version).  */

#define DEFAULT_TZ      "TZ=GMT0BST"
#define UNIXTODOSTIME   ((70UL * 365UL + 68/4 + 1) * 24UL * 3600UL)

/* Display bits for spq */

#define DEFAULT_PLINES  3               /* Size of printer list */
#define MAX_PLINES      15              /* Display maximum */

#define POLLMAX         90              /* Maximum polling interval */
#define POLLMIN         2               /* Minimum polling interval */
#define DEFAULT_REFRESH 10              /* Default initial */

#define MSGQ_BLOCKS     30              /* Number of times we try message queue */
#define MSGQ_BLOCKWAIT  10              /* Sleep time between message queue tries */

/* Moved from spq.h as needed in network.h which we put in first */

typedef LONG    jobno_t;
typedef LONG    netid_t;
typedef LONG    slotno_t;               /* May be -ve   */

/* Hold user/group/process ids internally thus to avoid machine variations */

typedef LONG            int_ugid_t;
typedef LONG            int_pid_t;
typedef ULONG           classcode_t;
