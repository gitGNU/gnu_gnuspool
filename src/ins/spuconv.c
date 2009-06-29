/* spuconv.c -- convert user permissions file to shell script

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
   of the user file. */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include "defaults.h"
#include "spuser.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "files.h"
#include "ecodes.h"

struct  {
	char	*srcdir;	/* Directory we read from if not pwd */
	char	*outfile;	/* Output file */
	long	errtol;		/* Number of errors we'll take */
	long	errors;		/* Number we've had */
	short	ignsize;	/* Ignore file size */
	short	ignfmt;		/* Ignore file format errors */
}  popts;

extern char  *expand_srcdir(char *);
extern char  *make_absolute(char *);

const	char	*progname;

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

static int	formok(const char *form, const unsigned flng)
{
	int	lng = strlen(form);
	if  (lng <= 0  ||  lng > flng)
		return  0;
	do  if  (!isalnum(*form) && *form != '.' && *form != '-' && *form != '_')
		return  0;
	while  (*++form);
	return  1;
}

static int	spu23fldsok(struct spdet * od)
{
	if  (od->spu_minp == 0  ||
	     od->spu_defp == 0  ||
	     od->spu_maxp == 0  ||
	     od->spu_cps == 0  ||
	     !formok(od->spu_form, MAXFORM)  ||
	     od->spu_class == 0  ||
	     (od->spu_flgs & ~ALLPRIVS) != 0)
		return  0;
	return  1;
}

static int	isit_r23(const int fd, const struct stat *sb)
{
	int	uidn = 0;
	struct	sphdr	oh;
	struct	spdet	od;

	if  ((sb->st_size - sizeof(struct sphdr)) % sizeof(struct spdet) != 0  &&  (!popts.ignsize || ++popts.errors > popts.errtol))
		return  0;

	lseek(fd, 0L, 0);

	if  (read(fd, (char *) &oh, sizeof(oh)) != sizeof(oh))
		return  0;

	/* We'll have to give up if the header is knackered */

	if  (oh.sph_version != 23)
		return  0;

	if  (oh.sph_minp == 0  ||
	     oh.sph_defp == 0  ||
	     oh.sph_maxp == 0  ||
	     oh.sph_cps == 0  ||
	     !formok(oh.sph_form, MAXFORM)  ||
	     oh.sph_class == 0  ||
	     (oh.sph_flgs & ~ALLPRIVS) != 0)
		return  0;

	while  (read(fd, (char *) &od, sizeof(od)) == sizeof(od))  {
		if  (!od.spu_isvalid)
			continue;
		if  (spu23fldsok(&od))
			uidn++;
		else  if  (!popts.ignfmt || ++popts.errors > popts.errtol)
			return  0;
	}
	return  uidn > 0;
}

void	conv_r23(const int ifd)
{
	struct	sphdr	oh;
	struct	spdet	od;

	lseek(ifd, 0L, 0);

	if  (read(ifd, (char *) &oh, sizeof(oh)) != sizeof(oh))  {
		fprintf(stderr, "Bad header old file\n");
		exit(3);
	}

	printf("#! /bin/sh\n# Converted from user file\n\ngspl-uchange -DA -l %d -d %d -m %d -n %d -f %s -c %s -p 0x%lx\n",
		      oh.sph_minp, oh.sph_defp, oh.sph_maxp, oh.sph_cps,
		      oh.sph_form, hex_disp(oh.sph_class, 0), (unsigned long) oh.sph_flgs);

	while  (read(ifd, (char *) &od, sizeof(od)) == sizeof(od))  {
		if  (!od.spu_isvalid)
			continue;
		if  (!spu23fldsok(&od))
			continue;
		printf("gspl-uchange -l %d -d %d -m %d -n %d -f %s -c %s -p 0x%lx %s\n",
		      od.spu_minp, od.spu_defp, od.spu_maxp, od.spu_cps,
		      od.spu_form, hex_disp(od.spu_class, 0), (unsigned long) od.spu_flgs, prin_uname((uid_t) od.spu_user));
	}
}

MAINFN_TYPE	main(int argc, char **argv)
{
	int		ifd, ch;
	struct	stat	sbuf;
	extern	int	optind;
	extern	char	*optarg;

	versionprint(argv, "$Revision: 1.2 $", 0);
	progname = argv[0];

	while  ((ch = getopt(argc, argv, "sfe:D:")) != EOF)
		switch  (ch)  {
		default:
			goto  usage;
		case  'D':
			popts.srcdir = optarg;
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

	if  (argc - optind < 1  ||  argc - optind > 2)  {
	usage:
		fprintf(stderr, "Usage: %s [-D dir] [-s] [-f] [-e n] ufile outfile\n", argv[0]);
		return  100;
	}

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

	if  (argc - optind > 1  &&  strcmp(argv[optind+1], "-") != 0)  {
		popts.outfile = argv[optind+1];
		if  (!freopen(popts.outfile, "w", stdout))  {
			fprintf(stderr, "Sorry cannot create %s\n", popts.outfile);
			return  3;
		}
	}

	/* Now change directory to the source directory if specified */

	if  (popts.srcdir)  {
		if  (popts.outfile)
			popts.outfile = make_absolute(popts.outfile);
		if  (chdir(popts.srcdir) < 0)  {
			fprintf(stderr, "Cannot open source directory %s\n", popts.srcdir);
			if  (popts.outfile)
				unlink(popts.outfile);
			return  13;
		}
	}

	/* Open source user file */

	if  ((ifd = open(argv[optind], O_RDONLY)) < 0)  {
		fprintf(stderr, "Sorry cannot open %s\n", argv[optind]);
		if  (popts.outfile)
			unlink(popts.outfile);
		return  2;
	}

	fstat(ifd, &sbuf);
	if  (isit_r23(ifd, &sbuf))
		conv_r23(ifd);
	else  {
		fprintf(stderr, "I am confused about the format of your user file\n");
		if  (popts.outfile)
			unlink(popts.outfile);
		return  9;
	}

	if  (popts.errors > 0)
		fprintf(stderr, "There were %ld error%s found\n", popts.errors, popts.errors > 1? "s": "");
	close(ifd);
#ifdef	HAVE_FCHMOD
	if  (popts.outfile)
		fchmod(fileno(stdout), 0755);
#else
	if  (popts.outfile)
		chmod(popts.outfile, 0755);
#endif
	fprintf(stderr, "Converted spufile ok\n");
	return  0;
}
