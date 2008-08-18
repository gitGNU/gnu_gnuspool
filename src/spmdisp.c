/* spmdisp.c -- decide how to notify users on mail/write

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
#include "incl_sig.h"
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "notify.h"
#include "cfile.h"
#include "extdefs.h"
#ifdef	SHAREDLIBS
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"
#include "displayopt.h"
#endif

char	configname[] = USER_CONFIG;
char	*writer, *doswriter, *mailer, *shellname;
char	*spuname, *homedir;

int_ugid_t	spuid, spool_uid;

int	has_file = 0, past_tense = 0, pass_thru = 0;
int	repl_mail = 0, repl_write = 0, repl_doswrite = 0;
int	msg_code = $E{Job errors msg};
char	*orig_host;

FILE	*Cfile;

extern	char	*Helpfile_path;

/* Keep library happy.  */
#ifdef	SHAREDLIBS
uid_t	Realuid, Effuid, Daemuid;
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
DEF_DISPOPTS;
#endif

void	nomem(void)
{
	fprintf(stderr, "Out of memory...\n");
	exit(E_NOMEM);
}

/* Invoke mail or write command.  Don't use popen since there is a bug
   in it which makes funny things happen if file descriptors 0 or
   1 are closed.  */

void	rmsg(char *cmd)
{
	char	*ptrname, *jnum;
	FILE	*po;
	int	pfds[2];
	PIDTYPE	pid;

	if  (pipe(pfds) < 0  || (pid = fork()) < 0)
		return;

	if  (pid == 0)  {		/*  Child process  */
		char	*cp, **ap, *arglist[5];

		close(pfds[1]);	/*  Write side  */
		if  (pfds[0] != 0)  {
			close(0);
			dup(pfds[0]);
			close(pfds[0]);
		}

		ap = arglist;
		if  ((cp = strrchr(cmd, '/')))
			cp++;
		else
			cp = cmd;
		*ap++ = cp;
		if  (orig_host)
			*ap++ = orig_host;
		*ap++ = spuname;
		*ap = (char *) 0;

		execv(cmd, arglist);
		if  (errno == ENOEXEC)  {
			ap = arglist;
			*ap++ = cp;
			*ap++ = cmd;
			if  (orig_host)
				*ap++ = orig_host;
			*ap++ = spuname;
			*ap = (char *) 0;
			execv(shellname, arglist);
		}
		exit(255);
	}

	close(pfds[0]);			/*  Read side  */
	if  ((po = fdopen(pfds[1], "w")) == (FILE *) 0)  {
		kill(pid, SIGKILL);
		return;
	}

	if  (!(ptrname = getenv("SPOOLPTR"))  ||  ptrname[0] == '\0')
		ptrname = gprompt($P{No printer allocated});
	else  {
		char	*hostname;
		if  ((hostname = getenv("SPOOLHOST"))  &&  hostname[0])  {
			char  *resname;
			if  (!(resname = malloc((unsigned) (strlen(hostname) + strlen(ptrname) + 2))))
				nomem();
			sprintf(resname, "%s:%s", hostname, ptrname);
			ptrname = resname;
		}
	}

	/* Note that ptrname doesn't get deallocated.
	   No point, we are about to finish.  */

	disp_str = ptrname;
	if  (!(disp_str2 = getenv("SPOOLHDR")))
		disp_str2 = "";
	disp_arg[0] = (jnum = getenv("SPOOLJOB"))? atol(jnum): 0L;
	if  (!pass_thru)
		fprint_error(po, disp_str2[0]? msg_code: msg_code - $S{No title offset});
	if  (has_file || pass_thru)  {
		int  ch;
		while  ((ch = getchar()) != EOF)
			putc(ch, po);
	}
	fclose(po);
	exit(0);
}

FILE  *getmsgfile(void)
{
	FILE		*res;
	char		*libf, *homedf, *sysf, *repl;
	unsigned	hdlng, llng = 0, lng;

#ifdef	SHAREDLIBS
	Realuid = getuid();
	Effuid = geteuid();
#endif

	/* Get user name which should be passed to it in environment */

	if  (!(spuname = getenv("SPOOLJUNAME")))
		spuname = SPUNAME;

	/* If I don't know the user, just return the standard file.  */

	if  ((spuid = lookup_uname(spuname)) == UNKNOWN_UID)  {
		homedir = "/";
		return  open_icfile();
	}

	/* Get user's home directory */

	homedir = unameproc("~", "/", (uid_t) spuid);
	if  ((libf = getenv("LIBRARY")))
		llng = strlen(libf);
	hdlng = strlen(homedir) + 1;
	lng = hdlng + llng + sizeof(configname);

	/* Get hold of .gnuspool file in home directory for user.  */

	if  (!(homedf = malloc(lng)))
		nomem();
	strcpy(homedf, homedir);
	if  (libf)
		strcat(homedf, libf);
	strcat(homedf, "/");
	strcat(homedf, configname);

	/* Whilst we're there, grab alternate programs if
	   specified. Also reset uid to the relevant user.  */

	if  ((repl = rdoptfile(homedf, "MAILER")))  {
		mailer = repl;
		repl_mail++;
	}
	if  ((repl = rdoptfile(homedf, "WRITER")))  {
		writer = repl;
		repl_write++;
	}
	if  ((repl = rdoptfile(homedf, "DOSWRITER")))  {
		doswriter = repl;
		repl_doswrite++;
	}

	if  (!(sysf = rdoptfile(homedf, "SYSMESG")))  {
		free(homedf);
		return  open_icfile();
	}

	/* If not absolute, bring it down from the home directory.  */

	if  (sysf[0] != '/')  {
		char	*abssysf;
		lng = hdlng + llng + strlen(sysf) + 1;
		if  (!(abssysf = malloc(lng)))
			nomem();
		strcpy(abssysf, homedir);
		if  (libf)
			strcat(abssysf, libf);
		strcat(abssysf, "/");
		strcat(abssysf, sysf);
		free(sysf);	/* Not needed any more */
		sysf = abssysf;
	}

	free(homedf);

	if  (!(res = fopen(sysf, "r")))  {
		free(sysf);
		return  open_icfile();
	}

	Helpfile_path = sysf;
	fcntl(fileno(res), F_SETFD, 1);
	return  res;
}

