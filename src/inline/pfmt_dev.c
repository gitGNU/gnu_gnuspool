/* pfmt_dev.c -- display printer device name

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

static  fmt_t	fmt_device(const struct spptr *pp, const int fwidth)
{
	if  (pp->spp_netflags & SPP_LOCALNET)  {
#ifdef	INLINE_SPLIST
		static	char	*nsmsg, *nemsg;
		if  (!nsmsg)  {
			nsmsg = gprompt($P{Netdev start str});
			nemsg = gprompt($P{Netdev end str});
		}
#endif
		sprintf(bigbuff, "%s%s%s", nsmsg, pp->spp_dev, nemsg);
	}
	else
		strcpy(bigbuff, pp->spp_dev);
	return  (fmt_t) strlen(bigbuff);
}
