/* pfmt_jobno.c -- display job number being printed

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

static  fmt_t	fmt_jobno(const struct spptr *pp, const int fwidth)
{
	if  (pp->spp_state >= SPP_PREST  &&  pp->spp_jslot >= 0  &&  pp->spp_jslot < Job_seg.dptr->js_maxjobs)  {
		const struct  spq  *jp = &Job_seg.jlist[pp->spp_jslot].j;

		/* If the field can't take the whole shebang, chop off the host name */

		if  (jp->spq_netid)  {
#ifdef	INLINE_SPLIST
#ifdef	CHARSPRINTF
			sprintf(bigbuff, "%s:%ld", look_host(jp->spq_netid), (long) pp->spp_job);
			return  (fmt_t) strlen(bigbuff);
#else
			return  (fmt_t) sprintf(bigbuff, "%s:%ld", look_host(jp->spq_netid), (long) pp->spp_job);
#endif
#else  /* Not SPLIST */
			int	lng;
			char	fbuf[HOSTNSIZE+30];
#ifdef	CHARSPRINTF
			sprintf(fbuf, "%s:%ld", look_host(jp->spq_netid), (long) pp->spp_job);
			lng = strlen(fbuf);
#else
			lng = sprintf(fbuf, "%s:%ld", look_host(jp->spq_netid), (long) pp->spp_job);
#endif
			if  (lng <= fwidth)  {
#ifdef	CHARSPRINTF
				sprintf(bigbuff, "%*s", fwidth, fbuf);
				return  (fmt_t) strlen(bigbuff);
#else
				return  (fmt_t) sprintf(bigbuff, "%*s", fwidth, fbuf);
#endif
			}
#endif /* Not SPLIST */
		}
#ifdef	CHARSPRINTF
		sprintf(bigbuff, "%*ld", fwidth, (long) pp->spp_job);
		return  (fmt_t) strlen(bigbuff);
#else
		return  (fmt_t) sprintf(bigbuff, "%*ld", fwidth, (long) pp->spp_job);
#endif
	}
	return  0;
}
