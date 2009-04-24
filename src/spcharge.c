/* spcharge.c -- charges program

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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>

#include "defaults.h"
#include "files.h"
#include "spuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "helpargs.h"
#include "network.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "ipcstuff.h"
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"
#include "displayopt.h"

FILE	*Cfile;

extern	char	freeze_wanted;
char	freeze_cd, freeze_hd;
char	*Curr_pwd;
uid_t	Realuid, Effuid, Daemuid;

/* Standard Xi-Text stuff */

int	spitoption(const int, const int, FILE *, const int, const int);
int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

int	nerrors	= 0,
	zero_usage = 0,
	print_usage = 0,
	consolidate = 0,
	reset_file = 0;

LONG	chargefee = 0L;

char	*file_name;

unsigned	num_users, hashed_users;
int_ugid_t	*user_list;

struct	huid	{
	struct	huid	*next;
	char		*uname;
	int_ugid_t	uid;
	int		wanted;
	double		charge;
};

#define	HASHMOD	97
static	struct	huid	*uhash[HASHMOD];

int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
DEF_DISPOPTS;

/* This is needed by the standard error handling stuff in the library */

void	nomem(void)
{
	print_error($E{spcharge no memory});
	exit(E_NOMEM);
}

/* Generate list of user ids from argument list.  */

static void	translate_users(char **ulist)
{
	int_ugid_t	ru, *lp;
	char	**up;

	/* Count them...  */

	for  (up = ulist;  *up;  up++)
		num_users++;

	if  (num_users == 0)
		return;

	if  (!(user_list = (int_ugid_t *) malloc(num_users * sizeof(int_ugid_t))))
		nomem();

	lp = user_list;

	for  (up = ulist;  *up;  up++)
		if  ((ru = lookup_uname(*up)) == UNKNOWN_UID)  {
			disp_str = *up;
			print_error($E{spcharge unknown user});
			nerrors++;
			num_users--;
		}
		else
			*lp++ = ru;

	/* If they're all dead, stop */

	if  (num_users == 0)
		exit(E_NOUSER);
}

/* Open file as required */

static	int	grab_file(const int omode)
{
	int	ret;

	if  ((ret = open(file_name, omode)) < 0)  {
		print_error(errno == EACCES? $E{Check file setup}: $E{No charges file yet});
		exit(E_SETUP);
	}
	return  ret;
}

/* Impose fee. If no arguments are given, it must be on everyone */

static	void	impose_fee(void)
{
	int	fd = grab_file(O_WRONLY|O_APPEND);
	struct	spcharge	spc;

	time(&spc.spch_when);
	spc.spch_host = 0;		/* Me */
	spc.spch_pri = 0;
	spc.spch_chars = 0;
	spc.spch_cpc = chargefee;

	if  (num_users == 0)  {
		spc.spch_user = -1;
		spc.spch_what = SPCH_FEEALL;
		write(fd, (char *) &spc, sizeof(spc));
	}
	else  {
		unsigned  uc;
		spc.spch_what = SPCH_FEE;
		for  (uc = 0;  uc < num_users;  uc++)  {
			spc.spch_user = user_list[uc];
			write(fd, (char *) &spc, sizeof(spc));
		}
	}
	close(fd);
}

/* Calculate hash and allocate hash chain item.  */

static	struct huid  *rhash(const int_ugid_t uid, const int had)
{
	struct	huid	*rp, **rpp;

	for  (rpp = &uhash[(ULONG) uid % HASHMOD]; (rp = *rpp);  rpp = &rp->next)
		if  (uid == rp->uid)  {
			rp->wanted |= had;
			return  rp;
		}
	if  (!(rp = (struct huid *) malloc(sizeof(struct huid))))
		nomem();
	rp->next = (struct huid *) 0;
	rp->uid = uid;
	rp->uname = (char *) 0;
	rp->wanted = had;
	rp->charge = 0;
	*rpp = rp;
	hashed_users++;
	return  rp;
}

/* Read in charge file and hash it up by user id and calculate charge.  */

