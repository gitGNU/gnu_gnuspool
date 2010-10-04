/* xsq_prompt.c -- generate lists of files and such for xspq

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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_dir.h"
#include <sys/stat.h>
#include <gtk/gtk.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "q_shm.h"
#include "spuser.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "errnums.h"
#include "xsq_ext.h"
#include "stringvec.h"

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

#define	MALLINIT	5
#define	MALLINC		3

#ifndef	HAVE_LONG_FILE_NAMES
#include "inline/owndirops.c"
#endif

static char *pathjoin(const char *d1, const char *d2, const char *f)
{
	char	*d;
	static	char	slash[] = "/";

	if  (d2)  {
		if  ((d = (char *) malloc((unsigned) (strlen(d1) + strlen(d2) + strlen(f) + 3))) == (char *) 0)
			nomem();
		strcpy(d, d1);
		strcat(d, slash);
		strcat(d, d2);
		strcat(d, slash);
		strcat(d, f);
	}
	else  {
		if  ((d = (char *) malloc((unsigned) (strlen(d1) + strlen(f) + 2))) == (char *) 0)
			nomem();
		strcpy(d, d1);
		strcat(d, slash);
		strcat(d, f);
	}
	return  d;
}

static int  isform(const char *d1, const char *d2, const char *f)
{
	char	*d;
	int	sret;
	struct	stat	sbuf;

	if  (strchr(DEF_SUFCHARS, f[0]))
		return  0;

	d = pathjoin(d1, d2, f);
	sret = stat(d, &sbuf);
	free(d);
	return  sret >= 0  &&  (sbuf.st_mode & S_IFMT) == S_IFREG  &&  sbuf.st_uid == Daemuid;
}

static int  isprin(const char *d1, const char *d2, const char *f)
{
	char	*d;
	struct	stat	sbuf;

	d = pathjoin(d1, d2, f);
	if  (stat(d, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) != S_IFDIR || sbuf.st_uid != Daemuid)  {
		free(d);
		return  0;
	}
	free(d);
	return  1;
}

static int  isaterm(const char *d1, const char *d2, const char *f)
{
	char	*d = pathjoin(d1, d2, f);
	struct	stat	sbuf;

	if  (strncmp(f, "lp", 2) != 0  &&  (strncmp(f, "tty", 3) != 0 || f[3] == '\0'))
		return  0;

	if  (stat(d, &sbuf) < 0)  {
		free(d);
		return  0;
	}

	free(d);

	if  ((sbuf.st_mode & S_IFMT) != S_IFCHR)
		return  0;

	if  (sbuf.st_uid == Daemuid)  {
		if  ((sbuf.st_mode & 0600) != 0600)
			return  0;
	}
	else  if  ((sbuf.st_mode & 0006) != 0006)
		return  0;
	return  1;
}

static void listdir(struct stringvec *sv, const char *d1, const char *d2, int (*chkfunc)(const char *,const char *,const char *))
{
	DIR	*dfd;
	struct	dirent	*dp;
	char	*d;

	if  (d2)  {
		d = pathjoin(d1, (char *) 0, d2);
		dfd = opendir(d);
		free(d);
	}
	else
		dfd = opendir(d1);

	if  (!dfd)
		return;

	while  ((dp = readdir(dfd)) != (struct dirent *) 0)  {
		if  (dp->d_name[0] == '.'  &&
		     (dp->d_name[1] == '\0' ||
		      (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
			continue;
		if  (!chkfunc  ||  (*chkfunc)(d1, d2, dp->d_name))
			stringvec_insert_unique(sv, dp->d_name);
	}
	closedir(dfd);
}

void  p_prins(struct stringvec *sv)
{
	const  Hashspptr  **pp, **ep;

	ep = &Ptr_seg.pp_ptrs[Ptr_seg.nptrs];
	for  (pp = &Ptr_seg.pp_ptrs[0];  pp < ep;  pp++)
		stringvec_insert_unique(sv, (*pp)->p.spp_ptr);
}

void  p_forms(struct stringvec *sv)
{
	const  Hashspptr  **pp, **ep;

	ep = &Ptr_seg.pp_ptrs[Ptr_seg.nptrs];
	for  (pp = &Ptr_seg.pp_ptrs[0];  pp < ep;  pp++)
		stringvec_insert_unique(sv, (*pp)->p.spp_form);
}

void  j_prins(struct stringvec *sv)
{
	unsigned  cnt;
	for  (cnt = 0;  cnt < Job_seg.njobs;  cnt++)  {
		const struct spq *jp = &Job_seg.jj_ptrs[cnt]->j;
		if  (jp->spq_ptr[0] != '\0')
			stringvec_insert_unique(sv, jp->spq_ptr);
	}
}

void  j_forms(struct stringvec *sv)
{
	unsigned  cnt;
	for  (cnt = 0;  cnt < Job_seg.njobs;  cnt++)
		stringvec_insert_unique(sv, Job_seg.jj_ptrs[cnt]->j.spq_form);
}

static void  listpfdirs(struct stringvec *sv, const struct spptr *current_prin)
{
	struct  stringvec  prins;
	stringvec_init(&prins);
	p_prins(&prins);
	if  (stringvec_count(prins) != 0)  {
		int	cnt;
		for  (cnt = 0;  cnt < stringvec_count(prins);  cnt++)
			listdir(sv, ptdir, stringvec_nth(prins, cnt), isform);
	}
	stringvec_free(&prins);
	if  (current_prin)
		listdir(sv, ptdir, current_prin->spp_ptr, isform);
}

FILE *hexists(const char *dir, const char *d2)
{
	char	*fname;
	static	char	*hname;
	FILE	*result;

	if  (!hname)
		hname = envprocess(HELPNAME);
	fname = pathjoin(dir, d2, hname);
	result = fopen(fname, "r");
	free(fname);
	return  result;
}

#define	MAXLINESIZE	80

void  makefvec(struct stringvec *sv, FILE *f)
{
	int	ch;
	unsigned  l;
	char	lbuf[MAXLINESIZE+1];

	while  ((ch = getc(f)) != EOF)  {
		l = 0;
		while  (ch != '\n'  &&  ch != EOF)  {
			if  (ch == '\t')  {
				do  {
					if  (l <= MAXLINESIZE)
						lbuf[l] = ' ';
					l++;
				}  while  ((l & 7) != 0);
			}
			else  {
				if  (l <= MAXLINESIZE)
					lbuf[l] = ch;
				l++;
			}
			ch = getc(f);
		}
		lbuf[l] = '\0';
		stringvec_append(sv, lbuf);
	}
	fclose(f);
}

void  wotjprin(struct stringvec *sv)
{
	listdir(sv, ptdir, (char *) 0, isprin);
	p_prins(sv);
}

void  wotjform(struct stringvec *sv)
{
	listpfdirs(sv, (const struct spptr *) 0);
	p_forms(sv);
}

void  wotpform(struct stringvec *sv)
{
	listpfdirs(sv, (const struct spptr *) 0);
	j_forms(sv);
}

void  wotpprin(struct stringvec *sv)
{
	listdir(sv, ptdir, (char *) 0, isprin);
	j_prins(sv);
}

void  wottty(struct stringvec *sv)
{
	listdir(sv, "/dev", (char *) 0, isaterm);
}

/* Look at proposed device and comment about it with a code (as a
   subsequent argument to Confirm()), or 0 if nothing wrong.  */

int  validatedev(const char *devname)
{
	const   char	*name;
	struct	stat	sbuf;
	char	fullpath[PATH_MAX];

	if  (devname[0] == '/')
		name = devname;
	else  {
		sprintf(fullpath, "/dev/%s", devname);
		name = fullpath;
	}
	if  (stat(name, &sbuf) < 0)
		return  $PH{Device does not exist};

	disp_arg[8] = sbuf.st_uid;

	if  (sbuf.st_uid != Daemuid)  {
		if  ((sbuf.st_mode & 022) != 022)
			return	$P{Device not writeable};
		return	$PH{Device not owned};
	}
	if  ((sbuf.st_mode & 0600) != 0600)
		return	$PH{Owned but not writable};
	switch  (sbuf.st_mode & S_IFMT)  {
	case  S_IFBLK:
		return  $PH{Device is block device};
	case  S_IFREG:
		return  $PH{Device is flat file};
	case  S_IFIFO:
		return	$PH{Device is FIFO};
	}
	return  0;
}
