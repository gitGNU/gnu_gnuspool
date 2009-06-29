/* spuchange.c -- shell-level user permission setting

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

#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_sig.h"
#include <pwd.h>
#include <sys/stat.h>
#include <curses.h>
#include <ctype.h>
#include "defaults.h"
#include "spuser.h"
#include "ecodes.h"
#include "files.h"
#include "helpargs.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"

unsigned Nusers;
extern	struct	sphdr	Spuhdr;
struct	spdet	*ulist;

char	*Curr_pwd;

static	char	set_default,
		copyall,
		priv_setting,
		abs_priv,
		class_set,
		rebuild_file,
		freeze_cd,
		freeze_hd;

static	int	admin_privs,	/* Number of ops which require admin priv */
		cprio_privs;	/* Number of ops which require cprio priv */

static	enum  { DEFAULT_PW, DO_DUMP_PW, KILL_PW } dump_opt = DEFAULT_PW;

extern	char	freeze_wanted;

static	unsigned  char	min_p,
			max_p,
			def_p,
			max_cps;

static	ULONG	set_flags = 0,
		reset_flags = ALLPRIVS;

static	classcode_t	classcode;

static	char	*def_form, *def_formallow, *def_ptr, *def_ptrallow;
static	char	*allname;

struct	perm	{
	int	number;
	char	*string;
	ULONG	flg, sflg, rflg;
}  ptab[] = {
	{ $P{Priv adm},		(char *) 0, PV_ADMIN,	ALLPRIVS,	~PV_ADMIN },
	{ $P{Priv stp},		(char *) 0, PV_SSTOP,	PV_SSTOP,	~PV_SSTOP },
	{ $P{Priv form},	(char *) 0, PV_FORMS,	PV_FORMS,	~PV_FORMS },
	{ $P{Priv othptrs},	(char *) 0, PV_OTHERP,	PV_OTHERP,	~PV_OTHERP },
	{ $P{Priv cpri},	(char *) 0, PV_CPRIO,	PV_CPRIO,	~(PV_CPRIO|PV_ANYPRIO) },
	{ $P{Priv otherj},	(char *) 0, PV_OTHERJ,	PV_OTHERJ|PV_VOTHERJ,	~PV_OTHERJ },
	{ $P{Priv prinq},	(char *) 0, PV_PRINQ,	PV_PRINQ,	~(PV_PRINQ|PV_HALTGO|PV_ADDDEL) },
	{ $P{Priv hgo},		(char *) 0, PV_HALTGO,	PV_PRINQ|PV_HALTGO,	~(PV_HALTGO|PV_ADDDEL) },
	{ $P{Priv anyp},	(char *) 0, PV_ANYPRIO,	PV_ANYPRIO|PV_CPRIO,	~PV_ANYPRIO },
	{ $P{Priv cdef},	(char *) 0, PV_CDEFLT,	PV_CDEFLT,	~PV_CDEFLT },
	{ $P{Priv addp},	(char *) 0, PV_ADDDEL,	PV_PRINQ|PV_HALTGO|PV_ADDDEL, ~PV_ADDDEL },
	{ $P{Priv cover},	(char *) 0, PV_COVER,	PV_COVER,	~PV_COVER },
	{ $P{Priv unq},		(char *) 0, PV_UNQUEUE,	PV_UNQUEUE,	~PV_UNQUEUE },
	{ $P{Priv votj},	(char *) 0, PV_VOTHERJ,	PV_VOTHERJ,	~(PV_OTHERJ|PV_VOTHERJ) },
	{ $P{Priv remj},	(char *) 0, PV_REMOTEJ,	PV_REMOTEJ,	~PV_REMOTEJ },
	{ $P{Priv remp},	(char *) 0, PV_REMOTEP,	PV_REMOTEP,	~PV_REMOTEP },
	{ $P{Priv access},	(char *) 0, PV_ACCESSOK,PV_ACCESSOK,	~(PV_ACCESSOK|PV_FREEZEOK) },
	{ $P{Priv freeze},	(char *) 0, PV_FREEZEOK,PV_ACCESSOK|PV_FREEZEOK,	~PV_FREEZEOK }
};

#define	MAXPERM	(sizeof (ptab)/sizeof(struct perm))

