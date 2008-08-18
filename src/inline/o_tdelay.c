/* o_tdelay.c -- option to set "hold time"

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

OPTION(o_tdelay)
{
	int	num, hours, mins, secs;

	if  (!arg)
		return  OPTRESULT_MISSARG;

	disp_str = arg;		/* In case we get an error */

	if  (*arg == '-')  {
		SPQ.spq_hold = 0l;
		return  OPTRESULT_ARG_OK;
	}

	if  (!isdigit(*arg))  {
	badtim:
		print_error($E{Invalid time digit});
		exit(E_USAGE);
	}

	num = 0;
	do  num = num * 10 + *arg++ - '0';
	while  (isdigit(*arg));

	if  (*arg != ':')  {
		if  (*arg != '\0')
			goto  badtim;
		hours = 0;
		mins = num;
		secs = 0;
	}
	else  {
		hours = num;
		mins = 0;
		arg++;
		while  (isdigit(*arg))
			mins = mins * 10 + *arg++ - '0';
		secs = 0;
		if  (*arg == ':')  {
			arg++;
			while  (isdigit(*arg))
				secs = secs * 10 + *arg++ - '0';
		}
		if  (*arg != '\0'  ||  mins >= 60  ||  secs >= 60)
			goto  badtim;
	}
#ifdef	INLINE_SQCHANGE
	doing_something++;
	tdel_changes++;
#endif
	SPQ.spq_hold = (LONG) (time((time_t *) 0) + (hours * 60 + mins) * 60 + secs);
	return  OPTRESULT_ARG_OK;
}
