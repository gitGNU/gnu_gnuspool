/* o_justu.c -- options to limit display to user(s)

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

OPTION(o_justu)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (Displayopts.opt_restru)
                free(Displayopts.opt_restru);
        Displayopts.opt_restru = (arg[0] && (arg[0] != '-' || arg[1]))? stracpy(arg): (char *) 0;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_justt)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (Displayopts.opt_restrt)
                free(Displayopts.opt_restrt);
        Displayopts.opt_restrt = (arg[0] && (arg[0] != '-' || arg[1]))? stracpy(arg): (char *) 0;
        return  OPTRESULT_ARG_OK;
}