static	void	buffer_up(const int had)
{
	int	fd = grab_file(O_RDONLY);
	unsigned  hp;
	struct	huid	*up;
	double	curr_chargeall = 0;
	struct	spcharge	spc;

	while  (read(fd, (char *) &spc, sizeof(spc)) == sizeof(spc))  {
		double	res;

		switch  (spc.spch_what)  {
		case  SPCH_RECORD:	/* Record left by spshed */
			up = rhash(spc.spch_user, had);
			res = spc.spch_pri;
			res /= U_DF_DEFP;
			up->charge += res * res * spc.spch_chars * spc.spch_cpc / 1E6;
			break;

		case  SPCH_FEE:			/* Impose fee */
			up = rhash(spc.spch_user, had);
			up->charge += (double) spc.spch_cpc;
			break;

		case  SPCH_FEEALL:		/* Impose fee to all*/
			curr_chargeall += (double) spc.spch_cpc;
			break;

		case  SPCH_CONSOL:		/* Consolidation of previous charges */
			up = rhash(spc.spch_user, had);
			up->charge = (double) spc.spch_cpc - curr_chargeall;
			break;

		case  SPCH_ZERO:		/* Zero record for given user */
			up = rhash(spc.spch_user, had);
			up->charge = -curr_chargeall; /* To cancel effect when added later */
			break;

		case  SPCH_ZEROALL:		/* Zero record for all users */
			curr_chargeall = 0;
			for  (hp = 0;  hp < HASHMOD;  hp++)
				for  (up = uhash[hp];  up;  up = up->next)
					up->charge = 0;
			break;
		}
	}

	if  (curr_chargeall != 0.0)
		for  (hp = 0;  hp < HASHMOD;  hp++)
			for  (up = uhash[hp];  up;  up = up->next)
				up->charge += curr_chargeall;
	close(fd);
}

/* A sort of sort routine for qsort */

static  int  sort_u(struct huid **a, struct huid **b)
{
	return  strcmp((*a)->uname, (*b)->uname);
}

/* Another sort of sort routine for qsort */

static	int	sort_uid(struct huid **a, struct huid **b)
{
	int_ugid_t	au = (*a)->uid, bu = (*b)->uid;
	return  au < bu? -1: au == bu? 0: 1;
}

/* Print out stuff.  */

static	void	do_print(void)
{
	unsigned  uc, wanted_users = 0;
	struct	huid	*hp, **sp, **sorted_list;

	if  (num_users == 0)
		buffer_up(1);
	else  {
		buffer_up(0);
		for  (uc = 0;  uc < num_users;  uc++)
			rhash(user_list[uc], 1);
	}

	/* Skip if file completely empty.  */

	if  (hashed_users == 0)
		return;

	/* Maybe we'll make this "wanted_users" rather than
	   "hashed_users" if this overflows, but we'll leave it for now.  */

	if  (!(sorted_list = (struct huid **) malloc(hashed_users * sizeof(struct huid))))
		nomem();

	sp = sorted_list;

	for  (uc = 0;  uc < HASHMOD;  uc++)
		for  (hp = uhash[uc];  hp;  hp = hp->next)
			if  (hp->wanted)  {
				wanted_users++;
				*sp++ = hp;
				hp->uname = prin_uname(hp->uid);
			}

	qsort(QSORTP1 sorted_list, wanted_users, sizeof(struct huid *), QSORTP4 sort_u);

	/* Print the summary.  */

	for  (uc = 0;  uc < wanted_users;  uc++)  {
		disp_str = sorted_list[uc]->uname;
		disp_arg[1] = (LONG) sorted_list[uc]->charge;
		fprint_error(stdout, $E{spcharge summary format});
	}
	free((char *) sorted_list);
}

