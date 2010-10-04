/* sqchange.c -- change job parameters

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
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "xfershm.h"
#include "pages.h"
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"
#include "cgifndjb.h"
#include "shutilmsg.h"

#define	SECSPERDAY	(24 * 60 * 60L)

#define	MAXLONG	0x7fffffffL	/*  Change this?  */

#define	HTIME	5		/* Forge prompt if one doesn't come */

int	spitoption(const int, const int, FILE *, const int, const int);
int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

#define	IPC_MODE	0600

struct	spq	SPQ;

char	*Realuname,
	*Curr_pwd;

struct	spdet	*mypriv;

char	Sufchars[] = DEF_SUFCHARS;

extern	char	freeze_wanted;
char	freeze_cd, freeze_hd;

int	doing_something,	/* Set if something would affect jobs */
	cps_changes,
	flags_changes,
	form_changes,
	mail_changes,
	mattn_changes,
	noh_changes,
	npto_changes,
	oe_changes,
	cc_changes,
	pri_changes,
	pto_changes,
	ptr_changes,
	range_changes,
	retn_changes,
	local_changes,
	tdel_changes,
	title_changes,
	uname_changes,
	wattn_changes,
	wrt_changes,
	exit_code;

/* Keep library happy */

void  nomem()
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

/* "Read" job file.  */

void  rjobfile()
{
	jobshm_lock();
#ifdef	USING_MMAP
	if  (Job_seg.dinf.segsize != Job_seg.dptr->js_did)
#else
	if  (Job_seg.dinf.base != Job_seg.dptr->js_did)
#endif
		jobgrown();
	Job_seg.Last_ser = Job_seg.dptr->js_serial;
	jobshm_unlock();
}

/* This is the main processing routine.  */

