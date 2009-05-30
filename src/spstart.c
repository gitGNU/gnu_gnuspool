/* spstart.c -- start up spooler/start printer etc

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
#include <errno.h>
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <sys/shm.h>
#include <stdio.h>
#include <ctype.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "xfershm.h"
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "displayopt.h"
#include "shutilmsg.h"

typedef	enum	{ START, HALT, STOP, INTER, OK, NOK, ADD, CHANGE, DEL, STAT_ENQ, CONN, DISCONN }  cmd_type;

int	spitoption(const int, const int, FILE *, const int, const int);
int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

FILE	*Cfile;
char	*Curr_pwd;

DEF_DISPOPTS;
int	network;
extern	char	freeze_wanted;
char	freeze_cd, freeze_hd;

#define	IPC_MODE	0600
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	xfershm		*Xfer_shmp;
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;

#define	START_COUNT	12
#define	SLEEP_TIME	10

#define	ERR_TAIL	8

/* Semaphore structures.  */

classcode_t	set_classcode;
struct	spdet	*mypriv;


char	*Line_name,
	*New_line_name,
	*Description;

int	Setlnet = 0,
	Setloco = 0,
	setc = 0;

static	cmd_type	cmd = START;

char	forceall = 0, waitcompl = 0;

static	char	Sufchars[] = DEF_SUFCHARS;

extern	char	hostf_errors;

static void	ptail(char *fname, int nlines)
{
	FILE	*ifl = fopen(fname, "r");
	int	cnt;
	long	*spos;
	char	inbuf[100];

	if  (!ifl)
		return;

	if  (!(spos = (long *) malloc((unsigned) (nlines * sizeof(long)))))
		nomem();

	for  (cnt = 0;  cnt < nlines;  cnt++)
		spos[cnt] = 0;

	while  (fgets(inbuf, sizeof(inbuf), ifl))  {
		for  (cnt = 1;  cnt < nlines;  cnt++)
			spos[cnt-1] = spos[cnt];
		spos[nlines - 1] = ftell(ifl);
	}

	fseek(ifl, spos[0], 0);
	while  (fgets(inbuf, sizeof(inbuf), ifl))
		fputs(inbuf, stderr);
	fclose(ifl);
	free((char *) spos);
}

/* Write message to scheduler.  */

void	womsg(const int act, const slotno_t sl)
{
	struct	spr_req	oreq;

	oreq.spr_mtype = MT_SCHED;
	oreq.spr_un.o.spr_act = (USHORT) act;
	oreq.spr_un.o.spr_pid = getpid();
	oreq.spr_un.o.spr_jpslot = sl;
	oreq.spr_un.o.spr_arg1 = Realuid;
	oreq.spr_un.o.spr_seq = 0;
	oreq.spr_un.o.spr_netid = 0;
	oreq.spr_un.o.spr_arg2 = 0;

	while  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)
		if  (errno != EINTR)  {
			print_error(errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error});
			exit(E_SETUP);
		}
}

void	my_wpmsg(const int act, const slotno_t sl, struct spptr *spp)
{
	int	ret;
	struct	spr_req	preq;

	preq.spr_mtype = MT_SCHED;
	preq.spr_un.p.spr_act = (USHORT) act;
	preq.spr_un.p.spr_pid = getpid();
	preq.spr_un.p.spr_pslot = sl;
	preq.spr_un.p.spr_seq = 0;
	preq.spr_un.p.spr_netid = 0;

	if  ((ret = wpmsg(&preq, spp)) != 0)  {
		print_error(ret);
		exit(E_SETUP);
	}
}

void	wexit(int n)
{
	msg_log(SO_DMON, 0);
	exit(n);
}

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	wexit(E_NOMEM);
}

/* Compare printer form type up to suffix with specified paper type. */

int	compare(const char *pform, const char *sform, const int slength)
{
	int	ret;

	if  ((ret = ncstrncmp(pform, sform, slength)) != 0)
		return  ret;
	if  (slength >= MAXFORM)
		return  0;
	return  strchr(Sufchars, pform[slength]) == (char *) 0;
}

/* Squeeze spaces & tabs out of thing */

