/* o_copies.c -- option to select copies

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

OPTION(o_copies)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;

	num = atoi(arg);
	if  (num < 0 || num > 255)  {
		disp_arg[3] = 255;
		disp_arg[1] = num;
		disp_arg[2] = 0;
		print_error($E{Bad arg copies});
		exit(E_USAGE);
	}

#ifdef	INLINE_SQCHANGE
	if  (!(mypriv->spu_flgs & PV_ANYPRIO)  &&  num > (int) mypriv->spu_cps)  {
		disp_arg[0] = num;
		disp_arg[1] = mypriv->spu_cps;
		print_error($E{Invalid copies});
		exit(E_BADCPS);
	}
	doing_something++;
	cps_changes++;
#endif
	SPQ.spq_cps = (unsigned char) num;
	return  OPTRESULT_ARG_OK;
}
