/* spr.c -- submit spool jobs

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
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "pages.h"
#include "files.h"
#include "helpargs.h"
#include "errnums.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "xfershm.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "extdefs.h"
#ifdef	SHAREDLIBS
#include "q_shm.h"
#include "displayopt.h"
#endif

#ifndef	ROOTID
#define	ROOTID	0
#endif

#define	IPC_MODE	0600
#define	C_MASK	0177		/*  Umask value  */
#define	MAXLONG	0x7fffffffL	/*  Change this?  */

#define	SECSPERDAY	(24 * 60 * 60L)

extern	char	freeze_wanted;
char	freeze_cd, freeze_hd;
char	verbose, interpolate, nopagef;
jmp_buf	ajb;
#ifndef	ID_SWAP
PIDTYPE	lastpid = -1;
#endif
uid_t	Realuid, Daemuid, Effuid;
struct	spdet	*mypriv;
struct	spr_req	sp_req;
struct	spq	SPQ;
jobno_t	jobn;
netid_t		Out_host;	/* For re-routing to rspr */
struct	xfershm		*Xfer_shmp;

/* Default delimiter and length
   If these are unchanged we don't create a page file */

struct	pages	pfe = { 1, 1, 0 };
char	*delimiter = "\f";

int	jobtimeout = 0;

#define	JN_INC	80000		/*  Add this to job no if clashes */

FILE	*Cfile;

char	Sufchars[] = DEF_SUFCHARS;
char	wotform[MAXFORM+1];
int	wotl;

int	Ctrl_chan = -1;
#ifndef	USING_FLOCK
int	Sem_chan = -1;
#endif
char	*Curr_pwd, *spdir, *tmpfl, *pgfl;

#ifdef	SHAREDLIBS
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
DEF_DISPOPTS;
#endif

int	spitoption(const int, const int, FILE *, const int, const int);
int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));
char	*spath(const char *, const char *);

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

/* On a signal, remove file.  */

RETSIGTYPE	catchit(int n)
{
	unlink(tmpfl);
	unlink(pgfl);
	exit(E_SIGNAL);
}

/* On alarm signal...  */

RETSIGTYPE	acatch(int n)
{
#ifdef	HAVE_SIGACTION
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
   We keep ignoring the	signals we were ignoring, and sometimes we catch,
   sometimes we	ingore the rest.  */

static	char	sigstocatch[] =	{ SIGINT, SIGQUIT, SIGTERM, SIGHUP };
static	char	sigchkd;	/*  Worked out which ones once  */
static	char	sig_ignd[sizeof(sigstocatch)];	/*  Ignore these indices */

/* If we have got better signal handling, use it.
   What a hassle!!!!!  */

void	catchsigs(void)
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

static void	holdsigs(void)
{
	int	i;
#ifdef	HAVE_SIGACTION
	sigset_t	sset;
	sigemptyset(&sset);
	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		sigaddset(&sset, sigstocatch[i]);
	sigprocmask(SIG_BLOCK, &sset, (sigset_t *) 0);

#elif	defined(STRUCT_SIG)

	int	msk = 0;
	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		msk |= sigmask(sigstocatch[i]);
	sigsetmask(msk);

#elif	defined(HAVE_SIGSET)

	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		if  (!sig_ignd[i])
			sighold(sigstocatch[i]);

#else
	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		if  (!sig_ignd[i])
			signal(sigstocatch[i], SIG_IGN);
#endif
}

static void	releasesigs(void)
{
	int	i;
#ifdef	HAVE_SIGACTION
	sigset_t	sset;
	sigemptyset(&sset);
	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		sigaddset(&sset, sigstocatch[i]);
	sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);

#elif	defined(STRUCT_SIG)

	sigsetmask(0);

#elif	defined(HAVE_SIGSET)

	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		if  (!sig_ignd[i])
			sigrelse(sigstocatch[i]);

#else
	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		if  (!sig_ignd[i])
			signal(sigstocatch[i], catchit);
#endif
}

