/* rjoblist.c -- read job list from shared memory

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
#include <stdio.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "displayopt.h"
#include "incl_unix.h"

/* Include this here to resolve library undefined symbol */

dispopt_t	Displayopts =  { JINCL_NULL, JRESTR_ALL, NRESTR_NONE, SORTP_NONE };

/* Read through job queue and prune according to the options */

void  readjoblist(const int andunlock)
{
	LONG  jind;

	jobshm_lock();
#ifdef	USING_MMAP
	if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)
#else
	if  (Job_seg.dinf.base != Job_seg.dptr->js_did)
#endif
		jobgrown();

	/* Do nothing if no changes have taken place */

	if  (Job_seg.dptr->js_serial != Job_seg.Last_ser)  {

		Job_seg.Last_ser = Job_seg.dptr->js_serial;
		Job_seg.njobs = 0;		/* Ones we're interested in */

		jind = Job_seg.dptr->js_q_head;

		while  (jind >= 0L)  {
			const  Hashspq  *hjp = &Job_seg.jlist[jind];
			const  struct  spq  *jp = &hjp->j;

			jind = hjp->q_nxt;

			if  ((jp->spq_class & Displayopts.opt_classcode) == 0)
				continue;

			if  (jp->spq_netid  &&  Displayopts.opt_localonly != NRESTR_NONE)
				continue;

			switch  (Displayopts.opt_jprindisp)  {
			case  JRESTR_UNPRINT:
				if  (jp->spq_dflags & SPQ_PRINTED)
					continue;
				break;

			case  JRESTR_PRINT:
				if  (!(jp->spq_dflags & SPQ_PRINTED))
					continue;
			default:
				break;
			}

			if  (Displayopts.opt_restru  &&  !qmatch(Displayopts.opt_restru, jp->spq_uname))
				continue;

			if  (Displayopts.opt_restrt  &&  !qmatch(Displayopts.opt_restrt, jp->spq_file))
				continue;

			if  (Displayopts.opt_restrp)  {
				/* Gyrations as jp is read-only (and issubset modifies args) */
				char	pb[JPTRNAMESIZE+1];

				switch  (Displayopts.opt_jinclude)  {
				case  JINCL_NONULL:
					if  (!jp->spq_ptr[0])
						continue;
					goto  chkmtch;
				case  JINCL_NULL:
					if  (!jp->spq_ptr[0])
						break;
				chkmtch:
					strncpy(pb, jp->spq_ptr, JPTRNAMESIZE);
					pb[JPTRNAMESIZE] = '\0';
					if  (!issubset(Displayopts.opt_restrp, pb))
						continue;

				default:
					break;
				}
			}
			Job_seg.jj_ptrs[Job_seg.njobs] = hjp;
			Job_seg.njobs++;
		}
	}
	if  (andunlock)
		jobshm_unlock();
}
