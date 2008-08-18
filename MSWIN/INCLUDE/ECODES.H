/*
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/ECODES.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: ECODES.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 18.2  1992/10/07  16:22:31  jmc
 * Tidy up declarations and add new network facilities.
 *
 * Revision 18.1  1992/01/11  19:19:19  jmc
 * New release 18
 *
 * Revision 17.1  91/06/14  18:17:47  jmc
 * New version 17.
 *
 * Revision 16.2  91/05/04  13:42:54  jmc
 * Exit code for broken setup files.
 *
 * Revision 16.1  90/12/03  15:12:32  jmc
 * Brand new version 16
 *
 * Revision 15.1  89/10/22  09:58:48  jmc
 * Brand new version 15
 *
 * Revision 14.1  89/10/14  15:52:39  jmc
 * Release 14 introducing shared memory job queues.
 *
 * Revision 13.1  89/10/09  20:17:18  jmc
 * New version 13 with shared memory printer list.
 *
 * Revision 12.1  89/09/30  16:15:47  jmc
 * Version 12 release (to use msgsnd/semaphores).
 *
 * Revision 11.1  89/09/27  20:53:16  jmc
 * Version 11 first release
 *
 *----------------------------------------------------------------------
 *
 *	Standard exit codes
 */

#define	E_USAGE		1
#define	E_BADCLASS	2
/* 3 & 4 unused ... */
#define	E_NOCHDIR	5
#define	E_NOTRUN	6
#define	E_BADPRI	7
#define	E_BADFORM	8
#define	E_BADCPS	9
#define	E_BADPTR	10
#define	E_SHUTD		11
#define	E_AWOPER	12
#define	E_NOJOB		13
#define	E_NOHOST	14
#define	E_NOPRIV	16
#define	E_BADSETUP	40
#define	E_BADCFILE	100
#define	E_INPUTIO	101
#define	E_JDFNFND	150
#define	E_JDNOCHDIR	151
#define	E_JDFNOCR	152
#define	E_JDJNFND	153
#define	E_CANTDEL	154
#define E_NODESCR   	170
#define E_NONOBLOCK 	171
#define E_NOASYNC   	172
#define E_NOLISTEN  	173
#define	E_SIGNAL	200
#define	E_NOPIPE	201
#define	E_NOFORK	202
#define	E_JOBQ		203
#define	E_PRINQ		204
#define	E_IO		230
#define	E_SHEDERR	240
#define	E_SETUP		250
#define E_BUG		251
#define E_NOSERV	252
#define	E_NOMEM		254
#define	E_NOCONFIG	255

extern	void	do_exit(const int);
