// Ptrcolours.cpp: implementation of the CPtrcolours class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "spqw.h"
#include "Ptrcolours.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPtrcolours::CPtrcolours()
{
	COLORREF dflt = ::GetSysColor(COLOR_WINDOWTEXT);
	for  (int i = 0;  i < 8;  i++)
		m_table[i] = dflt;
}

CPtrcolours::~CPtrcolours()
{

}

COLORREF	&CPtrcolours::operator [] (const unsigned req)
{
	return  m_table[whichcol(req)];
}

const CPtrcolours::pcl_type  CPtrcolours::whichcol(const unsigned req)
{
	switch  (req)  {
	case SPP_WAIT:		return  IDLE_CL;
	case SPP_RUN:		return  PRINTING_CL;
	case SPP_OPER:		return  AWOPER_CL;
	default:
	case SPP_HALT:		return  HALTED_CL;
	case SPP_ERROR:		return  ERROR_CL;
	case SPP_OFFLINE:	return  OFFLINE_CL;
	case SPP_INIT: 		return  STARTUP_CL;
	case SPP_SHUTD:		return  SHUTD_CL;
	}
}
