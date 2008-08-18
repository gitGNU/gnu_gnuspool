/* pfmt_user.c -- display user name of job being printed

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

static  fmt_t	fmt_user(const struct spptr *pp, const int fwidth)
{
	if  (pp->spp_state >= SPP_PREST  &&  pp->spp_jslot >= 0  &&  pp->spp_jslot < Job_seg.dptr->js_maxjobs)  {
		const struct  spq  *jp = &Job_seg.jlist[pp->spp_jslot].j;
		return  (fmt_t) strlen(strcpy(bigbuff, jp->spq_uname));
	}
	return  0;
}
