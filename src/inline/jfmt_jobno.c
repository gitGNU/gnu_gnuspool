/* jfmt_jobno.c -- display job numbers prefixed with host name

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

static fmt_t fmt_jobno(const struct spq *jp, const int fwidth)
{
#ifdef	INLINE_SQLIST
#ifdef	CHARSPRINTF
	if  (jp->spq_netid)
		sprintf(bigbuff, "%s:%.*ld", look_host(jp->spq_netid), jno_width, (long) jp->spq_job);
	else
		sprintf(bigbuff, "%.*ld", jno_width, (long) jp->spq_job);
	return  (fmt_t) strlen(bigbuff);
#else
	if  (jp->spq_netid)
		return  (fmt_t) sprintf(bigbuff, "%s:%.*ld", look_host(jp->spq_netid), (int) jno_width, (long) jp->spq_job);
	else
		return  (fmt_t) sprintf(bigbuff, "%.*ld", (int) jno_width, (long) jp->spq_job);
#endif
#else  /* ! INLINE_SQLIST */
	if  (jp->spq_netid)  {
		char	fbuf[HOSTNSIZE+30];
		sprintf(fbuf, "%s:%ld", look_host(jp->spq_netid), (long) jp->spq_job);
#ifdef	CHARSPRINTF
		sprintf(bigbuff, "%*s", fwidth, fbuf);
		return  (fmt_t) strlen(bigbuff);
#else
		return  (fmt_t) sprintf(bigbuff, "%*s", fwidth, fbuf);
#endif
	}
	else
#ifdef	CHARSPRINTF
		sprintf(bigbuff, "%*ld", fwidth, (long) jp->spq_job);
#else
		return  (fmt_t) sprintf(bigbuff, "%*ld", fwidth, (long) jp->spq_job);
#endif
#endif
}
