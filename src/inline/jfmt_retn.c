/* jfmt_retn.c -- display whether retaining on queue after printing

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

static fmt_t    fmt_retain(const struct spq *jp, const int fwidth)
{
        if  (jp->spq_jflags & SPQ_RETN)  {
                static  char    *retn_msg;
                if  (!retn_msg)
                        retn_msg = gprompt($P{Fmt retain});
                return  (fmt_t) strlen(strcpy(bigbuff, retn_msg));
        }
        return  0;
}
