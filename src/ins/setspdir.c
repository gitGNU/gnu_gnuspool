/* setspdir.c -- tidy up spool directories / reorganise in subdirs

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
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <ctype.h>
#include "defaults.h"
#include "files.h"
#include "incl_ugid.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "network.h"
#include "spq.h"
#include "incl_unix.h"
#include "incl_dir.h"
#ifndef HAVE_LONG_FILE_NAMES
#include "inline/owndirops.c"
#endif

struct  job_save  {
        struct  job_save *next;
        unsigned  char  in_jfile;               /* 0=not in jfile, 1=in it 2=in it more than once */
        unsigned  char  wrong_pl;               /* in wrong directory */
        jobno_t         jnum;
        jobno_t         newnum;
        char            *job_path;
        char            *page_path;
        char            *job_newname;
        char            *page_newname;
};

#define HASHMOD         293
#define hashit(jn)      (((unsigned)(jn)) % HASHMOD)

struct  job_save        *hashtab[HASHMOD];

struct  job_save        *dw_list;

int     yesall,                 /* Answer yes to all questions */
        cleardead,              /* Clear away any dead wood */
        renumber,               /* Keep jobs where they are - just renumber */
        set_subs = -1;          /* >= 0 means set number of sub directories to that number */

char    *progname,              /* Program name */
        *requeue_form;          /* Form type to requeue with */

int     existing_subds,         /* Existing sub directories */
        dupjoblist,             /* Duplicated jobs */
        renumbered,             /* Renumbered jobs */
        wrongplace,             /* Number of jobs in wrong place */
        deadwood;               /* Number of jobs not in job list */

char    *spdir;                 /* Spool directory */

#define SLURPSIZE       80
#define REBUILDJNAM     "Rebuild_jfile"
#define REBUILDTIT      "REQUEUED"
#define RENUMB_OFFSET   90000
#define REQUEUE_START   95000

/*      This are just to keep the library happy....*/

void    nomem()
{
        fprintf(stderr, "Run out of memory\n");
        exit(E_NOMEM);
}

static  struct job_save *alloc_j()
{
        struct  job_save        *result = (struct job_save *) malloc(sizeof(struct job_save));
        if  (!result)  {
                fprintf(stderr, "Run out of memory for jobs\n");
                exit(E_NOMEM);
        }
        result->next = (struct job_save *) 0;
        result->in_jfile = 0;
        result->wrong_pl = 0;
        result->jnum = 0;
        result->newnum = 0;
        result->job_path = (char *) 0;
        result->page_path = (char *) 0;
        result->job_newname = (char *) 0;
        result->page_newname = (char *) 0;
        return  result;
}

static int  is_prime(const int x)
{
        /*  Primes up to the square root of the largest number we allow - 999 */
        static  unsigned  char  ps[] =  { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37  };
        int     cnt;

        for  (cnt = 0;  cnt < sizeof(ps);  cnt++)  {
                int     pr = ps[cnt];
                if  (x < pr)
                        return  0;
                if  (x == pr)
                        return  1;
                if  (x % pr == 0)
                        return  0;
        }
        return  1;
}

/* Bullet-proof 'cous we daren't use gets */

static void  slurp(char *buf)
{
        int     ch, n = 0;

        while  ((ch = getchar()) != '\n'  &&  ch != EOF)
                if  (n < SLURPSIZE-1)
                        buf[n++] = ch;
        buf[n] = '\0';
}

/* Ask a question with y or n answer.  */

static int  Ask(char *msg)
{
        char    buf[SLURPSIZE];

        if  (yesall)
                return  1;

        for  (;;)  {
                char    *cp;

                fprintf(stderr, "%s [y/n]: ", msg);
                slurp(buf);

                for  (cp = buf;  isspace(*cp);  cp++)
                        ;

                switch  (*cp)  {
                case  'y':case  'Y':    return  1;
                case  'n':case  'N':    return  0;
                }

                fprintf(stderr, "Please reply 'y' or 'n'\n");
        }
}

/* Generate subd name in std format */

static char *gen_subd(const int n)
{
        static  char    res[4];
        sprintf(res, "%.3d", n);
        return  res;
}

/* "my" mkspid to generate the right answer with varying subdirectories.  */

