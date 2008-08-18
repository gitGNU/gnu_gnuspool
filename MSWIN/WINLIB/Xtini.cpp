// Copyright (c) Xi Software Ltd. 1993.
//
// xtini.cpp: created by John Collins on Sat Jan 23 1993.
//----------------------------------------------------------------------
// $Header: /sources/gnuspool/gnuspool/MSWIN/WINLIB/Xtini.cpp,v 1.1 2008/08/18 16:25:54 jmc Exp $
// $Log: Xtini.cpp,v $
// Revision 1.1  2008/08/18 16:25:54  jmc
// Initial revision
//
//----------------------------------------------------------------------
// MS C++ Version: 

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
