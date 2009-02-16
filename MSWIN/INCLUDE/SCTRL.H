/* sctrl.h -- "magic" char handling

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

typedef	char	**FPPC(const char *);

#define	HELPLESS	(FPPC *) 0

struct	sctrl	{
	int	helpcode;		//  Help code (see help file)
	FPPC	*helpfn;		//  Routine which will return block
					//  of specific possibilities
	unsigned  short	size;		//  size of field
	short	retv;			//  Return value for MAG_R
	unsigned  char	col;		//  Position on screen
	unsigned  char	magic_p;	//  Magic printing chars
	long	min, max;		//  Min/max values
	char	*msg;			//  Special description (if any)
};

#define	MAG_A	1			//  Alphabetic chars can be magic 
#define	MAG_P	2			//  Any printing chars can be magic
#define	MAG_R	4			//  Return value in retv 
#define	MAG_OK	8			//  Wgets - allow non-alpha chars 
#define	MAG_CRS	16			//  Cursor up/down
#define	MAG_NL	32			//  Treat null input as leave unch
