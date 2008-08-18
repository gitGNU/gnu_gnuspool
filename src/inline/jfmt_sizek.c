/* jfmt_sizek.c -- display size of job in K/M etc

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

static fmt_t	fmt_sizek(const struct spq *jp, const int fwidth)
{
	char	*res = prin_size(jp->spq_size);
	int	resl = strlen(res);
	if  (resl >= fwidth)  {
		strcpy(bigbuff, res);
		return  (fmt_t) resl;
	}
	sprintf(bigbuff, "%*s", fwidth, res);
	return  (fmt_t) fwidth;
}
