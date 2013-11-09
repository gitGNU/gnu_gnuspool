/* hextoi.c -- convert class code chars to binary.

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

#include "config.h"
#include "defaults.h"
#include <ctype.h>

classcode_t  hextoi(const char *arg)
{
        classcode_t  result  =  0;
        unsigned  ch;

        while  (*arg)  {
                if  (isalpha(*arg))  {
                        if  (toupper(*arg) > 'P')
                                return  result;
                        if  (isupper(*arg))
                                ch = *arg++ - 'A';
                        else
                                ch = *arg++ - 'a' + 16;
                        result |= 1 << ch;
                        if  (arg[0] == '-'  &&  isalpha(arg[1])  &&  toupper(arg[1]) <= 'P')  {
                                unsigned        ch2;
                                arg++;
                                if  (isupper(*arg))
                                        ch2 = *arg++ - 'A';
                                else
                                        ch2 = *arg++ - 'a' + 16;
                                if  (ch2 < ch)
                                        return  result;
                                while  (ch <= ch2)
                                        result |= 1 << ch++;
                        }
                }
                else  if  (*arg == '.')
                        arg++;
                else
                        return  result;
        }
        return  result;
}
