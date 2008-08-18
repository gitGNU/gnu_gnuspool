/*
 * Copyright (c) Xi Software Ltd. 1992.
 *
 * errnums.h: created by John Collins on Sat Oct 17 1992.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/ERRNUMS.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: ERRNUMS.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 */

#define	ERR_SPR_EXPLAIN		1
#define	ERR_SPQ_EXPLAIN		2
#define	ERR_SPUSER_EXPLAIN	3
#define	ERR_SPST_EXPLAIN	4
#define	ERR_SPL_EXPLAIN		5
#define	ERR_SQL_EXPLAIN		6
#define	ERR_SQDEL_EXPLAIN	7
#define	ERR_SQCHG_EXPLAIN	8
#define	ERR_SPC_EXPLAIN		9
#define	ERR_SPRSET_EXPLAIN	10
#define	ERR_SPQSET_EXPLAIN	11

#define	ERR_GSPU_NOENQ		4020
#define	ERR_GSPU_NOSERV		4021

#define	LIB_USAGE	9000		// Bad argument
#define	LIB_AUSAGE	9001		// Bad argument - string
#define	LIB_MISSARG	9002		// Missing argument

#define	LIB_ARGCHCLASH	10007		// Argument clash char
#define	LIB_ARGCLASH	10008		// Argument clash string
#define	LIB_ARGERRS	10009		// Aborted due to arg errors

class	errparams	{
 public:
	char	*progname;		// Program name
	char	*disp_str;		// First error string printed by %s
	char	*disp_str2;		// Second error string printed by %t
	long	disp_arg[10];		// Numeric arguments %d0 to %d9
	int	errno;			// Saved error number
};

extern	void	getpname(const char **);
extern	void	print_error(const int);
extern	void	count_hv(char **, int &, int &);
extern	void	fprint_error(ostream_withassign &, const int);
extern	void	freehelp(char **);
extern	void	nomem(void);
extern	char	**helpvec(const short, const char);
extern	char	**helphdr(const char);

extern	errparams	Errblock;	// Defined in main module
