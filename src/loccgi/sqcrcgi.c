/* sqcrcgi.c -- create job CGI program

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
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <errno.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "pages.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "cgifndjb.h"
#include "xfershm.h"

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

#define	C_MASK	0177		/*  Umask value  */
#define	MAXLONG	0x7fffffffL	/*  Change this?  */
#define	SUBTRIES	10
#define	JN_INC	80000		/*  Add this to job no if clashes */

struct	spdet	*mypriv;
char	*Realuname;

DEF_DISPOPTS;
FILE	*Cfile;

#define	IPC_MODE	0600

#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
struct	spq	SPQ;
char	Sufchars[] = DEF_SUFCHARS;
char	wotform[MAXFORM+1];
char	*page_delim;
struct	pages	page_fe;

int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

static void	init_jobdefaults(void)
{
	/* Assumes SPQ initialised to zero of course */
	strcpy(wotform, mypriv->spu_form);
	strcpy(SPQ.spq_ptr, mypriv->spu_ptr);
	SPQ.spq_pri = mypriv->spu_defp;
	SPQ.spq_class = mypriv->spu_class;
	SPQ.spq_uid = Realuid;
	strncpy(SPQ.spq_uname, Realuname, UIDSIZE);
	strcpy(SPQ.spq_puname, SPQ.spq_uname);
	SPQ.spq_cps = 1;
	SPQ.spq_end = MAXLONG - 1L;
	SPQ.spq_nptimeout = QNPTIMEOUT;
	SPQ.spq_ptimeout = QPTIMEOUT;
}

/* Interpret argument if any as being a "template job" job number */

