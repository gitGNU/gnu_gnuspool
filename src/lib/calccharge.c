/* calccharge.c -- calculate charge for given uid

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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "errnums.h"
#include "defaults.h"
#include "spuser.h"
#include "files.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "incl_sig.h"

/* Calculate charge for given user id.
   The intention is to deprecate charges in later releases
   so this is likely to go. */

LONG	calccharge(const int_ugid_t uid)
{
	int			fd;
	LONG			had = 0;
	char			*fname = envprocess(CHFILE);
	double			result = 0.0;
	struct	spcharge	rec;

	fd = open(fname, O_RDONLY);
	free(fname);
	if  (fd < 0)
		return  0;

	while  (read(fd, (char *) &rec, sizeof(rec)) == sizeof(rec))
		switch  (rec.spch_what)  {
		case  SPCH_RECORD:	/* Record left by spshed */
			if  (rec.spch_user == uid)  {
				double	res = rec.spch_pri;
				res /= U_DF_DEFP;
				result += res * res * rec.spch_chars * rec.spch_cpc / 1E6;
				had++;
			}
			break;

		case  SPCH_FEE:			/* Impose fee */
			if  (rec.spch_user == uid)  {
				result += (double) rec.spch_cpc;
				had++;
			}
			break;

		case  SPCH_FEEALL:		/* Impose fee to all current */
			if  (had)
				result += (double) rec.spch_cpc;
			break;

		case  SPCH_CONSOL:		/* Consolidation of previous charges */
			if  (rec.spch_user == uid)  {
				result = (double) rec.spch_cpc;
				had++;
			}
			break;

		case  SPCH_ZERO:		/* Zero record for given user */
			if  (rec.spch_user == uid)  {
				result = 0.0;
				had++;
			}
			break;

		case  SPCH_ZEROALL:		/* Zero record for all users */
			result = 0.0;
			break;
		}

	close(fd);
	return  result;
}
