/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xtapi_int.h: created by John Collins on Tue Mar  8 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/XTAPI_IN.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: XTAPI_IN.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 23.6  2000/08/28 19:48:58  jmc
 * Add new API calls.
 *
 * Revision 23.5  1999/04/06 10:18:08  jmc
 * Mostly changes to job shared memory to split segment into 2 and make r/o.
 *
 * Revision 23.4  1998/02/23 18:53:13  jmc
 * Revisions for new-style configure.
 *
 * Revision 23.3  1997/12/01 14:56:59  jmc
 * Add stuff for dynamic IP.
 *
 * Revision 23.2  1996/02/23 09:42:17  jmc
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
 * Revision 20.1  1994/03/24  16:58:59  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 */

struct	api_fd	{
	SHORT		portnum;	/* Port number local byte order */
	SOCKET		sockfd;		/* Socket fd */
	SOCKET		prodfd;		/* "Prod" socket fd */
	classcode_t	classcode;	/* Class code to use */
	netid_t		hostid;		/* Host we are talking to net byte order */
	char		username[UIDSIZE+1]; /* User name */
	ULONG		jserial;	/* Serial of jobs */
	ULONG		pserial;	/* Serial of ptrs */
	unsigned	bufmax;		/* Size of Buffer allocated for list operations */
	char		*buff;		/* The buffer itself */
};

struct	api_msg	{
	unsigned  char	code;		/* Code number see below */
#define	API_SIGNON	0
#define	API_SIGNOFF	1
#define	API_JOBLIST	2
#define	API_PTRLIST	3
#define API_JOBREAD	4
#define	API_PTRREAD	5
#define	API_JOBDEL	6
#define	API_PTRDEL	7
#define	API_JOBADD	8
#define	API_PTRADD	9
#define	API_JOBUPD	10
#define	API_PTRUPD	11
#define	API_PTROP	12
#define	API_JOBDATA	13
#define	API_JOBPBRK	14
#define	API_GETSPU	15
#define	API_GETSPD	16
#define	API_PUTSPU	17
#define	API_PUTSPD	18
#define	API_JOBPROD	19
#define	API_PTRPROD	20
#define	API_REQPROD	21
#define	API_UNREQPROD	22
#define	API_DATAIN	30
#define	API_DATAOUT	31
#define	API_DATAEND	32
#define	API_DATAABORT	33
#define	API_FINDJOBSLOT	34
#define	API_FINDPTRSLOT	35
#define	API_FINDJOB	36
#define	API_FINDPTR	37

#define	API_LOGIN	40

	char	resvd[1];		/* Reserved */
	SHORT	retcode;		/* Error return/0 */
	union  {
		struct	{
			classcode_t	classcode;
			char	username[UIDSIZE+1];
		}  signon;
		struct  {
			classcode_t	classcode;
		}  r_signon;
		struct	{
			ULONG	flags;
		}  lister;
		struct  {
			ULONG	nitems;		/* Number of jobs or printers */
			ULONG	seq;		/* Sequence number */
		}  r_lister;
		struct	{
			ULONG	flags;
			ULONG	seq;		/* Sequence number */
			slotno_t	slotno;		/* Slot number */
		}  reader;
		struct	{
			ULONG	flags;
			ULONG	seq;		/* Sequence number */
			slotno_t	slotno;		/* Slot number */
			ULONG	op;
		}  pop;
		struct	{
			ULONG	seq;
		}  r_reader;
		struct  {
			char	username[UIDSIZE+1];
		}  us;
		struct	{
			jobno_t		jobno;
			USHORT	nbytes;
		}  jobdata;
		struct  {
			ULONG	flags;
			netid_t	netid;
			jobno_t	jobno;
		}  jobfind;
		struct  {
			ULONG	flags;
			netid_t	netid;
		}  ptrfind;	/* Followed by ptr name */
		struct  {
			ULONG		seq;
			slotno_t	slotno;
		}  r_find;	/* Possibly followed by job or ptr */
	}  un;
};

#define	XTA_BUFFSIZE	512
#define	API_PASSWDSIZE	127
#define	XI_SOFTWARE	"Xi Software"
#define	PRODNAME	"Xi-Text"
#define	DEFBASED	"\\XITEXT"
#define	INIFILE		"xitext.ini"

#define	DEFAULT_SERVICE	"xtapi"
#define	MON_SERVICE		"xtapi"
#define	XTA_BUFFSIZE	512
#define	DEF_TCPAPIPORTNUM	2210	// Default port number for API TCP
#define	DEF_UDPAPIPORTNUM	2210	// Default port number for API UDP

/*ERRSTART*/

/* Error codes */

#define	XT_OK			(0)
#define	XT_INVALID_FD		(-1)
#define	XT_NOMEM		(-2)
#define	XT_INVALID_HOSTNAME	(-3)
#define	XT_INVALID_SERVICE	(-4)
#define	XT_NODEFAULT_SERVICE	(-5)
#define	XT_NOSOCKET		(-6)
#define	XT_NOBIND		(-7)
#define	XT_NOCONNECT		(-8)
#define	XT_BADREAD		(-9)
#define	XT_BADWRITE		(-10)
#define	XT_CHILDPROC		(-11)

/* These errors should correspond to client_if.h sort of  */
#define	XT_CONVERT_XTNR(code)	(-20-(code))
#define	XT_UNKNOWN_USER		(-23)
#define	XT_ZERO_CLASS		(-24)
#define	XT_BAD_PRIORITY		(-25)
#define	XT_BAD_COPIES		(-26)
#define XT_BAD_FORM		(-27)
#define	XT_NOMEM_QF		(-28)
#define	XT_BAD_PF		(-29)
#define	XT_NOMEM_PF		(-30)
#define	XT_CC_PAGEFILE		(-31)
#define	XT_FILE_FULL		(-32)
#define	XT_QFULL		(-33)
#define	XT_EMPTYFILE		(-34)
#define	XT_BAD_PTR		(-35)
#define	XT_WARN_LIMIT		(-36)
#define	XT_PAST_LIMIT		(-37)
#define	XT_NO_PASSWD		(-38)
#define	XT_PASSWD_INVALID	(-39)

#define	XT_UNKNOWN_COMMAND	(-40)
#define	XT_SEQUENCE		(-41)
#define	XT_UNKNOWN_JOB		(-42)
#define	XT_UNKNOWN_PTR		(-43)
#define	XT_NOPERM		(-44)
#define	XT_NOTPRINTED		(-45)
#define	XT_PTR_NOTRUNNING	(-46)
#define	XT_PTR_RUNNING		(-47)
#define	XT_PTR_NULL		(-48)
#define	XT_PTR_CDEV		(-49)
#define	XT_INVALIDSLOT		(-50)

/* Flags for accessing things */

#define	XT_FLAG_LOCALONLY	(1 << 0)
#define	XT_FLAG_USERONLY	(1 << 1)
#define	XT_FLAG_IGNORESEQ	(1 << 2)
#define	XT_FLAG_FORCE		(1 << 3)
/*ERREND*/