char *my_mkspid(const char *nam, const jobno_t jnum, const int subds)
{
        static  char    result[NAMESIZE+4+1];
        if  (subds > 0)
                sprintf(result, "%.3lu/%s%.8lu", (unsigned long) (jnum % subds), nam, (unsigned long) jnum);
        else
                sprintf(result, "%s%.8lu", nam, (unsigned long) jnum);
        return  result;
}

/* Make directory and make it owned by spooler.  Cope with geysers who
   don't have mkdir We only ever do this chdir-ed to spdir */

static int  make_directory(char *d)
{
#ifdef  HAVE_MKDIR
        if  (mkdir(d, 0777) < 0  ||  chown(d, Daemuid, getegid()) < 0)
                return  0;
#else
        char    buf[80];
        sprintf(buf, "mkdir %s", d);
        if  (system(buf) != 0  ||  chown(d, Daemuid, getegid()) < 0)
                return  0;
#endif
        return  1;
}

/* Ditto remove directory.  We only ever do this chdir-ed to spdir */

static int  remove_directory(char *d)
{
#ifdef  S_IFLNK
        struct  stat    sbuf;
        if  (lstat(d, &sbuf) >= 0  &&  (sbuf.st_mode & S_IFMT) == S_IFLNK)
                return  unlink(d) >= 0;
#endif
#ifdef  HAVE_RMDIR
        return  rmdir(d) >= 0;
#else
        char    buf[80];
        sprintf(buf, "rmdir %s", d);
        return  system(buf) == 0;
#endif
}

/* Validate permissions etc of a subdirectory.  */

static int  val_subdir(const int n)
{
        int     errs = 0;
        int     nomatter = set_subs > 0  &&  set_subs <= n;
        char    *d = gen_subd(n);
        struct  stat    sbuf;

        if  (stat(d, &sbuf) < 0)  {

                fprintf(stderr, "Cannot find subdirectory %s/%s\n", spdir, d);

                if  (nomatter)  {
                        fprintf(stderr, "(no problem as you are reducing the subdirectories to %d)\n", set_subs);
                        return  0;
                }

                if  (Ask("Create it"))  {
                        if  (!make_directory(d))  {
                                fprintf(stderr, "Failed to create subdirectory %s = aborting\n", d);
                                exit(E_SETUP);
                        }
                }

                return  1;
        }

        if  ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                fprintf(stderr, "Subdirectory %s/%s is not a directory\n", spdir, d);
                if  (nomatter)  {
                        fprintf(stderr, "(no problem as you are reducing the subdirectories to %d)\n", set_subs);
                        return  0;
                }
#ifdef  S_IFLNK
                if  ((sbuf.st_mode & S_IFMT) == S_IFLNK)  {
                        fprintf(stderr, "It's a symbolic link which goes nowhere.\n");
                        if  (Ask("Fix it"))  {
                                if  (unlink(d) < 0  ||  !make_directory(d))  {
                                        fprintf(stderr, "Unable to recreate subdirectory %s\n", d);
                                        exit(E_SETUP);
                                }
                        }
                }
#endif
                exit(E_SETUP);
        }

        if  (sbuf.st_uid != Daemuid)  {
                errs++;
                fprintf(stderr, "Subdirectory %s/%s is owned by %s not %s\n", spdir, d, prin_uname(sbuf.st_uid), prin_uname(Daemuid));
                if  (nomatter)  {
                        fprintf(stderr, "(no problem as you are reducing the subdirectories to %d)\n", set_subs);
                        return  errs;
                }
                if  (Ask("Fix it"))  {
                        if  (chown(d, Daemuid, sbuf.st_gid) < 0)  {
                                fprintf(stderr, "Failed to change owner of %s - aborting\n", d);
                                exit(E_SETUP);
                        }
                }
        }

        if  ((sbuf.st_mode & 0700) != 0700)  {
                errs++;
                fprintf(stderr, "Subdirectory %s/%s has incorrect perms - %o no RWX\n", spdir, d, sbuf.st_mode & 07777);
                if  (nomatter)  {
                        fprintf(stderr, "(no problem as you are reducing the subdirectories to %d)\n", set_subs);
                        return  errs;
                }
                if  (Ask("Fix it"))  {
                        if  (chmod(d, 0700 | (sbuf.st_mode & 07777)) < 0)  {
                                fprintf(stderr, "Failed to fix modes - aborting\n");
                                exit(E_SETUP);
                        }
                }
        }

        return  errs;
}

