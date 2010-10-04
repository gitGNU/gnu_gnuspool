/* sp.lpq.c -- user level program to get / remove LPD queue jobs

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
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <sys/shm.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#ifdef	HAVE_TERMIO_H
#include <termio.h>
#else
#include <sgtty.h>
#endif
#include "initp.h"
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"
#include "xfershm.h"

#define	LOCALHOST_NAME	"localhost"

int  spitoption(const int, const int, FILE *, const int, const int);
int  proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

extern	char	freeze_wanted;
char	freeze_cd, freeze_hd;
#ifndef SP_LPRM
int	longlist = 0;
#endif
char	*spechost;
char	*Curr_pwd;

#define	IPC_MODE	0600

struct	spdet	*mypriv;

/* When we poke through xtlpc stuff, we extract ptr names etc
   from the files and return herein. */

struct	prin_params  {
	char	*hostname;	/* Usually the local host but sometimes different */
	char	*ptrname;	/* Usually the supplied ptr name but can change */
	char	*formname;	/* Doesn't change */
	char	**other;	/* Vector of "other" args */
};

#define	MAX_RECURSE	5	/* Levels of shell script we'll look at */

/* For when we run out of memory.....  */

void  nomem()
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

const struct spptr *find_ptr(char *ptr)
{
	unsigned  cnt;
	for  (cnt = 0;  cnt < Ptr_seg.nptrs;  cnt++)
		if  (ncstrcmp(Ptr_seg.pp_ptrs[cnt]->p.spp_ptr, ptr) == 0)
			return  &Ptr_seg.pp_ptrs[cnt]->p;
	return  (const struct spptr *) 0;
}

/* Anchored version of find2nd */

static int  match2nd(const char *subj, const char *comp)
{
	int	lng;
	return  strncmp(subj, comp, lng = strlen(comp)) == 0? lng: 0;
}

/* Find the first occurrence of the 2nd string in the first string,
   then return the index of the first char of the first string after
   the matched string. */

static int  find2nd(const char *subj, const char *comp)
{
	const	char	*cp, *np;
	int	ret;

	for  (cp = subj; (np = strchr(cp, *comp));  cp = np + 1)
		if  ((ret = match2nd(np, comp)) != 0)
			return  (np - subj) + ret;
	return  0;
}

/* Having found the control file for xtlpc, plug in data therefrom */

static int  parse_xtctrl(FILE *cfile, struct prin_params *ppars)
{
	char	**other = ppars->other;
	char	linbuf[200];

	while  (*other)
		other++;

	/* We have just opened the xtlpc-ctrl file.  For now, we just
	   look for assignment to PORTNAME and SPOOLPTR.  Trouble
	   is that the >^B$SPOOLPTR^J line could be replaced with
	   something else, but we'd have to emulate the whole of
	   xtlpc to work out what was going on.  */

	while  (fgets(linbuf, sizeof(linbuf), cfile))  {
		int  lng = strlen(linbuf) - 1;
		char	*cp;

		/* Trim trailing \n  and trailing spaces */

		if  (linbuf[lng] == '\n')
			linbuf[lng--] = '\0';
		while  (lng >= 0  &&  isspace(linbuf[lng]))
			linbuf[lng--] = '\0';

		/* Skip empty lines and comments */

		if  (lng <= 0)
			continue;
		cp = linbuf;
		while  (isspace(*cp))
			cp++;
		if  (*cp == '#')
			continue;

		/* Note assignments to Portnames and Printers */

		if  ((lng = match2nd(cp, "PORTNAME=")))  {
			cp += lng;
			while  (isspace(*cp))
				cp++;
			*other++ = "-P";
			*other++ = stracpy(cp);
			continue;
		}
		if  ((lng = match2nd(cp, "SPOOLPTR=")))  {
			cp += lng;
			while  (isspace(*cp))
				cp++;
			ppars->ptrname = stracpy(cp);
			continue;
		}
	}

	return  1;
}

/* Grab argument from buffer, expanding environment vars
   and advancing pointer */

char *grabarg(char **bufp)
{
	char	*result, *cp, save;

	cp = *bufp;
	do  cp++;
	while  (*cp  &&  !isspace(*cp));
	save = *cp;
	*cp = '\0';
	if  (!(result = envprocess(*bufp)))
		result = stracpy(*bufp);
	*cp = save;
	*bufp = cp;
	return  result;
}

