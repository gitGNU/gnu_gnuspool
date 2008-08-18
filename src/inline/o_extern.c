/* o_extern.c -- options to select host/user for "external" programs

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

OPTION(o_orighost)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
#ifdef	INLINE_SPR
	if  (Realuid == Daemuid  ||  Realuid == ROOTID)  {
		SPQ.spq_orighost = look_hostname(arg);
		if  (SPQ.spq_orighost == myhostid)
			SPQ.spq_orighost = 0L;
	}
#endif
	return  OPTRESULT_ARG_OK;
}

OPTION(o_queueuser)
{
#ifdef	INLINE_SPR
	int_ugid_t	nuid;
#endif
	if  (!arg)
		return  OPTRESULT_MISSARG;
#ifdef	INLINE_SPR
	if  ((Realuid == Daemuid  ||  Realuid == ROOTID)  &&  (nuid = lookup_uname(arg)) != UNKNOWN_UID)  {
		SPQ.spq_uid = nuid;
		if  (strcmp(SPQ.spq_puname, SPQ.spq_uname) == 0)
			strncpy(SPQ.spq_puname, arg, UIDSIZE);
		strncpy(SPQ.spq_uname, arg, UIDSIZE);
	}
#endif
	return  OPTRESULT_ARG_OK;
}

OPTION(o_external)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
#ifdef	INLINE_SPR
	if  (Realuid == Daemuid  ||  Realuid == ROOTID)  {
		int	num;
		if  (isdigit(arg[0]))  {
			num = atoi(arg);
			if  (!ext_numtoname(num))
				return  OPTRESULT_ARG_OK;
		}
		else  if  ((num = ext_nametonum(arg)) < 0)
			return  OPTRESULT_ARG_OK;
		SPQ.spq_extrn = (USHORT) num;
	}
#endif
	return  OPTRESULT_ARG_OK;
}
