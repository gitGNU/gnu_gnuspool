/*
 * Copyright (c) Xi Software Ltd. 1992.
 *
 * gprompt.h: created by John Collins on Fri Oct 16 1992.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/GPROMPT.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: GPROMPT.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 */

class	gprompt	{
 private:
	short	statenum;
	char	*string;
 public:
	int	helpcode()	{	return  statenum;	};
	int fhelpcode() __far {	return statenum;	};
	operator char *();
	gprompt(const int);
	~gprompt();
};