int	proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));
int	spitoption(const int, const int, FILE *, const int, const int);

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

/* Expand privilege codes into messages */

static	void	expcodes(void)
{
	int	i;

	allname = gprompt($P{Spulist all name});
	for  (i = 0;  i < MAXPERM;  i++)
		ptab[i].string = gprompt(ptab[i].number);
}

OPTION(o_explain)
{
	print_error($E{spuchange options});
	exit(0);
}

OPTION(o_defaults)
{
	set_default = 1;	/* Don't set admin_privs etc, might change mind */
	return  OPTRESULT_OK;
}

OPTION(o_nodefaults)
{
	set_default = 0;
	return  OPTRESULT_OK;
}

OPTION(o_copyall)
{
	copyall = 1;		/* Ditto regarding admin_privs */
	return  OPTRESULT_OK;
}

OPTION(o_nocopyall)
{
	copyall = 0;
	return  OPTRESULT_OK;
}

OPTION(o_rebuild)
{
	rebuild_file = 1;	/* Ditto regarding admin privs */
	return  OPTRESULT_OK;
}

OPTION(o_norebuild)
{
	rebuild_file = 0;
	return  OPTRESULT_OK;
}

OPTION(o_minpri)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (!isdigit(*arg))  {
		disp_str = arg;
		print_error($E{spuchange priority error});
		exit(E_USAGE);
	}
	num = atoi(arg);
	if  (num <= 0  || num > 255)  {
		disp_str = arg;
		print_error($E{spuchange priority range});
		exit(E_USAGE);
	}
	min_p = (unsigned char) num;
	admin_privs++;
	return  OPTRESULT_ARG_OK;
}

OPTION(o_maxpri)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (!isdigit(*arg))  {
		disp_str = arg;
		print_error($E{spuchange priority error});
		exit(E_USAGE);
	}
	num = atoi(arg);
	if  (num <= 0  || num > 255)  {
		disp_str = arg;
		print_error($E{spuchange priority range});
		exit(E_USAGE);
	}
	max_p = (unsigned char) num;
	admin_privs++;
	return  OPTRESULT_ARG_OK;
}

OPTION(o_defpri)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (!isdigit(*arg))  {
		disp_str = arg;
		print_error($E{spuchange priority error});
		exit(E_USAGE);
	}
	num = atoi(arg);
	if  (num <= 0  || num > 255)  {
		disp_str = arg;
		print_error($E{spuchange priority range});
		exit(E_USAGE);
	}
	def_p = (unsigned char) num;
	cprio_privs++;
	return  OPTRESULT_ARG_OK;
}

OPTION(o_copies)
{
	int	num;

	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (!isdigit(*arg))  {
		disp_str = arg;
		print_error($E{spuchange copies error});
		exit(E_USAGE);
	}
	num = atoi(arg);
	if  (num <= 0  || num > 255)  {
		disp_str = arg;
		print_error($E{spuchange copies range});
		exit(E_USAGE);
	}
	max_cps = (unsigned char) num;
	admin_privs++;
	return  OPTRESULT_ARG_OK;
}

#define	formpopt(routname, varname, priv)	OPTION(routname)\
{\
	if  (!arg)\
		return  OPTRESULT_MISSARG;\
	if  (varname)\
		free(varname);\
	if  (strcmp(arg, "-") == 0)\
		varname = (char *) 0;\
	else\
		varname = stracpy(arg);\
	priv++;\
	return  OPTRESULT_ARG_OK;\
}

formpopt(o_form, def_form, cprio_privs)
formpopt(o_formallow, def_formallow, admin_privs)
formpopt(o_ptr, def_ptr, cprio_privs)
formpopt(o_ptrallow, def_ptrallow, admin_privs)

OPTION(o_class)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  (strcmp(arg, "-") == 0)  {
		class_set = 0;
		return  OPTRESULT_ARG_OK;
	}
	classcode = hextoi(arg);
	if  (classcode == 0)  {
		disp_str = arg;
		print_error($E{specifying zero class});
		exit(E_BADCLASS);
	}
	class_set = 1;
	/* No admin_privs++ - user can lower class if he wants to */
	return  OPTRESULT_ARG_OK;
}