void  process(char **joblist)
{
	char		*jobc;
	struct	spr_req	jreq;

	jreq.spr_mtype = MT_SCHED;
	jreq.spr_un.j.spr_act = SJ_CHNG;
	jreq.spr_un.j.spr_pid = getpid();
	jreq.spr_un.j.spr_seq = 0;
	jreq.spr_un.j.spr_netid = 0;

	while  ((jobc = *joblist++))  {
		const  Hashspq	*hjp;
		const  struct  spq  *jp;
		int		ret;
		struct	jobswanted  jw;

		if  ((ret = decode_jnum(jobc, &jw)) != 0)  {
			print_error(ret);
			exit_code = E_NOJOB;
			continue;
		}

		if  (jw.host  &&  !(mypriv->spu_flgs & PV_REMOTEJ))  {
			disp_str = jobc;
			print_error($E{sqdel no remote job priv});
			exit_code = E_NOPRIV;
			continue;
		}

		if  (!(hjp = find_job(&jw)))  {
			disp_str = jobc;
			print_error($E{Unknown job number});
			exit_code = E_NOJOB;
			continue;
		}
		jp = &hjp->j;

		if  (!(mypriv->spu_flgs & PV_OTHERJ)  &&  strcmp(Realuname, jp->spq_uname) != 0)  {
			print_error($E{Chngdel not yours});
			exit_code = E_NOPRIV;
			continue;
		}

		if  (form_changes)  {
			if  (!((mypriv->spu_flgs & PV_FORMS) || qmatch(mypriv->spu_formallow, SPQ.spq_form)))  {
				disp_str = SPQ.spq_form;
				disp_str2 = jp->spq_form;
				print_error($E{No change form priv});
				exit_code = E_BADFORM;
				continue;
			}
		}
		else
			strncpy(SPQ.spq_form, jp->spq_form, MAXFORM);

		if  (ptr_changes)  {
			if  (!((mypriv->spu_flgs & PV_OTHERP)  ||  issubset(mypriv->spu_ptrallow, SPQ.spq_ptr)))  {
				disp_str = SPQ.spq_ptr;
				disp_str2 = jp->spq_ptr;
				print_error($E{No change ptr priv});
				exit_code = E_BADPTR;
				continue;
			}
		}
		else
			strncpy(SPQ.spq_ptr, jp->spq_ptr, JPTRNAMESIZE);

		jreq.spr_un.j.spr_jslot = hjp - Job_seg.jlist;

		SPQ.spq_job = jp->spq_job;
		SPQ.spq_netid = jw.host;
		SPQ.spq_haltat = jp->spq_haltat;
		SPQ.spq_dflags = jp->spq_dflags;
		SPQ.spq_npages = jp->spq_npages;

		if  (!tdel_changes)
			SPQ.spq_hold = jp->spq_hold;
		if  (!npto_changes)
			SPQ.spq_nptimeout = jp->spq_nptimeout;
		if  (!pto_changes)
			SPQ.spq_ptimeout = jp->spq_ptimeout;
		if  (!uname_changes)
			strcpy(SPQ.spq_puname, jp->spq_puname);
		if  (!cps_changes)
			SPQ.spq_cps = jp->spq_cps;
		if  (!pri_changes)
			SPQ.spq_pri = jp->spq_pri;
		if  (!noh_changes)  {
			SPQ.spq_jflags &= ~SPQ_NOH;
			SPQ.spq_jflags |= jp->spq_jflags & SPQ_NOH;
		}
		if  (!wrt_changes)  {
			SPQ.spq_jflags &= ~SPQ_WRT;
			SPQ.spq_jflags |= jp->spq_jflags & SPQ_WRT;
		}
		if  (!mail_changes)  {
			SPQ.spq_jflags &= ~SPQ_MAIL;
			SPQ.spq_jflags |= jp->spq_jflags & SPQ_MAIL;
		}
		if  (!retn_changes)  {
			SPQ.spq_jflags &= ~SPQ_RETN;
			SPQ.spq_jflags |= jp->spq_jflags & SPQ_RETN;
		}
		if  (!local_changes)  {
			SPQ.spq_jflags &= ~SPQ_LOCALONLY;
			SPQ.spq_jflags |= jp->spq_jflags & SPQ_LOCALONLY;
		}
		if  (!oe_changes)  {
			SPQ.spq_jflags &= ~(SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);
			SPQ.spq_jflags |= jp->spq_jflags & (SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);
		}
		if  (!cc_changes)
			SPQ.spq_class = jp->spq_class;
		if  (!mattn_changes)  {
			SPQ.spq_jflags &= ~SPQ_MATTN;
			SPQ.spq_jflags |= jp->spq_jflags & SPQ_MATTN;
		}
		if  (!wattn_changes)  {
			SPQ.spq_jflags &= ~SPQ_WATTN;
			SPQ.spq_jflags |= jp->spq_jflags & SPQ_WATTN;
		}
		if  (!range_changes)  {
			SPQ.spq_start = jp->spq_start;
			SPQ.spq_end = jp->spq_end;
		}
		if  (!title_changes)
			strncpy(SPQ.spq_file, jp->spq_file, MAXTITLE);
		if  (!flags_changes)
			strncpy(SPQ.spq_flags, jp->spq_flags, MAXFLAGS);
		if  ((ret = wjmsg(&jreq, &SPQ)))  {
			print_error(ret);
			exit(E_SETUP);
		}
		waitsig();
	}
}

OPTION(o_explain)
{
	print_error($E{sqchange options});
	exit(0);
}

#define	INLINE_SQCHANGE
#include "inline/o_hdrs.c"
#include "inline/o_retn.c"
#include "inline/o_loco.c"
#include "inline/o_mailwrt.c"
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
#include "inline/o_setclass.c"
#include "inline/o_freeze.c"
#include "inline/o_classc.c"

/* Defaults and proc table for arg interp.  */