static void	default_sigs(void)
{
	int	i;

#ifdef	HAVE_SIGACTION
	struct	sigaction	ss;
	sigset_t	sset;
	ss.sa_handler = SIG_DFL;
	sigemptyset(&ss.sa_mask);
	ss.sa_flags = 0;
	sigemptyset(&sset);
	for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
		sigaddset(&sset, sigstocatch[i]);
		sigaction(sigstocatch[i], &ss, (struct sigaction *) 0);
	}
	sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);

#elif	defined(STRUCT_SIG)
	struct	sigstruct_name	ss;
	ss.sv_handler = SIG_DFL;
	ss.sv_mask = 0;
	ss.sv_flags = 0;
	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		sigact_routine(sigstocatch[i], &ss, (struct sigstruct_name *) 0);
	sigsetmask(0);

#else

	for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
		if  (!sig_ignd[i])
			signal(sigstocatch[i], SIG_DFL);
#ifdef	HAVE_SIGSET
		sigrelse(sigstocatch[i]);
#endif
	}
#endif
}

/* Generate output file name */

FILE *	goutfile(void)
{
	FILE	*res;
	int	fid;

	for  (;;)  {
		sprintf(tmpfl, "%s/%s", spdir, mkspid(SPNAM, jobn));
		if  ((fid = open(tmpfl, O_WRONLY|O_CREAT|O_EXCL, 0400)) >= 0)
			break;
		jobn += JN_INC;
	}
	catchsigs();

	if  ((res = fdopen(fid, "w")) == (FILE *) 0)  {
		unlink(tmpfl);
		nomem();
	}

	/* Generate name now, worry later */

	sprintf(pgfl, "%s/%s", spdir, mkspid(PFNAM, jobn));
	return  res;
}

/* Get input file.  */

