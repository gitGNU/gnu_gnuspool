/*
 * Copyright (c) Xi Software Ltd. 1996.
 *
 * defaults.h: created by John Collins on Fri Oct 16 1992.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/DEFAULTS.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: DEFAULTS.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 21.1  1994/08/31  18:19:41  jmc
 * Brand new Release 21
 *
 * Revision 20.1  1994/03/24  16:57:52  jmc
 * Brand new Release 20.
 *
 * Revision 19.3  1993/04/27  15:13:41  jmc
 * Change type of process id to long.
 *
 * Revision 19.2  1993/04/26  18:09:26  jmc
 * Revisions mostly for ansification.
 *
 * Revision 19.1  1992/12/19  18:03:55  jmc
 * New Release 19.
 *
 * Revision 18.3  1992/10/07  16:22:31  jmc
 * Tidy up declarations and add new network facilities.
 *
 * Revision 18.2  1992/09/17  19:51:28  jmc
 * Increase max length of UID field.
 *
 * Revision 18.1  1992/01/11  19:19:08  jmc
 * New release 18
 *
 *----------------------------------------------------------------------
 * DOS C++ Version
 */

#define	NAMESIZE	14		/* File name size allowing for restricted names */
#define	MAXTITLE	30		/* Size of header */
#define	LINESIZE	29		/* Size of dev field */
#define MAXFLAGS	62		// Size of P/P flags buffer
#define	UIDSIZE		11		/* Size of UID field */
#define	HOSTNSIZE	14		/* Host name size */
#define	PFEEDBACK	39		/* Feedback field on printer */

#if	XITEXT_VN < 23
#define	MAXFORM		18		// Size of form field INCREASED
#else
#define	MAXFORM		34		// Size of form field INCREASED
#define	PTRNAMESIZE	28		// Printer name size
#define	JPTRNAMESIZE	(PTRNAMESIZE*2+2)	// Printer name size in job
#define	COMMENTSIZE	41		// Comment size on printer
#define	ALLOWFORMSIZE	63
#endif

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
#if	XITEXT_VN < 22
typedef unsigned long int_ugid_t;
typedef	unsigned short classcode_t;
#else
typedef	long	int_ugid_t;       
typedef unsigned long classcode_t;
#endif

//      Fields in \Xitext\hosts lines

#define HOSTF_HNAME     0
#define HOSTF_ALIAS     1
#define HOSTF_FLAGS     2
#define HOSTF_TIMEOUT   3
