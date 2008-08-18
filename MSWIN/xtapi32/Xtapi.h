/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xtapi.pre: created by John Collins on Tue Mar  8 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xtapi.h,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xtapi.h,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 23.1  1996/02/13 09:02:51  jmc
 * Brand New Release 23.
 *
 * Revision 22.2  1995/10/05  18:26:45  jmc
 * Fix problem with CONSTs.
 *
 * Revision 22.1  1995/01/13  17:07:20  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:22:26  jmc
 * Brand new Release 21
 *
 * Revision 20.1  1994/03/24  17:25:52  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 * Prefix to xtapi.h
 */

#ifndef	_XTAPI_H
#define	_XTAPI_H

/*	Copied from config.h for i386 */

#define STDC_HEADERS 1
#define NETWORK_VERSION 1
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_SHORT 2
#define SIZEOF_UNSIGNED 4
#define SIZEOF_UNSIGNED_LONG 4
#define SIZEOF_UNSIGNED_SHORT 2
#define HAVE_ATEXIT 1
#define HAVE_MEMCPY 1
#define HAVE_STRCHR 1
#define	LONG	long
#define	ULONG	unsigned long
#define	SHORT	short
#define	USHORT	unsigned short
#define	ROOTID	0

/*
 *	Copied from defaults.h
 */

#define	NAMESIZE	14		/* File name size allowing for restricted names */
#define	MAXTITLE	30		/* Size of header */
#define	PTRNAMESIZE	28		/* Printer name size  */
#define	JPTRNAMESIZE	(PTRNAMESIZE*2+2)/* Printer name size in job  */
#define	LINESIZE	29		/* Size of dev field */
#define	MAXFORM		34		/* Size of form field INCREASED */
#define MAXFLAGS	62		/* Size of P/P flags buffer */
#define	UIDSIZE		11		/* Size of UID field */
#define	HOSTNSIZE	14		/* Host name size */
#define	PFEEDBACK	39		/* Feedback field on printer */
#define	COMMENTSIZE	41		/* Comment size on printer */
#define	ALLOWFORMSIZE	63
typedef	LONG	jobno_t;
typedef	LONG	netid_t;
typedef	LONG	slotno_t;		/* May be -ve	*/
typedef	LONG		int_ugid_t;
typedef	LONG		int_pid_t;
typedef	ULONG	classcode_t;

/*
 *	Copied from spq.h
 */

struct	apispq	{	/*  Entry in spool queue  */
	jobno_t		apispq_job;	/*  Job number  */
	netid_t		apispq_netid;	/*  Network id (local byte order) 0 local */
	netid_t		apispq_orighost;	/*  Who it came from 0 if this machine or APISPQ_ROAMUSER set*/
	slotno_t	apispq_rslot;	/*  Slot number remote machine */
	time_t		apispq_time;	/*  When submitted  */
	time_t		apispq_proptime;	/*  Proposal (etc) time in cases messages go astray */
	time_t		apispq_starttime;	/*  When (first) started */
	time_t		apispq_hold;	/*  Hold until ... */
	USHORT		apispq_nptimeout;	/*  Timeout if not printed (hours) */
	USHORT		apispq_ptimeout;	/*  Timeout if printed (hours)  */
	LONG		apispq_size;	/*  Size of job in bytes  */
	LONG		apispq_posn;	/*  Position in job in bytes  */
	LONG		apispq_pagec;	/*  Page count in job */
	ULONG		apispq_npages;	/*  Number of pages  */

	unsigned  char  apispq_cps;	/*  Copies  */
	unsigned  char  apispq_pri;	/*  Priority  */
	SHORT		apispq_wpri;	/*  Working priority  */

	USHORT	apispq_jflags;		/*  Job flags */
#define	APISPQ_NOH		(1 << 0)	/*  Suppress heading  */
#define	APISPQ_WRT		(1 << 1)	/*  Send message to users terminal  */
#define	APISPQ_MAIL	(1 << 2)	/*  Mail message to user  */
#define	APISPQ_RETN	(1 << 3)	/*  Retain in Q  */
#define	APISPQ_ODDP	(1 << 4)	/*  Omit Odd pages */
#define	APISPQ_EVENP	(1 << 5)	/*  Omit Even pages */
#define	APISPQ_REVOE	(1 << 6)	/*  Reverse odd/even */
#define	APISPQ_MATTN	(1 << 7)	/*  Mail attention message */
#define	APISPQ_WATTN	(1 << 8)	/*  Write attention message */
#define	APISPQ_LOCALONLY	(1 << 9)	/*  Local only */
#define	APISPQ_CLIENTJOB	(1 << 10)	/*  Job arrived from client */
#define	APISPQ_ROAMUSER	(1 << 11)	/*  Roaming user */