int  parse_xtlpccmd(struct prin_params *ppars, char *buf)
{
	char	*workhost, *workptr, **other, *filename;
	FILE	*cfile;
	int	lng, hadsome = 0;

	workhost = stracpy(ppars->hostname);
	workptr = stracpy(ppars->ptrname);

 findend:
	other = ppars->other;
	while  (*other)
		other++;
 looknxt:
	while  (*buf  &&  *buf != '-')
		buf++;
	if  (!*buf)
		return  hadsome;

	/* Looking at a -argument.  Try to interpret this.  We are
	   only interested in -H -S -f and -P */

	switch  (*++buf)  {
	default:
		goto  looknxt;

	case  'H':

		do  buf++;
		while  (*buf && isspace(*buf));

		if  (!*buf)
			return  0; /* Syntax error not my problem */

		/* If we set the host name as SPOOLDEV we're probably OK */

		if  ((lng = match2nd(buf, "$SPOOLDEV")))  {
			ppars->hostname = workhost;
			buf += lng;
			goto  looknxt;
		}

		/* Might use printer name */

		if  ((lng = match2nd(buf, "$SPOOLPTR")))  {
			ppars->hostname = workptr;
			buf += lng;
			goto  looknxt;
		}

		/* Might use local name?? */

		if  ((lng = match2nd(buf, "$SPOOLHOST")))  {
			ppars->hostname = LOCALHOST_NAME;
			buf += lng;
			goto  looknxt;
		}

		/* Failed to find names we know, grab what we can.
		   We can't be too sure about environment vars, but
		   we could go on for ever emulating the shell and
		   this is surely an overkill as it stands. */

		ppars->hostname = grabarg(&buf);
		hadsome++;
		goto  looknxt;

	case  'S':

		do  buf++;
		while  (*buf && isspace(*buf));

		if  (!*buf)
			return  0; /* Syntax error not my problem */

		/* Same sort of stuff as for hostname */

		*other++ = "-S";

		if  ((lng = match2nd(buf, "$SPOOLDEV")))  {
			*other++ = workhost;
			buf += lng;
			goto  looknxt;
		}

		if  ((lng = match2nd(buf, "$SPOOLHOST")))  {
			*other++ = LOCALHOST_NAME;
			buf += lng;
			goto  looknxt;
		}

		*other++ = grabarg(&buf);
		hadsome++;
		goto  looknxt;

	case  'f':

		do  buf++;
		while  (*buf && isspace(*buf));

		if  (!*buf)
			return  0; /* Syntax error not my problem */

		filename = grabarg(&buf);
		cfile = fopen(filename, "r");
		if  (!cfile)
			goto  looknxt;

		if  (parse_xtctrl(cfile, ppars))
			hadsome++;
		fclose(cfile);
		goto  findend;	/* Need to find the end of "other" */

	case  'P':		/* Specified printer name */

		do  buf++;
		while  (*buf && isspace(*buf));

		if  (!*buf)
			return  0; /* Syntax error not my problem */

		ppars->ptrname = grabarg(&buf);
		hadsome++;
		goto  looknxt;
	}
}

FILE *getexec(char *cmd)
{
	char	*ec, *pathenv, *ep;
	int	plen, clen;
	FILE	*ret;
	char	bld[200];

	for  (ec = cmd;  *ec && !isspace(*ec);  ec++)
		;
	if  (*ec)
		*ec = '\0';

	/* If it's an absolute path, we've made it */

	if  (cmd[0] == '/')
		return  fopen(cmd, "r");

	if  ((clen = strlen(cmd)) == 0  ||  clen >= sizeof(bld))
		return  (FILE *) 0;

	pathenv = getenv("PATH");
	for  (;  (ep = strchr(pathenv, ':'));  pathenv = ep + 1)  {
		if  (pathenv[0] != '/')
			continue;
		plen = ep - pathenv;
		if  (plen + clen + 2 >= sizeof(bld))
		     continue;
		strncpy(bld, pathenv, plen);
		bld[plen] = '/';
		strcpy(&bld[plen+1], cmd);
		if  ((ret = fopen(bld, "r")))
			return  ret;
	}

	/* Now for the last chunk of the PATH */

	plen = strlen(pathenv);
	if  (plen + clen + 2 >= sizeof(bld)  ||  pathenv[0] != '/')
		return  (FILE *) 0;
	strcpy(bld, pathenv);
	bld[plen] = '/';
	strcpy(&bld[plen+1], cmd);
	return  fopen(bld, "r");
}

