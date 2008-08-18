/* pfmt_limit.c -- display if printer has upper/lower job size limits

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

static  fmt_t	fmt_limit(const struct spptr *pp, const int fwidth)
{
	char	*lbp = bigbuff;
	fmt_t	ret = 0;
	if  (pp->spp_minsize != 0)  {
		*lbp++ = '<';
		ret++;
	}
	if  (pp->spp_maxsize != 0)  {
		*lbp++ = '>';
		ret++;
	}
	return  ret;
}
