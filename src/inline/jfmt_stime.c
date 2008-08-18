/* jfmt_stime.c -- display job submission time

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

static  fmt_t	fmt_stime(const struct spq *jp, const int fwidth)
{
	time_t	st = jp->spq_time;
	struct	tm	*tp = localtime(&st);
	int	mon = tp->tm_mon+1;
	int	day = tp->tm_mday;
#ifdef	HAVE_TM_ZONE
	if  (tp->tm_gmtoff <= -4 * 60 * 60)
#else
	if  (timezone >= 4 * 60 * 60)
#endif
	{
		day = mon;
		mon = tp->tm_mday;
	}
#ifdef	CHARSPRINTF
#ifndef	INLINE_SQLIST
	if  (fwidth < 14)  {
		time_t	now = time((time_t) 0);
		if  ((time_t) jp->spq_time < now || (time_t) jp->spq_time >= now + 24L * 60L * 60L)
			if  (fwidth >= 10)
				sprintf(bigbuff, "%.2d/%.2d/%.4d", day, mon, tp->tm_year + 1900);
			else  if  (fwidth >= 8)
				sprintf(bigbuff, "%.2d/%.2d/%.2d", day, mon, tp->tm_year % 100);
			else
				sprintf(bigbuff, "%.2d/%.2d", day, mon);
		else
			sprintf(bigbuff, "%.2d:%.2d", tp->tm_hour, tp->tm_min);
	}
	else  if  (fwidth < 16)
		sprintf(bigbuff, "%.2d/%.2d/%.2d %.2d:%.2d", day, mon, tp->tm_year % 100, tp->tm_hour, tp->tm_min);
	else
#endif
		sprintf(bigbuff, "%.2d/%.2d/%.2d %.2d:%.4d", day, mon, tp->tm_year + 1900, tp->tm_hour, tp->tm_min);
	return  (fmt_t) strlen(bigbuff);
#else
#ifndef	INLINE_SQLIST
	if  (fwidth < 14)  {
		time_t	now = time((time_t) 0);
		if  ((time_t) jp->spq_time < now || (time_t) jp->spq_time >= now + 24L * 60L * 60L)
			if  (fwidth >= 10)
				return (fmt_t) sprintf(bigbuff, "%.2d/%.2d/%.4d", day, mon, tp->tm_year + 1900);
			else  if  (fwidth >= 8)
				return (fmt_t) sprintf(bigbuff, "%.2d/%.2d/%.2d", day, mon, tp->tm_year % 100);
			else
				return (fmt_t) sprintf(bigbuff, "%.2d/%.2d", day, mon);
		else
			return (fmt_t) sprintf(bigbuff, "%.2d:%.2d", tp->tm_hour, tp->tm_min);
	}
	else  if  (fwidth < 16)
		return  (fmt_t) sprintf(bigbuff, "%.2d/%.2d/%.2d %.2d:%.2d", day, mon, tp->tm_year % 100, tp->tm_hour, tp->tm_min);
	else
#endif
		return  (fmt_t) sprintf(bigbuff, "%.2d/%.2d/%.4d %.2d:%.2d", day, mon, tp->tm_year + 1900, tp->tm_hour, tp->tm_min);
#endif
}