static	void	do_printall(void)
{
	int	fd = grab_file(O_RDONLY);
	struct	spcharge	spc;

	while  (read(fd, (char *) &spc, sizeof(spc)) == sizeof(spc))  {
		disp_arg[0] = spc.spch_user;
		disp_arg[1] = spc.spch_pri;
		disp_arg[2] = spc.spch_chars;
		disp_arg[3] = spc.spch_cpc;
		disp_arg[4] = spc.spch_when;

		switch  (spc.spch_what)  {
		default:
			disp_arg[0] = spc.spch_what;
			print_error($E{spcharge unexpected record});
			break;

		case  SPCH_RECORD:	/* Record left by spshed */
			if  (spc.spch_host)  {
				disp_str = look_host(spc.spch_host);
				fprint_error(stdout, $E{spcharge print record format host});
			}
			else
				fprint_error(stdout, $E{spcharge print record format});
			break;

		case  SPCH_FEE:			/* Impose fee */
			fprint_error(stdout, $E{spcharge fee format});
			break;

		case  SPCH_FEEALL:		/* Impose fee to all*/
			fprint_error(stdout, $E{spcharge feeall format});
			break;

		case  SPCH_CONSOL:		/* Consolidation of previous charges */
			fprint_error(stdout, $E{spcharge consol format});
			break;

		case  SPCH_ZERO:		/* Zero record for given user */
			fprint_error(stdout, $E{spcharge zero format});
			break;

		case  SPCH_ZEROALL:		/* Zero record for all users */
			fprint_error(stdout, $E{spcharge zeroall format});
			break;
		}
	}
	close(fd);
}

static	struct huid **get_sorted(void)
{
	unsigned	uc;
	struct	huid	*hp, **sp, **sorted_list;

	if  (hashed_users == 0)  {
		buffer_up(0);
		if  (hashed_users == 0)
			return  (struct huid **) 0;
	}
	if  (!(sorted_list = (struct huid **) malloc(hashed_users * sizeof(struct huid))))
		nomem();
	sp = sorted_list;
	for  (uc = 0;  uc < HASHMOD;  uc++)
		for  (hp = uhash[uc];  hp;  hp = hp->next)
			*sp++ = hp;
	qsort(QSORTP1 sorted_list, hashed_users, sizeof(struct huid *), QSORTP4 sort_uid);
	return  sorted_list;
}

static	void	write_consol(struct huid **sorted_list, const int omode)
{
	int		fd;
	unsigned	uc;
	struct  spcharge	orec;

	time(&orec.spch_when);
	orec.spch_host = 0;
	orec.spch_pri = 0;
	orec.spch_what = SPCH_CONSOL;
	orec.spch_chars = 0;

	fd = grab_file(omode);
	for  (uc = 0;  uc < hashed_users;  uc++)  {
		if  ((orec.spch_cpc = (LONG) sorted_list[uc]->charge) == 0)
			continue;
		orec.spch_user = sorted_list[uc]->uid;
		write(fd, (char *) &orec, sizeof(orec));
	}
	close(fd);
	free((char *) sorted_list);
}

static void	do_consol(void)
{
	struct	huid	**sorted_list = get_sorted();
	if  (!sorted_list)
		return;
	write_consol(sorted_list, O_WRONLY|O_APPEND);
}

static	void	do_zero(void)
{
	int	fd = grab_file(O_WRONLY|O_APPEND);
	struct	spcharge	spc;

	time(&spc.spch_when);
	spc.spch_host = 0;		/* Me */
	spc.spch_pri = 0;
	spc.spch_chars = 0;
	spc.spch_cpc = 0;

	/* Any previous thing is invalid in case we do reset.  */

	if  (hashed_users != 0)  {
		unsigned  uc;
		struct	huid  *rp, *np;
		for  (uc = 0;  uc < HASHMOD;  uc++)  {
			for  (rp = uhash[uc];  rp;  rp = np)  {
				np = rp->next;
				free((char *) rp);
			}
			uhash[uc] = (struct huid *) 0;
		}
		hashed_users = 0;
	}

	if  (num_users == 0)  {
		spc.spch_user = -1;
		spc.spch_what = SPCH_ZEROALL;
		write(fd, (char *) &spc, sizeof(spc));
	}
	else  {
		unsigned  uc;
		spc.spch_what = SPCH_ZERO;
		for  (uc = 0;  uc < num_users;  uc++)  {
			spc.spch_user = user_list[uc];
			write(fd, (char *) &spc, sizeof(spc));
		}
	}
	close(fd);
}

static	void	do_reset(void)
{
	struct	huid	**sorted_list;
	if  (msgget(MSGID, 0) >= 0)  {
		print_error($E{spcharge spooler running});
		exit(E_RUNNING);
	}
	sorted_list = get_sorted();
	if  (!sorted_list)
		return;
	write_consol(sorted_list, O_WRONLY|O_TRUNC|O_APPEND);
}