OPTION(o_priv)
{

	if  (!arg)
		return  OPTRESULT_MISSARG;

	/* Start from scratch as we don't want to be confusebd */

	set_flags = 0;
	reset_flags = ALLPRIVS;

	/* Accept flags as hex for benefit of conversion from old versions in spuconv */

	if  (arg[0] == '0' && toupper(arg[1]) == 'X')  {
		disp_str = arg;
		arg += 2;
		while  (*arg)
			if  (isdigit(*arg))
				set_flags = (set_flags << 4) + *arg++ - '0';
			else  if  (isupper(*arg))
				set_flags = (set_flags << 4) + *arg++ - 'A' + 10;
			else  if  (islower(*arg))
				set_flags = (set_flags << 4) + *arg++ - 'a' + 10;
			else  {
				print_error($E{Privilege specification error});
				exit(E_USAGE);
			}
		priv_setting = 1;
		abs_priv = 1;
		admin_privs++;
		return  OPTRESULT_ARG_OK;
	}

	while  (*arg)  {
		int  isminus = 0, ac = 0, pc;
		char	abuf[20];
		if  (*arg == '-')  {
			isminus++;
			arg++;
		}
		do  if  (ac < sizeof(abuf)-1)
			abuf[ac++] = *arg++;
		while  (*arg && *arg != ',');
		abuf[ac] = '\0';
		for  (pc = 0;  pc < MAXPERM;  pc++)
			if  (ncstrcmp(abuf, ptab[pc].string) == 0)
				goto  gotit;
		if  (ncstrcmp(abuf, allname) == 0  &&  !isminus)  {
			set_flags = ALLPRIVS;
			goto  nextcomm;
		}
		disp_str = abuf;
		print_error($E{Privilege specification error});
		exit(E_USAGE);
	gotit:
		if  (isminus)
			reset_flags &= ptab[pc].rflg;
		else
			set_flags |= ptab[pc].sflg;
	nextcomm:
		priv_setting = 1;
		abs_priv = 0;
		if  (*arg == ',')
			arg++;
	}
	admin_privs++;
	return  OPTRESULT_ARG_OK;
}

OPTION(o_dumppw)
{
	dump_opt = DO_DUMP_PW;
	return  OPTRESULT_OK;
}

OPTION(o_aswaspw)
{
	dump_opt = DEFAULT_PW;
	return  OPTRESULT_OK;
}

OPTION(o_killpw)
{
	dump_opt = KILL_PW;
	return  OPTRESULT_OK;
}

#include "inline/o_freeze.c"

/* Defaults and proc table for arg interp.  */

static	const	Argdefault	Adefs[] = {
  {  '?', $A{spuchange explain}	},
  {  'D', $A{spuchange set def}	},
  {  'u', $A{spuchange set users}},
  {  'A', $A{spuchange copy def}},
  {  's', $A{spuchange no copy def}},
  {  'l', $A{spuchange minp}	},
  {  'd', $A{spuchange defp}	},
  {  'm', $A{spuchange maxp}	},
  {  'n', $A{spuchange maxc}	},
  {  'f', $A{spuchange defform}	},
  {  'F', $A{spuchange formallow}},
  {  'o', $A{spuchange defptr}	},
  {  'O', $A{spuchange ptrallow}},
  {  'p', $A{spuchange privs}	},
  {  'c', $A{spuchange class}	},
  {  'R', $A{spuchange rebuild}	},
  {  'N', $A{spuchange no rebuild}},
  {  'X', $A{spuchange dump pw}  },
  {  'Y', $A{spuchange aswas pw} },
  {  'Z', $A{spuchange kill pw}  },
  {  0, 0 }
};

optparam  optprocs[] = {
o_explain,
o_defaults,	o_nodefaults,	o_copyall,	o_nocopyall,
o_minpri,	o_defpri,	o_maxpri,	o_copies,
o_form,		o_ptr,		o_formallow,	o_ptrallow,
o_priv,		o_class,	o_rebuild,	o_norebuild,
o_dumppw,	o_aswaspw,	o_killpw,
o_freezecd,	o_freezehd
};

