/* pfmt_ab.c -- display "aborted/interrupted"

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

static  fmt_t	fmt_ab(const struct spptr *pp, const int fwidth)
{
	if  (pp->spp_dflags & SPP_HADAB)  {
#ifdef	INLINE_SPLIST
		static	char	*intermsg;
		if  (!intermsg)
			intermsg = gprompt($P{Printer interrupted});
#endif
		return  (fmt_t) strlen(strcpy(bigbuff, intermsg));
	}
	return  0;
}
