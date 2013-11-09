/* o_jinclall.c -- option to set whether "no printer selected" jobs are included

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

OPTION(o_jinclnonull)
{
        Displayopts.opt_jinclude = JINCL_NONULL;
        return  OPTRESULT_OK;
}

OPTION(o_jinclnull)
{
        Displayopts.opt_jinclude = JINCL_NULL;
        return  OPTRESULT_OK;
}

OPTION(o_jinclall)
{
        Displayopts.opt_jinclude = JINCL_ALL;
        return  OPTRESULT_OK;
}
