/* o_range.c -- option to set page range

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

static int	o_range(const char * arg)
{
	LONG	start = 1L, nd = MAXLONG;

	if  (!arg)
		return  -1;

	if  (*arg == '-')  {
		if  (*++arg)
			nd = atol(arg);
	}
	else  {
		start = atol(arg);
		while  (*++arg && *arg != '-')
			;
		if  (*arg == '-')  {
			if  (*++arg)
				nd = atol(arg);
		}
		else
			nd = start;
	}
	if  (start > nd  ||  start <= 0L)  {
		disp_arg[0] = start;
		disp_arg[1] = nd;
		print_error($E{Bad page range});
		exit(E_USAGE);
	}

	/* Humans number pages at 1...
	   (silly people).  */

#ifdef	INLINE_SQCHANGE
	doing_something++;
	range_changes++;
#endif
	SPQ.spq_start = start - 1L;
	SPQ.spq_end = nd - 1L;
	return  1;
}
