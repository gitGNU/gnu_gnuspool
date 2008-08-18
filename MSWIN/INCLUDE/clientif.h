/*
 * Copyright (c) Xi Software Ltd. 1992.
 *
 * client_if.h: created by John Collins on Tue Dec 15 1992.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/clientif.h,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: clientif.h,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 23.5  1997/12/23 09:52:15  jmc
 * Add keep alive stuff to xtnetserv.
 *
 * Revision 23.4  1997/12/01 14:56:59  jmc
 * Add stuff for dynamic IP.
 *
 * Revision 23.3  1997/06/03 16:25:41  jmc
 * Add sequence checking in rspr and xtnetserv.
 *
 * Revision 23.2  1996/02/23 09:42:54  jmc
 * Add user-checking code to DOS interface.
 *
 * Revision 23.1  1996/02/13  09:03:38  jmc
 * Brand New Release 23.
 *
 * Revision 22.1  1995/01/13  17:07:58  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:19:41  jmc
 * Brand new Release 21
 *
 * Revision 20.1  1994/03/24  16:57:52  jmc
 * Brand new Release 20.
 *
 * Revision 19.3  1993/03/08  12:02:16  jmc
 * Add new option to send user list.
 *
 * Revision 19.2  1993/01/14  08:43:24  jmc
 * Revisions for additional UDP interface.
 *
 * Revision 19.1  1992/12/19  18:03:55  jmc
 * New Release 19.
 *
 *----------------------------------------------------------------------
 * Structure of reply and error codes sent back to clients
 */

struct	client_if	{
	unsigned  char	flag;			/* 0 ok otherwise error */
	unsigned  char	resvd[3];		/* Padding */
	jobno_t		jobnum;			/* Job number or error code */
};

/*
 *	OK (or not)
 */

#define	XTNQ_OK		0

/*
 *	Error codes
 */

#define XTNR_UNKNOWN_CLIENT	1
#define XTNR_NOT_CLIENT		2
#define XTNR_NOT_USERNAME	3
#define XTNR_ZERO_CLASS		4
#define XTNR_BAD_PRIORITY	5
#define XTNR_BAD_COPIES		6
#define XTNR_BAD_FORM		7
#define XTNR_NOMEM_QF		8
#define XTNR_BAD_PF		9
#define XTNR_NOMEM_PF		10
#define XTNR_CC_PAGEFILE	11
#define XTNR_FILE_FULL		12
#define XTNR_QFULL		13
#define	XTNR_EMPTYFILE		14
#define	XTNR_BAD_PTR		15
#define	XTNR_WARN_LIMIT		16
#define	XTNR_PAST_LIMIT		17
#define	XTNR_NO_PASSWD		18
#define	XTNR_PASSWD_INVALID	19
#define	XTNR_LOST_SYNC		20
#define	XTNR_LOST_SEQ		21

/*
 *	UDP interface for RECEIVING data from client.
 *	The interface which we listen on to accept jobs and enquiries.
 */

#define	CL_SV_UENQUIRY		0	/* Request for permissions (single byte) */
#define	CL_SV_STARTJOB		1	/* Start job */
#define	CL_SV_CONTDELIM		2	/* More delimiter */
#define	CL_SV_JOBDATA		3	/* Job data */
#define	CL_SV_ENDJOB		4	/* End of last job */
#define	CL_SV_HANGON		5	/* Hang on for next block of data */

#define	CL_SV_ULIST		10	/* Send list of valid users */

#define	SV_CL_TOENQ		20	/* Are you still there? (single byte) */
#define	SV_CL_PEND_FULL		21	/* Queue of pending jobs full */
#define	SV_CL_UNKNOWNC		22	/* Unknown command */
#define	SV_CL_BADPROTO		23	/* Something wrong protocol */
#define	SV_CL_UNKNOWNJ		24	/* Out of sequence job */

/*
 *	Reply to request for permissions
 */

struct	ua_reply	{
	char	ua_uname[UIDSIZE+1];		/* Size of user name */
	struct	spdet	ua_perm;
};

#define	UA_PASSWDSZ	31

/*
 *	ua_login structure has now been enhanced to get the machine name
 *	in as well.
 *	We try to support the old procedure (apart from password check)
 *	as much as possible.
 */

struct	ua_login	{
	unsigned  char	ual_op;				/* Operation/result as below */
	unsigned  char	ual_fill; 			/* Filler */
	USHORT		ual_fill1; 			/* Filler */
	char		ual_name[UIDSIZE+1];		/* User or default user */
	char		ual_passwd[UA_PASSWDSZ+1];	/* Password */
	char		ual_machname[HOSTNSIZE+2]; 	/* Client's machine name + 1 byte filler */
};

#define	UAL_LOGIN	30	/* Log in with user name & password */
#define	UAL_LOGOUT	31	/* Log out */
#define	UAL_ENQUIRE	32	/* Enquire about user id */
#define	UAL_OK		33	/* Logged in ok */
#define	UAL_NOK		34	/* Not logged in yet */
#define	UAL_INVU	35	/* Not logged in, invalid user */
#define	UAL_INVP	36	/* Not logged in, invalid passwd */

#define	CL_SV_BUFFSIZE	512	/* Buffer for data client/server */

struct	ua_pal  {		/* Talk to friends */
	unsigned  char	uap_op;		/* Msg - see below */
	unsigned  char	uap_fill; 	/* Filler */
	USHORT		uap_fill1;	/* Filler */
	netid_t		uap_netid; 	/* IP we'return talking about */
	char		uap_name[UIDSIZE+1];	/* Unix end user */
	char		uap_wname[UIDSIZE+1];	/* Windross server */
};

#define	UAU_MAXU	20	/* Limit on number times one user logged in */

struct	ua_asku_rep  {
	USHORT		uau_n;		/* Number of people */
	USHORT		uau_fill; 	/* Filler */
	netid_t		uau_ips[UAU_MAXU];
};

#define	SV_SV_LOGGEDU	50 	/* Confirm OK to other servers */
#define	SV_SV_ASKU	51	/* Ask other servers about specific user */
#define	SV_SV_ASKALL	52	/* Ask other servers about all users */

#define	CL_SV_KEEPALIVE	70	/* Keep connection alive */

/*
 *	Structure for passing jobs etc across on TCP links.
 *	We resort to this to avoid problems with missed packets.
 */

struct	tcp_data  {
	unsigned  char	tcp_code; 	/* What we're doing */

#define	TCP_STARTJOB	0		/* Start of job */
#define	TCP_PAGESPEC	1		/* Page spec */
#define	TCP_ENDJOB	2		/* Job end */
#define	TCP_CLOSESOCK	3		/* End of conversation */
#define	TCP_DATA	4		/* Data */

	unsigned  char	tcp_seq;	/* Sequence number in data */

	unsigned  short	tcp_size;	/* Size for sending data */

	char	tcp_buff[CL_SV_BUFFSIZE];	/* Yer actual data */
};
