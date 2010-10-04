/* rspr.c -- remote queueing

   Copyright 2008 Free Software Foundation, Inc.

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

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <errno.h>
#include "incl_sig.h"
#include <sys/types.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "pages.h"
#include "files.h"
#include "helpargs.h"
#include "errnums.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xfershm.h"
#include "q_shm.h"

#ifndef	P_tmpdir
#define	P_tmpdir	"/tmp/"
#endif
#ifndef	L_tmpnam
#define	L_tmpnam	25
#endif

#ifndef	ROOTID
#define	ROOTID	0
#endif

#define	C_MASK	0177		/*  Umask value  */
#define	MAXLONG	0x7fffffffL	/*  Change this?  */

#define	SECSPERDAY	(24 * 60 * 60L)

extern	char	freeze_wanted;
char	freeze_cd, freeze_hd;
char	verbose, interpolate;
jmp_buf	ajb;
#ifndef	ID_SWAP
PIDTYPE	lastpid = -1;
#endif
struct	spdet	*mypriv;
jobno_t	jobn;
netid_t		Out_host;

struct	spq	SPQ;
struct	pages	pfe = { 1, 1, 0 };
char	*delimiter = "\f";

int	jobtimeout = 0;

#define	JN_INC	80000		/*  Add this to job no if clashes */

char	Sufchars[] = DEF_SUFCHARS;
char	wotform[MAXFORM+1];
int	wotl;

char	*Curr_pwd;

char	tmpfl[L_tmpnam];

extern	int	spitoption(const int, const int, FILE *, const int, const int);
extern	int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));
extern	int	remenqueue(const netid_t, const struct spq *, const struct pages *, const char *, FILE *, jobno_t *);
extern	struct	spdet	*remgetspuser(const netid_t, char *);
extern	void	remgoodbye();

void	nomem()
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

/* On a signal, remove file.  */

RETSIGTYPE	catchit(int n)
{
	unlink(tmpfl);
	exit(E_SIGNAL);
}

/* On alarm signal...  */

RETSIGTYPE	acatch(int n)
{
#if	defined(HAVE_SIGACTION)
#ifndef	SA_NODEFER
	sigset_t	nset;
	sigemptyset(&nset);
	sigaddset(&nset, n);
	sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *) 0);
#endif
#elif	defined(STRUCT_SIG)
	sigsetmask(sigsetmask(~0) & ~sigmask(n));
#elif	defined(HAVE_SIGSET)
	sigrelse(n);
#elif	defined(UNSAFE_SIGNALS)
	signal(n, acatch);
#endif
	longjmp(ajb, 1);
}

/* The following stuff is to try to keep consistency with the
   environment from which we are called.
   We keep ignoring the signals we were ignoring,
   and sometimes we catch, sometimes we ingore the rest. */

static	char	sigstocatch[] =	{ SIGINT, SIGQUIT, SIGTERM, SIGHUP };
static	char	sigchkd;	/*  Worked out which ones once  */
static	char	sig_ignd[sizeof(sigstocatch)];	/*  Ignore these indices */

/* If we have got better signal handling, use it.
   What a hassle!!!!! */

void	catchsigs()
{
	int	i;
#ifdef	STRUCT_SIG
	struct	sigstruct_name  z;

	z.sighandler_el = catchit;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG;

	if  (sigchkd)  {
		for  (i = 0;  i < sizeof(sigstocatch);  i++)
			if  (!sig_ignd[i])
				sigact_routine(sigstocatch[i], &z, (struct sigstruct_name *) 0);
	}
	else  {
		sigchkd++;
		for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
			struct	sigstruct_name	oldsig;
			sigact_routine(sigstocatch[i], &z, &oldsig);
			if  (oldsig.sighandler_el == SIG_IGN)  {
				sigact_routine(sigstocatch[i], &oldsig, (struct sigstruct_name *) 0);
				sig_ignd[i] = 1;
			}
		}
	}
	z.sighandler_el = acatch;
	z.sigflags_el = SIGVEC_INTFLAG | SIGACT_INTSELF;
	sigact_routine(SIGALRM, &z, (struct sigstruct_name *) 0);