	unsigned char	apispq_sflags;	/*  Flags set by scheduler */
#define	APISPQ_ASSIGN	(1 << 0)	/*  Assigned to printer  */
#define	APISPQ_WARNED	(1 << 1)	/*  Warned about top of queue */
#define	APISPQ_PROPOSED	(1 << 2)	/*  Proposed non-local print */
#define	APISPQ_ABORTJ	(1 << 3)	/*  Set for job abort */

	unsigned char	apispq_dflags;	/*  Flags set by spd */
#define	APISPQ_PQ		(1 << 0)	/*  Being printed  */
#define	APISPQ_PRINTED	(1 << 1)	/*  Printed it at least once */
#define	APISPQ_STARTED	(1 << 2)	/*  Job has been started sometime */
#define	APISPQ_PAGEFILE	(1 << 3)	/*  Has a page delimiter file */
#define	APISPQ_ERRLIMIT	(1 << 4)	/*  Error if exceeds upper bound */
#define	APISPQ_PGLIMIT	(1 << 5)	/*  Limit is by pages */

	USHORT		apispq_extrn;	/*  External job index 0=Xi-Text */
	USHORT		apispq_pglim;	/*  K byte limit or number of pages (enqueue only) */

	classcode_t	apispq_class;	/*  Class code */

	slotno_t	apispq_pslot;	/*  Printer slot if printing or -1
					    also -1 if printing by unknown remote */

	ULONG		apispq_start,
			apispq_end,	/*  Record to start/finish at  */
			apispq_haltat;	/*  Page number we were halted at */

	int_ugid_t	apispq_uid;	/*  Originating user (binary) */

	char	apispq_uname[UIDSIZE+1];	/*  Originating user  */
	char	apispq_puname[UIDSIZE+1];	/*  User to post output to  */

	char  apispq_file[MAXTITLE+1];	/*  File name  */
	char  apispq_form[MAXFORM+1];	/*  Paper type  */
	char  apispq_ptr[JPTRNAMESIZE+1];	/*  Printer type */
	char  apispq_flags[MAXFLAGS+1];	/*  Flags to use for filter */
};

struct	apispptr	{	/*  Details of printer  */
	netid_t		apispp_netid;	/*  Network id 0 local */
	slotno_t	apispp_rslot;	/*  Slot number on remote machine */

	int_pid_t	apispp_pid;	/*  Process id of spd process if applicable  */
	jobno_t		apispp_job;	/*  Current job  */
	netid_t		apispp_rjhostid;	/*  Machine owning job being printed or 0 */
	slotno_t	apispp_rjslot;	/*  Slot on remote of job being printed or same as apispp_jslot */
	slotno_t	apispp_jslot;	/*  Slot in Jlist on this machine or -1  */

	char		apispp_state;	/*  Process state  */
	unsigned  char	apispp_sflags;	/*  Scheduler flags */
#define	APISPP_SELECT	(1 << 0)	/*  Selected for printing */
#define	APISPP_INTER	(1 << 1)	/*  Set for job interrupt */
#define	APISPP_HEOJ	(1 << 2)	/*  Set for halt end of job */
#define	APISPP_PROPOSED	(1 << 3)	/*  Proposed */
	unsigned  char	apispp_dflags;	/*  Flags set by spd */
#define	APISPP_HADAB	(1 << 0)	/*  Had abort message  */
#define	APISPP_REQALIGN	(1 << 1)	/*  Requires alignment */
	unsigned  char	apispp_netflags;	/*  Network printer flags */
#define	APISPP_LOCALNET	(1 << 0)	/*  Access by network= stuff */
#define	APISPP_LOCALONLY	(1 << 1)	/*  local printer only */

	classcode_t	apispp_class;	/*  Class  */

	ULONG		apispp_minsize;	/*  Minimum size we'll accept */
	ULONG		apispp_maxsize;	/*  Maximum size we'll accept */

	USHORT		apispp_extrn;	/*  External printer index 0=Xi-Text */
	USHORT		apispp_resvd;	/*  Reserved pad to 4 bytes */

