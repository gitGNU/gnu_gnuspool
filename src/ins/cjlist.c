/* cjlist.c -- dump out job list into shell script

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
   of the job file. */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include <limits.h>
#include <pwd.h>
#include "defaults.h"
#include "files.h"
#include "ecodes.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "incl_unix.h"

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

struct  {
        char    *srcdir;        /* Directory we read from if not pwd */
        char    *outdir;        /* Directory we write to */
        char    *outfile;       /* Output file */
        long    errtol;         /* Number of errors we'll take */
        long    errors;         /* Number we've had */
        short   ignsize;        /* Ignore file size */
        short   ignfmt;         /* Ignore file format errors */
        short   ignusers;       /* Ignore invalid users */
        short   createmissjobs; /* Create missing jobs */
}  popts;

extern char *expand_srcdir(char *);
extern char *make_absolute(char *);
extern char *hex_disp(const classcode_t, const int);

void    nomem()
{
        fprintf(stderr, "Run out of memory\n");
        exit(E_NOMEM);
}

static int  formok(const char *form, const unsigned flng)
{
        int     lng = strlen(form);
        if  (lng <= 0  ||  lng > flng)
                return  0;
        do  if  (!isalnum(*form) && *form != '.' && *form != '-' && *form != '_')
                return  0;
        while  (*++form);
        return  1;
}

static int  ptrok(const char *ptr, const unsigned plng, const int ispatt)
{
        if  (strlen(ptr) > plng)
                return  0;
        if  (ispatt)  {
                while  (*ptr)  {
                        if  (!isgraph(*ptr))
                                return  0;
                        ptr++;
                }
        }
        else
                while  (*ptr)  {
                        if  (!isalnum(*ptr) && *ptr != '.' && *ptr != '-' && *ptr != '_')
                                return  0;
                        ptr++;
                }
        return  1;
}

static int  unameok(const char *un, const int_ugid_t uid)
{
        struct  passwd  *pw;

        if  (strlen(un) > UIDSIZE)
                return  0;
        if  (!popts.ignusers)  {
                if  (!(pw = getpwnam(un)))
                        return  0;
                if  (pw->pw_uid != uid)
                        return  0;
        }
        return  1;
}

static  char    misscreate[] = "***** Missing job recreated *****\n";

