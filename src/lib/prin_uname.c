/* prin_uname.c -- for hashing of user names/idsxd

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
#include <pwd.h>
#include "defaults.h"
#include "files.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "errnums.h"

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

/* Structure used to hash user ids.  */

struct	uhash	{
	struct	uhash	*pwh_next, *pwu_next;
	int_ugid_t	pwh_uid;
	char		*pwh_homed;	/* Used for unameproc */
	char		pwh_name[1];
};

struct	fuhash  {
	LONG		pwh_next;
	int_ugid_t	pwh_uid;
	LONG		pwh_homed;
};

#define	HASHMOD	97

static	int	doneit;
static	struct	uhash	*uhash[HASHMOD];
static	struct	uhash	*unhash[HASHMOD];

void	dump_pwfile(void)
{
	LONG	offset;
	int	fd, hn, hdl;
	unsigned	cnt;
	struct	uhash	*hp;
	char	*name = envprocess(DUMPPWFILE);
	struct	fuhash	fh;
	LONG	dhash[HASHMOD];

	if  ((fd = open(name, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
		goto  zapit;

	offset = HASHMOD * sizeof(LONG);
	lseek(fd, offset, 0);

	for  (cnt = 0;  cnt < HASHMOD;  cnt++)  {
		dhash[cnt] = uhash[cnt]? offset: 0L;
		for  (hp = uhash[cnt];  hp;  hp = hp->pwh_next)  {
			fh.pwh_uid = hp->pwh_uid;
			hn = strlen(hp->pwh_name) + 1;
			hdl = strlen(hp->pwh_homed) + 1;
			offset += sizeof(struct fuhash) + hn;
			fh.pwh_homed = offset;
			offset += hdl;
			fh.pwh_next = hp->pwh_next? offset: 0L;
			if  (write(fd, (char *) &fh, sizeof(fh)) != sizeof(fh))
				goto  zapitc;
			if  (write(fd, hp->pwh_name, hn) != hn)
				goto  zapitc;
			if  (write(fd, hp->pwh_homed, hdl) != hdl)
				goto  zapitc;
		}
	}

	/* Dump out hash table of uids.
	   We'd like to do it for names but we want to be sure of
	   preserving the order of uid->name lookups
	   (See comment in pwfile below); */

	lseek(fd, 0L, 0);
	if  (write(fd, (char *) dhash, sizeof(dhash)) != sizeof(dhash))
		goto  zapitc;

	close(fd);
	free(name);
	return;
zapitc:
	close(fd);
zapit:
	unlink(name);
	free(name);
}

static int	undump_pwfile(void)
{
	char	*name = envprocess(DUMPPWFILE);
	struct	uhash	*hp, **hpp;
	unsigned	cnt;
	int	fd, nl;
	LONG	fp;
	struct	fuhash	fh;
	LONG	dhash[HASHMOD];
	char	dirbuf[PATH_MAX];

	fd = open(name, O_RDONLY);
	free(name);
	if  (fd < 0)
		return  0;
	if  (read(fd, (char *) dhash, sizeof(dhash)) != sizeof(dhash))  {
	yuk:
		close(fd);
		return  0;
	}

	/* Read in and reallocate the user hash table.  */

	for  (cnt = 0;  cnt < HASHMOD;  cnt++)  {
		hpp = &uhash[cnt];
		fp = dhash[cnt];
		while  (fp)  {
			lseek(fd, fp, 0);
			if  (read(fd, (char *) &fh, sizeof(fh)) != sizeof(fh))
				goto  yuk;

			/* Take advantage of the fact that we write
			   the home dirs after the hash structure
			   to get user name length (including
			   null char).  */

			nl = fh.pwh_homed - fp - sizeof(fh);
			fp = fh.pwh_next;
			if  (!(hp = (struct uhash *) malloc((unsigned) (nl + sizeof(struct uhash) - 1))))
				nomem();
			*hpp = hp;
			hpp = &hp->pwh_next;
			hp->pwh_uid = fh.pwh_uid;
			if  (read(fd, hp->pwh_name, nl) != nl)
				goto  yuk;
			if  (read(fd, dirbuf, sizeof(dirbuf)) <= 0)
				goto  yuk;
			hp->pwh_homed = stracpy(dirbuf);
		}
		*hpp = (struct uhash *) 0;
	}
	close(fd);
	BLOCK_ZERO(unhash, sizeof(unhash));

	for  (cnt = 0;  cnt < HASHMOD;  cnt++)
		for  (hp = uhash[cnt];  hp;  hp = hp->pwh_next)  {
			char	*pn = hp->pwh_name;
			unsigned  sum = 0;
			while  (*pn)
				sum += *pn++;
			sum %= HASHMOD;
			hp->pwu_next = unhash[sum];
			unhash[sum] = hp;
		}

	return  1;
}

/* "unread" password file to order */

void	un_rpwfile(void)
{
	unsigned   cnt;
	struct	uhash	*hp, *np;

	if  (!doneit)		/* Nothing to undo */
		return;
	for  (cnt = 0;  cnt < HASHMOD;  cnt++)  {
		for  (hp = uhash[cnt];  hp;  hp = np)  {
			np = hp->pwh_next;
			if  (hp->pwh_homed)
				free(hp->pwh_homed);
			free((char *) hp);
		}
		uhash[cnt] = unhash[cnt] = (struct uhash *) 0;
	}
	doneit = 0;
}

/* Read password file to build up hash table of user ids.  This is
   done once only at the start of the program.  */

void	rpwfile(void)
{
	if  (!undump_pwfile())  {
		struct  passwd  *upw;
		struct  uhash  *hp, **hpp, **hnpp;
		char	*pn;
		unsigned  sum;

		while  ((upw = getpwent()) != (struct passwd *) 0)  {
			pn = upw->pw_name;
			sum = 0;
			while  (*pn)
				sum += *pn++;

			/* Find END of collision chain for uid lookup.
			   This is so system ids with the same
			   uid come out with first name in the
			   password file.  */

			for  (hpp = &uhash[(ULONG)upw->pw_uid % HASHMOD]; (hp = *hpp); hpp = &hp->pwh_next)
				;

			hnpp = &unhash[sum % HASHMOD];
			if  ((hp = (struct uhash *) malloc(sizeof(struct uhash) + strlen(upw->pw_name))) == (struct uhash *) 0)
				nomem();
			hp->pwh_uid = upw->pw_uid;
			hp->pwh_homed = stracpy(upw->pw_dir);
			strcpy(hp->pwh_name, upw->pw_name);
			hp->pwh_next = *hpp;
			hp->pwu_next = *hnpp;
			*hpp = hp;
			*hnpp = hp;
		}
		endpwent();
	}
	doneit = 1;
}

/* Look up uid in hash table of user ids */

static struct uhash *luid_lookup(const uid_t uid)
{
	struct  uhash  *hp;

	if  (!doneit)
		rpwfile();

	hp = uhash[(unsigned) uid % HASHMOD];

	while  (hp)  {
		if  (uid == hp->pwh_uid)
			return	hp;
		hp = hp->pwh_next;
	}
	return	(struct uhash *) 0;
}

/* Look up user name in hash table of user names */

static const struct uhash *luname_lookup(const char * name)
{
	const char	*cp;
	unsigned  sum = 0;
	const struct  uhash  *hp;

	if  (!doneit)
		rpwfile();
	cp = name;
	while  (*cp)
		sum += *cp++;
	hp = unhash[sum % HASHMOD];
	while  (hp)  {
		if  (strcmp(name, hp->pwh_name) == 0)
			return  hp;
		hp = hp->pwu_next;
	}
	return  (struct uhash *) 0;
}

/* Given a user id, return a user name.  */

char *	prin_uname(const uid_t uid)
{
	struct  uhash  *hp = luid_lookup(uid);
	if  (hp)
		return  hp->pwh_name;
	else  {
		static	char	nbuf[10];
		sprintf(nbuf, "U%ld", (long) uid);
		return  nbuf;
	}
}

/* Validate user id */

int	isvuser(const uid_t uid)
{
	struct  uhash  *hp = luid_lookup(uid);
	return  hp != (struct uhash *) 0;
}

/* Get uid from name - (long in case uid_t isn't signed thankyou Amdahl) */

int_ugid_t	lookup_uname(const char * name)
{
	const	struct  uhash  *hp = luname_lookup(name);
	return  hp? hp->pwh_uid: UNKNOWN_UID;
}

/* Loop over all known user ids, calling the supplied function with
   argument arg and uid.  */

void	uloop_over(const int fid, void (*fn)(int, char *, int_ugid_t), char *arg)
{
	struct  uhash  *hp;
	unsigned	hi;

	if  (!doneit)
		rpwfile();
	for  (hi = 0;  hi < HASHMOD;  hi++)
		for  (hp = uhash[hi];  hp;  hp = hp->pwh_next)
			(*fn)(fid, arg, hp->pwh_uid);
}

/* Generate a matrix for use when prompting for user names */

#define	ULINIT	40
#define	ULINCR	10

char  **gen_ulist(const char *prefix, const int notused)
{
	struct  uhash  *hp;
	unsigned	hi;
	char	**result;
	unsigned  maxr, countr;
	int	sfl = 0;

	if  (!doneit)
		rpwfile();

	if  ((result = (char **) malloc(ULINIT * sizeof(char *))) == (char **) 0)
		nomem();

	maxr = ULINIT;
	countr = 0;
	if  (prefix)
		sfl = strlen(prefix);

	for  (hi = 0;  hi < HASHMOD;  hi++)
		for  (hp = uhash[hi];  hp;  hp = hp->pwh_next)  {

			/* Skip ones which don't match the prefix */

			if  (strncmp(hp->pwh_name, prefix, sfl) != 0)
				continue;

			if  (countr + 1 >= maxr)  {
				maxr += ULINCR;
				if  ((result = (char**) realloc((char *) result, maxr * sizeof(char *))) == (char **) 0)
					nomem();
			}

			result[countr++] = stracpy(hp->pwh_name);
		}

	if  (countr == 0)  {
		free((char *) result);
		return  (char **) 0;
	}

	result[countr] = (char *) 0;
	return  result;
}

/* Process string to extract ~username constructs or ~ for $HOME
   ~+ for $PWD if it exists ~- for $OLDPWD if it exists */

char  *unameproc(char *str, const char *currdir, const uid_t realuid)
{
	char	*ep;
	char	*cp, *newstr;
	const	char	*ins;
	int	l1, l2, l3, alloc = 0;

	/* It's probably unlikely that there will be more than one ~
	   in the string, but let's not be taken by surprise if
	   this happens.  */

	while  ((cp = strchr(str, '~')) != (char *) 0)  {
		ep = cp;
		if  (*++ep == '+')  {
			ep++;
			ins = currdir;
		}
		else  if  (*ep == '-')  {
			if  ((ins = getenv("OLDPWD")) == (char *) 0)  {
				disp_str = "~- (OLDPWD)";
				if  (alloc)
					free(str);
				return  (char *) 0;
			}
			ep++;
		}
		else  if  (!isalpha(*ep))  {
			if  ((ins = getenv("HOME")) == (char *) 0)  {
				struct	uhash	*hp = luid_lookup(realuid);
				if  (!hp)  {
					disp_str = "HOME";
					if  (alloc)
						free(str);
					return  (char *) 0;
				}
				ins = hp->pwh_homed;
			}
		}
		else  {
			int	ls = 0;
			static	char	cv[12];
			const	struct	uhash	*hp;

			do  if  (ls < 11)
				cv[ls++] = *ep++;
			while  (isalnum(*ep));
			cv[ls] = '\0';
			if  (!(hp = luname_lookup(cv)))  {
				disp_str = cv;
				if  (alloc)
					free(str);
				return  (char *) 0;
			}
			ins = hp->pwh_homed;
		}

		l1 = cp - str;
		l2 = strlen(ins);
		l3 = strlen(ep);

		if  ((newstr = (char *) malloc((unsigned) (l1 + l2 + l3 + 1))) == (char *) 0)
			nomem();

		strncpy(newstr, str, l1);
		strcpy(newstr + l1, ins);
		strcpy(newstr + l1 + l2, ep);
		if  (alloc)
			free(str);
		str = newstr;
		alloc++;
	}

	/* Insist on a "malloced" version */

	return  alloc? str: stracpy(str);
}
