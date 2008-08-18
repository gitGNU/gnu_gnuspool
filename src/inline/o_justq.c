/* o_justq.c -- option to limit display to given printer(s)

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

OPTION(o_justq)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (Displayopts.opt_restrp)
		free(Displayopts.opt_restrp);
	Displayopts.opt_restrp = (arg[0] && (arg[0] != '-' || arg[1]))? stracpy(arg): (char *) 0;
	return  OPTRESULT_ARG_OK;
}