static int  jobok(const jobno_t jobnum, const long jsize, const int cr)
{
        char    *nam;
        struct  stat    sbuf;

        if  (jobnum == 0)
                return  0;
        nam = mkspid(SPNAM, jobnum);
        if  (stat(nam, &sbuf) < 0  ||  sbuf.st_size != jsize)  {
                if  (popts.createmissjobs)  {
                        int     fd;
                        long    nbytes = 0, towrite;
                        char    nbuf[PATH_MAX];
                        if  (!cr)
                                return  0;
                        fprintf(stderr, "Creating missing/incorrect job %ld\n", (long) jobnum);
                        sprintf(nbuf, "%s/%s", popts.outdir, nam);
                        if  ((fd = open(nbuf, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)  {
                                fprintf(stderr, "Sorry couldn't create saved job %s\n", nbuf);
                                return  0;
                        }
                        while  (nbytes < jsize)  {
                                towrite = jsize - nbytes;
                                if  (towrite > sizeof(misscreate) - 1)
                                        towrite = sizeof(misscreate) -1;
                                Ignored_error = write(fd, misscreate, towrite);
                                nbytes += towrite;
                        }
                        close(fd);
                        return  1;
                }
                else
                        return  0;
        }

        return  1;
}

static int      copyjob(const jobno_t jobnum)
{
        int     ch;
        FILE    *ifd, *ofd;
        char    *nam = mkspid(SPNAM, jobnum);
        char    path[PATH_MAX];

        if  (!(ifd = fopen(nam, "r")))
                return  0;
        sprintf(path, "%s/%s", popts.outdir, nam);
        if  (!(ofd = fopen(path, "w")))  {
                fclose(ifd);
                return  0;
        }
        while  ((ch = getc(ifd)) != EOF)
                putc(ch, ofd);
        fclose(ifd);
        fclose(ofd);
        return  1;
}

static void  outpagefile(const jobno_t jobnum)
{
        int     pfid, ii;
        char    *nam = mkspid(PFNAM, jobnum);
        char    *dblk;
        struct  pages   pblk;

        if  ((pfid = open(nam, O_RDONLY)) < 0)
               return;
        if  (read(pfid, (char *) &pblk, sizeof(pblk)) != sizeof(pblk))  {
                close(pfid);
                return;
        }
        if  (pblk.deliml <= 0  ||  pblk.deliml > 511)  {
                close(pfid);
                return;
        }
        if  (!(dblk = malloc((unsigned) pblk.deliml)))  {
                close(pfid);
                return;
        }
        if  (read(pfid, dblk, (unsigned) pblk.deliml) != pblk.deliml)  {
                free(dblk);
                close(pfid);
                return;
        }
        close(pfid);
        printf(" -d %ld -D \'", (long) pblk.delimnum);
        for  (ii = 0;  ii < pblk.deliml;  ii++)  {
                int     ch = dblk[ii] & 255;
                if  (!isascii(ch))
                        printf("\\x%.2x", ch);
                if  (iscntrl(ch))  {
                        switch  (ch)  {
                        case  033:
                                fputs("\\e", stdout);
                                break;
                        case  ('h' & 0x1f):
                                fputs("\\b", stdout);
                                break;
                        case  '\r':
                                fputs("\\r", stdout);
                                break;
                        case  '\n':
                                fputs("\\n", stdout);
                                break;
                        case  '\f':
                                fputs("\\f", stdout);
                                break;
                        case  '\t':
                                fputs("\\t", stdout);
                                break;
                        case  '\v':
                                fputs("\\v", stdout);
                                break;
                        default:
                                printf("^%c", ch | 0x40);
                                break;
                        }
                }
                else  switch  (ch)  {
                case  '\\':
                case  '^':
                        putchar(ch);
                default:
                        putchar(ch);
                        break;
                case  '\'':
                case  '\"':
                        putchar('\\');
                        putchar(ch);
                        break;
                }
        }
        free(dblk);
        putchar('\'');
}

static int      fldsok23(struct spq * old)
{
        if  (old->spq_class == 0  ||
              !formok(old->spq_form, MAXFORM) ||
              !ptrok(old->spq_ptr, JPTRNAMESIZE, 1) ||
              !unameok(old->spq_uname, old->spq_uid) ||
              !jobok(old->spq_job, old->spq_size, 0))
             return  0;
        return  1;
}

int  isit_r23(const int ifd, const struct stat *sb)
{
        struct  spq     old;

        if  ((sb->st_size % sizeof(struct spq)) != 0  &&  (!popts.ignsize || ++popts.errors > popts.errtol))
                return  0;

        lseek(ifd, 0L, 0);

        while  (read(ifd, (char *) &old, sizeof(old)) == sizeof(old))
                if  (!fldsok23(&old)  &&  (!popts.ignfmt || ++popts.errors > popts.errtol))
                        return  0;
        return  1;
}

void  conv_r23(const int ifd)
{
        struct  spq     old;

        printf("#! /bin/sh\n# Conversion of job list file\n\n");
        lseek(ifd, 0L, 0);

        while  (read(ifd, (char *) &old, sizeof(old)) == sizeof(old))  {
                if  (!fldsok23(&old))
                        continue;
                if  (!copyjob(old.spq_job))
                        continue;
                printf("gspl-pr -f %s -c %d -p %d -C %s -t %d -T %d",
                              old.spq_form, old.spq_cps,
                              old.spq_pri, hex_disp(old.spq_class, 0),
                              old.spq_ptimeout, old.spq_nptimeout);
                if  (old.spq_jflags & SPQ_NOH)
                        fputs(" -s", stdout);
                if  (old.spq_file[0])
                        printf(" -h \'%s\'", old.spq_file);
                if  (old.spq_jflags & SPQ_WRT)
                        fputs(" -w", stdout);
                if  (old.spq_jflags & SPQ_MAIL)
                        fputs(" -m", stdout);
                if  (old.spq_jflags & SPQ_WATTN)
                        fputs(" -A", stdout);
                if  (old.spq_jflags & SPQ_MATTN)
                        fputs(" -a", stdout);
                if  (old.spq_jflags & SPQ_LOCALONLY)
                        fputs(" -l", stdout);
                outpagefile(old.spq_job);
                if  (old.spq_ptr[0])
                        printf(" -P \'%s\'", old.spq_ptr);
                printf(" -U %s", old.spq_uname);
                if  (strcmp(old.spq_uname, old.spq_puname) != 0)
                        printf(" -u %s", old.spq_puname);
                if  (old.spq_jflags & (SPQ_ODDP|SPQ_EVENP))
                        printf(" -O %c", old.spq_jflags & SPQ_ODDP?
                                      (old.spq_jflags & SPQ_REVOE? 'A': 'O'):
                                      (old.spq_jflags & SPQ_REVOE? 'B': 'E'));
                if  (old.spq_start > 0  ||  old.spq_end <= LOTSANDLOTS)  {
                        fputs(" -R ", stdout);
                        if  (old.spq_start > 0)
                                printf("%ld", old.spq_start+1L);
                        putchar('-');
                        if  (old.spq_end <= LOTSANDLOTS)
                                printf("%ld", old.spq_end+1L);
                }
                if  (old.spq_flags[0])
                        printf(" -F \'%s\'", old.spq_flags);

                printf(" %s/%s\n", popts.outdir, mkspid(SPNAM, old.spq_job));
        }
}

MAINFN_TYPE     main(int argc, char **argv)
{
        int             ifd, ch;
        struct  stat    sbuf;
        struct  flock   rlock;
        extern  int     optind;
        extern  char    *optarg;

        versionprint(argv, "$Revision: 1.9 $", 0);

        while  ((ch = getopt(argc, argv, "umsfe:D:")) != EOF)
                switch  (ch)  {
                default:
                        goto  usage;
                case  'D':
                        popts.srcdir = optarg;
                        continue;
                case  'u':
                        popts.ignusers++;
                        continue;
                case  'm':
                        popts.createmissjobs++;
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

        if  (argc - optind != 3)  {
        usage:
                fprintf(stderr, "Usage: %s [-D dir] [-m] [-u] [-s] [-f] [-e n] jfile outfile workdir\n", argv[0]);
                return  100;
        }

        if  (popts.srcdir)  {
                char    *newd = expand_srcdir(popts.srcdir);
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

        /* Get out directory for saved jobs before we mess around with
           output files and suchwhat. */

        popts.outdir = argv[optind+2];
        if  (stat(popts.outdir, &sbuf) < 0)  {
                fprintf(stderr, "Cannot find directory %s\n", popts.outdir);
                return  4;
        }
        if  ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                fprintf(stderr, "%s is not a directory\n", popts.outdir);
                return  5;
        }

        /* Create output file, remembering to unlink it later
           if something goes wrong */

        popts.outfile = argv[optind+1];
        if  (!freopen(popts.outfile, "w", stdout))  {
                fprintf(stderr, "Sorry cannot create %s\n", popts.outfile);
                return  3;
        }

        /* Now change directory to the source directory if specified */

        if  (popts.srcdir)  {
                popts.outfile = make_absolute(popts.outfile);
                popts.outdir = make_absolute(popts.outdir);
                if  (chdir(popts.srcdir) < 0)  {
                        fprintf(stderr, "Cannot open source directory %s\n", popts.srcdir);
                        unlink(popts.outfile);
                        return  13;
                }
        }

        /* Open source job file */

        if  ((ifd = open(argv[optind], O_RDONLY)) < 0)  {
                fprintf(stderr, "Sorry cannot open %s\n", argv[optind]);
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
        if  (isit_r23(ifd, &sbuf))
                conv_r23(ifd);
        else  {
                fprintf(stderr, "I am confused about the format of your job file\n");
                unlink(popts.outfile);
                return  9;
        }

        if  (popts.errors > 0)
                fprintf(stderr, "There were %ld error%s found\n", popts.errors, popts.errors > 1? "s": "");
        close(ifd);
#ifdef  HAVE_FCHMOD
        fchmod(fileno(stdout), 0755);
#else
        chmod(popts.outfile, 0755);
#endif
        fprintf(stderr, "Finished outputting job file\n");
        return  0;
}
