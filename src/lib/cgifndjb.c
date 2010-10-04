/* cgifndjb.c -- decode job numbers/printer names for use in CGI routines

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

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "displayopt.h"
#include "cgifndjb.h"

int  numeric(const char *x)
{
	while  (*x)  {
		if  (!isdigit(*x))
			return  0;
		x++;
	}
	return  1;
}

int  decode_jnum(char *jnum, struct jobswanted *jwp)
{
	char	*cp;

	if  ((cp = strchr(jnum, ':')))  {
		*cp = '\0';
		if  ((jwp->host = look_hostname(jnum)) == 0L)  {
			*cp = ':';
			disp_str = jnum;
			return  $E{Unknown host name};
		}
		if  (jwp->host == myhostid)
			jwp->host = 0L;
		*cp++ = ':';
	}
	else  {
		jwp->host = 0L;
		cp = jnum;
	}
	if  (!numeric(cp)  ||  (jwp->jno = (jobno_t) atol(cp)) == 0)  {
		disp_str = jnum;
		return  $E{job num not numeric};
	}
	return  0;
}

int  decode_pname(char *pname, struct ptrswanted *pwp)
{
	char	*cp;

	if  ((cp = strchr(pname, ':')))  {
		*cp = '\0';
		if  ((pwp->host = look_hostname(pname)) == 0L)
			return  0;
		if  (pwp->host == myhostid)
			pwp->host = 0L;
		*cp++ = ':';
	}
	else  {
		pwp->host = 0L;
		cp = pname;
	}

 	pwp->ptrname = stracpy(cp);
	return  1;
}

const Hashspq *find_job(struct jobswanted *jw)
{
	LONG  jind;

	jobshm_lock();
	jind = Job_seg.hashp_jno[jno_jhash(jw->jno)];

	while  (jind >= 0L)  {
		const  Hashspq  *hjp = &Job_seg.jlist[jind];
		if  (hjp->j.spq_job == jw->jno  &&  hjp->j.spq_netid == jw->host  &&  (hjp->j.spq_class & Displayopts.opt_classcode) != 0)  {
			jobshm_unlock();
			jw->jp = &hjp->j;
			return  hjp;
		}
		jind = hjp->nxt_jno_hash;
	}
	jobshm_unlock();
	jw->jp = (const struct spq *) 0;
	return  (Hashspq *) 0;
}

const Hashspptr *find_ptr(struct ptrswanted *pw)
{
	LONG  pind;

	ptrshm_lock();
	pind = Ptr_seg.dptr->ps_l_head;

	while  (pind >= 0L)  {
		const  Hashspptr  *cp = &Ptr_seg.plist[pind];
		pind = cp->l_nxt;
		if  (cp->p.spp_state == SPP_NULL  ||  cp->p.spp_netid != pw->host)
			continue;
		/* Warning - this finds the first printer of that name on that
		   host only - is this a problem? If so, may have to identify by
 		   device */
		if  (strcmp(cp->p.spp_ptr, pw->ptrname) != 0)
			continue;
		pw->pp = &cp->p;
		ptrshm_unlock();
		return  cp;
	}
	ptrshm_unlock();
	return  (Hashspptr *) 0;
}
