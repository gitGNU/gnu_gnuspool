/*
 * Copyright (c) Xi Software Ltd. 1992.
 *
 * sctrl.h: created by John Collins on Wed Oct 28 1992.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/SCTRL.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: SCTRL.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 */

typedef	char	**FPPC(const char *);

#define	HELPLESS	(FPPC *) 0

struct	sctrl	{
	int	helpcode;		//  Help code (see help file)
	FPPC	*helpfn;		//  Routine which will return block
					//  of specific possibilities
	unsigned  short	size;		//  size of field
	short	retv;			//  Return value for MAG_R
	unsigned  char	col;		//  Position on screen
	unsigned  char	magic_p;	//  Magic printing chars
	long	min, max;		//  Min/max values
	char	*msg;			//  Special description (if any)
};

#define	MAG_A	1			//  Alphabetic chars can be magic 
#define	MAG_P	2			//  Any printing chars can be magic
#define	MAG_R	4			//  Return value in retv 
#define	MAG_OK	8			//  Wgets - allow non-alpha chars 
#define	MAG_CRS	16			//  Cursor up/down
#define	MAG_NL	32			//  Treat null input as leave unch