int  lookat_cmd(const int level, char *cmd, struct prin_params *ppars)
{
	int	len = find2nd(cmd, "xtlpc "), cnt;
	FILE	*fp;
	char	linbuf[200];

	if  (len > 0)
		return  parse_xtlpccmd(ppars, cmd + len);

	/* We've done enough recursing for one day perhaps... */

	if  (level <= 0)
		return  0;

	while  (isspace(*cmd))
		cmd++;

	if  (!(fp = getexec(cmd)))
		return  0;

	if  ((len = fread(linbuf, 1, sizeof(linbuf), fp)) <= 0)  {
		fclose(fp);
		return  0;
	}

	/* If we find any non-printing chars in the first
	   bufferful of stuff, then take it as binary */

	for  (cnt = 0;  cnt < len;  cnt++)
		if  (!isascii(linbuf[cnt]))  {
			fclose(fp);
			return  0;
		}

	/* Make the line look as if we'd just read it with
	   fgets. We didn't use that in the first place in
	   case binary files cause it grief */

	for  (cnt = 0;  cnt < len;  cnt++)
		if  (linbuf[cnt] == '\n')  {
			linbuf[cnt] = '\0';
			len = cnt - 1;
			fseek(fp, (long) (cnt + 1), 0);
			goto  flin;
		}

	fclose(fp);
	return  0;		/* Confused by long line */

 flin:
	for  (;;)  {
		/* Trim trailing spaces */

		while  (len >= 0  &&  isspace(linbuf[len]))
			linbuf[len--] = '\0';

		if  (len > 0)  {
			char	*cp = linbuf;

			while  (isspace(*cp))
				cp++;

			if  (*cp  &&  *cp != '#' && lookat_cmd(level-1, cp, ppars))  {
				fclose(fp);
				return  1;
			}
		}

		if  (!fgets(linbuf, sizeof(linbuf), fp))
			break;

		len = strlen(linbuf) - 1;

		if  (linbuf[len] == '\n')
			linbuf[len--] = '\0';
	}

	fclose(fp);
	return  0;
}


/* Parse the setup file looking for xtlpc commands (and such - maybe)
   Return 0 if we didn't get anywhere (not that we test this as yet, but
   for consistency. We run spdinit as it is pointless writing the code
   twice (and I keep thinking I ought to change it). */

int  parse_su(struct prin_params *ppars)
{
	char	*diname = envprocess(DAEMINIT);
	char	*bldcmd;
	FILE	*pf;
	int	toskip, ret;
	struct	initpkt	params;

	/* Grab the spdinit process on a pipe to get the network filter string */

	if  (!(bldcmd = malloc((unsigned) (strlen(diname) + strlen(ppars->ptrname) + strlen(ppars->formname) + 7))))
		nomem();
	sprintf(bldcmd, "%s '%s' '%s'", diname, ppars->ptrname, ppars->formname);
	free(diname);
	pf = popen(bldcmd, "r");
	free(bldcmd);
	if  (!pf)
		return  0;

	/* Now read the initial packet */

	if  (fread((char *) &params, sizeof(params), 1, pf) != 1)  {
		pclose(pf);
		return  0;
	}

	/* We have to skip over the packet by reading through, as seeks
	   don't work on pipes */

	toskip = params.pi_setup +
		params.pi_halt +
		params.pi_docstart +
		params.pi_docend +
		params.pi_bdocstart +
		params.pi_bdocend +
		params.pi_sufstart +
		params.pi_sufend +
		params.pi_pagestart +
		params.pi_pageend +
		params.pi_abort +
		params.pi_restart +
		params.pi_align +
		params.pi_filter +
		params.pi_rfile +
		params.pi_rcstring +
		params.pi_logfile +
		params.pi_portsu +
		params.pi_bannprog;

	while  (toskip > 0)  {
		getc(pf);
		toskip--;
	}

	if  (params.pi_netfilt == 0)  {
		pclose(pf);
		return  0;
	}

	if  (!(bldcmd = malloc(params.pi_netfilt)))
		nomem();

	if  (fread(bldcmd, params.pi_netfilt, 1, pf) != 1)  {
		pclose(pf);
		return  0;
	}

	/* It's kind of kinder to read to the end... */

	for  (toskip = params.pi_sttystring;  toskip > 0;  toskip--)
		getc(pf);

	if  (pclose(pf) != 0)  {
		free(bldcmd);
		return  0;
	}

	/* If we're executing the thing directly we don't
	   recurse looking through shell scripts. */

	ret = lookat_cmd(params.pi_flags & PI_EXNETFILT? 0: MAX_RECURSE, bldcmd, ppars);
	free(bldcmd);
	return  ret;
}

