/* jfmt_krchd.c -- display where we've reached in K

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

static  fmt_t  fmt_kreached(const struct spq *jp, const int fwidth)
{
	return  (jp->spq_dflags & SPQ_PQ) ? (fmt_t) strlen(strcpy(bigbuff, prin_size(jp->spq_posn))): 0;
}
