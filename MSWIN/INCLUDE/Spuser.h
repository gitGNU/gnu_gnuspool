/*
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/Spuser.h,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: Spuser.h,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
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
 * Revision 19.4  1993/04/27  15:13:41  jmc
 * Change type of process id to long.
 *
 * Revision 19.3  1993/04/26  18:09:26  jmc
 * Revisions mostly for ansification.
 *
 * Revision 19.2  1993/03/20  12:32:34  jmc
 * Add MS Windows declarations.
 *
 * Revision 19.1  1992/12/19  18:03:55  jmc
 * New Release 19.
 *
 * Revision 18.1  1992/01/11  19:19:36  jmc
 * New release 18
 *
 * Revision 17.1  91/06/14  18:18:03  jmc
 * New version 17.
 *
 * Revision 16.1  90/12/03  15:13:47  jmc
 * Brand new version 16
 *
 * Revision 15.1  89/10/22  09:59:05  jmc
 * Brand new version 15
 *
 * Revision 14.1  89/10/14  15:53:06  jmc
 * Release 14 introducing shared memory job queues.
 *
 * Revision 13.1  89/10/09  20:18:15  jmc
 * New version 13 with shared memory printer list.
 *
 * Revision 12.1  89/09/30  16:17:06  jmc
 * Version 12 release (to use msgsnd/semaphores).
 *
 * Revision 11.1  89/09/27  20:53:44  jmc
 * Version 11 first release
 *
 *----------------------------------------------------------------------
 */

#if		XITEXT_VN < 22
#define	SMAXUID	500			/*  Above that we do special things  */
#elif	XITEXT_VN < 23
#define	SMAXUID	5000		/*  Above that we do special things  */
#else
#define	SMAXUID	30000		/*  Above that we do special things  */
#endif

/*
 *	Privileges.
 */

#define	PV_ADMIN	0x1			/*  Administrator  */
#define	PV_SSTOP	0x2			/*  Can run sstop  */
#define	PV_FORMS	0x4			/*  Can use other forms  */
#define	PV_CPRIO	0x8			/*  Can change priority on spq  */
#define	PV_OTHERJ	0x10		/*  Can change other users' jobs */
#define	PV_PRINQ	0x20		/*  Can move to printer queue  */
#define	PV_HALTGO	0x40		/*  Can halt, restart printer  */
#define	PV_ANYPRIO	0x80		/*  Can set any priority on spq  */
#define	PV_CDEFLT	0x100		/*  Can change own default prio  */
#define	PV_ADDDEL	0x200		/*  Can add/delete printers */
#define	PV_COVER	0x400		/*  Can override class  */
#define	PV_UNQUEUE	0x800		/*  Can undump queue */
#if		XITEXT_VN < 22
#define ALLPRIVS	0xfff		/*  All of the above  */
#else
#define	PV_VOTHERJ	0x1000		/*  Can view other jobs not necc edit */
#define	PV_REMOTEJ	0x2000		/*  Can access remote jobs */
#define	PV_REMOTEP	0x4000		/*  Can access remote printers */
#if		XITEXT_VN < 23
#define ALLPRIVS	0x7fff		/*  All of the above  */
#else
#define	PV_FREEZEOK	0x8000		/*  Can freeze parameters */
#define	PV_ACCESSOK	0x10000		/*  Can access other options */
#define	PV_OTHERP	0x20000		/*  Can use other printers */
#define ALLPRIVS	0x3ffff		/*  All of the above  */
#define	NUM_PRIVS	18			/*  Number of bits */
#endif
#endif

/*
 *	Default defaults.
 */

#define	U_DF_MINP	100
#define	U_DF_MAXP	200
#define	U_DF_DEFP	150
#define	U_DF_CPS	10
#if		XITEXT_VN < 22
#define	U_DF_PRIV	0
#define	U_DF_CLASS	0xffff
#elif	XITEXT_VN < 23
#define	U_DF_PRIV	0
#define	U_DF_CLASS	0xffffffffL
#define	U_MAX_CLASS	0xffffffffL
#else
#define	U_DF_PRIV	(PV_FREEZEOK|PV_ACCESSOK|PV_OTHERP)
#define	U_DF_CLASS	0xffffffffL
#define	U_MAX_CLASS	0xffffffffL
#endif

/*
 *	Try quite hard to make everything on a boundary which is a multiple of its size.
 *	This will hopefully avoid arguments about gaps between machines.
 *	Note that MAXFORM % 4 must == 2
 */

/*APISTART - beginning of section copied for API*/
#ifdef	unix

/* DOS C++ Version we aren't interested in header file stuff. */

struct	sphdr	{
#if		XITEXT_VN < 22
	long			sph_lastp;		/* Last read password file */
	unsigned  char	sph_minp, 		/* Minimum pri */
					sph_maxp,		/* Maximum pri */
					sph_defp;		/* Default pri */
	char			sph_form[MAXFORM+1];	/* Form type */
	unsigned  short	sph_flgs;
	unsigned  short sph_class;
	unsigned  char	sph_cps,
					sph_version;
#elif	XITEXT_VN < 23
	unsigned  char	sph_version;/* Major Xi-Text version number  */
	char			sph_form[MAXFORM+1];	/* Form type (19 bytes) */
	time_t			sph_lastp;	/* Last read password file */
	unsigned  char	sph_minp, 	/* Minimum pri */
					sph_maxp,	/* Maximum pri */
					sph_defp,	/* Default pri */
					sph_cps; 	/* Copies */
	unsigned  long	sph_flgs;	/* Privileges */
	unsigned  long	sph_class;	/* Class code */
#else
	char			sph_form[MAXFORM+1];/* Form type (35 bytes) */
	time_t			sph_lastp;	/* Last read password file */
	unsigned  char	sph_minp,	/* Minimum pri */
					sph_maxp,	/* Maximum pri */
					sph_defp,	/* Default pri */
					sph_cps;	/* Copies */