#else
	if  (sigchkd)  {
		for  (i = 0;  i < sizeof(sigstocatch);  i++)
			if  (!sig_ignd[i])
				signal(sigstocatch[i], catchit);
	}
	else  {
		sigchkd++;
		for  (i = 0;  i < sizeof(sigstocatch);  i++)
			if  (signal(sigstocatch[i], catchit) == SIG_IGN)  {
				signal(sigstocatch[i], SIG_IGN);
				sig_ignd[i] = 1;
			}
	}
	signal(SIGALRM, acatch);
#endif
}

/* Generate output temporary file name for the benefit of standard
   input where we need to copy to a file and count the size. */

FILE	*goutfile()
{
	FILE	*res;
	int	fid;

	for  (;;)  {
		sprintf(tmpfl, "%s%s%.8lu", P_tmpdir, SPNAM, (unsigned long) (jobn%100000000));
		if  ((fid = open(tmpfl, O_RDWR|O_CREAT|O_EXCL, 0400)) >= 0)
			break;
		jobn += JN_INC;
	}
	catchsigs();

	if  ((res = fdopen(fid, "w+")) == (FILE *) 0)  {
		unlink(tmpfl);
		nomem();
	}
	return  res;
}

/* Get input file, set its size in SPQ.spq_size */

FILE	*ginfile(const char *arg)
{
	FILE  *inf;
	struct	stat	sbuf;

#ifdef	HAVE_SETEUID
	seteuid(Realuid);
	inf = fopen(arg, "r");
	seteuid(Daemuid);
	if  (inf)  {
		fstat(fileno(inf), &sbuf);
		if  (sbuf.st_size == 0)  {
			fclose(inf);
			disp_str = arg;
			print_error($E{No spool input});
			return  (FILE *) 0;
		}
		SPQ.spq_size = sbuf.st_size;
		return  inf;
	}
#else

#ifdef	ID_SWAP

	/* If we can shuffle between uids, revert to real uid to get
	   at file.  Use "access" call if we are sticking to root.  */

	if  (Realuid != ROOTID)  {
#if	defined(NHONSUID) || defined(DEBUG)
		if  (Daemuid != ROOTID  &&  Effuid != ROOTID)  {
			setuid(Realuid);
			inf = fopen(arg, "r");
			setuid(Daemuid);
		}
		else  {
			if  (access(arg, 04) < 0)
				goto  noopen;
			inf = fopen(arg, "r");
		}
#else
		if  (Daemuid != ROOTID)  {
			setuid(Realuid);
			inf = fopen(arg, "r");
			setuid(Daemuid);
		}
		else  {
			if  (access(arg, 04) < 0)
				goto  noopen;
			inf = fopen(arg, "r");
		}
#endif
	}
	else
		inf = fopen(arg, "r");

	if  (inf)  {
		fstat(fileno(inf), &sbuf);
		if  (sbuf.st_size == 0)  {
			fclose(inf);
			disp_str = arg;
			print_error($E{No spool input});
			return  (FILE *) 0;
		}
		SPQ.spq_size = sbuf.st_size;
		return  inf;
	}

#else  /* !ID_SWAP */

	if  (Daemuid == ROOTID)  {
		if  ((Realuid != ROOTID  &&  access(arg, 04) < 0) || (inf = fopen(arg, "r")) == (FILE *) 0)
			goto  noopen;
		fstat(fileno(inf), &sbuf);
		if  (sbuf.st_size == 0)  {
			fclose(inf);
			disp_str = arg;
			print_error($E{No spool input});
			return  (FILE *) 0;
		}
		SPQ.spq_size = sbuf.st_size;
		return  inf;
	}
	else  {
		FILE	*res;
		int	ch, pfile[2];

		if  (stat(arg, &sbuf) < 0)
			goto  noopen;

		if  (sbuf.st_size == 0)  {
			disp_str = arg;
			print_error($E{No spool input});
			return  (FILE *) 0;
		}
		SPQ.spq_size = sbuf.st_size;

		/* Otherwise we fork off a process to read the file as
		   the files might not be readable by the spooler
		   effective userid, and it might be done as a
		   backdoor method of reading files only readable
		   by the spooler effective uid.  */

		if  (lastpid >= 0)  {
#ifdef	HAVE_WAITPID
			while  (waitpid(lastpid, (int *) 0, 0) < 0  &&  errno == EINTR)
				;
#else
			PIDTYPE	pid;

			/* Clean up zombie from last time around */

			while  ((pid = wait((int *) 0)) != lastpid  &&  (pid >= 0 || errno == EINTR))
				;
#endif
		}

		if  (pipe(pfile) < 0)  {
			print_error($E{Cannot create pipe});
			exit(E_NOPIPE);
		}

		if  ((lastpid = fork()) < 0)  {
			print_error($E{Cannot fork});
			exit(E_NOFORK);
		}

		if  (lastpid != 0)  {	/*  Parent process  */
			close(pfile[1]);	/*  Write side of pipe  */
			res = fdopen(pfile[0], "r");
			if  (res == (FILE *) 0)  {
				print_error($E{Cannot create pipe});
				exit(E_NOPIPE);
			}
#ifdef	SETVBUF_REVERSED
			setvbuf(res, _IOFBF, (char *) 0, BUFSIZ);
#else
			setvbuf(res, (char *) 0, _IOFBF, BUFSIZ);
#endif
			return  res;
		}

		/* The remaining code is executed by the child process.  */

		close(pfile[0]);
		if  ((res = fdopen(pfile[1], "w")) == (FILE *) 0)
			exit(E_NOPIPE);
#ifdef	SETVBUF_REVERSED
		setvbuf(res, _IOFBF, (char *) 0, BUFSIZ);
#else
		setvbuf(res, (char *) 0, _IOFBF, BUFSIZ);
#endif

		/* Reset uid to real uid.  */

		setuid(Realuid);

		if  ((inf = fopen(arg, "r")) == (FILE *) 0)  {
			disp_str = arg;
			print_error($E{Cannot open print file});
		}
		else  {
			while  ((ch = getc(inf)) != EOF)
				putc(ch, res);
			fclose(inf);
		}
		fclose(res);
		exit(0);
	}
#endif /* !ID_SWAP */

  noopen:

#endif /* !HAVE_SETEUID */

	disp_str = arg;
	print_error($E{Cannot open print file});
	return  (FILE *) 0;
}