char *	squeezesp(char *subj)
{
	char	*cp2 = subj, *cp3;

	while  (*cp2)  {
		if  (isspace(*cp2))
			for  (cp3 = cp2;  *cp3;  cp3++)
				cp3[0] = cp3[1];
		else
			cp2++;
	}
	return  subj;
}

void	actprin(char *printer, char *line, char *paper)
{
	const  Hashspptr  *cp;
	LONG	pind;
	int	*actlist;
	char	*colp;
	int	plng = 0, countv = 0, nmatch = 0, donep = 0, cnt;
	int	isparent, ecode, waitneeded, timesround = 0;
	netid_t	netid = 0L;
	struct	spptr	PREQ;

	/* Build up vector of printers which fit the criteria we give.

	   For delete and change we will later insist that only
	   one applies.

	   For add we want it to be zero (add 1 to allocation though).  */

	if  (!(actlist = (int *) malloc((Ptr_seg.nptrs + 1) * sizeof(int))))
		nomem();

	/* See if command is referring to remote printers */

	if  ((colp = strchr(printer, ':')))  {
		if  (!(mypriv->spu_flgs & PV_REMOTEP))  {
			print_error($E{No remote ptr priv});
			wexit(E_NOPRIV);
		}
		*colp = '\0';
		if  ((netid = look_hostname(printer)) == 0L)  {
			disp_str = printer;
			print_error($E{Unknown host name});
			wexit(E_BADPTR);
		}
		printer = colp + 1;
	}

	if  (paper != (char *) 0)  {
		char	*p;
		if  ((p = strpbrk(paper, Sufchars)) != (char *) 0)
			plng = p - paper;
		else
			plng = strlen(paper);
	}

	/* Count printers which match. */

	pind = Ptr_seg.dptr->ps_l_head;
	while  (pind >= 0L)  {
		cp = &Ptr_seg.plist[pind];
		pind = cp->l_nxt;
		if  (cp->p.spp_state == SPP_NULL  ||  cp->p.spp_netid != netid)
			continue;
		if  (!qmatch(printer, cp->p.spp_ptr))
			continue;
		nmatch++;
		if  (line  &&  !qmatch(line, cp->p.spp_dev))
			continue;
		actlist[countv++] = cp - Ptr_seg.plist;
	}

	if  (countv <= 0  &&  cmd != ADD)  {
		disp_str = printer;
		disp_str2 = line;
		print_error(nmatch > 0? $E{spstart no match printer name} : $E{spstart unknown printer});
		wexit(E_BADPTR);
	}

	/* Catch special cases... */

	switch  (cmd)  {
	case  ADD:		/* Nothing may clash */
		if  (netid)  {
			disp_str = look_host(netid);
			print_error($E{spstart cannot add remote printer});
			wexit(E_USAGE);
		}
		if  (countv > 0  &&  !(network & SPP_LOCALNET))  {
			disp_str = printer;
			disp_str2 = line;
			print_error($E{spstart device name clash});
			wexit(E_BADPTR);
		}
		BLOCK_ZERO(&PREQ, sizeof(PREQ));
		PREQ.spp_class = Displayopts.opt_classcode;
		PREQ.spp_netflags = (unsigned char) network;
		/* PREQ.spp_extrn = 0; */
		strncpy(PREQ.spp_dev, line, LINESIZE);
		strncpy(PREQ.spp_form, paper, MAXFORM);
		strncpy(PREQ.spp_ptr, printer, PTRNAMESIZE);
		if  (Description)
			strncpy(PREQ.spp_comment, Description, COMMENTSIZE);
		my_wpmsg(SP_ADDP, 0, &PREQ);
		wexit(0);

	case  DEL:		/* Only delete one */
		if  (netid)  {
			disp_str = look_host(netid);
			print_error($E{spstart cannot delete remote printer});
			wexit(E_BADPTR);
		}
		if  (countv > 1)  {
			disp_str = printer;
			disp_arg[0] = countv;
			print_error($E{spstart delete one only});
			wexit(E_BADPTR);
		}
		cp = &Ptr_seg.plist[actlist[0]];
		if  (cp->p.spp_state >= SPP_PROC)  {
			disp_str = cp->p.spp_ptr;
			print_error($E{spstart printer is running});
			wexit(E_RUNNING);
		}
		womsg(SO_DELP, cp - Ptr_seg.plist);
		wexit(0);

	case  STAT_ENQ:		/* Only enquire about one */
		if  (countv > 1)  {
			disp_str = printer;
			disp_arg[0] = countv;
			print_error($E{spstart identify printer});
			wexit(E_BADPTR);
		}
		cp = &Ptr_seg.plist[actlist[0]];

		/* If state not specified, return TRUE if running,
		 otherwise FALSE */

		if  (paper == (char *) 0)
			wexit(cp->p.spp_state >= SPP_PROC? E_TRUE: E_FALSE);

		/* Fetch prompt according to state printer is in If it
		   matches, return TRUE otherwise FALSE */

		colp = squeezesp(gprompt(cp->p.spp_state + $P{Printer status null}));
		paper = squeezesp(paper);
		wexit(qmatch(paper, colp)? E_TRUE: E_FALSE);

	case  CHANGE:		/* If changing device, only permit one */
		if  (New_line_name  &&  countv > 1)  {
			disp_str = printer;
			disp_arg[0] = countv;
			print_error($E{spstart identify printer});
			wexit(E_BADPTR);
		}
	default:		/* For C compilers which winge at missed enums */
		break;
	}

	if  (countv > 1  &&  !forceall)  {
		disp_str = printer;
		disp_arg[0] = countv;
		print_error($E{spstart needs force opt});
		wexit(E_BADPTR);
	}

	/* For spchange, spok, spnok, fire off changes and forget.  */

	switch  (cmd)  {
	case  OK:
	case  NOK:
		for  (cnt = 0;  cnt < countv;  cnt++)  {
			cp = &Ptr_seg.plist[actlist[cnt]];
			if  (cp->p.spp_state < SPP_PROC)
				continue;
			womsg(cmd == OK? SO_OYES: SO_ONO, cp - Ptr_seg.plist);
		}
		wexit(0);
	case  CHANGE:
		for  (cnt = 0;  cnt < countv;  cnt++)  {
			cp = &Ptr_seg.plist[actlist[cnt]];
			if  (cp->p.spp_state >= SPP_PROC)
				continue;
			PREQ = cp->p;
			if  (paper)  {
				strncpy(PREQ.spp_form, paper, MAXFORM);
				PREQ.spp_form[MAXFORM] = 0;
			}
			if  (Description)  {
				strncpy(PREQ.spp_comment, Description, COMMENTSIZE);
				PREQ.spp_comment[COMMENTSIZE] = 0;
			}
			if  (New_line_name)  {
				strncpy(PREQ.spp_dev, New_line_name, LINESIZE);
				PREQ.spp_dev[LINESIZE] = 0;
			}
			if  (Setlnet)  {
				PREQ.spp_netflags &= ~SPP_LOCALNET;
				PREQ.spp_netflags |= network & SPP_LOCALNET;
			}
			if  (Setloco)  {
				PREQ.spp_netflags &= ~SPP_LOCALONLY;
				PREQ.spp_netflags |= network & SPP_LOCALONLY;
			}
			if  (setc)
				PREQ.spp_class = set_classcode;
			my_wpmsg(SP_CHGP, cp - Ptr_seg.plist, &PREQ);
		}
		exit(0);
	default:
		break;		/* Keep some C compilers happy */
	}

	/* Start or halt cases.  May wait to complete operation.  */

	isparent = -1;
	ecode = 0;

	while  (donep < countv)  {

		waitneeded = 0;		/* Set to indicate that we have steps to go we MUST wait for */

		for  (cnt = 0;  cnt < countv;  cnt++)  {

			/* Set actlist[n] to -1 once we're done */

			if  (actlist[cnt] < 0)
				continue;

			cp = &Ptr_seg.plist[actlist[cnt]];

			switch  (cp->p.spp_state)  {
			case  SPP_NULL:
				actlist[cnt] = -1;
				donep++;
				continue;

			case  SPP_OFFLINE:
			case  SPP_ERROR:
			case  SPP_HALT:
				if  (cmd != START)  {
					actlist[cnt] = -1;
					donep++;
					continue;
				}

				/* Reset form type?  */

				if  (paper  &&  compare(cp->p.spp_form, paper, plng))  {
					PREQ = cp->p;
					strncpy(PREQ.spp_form, paper, MAXFORM);
					PREQ.spp_form[MAXFORM] = 0;
					my_wpmsg(SP_CHGP, cp - Ptr_seg.plist, &PREQ);
					waitneeded++;
					continue;
				}

				/* Kick it off */

				womsg(SO_PGO, cp - Ptr_seg.plist);
				donep++;
				actlist[cnt] = -1;
				continue;

			case  SPP_INIT:

				/* If just started up with a different
				   form type, it seems a pity to
				   spoil it don't you think.  */

				switch  (cmd)  {
				case  START:
					if  (paper  &&  compare(cp->p.spp_form, paper, plng))  {
						disp_str = printer;
						disp_str2 = cp->p.spp_form;
						print_error($E{spstart just started up});
						ecode = E_BADFORM;
					}
				case  INTER:
					donep++;
					actlist[cnt] = -1;
					continue;
				case  HALT:
					womsg(SO_PHLT, cp - Ptr_seg.plist);
					continue;
				case  STOP:
					womsg(SO_PSTP, cp - Ptr_seg.plist);
					continue;
				default:
					continue; /* Not reached I don't think */
				}

			case  SPP_RUN:
			case  SPP_WAIT:
			case  SPP_OPER:
				switch  (cmd)  {
				case  START:
					if  (paper  &&  compare(cp->p.spp_form, paper, plng))  {
						if  (!(cp->p.spp_sflags & SPP_HEOJ))
							womsg(SO_PHLT, cp - Ptr_seg.plist);
						waitneeded++;
						continue;
					}
					else  if  (cp->p.spp_sflags & SPP_HEOJ)		/* Un-heoj */
						womsg(SO_PGO, cp - Ptr_seg.plist);
					donep++;
					actlist[cnt] = -1;
					continue;
				case  HALT:
					womsg(SO_PHLT, cp - Ptr_seg.plist);
					donep++;
					actlist[cnt] = -1;
					continue;
				case  STOP:
					womsg(SO_PSTP, cp - Ptr_seg.plist);
					donep++;
					actlist[cnt] = -1;
					continue;
				case  INTER:
					womsg(SO_INTER, cp - Ptr_seg.plist);
					donep++;
					actlist[cnt] = -1;
					continue;
				default:
					continue; /* Not reached */
				}

			case  SPP_SHUTD:
				switch  (cmd)  {
				case  START:
					if  (timesround <= 0)  {
						disp_str = printer;
						print_error($E{spstart shutting down});
						ecode = E_SHUTD;
						donep++;
						actlist[cnt] = -1;
						continue;
					}
					waitneeded++;
					continue;
				case  HALT:
					continue;
				case  STOP:		 /* Desperate measures */
					womsg(SO_PSTP, cp - Ptr_seg.plist);
					continue;
				case  INTER:
					donep++;
					actlist[cnt] = -1;
					continue;
				default:
					continue; /* Not reached */
				}
			}

			/* If we had an error somewhere, give up.  */

			if  (ecode)
				wexit(ecode);

			if  (cmd == START)  {
				if  (waitneeded  &&  !waitcompl  &&  isparent)  {

					/* This could take a while, so
					   get a child process to
					   do it and go away.  */

					if  ((isparent = fork()) != 0)
						wexit(0);

					/* Start again as child process.
					   Don't wait for first msg */

					if  ((ecode = msg_log(SO_MON, 0)) != 0)  {
						print_error(ecode);
						exit(E_SETUP);
					}
				}

				if  (donep < countv)
					waitsig();
			}
			else
				waitsig();
		}
		timesround++;
	}
	wexit(0);
}

