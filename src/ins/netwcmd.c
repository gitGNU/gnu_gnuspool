/* netwcmd.c -- Extract what network command would be run for given printer/form

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
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"

DEF_DISPOPTS;

FILE	*Cfile;

#define	IPC_MODE	0600

#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;

int	expandem = 1;
char	*progname;
struct	spdet	*mypriv;

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

const struct spptr *	find_ptr(char * ptr)
{
	unsigned  cnt;
	for  (cnt = 0;  cnt < Ptr_seg.nptrs;  cnt++)
		if  (ncstrcmp(Ptr_seg.pp_ptrs[cnt]->p.spp_ptr, ptr) == 0)
			return  &Ptr_seg.pp_ptrs[cnt]->p;
	return  (const struct spptr *) 0;
}

/* Anchored version of find2nd */

static int	match2nd(const char *subj, const char *comp)
{
	int	lng;
	return  strncmp(subj, comp, lng = strlen(comp)) == 0? lng: 0;
}

/* Find the first occurrence of the 2nd string in the first string,
   then return the index of the first char of the first string after
   the matched string. */

static int	find2nd(const char *subj, const char *comp)
{
	const	char	*cp, *np;
	int	ret;

	for  (cp = subj; (np = strchr(cp, *comp));  cp = np + 1)
		if  ((ret = match2nd(np, comp)) != 0)
			return  (np - subj) + ret;
	return  0;
}

/* Look for specific strings in command and rewrite. */

char  *rebuild(char * cmd, const char * str, const char * repl)
{
	int	mp, limit = 10;
	char	*res = cmd;

	while  (--limit >= 0  &&  (mp = find2nd(res, str)) > 0)  {	/* Loop in case of several matches
									   but set a limit in case replacement recurses */
		int	sl = strlen(str), rl = strlen(repl);
		int	mb = mp - sl;		/* Index of first char matched */
		char	*nres = malloc((unsigned) (strlen(res) + 1 + rl - sl));
		if  (!nres)
			nomem();

		strncpy(nres, res, mb);		/* Copy up to first char matched */
		strcpy(&nres[mb], repl);	/* Insert replacement */
		strcpy(&nres[mb+rl], &res[mp]);	/* Copy following replacement */
		free(res);
		res = nres;
	}

	return  res;
}

/* Expand various environment variables in network command. */

char  *expandenv(char *cmd, const struct spptr *pp, char *formname)
{
	char  *res = rebuild(cmd, "$SPOOLDEV", pp->spp_dev), *res2;
	res = rebuild(res, "$SPOOLPTR", pp->spp_ptr);
	res = rebuild(res, "$SPOOLFORM", formname);
	res2 = envprocess(res);
	free(res);
	return  res2;
}

/* Extract network command from our setup file and print out. */

int	do_bizniz(char *pname, char *formname)
{
	const  struct  spptr  *pp = find_ptr(pname);
	char	*diname = envprocess(DAEMINIT);
	char	*bldcmd, *rescmd;
	FILE	*pf;
	int	toskip;
	struct	initpkt	params;

	if  (!pp)  {
		fprintf(stderr, "%s: Unknown printer %s\n", progname, pname);
		return  E_BADPTR;
	}

	if  (pp->spp_netid)  {
		fprintf(stderr, "%s: Printer %s is not local\n", progname, pname);
		return  E_BADPTR;
	}

	if  (!formname)
		formname = stracpy(pp->spp_form);

	/* Grab the spdinit process on a pipe to get the network filter string */

	if  (!(bldcmd = malloc((unsigned) (strlen(diname) + strlen(pp->spp_ptr) + strlen(formname) + 7))))
		nomem();
	sprintf(bldcmd, "%s '%s' '%s'", diname, pp->spp_ptr, formname);
	free(diname);
	pf = popen(bldcmd, "r");
	free(bldcmd);
	if  (!pf)  {
		fprintf(stderr, "%s: cannot create pipe command\n", progname);
		return  E_SETUP;
	}

	/* Now read the initial packet */

	if  (fread((char *) &params, sizeof(params), 1, pf) != 1)  {
		pclose(pf);
		fprintf(stderr, "%s: cannot read pipe\n", progname);
		return  E_SETUP;
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
		fprintf(stderr, "%s: no network command for printer %s\n", progname, pname);
		return  E_BADPTR;
	}

	if  (!(bldcmd = malloc(params.pi_netfilt)))
		nomem();

	if  (fread(bldcmd, params.pi_netfilt, 1, pf) != 1)  {
		pclose(pf);
		fprintf(stderr, "%s: cannot read pipe\n", progname);
		return  E_SETUP;
	}

	/* It's kind of kinder to read to the end... */

	for  (toskip = params.pi_sttystring;  toskip > 0;  toskip--)
		getc(pf);

	if  (pclose(pf) != 0)  {
		free(bldcmd);
		fprintf(stderr, "%s: error on pipe command\n", progname);
		return  E_SETUP;
	}

	rescmd = expandem  &&  strchr(bldcmd, '$')?  expandenv(bldcmd, pp, formname): bldcmd;
	if  (!(params.pi_flags & PI_EXNETFILT))
		printf("#! /bin/sh\n\n");
	if  (!(params.pi_flags & PI_REOPEN))
		printf("# Warning: No reopen keyword - is this right?\n\n");
	printf("%s\n", rescmd);
	free(rescmd);
	return  0;
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char	*pdir;
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
	mypriv = getspuser(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	/* Now we want to be Daemuid throughout if possible.  */
	setuid(Daemuid);

	if  (argc < 2)  {
	usage:
		fprintf(stderr, "Usage: %s [-n] ptr [form]\n", progname);
		exit(E_USAGE);
	}
	if  (strcmp(argv[1], "-n") == 0)  {
		expandem = 0;
		argv++;
		argc--;
	}

	if  (argc < 2  ||  argc > 3)
		goto  usage;

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		fprintf(stderr, "%s: Spooler not running\n", progname);
		exit(E_NOTRUN);
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		fprintf(stderr, "%s: Cannot open semaphore\n", progname);
		exit(E_SETUP);
	}
#endif

	if  (!jobshminit(0))  {
		fprintf(stderr, "%s: Cannot open jshm\n", progname);
		return  E_JOBQ;
	}
	if  (!ptrshminit(0))  {
		fprintf(stderr, "%s: Cannot open pshm\n", progname);
		return  E_PRINQ;
	}
	readptrlist(1);
	pdir = envprocess(PTDIR);
	if  (chdir(pdir) < 0)  {
		fprintf(stderr, "%s: Cannot open ptr dir %s\n", progname, pdir);
		exit(E_SETUP);
	}

	return  do_bizniz(argv[1], argv[2]);
}
