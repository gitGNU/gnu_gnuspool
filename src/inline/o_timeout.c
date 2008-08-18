/* o_timeout.c -- options to set job deletion timeouts

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

OPTION(o_ptimeout)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;

	num = atoi(arg);
	if  (num <= 0 || num > 32767)  {
		disp_arg[3] = 32767;
		disp_arg[1] = num;
		disp_arg[2] = 1;
		print_error($E{Timeout range});
		exit(E_USAGE);
	}

#ifdef	INLINE_SQCHANGE
	doing_something++;
	pto_changes++;
#endif
	SPQ.spq_ptimeout = (USHORT) num;
	return  OPTRESULT_ARG_OK;
}

OPTION(o_nptimeout)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;

	num = atoi(arg);
	if  (num <= 0 || num > 32767)  {
		disp_arg[3] = 32767;
		disp_arg[1] = num;
		disp_arg[2] = 1;
		print_error($E{Timeout range});
		exit(E_USAGE);
	}

#ifdef	INLINE_SQCHANGE
	doing_something++;
	npto_changes++;
#endif
	SPQ.spq_nptimeout = (USHORT) num;
	return  OPTRESULT_ARG_OK;
}
