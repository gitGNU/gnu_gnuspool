//
// Copyright (c) Xi Software Ltd. 1992.
//
// hex_disp.cpp: created by John Collins on Mon Nov 30 1992.
//----------------------------------------------------------------------
// $Header: /sources/gnuspool/gnuspool/MSWIN/WINLIB/HEX_DISP.CPP,v 1.1 2008/08/18 16:25:54 jmc Exp $
// $Log: HEX_DISP.CPP,v $
// Revision 1.1  2008/08/18 16:25:54  jmc
// Initial revision
//
//----------------------------------------------------------------------
// MS C++ Version: 

#include "stdafx.h"
#include "defaults.h"                                  
                                  
char	*hex_disp(const classcode_t value)
{
#ifdef	PREREL22
	static	char	result[17];
	for  (int i = 0;  i < 16;  i++)
		result[i] = (value & (1 << i))? 'A' + i: '.';

	return  result;
#else
	static	char	result[33];
	for  (int i = 0;  i < 16;  i++)
		result[i] = (value & (1L << i))? 'A' + i: '.';
	for  (i = 0;  i < 16;  i++)
		result[i] = (value & (1L << (i+16)))? 'a' + i: '.';
	return  result;
#endif
}