#ifdef SP_LPRM
void  sppremove(char *ptr, char **arglist)
#else
void  sppdisplay(char *ptr)
#endif
{
	char	**ccptr;

#ifdef SP_LPRM
	char	**cmdbuf;
	unsigned  cnt = 16;

	/* Count arguments and allocate cmdbuf to fit.  */

	for  (ccptr = arglist;  *ccptr;  ccptr++)
		cnt++;
	if  (!(cmdbuf = (char **) malloc(cnt * sizeof(char *))))
		nomem();
	progname = envprocess(SPLPRMPROG);
#else
	char	*cmdbuf[20];
	progname = envprocess(SPLPQPROG);
#endif
	ccptr = cmdbuf;
	*ccptr++ = (char *) progname;
	*ccptr++ = "-H";
	*ccptr++ = spechost;
	*ccptr++ = "-p";
	*ccptr++ = ptr;
#ifdef SP_LPRM
	while  (*arglist)
		*ccptr++ = *arglist++;
#else
	if  (longlist)
		*ccptr++ = "-l";
#endif
	*ccptr = (char *) 0;
	execv(progname, cmdbuf);
	disp_str = progname;
	print_error($E{sp.lpq no prog});
	exit(E_SETUP);
}

#ifdef SP_LPRM
void  premove(char *ptr, char **arglist)
#else
void  pdisplay(char *ptr)
#endif
{
	const  struct  spptr  *pp = find_ptr(ptr);
	char	**ccptr, **othptr;
	struct	prin_params	ppars;
#ifdef SP_LPRM
	unsigned	cnt = 16;
	char	**cmdbuf;
	char	*other[10];

	/* Count arguments and allocate cmdbuf to fit.  */

	for  (ccptr = arglist;  *ccptr;  ccptr++)
		cnt++;
	if  (!(cmdbuf = (char **) malloc(cnt * sizeof(char *))))
		nomem();
#else
	char	*other[10], *cmdbuf[20];
#endif

	if  (!pp)  {
		disp_str = ptr;
		print_error($E{sp.lpq unknown printer});
		exit(E_BADPTR);
	}

	/* Set these until we know better.  If it's a remote printer
	   we'll never know any better.  */

	ppars.hostname = (char *) pp->spp_dev;
	ppars.ptrname = ptr;
	ppars.formname = (char *) pp->spp_form;
	BLOCK_ZERO(other, sizeof(other));
	ppars.other = other;

	if  (pp->spp_netid == 0)  {

		/* If no printers directory, there cant be a setup
		   file so we must be dealing with a local
		   printer whatever spp_netflags says (but it's
		   probably wrong and won't work).  */

		if  (chdir(ptr) < 0)
			ppars.hostname = LOCALHOST_NAME;
		else  if  (pp->spp_netflags & SPP_LOCALNET)
			parse_su(&ppars);
	}

#ifdef SP_LPRM
	progname = envprocess(SPLPRMPROG);
#else
	progname = envprocess(SPLPQPROG);
#endif
	ccptr = cmdbuf;
	*ccptr++ = (char *) progname;
	*ccptr++ = "-H";
	*ccptr++ = ppars.hostname;
	*ccptr++ = "-p";
	*ccptr++ = ppars.ptrname;
#ifndef SP_LPRM
	if  (longlist)
		*ccptr++ = "-l";
#endif
	othptr = other;
	while  (*othptr)
		*ccptr++ = *othptr++;
#ifdef SP_LPRM
	while  (*arglist)
		*ccptr++ = *arglist++;
#endif
	*ccptr = (char *) 0;
	execv(progname, cmdbuf);
	disp_str = progname;
	print_error($E{sp.lpq no prog});
	exit(E_SETUP);
}

