/* sortp.c -- sort routine for printers

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
#include <ctype.h>
#include <stdio.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "incl_unix.h"

int  qsort_ptrs(const Hashspptr **a, const Hashspptr **b)
{
	const  struct  spptr  *ap = &(*a)->p, *bp = &(*b)->p;
	int	ret;

	if  (ap->spp_netid != bp->spp_netid)
		return  ap->spp_netid == 0? -1:
			bp->spp_netid == 0? 1:
			ap->spp_netid < bp->spp_netid? -1: 1;
	if  ((ret = strcmp(ap->spp_ptr, bp->spp_ptr)) != 0)
		return  ret;
	return  strcmp(ap->spp_dev, bp->spp_dev);
}