	char	apispp_dev[LINESIZE+1];	/*  Device  */
	char	apispp_form[MAXFORM+1];	/*  Paper type  */
	char	apispp_ptr[PTRNAMESIZE+1];	/*  Printer type  */
	char	apispp_feedback[PFEEDBACK+1]; /* Feedback from terminal server */
	char	apispp_comment[COMMENTSIZE+1];/* Description of printer */
};
struct	apipages	{
	LONG	delimnum;	/* Number of delimiters */
	LONG	deliml;		/* Length of delimiters */
	LONG	lastpage;	/* Delimiters remaining on last page */
};

#define	PV_ADMIN	0x1		/*  Administrator  */
#define	PV_SSTOP	0x2		/*  Can run sstop  */
#define	PV_FORMS	0x4		/*  Can use other forms  */
#define	PV_CPRIO	0x8		/*  Can change priority on spq  */
#define	PV_OTHERJ	0x10		/*  Can change other users' jobs */
#define	PV_PRINQ	0x20		/*  Can move to printer queue  */
#define	PV_HALTGO	0x40		/*  Can halt, restart printer  */
#define	PV_ANYPRIO	0x80		/*  Can set any priority on spq  */
#define	PV_CDEFLT	0x100		/*  Can change own default prio  */
#define	PV_ADDDEL	0x200		/*  Can add/delete printers */
#define	PV_COVER	0x400		/*  Can override class  */
#define	PV_UNQUEUE	0x800		/*  Can undump queue */
#define	PV_VOTHERJ	0x1000		/*  Can view other jobs not necc edit */
#define	PV_REMOTEJ	0x2000		/*  Can access remote jobs */
#define	PV_REMOTEP	0x4000		/*  Can access remote printers */
#define	PV_FREEZEOK	0x8000		/*  Can freeze parameters */
#define	PV_ACCESSOK	0x10000		/*  Can access other options */
#define	PV_OTHERP	0x20000		/*  Can use other printers */
#define ALLPRIVS	0x3ffff		/*  All of the above  */

struct	apisphdr	{
	unsigned  char	sph_version;	/* Major Xi-Text version number  */

	char	sph_form[MAXFORM+1];	/* Form type (35 bytes) */

	time_t		sph_lastp;	/* Last read password file */

	unsigned  char	sph_minp,	/* Minimum pri */
			sph_maxp,	/* Maximum pri */
			sph_defp,	/* Default pri */
			sph_cps;	/* Copies */

	ULONG	sph_flgs;		/* Privileges */
	classcode_t	sph_class;	/* Class code */
	char		sph_formallow[ALLOWFORMSIZE+1]; /* Allowed form types (pattern) */
	char		sph_ptr[PTRNAMESIZE+1];	/* Default printer */
	char		sph_ptrallow[JPTRNAMESIZE+1]; /* Allow printer types (pattern) */
};

struct	apispdet	{
	unsigned  char	spu_isvalid;	/* Valid user id = 1, valid but unlic = 2 */
#define	SPU_INVALID	0
#define	SPU_VALIDLIC	1
#define	SPU_VALIDNOLIC	2

	char	spu_form[MAXFORM+1];	/* Default form (35 bytes) */

	int_ugid_t	spu_user;	/* User id */

	unsigned  char	spu_minp,	/* Minimum priority  */
			spu_maxp,	/* Maximum priority  */
			spu_defp,	/* Default priority  */
			spu_cps;	/* Copies */

	ULONG	spu_flgs;		/* Privileges  */
	classcode_t	spu_class;	/* Class of printers */
	char		spu_formallow[ALLOWFORMSIZE+1]; /* Allowed form types (pattern) */
	char		spu_ptr[PTRNAMESIZE+1];	/* Default printer */
	char		spu_ptrallow[JPTRNAMESIZE+1]; /* Allow printer types (pattern) */
#ifdef	__cplusplus
	classcode_t	resultclass(const classcode_t rclass) const
	{
		return  (spu_flgs & PV_COVER) ? rclass: rclass & spu_class;
	}
	int  ispriv(const ULONG flag) const
	{
		return  flag & spu_flgs? 1: 0;
	}
#endif
};

/* Charge record generated by scheduler */

