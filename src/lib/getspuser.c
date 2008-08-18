/* getspuser.c -- get user profile from user file etc

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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "errnums.h"
#include "defaults.h"
#include "spuser.h"
#include "files.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "incl_sig.h"

#define	INITU	70
#define	INCU	10

struct	sphdr	Spuhdr;
static	int	spuf_fid = -1;
int		spu_needs_rebuild;

static	unsigned  char	igsigs[]= { SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGALRM, SIGUSR1, SIGUSR2 };

#ifdef	UNSAFE_SIGNALS
static	RETSIGTYPE	(*oldsigs[sizeof(igsigs)])(int);
#endif

static void	savesigs(const int saving)
{
	int	cnt;
#ifdef	HAVE_SIGACTION
	sigset_t	nset;
	sigemptyset(&nset);
	for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
		sigaddset(&nset, igsigs[cnt]);
	sigprocmask(saving? SIG_BLOCK: SIG_UNBLOCK, &nset, (sigset_t *) 0);
#elif	defined(STRUCT_SIG)
	int	msk = 0;
	for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
		msk |= sigmask(igsigs[cnt]);
	if  (saving)
		sigsetmask(sigsetmask(~0) | msk);
	else
		sigsetmask(sigsetmask(~0) & ~msk);
#elif	defined(HAVE_SIGSET)
	if  (saving)
		for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
			sighold(igsigs[cnt]);
	else
		for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
			sigrelse(igsigs[cnt]);
#else
	if  (saving)
		for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
			oldsigs[cnt] = signal((int) igsigs[cnt], SIG_IGN);
	else
		for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
			signal((int) igsigs[cnt], oldsigs[cnt]);
#endif
}

static void	iu(int fid, char * arg, int_ugid_t uid)
{
	((struct spdet *) arg) ->spu_user = uid;
	insertu(fid, (struct spdet *) arg);
}

/* Create user control file from scratch.
   Return 0 - failure, 1 - ok */