/* Check permissions etc of the main directory.  */

static int  val_maindir()
{
        int     errs = 0;
        struct  stat    sbuf;

        if  (stat(".", &sbuf) < 0)  {
                fprintf(stderr, "Cannot \"stat\" main directory %s - aborting\n", spdir);
                exit(E_SETUP);
        }

        if  (sbuf.st_uid != Daemuid)  {
                errs++;
                fprintf(stderr, "Spool directory %s is owned by %s not %s\n", spdir, prin_uname(sbuf.st_uid), prin_uname(Daemuid));
                if  (Ask("Fix it"))  {
                        if  (chown(".", Daemuid, sbuf.st_gid) < 0)  {
                                fprintf(stderr, "Failed to change owner of %s - aborting\n", spdir);
                                exit(E_SETUP);
                        }
                }
        }

        if  ((sbuf.st_mode & 0700) != 0700)  {
                errs++;
                fprintf(stderr, "Spool directory %s has incorrect perms - %o no RWX\n", spdir, sbuf.st_mode & 07777);
                if  (Ask("Fix it"))  {
                        if  (chmod(".", 0700 | (sbuf.st_mode & 07777)) < 0)  {
                                fprintf(stderr, "Failed to fix modes - aborting\n");
                                exit(E_SETUP);
                        }
                }
        }

        return  errs;
}

/* Look up a job number in the hash table.  */

static struct job_save *find_hash(const jobno_t jn)
{
        struct  job_save  *result;
        for  (result = hashtab[hashit(jn)];  result;  result = result->next)
                if  (result->jnum == jn)
                        return  result;
        return  (struct job_save *) 0;
}

/* Slurp up job list from spshed_jlist and initialise hash table from result.  */

static int  load_joblist()
{
        int             fd, errs = 0;
        struct  spq     inj;

        if  ((fd = open(JFILE, O_RDONLY)) < 0)  {
                fprintf(stderr, "Sorry cannot open job description file\n");
                return  1;
        }

        while  (read(fd, (char *) &inj, sizeof(inj)) == sizeof(inj))  {
                struct  job_save  *jp;
                unsigned        hashval;
                if  (find_hash(inj.spq_job))  {
                        fprintf(stderr, "Duplicated reference to job %ld in job list\n", (long) inj.spq_job);
                        dupjoblist++;
                        errs++;
                        continue;
                }
                jp = alloc_j();
                hashval = hashit(inj.spq_job);
                jp->jnum = inj.spq_job;
                jp->in_jfile = 1;
                jp->next = hashtab[hashval];
                hashtab[hashval] = jp;
        }
        close(fd);
        return  errs;
}

/* Scan the subdirectories for things looking like job or page files.  */