static	const	Argdefault	Adefs[] = {
  {  '?', $A{sqchange explain}		},
  {  's', $A{sqchange no banner}	},
  {  'r', $A{sqchange banner}		},
  {  'x', $A{sqchange no messages}	},
  {  'w', $A{sqchange write message}	},
  {  'm', $A{sqchange mail message}	},
  {  'b', $A{sqchange no attention}	},
  {  'a', $A{sqchange mail attention}	},
  {  'A', $A{sqchange write attention}	},
  {  'z', $A{sqchange no retain}	},
  {  'q', $A{sqchange retain}		},
  {  'l', $A{sqchange local only}	},
  {  'L', $A{sqchange network wide}	},
  {  'c', $A{sqchange copies}		},
  {  'h', $A{sqchange header}		},
  {  'f', $A{sqchange formtype}		},
  {  'p', $A{sqchange priority}		},
  {  'P', $A{sqchange printer}		},
  {  'u', $A{sqchange post user}	},
  {  'C', $A{sqchange classcode}	},
  {  'R', $A{sqchange page range}	},
  {  'F', $A{sqchange post proc flags}	},
  {  't', $A{sqchange printed timeout}	},
  {  'T', $A{sqchange not printed timeout}	},
  {  'n', $A{sqchange delay for}	},
  {  'N', $A{sqchange delay until}	},
  {  'O', $A{sqchange odd even}		},
  {  'S', $A{sqchange set classcode}	},
  {  0, 0 }
};

optparam  optprocs[] = {
o_explain,	o_nohdrs,	o_hdrs,		o_nomailwrt,
o_wrt,		o_mail,		o_noattn,	o_mattn,
o_wattn,	o_noretn,	o_retn,		o_localonly,
o_nolocalonly,	o_copies,	o_header,	o_formtype,
o_priority,	o_printer,	o_user,		o_classcode,
o_range,	o_flags,	o_ptimeout,	o_nptimeout,
o_tdelay,	o_dtime,	o_oddeven,	o_setclass,
o_freezecd,	o_freezehd
};

static	void  dumpstr(FILE *dest, const char *str)
{
	if  (strpbrk(str, " \t"))
		fprintf(dest, " \'%s\'", str);
	else
		fputs(str, dest);
}

