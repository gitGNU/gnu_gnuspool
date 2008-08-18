/*
 * Copyright (c) Xi Software Ltd. 1993.
 *
 * serv_if.h: created by John Collins on Fri Jan  8 1993.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/SERV_IF.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: SERV_IF.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 *
 *	Interface used by sendudp program to send to local printer
 *	The numbers don't clash with the above because we are doing the sending.
 */

#define	SV_CL_DATA		0	/* Data from server */
#define	CL_SV_OK		0	/* Data ok */
#define	CL_SV_OFFLINE	1	/* Device is offline */
#define	CL_SV_ERROR		2	/* Device is error-bound */

#define	SV_CL_BUFFSIZE	256