struct	spcharge	{
	time_t		spch_when;	/* When it happened */
	netid_t		spch_host;	/* Host responsible */
	int_ugid_t	spch_user;	/* Uid charged for */
	USHORT	spch_pri;	/* Priority */
	USHORT	spch_what;	/* Type of charge */
#define	SPCH_RECORD	0		/* Record left by spshed */
#define	SPCH_FEE	1		/* Impose fee */
#define	SPCH_FEEALL	2		/* Impose fee everywhere */
#define	SPCH_CONSOL	3		/* Consolidation of previous charges */
#define	SPCH_ZERO	4		/* Zero record for given user */
#define	SPCH_ZEROALL	5		/* Zero record for all users */
	LONG		spch_chars;	/* Chars printed */
	LONG		spch_cpc;	/* Charge per character or charge (FEE/CONSOL) */
};

/*
 *	Printer state codes
 */

#define	API_PRNULL	0	/*  Null entry  */
#define	API_PROFFLINE	1	/*  Device is offline  */
#define	API_PRERROR	2	/*  Some error  */
#define	API_PRHALT	3	/*  Halted  */
#define	API_PRINIT	4	/*  Initialising  */
#define	API_PRWAIT	5	/*  Idle  */
#define	API_PRSHUTD	6	/*  Shutdown */
#define	API_PRRUN		7	/*  Printing something  */
#define	API_PROPER	8	/*  Awaiting operator  */
#define	API_PRPROC	API_PRINIT /* Process allocated from this state up */
#define	API_PRPREST	API_PRRUN	 /* Print details of job etc */
#define	API_PRNSTATES	9

/*
 *	Printer op codes
 */

#define	PRINOP_RSP		26	/*  restart printer */
#define	PRINOP_PHLT		27	/*  halt printer at eoj */
#define	PRINOP_PSTP		28	/*  halt printer at once */
#define	PRINOP_PGO		29	/*  start printer */
#define	PRINOP_OYES		32	/*  spq->spshed operator continue  */
#define	PRINOP_ONO		33	/*  spq->spshed operator retry  */
#define	PRINOP_INTER	34	/*  interrupt printer  */
#define	PRINOP_PJAB		35	/*  abort job on printer */

/* Error codes */

#define	XTAPI_OK			(0)
#define	XTAPI_INVALID_FD		(-1)
#define	XTAPI_NOMEM		(-2)
#define	XTAPI_INVALID_HOSTNAME	(-3)
#define	XTAPI_INVALID_SERVICE	(-4)
#define	XTAPI_NODEFAULT_SERVICE	(-5)
#define	XTAPI_NOSOCKET		(-6)
#define	XTAPI_NOBIND		(-7)
#define	XTAPI_NOCONNECT		(-8)
#define	XTAPI_BADREAD		(-9)
#define	XTAPI_BADWRITE		(-10)
#define	XTAPI_CHILDPROC		(-11)

/* These errors should correspond to client_if.h sort of  */
#define	XTAPI_CONVERT_XTNR(code)	(-20-(code))
#define	XTAPI_UNKNOWN_USER		(-23)
#define	XTAPI_ZERO_CLASS		(-24)
#define	XTAPI_BAD_PRIORITY		(-25)
#define	XTAPI_BAD_COPIES		(-26)
#define XTAPI_BAD_FORM		(-27)
#define	XTAPI_NOMEM_QF		(-28)
#define	XTAPI_BAD_PF		(-29)
#define	XTAPI_NOMEM_PF		(-30)
#define	XTAPI_CC_PAGEFILE		(-31)
#define	XTAPI_FILE_FULL		(-32)
#define	XTAPI_QFULL		(-33)
#define	XTAPI_EMPTYFILE		(-34)
#define	XTAPI_BAD_PTR		(-35)
#define	XTAPI_WARN_LIMIT		(-36)
#define	XTAPI_PAST_LIMIT		(-37)
#define	XTAPI_NO_PASSWD		(-38)
#define	XTAPI_PASSWD_INVALID	(-39)

#define	XTAPI_UNKNOWN_COMMAND	(-40)
#define	XTAPI_SEQUENCE		(-41)
#define	XTAPI_UNKNOWN_JOB		(-42)
#define	XTAPI_UNKNOWN_PTR		(-43)
#define	XTAPI_NOPERM		(-44)
#define	XTAPI_NOTPRINTED		(-45)
#define	XTAPI_PTR_NOTRUNNING	(-46)
#define	XTAPI_PTR_RUNNING		(-47)
#define	XTAPI_PTR_NULL		(-48)
#define	XTAPI_PTR_CDEV		(-49)
#define	XTAPI_INVALIDSLOT		(-50)

