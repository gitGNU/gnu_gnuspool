/*
 * Copyright (c) Xi Software Ltd. 1992.
 *
 * pages.h: created by John Collins on Mon Nov  9 1992.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/PAGES.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: PAGES.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 * Layout of page file
 */

struct	pages	{
	long	delimnum;	// Number of delimiters
	long	deliml;		// Length of delimiters
	long	lastpage;	// Delimiters remaining on last page
};

//	This is followed by the delimiter itself, and then a vector of
//	longs, giving the starts of page 2 onwards.
//	Thinks:
//		Maybe UNIX's idea of a file offset isn't DOS'es.
//		Use binary files for now and hope for the best....
