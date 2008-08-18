/* o_mailwrt.c -- options to set/unset mail/write flags

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

OPTION(o_wrt)
{
#ifdef	INLINE_SQCHANGE
	doing_something++;
	wrt_changes++;
#endif
	SPQ.spq_jflags |= SPQ_WRT;
	return  OPTRESULT_OK;
}

OPTION(o_mail)
{
#ifdef	INLINE_SQCHANGE
	doing_something++;
	mail_changes++;
#endif
	SPQ.spq_jflags |= SPQ_MAIL;
	return  OPTRESULT_OK;
}

OPTION(o_mattn)
{
#ifdef	INLINE_SQCHANGE
	doing_something++;
	mattn_changes++;
#endif
	SPQ.spq_jflags |= SPQ_MATTN;
	return  OPTRESULT_OK;
}

OPTION(o_wattn)
{
#ifdef	INLINE_SQCHANGE
	doing_something++;
	wattn_changes++;
#endif
	SPQ.spq_jflags |= SPQ_WATTN;
	return  OPTRESULT_OK;
}

OPTION(o_nomailwrt)
{
#ifdef	INLINE_SQCHANGE
	doing_something++;
	wrt_changes++;
	mail_changes++;
#endif
	SPQ.spq_jflags &= ~(SPQ_WRT|SPQ_MAIL);
	return  OPTRESULT_OK;
}

OPTION(o_noattn)
{
#ifdef	INLINE_SQCHANGE
	doing_something++;
	mattn_changes++;
	wattn_changes++;
#endif
	SPQ.spq_jflags &= ~(SPQ_MATTN|SPQ_WATTN);
	return  OPTRESULT_OK;
}
