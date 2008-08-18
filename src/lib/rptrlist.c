/* rptrlist.c -- read printer list from shared memory

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
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "displayopt.h"
#include "incl_unix.h"

#ifndef	USING_FLOCK
extern	int	Sem_chan;
#endif

int	qsort_ptrs(const Hashspptr **, const Hashspptr **);

/* Read through ptrs list and prune according to the options */

void	readptrlist(const int andunlock)
{
	LONG	pind;

	ptrshm_lock();
#ifdef	USING_MMAP
	if  (Ptr_seg.inf.segsize != Job_seg.dptr->js_psegid)
#else
	if  (Ptr_seg.inf.base != Job_seg.dptr->js_psegid)
#endif
		ptrgrown();
	if  (Ptr_seg.dptr->ps_serial == Ptr_seg.Last_ser)  {
		if  (andunlock)
			ptrshm_unlock();
		return;
	}
	Ptr_seg.Last_ser = Ptr_seg.dptr->ps_serial;
	Ptr_seg.nptrs = 0;		/* Ones we're interested in */
	Ptr_seg.npprocesses = 0;	/* Number with active processes  */

	pind = Ptr_seg.dptr->ps_l_head;
	while  (pind >= 0L)  {
		const  Hashspptr  *hpp = &Ptr_seg.plist[pind];
		const  struct  spptr  *pp = &hpp->p;
		pind = hpp->l_nxt;
		if  ((pp->spp_class & Displayopts.opt_classcode) == 0  ||  pp->spp_state == SPP_NULL)
			continue;
		if  (pp->spp_netid  &&  Displayopts.opt_localonly != NRESTR_NONE)
			continue;
		if  (Displayopts.opt_restrp  &&  !qmatch(Displayopts.opt_restrp, pp->spp_ptr))
			continue;
		Ptr_seg.pp_ptrs[Ptr_seg.nptrs++] = hpp;
		if  (pp->spp_state >= SPP_PROC)
			Ptr_seg.npprocesses++;
	}
	if  (andunlock)
		ptrshm_unlock();
	if  (Displayopts.opt_sortptrs && Ptr_seg.nptrs > 1)
		qsort(QSORTP1 Ptr_seg.pp_ptrs, Ptr_seg.nptrs, sizeof(const Hashspptr *), QSORTP4 qsort_ptrs);
}
