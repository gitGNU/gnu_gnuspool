/* mspr.c -- run spr if scheduler running otherwise save in alternate spool

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
#include <limits.h>
#include "defaults.h"
#include "files.h"
#include "helpargs.h"
#include "errnums.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "extdefs.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"
#include "displayopt.h"

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

#define	SEQUENCE_FILE	".seq"
#define	CMDFILENAME	"CMDF"
#define	SPOOLFILENAME	"ASPF"

#define	IPC_MODE	0600
#define	C_MASK	0077		/*  Umask value  */

jmp_buf	ajb;
#ifndef	ID_SWAP
PIDTYPE	lastpid = -1;
#endif

int	jobtimeout = 0;
long	count_chars;

FILE	*Cfile;
char	*Curr_pwd;

char	**file_list;
int	filenums = 0;

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
DEF_DISPOPTS;

char    *spath(const char *, const char *);
int	spitoption(const int, const int, FILE *, const int, const int);

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

static int	getseq(char * spdir)
{
	int	fd, result;
	char	buf[100];
	char	path[PATH_MAX];

	sprintf(path, "%s/%s", spdir, SEQUENCE_FILE);

	if  ((fd = open(path, O_RDWR)) < 0)  {
		if  ((fd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0)  {
			print_error($E{Cannot create sequence file});
			exit(E_SETUP);
		}
		result = 1;
	}
	else  {
		int	inb = read(fd, buf, sizeof(buf));
		if  (inb <= 1)
			result = 1;
		result = atoi(buf) + 1;
		lseek(fd, 0L, 0);
	}
	sprintf(buf, "%d\n", result);
	if  (write(fd, buf, strlen(buf)) < 0)  {
		print_error($E{Cannot create sequence file});
		exit(E_SETUP);
	}
	close(fd);
	return  result;
}

static void	zapfiles(void)
{
	while  (--filenums >= 0)
		unlink(file_list[filenums]);
}

/* On a signal, remove file.  */

RETSIGTYPE	catchit(int n)
{
	zapfiles();
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
   environment from which we are called.  We keep ignoring the
   signals we were ignoring, and sometimes we catch, sometimes we
   ingore the rest.  */

static	char	sigstocatch[] =	{ SIGINT, SIGQUIT, SIGTERM, SIGHUP };
static	char	sigchkd;	/*  Worked out which ones once  */
static	char	sig_ignd[sizeof(sigstocatch)];	/*  Ignore these indices */

/* If we have got better signal handling, use it.
   What a hassle!!!!! */

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

FILE *goutfile(char *spdir, char *name, const unsigned mode, char **fnamep)
{
	FILE	*res;
	int	fid;
	char	tmpfl[PATH_MAX];

	do	sprintf(tmpfl, "%s/%s%.8d", spdir, name, getseq(spdir));
	while  ((fid = open(tmpfl, O_WRONLY|O_CREAT|O_EXCL, mode)) < 0);

	filenums++;
	if  (file_list)
		file_list = (char **) realloc((char *) file_list, (unsigned) (filenums * sizeof(char *)));
	else
		file_list = (char **) malloc((unsigned) (filenums * sizeof(char *)));

	if  (!file_list)
		nomem();

	file_list[filenums-1] = stracpy(tmpfl);
	if  (fnamep)
		*fnamep = file_list[filenums-1];

#ifdef	HAVE_FCHOWN
#ifdef	NHONSUID
	if  (Daemuid != ROOTID  && (Realuid == ROOTID || Effuid == ROOTID))
		fchown(fid, Daemuid, getgid());
#else
#if	!defined(HAVE_SETEUID) && defined(ID_SWAP)
	if  (Daemuid != ROOTID  &&  Realuid == ROOTID)
		fchown(fid, Daemuid, getgid());
#endif /* ID_SWAP */
#endif /* !NHONSUID */
#else  /* !FCHOWN */
#ifdef	NHONSUID
	if  (Daemuid != ROOTID  && (Realuid == ROOTID || Effuid == ROOTID))
		chown(tmpfl, Daemuid, getgid());
#else
#if	!defined(HAVE_SETEUID) && defined(ID_SWAP)
	if  (Daemuid != ROOTID  &&  Realuid == ROOTID)
		chown(tmpfl, Daemuid, getgid());
#endif /* ID_SWAP */
#endif /* !NHONSUID */
#endif /* !FCHOWN */

	if  ((res = fdopen(fid, "w")) == (FILE *) 0)  {
		zapfiles();
		nomem();
	}
	return  res;
}

/* Get input file. */

FILE *	ginfile(const char * arg)
{
	FILE  *inf;

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
#else  /* ! HAVE_SETEUID */
#ifdef	ID_SWAP
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
		int	ch;
		int	pfile[2];

		if  (lastpid >= 0)  {
#ifdef	HAVE_WAITPID
			while  (waitpid(lastpid, (int *) 0, 0) < 0  &&  errno == EINTR)
				;
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
		close(pfile[0]);
		if  ((res = fdopen(pfile[1], "w")) == (FILE *) 0)
			exit(E_NOPIPE);
#ifdef	SETVBUF_REVERSED
		setvbuf(res, _IOFBF, (char *) 0, BUFSIZ);
#else
		setvbuf(res, (char *) 0, _IOFBF, BUFSIZ);
#endif
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
#endif /* Not ID_SWAP */
noopen:
#endif /* Not HAVE_SETEUID */
	disp_str = arg;
	print_error($E{Cannot open print file});
	return  (FILE *) 0;
}

/* Check for alarm and read input */

int	togetc(FILE * f)
{
	if  (count_chars > 0L)  {
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
static	int	fgetc(FILE * f)
{
	return	getc(f);
}
#endif

/* Write to output file.
   If we have a timeout, return 1, otherwise 0. */

int	copyout(FILE * inf, FILE * outf, const int fpipe)
{
	int	ch;
	int	(*infunc)() = fgetc;

	if  (fpipe  &&  jobtimeout)  {
		infunc = togetc;
		if  (setjmp(ajb))  {
			clearerr(inf);
			if  (fclose(outf) != EOF)
				return  1;
			goto  ffull;
		}
	}

	while  ((ch = (*infunc)(inf)) != EOF)  {
		count_chars++;
		if  (putc(ch, outf) == EOF)
			goto  ffull;
	}
	fclose(inf);
	if  (fclose(outf) != EOF)
		return  0;
 ffull:
	print_error($E{Spool directory full});
	zapfiles();
	exit(E_IO);
}

OPTION(o_ret0)
{
	return  OPTRESULT_OK;
}
OPTION(o_ret1)
{
	return  OPTRESULT_ARG_OK;
}

#define	o_explain	o_ret0
#define	o_hdrs		o_ret0
#define	o_nohdrs	o_ret0
#define	o_retn		o_ret0
#define	o_noretn	o_ret0
#define	o_localonly	o_ret0
#define	o_nolocalonly	o_ret0
#define	o_wrt		o_ret0
#define	o_mail		o_ret0
#define	o_mattn		o_ret0
#define	o_wattn		o_ret0
#define	o_nomailwrt	o_ret0
#define	o_noattn	o_ret0
#define	o_interp	o_ret0
#define	o_nointerp	o_ret0
#define	o_verbose	o_ret0
#define	o_noverbose	o_ret0
#define	o_togverbose	o_ret0
#define	o_copies	o_ret1
#define	o_priority	o_ret1
#define	o_ptimeout	o_ret1
#define	o_nptimeout	o_ret1
#define	o_formtype	o_ret1
#define	o_header	o_ret1
#define	o_flags		o_ret1
#define	o_printer	o_ret1
#define	o_user		o_ret1
#define	o_range		o_ret1
#define	o_tdelay	o_ret1
#define	o_dtime		o_ret1
#define	o_oddeven	o_ret1
#define	o_delimnum	o_ret1
#define	o_delim		o_ret1
#define	o_pagelimit	o_ret1
#define	o_freezecd	o_ret0
#define	o_freezehd	o_ret0
#define	o_setclass	o_ret1
#define	o_queuehost	o_ret1
#define	o_orighost	o_ret1
#define	o_queueuser	o_ret1
#define	o_external	o_ret1

/* This is actually implemented in case we need to use it.  */

OPTION(o_jobwait)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	jobtimeout = atoi(arg);
	return  OPTRESULT_ARG_OK;
}

#include "inline/spr_adefs.c"
#include "inline/spr_oproc.c"

static void	rerun(const char * pname, char **argv)
{
	char	*epath = argv[0];
	char	*npath, *sp;

	/* If 0th arg has a path name in, then use that as the directory to work from.  */

	if  ((sp = strrchr(epath, '/')))  {
		int	rp = sp - epath + 1;
		if  (!(npath = malloc((unsigned) (rp + strlen(pname) + 1))))
			nomem();
		strncpy(npath, epath, (unsigned) rp);
		strcpy(&npath[rp], pname);
	}
	else
		npath = spath(pname, Curr_pwd);

	if  (!npath)  {
		disp_str = pname;
		print_error($E{Cannot find program});
		exit(E_SETUP);
	}

	execv(npath, argv);
	disp_str = pname;
	print_error($E{Cannot run program});
	exit(E_SETUP);
}

static void	cwriteenv(FILE *outf, char *envn)
{
	char	*ep = getenv(envn);
	if  (ep)
		fprintf(outf, "%s=\'%s\'\nexport %s\n", envn, ep, envn);
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	FILE  *inf, *outf, *cmdfile;
	int	exitcode = 0, is_rspr = 0, hadsome = 0;
	char	**origargv = argv, **nargv;
	char	*Varname = "GSPL_PR";
	char	*spooldirs, *spdir, *outcmd;
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

	/*
		Get current directory before we start messing around
	 */

	if  (!(Curr_pwd = getenv("PWD")))
		Curr_pwd = runpwd();
	catchsigs();

	/**************************************************************
		CHECK WHETHER WE ARE RUNNING.
	 **************************************************************

		Decide which we are running the mpr or mrpr version
		Mind you, spr might go on to run rspr....	*/

	if  (strcmp(progname, "gspl-mrpr") == 0)  {
		PIDTYPE	pid;
		int	status;
		Varname = "GSPL_RPR";
		is_rspr++;

		/* Test rspr is running by trying to run it with the same arguments.  */

		if  ((pid = fork()) == 0)
			rerun("gspl-rpr", argv); /* Doesn't return */
		if  (pid < 0)  {
			print_error($E{Cannot fork});
			exit(E_NOFORK);
		}
#ifdef	HAVE_WAITPID
		while  (waitpid(pid, &status, 0) < 0)
			;
#else
		while  (wait(&status) != pid)
			;
#endif
		if  ((status & 0xff)  ||  ((status >> 8) & 0xff) == E_SETUP)  {
			print_error($E{Cannot run rspr});
			exit(E_SETUP);
		}
		if  (((status >> 8) & 0xff) != E_NETERR)	/* Must have run */
			exit(status >> 8);	/* Whatever rspr said */
	}
	else	if  (msgget(MSGID, 0) >= 0)  {
		/* Easy case, it's running, just hand over to spr and beat it. */
		rerun("gspl-pr", argv);
	}

	/**************************************************************
	 Not running case, so store up for future reference */

	Save_umask = umask(C_MASK);

	/* We go through the motions of looking at the arguments,
	   but we don't actually do anything with them.
	   We need to do this in order to skip over any options
	   and get to the file names (if any). */

	argv = optprocess(argv, Adefs, optprocs, $A{spr explain}, $A{spr freeze home}, 1);
	SWAP_TO(Daemuid);

	/* Work out what alternative spool directory we want to use */

	spdir = spooldirs = envprocess(ALTSPOOL);

	/* Find directory on path.
	   Maybe upgrade this sometime to find the emptiest?  */

	for  (;;)  {
		char	*colp;
		struct	stat	sbuf;

		if  ((colp = strchr(spdir, ':')))
			*colp = '\0';
		if  (spdir[0] == '/'  &&
		     stat(spdir, &sbuf) >= 0  &&
		     (sbuf.st_mode & S_IFMT) == S_IFDIR  &&
		     (sbuf.st_mode & 0700) == 0700  &&
		     (sbuf.st_uid == Daemuid))
			break;
		if  (colp)  {
			*colp = ':';
			spdir = colp + 1;
			break;
		}
		disp_str = spooldirs;
		print_error($E{Unknown alternative spool directories});
		exit(E_NOCHDIR);
	}

	/* OK, now have desired spool directory in spdir.
	   Make file to hold command, which we build up as we go */

	cmdfile = goutfile(spdir, CMDFILENAME, 0700, (char **) 0);
	fprintf(cmdfile, "#! /bin/sh\n#\n#Command file generated by %s\n\n", progname);

	/* Generate all the commands to get back to where we were
	   We need to be in the current directory in order to pick up
	   the correct .xitext files if applicable.
	   I'm not going to bother with the environment.
	   I hope that is a good idea. */

	fprintf(cmdfile, "cd %s\n", Curr_pwd);
	cwriteenv(cmdfile, Varname);
	cwriteenv(cmdfile, MISC_UCONFIG);

	/* Now generate the spr or rspr command. */

	disp_str = is_rspr? "gspl-rpr" : "gspl-pr";
	outcmd = spath(disp_str, Curr_pwd);
	if  (!outcmd)  {
		print_error($E{Cannot find program});
		exit(E_SETUP);
	}

	fprintf(cmdfile, "\n%s", outcmd);
	free(outcmd);

	/* Reproduce all the - (or +name) arguments (apart from the first) */

	for  (nargv = origargv + 1;  *nargv  &&  nargv != argv;  nargv++)  {
		if  (strpbrk(*nargv, " \t"))
			fprintf(cmdfile, " \'%s\'", *nargv);
		else
			fprintf(cmdfile, " %s", *nargv);
	}

	/* Now force the submitting user to be whoever we are running as. */

	if  (Daemuid != Realuid)  {
		spitoption($A{spr originating user}, $A{spr explain}, cmdfile, ' ', 0);
		fprintf(cmdfile, " %s", prin_uname(Realuid));
	}

	if  (argv[0] == (char *) 0)  {
		int	maybemore = 0;

		/* Case where we are taking from standard input.  */

		inf = ginfile((char *) 0);

		do   {
			char	*resf;
			outf = goutfile(spdir, SPOOLFILENAME, 0400, &resf);
			maybemore = copyout(inf, outf, 1);
			if  (count_chars <= 0)  {
				if  (!hadsome)  {
					zapfiles();
					print_error($E{No standard input});
					exit(E_USAGE);
				}
				unlink(resf);
				exit(0);
			}
			hadsome++;

			/* Now put file name on the end.  */

			fprintf(cmdfile, "\\\n\t%s", resf);
			count_chars = 0;
		}  while  (maybemore);
	}
	else  {

		/* File args cases. */

		do  {
			char	*resf;

			if  ((inf = ginfile(*argv)) == (FILE *) 0)  {
				exitcode = E_FALSE;
				continue;
			}
			outf = goutfile(spdir, SPOOLFILENAME, 0400, &resf);
			copyout(inf, outf, 0);
			if  (count_chars <= 0)  {
				disp_str = *argv;
				print_error($E{No spool input});
				unlink(resf);
				exitcode = E_FALSE;
				continue;
			}
			hadsome++;
			fprintf(cmdfile, "\\\n\t%s", resf);
			count_chars = 0;
		}  while  (*++argv != (char *) 0);
	}

	putc('\n', cmdfile);
	fclose(cmdfile);
	if  (!hadsome)
		zapfiles();
	return  exitcode;
}