void	spit_options(FILE *dest, const char *name)
{
	int	cancont = 0;
	fprintf(dest, "%s", name);

	cancont = spitoption(set_default?
			     $A{spuchange set def}:
			     $A{spuchange set users},
			     $A{spuchange explain}, dest, '=', cancont);
	cancont = spitoption(copyall? $A{spuchange copy def}:
			     $A{spuchange no copy def},
			     $A{spuchange explain}, dest, ' ', cancont);
	cancont = spitoption(rebuild_file? $A{spuchange rebuild}:
			     $A{spuchange no rebuild},
			     $A{spuchange explain}, dest, ' ', cancont);
	cancont = spitoption(dump_opt == KILL_PW? $A{spuchange kill pw} :
			     dump_opt == DO_DUMP_PW? $A{spuchange dump pw} :
			     $A{spuchange aswas pw},
			     $A{spuchange explain}, dest, ' ', cancont);
	if  (min_p != 0)  {
		spitoption($A{spuchange minp}, $A{spuchange explain}, dest, ' ', 0);
		fprintf(dest, " %u", min_p);
	}
	if  (def_p != 0)  {
		spitoption($A{spuchange defp}, $A{spuchange explain}, dest, ' ', 0);
		fprintf(dest, " %u", def_p);
	}
	if  (max_p != 0)  {
		spitoption($A{spuchange maxp}, $A{spuchange explain}, dest, ' ', 0);
		fprintf(dest, " %u", max_p);
	}
	if  (max_cps != 0)  {
		spitoption($A{spuchange maxc}, $A{spuchange explain}, dest, ' ', 0);
		fprintf(dest, " %u", max_cps);
	}
	if  (def_form)  {
		spitoption($A{spuchange defform}, $A{spuchange explain}, dest, ' ', 0);
		fprintf(dest, " %s", def_form);
	}
	if  (def_formallow)  {
		spitoption($A{spuchange formallow}, $A{spuchange explain}, dest, ' ', 0);
		fprintf(dest, " %s", def_formallow);
	}
	if  (def_ptr)  {
		spitoption($A{spuchange defptr}, $A{spuchange explain}, dest, ' ', 0);
		fprintf(dest, " %s", def_ptr);
	}
	if  (def_ptrallow)  {
		spitoption($A{spuchange ptrallow}, $A{spuchange explain}, dest, ' ', 0);
		fprintf(dest, " %s", def_ptrallow);
	}
	if  (priv_setting)  {
		spitoption($A{spuchange privs}, $A{spuchange explain}, dest, ' ', 0);

		if  (abs_priv)
			fprintf(dest, "0x%lx", (unsigned long) set_flags);
		else  {
			int	ch = ' ', cnt;
			ULONG	cflags = set_flags;

			for  (cnt = 0;  cnt < MAXPERM;  cnt++)
				if  (cflags & ptab[cnt].flg)  {
					fprintf(dest, "%c%s", ch, ptab[cnt].string);
					cflags &= ~ ptab[cnt].sflg;
					ch = ',';
				}
			cflags = reset_flags;
			for  (cnt = 0;  cnt < MAXPERM;  cnt++)
				if  (!(cflags & ptab[cnt].flg))  {
					fprintf(dest, "%c-%s", ch, ptab[cnt].string);
					cflags |= ~ ptab[cnt].rflg;
					ch = ',';
				}
		}
	}
	putc('\n', dest);
}

struct spdet *find_user(const char *uname)
{
	int_ugid_t	uid;
	int	first, last, middle;

	if  ((uid = lookup_uname(uname)) == UNKNOWN_UID)
		return  (struct spdet *) 0;

	first = 0;
	last = Nusers;

	while  (first < last)  {
		middle = (first + last) / 2;
		if  (ulist[middle].spu_user == uid)
			return  &ulist[middle];
		if  ((ULONG) ulist[middle].spu_user < (ULONG) uid)
			first = middle + 1;
		else
			last = middle;
	}
	return  (struct  spdet  *) 0;
}

