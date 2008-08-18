/* o_delimnum.c -- option to select delimiter number

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

OPTION(o_delimnum)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	pfe.delimnum = atoi(arg);
	if  (pfe.delimnum <= 0)  {
		disp_arg[0] = pfe.delimnum;
		disp_str = arg;
		print_error($E{Invalid delim num});
		exit(E_USAGE);
	}
	return  OPTRESULT_ARG_OK;
}