/* Flags for accessing things */

#define	XTAPI_FLAG_LOCALONLY	(1 << 0)
#define	XTAPI_FLAG_USERONLY	(1 << 1)
#define	XTAPI_FLAG_IGNORESEQ	(1 << 2)
#define	XTAPI_FLAG_FORCE		(1 << 3)

#define	XTWINAPI_JOBPROD	1
#define	XTWINAPI_PTRPROD	2

/*
 * Copyright (c) Xi Software Ltd. 1994.
 *
 * xtapi.post: created by John Collins on Tue Mar  8 1994.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/xtapi32/Xtapi.h,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Xtapi.h,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 * Revision 23.3  2000/08/28 20:58:39  jmc
 * Add new facilities to API for job and printer find.
 * Fix bug in jobdata and jobpbrk.
 *
 * Revision 23.2  1998/02/24 10:27:13  jmc
 * Revisions for new-style configure.
 *
 * Revision 23.1  1996/02/13 09:02:51  jmc
 * Brand New Release 23.
 *
 * Revision 22.2  1995/10/05  18:26:45  jmc
 * Fix problem with CONSTs.
 *
 * Revision 22.1  1995/01/13  17:07:20  jmc
 * Brand New Release 22
 *
 * Revision 21.1  1994/08/31  18:22:26  jmc
 * Brand new Release 21
 *
 * Revision 20.2  1994/04/05  09:02:00  jmc
 * Fix errors in C++ version.
 *
 * Revision 20.1  1994/03/24  17:25:52  jmc
 * Brand new Release 20.
 *
 *----------------------------------------------------------------------
 * Postfix to xtapi.h
 */

#ifdef	__cplusplus
extern	"C"	{
#endif
extern	int	xt_open(const char *, const char *, const char *, const classcode_t);
extern	int	xt_login(const char *, const char *, const char *, char *, const classcode_t);
extern	int	xt_close(const int);
extern	int	xt_joblist(const int, const unsigned, int *, slotno_t **);
extern	int	xt_jobread(const int, const unsigned, const slotno_t, struct apispq *);
extern	int	xt_jobdel(const int, const unsigned, const slotno_t);
extern	int	xt_jobfind(const int, const unsigned, const jobno_t, const netid_t, slotno_t *, struct apispq *);
extern	int	xt_jobfindslot(const int, const unsigned, const jobno_t, const netid_t, slotno_t *);
extern	int	xt_jobupd(const int, const unsigned, const slotno_t, const struct apispq *);
extern	int	xt_ptrlist(const int, const unsigned, int *, slotno_t **);
extern	int	xt_ptrread(const int, const unsigned, const slotno_t, struct apispptr *);
extern	int	xt_ptradd(const int, const struct apispptr *);
extern	int	xt_ptrdel(const int, const unsigned, const slotno_t);
extern  int	xt_ptrfind(const int, const unsigned, const char *, const netid_t, slotno_t *, struct apispptr *);
extern	int	xt_ptrfindslot(const int, const unsigned, const char *, const netid_t, slotno_t *);
extern	int	xt_ptrop(const int, const unsigned, const slotno_t, const unsigned);
extern	int	xt_ptrupd(const int, const unsigned, const slotno_t, const struct apispptr *);
extern	int	xt_getspd(const int, struct apisphdr *);
extern	int	xt_getspu(const int, const char *, struct apispdet *);
extern	int	xt_putspd(const int, const struct apisphdr *);
extern	int	xt_putspu(const int, const char *, const struct apispdet *);
extern	int	xt_procmon(const int);
extern	int	xt_setmon(const int, HWND, UINT);
		
extern	void	xt_unsetmon(const int);

extern	int	xt_jobadd(const int, const int, int (*)(int,void*,unsigned), struct apispq *, const char *, const unsigned, const unsigned);
extern	int	xt_jobdata(const int, const int, int (*)(int,void*,unsigned), const unsigned, const slotno_t);
extern	int	xt_jobpbrk(const int, const int, int (*)(int,void*,unsigned), const unsigned, const slotno_t);

extern	const  char  *	getxtlogin(HANDLE, const char *, const char *, const UINT);
extern	BOOL	runxtlog(const char *, const char *);

#ifdef	__cplusplus
}
#endif

extern	int	xtapi_dataerror;

#endif	/*_XTAPI_H*/