static  void	u_update(struct spdet *up, const int metoo)
{
	if  (min_p != 0)
		up->spu_minp = min_p;
	if  (max_p != 0)
		up->spu_maxp = max_p;
	if  (def_p != 0)
		up->spu_defp = def_p;
	if  (max_cps != 0)
		up->spu_cps = max_cps;
	if  (def_form)  {
		strncpy(up->spu_form, def_form, MAXFORM);
		up->spu_form[MAXFORM] = '\0';
	}
	if  (def_formallow)  {
		strncpy(up->spu_formallow, def_formallow, ALLOWFORMSIZE);
		up->spu_formallow[ALLOWFORMSIZE] = '\0';
	}
	if  (def_ptr)  {
		strncpy(up->spu_ptr, def_ptr, PTRNAMESIZE);
		up->spu_ptr[PTRNAMESIZE] = '\0';
	}
	if  (def_ptrallow)  {
		strncpy(up->spu_ptrallow, def_ptrallow, JPTRNAMESIZE);
		up->spu_ptrallow[JPTRNAMESIZE] = '\0';
	}
	if  (class_set)
		up->spu_class = classcode;
	if  (priv_setting  &&  (metoo || up->spu_user != Realuid))  {
		if  (abs_priv)
			up->spu_flgs = set_flags;
		else  {
			up->spu_flgs |= set_flags;
			up->spu_flgs &= reset_flags;
		}
	}
}