#ifdef	NETWORK_VERSION
static	int	doconnop(const char *hostn)
{
	struct	remote	*rp;
	struct	spr_req	oreq;

	disp_str = hostn;
	while  ((rp = get_hostfile()))
		if  (!(rp->ht_flags & HT_ROAMUSER)  &&  (strcmp(hostn, rp->hostname) == 0  ||  strcmp(hostn, rp->alias) == 0))
			goto  found;
	end_hostfile();
	if  (hostf_errors)
		print_error($E{Warn errors in host file});
	print_error($E{Connop unknown host name});
	return  E_NOHOST;
 found:

	if  (hostf_errors)
		print_error($E{Warn errors in host file});

	/* Don't allow connections to client machines */

	if  (rp->ht_flags & HT_DOS)  {
		print_error($E{Connop is client});
		return  E_NOHOST;
	}
	oreq.spr_mtype = MT_SCHED;
	oreq.spr_un.n.spr_act = cmd == CONN? SON_CONNECT: SON_DISCONNECT;
	oreq.spr_un.n.spr_seq = 0;
	oreq.spr_un.n.spr_pid = getpid();
	oreq.spr_un.n.spr_n = *rp;
	oreq.spr_un.n.spr_n.ht_flags &= ~HT_MANUAL;
	if  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_nmsg), IPC_NOWAIT) < 0)  {
		print_error(errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error});
		exit(E_SETUP);
	}
	return  0;
}
#endif