static void  load_subds(const int n)
{
        DIR     *dfd;
        struct  dirent  *dp;
        struct  stat    sbuf;
        char    nbuf[4+2*NAMESIZE+1];

        if  (!(dfd = opendir(gen_subd(n))))
                return;

        while  ((dp = readdir(dfd)))  {

                jobno_t jn;
                struct  job_save  *jp;

                /* Skip over '.' and '..' */

                if  (dp->d_name[0] == '.'  &&  (dp->d_name[1] == '\0' ||
                     (dp->d_name[1] == '.' &&  dp->d_name[2] == '\0')))
                        continue;

                /* Safety in case of really long name */

                if  (strlen(dp->d_name) > 2*NAMESIZE)  {
                        fprintf(stderr, "Unexpected entry %s in subd %.3d\n", dp->d_name, n);
                        continue;
                }

                sprintf(nbuf, "%.3d/%s", n, dp->d_name);

                if  (stat(nbuf, &sbuf) < 0)
                        continue;

                if  (sbuf.st_uid != Daemuid)  {
                        fprintf(stderr, "%s does is not owned by %s\n", nbuf, prin_uname(Daemuid));
                        if  (Ask("Fix it"))
                                Ignored_error = chown(nbuf, Daemuid, sbuf.st_gid);
                }

                /* Check for directories and funny things */

                if  ((sbuf.st_mode & S_IFMT) != S_IFREG)  {
                        if  ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                                fprintf(stderr, "Unexpected directory %s\n", nbuf);
                                if  (Ask("Delete it"))
                                        remove_directory(nbuf);
                                continue;
                        }
                        fprintf(stderr, "Unexpected dir entry %s\n", nbuf);
                        if  (Ask("Delete it"))
                                unlink(nbuf);
                        continue;
                }

                if  (!(sbuf.st_mode & 0400))  {
                        fprintf(stderr, "%s is not readable\n", nbuf);
                        if  (Ask("Fix it"))
                                chmod(nbuf, 0400);
                }

                /* If we find something looking like a job file, check
                   it makes sense and record details.  */

                if  (strncmp(dp->d_name, SPNAM, 2) == 0  &&  isdigit(dp->d_name[2]))  {
                        jn = atol(&dp->d_name[2]);
                        if  (jn % existing_subds != n)  {
                                fprintf(stderr, "job %ld in subdirectory %d does not belong there\n", (long) jn, n);
                                jp = alloc_j();
                                jp->wrong_pl = 1;
                                jp->next = dw_list;
                                dw_list = jp;
                                jp->jnum = jn;
                                jp->job_path = stracpy(nbuf);
                                wrongplace++;
                                continue;
                        }
                        if  ((jp = find_hash(jn)))  {
                                jp->job_path = stracpy(nbuf);
                                continue;
                        }
                        jp = alloc_j();
                        jp->next = dw_list;
                        dw_list = jp;
                        jp->jnum = jn;
                        jp->job_path = stracpy(nbuf);
                        deadwood++;
                        continue;
                }

                /* We're less fussy with page files.  */

                if  (strncmp(dp->d_name, PFNAM, 2) == 0  &&  isdigit(dp->d_name[2]))  {
                        jn = atol(&dp->d_name[2]);
                        if  (jn % existing_subds == n  &&  (jp = find_hash(jn)))  {
                                jp->page_path = stracpy(nbuf);
                                continue;
                        }
                        unlink(nbuf);
                }

                /* Any other kind of files get deleted if we're
                   clearing dead wood.  */

                if  (cleardead  ||  (strncmp(dp->d_name, ERNAM, 2) == 0  &&  isdigit(dp->d_name[2])))
                     unlink(nbuf);
        }

        closedir(dfd);
}

/* Ditto for main directory.  */

static void  load_maind()
{
        DIR     *dfd;
        struct  dirent  *dp;
        struct  stat    sbuf;

        if  (!(dfd = opendir(".")))
                return;

        while  ((dp = readdir(dfd)))  {
                jobno_t jn;
                struct  job_save  *jp;

                /* Skip over '.' and '..' */

                if  (dp->d_name[0] == '.'  &&  (dp->d_name[1] == '\0' ||
                     (dp->d_name[1] == '.' &&  dp->d_name[2] == '\0')))
                        continue;

                if  (strlen(dp->d_name) > 2*NAMESIZE)
                        continue;
                if  (stat(dp->d_name, &sbuf) < 0)
                        continue;
                if  ((sbuf.st_mode & S_IFMT) != S_IFREG)
                        continue;

                if  (sbuf.st_uid != Daemuid)  {
                        fprintf(stderr, "%s does is not owned by %s\n", dp->d_name, prin_uname(Daemuid));
                        if  (Ask("Fix it"))
                                Ignored_error = chown(dp->d_name, Daemuid, sbuf.st_gid);
                }
                if  (!(sbuf.st_mode & 0400))  {
                        fprintf(stderr, "%s is not readable\n", dp->d_name);
                        if  (Ask("Fix it"))
                                chmod(dp->d_name, 0400);
                }

                /* If we find something looking like a job file, check
                   it makes sense and record details.  */

                if  (strncmp(dp->d_name, SPNAM, 2) == 0  &&  isdigit(dp->d_name[2]))  {
                        jn = atol(&dp->d_name[2]);
                        if  (existing_subds > 0)  {
                                fprintf(stderr, "job %ld in main directory does not belong there\n", (long) jn);
                                jp = alloc_j();
                                jp->wrong_pl = 1;
                                jp->next = dw_list;
                                dw_list = jp;
                                jp->jnum = jn;
                                jp->job_path = stracpy(dp->d_name);
                                wrongplace++;
                                continue;
                        }
                        if  ((jp = find_hash(jn)))  {
                                jp->job_path = stracpy(dp->d_name);
                                continue;
                        }
                        jp = alloc_j();
                        jp->next = dw_list;
                        dw_list = jp;
                        jp->jnum = jn;
                        jp->job_path = stracpy(dp->d_name);
                        deadwood++;
                        continue;
                }

                /* We're less fussy with page files - just delete them
                   if they don't fit.  */

                if  (strncmp(dp->d_name, PFNAM, 2) == 0  &&  isdigit(dp->d_name[2]))  {
                        jn = atol(&dp->d_name[2]);
                        if  (existing_subds == 0  &&  (jp = find_hash(jn)))  {
                                jp->page_path = stracpy(dp->d_name);
                                continue;
                        }
                        unlink(dp->d_name);
                }

                /* Just delete error files.  */

                 if  (strncmp(dp->d_name, ERNAM, 2) == 0  &&  isdigit(dp->d_name[2]))
                        unlink(dp->d_name);
        }
        closedir(dfd);
}

