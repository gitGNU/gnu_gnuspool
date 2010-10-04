/* cplist.c -- convert printer list to shell script file

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

/* The slightly strange way this is written and the references to "23"
   are the version of Xi-Text from which Gnuspool was derived.
   The original version of this program converted various ancient versions
   of the printer file. */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "incl_unix.h"
#include "ecodes.h"
#include "files.h"

struct  {
	char	*srcdir;	/* Directory we read from if not pwd */
	char	*outfile;	/* Output file */
	long	errtol;		/* Number of errors we'll take */
	long	errors;		/* Number we've had */
	short	ignsize;	/* Ignore file size */
	short	ignfmt;		/* Ignore file format errors */
	short	localptr;	/* Local printers only */
}  popts;

extern char *expand_srcdir(char *);
extern char *make_absolute(char *);
extern char *hex_disp(const classcode_t, const int);

const	char	*progname;

void	nomem()
{
	fprintf(stderr, "Run out of memory\n");
	exit(E_NOMEM);
}

static int  devok(const char *dev)
{
	int	lng = strlen(dev);
	if  (lng <= 0  ||  lng > LINESIZE)
		return  0;
	do  if  (!isprint(*dev))
		return  0;
	while  (*++dev);
	return  1;
}

static int  formok(const char *form, const unsigned flng)
{
	int	lng = strlen(form);
	if  (lng <= 0  ||  lng > flng)
		return  0;
	do  if  (!isalnum(*form) && *form != '.' && *form != '-' && *form != '_')
		return  0;
	while  (*++form);
	return  1;
}

static int  ptrok(const char *ptr, const unsigned plng)
{
	int	lng = strlen(ptr);
	if  (lng <= 0  ||  lng > plng)
		return  0;
	do  if  (!isalnum(*ptr) && *ptr != '.' && *ptr != '-' && *ptr != '_')
		return  0;
	while  (*++ptr);
	return	1;
}

static int	fldsok23(struct spptr *old)
{
	if  (old->spp_class == 0  ||
	     !((old->spp_netflags & SPP_LOCALNET) || devok(old->spp_dev))  ||
	     !formok(old->spp_form, MAXFORM)  ||
	     !ptrok(old->spp_ptr, PTRNAMESIZE))
		return  0;
	return  1;
}

int  isit_r23(const int ifd, const struct stat *sb)
{
	struct	spptr	old;

	if  ((sb->st_size % sizeof(struct spptr)) != 0  &&  (!popts.ignsize  ||  ++popts.errors > popts.errtol))
		return  0;

	lseek(ifd, 0L, 0);

	while  (read(ifd, (char *) &old, sizeof(old)) == sizeof(old))
		if  (!fldsok23(&old)  &&  (!popts.ignfmt  ||  ++popts.errors > popts.errtol))
			return  0;
	return  1;
}

void  conv_r23(const int ifd)
{
	struct	spptr	old;

	lseek(ifd, 0L, 0);

	while  (read(ifd, (char *) &old, sizeof(old)) == sizeof(old))  {
		if  (!fldsok23(&old))
			continue;
		printf("gspl-padd -%c -%c -C %s -l \'%s\'",
			      old.spp_netflags & SPP_LOCALONLY? 's': 'w',
			      old.spp_netflags & SPP_LOCALNET? 'N': 'L',
			      hex_disp(old.spp_class, 0), old.spp_dev);
		if  (old.spp_comment[0])
			printf(" -D \'%s\'", old.spp_comment);
		printf(" %s \'%s\'\n", old.spp_ptr, old.spp_form);
	}
}

MAINFN_TYPE	main(int argc, char **argv)
{
	int		ifd, ch;
	struct	stat	sbuf;
	struct	flock	rlock;
	extern	int	optind;
	extern	char	*optarg;

	versionprint(argv, "$Revision: 1.2 $", 0);

	progname = argv[0];

	while  ((ch = getopt(argc, argv, "lsfe:D:")) != EOF)  {
		switch  (ch)  {
		default:
			goto  usage;
		case  'D':
			popts.srcdir = optarg;
			continue;
		case  'l':
			popts.localptr = 1;
			continue;
		case  's':
			popts.ignsize++;
			continue;
		case  'f':
			popts.ignfmt++;
			continue;
		case  'e':
			popts.errtol = atol(optarg);
			continue;
		}
	}

	if  ((ch = argc - optind) != 2  &&  ch != 3)  {
	usage:
		fprintf(stderr, "Usage: %s [-D dir] [-l] [-s] [-f] [-e n] pfile outfile [local]\n", argv[0]);
		return  100;
	}

	if  (ch > 2)
		popts.localptr = argv[optind+2][0] == 'l';

	if  (popts.srcdir)  {
		char	*newd = expand_srcdir(popts.srcdir);
		if  (!newd)  {
			fprintf(stderr, "Invalid source directory %s\n", popts.srcdir);
			return  10;
		}
		if  (stat(newd, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
			fprintf(stderr, "Source dir %s is not a directory\n", newd);
			return  12;
		}
		popts.srcdir = newd;
	}

	popts.outfile = argv[optind+1];
	if  (!freopen(popts.outfile, "w", stdout))  {
		fprintf(stderr, "Sorry cannot create \'%s\'\n", popts.outfile);
		return  3;
	}

	/* Now change directory to the source directory if specified */

	if  (popts.srcdir)  {
		popts.outfile = make_absolute(popts.outfile);
		if  (chdir(popts.srcdir) < 0)  {
			fprintf(stderr, "Cannot open source directory %s\n", popts.srcdir);
			unlink(popts.outfile);
			return  13;
		}
	}

	/* Open source file */

	if  ((ifd = open(argv[optind], O_RDONLY)) < 0)  {
		fprintf(stderr, "Sorry cannot open \'%s\'\n", argv[optind]);
		unlink(popts.outfile);
		return  2;
	}

	rlock.l_type = F_RDLCK;
	rlock.l_whence = 0;
	rlock.l_start = 0L;
	rlock.l_len = 0L;
	if  (fcntl(ifd, F_SETLKW, &rlock) < 0)  {
		fprintf(stderr, "Sorry could not lock %s\n", argv[optind]);
		return  3;
	}

	fstat(ifd, &sbuf);

	if  (isit_r23(ifd, &sbuf))  {
		printf("#! /bin/sh\n");
		conv_r23(ifd);
	}
	else  {
		fprintf(stderr, "I am confused about the format of your printer file\n");
		unlink(popts.outfile);
		return  9;
	}

	if  (popts.errors > 0)
		fprintf(stderr, "There were %ld error%s found\n", popts.errors, popts.errors > 1? "s": "");
	close(ifd);
#ifdef	HAVE_FCHMOD
	fchmod(fileno(stdout), 0755);
#else
	chmod(popts.outfile, 0755);
#endif
	fprintf(stderr, "Finished outputting printer file\n");
	return  0;
}
