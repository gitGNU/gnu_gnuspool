/* jfmt_ptr.c -- display printer assigned to job

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

static  fmt_t	fmt_printer(const struct spq *jp, const int fwidth)
{
	if  (jp->spq_dflags & SPQ_PQ)  {
		if  (jp->spq_pslot < 0  || jp->spq_pslot >= Ptr_seg.dptr->ps_maxptrs)
			return  (fmt_t) strlen(strcpy(bigbuff, localptr));
		else  {
			const struct  spptr  *pp = &Ptr_seg.plist[jp->spq_pslot].p;
			if  (pp->spp_state == SPP_NULL)
				return  (fmt_t) strlen(strcpy(bigbuff, localptr));
#ifdef	CHARSPRINTF
			if  (pp->spp_netid)
				sprintf(bigbuff, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
			else
				strcpy(bigbuff, pp->spp_ptr);
			return  (fmt_t) strlen(bigbuff);
#else
			if  (pp->spp_netid)
				return  (fmt_t) sprintf(bigbuff, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
			else
				return  (fmt_t) strlen(strcpy(bigbuff, pp->spp_ptr));
#endif
		}
	}
	if  (jp->spq_ptr[0])
		return  (fmt_t) strlen(strcpy(bigbuff, jp->spq_ptr));
	return  0;
}
