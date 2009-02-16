/* ecodes.h -- standard exit codes

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
