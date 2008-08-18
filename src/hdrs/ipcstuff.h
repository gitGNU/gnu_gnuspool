/* ipcstuff.h -- constants for keyid etc on IPC stuff

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

#define	IPC_ID(appl, no)	(('X'<<24)|('i'<<16)|((appl)<<12)|no)
#define	XT_ID(no)		IPC_ID(1, no)

#define	MSGID			XT_ID(0)
#define	SEMID			XT_ID(1)
#define	XSHMID			XT_ID(2)
#define	JSHMID			XT_ID(3)	/*  Job info segmentt */
#define	SHMID			XT_ID(4)	/*  A base  */
#define	SEMNUMS			5
#define	MAXSHMS			100		/*  Before wrapping  */
#define	JSHMOFF			0		/*  Offset on base for job data segment */
#define	PSHMOFF			1		/*  Offset on base for ptr data segment */
#define	SHMINC			2		/*  Number to increment by */

/* Indices into semaphore array for various ops */

#define	JQ_FIDDLE		0
#define	JQ_READING		1
#define	PQ_FIDDLE		2
#define	PQ_READING		3
#define	XT_LOCK			4

/* Some funny machines (e.g. Sequent) don't define this */
#ifndef	MAP_FAILED
#define	MAP_FAILED ((void *) -1)
#endif