#define	INLINE_SPSTART
OPTION(o_explain)
{
	print_error($E{spstart options} + (int) cmd);
	exit(0);
}

OPTION(o_network)
{
	network |= SPP_LOCALNET;
	Setlnet++;
	return  OPTRESULT_OK;
}

OPTION(o_nonetwork)
{
	network &= ~SPP_LOCALNET;
	Setlnet++;
	return  OPTRESULT_OK;
}

OPTION(o_localonly)
{
	network |= SPP_LOCALONLY;
	Setloco++;
	return  OPTRESULT_OK;
}

OPTION(o_nolocalonly)
{
	network &= ~SPP_LOCALONLY;
	Setloco++;
	return  OPTRESULT_OK;
}

OPTION(o_line)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (Line_name)
		free(Line_name);
	Line_name = stracpy(arg);
	return  OPTRESULT_ARG_OK;
}

OPTION(o_newdevice)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (New_line_name)
		free(New_line_name);
	New_line_name = stracpy(arg);
	return  OPTRESULT_ARG_OK;
}

OPTION(o_description)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (Description)
		free(Description);
	Description = stracpy(arg);
	return  OPTRESULT_ARG_OK;
}

OPTION(o_forceall)
{
	forceall = 1;
	return  OPTRESULT_OK;
}

