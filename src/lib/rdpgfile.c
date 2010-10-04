/* rdpgfile.c -- get page file

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
#include <sys/types.h>
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "files.h"
#include "incl_unix.h"

#define	INITPAGES	20	/* Initial size to allocate vector for */

#ifdef	NETWORK_VERSION
static	char	Needs_byte_swap,
		Swap_checked;

FILE *net_feed(const int, const netid_t, const slotno_t, const jobno_t);
#endif

int rdpgfile(const struct spq *jp, struct pages *pfep, char **delimp, 	 unsigned *pagenump, LONG **pageoffp)
{
	LONG	*pvec;
	FILE	*fp;

	/* Ensure that we have enough space for known pages
	   Allow margin for error on allocation of 2.  */

	if  (*pagenump < jp->spq_npages + 2)  {
		if  (*pageoffp)
			free((char *) *pageoffp);
		*pagenump = jp->spq_npages < INITPAGES ? INITPAGES: jp->spq_npages;
		if  (!(*pageoffp = (LONG *) malloc((2 + *pagenump) * sizeof(LONG))))
			return  -1;
	}

	*delimp = (char *) 0;		/* None yet */
	if  (!(jp->spq_dflags & SPQ_PAGEFILE))
		return  0;

#ifdef	NETWORK_VERSION
	if  (jp->spq_netid)  {

		/* Remote job - get hook to page file */

		if  (!(fp = net_feed(FEED_PF, jp->spq_netid, jp->spq_rslot, jp->spq_job)))
			return  0;

		/* See if we need to swap round bits */

		if  (!Swap_checked)  {
			Swap_checked = 1;
			Needs_byte_swap = htonl(1234L) != 1234;
		}

		/* Slurp up header */

		if  (fread((char *) pfep, sizeof(struct pages), 1, fp) != 1)  {
			fclose(fp);
			return  0;
		}

		/* Swap around bytes if we have to */

		if  (Needs_byte_swap)  {
			pfep->delimnum = ntohl(pfep->delimnum);
			pfep->deliml = ntohl(pfep->deliml);
			pfep->lastpage = ntohl(pfep->lastpage);
		}

		/* Slurp up delimiter */

		if  (!(*delimp = (char *) malloc((unsigned) pfep->deliml)))  {
			fclose(fp);
			return  -1;
		}

		if  (fread(*delimp, 1, (int) pfep->deliml, fp) != (int) pfep->deliml)
			goto  badfile;

		pvec = *pageoffp;
		pvec[0] = 0L;

		/* Read in vector of page offsets starting at 1 */

		if  (fread((char *) &pvec[1], sizeof(LONG), jp->spq_npages, fp) != jp->spq_npages)
			goto  badfile;

		if  (Needs_byte_swap)  {
			int	i;
			for  (i = 1;  i <= jp->spq_npages;  i++)
				pvec[i] = ntohl(pvec[i]);
		}
	}
	else  {
#endif
		if  (!(fp = fopen(mkspid(PFNAM, jp->spq_job), "r")))
			return  0;

		/* Slurp up header */

		if  (fread((char *) pfep, sizeof(struct pages), 1, fp) != 1)  {
			fclose(fp);
			return  0;
		}

		/* Slurp up delimiter */

		if  (!(*delimp = (char *) malloc((unsigned) pfep->deliml)))  {
			fclose(fp);
			return  -1;
		}

		if  (fread(*delimp, 1, (int) pfep->deliml, fp) != (int) pfep->deliml)
			goto  badfile;

		pvec = *pageoffp;
		pvec[0] = 0L;

		/* Read in vector of page offsets starting at 1 */

		if  (fread((char *) &pvec[1], sizeof(LONG), jp->spq_npages, fp) != jp->spq_npages)
			goto  badfile;
#ifdef	NETWORK_VERSION
	}
#endif

	fclose(fp);
	return  1;

 badfile:
	fclose(fp);
	free(*delimp);
	*delimp = (char *) 0;
	return  0;
}
