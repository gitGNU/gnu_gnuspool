/*
 * Copyright (c) Xi Software Ltd. 1995.
 *
 * matchrouts.c: created by John Collins on Wed Dec 27 1995.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/WINLIB/MATCHROU.CPP,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: MATCHROU.CPP,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 */

#include "stdafx.h"
#include "defaults.h"
#include <ctype.h>                                  

int		match_comma(const CString &patt)
{                     
	int	result = 0, lng = patt.GetLength();
	
	while  (result < lng  &&  patt[result] != ',')  {
		if  (patt[result] == '[')  {
			do  result++;
			while  (result < lng  &&  patt[result] != ']');
		}
		if  (result >= lng)
			return  -1;
		result++;
	}
	return  result >= lng? -1: result;
}

BOOL	isinrange(const int ch, const char *patt, int &cnt)
{
	int		nott = 0;
	const	char	*patta = patt;
	
	if  (*++patta == '!')  {
		nott = 1;
		patta++;
	}

	//	Safety in case pattern truncated

	if  (*patta == '\0')
		return  FALSE;

	do  {
		//	Initialise limits of range
		
		int	lrange = *patta++;
		int	hrange = lrange;
		
		if  (*patta == '-')  {
			hrange = *++patta;
			if  (hrange == 0) // Safety in case trunacated
				return  FALSE;

			//	Be relaxed about backwards ranges
			
			if  (hrange < lrange)  {
				int	tmp = hrange;
				hrange = lrange;
				lrange = tmp;
			}
			patta++; // Past rhs of range
		}

		//	If value matches, and we are excluding range, then
		//	pattern doesn't and we quit.
		//	Otherwise we skip to the end.

		int	lv = ch, uv = ch;
		
		if  (isupper(lv))
			lv = tolower(lv);
		if  (islower(uv))
			uv = toupper(uv);
		
		if  ((lv >= lrange  &&  lv <= hrange)  ||  (uv >= lrange  &&  uv <= hrange))  {
			if  (nott)
				return  FALSE;
			while  (*patta  &&  *patta != ']')
				patta++;
			if  (*patta == '\0') // Safety
				return  FALSE;
			patta++;
			cnt = patta - patt;
			return  TRUE;
		}
	}  while  (*patta  &&  *patta != ']');
	
	if  (*patta == '\0') // Safety
		return  FALSE;
	while  (*patta++ != ']')
		;
	if  (!nott)
		return  FALSE;
	cnt = patta - patt;
	return  TRUE;
}

BOOL	repattok(const  CString  &str)
{
	int	 cnt = 0, lng = str.GetLength();
	
	while  (cnt < lng)  {
		if  (str[cnt] == '[')  {
			do  {
				cnt++;
				if  (str[cnt] == ',' || str[cnt] == '[')
					return  FALSE;
			}  while  (str[cnt] && str[cnt] != ']');
			if  (cnt >= lng)
				return  FALSE;
		}
		cnt++;
	}
	return  TRUE;
}
