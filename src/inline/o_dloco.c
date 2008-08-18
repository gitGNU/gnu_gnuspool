/* o_dloco.c -- option to restrict display to local jobs only

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

OPTION(o_localonly)
{
	Displayopts.opt_localonly = NRESTR_LOCALONLY;
	return  OPTRESULT_OK;
}

OPTION(o_nolocalonly)
{
	Displayopts.opt_localonly = NRESTR_NONE;
	return  OPTRESULT_OK;
}