/* Expunge from the spshed_jfiles file jobs which we didn't find the files for .  */

static void  kill_nojobfiles()
{
        int             fd1, fd2;
        unsigned        cnt, errs = 0;
        struct  job_save        *jp;
        struct  spq     inj;
        struct  stat    sbuf;

        for  (cnt = 0;  cnt < HASHMOD;  cnt++)
                for  (jp = hashtab[cnt];  jp;  jp = jp->next)
                        if  (jp->in_jfile  &&  !jp->job_path)
                                errs++;

        if  (errs == 0  &&  dupjoblist == 0)
                return;

        if  (errs != 0)
                fprintf(stderr, "There are %u job(s) in the job list with no file\n", errs);
        if  (dupjoblist != 0)
                fprintf(stderr, "There are %d job(s) in the job list pointing to the same file\n", errs);

        if  (!Ask("Rewrite job list"))
                return;
        if  ((fd1 = open(JFILE, O_RDONLY)) < 0)
                return;
        if  ((fd2 = open(REBUILDJNAM, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)  {
                close(fd1);
                return;
        }

        while  (read(fd1,  (char *) &inj, sizeof(inj)) == sizeof(inj))  {
                if  (!((jp = find_hash(inj.spq_job))  &&  jp->job_path))
                        continue;
                if  (jp->in_jfile > 1)
                        continue;
                jp->in_jfile = 2;       /* To skip duplicates */
                Ignored_error = write(fd2, (char *) &inj, sizeof(inj));
        }

        fstat(fd1, &sbuf);
#ifdef  HAVE_FCHOWN
        Ignored_error = fchown(fd2, sbuf.st_uid, sbuf.st_gid);
#else
        Ignored_error = chown(REBUILDJNAM, sbuf.st_uid, sbuf.st_gid);
#endif
        close(fd1);
        close(fd2);

#ifdef  HAVE_RENAME
        rename(REBUILDJNAM, JFILE);
#else
        unlink(JFILE);
        link(REBUILDJNAM, JFILE);
        unlink(REBUILDJNAM);
#endif
}

/* Zap dead wood type jobs.  */

static void  clearaway_jobs()
{
        struct  job_save        *jp, *nxt = (struct job_save *) 0;

        for  (jp = dw_list;  jp;  jp = nxt)  {
                unlink(jp->job_path);
                nxt = jp->next;
                free(jp->job_path);
                free((char *) jp);
        }
        dw_list = (struct job_save *) 0;
}

/* Try to rename job, allowing for cross-device symbolic links.  */

static int  job_rename(char *oldn, char *newn)
{
        int     ofd, nfd, bytes;
        char    bigbuff[1024];
#ifdef  HAVE_RENAME
        if  (rename(oldn, newn) >= 0)
                return  1;
#else
        if  (link(oldn, newn) >= 0)  {
                if  (unlink(oldn) < 0)  {
                        unlink(newn);
                        return  0;
                }
                return  1;
        }
#endif
        if  ((ofd = open(oldn, O_RDONLY)) < 0)  {
                fprintf(stderr, "Help cannot open %s\n", oldn);
                return  0;
        }
        if  ((nfd = open(newn, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)  {
                fprintf(stderr, "Help cannot create %s\n", newn);
                close(ofd);
                return  0;
        }
#ifdef  HAVE_FCHOWN
        Ignored_error = fchown(nfd, Daemuid, getegid());
#else
        Ignored_error = chown(newn, Daemuid, getegid());
#endif

        while  ((bytes = read(ofd, bigbuff, sizeof(bigbuff))) > 0)
                if  (write(nfd, bigbuff, bytes) != bytes)  {
                        fprintf(stderr, "%s - running out of space\n", newn);
                        close(ofd);
                        close(nfd);
                        unlink(newn);
                        return  0;
                }

        if  (close(nfd) < 0)  {
                fprintf(stderr, "%s - running out of space\n", newn);
                close(ofd);
                close(nfd);
                unlink(newn);
                return  0;
        }
        close(ofd);

        if  (unlink(oldn) < 0)  {
                fprintf(stderr, "Could not delete old name %s\n", oldn);
                unlink(newn);
                return  0;
        }
        return  1;
}

/* Put requeued jobs back on the job queue.  We set copies to 0 and priority to 1.  */

static void  requeue_jobs()
{
        struct  job_save        *jp, *nxt = (struct job_save *) 0;
        jobno_t         jn = REQUEUE_START;
        int     fd;
        unsigned        hashval;
        struct  spq     newj;
        struct  stat    sbuf;

        BLOCK_ZERO(&newj, sizeof(newj));
        newj.spq_nptimeout = newj.spq_ptimeout = 168;
        newj.spq_pri = 1;
        newj.spq_class = 0xffffffffL;
        newj.spq_end = LOTSANDLOTS;
        newj.spq_uid = Daemuid;
        strncpy(newj.spq_uname, SPUNAME, UIDSIZE);
        strncpy(newj.spq_puname, SPUNAME, UIDSIZE);
        strncpy(newj.spq_form, requeue_form, MAXFORM);
        strncpy(newj.spq_file, REBUILDTIT, MAXTITLE);

        if  ((fd = open(JFILE, O_WRONLY|O_APPEND)) < 0)  {
                if  ((fd = open(JFILE, O_WRONLY|O_CREAT, 0666)) < 0)  {
                        fprintf(stderr, "Cannot output to job file\n");
                        return;
                }
#ifdef  HAVE_FCHOWN
                Ignored_error = fchown(fd, Daemuid, getegid());
#else
                Ignored_error = chown(JFILE, Daemuid, getegid());
#endif
        }

        for  (jp = dw_list;  jp;  jp = nxt)  {
                char    *npath;

                nxt = jp->next; /* ere we mangle it */

                /*      Find the next available job number  */

                while  (find_hash(jn))
                        jn++;
                newj.spq_job = jn;
                stat(jp->job_path, &sbuf);
                newj.spq_time = (LONG) sbuf.st_mtime;
                newj.spq_size = (LONG) sbuf.st_size;
                npath = my_mkspid(SPNAM, jn, existing_subds);
                if  (strcmp(npath, jp->job_path) != 0)  { /* Better check... */
                        job_rename(jp->job_path, npath);
                        free(jp->job_path);
                        jp->job_path = stracpy(npath);
                }
                Ignored_error = write(fd, (char *) &newj, sizeof(newj));
                jp->jnum = jn;
                jp->in_jfile = 1;
                hashval = hashit(jn);
                jp->next = hashtab[hashval];
                hashtab[hashval] = jp;
                fprintf(stderr, "Requeued orphaned job file as job %ld\n", (long) jn);
                jn++;
        }

        close(fd);
}

static int  move_jobs()
{
        unsigned        cnt;
        struct  job_save        *jp;

        for  (cnt = 0;  cnt < HASHMOD;  cnt++)
                for  (jp = hashtab[cnt];  jp;  jp = jp->next)  {
                        char    *nnam = my_mkspid(SPNAM, jp->jnum, set_subs);

                        /* Might end up in the same place, in which case, skip it. */

                        if  (strcmp(nnam, jp->job_path) == 0)
                                continue;

                        /* We might be able to renumber the jobs - but not if we
                           are reducing the subdirectories and we are off the end. */

                        if  (renumber  &&  cnt < set_subs)  {

                                /* Find a vacant job number. */

                                jobno_t jn = cnt + set_subs + RENUMB_OFFSET;
                                jn -= jn % set_subs;

                                while  (find_hash(jn))
                                        jn += set_subs;

                                nnam = my_mkspid(SPNAM, jn, set_subs);

                                if  (!job_rename(jp->job_path, nnam))
                                        goto  abort_move;
                                jp->job_newname = stracpy(nnam);

                                if  (jp->page_path)  {
                                        nnam = my_mkspid(PFNAM, jn, set_subs);
                                        if  (!job_rename(jp->page_path, nnam))
                                                goto  abort_move;
                                        jp->page_newname = stracpy(nnam);
                                }
                                jp->newnum = jn;
                                renumbered++;
                        }
                        else  {
                                if  (!job_rename(jp->job_path, nnam))
                                        goto  abort_move;
                                jp->job_newname = stracpy(nnam);
                                if  (jp->page_path)  {
                                        nnam = my_mkspid(PFNAM, jp->jnum, set_subs);
                                        if  (!job_rename(jp->page_path, nnam))
                                                goto  abort_move;
                                        jp->page_newname = stracpy(nnam);
                                }
                        }
                }
        return  1;

 abort_move:
        fprintf(stderr, "Cancelling job move\n");

        for  (cnt = 0;  cnt < HASHMOD;  cnt++)
                for  (jp = hashtab[cnt];  jp;  jp = jp->next)  {
                        if  (jp->job_newname  &&  !job_rename(jp->job_newname, jp->job_path))
                                fprintf(stderr, "Panic! could not fix back %s\n", jp->job_path);
                        if  (jp->page_newname  &&  !job_rename(jp->page_newname, jp->page_path))
                                fprintf(stderr, "Panic! could not fix back %s\n", jp->job_path);
                }
        return  0;
}

/* Mangle the job file to cope with new job numbers */

static int  renumber_jobs()
{
        int     fd;
        struct  job_save        *jp;
        struct  spq             inj;

        if  ((fd = open(JFILE, O_RDWR)) < 0)  {
                fprintf(stderr, "Could not reopen job file\n");
                return  0;
        }

        while  (read(fd, (char *) &inj, sizeof(inj)) == sizeof(inj))  {
                jp = find_hash(inj.spq_job);
                if  (jp->newnum != 0)  {
                        inj.spq_job = jp->newnum;
                        lseek(fd, -(long) sizeof(inj), 1);
                        Ignored_error = write(fd, (char *) &inj, sizeof(inj));
                }
        }
        close(fd);
        return  1;
}

/* Edit the master config file.  */

static void  rewrite_mcfile()
{
        FILE    *mc1, *tmc;
        int     hadit = 0;
        char    buf[120];
        static  char    ss[] = "SPOOLSUBDS=";

        if  (!(mc1 = fopen(MASTER_CONFIG, "r")))  {
                time_t          now = time((time_t *) 0);
                struct  tm      *tp = localtime(&now);

                if  (!(mc1 = fopen(MASTER_CONFIG, "w")))  {
                        fprintf(stderr, "Help! Cannot create master config file\n");
                        exit(E_SETUP);
                }
#ifdef  HAVE_FCHMOD
                fchmod(fileno(mc1), 0644);
#else
                chmod(MASTER_CONFIG, 0644);
#endif
                fprintf(mc1, "# Constructed by %s at %.2d:%.2d:%.2d on %.2d/%.2d/%.2d\n\n",
                        progname, tp->tm_hour, tp->tm_min, tp->tm_sec, tp->tm_mday, tp->tm_mon+1, tp->tm_year%100);

                fprintf(mc1, "# Number of spool subdirectories - EDIT WITH EXTREME CARE!!\n%s%d\n", ss, set_subs);
                fclose(mc1);
                return;
        }

        if  (!(tmc = fopen("/tmp/xtmcfile", "w+")))  {
                fprintf(stderr, "Help! Cannot create temp master config file\n");
                exit(E_SETUP);
        }

        while  (fgets(buf, sizeof(buf), mc1))  {
                if  (strncmp(buf, ss, sizeof(ss)-1) == 0)  {
                        if  (!hadit)
                                fprintf(tmc, "%s%d\n", ss, set_subs);
                        hadit++;
                        continue;
                }
                fputs(buf, tmc);
        }
        if  (!hadit)
                fprintf(tmc, "# Number of spool subdirectories - EDIT WITH EXTREME CARE!!\n%s%d\n", ss, set_subs);
        rewind(tmc);
        fclose(mc1);
        if  (!(mc1 = fopen(MASTER_CONFIG, "w")))  {
                fprintf(stderr, "Help! Cannot recreate master config file\n");
                exit(E_SETUP);
        }
        while  (fgets(buf, sizeof(buf), tmc))
                fputs(buf, mc1);
        fclose(mc1);
        fclose(tmc);
}

MAINFN_TYPE  main(int argc, char **argv)
{
        int             ch, errs = 0;
        int_ugid_t      chku;
        char            *ss;
        extern  int     optind;
        extern  char    *optarg;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  (geteuid() != ROOTID)  {
                fprintf(stderr, "Sorry, but you must run this as the super-user\n");
                return  E_NOPRIV;
        }

        umask(0077);

        if  (msgget(MSGID, 0) >= 0)  {
                fprintf(stderr, "Sorry - cannot continue, spooler is running\n");
                return  E_RUNNING;
        }

        progname = argv[0];

        while  ((ch = getopt(argc, argv, "ryds:f:")) != EOF)
                switch  (ch)  {
                default:
                        goto  usage;
                case  'r':
                        renumber++;
                        continue;
                case  'y':
                        yesall++;
                        continue;
                case  'd':
                        cleardead++;
                        continue;
                case  's':
                        set_subs = atoi(optarg);
                        continue;
                case  'f':
                        requeue_form = optarg;
                        continue;
                }

        if  (argc - optind != 0)  {
        usage:
                fprintf(stderr, "Usage: %s [-d] [-s n]\n", progname);
                return  E_USAGE;
        }

        if  (set_subs > 0)  {
                if  (set_subs > 999)  {
                        fprintf(stderr, "%s - %d is too big a number of subdirectories, please try again\n", progname, set_subs);
                        return  E_USAGE;
                }
                if  (!is_prime(set_subs))  {
                        fprintf(stderr, "%d is not a prime number of directories\n", set_subs);
                        if  (!Ask("Continue anyway"))
                                return  E_USAGE;
                }
        }

        if  ((chku = lookup_uname(SPUNAME)) == UNKNOWN_UID)
                Daemuid = ROOTID;
        else
                Daemuid = chku;

        init_mcfile();
        ss = envprocess("${SPOOLSUBDS-0}");
        existing_subds = atoi(ss);
        free(ss);

        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)  {
                fprintf(stderr, "Cannot find spool directory %s\n", spdir);
                return  E_NOCHDIR;
        }

        errs += val_maindir();

        if  (load_joblist()  &&  !Ask("Continue"))
                return  E_SETUP;

        load_maind();           /* Check main directory anyhow to check for dead wood */

        if  (existing_subds > 0)  {
                int     cnt;

                for  (cnt = 0;  cnt < existing_subds;  cnt++)
                        errs += val_subdir(cnt);

                for  (cnt = 0;  cnt < existing_subds;  cnt++)
                        load_subds(cnt);
        }

        kill_nojobfiles();
        if  (wrongplace > 0  ||  deadwood > 0)  {
                if  (requeue_form)
                        requeue_jobs();
                else  if  (cleardead  ||  Ask("Clear away orphaned jobs"))
                        clearaway_jobs();
        }

        if  (set_subs >= 0  &&  set_subs != existing_subds)  {
                int     cnt;
                if  (set_subs > existing_subds)
                        for  (cnt = existing_subds;  cnt < set_subs;  cnt++)
                                if  (!make_directory(gen_subd(cnt))  &&  val_subdir(cnt))  {
                                        fprintf(stderr, "Cannot create directories to reassign\n");
                                        return  E_SETUP;
                                }

                if  (!move_jobs())
                        return  E_IO;

                if  (renumbered > 0  &&  !renumber_jobs())
                        return  E_IO;

                if  (set_subs < existing_subds)
                        for  (cnt = set_subs;  cnt < existing_subds;  cnt++)
                                if  (!remove_directory(gen_subd(cnt)))
                                        fprintf(stderr, "Cannot remove directory %.3d - you may have to\n", cnt);
                rewrite_mcfile();
        }

        return  0;
}