void	interp_args(char **args)
{
	char	*arg = *args;
	int	ret;
	const	struct	spq  *jp;
	struct	jobswanted  jw;
	unsigned	pagenums = 0;
	LONG		*pageoffsets = (LONG *) 0;

	/* Set up semaphores and job shared memory. We only need
	   this if we have a job to look at. */

#ifndef	USING_FLOCK
	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		html_disperror($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif
	if  (!arg)		/* No job to look at */
		return;
	if  (!jobshminit(0))  {
		html_disperror($E{Cannot open jshm});
		exit(E_JOBQ);
	}
	if  ((ret = decode_jnum(arg, &jw)) != 0)  {
		html_disperror(ret);
		exit(E_USAGE);
	}
	if  (!find_job(&jw))  {
		disp_str = arg;
		html_disperror($E{Unknown job number});
		exit(E_NOJOB);
	}

	/* Copy in parameters for the job */

	jp = jw.jp;
	SPQ.spq_hold = jp->spq_hold;
	SPQ.spq_nptimeout = jp->spq_nptimeout;
	SPQ.spq_ptimeout = jp->spq_ptimeout;
	SPQ.spq_cps = (!(mypriv->spu_flgs & PV_ANYPRIO)  &&  jp->spq_cps > mypriv->spu_cps)?
		mypriv->spu_cps: jp->spq_cps;
	if  (jp->spq_pri >= mypriv->spu_minp  &&  jp->spq_pri <= mypriv->spu_maxp)
		SPQ.spq_pri = jp->spq_pri;
	SPQ.spq_jflags = jp->spq_jflags & (SPQ_NOH|SPQ_WRT|SPQ_MAIL|SPQ_RETN|
					   SPQ_ODDP|SPQ_EVENP|SPQ_REVOE|SPQ_MATTN|
					   SPQ_WATTN|SPQ_LOCALONLY);

	if  ((mypriv->spu_flgs & PV_COVER))
		SPQ.spq_class = jp->spq_class;
	else  if  ((SPQ.spq_class = jp->spq_class & mypriv->spu_class) == 0)
		SPQ.spq_class = mypriv->spu_class;

	SPQ.spq_start = jp->spq_start;
	SPQ.spq_end = jp->spq_end;

	if  (strcmp(jp->spq_uname, jp->spq_puname) != 0)
		strcpy(SPQ.spq_puname, jp->spq_puname);

	strcpy(SPQ.spq_file, jp->spq_file);
	if  ((mypriv->spu_flgs & PV_FORMS) || qmatch(mypriv->spu_formallow, jp->spq_form))
		strcpy(wotform, jp->spq_form);
	if  (mypriv->spu_flgs & PV_OTHERP)
		strcpy(SPQ.spq_ptr, jp->spq_ptr);
	else  {
		char	cptr[JPTRNAMESIZE+1];
		strcpy(cptr, jp->spq_ptr);
		if  (issubset(mypriv->spu_ptrallow, cptr))
			strcpy(SPQ.spq_ptr, jp->spq_ptr);
	}
	strcpy(SPQ.spq_flags, jp->spq_flags);

	/* Grab page delimiter if applicable */
	rdpgfile(jp, &page_fe, &page_delim, &pagenums, &pageoffsets);
	if  (pageoffsets)
		free((char *) pageoffsets);
}

char	had_hdr = 0, had_pri = 0, had_cps = 0;
char	*buff_filename;

static void	arg_jobdata(char *arg)
{
	if  (!had_hdr)
		strncpy(SPQ.spq_file, arg, MAXTITLE);
}

static void	arg_hdr(char *arg)
{
	strncpy(SPQ.spq_file, arg, MAXTITLE);
	if  (arg[0])		/* Only if it isn't blank */
		had_hdr = 1;
}

static void	arg_sp(char *arg)
{
	ULONG  v = strtoul(arg, (char **) 0, 0);
	SPQ.spq_start = v != 0? v - 1: 0;
}

static void	arg_ep(char *arg)
{
	ULONG  v = strtoul(arg, (char **) 0, 0);
	SPQ.spq_end = v != 0? v - 1: MAXLONG-1;
}

static void	arg_cps(char *arg)
{
	int	c = atoi(arg);

	if  (had_cps)
	    SPQ.spq_cps = (unsigned char) (c + SPQ.spq_cps);
	else  {
	    SPQ.spq_cps = (unsigned char) c;
	    had_cps = 1;
	}
}

static void	arg_pri(char *arg)
{
	int	n = atoi(arg);
	if  (had_pri)
	    SPQ.spq_pri = (unsigned char) (n + SPQ.spq_pri);
	else  {
	    SPQ.spq_pri = (unsigned char) n;
	    had_pri = 1;
	}
}

static void	arg_form(char *arg)
{
	strncpy(wotform, arg, MAXFORM);
}

static void	arg_ptr(char *arg)
{
	if  (!((mypriv->spu_flgs & PV_OTHERP)  ||  issubset(mypriv->spu_ptrallow, arg)))  {
		disp_str = arg;
		disp_str2 = mypriv->spu_ptrallow;
		html_disperror($E{Invalid ptr type});
		exit(E_BADPTR);
	}
	strncpy(SPQ.spq_ptr, arg, JPTRNAMESIZE);
}

static void	arg_npt(char *arg)
{
	int	n = atoi(arg);
	SPQ.spq_nptimeout = (USHORT) n;
}

static void	arg_pt(char *arg)
{
	int	n = atoi(arg);
	SPQ.spq_ptimeout = (USHORT) n;
}

static void	arg_jflags(char *arg, const unsigned flag)
{
	switch  (*arg)  {
	case  'y':case  'Y':
	case  't':case  'T':
	case  '1':
		SPQ.spq_jflags |= flag;
		break;
	case  'n':case  'N':
	case  'f':case  'F':
	case  '0':
		SPQ.spq_jflags &= ~flag;
		break;
	}
}

static void	arg_jflags_noh(char *arg)
{
	arg_jflags(arg, SPQ_NOH);
}

static void	arg_jflags_wrt(char *arg)
{
	arg_jflags(arg, SPQ_WRT);
}

static void	arg_jflags_mail(char *arg)
{
	arg_jflags(arg, SPQ_MAIL);
}

static void	arg_jflags_retn(char *arg)
{
	arg_jflags(arg, SPQ_RETN);
}

static void	arg_jflags_oddp(char *arg)
{
	arg_jflags(arg, SPQ_ODDP);
}

static void	arg_jflags_evenp(char *arg)
{
	arg_jflags(arg, SPQ_EVENP);
}

static void	arg_jflags_revoe(char *arg)
{
	arg_jflags(arg, SPQ_REVOE);
}

static void	arg_jflags_mattn(char *arg)
{
	arg_jflags(arg, SPQ_MATTN);
}

static void	arg_jflags_wattn(char *arg)
{
	arg_jflags(arg, SPQ_WATTN);
}

static void	arg_jflags_loco(char *arg)
{
	arg_jflags(arg, SPQ_LOCALONLY);
}

static void	arg_class(char *arg)
{
	classcode_t	cl = strtoul(arg, (char **) 0, 0);
	if  (!(mypriv->spu_flgs & PV_COVER))
		cl &= mypriv->spu_class;
	if  (cl == 0)  {
		disp_str = arg;
		disp_str2 = hex_disp(mypriv->spu_class, 0);
		html_disperror($E{setting zero class});
		exit(E_BADCLASS);
	}
	SPQ.spq_class = cl;
}

static void	arg_hold(char *arg)
{
	time_t	ht = atol(arg), now = time((time_t *) 0);
	if  (ht < now)
		ht = 0;
	SPQ.spq_hold = (LONG) ht;
}

static void	arg_puser(char *arg)
{
	strncpy(SPQ.spq_puname,
		lookup_uname(arg) == UNKNOWN_UID? Realuname: arg,
		UIDSIZE);
}

static void	arg_flags(char *arg)
{
	strncpy(SPQ.spq_flags, arg, MAXFLAGS);
}

struct	posttab  ptab[] =  {
	{	"jobfile",	arg_jobdata,	&buff_filename  },
	{	"sp",		arg_sp		},
	{	"ep",		arg_ep		},
	{	"cps",		arg_cps		},
	{	"cps_10",	arg_cps		},
	{	"cps_100",	arg_cps		},
	{	"pri",		arg_pri		},
	{	"pri_10",	arg_pri		},
	{	"pri_100",	arg_pri		},
	{	"noh",		arg_jflags_noh 	},
	{	"hdr",		arg_hdr		},
	{	"form",		arg_form	},
	{	"ptr",		arg_ptr		},
	{	"npto",		arg_npt		},
	{	"pto",		arg_pt		},
	{	"wrt",		arg_jflags_wrt	},
	{	"mail",		arg_jflags_mail	},
	{	"retn",		arg_jflags_retn	},
	{	"oddp",		arg_jflags_oddp	},
	{	"evenp",	arg_jflags_evenp},
	{	"revoe",	arg_jflags_revoe},
	{	"mattn",	arg_jflags_mattn},
	{	"wattn",	arg_jflags_wattn},
	{	"loco",		arg_jflags_loco	},
	{	"class",	arg_class	},
	{	"hold", 	arg_hold	},
	{	"puser",	arg_puser	},
	{	"flags",	arg_flags	},
	{	(char *) 0  }
};

jobno_t	perform_submit(void)
{
	jobno_t	jn;
	FILE	*inf, *outf;
	int	fid, nopagef = 1, scnt = SUBTRIES;
	unsigned  sleeptime = 1;
	LONG	onpage;
	struct	spr_req	sp_req;
	char	tmpfl[NAMESIZE+6], pgfl[NAMESIZE+6];

	if  (!buff_filename)  {
		html_disperror($E{sqcrcgi no file supplied});
		exit(E_USAGE);
	}
	if  (!(inf = fopen(buff_filename, "r")))  {
		html_disperror($E{sqcrcgi cannot reopen temp file});
		unlink(buff_filename);
		exit(E_SETUP);
	}
	if  (!(mypriv->spu_flgs & PV_ANYPRIO)  &&  SPQ.spq_cps > mypriv->spu_cps)  {
		disp_arg[0] = SPQ.spq_cps;
		disp_arg[1] = mypriv->spu_cps;
		html_disperror($E{Invalid copies});
		unlink(buff_filename);
		exit(E_BADCPS);
	}
	if  (SPQ.spq_pri < mypriv->spu_minp || SPQ.spq_pri > mypriv->spu_maxp)  {
		disp_arg[0] = SPQ.spq_pri;
		disp_arg[1] = mypriv->spu_minp;
		disp_arg[2] = mypriv->spu_maxp;
		html_disperror(mypriv->spu_minp > mypriv->spu_maxp?
			       $E{Priority range excludes}:
			       SPQ.spq_pri == mypriv->spu_defp?
			       $E{No default priority}: $E{Invalid priority});
		unlink(buff_filename);
		exit(E_BADPRI);
	}
	BLOCK_ZERO(&sp_req, sizeof(sp_req));
	sp_req.spr_mtype = MT_SCHED;
	sp_req.spr_un.j.spr_act = SJ_ENQ;
	jn = sp_req.spr_un.j.spr_pid = getpid();

	for  (;;)  {
		strcpy(tmpfl, mkspid(SPNAM, jn));
		if  ((fid = open(tmpfl, O_WRONLY|O_CREAT|O_EXCL, 0400)) >= 0)
			break;
		jn += JN_INC;
	}
	if  (!(outf = fdopen(fid, "w")))  {
		unlink(tmpfl);
		unlink(buff_filename);
		html_nomem();
	}
	SPQ.spq_job = jn;
	SPQ.spq_npages = 0;
	strcpy(SPQ.spq_form, wotform);
	onpage = 0;

	if  (!page_delim || (page_fe.deliml == 1  &&  page_fe.delimnum == 1 &&  page_delim[0] == '\f'))  {
		int	ch;
		while  ((ch = getc(inf)) != EOF)  {
			SPQ.spq_size++;
			onpage++;
			if  (putc(ch, outf) == EOF)
				goto  ffull;
			if  (ch == '\f')  {
				onpage = 0;
				SPQ.spq_npages++;
			}
		}
		if  (onpage)
			SPQ.spq_npages++;
	}
	else  {
		char	*rcp, *rcdend;
		int	rec_cnt, ch;

		nopagef = 0;
		SPQ.spq_dflags |= SPQ_PAGEFILE;
		strcpy(pgfl, mkspid(PFNAM, jn));
		if  ((fid = open(pgfl, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0)  {
			disp_str = pgfl;
			html_disperror($E{Cannot create page file});
			unlink(tmpfl);
			unlink(buff_filename);
			exit(E_IO);
		}

		page_fe.lastpage = 0;	/* Fix this later perhaps */
		write(fid, (char *) &page_fe, sizeof(page_fe));
		write(fid, page_delim, (unsigned) page_fe.deliml);

		rcp = page_delim;
		rcdend = page_delim + page_fe.deliml;
		rec_cnt = 0;

		while  ((ch = getc(inf)) != EOF)  {
			SPQ.spq_size++;
			onpage++;
			if  (ch == *rcp)  {
				if  (++rcp >= rcdend)  {
					if  (++rec_cnt >= page_fe.delimnum)  {
						if  (write(fid, (char *) &SPQ.spq_size, sizeof(LONG)) != sizeof(LONG))
							goto  ffull;
						onpage = 0;
						rec_cnt = 0;
						SPQ.spq_npages++;
					}
					rcp = page_delim;
				}
			}
			else  if  (rcp > page_delim)  {
				char	*pp, *prcp, *prevpl;
				prevpl = --rcp;	/*  Last one matched  */
				for  (;  rcp > page_delim;  rcp--)  {
					if  (*rcp != ch)
						continue;
					pp = prevpl;
					prcp = rcp - 1;
					for  (;  prcp >= page_delim;  pp--, prcp--)
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

		if  (write(fid, (char *) &SPQ.spq_size, sizeof(LONG)) != sizeof(LONG))
			goto  ffull;

		/* Remember how big the last page was */

		if  (onpage > 0)  {
			SPQ.spq_npages++;
			if  ((page_fe.lastpage = page_fe.delimnum - rec_cnt) > 0)  {
				lseek(fid, 0L, 0);
				write(fid, (char *) &page_fe, sizeof(page_fe));
			}
		}

		if  (close(fid) < 0)
			goto  ffull;
	}

	if  (fclose(outf) == EOF)
		goto  ffull;
	fclose(inf);
	unlink(buff_filename);	/* Finished with that */

	if  (SPQ.spq_size <= 0)  {
		html_disperror($E{sqcrcgi no job data});
		unlink(tmpfl);
		if  (!nopagef)
			unlink(pgfl);
		exit(E_FALSE);
	}
	SPQ.spq_time = (LONG) time((time_t *) 0);

	for  (;  scnt > 0  &&  wjmsg(&sp_req, &SPQ) != 0;  sleeptime <<= 1, scnt--)
		sleep(sleeptime);

	return  jn;

 ffull:
	unlink(tmpfl);
	if  (!nopagef)
		unlink(pgfl);
	unlink(buff_filename);
	html_disperror($E{sqcrcgi file full});
	exit(E_IO);
	return  0;		/* Shut up C compilers */
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char	*spdir, **newargs;
	jobno_t	jobnum;
	int	ret;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif
	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();
	html_openini();
	newargs = cgi_arginterp(argc, argv, 0); /* Side effect of cgi_arginterp is to set Realuid */
	Effuid = geteuid();
	INIT_DAEMUID;

	Cfile = open_cfile(MISC_UCONFIG, "rest.help");

	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	Realuname = prin_uname(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	hash_hostfile();

	/* Now we want to be Daemuid throughout if possible.  */

#if  	defined(OS_BSDI) || defined(OS_FREEBSD)
	seteuid(Daemuid);
#else
	setuid(Daemuid);
#endif

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		html_disperror($E{Spooler not running});
		exit(E_NOTRUN);
	}
	if  ((ret = init_xfershm(0)))  {
		html_disperror(ret);
		exit(E_SETUP);
	}

	umask(C_MASK);
	init_jobdefaults();
	interp_args(newargs);
	html_postvalues(ptab);
	spdir = envprocess(SPDIR);
	if  (chdir(spdir) < 0)  {
		html_disperror($E{Cannot chdir});
		exit(E_NOCHDIR);
	}

	jobnum = perform_submit();
	html_out_param_file("submitok", 1, jobnum, 0L);
	return  0;
}
