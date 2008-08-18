/* o_jobwait.c -- option to set job timeout limit / page limit

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

OPTION(o_jobwait)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	jobtimeout = atoi(arg);
	return  OPTRESULT_ARG_OK;
}

OPTION(o_pagelimit)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;

	if  (*arg == '-')  {
	zapit:
		SPQ.spq_dflags &= ~(SPQ_ERRLIMIT|SPQ_PGLIMIT);
		SPQ.spq_pglim = 0;
		return  OPTRESULT_ARG_OK;
	}
	if  (toupper(*arg) == 'N')  {
		SPQ.spq_dflags &= ~SPQ_ERRLIMIT;
		arg++;
	}
	else  if  (toupper(*arg) == 'E')  {
		SPQ.spq_dflags |= SPQ_ERRLIMIT;
		arg++;
	}

	num = 0;
	while  (isdigit(*arg))
		num = num * 10 + *arg++ - '0';

	if  (num <= 0  ||  num  > 65535)
		goto  zapit;

	SPQ.spq_pglim = (USHORT) num;

	if  (toupper(*arg) == 'P')
		SPQ.spq_dflags |= SPQ_PGLIMIT;
	else
		SPQ.spq_dflags &= ~SPQ_PGLIMIT;
	return  OPTRESULT_ARG_OK;
}