/* Check for alarm and read input */

int	togetc(FILE *f)
{
	if  (SPQ.spq_size > 0L)  {
		int	ch;
		alarm((unsigned) jobtimeout);
		ch = getc(f);
		alarm(0);
		return  ch;
	}
	else
		return  getc(f);
}

#ifndef	HAVE_FGETC

/* Yes it's true.... DYNIX doesn't have this function in stdio...
   Neither do some Suns. Neither do lots and lots lets test for it. */

static	int	fgetc(FILE *f)
{
	return	getc(f);
}
#endif

/* Write stdin to temporary output file watching for timeouts
   If we have a timeout, return 1, otherwise 0.  */

int	copyout(FILE *outf)
{
	int	ch;
	int	(*infunc)() = fgetc;

	if  (jobtimeout)  {
		infunc = togetc;
		if  (setjmp(ajb))  {
			clearerr(stdin);
			return  1;
		}
	}

	while  ((ch = (*infunc)(stdin)) != EOF)  {
		SPQ.spq_size++;
		if  (putc(ch, outf) == EOF)  {
			print_error($E{Trouble writing socket file});
			unlink(tmpfl);
			exit(E_IO);
		}
	}
	return  0;
}

#define	INLINE_RSPR
#include "inline/o_spr_expl.c"
#include "inline/o_hdrs.c"
#include "inline/o_retn.c"
#include "inline/o_loco.c"
#include "inline/o_mailwrt.c"
#include "inline/o_interp.c"
#include "inline/o_verbose.c"
#include "inline/o_copies.c"
#include "inline/o_priority.c"
#include "inline/o_timeout.c"
#include "inline/o_formtype.c"
#include "inline/o_header.c"
#include "inline/o_flags.c"
#include "inline/o_printer.c"
#include "inline/o_user.c"
#include "inline/o_range.c"
#include "inline/o_tdelay.c"
#include "inline/o_dtime.c"
#include "inline/o_oddeven.c"
#include "inline/o_delimnum.c"
#include "inline/o_delim.c"
#include "inline/o_jobwait.c"
#include "inline/o_freeze.c"
#include "inline/o_setclass.c"
#include "inline/o_qhost.c"
#include "inline/o_extern.c"