static char  *app_dir(char *nam)
{
	static	char	*basedir;
	char	*result;
	if  (!nam || nam[0] == '/')
		return  nam;
	if  (!basedir)
		basedir = envprocess(IPROGDIR);
	if  (!(result = malloc((unsigned) (strlen(basedir) + strlen(nam) + 2))))
		nomem();
	sprintf(result, "%s/%s", basedir, nam);
	return  result;
}

MAINFN_TYPE	main(int argc, char **argv)
{
	cmd_type	cmd = NOTIFY_MAIL;
	int		extnum = 0;
	char		**ep, **lep;
	extern	char	**environ;
	static	char	lnam[] = "LOGNAME=";

	versionprint(argv, "$Revision: 1.1 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	/* Initialise default message despatch */

	shellname = envprocess(SHELL);
	writer = envprocess(WRITER);
	doswriter = envprocess(DOSWRITER);
	mailer = envprocess(MAILER);

	if  ((spool_uid = lookup_uname(SPUNAME)) == UNKNOWN_UID)
		spool_uid = ROOTID;

	/* Get message file.
	   Side effect is to set "spuname" to SPOOLJUNAME */

	if  (!(Cfile = getmsgfile()))
		exit(E_SETUP);

	/* Now decode arguments.

		-m	Mail style
		-w	Write style
		-d	Dos write
		-f	Error file on std input
		-p	Past tense
		-x	Pass through (no message code)
		-e n	External n

		Follow by message code (integer) and possible host name.
		Any other arguments are read from environment.
		We don't check the arguments very well.		*/

	while  (argv[1])  {
		char	*arg = argv[1];
		if  (*arg++ != '-')
			break;
		argv++;
		while  (*arg)
			switch  (*arg++)  {
			default:
				continue;
			case  'm':case  'M':
				cmd = NOTIFY_MAIL;
				continue;
			case  'w':case  'W':
				cmd = NOTIFY_WRITE;
				continue;
			case  'd':case  'D':
				cmd = NOTIFY_DOSWRITE;
				continue;
			case  'f':case  'F':
				has_file = 1;
				continue;
			case  'p':case  'P':
				past_tense = 1;
				continue;
			case  'x':case  'X':
				pass_thru = 1;
				continue;
			case  'e':case  'E':
				if  (*arg)
					extnum = atoi(arg);
				else  {
					if  (!argv[1])
						exit(E_USAGE);
					extnum = atoi(*++argv);
				}
				goto  ex;
			}
	ex:
		;
	}

	if  (argv[1])  {
		msg_code = atoi(argv[1]);
		if  (argv[2])
			orig_host = argv[2];
	}

	/* And now a whole load of nonsense to delete LOGNAME from the
	   environment. For some idiotic reason mail uses this
	   rather than the uid, and as a result will pick up some
	   fossil LOGNAME which happens to be lying around.  */

	for  (ep = environ;  *ep;  ep++)  {
		if  (strncmp(*ep, lnam, sizeof(lnam) - 1) == 0)  {
			for  (lep = ep + 1;  *lep;  lep++)
				;
			*ep = *--lep;
			*lep = (char *) 0;
			break;
		}
	}

	if  (extnum != 0)  {
		char	*mailp, *wrtp;
		mailp = app_dir(ext_mail(extnum));
		wrtp = app_dir(ext_wrt(extnum));
		setuid(spool_uid);
		switch  (cmd)  {
		case  NOTIFY_MAIL:
			if  (mailp)
				rmsg(mailp);
			break;
		case  NOTIFY_WRITE:
		case  NOTIFY_DOSWRITE:
			if  (wrtp)
				rmsg(wrtp);
			break;
		}
	}
	else  switch  (cmd)  {
	case  NOTIFY_MAIL:
		setuid((uid_t) repl_mail? spuid: spool_uid);
		rmsg(mailer);
		break;
	case  NOTIFY_WRITE:
		setuid((uid_t) repl_write? spuid: spool_uid);
		rmsg(writer);
		break;
	case  NOTIFY_DOSWRITE:
		setuid((uid_t) repl_doswrite? spuid: spool_uid);
		rmsg(doswriter);
		break;
	}

	return  E_SETUP;		/* Actually only reached if buggy*/
}
