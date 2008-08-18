/* o_format.c -- option to set format string

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

OPTION(o_formatstr)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (formatstring)
		free(formatstring);
	formatstring = stracpy(arg);
	return  OPTRESULT_ARG_OK;
}

OPTION(o_formatdflt)
{
	if  (formatstring)  {
		free(formatstring);
		formatstring = (char *) 0;
	}
	return  OPTRESULT_OK;
}

OPTION(o_header)
{
	headerflag = 1;
	return	OPTRESULT_OK;
}

OPTION(o_noheader)
{
	headerflag = 0;
	return	OPTRESULT_OK;
}
