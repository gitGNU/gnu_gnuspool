/* o_verbose.c -- options to set verbose (job submitted ok)

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

OPTION(o_verbose)
{
	verbose = 1;
	return  OPTRESULT_OK;
}

OPTION(o_noverbose)
{
	verbose = 0;
	return  OPTRESULT_OK;
}

OPTION(o_togverbose)
{
	verbose = !verbose;
	return  OPTRESULT_OK;
}
