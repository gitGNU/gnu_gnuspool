/* jfmt_wattn.c -- display "write attention" flag

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

static  fmt_t	fmt_wattn(const struct spq *jp, const int fwidth)
{
	if  (jp->spq_jflags & SPQ_WATTN)  {
		static	char	*wattn_msg;

		if  (!wattn_msg)
			wattn_msg = gprompt($P{Fmt wattn});
		return  (fmt_t) strlen(strcpy(bigbuff, wattn_msg));
	}
	return  0;
}