	unsigned  long	sph_flgs;	/* Privileges */
	classcode_t		sph_class;	/* Class code */
	char			sph_formallow[ALLOWFORMSIZE+1]; /* Allowed form types (pattern) */
	char			sph_ptr[PTRNAMESIZE+1];	/* Default printer */
	char			sph_ptrallow[JPTRNAMESIZE+1]; /* Allow printer types (pattern) */
#endif
};

#endif	/* unix */

struct	spdet	{
#if		XITEXT_VN < 22
	unsigned  char	spu_isvalid;		/* Valid user id */
	char			spu_resvd1[3];		/* padding to 4 bytes */
	long			spu_user;			/* User id */
	unsigned  char	spu_minp, 			/* Minimum priority  */
					spu_maxp,			/* Maximum priority  */
					spu_defp;			/* Default priority  */
	char			spu_form[MAXFORM+1];/* Default form  */
	unsigned  short	spu_flgs;			/* Privilege flag  */
	unsigned  short spu_class;			/* Class of printers */
	unsigned  char	spu_cps,			/* Copies */
					spu_resvd;
	long			spu_charge;			/* Current bill  */
#ifdef	__cplusplus
	unsigned  short	resultclass(const unsigned short rclass) const
	{
		return  (spu_flgs & PV_COVER) ? rclass: rclass & spu_class;
	}
	int  ispriv(const unsigned short flag) const
	{
		return  flag & spu_flgs? 1: 0;
	}
#endif
#elif	XITEXT_VN < 23
	unsigned  char	spu_isvalid;			/* Valid user id */
	char			spu_form[MAXFORM+1];	/* Default form (19 bytes) */
	int_ugid_t		spu_user;				/* User id */
	unsigned  char	spu_minp,			 	/* Minimum priority  */
					spu_maxp,				/* Maximum priority  */
					spu_defp,				/* Default priority  */
					spu_cps;				/* Copies */
	
	unsigned  long	spu_flgs;				/* Privilege flag  */
	unsigned  long	spu_class;				/* Class of printers */
#ifdef	__cplusplus
	unsigned  long	resultclass(const unsigned long rclass) const
	{
		return  (spu_flgs & PV_COVER) ? rclass: rclass & spu_class;
	}
	int  ispriv(const unsigned long flag) const
	{
		return  flag & spu_flgs? 1: 0;
	}
#endif
#else	/*  Rel 23 */
	unsigned  char	spu_isvalid;			/* Valid user id */

	char			spu_form[MAXFORM+1];	/* Default form (35 bytes) */

	int_ugid_t		spu_user;				/* User id */

	unsigned  char	spu_minp,				/* Minimum priority  */
					spu_maxp,				/* Maximum priority  */
					spu_defp,				/* Default priority  */
					spu_cps;				/* Copies */

	unsigned  long	spu_flgs;				/* Privileges  */
	classcode_t		spu_class;				/* Class of printers */
	char			spu_formallow[ALLOWFORMSIZE+1]; /* Allowed form types (pattern) */
	char			spu_ptr[PTRNAMESIZE+1];	/* Default printer */
	char			spu_ptrallow[JPTRNAMESIZE+1]; /* Allow printer types (pattern) */
#ifdef	__cplusplus
	classcode_t	resultclass(const classcode_t rclass) const
	{
		return  (spu_flgs & PV_COVER) ? rclass: rclass & spu_class;
	}
	int  ispriv(const unsigned long flag) const
	{
		return  flag & spu_flgs? 1: 0;
	}
#endif
#endif /* REL23 */
};

#if		XITEXT_VN >= 22

/*
 *	Charge record generated by scheduler
 */

struct	spcharge	{
	time_t		spch_when;			/* When it happened */
	netid_t		spch_host;			/* Host responsible */
	int_ugid_t	spch_user;			/* Uid charged for */
	unsigned  short	spch_pri;		/* Priority */
	unsigned  short	spch_what;		/* Type of charge */
#define	SPCH_RECORD	0				/* Record left by spshed */
#define	SPCH_FEE	1				/* Impose fee */
#define	SPCH_FEEALL	2				/* Impose fee everywhere */
#define	SPCH_CONSOL	3				/* Consolidation of previous charges */
#define	SPCH_ZERO	4				/* Zero record for given user */
#define	SPCH_ZEROALL	5			/* Zero record for all users */
	long		spch_chars;			/* Chars printed */
	long		spch_cpc;			/* Charge per character or charge (FEE/CONSOL) */
};                                  
#endif
/*APIEND - end of section copied for API */

#ifdef	__cplusplus
#ifdef	WINVER
const	int	getspuser(spdet FAR &, CString &, const short = 0);
const	int	xt_enquire(CString &, CString &, CString &);
const	int	xt_login(CString &, CString &, const char *, CString &);
const	int	xt_logout();
char	FAR * FAR *gen_ulist(const char FAR *, const short = 0);
#else
const	spdet	&getspuser(void);
extern	char	Realuname[];
extern	spdet	mypriv;
#endif
#endif	/* C++ */
