/* Spuser.h -- user permissions

   Copyright 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
