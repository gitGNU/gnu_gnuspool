/* jfmt_range.c -- display page range

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

static  fmt_t	fmt_range(const struct spq *jp, const int fwidth)
{
	if  (jp->spq_start != 0)  {
		if  (jp->spq_end < LOTSANDLOTS)
			sprintf(bigbuff, "%ld-%ld", jp->spq_start+1L, jp->spq_end+1L);
		else
			sprintf(bigbuff, "%ld-", jp->spq_start+1L);
	}
	else  if  (jp->spq_end < LOTSANDLOTS)
		sprintf(bigbuff, "-%ld", jp->spq_end+1L);
	else
		return  0;
	return  (fmt_t) strlen(bigbuff);
}

static fmt_t  fmt_hat(const struct spq *jp, const int fwidth)
{
	if  (jp->spq_haltat == 0)
		return  0;
#ifdef	CHARSPRINTF
	sprintf(bigbuff, "%*ld", jp->spq_haltat+1L);
	return  (fmt_t) strlen(bigbuff);
#else
	return  (fmt_t) sprintf(bigbuff, "%*ld", fwidth, jp->spq_haltat+1L);
#endif
}