void  spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;
	fprintf(dest, "%s", name);

	spitoption($A{sqchange classcode}, $A{sqchange explain}, dest, '=', 0);
	fprintf(dest, " %s", hex_disp(Displayopts.opt_classcode, 0));
	if  (noh_changes)
		cancont = spitoption(SPQ.spq_jflags & SPQ_NOH? $A{sqchange no banner}: $A{sqchange banner}, $A{sqchange explain}, dest, ' ', cancont);
	if  (mail_changes || wrt_changes)  {
		if  ((SPQ.spq_jflags & (SPQ_MAIL|SPQ_WRT)) != (SPQ_MAIL|SPQ_WRT))
			cancont = spitoption($A{sqchange no messages}, $A{sqchange explain}, dest, ' ', cancont);
		if  (SPQ.spq_jflags & SPQ_MAIL)
			cancont = spitoption($A{sqchange mail message}, $A{sqchange explain}, dest, ' ', cancont);
		if  (SPQ.spq_jflags & SPQ_WRT)
			cancont = spitoption($A{sqchange write message}, $A{sqchange explain}, dest, ' ', cancont);
	}
	if  (mattn_changes || wattn_changes) {
		if  ((SPQ.spq_jflags & (SPQ_MATTN|SPQ_WATTN)) != (SPQ_MATTN|SPQ_WATTN))
			cancont = spitoption($A{sqchange no attention}, $A{sqchange explain}, dest, ' ', cancont);
		if  (SPQ.spq_jflags & SPQ_MATTN)
			cancont = spitoption($A{sqchange mail attention}, $A{sqchange explain}, dest, ' ', cancont);
		if  (SPQ.spq_jflags & SPQ_WATTN)
			cancont = spitoption($A{sqchange write attention}, $A{sqchange explain}, dest, ' ', cancont);
	}
	if  (retn_changes)
		cancont = spitoption(SPQ.spq_jflags & SPQ_RETN? $A{sqchange retain}: $A{sqchange no retain}, $A{sqchange explain}, dest, ' ', cancont);
	if  (local_changes)
		cancont = spitoption(SPQ.spq_jflags & SPQ_LOCALONLY? $A{sqchange local only}: $A{sqchange network wide}, $A{sqchange explain}, dest, ' ', cancont);
	if  (cps_changes)  {
		spitoption($A{sqchange copies}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %d", SPQ.spq_cps);
	}
	if  (title_changes && SPQ.spq_file[0])  {
		spitoption($A{sqchange header}, $A{sqchange explain}, dest, ' ', 0);
		dumpstr(dest, SPQ.spq_file);
	}
	if  (form_changes)  {
		spitoption($A{sqchange formtype}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %s", SPQ.spq_form);
	}
	if  (ptr_changes)  {
		spitoption($A{sqchange printer}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %s", SPQ.spq_ptr);
	}
	if  (uname_changes)  {
		spitoption($A{sqchange post user}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %s", SPQ.spq_puname);
	}
	if  (pri_changes)  {
		spitoption($A{sqchange priority}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %d", SPQ.spq_pri);
	}
	if  (cc_changes)  {
		spitoption($A{sqchange set classcode}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %s", hex_disp(SPQ.spq_class, 0));
	}
	if  (flags_changes  &&  SPQ.spq_flags[0])  {
		spitoption($A{sqchange post proc flags}, $A{sqchange explain}, dest, ' ', 0);
		dumpstr(dest, SPQ.spq_flags);
	}
	if  (range_changes)  {
		spitoption($A{sqchange page range}, $A{sqchange explain}, dest, ' ', 0);
		if  (SPQ.spq_start != 0)
			fprintf(dest, " %ld", SPQ.spq_start+1L);
		putc('-', dest);
		if  (SPQ.spq_end <= LOTSANDLOTS)
			fprintf(dest, "%ld", SPQ.spq_end+1L);
	}
	if  (oe_changes)  {
		spitoption($A{sqchange odd even}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %c",
			       SPQ.spq_jflags & SPQ_ODDP? (SPQ.spq_jflags & SPQ_REVOE? 'A': 'O'):
			       SPQ.spq_jflags & SPQ_EVENP? (SPQ.spq_jflags & SPQ_REVOE? 'B': 'E'): '-');
	}
	if  (pto_changes)  {
		spitoption($A{sqchange printed timeout}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %d", SPQ.spq_ptimeout);
	}
	if  (npto_changes)  {
		spitoption($A{sqchange not printed timeout}, $A{sqchange explain}, dest, ' ', 0);
		fprintf(dest, " %d", SPQ.spq_nptimeout);
	}
	if  (tdel_changes)  {
		spitoption($A{sqchange delay until}, $A{sqchange explain}, dest, ' ', 0);
		if  (SPQ.spq_hold == 0)
			fputs(" -", dest);
		else  {
			time_t  ht = SPQ.spq_hold;
			struct	tm	*tp = localtime(&ht);
			fprintf(dest, " %.2d/%.2d/%.2d,%.2d:%.2d:%.2d",
				       tp->tm_year % 100,
				       tp->tm_mon + 1,
				       tp->tm_mday,
				       tp->tm_hour,
				       tp->tm_min,
				       tp->tm_sec);
		}
	}
	putc('\n', dest);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
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

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	if  ((mypriv->spu_flgs & (PV_OTHERJ|PV_VOTHERJ)) != (PV_OTHERJ|PV_VOTHERJ))
		Realuname = prin_uname(Realuid);
	SWAP_TO(Realuid);
	argv = optprocess(argv, Adefs, optprocs, $A{sqchange explain}, $A{sqchange freeze home}, 0);
	SWAP_TO(Daemuid);

#define	FREEZE_EXIT
#include "inline/freezecode.c"

	if  (argv[0] == (char *) 0)  {
		print_error($E{sqchange no args});
		return  E_USAGE;
	}
	if  (!doing_something)  {
		print_error($E{sqchange no ops});
		return  E_USAGE;
	}

	/* Grab message id */

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		print_error($E{Spooler not running});
		return  E_NOTRUN;
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		return  E_SETUP;
	}
#endif

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	if  ((ret = init_xfershm(0)))  {
		print_error(ret);
		return  E_SETUP;
	}
	if  (!jobshminit(0))  {
		print_error($E{Cannot open jshm});
		return  E_JOBQ;
	}
	if  ((ret = msg_log(SO_MON, 1)) != 0)  {
		print_error(ret);
		return  E_SETUP;
	}
	rjobfile();
	process(&argv[0]);
	if  ((ret = msg_log(SO_DMON, 0)) != 0)  {
		print_error(ret);
		return  E_SETUP;
	}
	return  exit_code;
}
