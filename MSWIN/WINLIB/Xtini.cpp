/* Xtini.cpp -- load save options in .ini file

   Copyright 2009 Free Software Foundation, Inc.

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

#include "stdafx.h"
#include <limits.h>
#include <memory.h>
#include <string.h>
#include "files.h"
#ifndef	SPRSETW
#include "pages.h"
#include "xtini.h"
#endif
#ifdef	SPQW
#include "spuser.h"
#endif

//  Set up initial options in absence of server info

xtini::xtini()
{
	memset((void *) &qparams, '\0', sizeof(spq));
	qparams.spq_pri = U_DF_DEFP;
	qparams.spq_cps = 1;
	qparams.spq_end = LONG_MAX - 1;
	qparams.spq_nptimeout = QNPTIMEOUT;
	qparams.spq_ptimeout = QPTIMEOUT;
	qparams.spq_class = spq_options.classcode = U_DF_CLASS;
}