OPTION(o_noforce)
{
	forceall = 0;
	return  OPTRESULT_OK;
}

OPTION(o_waitcompl)
{
	waitcompl = 1;
	return  OPTRESULT_OK;
}

OPTION(o_nowait)
{
	waitcompl = 0;
	return  OPTRESULT_OK;
}

#include "inline/o_classc.c"
#include "inline/o_setclass.c"
#include "inline/o_freeze.c"

/* Defaults and proc table for arg interp.  */

static	const	Argdefault	Adefs[] = {
  {  '?', $A{spstart explain}	},
  {  'N', $A{spstart network}	},
  {  'L', $A{spstart line}	},
  {  's', $A{spstart local}	},
  {  'w', $A{spstart remotes}	},
  {  'l', $A{spstart device}	},
  {  'C', $A{spstart classcode}	},
  {  'D', $A{spstart description} },
  {  'v', $A{spstart newdevice} },
  {  'S', $A{spstart newclass}	},
  {  'f', $A{spstart forceall}	},
  {  'n', $A{spstart noforce}	},
  {  'W', $A{spstart wait}	},
  {  'E', $A{spstart exit}	},
  {  0, 0 }
};

optparam  optprocs[] = {
o_explain,	o_network,	o_nonetwork,	o_localonly,
o_nolocalonly,	o_line,		o_classcode,	o_description,
o_newdevice,	o_setclass,	o_forceall,	o_noforce,
o_waitcompl,	o_nowait,	o_freezecd,	o_freezehd
};

