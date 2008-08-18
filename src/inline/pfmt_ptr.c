/* pfmt_ptr.c -- display printer name

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

static  fmt_t	fmt_printer(const struct spptr *pp, const int fwidth)
{
#ifdef	CHARSPRINTF
	if  (pp->spp_netid)  {
		sprintf(bigbuff, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
		return  (fmt_t) strlen(bigbuff);
	}
#else
	if  (pp->spp_netid)
		return  sprintf(bigbuff, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
#endif
	return  (fmt_t) strlen(strcpy(bigbuff, pp->spp_ptr));
}