#include "inline/spr_adefs.c"
#include "inline/spr_oproc.c"
#include "inline/spr_dumpo.c"

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	int	nohdr, ret, exitcode = 0;
	char	*realuname;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.2 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];
	init_mcfile();

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	Save_umask = umask(C_MASK);

	/* Initialise some default stuff.
	   We check against privileges after we know what host we are talking to.  */

	SPQ.spq_uid = Realuid;
	realuname = prin_uname(Realuid);
	strncpy(SPQ.spq_uname, realuname, UIDSIZE);
	strcpy(SPQ.spq_puname, SPQ.spq_uname);
	SPQ.spq_cps = 1;
	SPQ.spq_end = MAXLONG - 1L;
	SPQ.spq_nptimeout = QNPTIMEOUT;
	SPQ.spq_ptimeout = QPTIMEOUT;
	jobn = SPQ.spq_job = getpid();

	/*
	   SPQ.spq_pri = 0;
	   SPQ.spq_class = 0;
	   Invalid values - if the options set these
	   then we'll know.
	   SPQ.spq_extrn = 0;
	   SPQ.spq_pglim = 0;
	*/

	SWAP_TO(Realuid);
	argv = optprocess(argv, Adefs, optprocs, $A{spr explain}, $A{spr freeze home}, 0);
	SWAP_TO(Daemuid);

	/* We must specify a host name */

	if  (Out_host == 0)  {
		print_error($E{No host name given});
		exit(E_USAGE);
	}

	/* Get user control structure for current real user id.
	   Copy in default priority and set up default options. */

	mypriv = remgetspuser(Out_host, realuname);
	if  (!SPQ.spq_ptr[0]  &&  mypriv->spu_ptr) /* If it didn't get set by options set to default */
		strcpy(SPQ.spq_ptr, mypriv->spu_ptr);
	if  ((wotl = strlen(wotform)) == 0)  {
		strcpy(wotform, mypriv->spu_form);
		wotl = strlen(wotform);
	}
	if  (SPQ.spq_pri == 0)
		SPQ.spq_pri = mypriv->spu_defp;
	if  (SPQ.spq_class == 0)
		SPQ.spq_class = mypriv->spu_class;
	else  if  (!(mypriv->spu_flgs & PV_COVER))  {
		classcode_t  num = SPQ.spq_class & mypriv->spu_class;
		if  (num == 0)  {
			disp_str = stracpy(hex_disp(num, 0));
			disp_str2 = hex_disp(mypriv->spu_class, 0);
			print_error($E{setting zero class});
			exit(E_BADCLASS);
		}
		SPQ.spq_class = num;
	}

	/* If form type contains suffix, or is too long to have one,
	   forget it all.  */

	if  ((interpolate  &&  strpbrk(wotform, Sufchars) != (char *) 0) ||
	     *argv == (char *) 0 || wotl >= MAXFORM - 2)
		interpolate = 0;

	if  (!interpolate)
		strcpy(SPQ.spq_form, wotform);

	/* Validate priority. Note that a privileged user can change
	   the priority later, but at the moment he is limited.  */

	if  (SPQ.spq_pri < mypriv->spu_minp || SPQ.spq_pri > mypriv->spu_maxp)  {
		disp_arg[0] = SPQ.spq_pri;
		disp_arg[1] = mypriv->spu_minp;
		disp_arg[2] = mypriv->spu_maxp;
		print_error(mypriv->spu_minp > mypriv->spu_maxp?
			    $E{Priority range excludes}:
			    SPQ.spq_pri == mypriv->spu_defp?
			    $E{No default priority}: $E{Invalid priority});
		exit(E_BADPRI);
	}
	if  (!((mypriv->spu_flgs & PV_FORMS)  ||  qmatch(mypriv->spu_formallow, wotform)))  {
		disp_str = wotform;
		print_error($E{Invalid form type});
		disp_str = mypriv->spu_formallow;
		print_error($E{Form type restriction});
		exit(E_BADFORM);
	}
	if  (!((mypriv->spu_flgs & PV_OTHERP)  ||  issubset(mypriv->spu_ptrallow, SPQ.spq_ptr)))  {
		disp_str = SPQ.spq_ptr;
		disp_str2 = mypriv->spu_ptrallow;
		print_error($E{Invalid ptr type});
		exit(E_BADPTR);
	}
	if  (SPQ.spq_cps > mypriv->spu_cps)  {
		/* If the guy is only allowed zero copies, don't
		   punish him any more!  */
		if  (mypriv->spu_cps == 0  &&  SPQ.spq_cps == 1)
			SPQ.spq_cps = 0;
		else  {
			disp_arg[0] = SPQ.spq_cps;
			disp_arg[1] = mypriv->spu_cps;
			print_error($E{Invalid copies});
			exit(E_BADCPS);
		}
	}

	nohdr = SPQ.spq_file[0] == '\0';