void	spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;

	fprintf(dest, "%s", name);
	cancont = spitoption(forceall? $A{spstart forceall}: $A{spstart noforce},
			     $A{spstart explain}, dest, '=', cancont);
	cancont = spitoption(waitcompl? $A{spstart wait}: $A{spstart exit},
			     $A{spstart explain}, dest, ' ', cancont);
	if  (Setlnet)
		cancont = spitoption(network & SPP_LOCALNET?
				     $A{spstart network}: $A{spstart line},
				     $A{spstart explain}, dest, ' ', cancont);
	if  (Setloco)
		cancont = spitoption(network & SPP_LOCALONLY?
				     $A{spstart local}: $A{spstart remotes},
				     $A{spstart explain}, dest, ' ', cancont);
	if  (Line_name)  {
		spitoption($A{spstart device}, $A{spstart explain}, dest, ' ', 0);
		fprintf(dest, " %s", Line_name);
	}
	if  (New_line_name)  {
		spitoption($A{spstart newdevice}, $A{spstart explain}, dest, ' ', 0);
		fprintf(dest, " %s", New_line_name);
	}
	spitoption($A{spstart classcode}, $A{spstart explain}, dest, ' ', 0);
	fprintf(dest, " %s\n", hex_disp(Displayopts.opt_classcode, 0));
	if  (setc)  {
		spitoption($A{spstart newclass}, $A{spstart explain}, dest, ' ', 0);
		fprintf(dest, " %s\n", hex_disp(set_classcode, 0));
	}
	if  (Description)  {
		spitoption($A{spstart description}, $A{spstart explain}, dest, ' ', 0);
		fprintf(dest, " \'%s\'\n", Description);
	}
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	char	*pname, *papname = (char *) 0, *dir;
	const	char	*str;
	int		started = 0, ret;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	str = strchr(progname, '-'); /* Past the gspl- */
	if  (str)
		str++;
	else
		str = progname;

	init_mcfile();

	if  (strcmp(str, "phalt") == 0)
		cmd = HALT;
	else  if  (strcmp(str, "pstop") == 0)
		cmd = STOP;
	else  if  (strcmp(str, "pinter") == 0)
		cmd = INTER;
	else  if  (strcmp(str, "ok") == 0)
		cmd = OK;
	else  if  (strcmp(str, "nok") == 0)
		cmd = NOK;
	else  if  (strcmp(str, "padd") == 0)
		cmd = ADD;
	else  if  (strcmp(str, "pdel") == 0)
		cmd = DEL;
	else  if  (strcmp(str, "pchange") == 0)
		cmd = CHANGE;
	else  if  (strcmp(str, "pstat") == 0)
		cmd = STAT_ENQ;
#ifdef	NETWORK_VERSION
	else  if  (strcmp(str, "conn") == 0)
		cmd = CONN;
	else  if  (strcmp(str, "disconn") == 0)
		cmd = DISCONN;
#endif

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	SWAP_TO(Realuid);
	argv = optprocess(argv, Adefs, optprocs, $A{spstart explain}, $A{spstart freeze home}, 0);
	SWAP_TO(Daemuid);

#define	FREEZE_EXIT
#include "inline/freezecode.c"

	if  ((pname = *argv++) == (char *) 0)  {
		if  (cmd != START)  {
Usage:			print_error($E{spstart no args given});
			exit(E_USAGE);
		}
	}
	else  if  ((papname = *argv)  &&  *++argv != (char *) 0)
		goto  Usage;

	switch  (cmd)  {
#ifdef	NETWORK_VERSION
	case  CONN:
	case  DISCONN:
		if  (!(mypriv->spu_flgs & PV_SSTOP))  {
			print_error($E{spstart no conn/disconn priv});
			exit(E_NOPRIV);
		}
		break;
#endif
	case  DEL:
		if  (!(mypriv->spu_flgs & PV_ADDDEL))  {
			print_error($E{No delete priv});
			exit(E_NOPRIV);
		}
		break;
	case  ADD:
		if  (!(mypriv->spu_flgs & PV_ADDDEL))  {
			print_error($E{spstart no add priv});
			exit(E_NOPRIV);
		}
		if  (!Line_name)  {
			print_error($E{No line given});
			exit(E_USAGE);
		}
		if  (!papname)
			papname = mypriv->spu_form;
		break;
	case  CHANGE:
		if  (!(mypriv->spu_flgs & PV_ADDDEL))  {
			print_error($E{spstart no change priv});
			exit(E_NOPRIV);
		}
		if  (!papname  &&  !New_line_name  &&  !Description  &&  !Setlnet  &&  !Setloco  &&  !setc)  {
			print_error($E{No changes given});
			exit(E_USAGE);
		}
		break;
	case  START:
	case  HALT:
	case  STOP:
	case  INTER:
		if  (!(mypriv->spu_flgs & PV_HALTGO))  {
			print_error($E{No stop start priv});
			exit(E_NOPRIV);
		}
		break;
	case  OK:
	case  NOK:
		if  (!(mypriv->spu_flgs & PV_PRINQ))  {
			print_error($E{No ptr select priv});
			exit(E_NOPRIV);
		}
	case  STAT_ENQ:		/* Keep C compilers happy */
		break;
	}

	/* Change directory.
	   Open control MSGID, starting scheduler process if necessary.  */

	dir = envprocess(SPDIR);


	if  (chdir(dir) < 0)  {
		print_error($E{Cannot chdir});
		exit(E_NOCHDIR);
	}

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		int	code;
		PIDTYPE	pid, rpid;

		if  (errno == EACCES)  {
			print_error($E{Check file setup});
			exit(E_SETUP);
		}

		/* Not really an error, but easiest done this way.  */

		print_error($E{Restarting scheduler});

		while  ((pid = fork()) < 0)  {
			print_error($E{Fork wait});
			sleep(SLEEP_TIME);
		}

		if  (pid == 0)  {	/*  Child process  */
			char	*spshed = envprocess(SPSHED);
			execl(spshed, spshed, pname, papname, (char *) 0);
			exit(255);
		}

		/* Main path of scheduler exits at once with ipc created.  */