OPTION(o_explain)
{
	print_error($E{spcharge options});
	exit(0);
}

OPTION(o_print)
{
	print_usage = 1;
	return  OPTRESULT_OK;
}

OPTION(o_printfull)
{
	print_usage = 2;
	return  OPTRESULT_OK;
}

OPTION(o_zero)
{
	zero_usage = 1;
	return  OPTRESULT_OK;
}

OPTION(o_charge)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  ((chargefee = atol(arg)) <= 0L)  {
		disp_str = arg;
		print_error($E{spcharge bad amount});
		exit(E_USAGE);
	}
	return  OPTRESULT_ARG_OK;
}

OPTION(o_consolidate)
{
	consolidate = 1;
	return  OPTRESULT_OK;
}

OPTION(o_resetfile)
{
	reset_file = 1;
	return  OPTRESULT_OK;
}

OPTION(o_cancelflags)
{
	zero_usage = 0;
	print_usage = 0;
	consolidate = 0;
	reset_file = 0;
	return  OPTRESULT_OK;
}

#include "inline/o_freeze.c"

/* Defaults and proc table for arg interp.  */

static	const	Argdefault  Adefs[] = {
	{  '?', $A{spcharge explain}	},
	{  'p', $A{spcharge print}	},
	{  'P', $A{spcharge full}	},
	{  'z', $A{spcharge zero}	},
	{  'c', $A{spcharge add}	},
	{  'C', $A{spcharge consol}	},
	{  'R', $A{spcharge reset}	},
	{  'K', $A{spcharge cancel}	},
	{ 0, 0 }
};

optparam  optprocs[] = {
o_explain,	o_print,	o_printfull,	o_zero,
o_charge,	o_consolidate,	o_resetfile,	o_cancelflags,
o_freezecd,	o_freezehd
};

void	spit_options(FILE *dest, const char *name)
{
	int	cancont;
	fprintf(dest, "%s", name);
	cancont = spitoption($A{spcharge cancel}, $A{spcharge explain}, dest, '=', 0);
	if  (print_usage)
		cancont = spitoption(print_usage > 1?
				     $A{spcharge full}:
				     $A{spcharge print}, $A{spcharge explain}, dest, ' ', cancont);
	if  (zero_usage)
		cancont = spitoption($A{spcharge zero}, $A{spcharge explain}, dest, ' ', cancont);
	if  (reset_file)
		cancont = spitoption($A{spcharge reset}, $A{spcharge explain}, dest, ' ', cancont);
	if  (consolidate)
		cancont = spitoption($A{spcharge consol}, $A{spcharge explain}, dest, ' ', cancont);
	if  (chargefee)  {
		spitoption($A{spcharge add}, $A{spcharge explain}, dest, ' ', 0);
		fprintf(dest, " %ld", (long) chargefee);
	}
	putc('\n', dest);
}

MAINFN_TYPE	main(int argc, char **argv)
{
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif
	struct	spdet	*mypriv;

	versionprint(argv, "$Revision: 1.2 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];
	init_mcfile();
	file_name = envprocess(CHFILE);

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(MISC_UCONFIG, "rest.help");
	SCRAMBLID_CHECK
	argv = optprocess(argv, Adefs, optprocs, $A{spcharge explain}, $A{spcharge freeze home}, 0);

	/* No options defaults to -p */

	if  (chargefee == 0L  &&  zero_usage + print_usage + consolidate + reset_file == 0)
		print_usage = 1;

	SWAP_TO(Daemuid);

	mypriv = getspuser(Realuid);
	if  (!(mypriv->spu_flgs & PV_ADMIN))  {
		print_error($E{shell no admin file priv});
		exit(E_NOPRIV);
	}

#define	FREEZE_EXIT
#include "inline/freezecode.c"

	/* First impose any charges
	   Then do any printing.
	   Then do any consolidation.
	   Then reset file. */

	translate_users(&argv[0]);

	if  (chargefee)
		impose_fee();
	if  (print_usage)  {
		if  (print_usage > 1)
			do_printall();
		else
			do_print();
	}
	if  (consolidate)
		do_consol();
	if  (zero_usage)
		do_zero();
	if  (reset_file)
		do_reset();
	return  nerrors;
}
