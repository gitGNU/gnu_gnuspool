/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * qmatch.c: created by John Collins on Fri Dec 16 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/WINLIB/Qmatch.cpp,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Qmatch.cpp,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 22.2  1995/01/24  11:17:38  jmc
 * Fix buggette in qmatch.
 *
 * Revision 22.1  1995/01/13  17:14:47  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/12/19  13:37:26  jmc
 * Introduced in December 1994.
 *
 *----------------------------------------------------------------------
 */

#include "stdafx.h"
#include "defaults.h"
#include <ctype.h>                                  

extern	BOOL	isinrange(const int, const char *, int &);
extern	int		match_comma(const CString &);

static	BOOL	ematch(const char *pattern, const char FAR *value)
{
	int	cnt;

	for  (;;)  {

		switch  (*pattern)  {
		case  '\0':
			if  (*value == '\0')
				return  TRUE;
			return  FALSE;

		default:
			if  (*pattern != *value  &&  toupper(*pattern) != toupper(*value))
				return  FALSE;
			pattern++;
			value++;
			continue;

		case  '?':
			if  (*value == '\0')
				return  FALSE;
			pattern++;
			value++;
			continue;

		case  '*':
			pattern++;
			for  (cnt = strlen(value); cnt >= 0;  cnt--)
				if  (ematch(pattern, value+cnt))
					return  TRUE;
			return  FALSE;

		case  '[':
			if  (*value == '\0')
				return  FALSE;
			if  (!isinrange(*value, pattern, cnt))
				return  FALSE;
			value++;
			pattern += cnt;
			continue;
		}
	}
}

BOOL	qmatch(CString &pattern, const char FAR *value)
{
	BOOL	res;
	int		cp;
	CString	pp = pattern;

	do  {
		cp = match_comma(pp);
		if  (cp >= 0)  {
			res = ematch((const char *) pp.Left(cp), value);
			pp = pp.Mid(cp+1);
		}
		else
			res = ematch((const char *) pp, value);
		if  (res)
			return  TRUE;
	}  while  (cp >= 0);

	//	Not found...
	return  FALSE;
}