OPTION(o_explain)
{
#ifdef SP_LPRM
	print_error($E{sp.lprm options});
#else
	print_error($E{sp.lpq options});
#endif
	exit(0);
}

#ifndef SP_LPRM
OPTION(o_longlist)
{
	longlist = 1;
	return  OPTRESULT_OK;
}

OPTION(o_shortlist)
{
	longlist = 0;
	return  OPTRESULT_OK;
}

#endif

OPTION(o_spechost)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (spechost)
		free(spechost);
	if  (strcmp(arg, "-") == 0)
		spechost = 0;
	else
		spechost = stracpy(arg);
	return  OPTRESULT_ARG_OK;
}

#include "inline/o_freeze.c"

/* Defaults and proc table for arg interp.  */

static	const	Argdefault	Adefs[] = {
#ifdef SP_LPRM
	{  '?', $A{sp.lprm explain}	},
	{  'H', $A{sp.lprm spechost}	},
#else
	{  '?', $A{sp.lpq explain}	},
	{  'l', $A{sp.lpq longlist}	},
	{  's', $A{sp.lpq shortlist}	},
	{  'H', $A{sp.lpq spechost}	},
#endif
	{ 0, 0 }
};

optparam  optprocs[] = {
o_explain,
#ifndef SP_LPRM
o_longlist,	o_shortlist,
#endif
o_spechost,
o_freezecd,	o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
#ifdef SP_LPRM
	fprintf(dest, "%s", name);
	if  (spechost)  {
		spitoption($A{sp.lprm spechost}, $A{sp.lprm explain}, dest, '=', 0);
		fprintf(dest, " %s", spechost);
	}
	else
		putc('=', dest);
#else
	int	cancont = 0;
	fprintf(dest, "%s", name);
	cancont = spitoption(longlist? $A{sp.lpq longlist}: $A{sp.lpq shortlist}, $A{sp.lpq explain}, dest, '=', cancont);
	if  (spechost)  {
		spitoption($A{sp.lpq spechost}, $A{sp.lpq explain}, dest, ' ', 0);
		fprintf(dest, " %s", spechost);
	}
#endif
	putc('\n', dest);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
	char	*pdir;
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
	Cfile = open_cfile(LPDINTCONF, "splpd.help");
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;

#ifdef SP_LPRM
	argv = optprocess(argv, Adefs, optprocs, $A{sp.lprm explain}, $A{sp.lprm freeze home}, 0);
#else
	argv = optprocess(argv, Adefs, optprocs, $A{sp.lpq explain}, $A{sp.lpq freeze home}, 0);
#endif

	/* Now we want to be Daemuid throughout if possible.  */

	setuid(Daemuid);

#define	FREEZE_EXIT
#include "inline/freezecode.c"

#ifdef SP_LPRM
	if  (!argv[0] || !argv[1])  {
		print_error($E{sp.lprm no args});
		exit(E_USAGE);
	}
#else
	if  (!argv[0] || argv[1])  {
		print_error($E{sp.lpq no args});
		exit(E_USAGE);
	}
#endif
	if  (spechost)  {
#ifdef SP_LPRM
		sppremove(argv[0], &argv[1]);
#else
		sppdisplay(*argv);
#endif
	}
	else  {

		if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
			print_error($E{Spooler not running});
			exit(E_NOTRUN);
		}

#ifndef	USING_FLOCK
		/* Set up semaphores */

		if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
			print_error($E{Cannot open semaphore});
			exit(E_SETUP);
		}
#endif

		if  (!jobshminit(0))  {
			print_error($E{Cannot open jshm});
			return  E_JOBQ;
		}
		if  (!ptrshminit(0))  {
			print_error($E{Cannot open pshm});
			return  E_PRINQ;
		}
		readptrlist(1);
		pdir = envprocess(PTDIR);
		if  (chdir(pdir) < 0)  {
			disp_str = pdir;
			print_error($E{sp.lpq cannot open ptr dir});
			exit(E_SETUP);
		}

#ifdef SP_LPRM
		premove(argv[0], &argv[1]);
#else
		pdisplay(*argv);
#endif
	}
	return  0;
}