FILE *	ginfile(const char * arg)
{
	FILE  *inf;

	/* If we are reading from the standard input, then all is ok.
	   Put out a message if it looks like the terminal, as
	   the silent wait for action might confuse someone who
	   has made a mistake.  */

	if  (arg == (char *) 0)  {
		struct  stat  sbuf;

		fstat(0, &sbuf);
		if  ((sbuf.st_mode & S_IFMT) == S_IFCHR)
			print_error($E{Expecting terminal input});
		return  stdin;
	}

#ifdef	HAVE_SETEUID
	seteuid(Realuid);
	inf = fopen(arg, "r");
	seteuid(Daemuid);
	if  (inf)
		return  inf;
#else  /* !HAVE_SETEUID */
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

	if  (inf)
		return  inf;
#else
	if  (Daemuid == ROOTID)  {
		if  ((Realuid != ROOTID  &&  access(arg, 04) < 0) || (inf = fopen(arg, "r")) == (FILE *) 0)
			goto  noopen;
		return   inf;
	}
	else  {
		FILE	*res;
		int  ch;
		int	pfile[2];

		/* Otherwise we fork off a process to read the file as
		   the files might not be readable by the spooler
		   effective userid, and it might be done as a
		   backdoor method of reading files only readable
		   by the spooler effective uid.  */

		if  (lastpid >= 0)  {	/* Clean up zombie from last time around */
#ifdef	HAVE_WAITPID
			while  (waitpid(lastpid, (int *) 0, 0) < 0  &&  errno == EINTR)
#else
			PIDTYPE	pid;
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
#endif

 noopen:
#endif /* !HAVE_SETEUID */
	disp_str = arg;
	print_error($E{Cannot open print file});
	return  (FILE *) 0;
}

/* Check for alarm and read input */

int	togetc(FILE * f)
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
   Neither do some Suns. Neither do lots and lots lets test for it.  */

static	int	fgetc(FILE * f)
{
	return	getc(f);
}
#endif

/* Write to output file watching for the end of each page.  Create
   page descriptor file if we aren't happy with the standard one
   If we have a timeout, return 1, otherwise 0.  */

int	copyout(FILE * inf, FILE * outf, const int fpipe)
{
	int	ch;
	char	*rcp;
	int	rec_cnt, pgfid = -1;
	LONG	onpage;
	LONG	plim = 0x7fffffffL;
	ULONG	klim = 0xffffffffL;
	char	*rcdend;
	int	(*infunc)() = fgetc;

	if  (fpipe  &&  jobtimeout)  {
		infunc = togetc;
		if  (setjmp(ajb))  {
			clearerr(inf);
			if  (pgfid >= 0  &&  close(pgfid) < 0)
				goto  ffull;
			if  (fclose(outf) != EOF)
				return  1;
			goto  ffull;
		}
	}

	if  (SPQ.spq_pglim)  {
		if  (SPQ.spq_dflags & SPQ_PGLIMIT)
			plim = SPQ.spq_pglim;
		else
			klim = (ULONG) SPQ.spq_pglim << 10;
	}

	SPQ.spq_npages = 0;
	onpage = 0;

	if  (pfe.deliml == 1  &&  pfe.delimnum == 1 &&  delimiter[0] == '\f')  {
		nopagef = 1;
		SPQ.spq_dflags &= SPQ_ERRLIMIT | SPQ_PGLIMIT; /* I didnt mean a ~ here */

		while  ((ch = (*infunc)(inf)) != EOF)  {
			if  ((ULONG) ++SPQ.spq_size > klim)  {
				if  (SPQ.spq_dflags & SPQ_ERRLIMIT)
					goto  toobig;
				print_error($E{Too big warning});
				SPQ.spq_size--;
				break;
			}
			onpage++;
			if  (putc(ch, outf) == EOF)
				goto  ffull;
			if  (ch == '\f')  {
				onpage = 0;
				if  (++SPQ.spq_npages > plim)  {
					if  (SPQ.spq_dflags & SPQ_ERRLIMIT)
						goto  toobig;
					print_error($E{Too big warning});
					break;
				}
			}
		}
		if  (onpage)
			SPQ.spq_npages++;
		fclose(inf);
		if  (fclose(outf) != EOF)
			return  0;
		goto  ffull;
	}

	/* Case where we do have a page file */

	nopagef = 0;
	SPQ.spq_dflags |= SPQ_PAGEFILE;

	if  ((pgfid = open(pgfl, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0)  {
		disp_str = pgfl;
		print_error($E{Cannot create page file});
		unlink(tmpfl);
		exit(E_IO);
	}
	pfe.lastpage = 0;	/* Fix this later perhaps */
	write(pgfid, (char *) &pfe, sizeof(pfe));
	write(pgfid, delimiter, (unsigned) pfe.deliml);

	rcp = delimiter;
	rcdend = delimiter + pfe.deliml;
	onpage = 0;
	rec_cnt = 0;

	while  ((ch = (*infunc)(inf)) != EOF)  {
		if  ((ULONG) ++SPQ.spq_size > klim)  {
			if  (SPQ.spq_dflags & SPQ_ERRLIMIT)
				goto  toobig;
			print_error($E{Too big warning});
			SPQ.spq_size--;
			break;
		}
		onpage++;
		if  (ch == *rcp)  {
			if  (++rcp >= rcdend)  {
				if  (++rec_cnt >= pfe.delimnum)  {
					if  (write(pgfid, (char *) &SPQ.spq_size, sizeof(LONG)) != sizeof(LONG))
						goto  ffull;
					onpage = 0;
					rec_cnt = 0;
					if  (++SPQ.spq_npages > plim)  {
						if  (SPQ.spq_dflags & SPQ_ERRLIMIT)
							goto  toobig;
						print_error($E{Too big warning});
						break;
					}
				}
				rcp = delimiter;
			}
		}
		else  if  (rcp > delimiter)  {
			char	*pp, *prcp, *prevpl;
			prevpl = --rcp;	/*  Last one matched  */
			for  (;  rcp > delimiter;  rcp--)  {
				if  (*rcp != ch)
					continue;
				pp = prevpl;
				prcp = rcp - 1;
				for  (;  prcp >= delimiter;  pp--, prcp--)
					if  (*pp != *prcp)
						goto  rej;
				rcp++;
				break;
			rej:	;
			}
		}
		if  (putc(ch, outf) == EOF)
			goto  ffull;
	}

	/* Store the offset of the end of the file */

	if  (write(pgfid, (char *) &SPQ.spq_size, sizeof(LONG)) != sizeof(LONG))
		goto  ffull;

	/* Remember how big the last page was */

	if  (onpage > 0)  {
		SPQ.spq_npages++;
		if  ((pfe.lastpage = pfe.delimnum - rec_cnt) > 0)  {
			lseek(pgfid, 0L, 0);
			write(pgfid, (char *) &pfe, sizeof(pfe));
		}
	}

	fclose(inf);
	if  (close(pgfid) < 0)
		goto  ffull;
	if  (fclose(outf) != EOF)
		return  0;

 ffull:
	print_error($E{Spool directory full});
	unlink(tmpfl);
	unlink(pgfl);
	exit(E_IO);
 toobig:
	print_error($E{Too big aborted});
	unlink(tmpfl);
	unlink(pgfl);
	exit(E_IO);
}

/* Enqueue request to spool scheduler.  */

void	enqueue(char * name)
{
	unsigned  sleeptime = 1;

	SPQ.spq_job = jobn;
	SPQ.spq_time = (LONG) time((time_t *) 0);

	holdsigs();
	while  (wjmsg(&sp_req, &SPQ) != 0)  {
		releasesigs();
		if  (errno != EAGAIN  ||  sleeptime >= 512)  {
			print_error(errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error});
			unlink(tmpfl);
			unlink(pgfl);
			exit(E_SETUP);
		}
		print_error($E{IPC msg q fill wait});
		sleep(sleeptime);
		holdsigs();
		sleeptime <<= 1;
	}

	default_sigs();

	if  (verbose)  {
		disp_str = name;
		disp_arg[0] = jobn;
		print_error(name? $E{Created with title}: $E{Created no title});
	}
}

#define	INLINE_SPR
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
	FILE  *inf, *outf;
	int	nohdr, exitcode = 0, ret;
	char		**origargv = argv;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif
		
	versionprint(argv, "$Revision: 1.1 $", 0);

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

	/* Set up file names.
	   These are a bit more dynamic than they used to be.
	   NB Change this if we increase spdir etc!!  */

	spdir = envprocess(SPDIR);
	if  ((tmpfl = (char *) malloc((unsigned)(strlen(spdir) + 2 + NAMESIZE + 4))) == (char *) 0)
		nomem();
	if  ((pgfl = (char *) malloc((unsigned)(strlen(spdir) + 2 + NAMESIZE + 4))) == (char *) 0)
		nomem();

	/* Get user control structure for current real user id.
	   Copy in default priority and set up default options.  */

	mypriv = getspuser(Realuid);
	strcpy(wotform, mypriv->spu_form);
	strcpy(SPQ.spq_ptr, mypriv->spu_ptr);
	SPQ.spq_pri = mypriv->spu_defp;
	SPQ.spq_class = mypriv->spu_class;
	SPQ.spq_uid = Realuid;
	strncpy(SPQ.spq_uname, prin_uname(Realuid), UIDSIZE);
	strcpy(SPQ.spq_puname, SPQ.spq_uname);
	SPQ.spq_cps = 1;
	SPQ.spq_end = MAXLONG - 1L;
	SPQ.spq_nptimeout = QNPTIMEOUT;
	SPQ.spq_ptimeout = QPTIMEOUT;

	sp_req.spr_mtype = MT_SCHED;
	sp_req.spr_un.j.spr_act = SJ_ENQ;
	jobn = sp_req.spr_un.j.spr_pid = getpid();
	SWAP_TO(Realuid);
	argv = optprocess(argv, Adefs, optprocs, $A{spr explain}, $A{spr freeze home}, 0);
	SWAP_TO(Daemuid);

	/* If sending to remote, invoke rspr with the same arguments
	   we started with not the ones constructed.  Try to do
	   it with a version of rspr parallel to the spr we
	   started with. By the way we don't let rspr re-invoke
	   this or we could loop forever and ever and ever and
	   ever and ever */

	if  (Out_host)  {
		char	*epath = origargv[0];
		char	*npath, *sp;

		/* Get current directory before we start messing around */

		if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
			Curr_pwd = runpwd();

		if  ((sp = strrchr(epath, '/')))  {
			int	rp = sp - epath + 1;
			if  (!(npath = malloc((unsigned) (strlen(epath) + 2))))
				nomem();
			strncpy(npath, epath, (unsigned) (sp - epath) + 1);
			npath[rp] = 'r';
			strcpy(&npath[rp+1], sp+1);
		}
		else  {
			if  (!(npath = malloc((unsigned) (strlen(epath) + 2))))
				nomem();
			npath[0] = 'r';
			strcpy(&npath[1], epath);
			npath = spath(npath, Curr_pwd);	/* Leak here but we're about to ditch it */
		}
		if  (!npath)  {
			print_error($E{Cannot find rspr});
			exit(E_USAGE);
		}
		execv(npath, origargv);
		print_error($E{Cannot run rspr});
		exit(E_SETUP);
	}

	/* Initialise form type now if not changing - perhaps suffix
	   already or nothing to pick up from */

	wotl = strlen(wotform);

	/* If form type contains suffix, or is too long to have one, forget it all.  */

	if  ((interpolate && strpbrk(wotform, Sufchars) != (char *) 0) || *argv == (char *) 0 || wotl >= MAXFORM - 2)
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
		/* If the guy is only allowed zero copies, don't punish him any more!  */
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

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		print_error($E{Spooler not running});
		exit(E_NOTRUN);
	}

#ifndef	USING_FLOCK
	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif
	if  ((ret = init_xfershm(0)))  {
		print_error(ret);
		exit(E_SETUP);
	}

	if  (argv[0] == (char *) 0)  {
		int	maybemore = 0, hadsome = 0;

		inf = ginfile((char *) 0);

		do   {
			outf = goutfile();

			maybemore = copyout(inf, outf, 1);

			if  (SPQ.spq_size <= 0)  {
				unlink(tmpfl);
				unlink(pgfl);
				if  (!hadsome)  {
					print_error($E{No standard input});
					exit(E_USAGE);
				}
				exit(0);
			}
			hadsome++;
#ifdef	NHONSUID
			if  (Daemuid != ROOTID  && (Realuid == ROOTID || Effuid == ROOTID))  {
				chown(tmpfl, Daemuid, getgid());
				if  (!nopagef)
					chown(pgfl, Daemuid, getgid());
		}
#else
#if	!defined(HAVE_SETEUID) && defined(ID_SWAP)
			if  (Daemuid != ROOTID  &&  Realuid == ROOTID)  {
				chown(tmpfl, Daemuid, getgid());
				if  (!nopagef)
					chown(pgfl, Daemuid, getgid());
			}
#endif
#endif

			/* Now enqueue request to spool scheduler.  */

			enqueue((char *) 0);
			SPQ.spq_size = 0;
		}  while  (maybemore);
		exit(0);
	}

	do  {
		char	*name;

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

		/* Open the spool output file and input files.  */

		if  ((inf = ginfile(*argv)) == (FILE *) 0)  {
			exitcode = E_FALSE;
			continue;
		}
		outf = goutfile();

		copyout(inf, outf, 0);
		if  (SPQ.spq_size <= 0)  {
			disp_str = *argv;
			print_error($E{No spool input});
			unlink(tmpfl);
			unlink(pgfl);
			exitcode = E_FALSE;
			continue;
		}

#ifdef	NHONSUID
		if  (Daemuid != ROOTID  &&  (Realuid == ROOTID || Effuid == ROOTID))  {
			chown(tmpfl, Daemuid, getgid());
			if  (!nopagef)
				chown(pgfl, Daemuid, getgid());
		}
#else
#if	!defined(HAVE_SETEUID) && defined(ID_SWAP)
		if  (Daemuid != ROOTID  &&  Realuid == ROOTID)  {
			chown(tmpfl, Daemuid, getgid());
			if  (!nopagef)
				chown(pgfl, Daemuid, getgid());
		}
#endif
#endif

		/* Now enqueue request to spool scheduler.  */

		enqueue(*argv);
		jobn = (jobn + 1) % JN_INC;
		SPQ.spq_size = 0;
	}  while  (*++argv != (char *) 0);

	return  exitcode;
}