#ifdef	HAVE_WAITPID
		while  ((rpid = waitpid(pid, &code, 0)) < 0  &&  errno == EINTR)
			;
#else
		while  ((rpid = wait(&code)) != pid  &&  (rpid >= 0 || errno == EINTR))
			;
#endif

		if  (rpid < 0 || (code != 0  &&  code != (E_FALSE << 8)))  {
			print_error($E{Could not start scheduler});
			exit(code >> 8);
		}

		/* Have another go at opening msg chan */

		if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
			char	*f = REPFILE, *ff;
			if  (!(ff = malloc((unsigned) (strlen(dir) + strlen(f) + 2))))
				nomem();
			sprintf(ff, "%s/%s", dir, f);
			disp_str = ff;
			print_error($E{Still could not open});
			ptail(f, ERR_TAIL);
			exit(E_SETUP);
		}

#ifdef	NETWORK_VERSION

		if  (code == 0)  { /* I.e. E_TRUE */

			/* Restart xtnetserv as well.  */

			while  ((pid = fork()) < 0)  {
				print_error($E{Fork wait});
				sleep(SLEEP_TIME);
			}

			if  (pid == 0)  {	/*  Child process  */
				char	*xtnetserv = envprocess(XTNETSERV);
				execl(xtnetserv, xtnetserv, (char *) 0);
				exit(255);
			}

			/* Main path of xtnetserv exits at once */

#ifdef	HAVE_WAITPID
			while  ((rpid = waitpid(pid, &code, 0)) < 0  &&  errno == EINTR)
				;
#else
			while  ((rpid = wait(&code)) != pid  &&  (rpid >= 0 || errno == EINTR))
				;
#endif
		}
#endif
		started++;

		/* If just starting, then exit.
		   This also applies if a digit version of pname is given.  */

		if  (cmd == START  &&  (!pname || isdigit(pname[0])))
			exit(0);
	}

	/* If just starting, then exit.
	   This does not apply to digit-only pnames. */

	if  (cmd == START  &&  !pname)
		exit(0);

#ifdef	NETWORK_VERSION
	if  (cmd == CONN  ||  cmd == DISCONN)
		return  doconnop(pname);
#endif
	if  ((ret = msg_log(SO_MON, 1)) != 0)  {
		print_error(ret);
		return  E_SETUP;
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		wexit(E_SETUP);
	}
#endif

	if  ((ret = init_xfershm(1)))  {
		print_error(ret);
		exit(E_SETUP);
	}

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	if  (!jobshminit(1))  {
		if  (started)  { /* Give it time to digest */
			int  cnt;
			for  (cnt = 0;  cnt < START_COUNT;  cnt++)  {
				print_error($E{Wait to come up});
				sleep(SLEEP_TIME);
				if  (jobshminit(1))
					goto  sok;
			}
		}
		print_error($E{Cannot open jshm});
		wexit(E_JOBQ);
	}
 sok:
	if  (!ptrshminit(1))  {
		print_error($E{Cannot open pshm});
		wexit(E_PRINQ);
	}
	readptrlist(1);
	actprin(pname, Line_name, papname);
	return  0;		/* Not really reached */
}
