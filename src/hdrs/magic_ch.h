/* magic_ch.h -- designate whether keys are "magic" and have special meanings

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

#define MAG_A           0x1                     /*  Alphabetic chars can be magic  */
#define MAG_P           0x2                     /*  Any printing chars can be magic */
#define MAG_R           0x4                     /*  Return value in retv  */
#define MAG_OK          0x8                     /*  Wgets - allow non-alpha chars  */
#define MAG_CRS         0x10                    /*  Cursor up/down */
#define MAG_NL          0x20                    /*  Treat null input as leave unch */
#define MAG_FNL         0x40                    /*  As MAG_NL but even if typed and erased */
#define MAG_LONG        0x80                    /*  Wgets - allow long strings */

extern  int     getkey(const unsigned);
extern  void    endwinkeys();