static  void	copy_all(void)
{
	struct  spdet	*up, *ue = &ulist[Nusers];
	for  (up = ulist;  up < ue;  up++)  {
		up->spu_minp = Spuhdr.sph_minp;
		up->spu_maxp = Spuhdr.sph_maxp;
		up->spu_defp = Spuhdr.sph_defp;
		up->spu_cps = Spuhdr.sph_cps;
		strcpy(up->spu_form, Spuhdr.sph_form);
		strcpy(up->spu_formallow, Spuhdr.sph_formallow);
		strcpy(up->spu_ptr, Spuhdr.sph_ptr);
		strcpy(up->spu_ptrallow, Spuhdr.sph_ptrallow);
		up->spu_class = Spuhdr.sph_class;
		if  (up->spu_user != Realuid)
			up->spu_flgs = Spuhdr.sph_flgs;
	}
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	int	nerrors = 0, fixusers = 0;
	struct	spdet	*mypriv;
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
	expcodes();
	SCRAMBLID_CHECK
	argv = optprocess(argv, Adefs, optprocs, $A{spuchange explain}, $A{spuchange freeze home}, 0);
	SWAP_TO(Daemuid);

	if  (!(mypriv = getspuentry(Realuid)))  {
		print_error($E{Not registered yet});
		exit(E_UNOTSETUP);
	}

#define	FREEZE_EXIT
#include "inline/freezecode.c"

	if  (admin_privs > 0  ||  set_default  ||  copyall  ||  rebuild_file  ||
	     (class_set && (classcode & ~mypriv->spu_class) != 0))  {
		if  (!(mypriv->spu_flgs & PV_ADMIN))  {
			print_error($E{shell no admin file priv});
			exit(E_NOPRIV);
		}
	}
	else  {

		/* Case of permitted user setting his own stuff.

		   In the case of class code a user can reduce his
		   own class if he wants to but not increase it,
		   we checked just above.  */

		if  (cprio_privs <= 0  &&  !class_set)  {
			print_error($E{spuchange nothing to do});
			exit(E_USAGE);
		}
		if  (!(mypriv->spu_flgs & PV_CDEFLT))  {
			print_error($E{spuchange no change default});
			exit(E_NOPRIV);
		}
		if  (def_form)  {
			if  (!(mypriv->spu_flgs & PV_FORMS) && !qmatch(mypriv->spu_formallow, def_form))  {
				disp_str = def_form;
				disp_str2 = mypriv->spu_formallow;
				print_error($E{Form type doesnt match allowed});
				exit(E_BADFORM);
			}
			strncpy(mypriv->spu_form, def_form, MAXFORM);
			mypriv->spu_form[MAXFORM] = '\0';
		}
		if  (def_ptr)  {
			if  (!(mypriv->spu_flgs & PV_OTHERP) && !qmatch(mypriv->spu_ptrallow, def_ptr))  {
				disp_str = def_ptr;
				disp_str2 = mypriv->spu_ptrallow;
				print_error($E{Ptr type doesnt match allowed});
				exit(E_BADPTR);
			}
			strncpy(mypriv->spu_ptr, def_ptr, PTRNAMESIZE);
			mypriv->spu_ptr[PTRNAMESIZE] = '\0';
		}
		if  (def_p != 0)
			mypriv->spu_defp = def_p;
		if  (class_set)
			mypriv->spu_class = classcode;
		putspuentry(mypriv);
		return  0;
	}

	if  (rebuild_file)  {
		char  *name = envprocess(DUMPPWFILE);
		int	wuz = access(name, 0);
		un_rpwfile();
		unlink(name);
		free(name);
		if  (spu_needs_rebuild)  {
			print_error($E{Rebuilding spufile wait});
			rebuild_spufile();
			if  (dump_opt == DO_DUMP_PW  ||  (wuz >= 0 && dump_opt == DEFAULT_PW))
				dump_pwfile();
			produser();
			print_error($E{Finished rebuild spufile});
		}
		else  {
			fixusers++;
			rpwfile();
			if  (dump_opt == DO_DUMP_PW  ||  (wuz >= 0 && dump_opt == DEFAULT_PW))
				dump_pwfile();
		}
	}

	ulist = getspulist(&Nusers);

	if  (def_form && def_formallow && !qmatch(def_formallow, def_form))  {
		disp_str = def_form;
		disp_str2 = def_formallow;
		print_error($E{Form type doesnt match allowed});
		exit(E_BADFORM);
	}
	if  (def_ptr && def_ptrallow && !qmatch(def_ptrallow, def_ptr))  {
		disp_str = def_ptr;
		disp_str2 = def_ptrallow;
		print_error($E{Ptr type doesnt match allowed});
		exit(E_BADPTR);
	}

	if  (set_default)  {
		if  (*argv != (char *) 0)  {
			print_error($E{Unexpected arguments follow defaults});
			exit(E_USAGE);
		}
		if  (min_p != 0)
			Spuhdr.sph_minp = min_p;
		if  (max_p != 0)
			Spuhdr.sph_maxp = max_p;
		if  (def_p != 0)
			Spuhdr.sph_defp = def_p;
		if  (max_cps != 0)
			Spuhdr.sph_cps = max_cps;
		if  (def_form)  {
			strncpy(Spuhdr.sph_form, def_form, MAXFORM);
			Spuhdr.sph_form[MAXFORM] = '\0';
		}
		if  (def_formallow)  {
			strncpy(Spuhdr.sph_formallow, def_formallow, ALLOWFORMSIZE);
			Spuhdr.sph_formallow[ALLOWFORMSIZE] = '\0';
		}
		if  (def_ptr)  {
			strncpy(Spuhdr.sph_ptr, def_ptr, PTRNAMESIZE);
			Spuhdr.sph_ptr[PTRNAMESIZE] = '\0';
		}
		if  (def_ptrallow)  {
			strncpy(Spuhdr.sph_ptrallow, def_ptrallow, JPTRNAMESIZE);
			Spuhdr.sph_ptrallow[JPTRNAMESIZE] = '\0';
		}
		if  (class_set)
			Spuhdr.sph_class = classcode;
		if  (priv_setting)  {
			if  (abs_priv)
				Spuhdr.sph_flgs = set_flags;
			else  {
				Spuhdr.sph_flgs |= set_flags;
				Spuhdr.sph_flgs &= reset_flags;
			}
		}
		if  (copyall)
			copy_all();
		putspulist(ulist, Nusers, 1);
	}
	else  {
		struct	spdet	*up, *ue = &ulist[Nusers];
		unsigned	ncurr = 0;

		if  (fixusers) /* Set if rebuild file set but no rebuild done */
			for  (up = ulist;  up < ue;  up++)
				up->spu_isvalid = SPU_VALID;

		if  (copyall)
			copy_all();

		if  (*argv == (char *) 0)  {
			for  (up = ulist;  up < ue;  up++)
				u_update(up, 0);
			ncurr = 0;
		}
		else  {
			char	**ap;

			for  (ap = argv;  *ap;  ap++)  {
				up = find_user(*ap);
				if  (!up)  {
					disp_str = *ap;
					print_error($E{Unknown user name ignored});
					nerrors++;
				}
				else
					u_update(up, 1);
			}
		}
		putspulist(ulist, Nusers, 0);
	}
	return  nerrors > 0? E_FALSE: E_TRUE;
}