#define	FREEZE_EXIT
#include "inline/freezecode.c"

	/* Mark whether we've got a pagefile or not
	   Xtnetserv at the other end worries about the pages */

	if  (pfe.deliml == 1  &&  pfe.delimnum == 1 &&  delimiter[0] == '\f')
		SPQ.spq_dflags &= ~SPQ_PAGEFILE;
	else
		SPQ.spq_dflags |= SPQ_PAGEFILE;

	if  (argv[0] == (char *) 0)  {
		int	maybemore = 0, hadsome = 0;
		FILE	*outf;
		struct  stat  sbuf;

		/* Warn user if input looks like terminal */

		fstat(0, &sbuf);
		if  ((sbuf.st_mode & S_IFMT) == S_IFCHR)
			print_error($E{Expecting terminal input});

		do   {
			outf = goutfile(); /* This is just a temporary file */

			maybemore = copyout(outf);

			if  (SPQ.spq_size <= 0)  {
				fclose(outf);
				unlink(tmpfl);
				if  (!hadsome)  {
					print_error($E{No standard input});
					exit(E_USAGE);
				}
				exit(0);
			}
			hadsome++;
			rewind(outf);
			disp_str = SPQ.spq_file;
			if  ((ret = remenqueue(Out_host, &SPQ, &pfe, delimiter, outf, &jobn)) != 0)  {
				fclose(outf);
				unlink(tmpfl);
				print_error(ret);
				exit(ret == $E{remote job past limit}? E_TOOBIG: E_NETERR);
			}
			if  (verbose)  {
				disp_arg[0] = jobn;
				print_error(disp_str[0]? $E{Created with title}: $E{Created no title});
			}

			/* Do it again perhaps....
			   We ought to truncate the temporary file but not
			   all Unices (plural of Unix) let you.  */

			rewind(outf);
			SPQ.spq_size = 0;
			jobn = (jobn + 1) % JN_INC;
			SPQ.spq_job = jobn;
		}  while  (maybemore);
		fclose(outf);
		unlink(tmpfl);
		remgoodbye();
		exit(0);
	}

	/* Case where we have a list of files.
	   Splurge them out on one connection */

	do  {
		char	*name;
		FILE	*inf;

		/* Extract last part of name */

		if  ((name = strrchr(*argv, '/')) != (char *) 0)
			name++;
		else
			name = *argv;

		/* Stuff suffix on end if we want such */

		if  (interpolate)  {
			char	*suf;
			strcpy(SPQ.spq_form, wotform);
			if  ((suf = strpbrk(name, Sufchars)) != (char *) 0)  {
				char	*suf2;
				while  ((suf2 = strpbrk(suf+1, Sufchars)) != (char *) 0)
					suf = suf2;
				SPQ.spq_form[wotl] = *suf;
				SPQ.spq_form[wotl+1] = '\0';
				strncat(SPQ.spq_form, suf+1, MAXFORM-1-wotl);
				SPQ.spq_form[MAXFORM] = '\0';
			}
		}

		if  (nohdr)  {
			strncpy(SPQ.spq_file, name, MAXTITLE);
			SPQ.spq_file[MAXTITLE] = '\0';
		}
		if  ((inf = ginfile(*argv)) == (FILE *) 0)  {
			exitcode = E_FALSE;
			continue;
		}
		disp_str = SPQ.spq_file;
		if  ((ret = remenqueue(Out_host, &SPQ, &pfe, delimiter, inf, &jobn)) != 0)  {
			print_error(ret);
			if  (ret == $E{remote job past limit})  {
				exitcode = E_TOOBIG;
				fclose(inf);
				goto  skipbig;
			}
			exit(E_NETERR);
		}
		fclose(inf);
		if  (verbose)  {
			disp_arg[0] = jobn;
			print_error($E{Created with title});
		}
	    skipbig:
		jobn = (jobn + 1) % JN_INC;
		SPQ.spq_job = jobn;
	}  while  (*++argv != (char *) 0);

	remgoodbye();
	return  exitcode;
}
