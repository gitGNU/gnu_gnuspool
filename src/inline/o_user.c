/* o_user.c -- option to set "post to" user

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

static int      o_user(const char *arg)
{
        if  (!arg)
                return  -1;
#ifdef  INLINE_SQCHANGE
        doing_something++;
        uname_changes++;
#endif
        if  (arg[0] == '-' &&  arg[1] == '\0')
                strcpy(SPQ.spq_puname, SPQ.spq_uname);
        else  if  (lookup_uname(arg) == UNKNOWN_UID)  {
                disp_str = arg;
                print_error($E{Unknown post user});
                strcpy(SPQ.spq_puname, SPQ.spq_uname);
        }
        else
                strncpy(SPQ.spq_puname, arg, UIDSIZE);
        return  1;
}
