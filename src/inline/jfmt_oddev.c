/* jfmt_oddev.c -- display omit odd/even pages flags

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

static  fmt_t	fmt_oddeven(const struct spq *jp, const int fwidth)
{
	if  (jp->spq_jflags & (SPQ_ODDP|SPQ_EVENP))  {
		static	char	*no_odd, *no_even, *swap;
		if  (!no_odd)  {
			no_odd = gprompt($P{Fmt no odd});
			no_even = gprompt($P{Fmt no even});
			swap = gprompt($P{Fmt swap});
		}
		if  (jp->spq_jflags & SPQ_ODDP)
			strcpy(bigbuff, no_odd);
		else
			strcpy(bigbuff, no_even);
		if  (jp->spq_jflags & SPQ_REVOE)
			strcat(bigbuff, swap);
		return  (fmt_t) strlen(bigbuff);
	}
	return  0;
}
