//
// Copyright (c) Xi Software Ltd. 1992.
//
// hextoi.cpp: created by John Collins on Mon Nov 30 1992.
//----------------------------------------------------------------------
// $Header: /sources/gnuspool/gnuspool/MSWIN/WINLIB/HEXTOI.CPP,v 1.1 2008/08/18 16:25:54 jmc Exp $
// $Log: HEXTOI.CPP,v $
// Revision 1.1  2008/08/18 16:25:54  jmc
// Initial revision
//
//----------------------------------------------------------------------
// MS C++ Version: 

#include "stdafx.h"
#include <ctype.h>
#include "defaults.h"

classcode_t	hextoi(const char *arg)
{
	classcode_t  result  =  0;

	while  (*arg)  {
		if  (isalpha(*arg))  {
			int	ch;
			if  (toupper(*arg) > 'P')
				return  result;
			if  (isupper(*arg))
				ch = *arg++ - 'A';
			else
				ch = *arg++ - 'a' + 16;
			result |= 1L << ch;
		}
		else  if  (*arg == '.' || *arg == '-')
			arg++;
		else
			return  result;
	}
	return  result;
}
