/* pages.h -- layout of page file

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

/*APISTART - beginning of section copied for API*/
struct	pages	{
	LONG	delimnum;	/* Number of delimiters */
	LONG	deliml;		/* Length of delimiters */
	LONG	lastpage;	/* Delimiters remaining on last page */
};
/*APIEND - end of section copied for API */

/* This is followed by the delimiter itself, and then a vector of
   longs, giving the starts of page 2 onwards.  For DOS etc
   clients the vector of longs is not supplied.  */