static int	init_file(char * fname)
{
	int		fid;
	int_ugid_t	uu;
	char		*formname;
	struct	spdet	Spec;
	struct	stat	pwbuf;

	if  ((fid = open(fname, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)
		return  0;

	if  ((uu = lookup_uname(SPUNAME)) != UNKNOWN_UID)
#if	defined(HAVE_FCHOWN) && !defined(M88000)
		fchown(fid, (uid_t) uu, getegid());
#else
		chown(fname, (uid_t) uu, getegid());
#endif
	savesigs(1);
	if  ((formname = helpprmpt($P{Default user form type})) == (char *) 0)
		formname = "standard";

	stat("/etc/passwd", &pwbuf);
	Spuhdr.sph_lastp = pwbuf.st_mtime;
	Spuhdr.sph_minp = U_DF_MINP;
	Spuhdr.sph_maxp = U_DF_MAXP;
	Spuhdr.sph_defp = U_DF_DEFP;
	Spuhdr.sph_flgs = U_DF_PRIV;
	Spuhdr.sph_class = (classcode_t) U_DF_CLASS;
	Spuhdr.sph_cps = U_DF_CPS;
	Spuhdr.sph_version = GNU_SPOOL_MAJOR_VERSION;
	strncpy(Spuhdr.sph_form, formname, MAXFORM);
	strncpy(Spuhdr.sph_formallow, formname, ALLOWFORMSIZE);
	Spuhdr.sph_ptr[0] = '\0';
	Spuhdr.sph_ptrallow[0] = '\0';
	write(fid, (char *) &Spuhdr, sizeof(Spuhdr));

	Spec.spu_isvalid = SPU_VALID;
	strncpy(Spec.spu_form, formname, MAXFORM);
	strncpy(Spec.spu_formallow, formname, ALLOWFORMSIZE);
	Spec.spu_ptr[0] = '\0';
	Spec.spu_ptrallow[0] = '\0';
	Spec.spu_minp = U_DF_MINP;
	Spec.spu_maxp = U_DF_MAXP;
	Spec.spu_defp = U_DF_DEFP;
	Spec.spu_cps = U_DF_CPS;
	Spec.spu_flgs = U_DF_PRIV;
	Spec.spu_class = (classcode_t) U_DF_CLASS;

	uloop_over(fid, iu, (char *) &Spec);

	/* Set "root" and "spooler" to be super-people.  */

	Spec.spu_flgs = ALLPRIVS;
	Spec.spu_user = ROOTID;
	Spec.spu_isvalid = SPU_VALID;
	insertu(fid, &Spec);

	if  ((uu = lookup_uname(SPUNAME)) != UNKNOWN_UID)  {
		gid_t	lastgid = getegid();
		Spec.spu_user = uu;
		insertu(fid, &Spec);
#if	defined(HAVE_FCHOWN) && !defined(M88000)
		fchown(fid, uu, lastgid);
#else
		chown(fname, uu, lastgid);
#endif
	}
	close(fid);
	savesigs(0);
	return  1;
}

/* Lock the whole caboodle */

static  void  lockit(const int fid, const int type)
{
	struct	flock	lk;

	lk.l_type = (SHORT) type;
	lk.l_whence = 0;
	lk.l_start = 0;
	lk.l_len = 0;
	lk.l_pid = 0;
	if  (fcntl(fid, F_SETLKW, &lk) < 0)  {
		print_error($E{Cannot lock user ctrl file});
		exit(E_SETUP);
	}
}

/* Unlock the whole caboodle */

static  void  unlockit(const int fid)
{
	struct	flock	lk;

	lk.l_type = F_UNLCK;
	lk.l_whence = 0;
	lk.l_start = 0;
	lk.l_len = 0;
	lk.l_pid = 0;
	if  (fcntl(fid, F_SETLKW, &lk) < 0)  {
		print_error($E{Cannot unlock user ctrl file});
		exit(E_SETUP);
	}
}

/* Didn't find user, or he wasn't valid, so make new thing.  */

static  void  init_defaults(struct spdet *res, const int_ugid_t uid, const unsigned varg)
{
	res->spu_isvalid = varg;
	strncpy(res->spu_form, Spuhdr.sph_form, MAXFORM);
	strncpy(res->spu_formallow, Spuhdr.sph_formallow, ALLOWFORMSIZE);
	strncpy(res->spu_ptr, Spuhdr.sph_ptr, PTRNAMESIZE);
	strncpy(res->spu_ptrallow, Spuhdr.sph_ptrallow, JPTRNAMESIZE);
	res->spu_user = uid;
	res->spu_minp = Spuhdr.sph_minp;
	res->spu_maxp = Spuhdr.sph_maxp;
	res->spu_defp = Spuhdr.sph_defp;
	res->spu_cps = Spuhdr.sph_cps;
	res->spu_flgs = Spuhdr.sph_flgs;
	res->spu_class = Spuhdr.sph_class;
}

/* Routine called by uloop_over to check for new users */

static  void  chk_nuser(const int fid, char *arg, const int_ugid_t uid)
{
	struct	spdet	uu;

	if  (!readu(fid, uid, &uu))  {
		init_defaults(&uu, uid, (unsigned) *arg);
		insertu(fid, &uu);
	}
}

void	rebuild_spufile(void)
{
	int		needsquash;
	char		ulim;
	LONG		posn;
	char		*fname = envprocess(SPUFILE);
	struct	spdet	bu;
	struct	stat	pwbuf;

	/* First lock the file (it must be open to get here).  This
	   might take a long time because some other guy got in
	   first and locked the file.  If he did then we probably
	   don't need to do the regen.  */

	savesigs(1);
	lockit(spuf_fid, F_WRLCK);
	lseek(spuf_fid, 0L, 0);
	read(spuf_fid, (char *)&Spuhdr, sizeof(Spuhdr));
	stat("/etc/passwd", &pwbuf);
	if  (Spuhdr.sph_lastp >= pwbuf.st_mtime)
		goto  forgetit;

	/* Go through the users in the password file and see if we
	   know about them.  */

	ulim = SPU_VALID;	/* Kludge for initialising */
	uloop_over(spuf_fid, chk_nuser, &ulim);

	/* We now remove users who no longer belong.  'needsquash'
	   records users over the direct seek limit who have
	   disappeared. In this case we mark them as invalid and
	   rewrite the file in the next loop eliminating them.  */

	needsquash = 0;
	posn = sizeof(Spuhdr);
	lseek(spuf_fid, (long) posn, 0);

	for  (; read(spuf_fid, (char *) &bu, sizeof(struct spdet)) == sizeof(struct spdet);  posn += sizeof(struct spdet))  {

		if  (!bu.spu_isvalid)  {
			/* The expression in the next statement could
			   be replaced by bu.spu_user, but there
			   is always the possibility that the
			   file has been mangled which this will
			   tend to eliminate.  */
			if  ((posn - sizeof(Spuhdr)) / sizeof(struct spdet) >= SMAXUID)
				needsquash++;
			continue;
		}

		if  (isvuser((uid_t) bu.spu_user))  {  /* Still in pw file */
			if  (bu.spu_isvalid == SPU_VALID)
				continue;
			bu.spu_isvalid = SPU_VALID;
		}
		else  {
			/* Mark no longer valid.  If it's beyond the
			   magic limit we will need to squash the file.  */
			bu.spu_isvalid = SPU_INVALID;
			if  ((ULONG) bu.spu_user >= SMAXUID)
				needsquash++;
		}

		lseek(spuf_fid, -(long) sizeof(struct spdet), 1);
		write(spuf_fid, &bu, sizeof(struct spdet));
	}

	/* Copy password file mod time into header.  We do this rather
	   than the current time (a) to reduce the chances of
	   missing a further change (b) to cope with `time moving
	   backwards'.  */

	Spuhdr.sph_lastp = pwbuf.st_mtime;
	lseek(spuf_fid, 0L, 0);
	write(spuf_fid, (char *) &Spuhdr, sizeof(Spuhdr));

	/* Ok now for squashes. We copy out to a temporary file and
	   copy back in case anyone else is looking at the file
	   it saves messing around with locks.  */

	if  (needsquash)  {
		char	*tfname = envprocess(SPUTMP);
		int	outfd, uc;

		if  ((outfd = open(tfname, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)  {
			disp_str = tfname;
			print_error($E{Cannot create temp user file});
			free(tfname);
			goto  forgetit;
		}

		/* Copy over the direct seek bits.  First the
		   header. Note that we should be at just the
		   right place in the original file.  */

		write(outfd, (char *) &Spuhdr, sizeof(Spuhdr));
		for  (uc = 0;  uc < SMAXUID;  uc++)  {
			read(spuf_fid, (char *) &bu, sizeof(bu));
			write(outfd, (char *) &bu, sizeof(bu));
		}

		/* And now for the rest */

		while  (read(spuf_fid, (char *) &bu, sizeof(bu)) == sizeof(bu))
			if  (bu.spu_isvalid)
				write(outfd, (char *) &bu, sizeof(bu));

		/* Now truncate the original file.  If we have some
		   sort of `truncate' system call use it here
		   instead of this mangling.  */

		lseek(spuf_fid, 0L, 0);
		lseek(outfd, (long) sizeof(Spuhdr), 0);
#ifdef	HAVE_FTRUNCATE
		ftruncate(spuf_fid, 0L);
#else
		close(open(fname, O_RDWR|O_TRUNC));
#endif
		write(spuf_fid, (char *) &Spuhdr, sizeof(Spuhdr));
		while  (read(outfd, (char *) &bu, sizeof(bu)) == sizeof(bu))
			write(spuf_fid, (char *) &bu, sizeof(bu));

		close(outfd);
		unlink(tfname);
		free(tfname);
	}

 forgetit:
	savesigs(0);
	free(fname);
	unlockit(spuf_fid);
	spu_needs_rebuild = 0;
}

/* See if we need to regenerate user file because of new users added recently.
   Return file descriptor */

static void	open_file(int mode)
{
	char	*fname = envprocess(SPUFILE);
	struct	stat	pwbuf;

	if  ((spuf_fid = open(fname, mode)) < 0)  {
		if  (errno == EACCES)  {
			print_error($E{Check file setup});
			exit(E_SETUP);
		}
		if  (errno == ENOENT)
			init_file(fname);
		spuf_fid = open(fname, mode);
	}

	if  (spuf_fid < 0)  {
		print_error($E{Cannot open user file});
		exit(E_SETUP);
	}

	fcntl(spuf_fid, F_SETFD, 1);
	lockit(spuf_fid, F_RDLCK);
	read(spuf_fid, (char *)&Spuhdr, sizeof(Spuhdr));

	/* Check version number and print warning message if funny.  */

	if  (Spuhdr.sph_version != GNU_SPOOL_MAJOR_VERSION)  {
		disp_arg[0] = Spuhdr.sph_version;
		disp_arg[1] = GNU_SPOOL_MAJOR_VERSION;
		print_error($E{Wrong version of product});
	}

	if  (stat("/etc/passwd", &pwbuf) < 0)  {
		free(fname);
		return;
	}

	unlockit(spuf_fid);
	free(fname);

	if  (Spuhdr.sph_lastp >= pwbuf.st_mtime)  {
		if  (Spuhdr.sph_lastp > pwbuf.st_mtime)
			print_error($E{Funny times passwd file});
		spu_needs_rebuild = 0;
		return;
	}
	spu_needs_rebuild = 1;
}

/* Get info about specific user.
   If we haven't met the guy before copy over default stuff.  */

static struct spdet *gpriv(uid_t uid)
{
	static	struct	spdet	result;
	int	ret;

	lockit(spuf_fid, F_RDLCK);
	ret = readu(spuf_fid, uid, &result);
	unlockit(spuf_fid);
	return  ret? &result: (struct spdet *) 0;
}

/* Get list of all users known.  */

static struct spdet *gallpriv(unsigned *Np)
{
	struct  spdet  *result, *rp;
	unsigned  maxu = INITU, count = 0;

	if  ((result = (struct spdet *) malloc(INITU*sizeof(struct spdet))) == (struct spdet *) 0)
		nomem();

	/* NB assume that the last thing we did was read the header. (or seek to it!).  */

	rp = result;
	while  (read(spuf_fid, (char *) rp, sizeof(struct spdet)) == sizeof(struct spdet))  {
		if  (rp->spu_isvalid)  {
			rp++;
			count++;
			if  (count >= maxu)  {
				maxu += INCU;
				if  ((result = (struct spdet *) realloc((char *) result, maxu * sizeof(struct spdet))) == (struct spdet *) 0)
					nomem();
				rp = &result[count];
			}
		}
	}

	*Np = count;
	return  result;
}

/* Routine to access privilege/mode file.
   This is now the basic routine for user programs and does not return
   if there's a problem.  */

struct  spdet *getspuser(const uid_t uid)
{
	struct  spdet  *result;

	open_file(O_RDONLY);
	result = gpriv(uid);
	close(spuf_fid);
	spuf_fid = -1;

	if  (!result)  {
		print_error($E{Not registered yet});
		exit(E_UNOTSETUP);
	}
	return  result;
}

/* Get entry in user file, possibly for update Only done for utility routines.  */

struct  spdet *getspuentry(const uid_t uid)
{
	if  (spuf_fid < 0)
		open_file(O_RDWR);
	return  gpriv(uid);
}

/* Update details for given user only.  */

void	putspuentry(struct spdet *item)
{
	lockit(spuf_fid, F_WRLCK);
	insertu(spuf_fid, item);
	unlockit(spuf_fid);
}

struct  spdet *getspulist(unsigned *Nu)
{
	struct	spdet	*result;
	if  (spuf_fid < 0)
		open_file(O_RDWR);
	else
		lseek(spuf_fid, (long) sizeof(struct sphdr), 0);
	lockit(spuf_fid, F_RDLCK);
	result = gallpriv(Nu);
	unlockit(spuf_fid);
	return  result;
}

/* Save list.  */

void	putspulist(struct spdet *list, unsigned num, int hchanges)
{
	lockit(spuf_fid, F_WRLCK);
	if  (hchanges)  {
		lseek(spuf_fid, 0L, 0);
		write(spuf_fid, (char *) &Spuhdr, sizeof(Spuhdr));
	}
	else
		lseek(spuf_fid, (long) sizeof(Spuhdr), 0);

	if  (list)  {
		struct  spdet  *up;
		for  (up = list;  up < &list[num];  up++)
			insertu(spuf_fid, up);
	}
	unlockit(spuf_fid);
}
