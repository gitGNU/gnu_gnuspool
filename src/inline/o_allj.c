/* o_allj.c -- option routines for selecting jobs to display printed/unprinted

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

OPTION(o_allj)
{
        Displayopts.opt_jprindisp = JRESTR_ALL;
        return  OPTRESULT_OK;
}

OPTION(o_justnp)
{
        Displayopts.opt_jprindisp = JRESTR_UNPRINT;
        return  OPTRESULT_OK;
}

OPTION(o_justp)
{
        Displayopts.opt_jprindisp = JRESTR_PRINT;
        return  OPTRESULT_OK;
}
