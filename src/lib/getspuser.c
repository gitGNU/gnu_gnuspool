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
#include "errnums.h"
#include "defaults.h"
#include "spuser.h"
#include "files.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "incl_sig.h"

#define INC_USERDETS    10

struct  sphdr   Spuhdr;
static  struct  spdet  *userdets_buf;
static  int     Num_userdets, Max_userdets;

static  int     spuf_fid = -1;
int             spu_new_format;
static  time_t  last_mod_time;

static  unsigned  char  igsigs[]= { SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGALRM, SIGUSR1, SIGUSR2 };

#ifdef  UNSAFE_SIGNALS
static  RETSIGTYPE      (*oldsigs[sizeof(igsigs)])(int);
#endif

/* Lock the whole caboodle */

static  void  lockit(const int type)
{
        struct  flock   lk;

        lk.l_type = (SHORT) type;
        lk.l_whence = 0;
        lk.l_start = 0;
        lk.l_len = 0;
        lk.l_pid = 0;
        if  (fcntl(spuf_fid, F_SETLKW, &lk) < 0)  {
                print_error($E{Cannot lock user ctrl file});
                exit(E_SETUP);
        }
}

/* Unlock the whole caboodle */

static  void  unlockit()
{
        struct  flock   lk;

        lk.l_type = F_UNLCK;
        lk.l_whence = 0;
        lk.l_start = 0;
        lk.l_len = 0;
        lk.l_pid = 0;
        if  (fcntl(spuf_fid, F_SETLKW, &lk) < 0)  {
                print_error($E{Cannot unlock user ctrl file});
                exit(E_SETUP);
        }
}

static  void    savesigs(const int saving)
{
        int     cnt;
#ifdef  HAVE_SIGACTION
        sigset_t        nset;
        sigemptyset(&nset);
        for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
                sigaddset(&nset, igsigs[cnt]);
        sigprocmask(saving? SIG_BLOCK: SIG_UNBLOCK, &nset, (sigset_t *) 0);
#elif   defined(STRUCT_SIG)
        int     msk = 0;
        for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
                msk |= sigmask(igsigs[cnt]);
        if  (saving)
                sigsetmask(sigsetmask(~0) | msk);
        else
                sigsetmask(sigsetmask(~0) & ~msk);
#elif   defined(HAVE_SIGSET)
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

/* Binary search for user id in sorted list
   Return the index of where it's found (or just past that) which might be off the end. */

static  int  bsearch_spdet(const uid_t uid)
{
        int  first = 0, last = Num_userdets, mid;

        while  (first < last)  {
                struct  spdet  *sp;
                mid = (first + last) / 2;
                sp = &userdets_buf[mid];
                if  (sp->spu_user == uid)
                        return  mid;
                if  (sp->spu_user > uid)
                        last = mid;
                else
                        first = mid + 1;
        }
        return  first;
}

void    copy_defs(struct spdet *res, uid_t uid)
{
        res->spu_isvalid = SPU_VALID;
        res->spu_user = uid;
        strcpy(res->spu_form, Spuhdr.sph_form);
        strcpy(res->spu_formallow, Spuhdr.sph_formallow);
        strcpy(res->spu_ptr, Spuhdr.sph_ptr);
        strcpy(res->spu_ptrallow, Spuhdr.sph_ptrallow);
        res->spu_minp = Spuhdr.sph_minp;
        res->spu_maxp = Spuhdr.sph_maxp;
        res->spu_defp = Spuhdr.sph_defp;
        res->spu_cps = Spuhdr.sph_cps;
        res->spu_flgs = Spuhdr.sph_flgs;
        res->spu_class = Spuhdr.sph_class;
}

int     issame_defs(const struct spdet *item)
{
        return  item->spu_minp == Spuhdr.sph_minp  &&
                item->spu_maxp == Spuhdr.sph_maxp  &&
                item->spu_defp == Spuhdr.sph_defp  &&
                item->spu_cps == Spuhdr.sph_cps  &&
                item->spu_flgs == Spuhdr.sph_flgs  &&
                item->spu_class == Spuhdr.sph_class  &&
                strcmp(item->spu_form, Spuhdr.sph_form) == 0  &&
                strcmp(item->spu_formallow, Spuhdr.sph_formallow) == 0  &&
                strcmp(item->spu_ptr, Spuhdr.sph_ptr) == 0 &&
                strcmp(item->spu_ptrallow, Spuhdr.sph_ptrallow) == 0;
}

/* Read user from file or memory.
   File is assumed to be locked. */

void  readu(const uid_t uid, struct spdet *item)
{
        int     whu;
        struct  spdet  *sp;

        if  (!spu_new_format)  {

                /* If it's below the magic number at which we store them as a
                   vector, jump to the right place and go home.  */

                if  ((ULONG) uid < SMAXUID)  {
                        lseek(spuf_fid, (long)(sizeof(struct sphdr) + uid * sizeof(struct spdet)), 0);
                        if  (read(spuf_fid, (char *) item, sizeof(struct spdet)) != sizeof(struct spdet)  ||  !item->spu_isvalid)
                                copy_defs(item, uid);
                        return;
                }

        }

        /* For new format files and for excess over SMAXUID of old format, we save them in the vector */

        whu = bsearch_spdet(uid);
        sp = &userdets_buf[whu];
        if  (whu < Num_userdets  &&  sp->spu_user == uid)  {
                *item = *sp;
                return;
        }

        /* Otherwise copy defaults */

        copy_defs(item, uid);
}

static  void    insert_item_vec(const struct spdet *item, const int whu)
{
        struct  spdet  *sp = &userdets_buf[whu];
        int     cnt;

        if  (Num_userdets >= Max_userdets)  {
                Max_userdets += INC_USERDETS;
                if  (userdets_buf)
                        userdets_buf = (struct spdet *) realloc((char *) userdets_buf, Max_userdets * sizeof(struct spdet));
                else
                        userdets_buf = (struct spdet *) malloc(Max_userdets * sizeof(struct spdet));
                if  (!userdets_buf)
                        nomem();
                sp = &userdets_buf[whu];
        }
        for  (cnt = Num_userdets;  cnt > whu;  cnt--)
                userdets_buf[cnt] = userdets_buf[cnt-1];
        Num_userdets++;
        *sp = *item;
}

void    insertu(const struct spdet *item)
{
        if  (spu_new_format)  {
#ifndef HAVE_FTRUNCATE
                char    *fname;
#endif
                int  whu = bsearch_spdet(item->spu_user);
                struct  spdet  *sp = &userdets_buf[whu];

                if  (whu >= Num_userdets || sp->spu_user != item->spu_user)  {
                        /* We haven't met this user before.
                           If it's the same as the default, forget it. */
                        if  (issame_defs(item))
                                return;
                        insert_item_vec(item, whu);
                }
                else  {
                        *sp = *item;

                        /* If we're root or spooler user, force on all privs */

                        if  (item->spu_user == ROOTID  || (ULONG) item->spu_user == (ULONG) Daemuid)
                                sp->spu_flgs = ALLPRIVS;

                        /* If it's the same as the default, then we want to delete the user */

                        if  (issame_defs(sp))  {
                                int  cnt;
                                for  (cnt = whu+1;  cnt < Num_userdets;  cnt++)
                                        userdets_buf[cnt-1] = userdets_buf[cnt];
                                Num_userdets--;
                        }
                }

                /* And now write the thing out */

#ifdef  HAVE_FTRUNCATE
                Ignored_error = ftruncate(spuf_fid, 0L);
                lseek(spuf_fid, 0L, 0);
#else
                close(spuf_fid);
                fname = envprocess(SPUFILE);

                spuf_fid = open(fname, O_RDWR|O_TRUNC);
                free(fname);
                fcntl(spuf_fid, F_SETFD, 1);
                lockit(F_WRLCK);
#endif

                Ignored_error = write(spuf_fid, (char *) &Spuhdr, sizeof(Spuhdr));
                if  (Num_userdets != 0)
                        Ignored_error = write(spuf_fid, (char *) userdets_buf, Num_userdets * sizeof(struct spdet));
        }
        else  {                                 /* Old-style format */
                struct  spdet   c;

                /* Force all privs on for root and spooler user id - item is const so we can't fiddle with it */

                if  (item->spu_user == ROOTID  || (ULONG) item->spu_user == (ULONG) Daemuid)  {
                        c = *item;
                        c.spu_flgs = ALLPRIVS;
                        item = &c;
                }

                /* If it's below maximum for vector, stuff it in.  */

                if  ((ULONG) item->spu_user < SMAXUID)  {
                        lseek(spuf_fid, (long) (sizeof(struct sphdr) + item->spu_user * sizeof(struct spdet)), 0);
                        Ignored_error = write(spuf_fid, (char *) item, sizeof(struct spdet));
                }
                else  {
                        /* We now hold details for users >SMAXUID in the in-memory vector used for new fmt
                           We won't worry for now about items duplicating the default as that will happen
                           when the whole file is written out. */

                        int  whu = bsearch_spdet(item->spu_user);
                        struct  spdet  *sp = &userdets_buf[whu];

                        if  (whu >= Num_userdets || sp->spu_user != item->spu_user)
                                insert_item_vec(item, whu);
                        else
                                *sp = *item;

                        lseek(spuf_fid, (long) (sizeof(struct sphdr) + sizeof(struct spdet) * SMAXUID), 0);
                        Ignored_error = write(spuf_fid, (char *) userdets_buf, Num_userdets * sizeof(struct spdet));
                }
        }

        last_mod_time = time(0);
}

/* Create user control file from scratch.
   Return 0 - failure, 1 - ok */

static  int  init_file(char *fname)
{
        int             fid;
        char            *formname;
        struct  spdet   Spec;

        if  ((fid = open(fname, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)
                return  0;

#if     defined(HAVE_FCHOWN) && !defined(M88000)
        Ignored_error = fchown(fid, (uid_t) Daemuid, getegid());
#else
        Ignored_error = chown(fname, (uid_t) Daemuid, getegid());
#endif
        savesigs(1);
        if  ((formname = helpprmpt($P{Default user form type})) == (char *) 0)
                formname = stracpy("standard");

        Spuhdr.sph_lastp = 0;                   /* Set time zero to signify new format */
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
        Ignored_error = write(fid, (char *) &Spuhdr, sizeof(Spuhdr));

        /* Initialise root and Daemuid to have all privs */

        Spec.spu_isvalid = SPU_VALID;
        strncpy(Spec.spu_form, formname, MAXFORM);
        strncpy(Spec.spu_formallow, formname, ALLOWFORMSIZE);
        Spec.spu_ptr[0] = '\0';
        Spec.spu_ptrallow[0] = '\0';
        Spec.spu_minp = U_DF_MINP;
        Spec.spu_maxp = U_DF_MAXP;
        Spec.spu_defp = U_DF_DEFP;
        Spec.spu_cps = U_DF_CPS;
        Spec.spu_flgs = ALLPRIVS;
        Spec.spu_class = (classcode_t) U_DF_CLASS;
        Spec.spu_user = ROOTID;
        Ignored_error = write(fid, (char *) &Spec, sizeof(Spec));
        if  (Daemuid != ROOTID)  {
                Spec.spu_user = Daemuid;
                Ignored_error = write(fid, (char *) &Spec, sizeof(Spec));
        }

#if     defined(HAVE_FCHOWN) && !defined(M88000)
        Ignored_error = fchown(fid, Daemuid, getegid());
#else
        Ignored_error = chown(fname, Daemuid, getegid());
#endif
        close(fid);
        savesigs(0);
        free(formname);
        return  1;
}

/* Open user file and return file descriptor in spuf_fid
   Cope with new and old formats. New format has date = 0 */

static  void  open_file(int mode)
{
        char    *fname = envprocess(SPUFILE);
        struct  stat    fbuf;

        if  ((spuf_fid = open(fname, mode)) < 0)  {
                if  (errno == EACCES)  {
                        print_error($E{Check file setup});
                        exit(E_SETUP);
                }
                if  (errno == ENOENT)
                        init_file(fname);
                spuf_fid = open(fname, mode);
        }
        free(fname);

        if  (spuf_fid < 0)  {
                print_error($E{Cannot open user file});
                exit(E_SETUP);
        }

        lockit(F_RDLCK);
        fstat(spuf_fid, &fbuf);
        last_mod_time = fbuf.st_mtime;
        fcntl(spuf_fid, F_SETFD, 1);

        if  (read(spuf_fid, (char *)&Spuhdr, sizeof(Spuhdr)) != sizeof(Spuhdr))  {
                close(spuf_fid);
                spuf_fid = -1;
                print_error($E{Cannot open user file});
                exit(E_SETUP);
        }

        /* Check version number and print warning message if funny.  */

        if  (Spuhdr.sph_version != GNU_SPOOL_MAJOR_VERSION  ||  (fbuf.st_size - sizeof(struct sphdr)) % sizeof(struct spdet) != 0)  {
                disp_arg[0] = Spuhdr.sph_version;
                disp_arg[1] = GNU_SPOOL_MAJOR_VERSION;
                print_error($E{Wrong version of product});
        }

        Num_userdets = (fbuf.st_size - sizeof(struct sphdr)) / sizeof(struct spdet);

        /* We signify the new format (default + exceptions) by having password time = 0 */

        if  (Spuhdr.sph_lastp == 0)
                spu_new_format = 1;
        else  {
                struct  stat    pwbuf;
                if  (stat("/etc/passwd", &pwbuf) < 0)  {
                        close(spuf_fid);
                        spuf_fid = -1;
                        return;
                }
                if  (Spuhdr.sph_lastp > pwbuf.st_mtime)
                        print_error($E{Funny times passwd file});

                /* Number of users is reduced by the ones saved as a vector */

                Num_userdets -= SMAXUID;
                if  (Num_userdets > 0)
                        lseek(spuf_fid, (long) (sizeof(struct sphdr) + SMAXUID * sizeof(struct spdet)), 0);
                spu_new_format = 0;
        }

        if  (Num_userdets > 0)  {
                unsigned  sizeb = Num_userdets * sizeof(struct spdet);
                if  (!(userdets_buf = (struct spdet *) malloc(sizeb)))
                        nomem();
                if  (read(spuf_fid, (char *) userdets_buf, sizeb) != (int) sizeb)  {
                        print_error($E{Cannot open user file});
                        exit(E_SETUP);
                }
        }

        Max_userdets = Num_userdets;
        unlockit();
}

static  void    close_file()
{
        if  (userdets_buf)  {
                free((char *) userdets_buf);
                userdets_buf = 0;
        }
        close(spuf_fid);
        spuf_fid = -1;
}

/* Get info about specific user. */

static struct spdet *gpriv(uid_t uid)
{
        static  struct  spdet   result;
        struct  stat  ufst;

        lockit(F_RDLCK);
        fstat(spuf_fid, &ufst);

        /* If file has changed since we read stuff in, close and reopen.
           This is assumed not to happen very often.
           Probably the only thing it will happen with is the API or if
           2 or more people are editing the user file at the same time */

        if  (ufst.st_mtime != last_mod_time)  {
                close_file();                   /* Kills the lock */
                open_file(O_RDWR);              /* Probably only done for edit-type cases */
                lockit(F_RDLCK);
        }
        readu(uid, &result);
        unlockit();
        return  &result;
}

/* Routine to access privilege/mode file.
   This is now the basic routine for user programs and does not return
   if there's a problem.  */

struct spdet *getspuser(const uid_t uid)
{
        struct  spdet  *result;

        open_file(O_RDONLY);
        result = gpriv(uid);
        close_file();
        return  result;
}

/* Get entry in user file only done for utility routines and API.  */

struct spdet *getspuentry(const uid_t uid)
{
        if  (spuf_fid < 0)
                open_file(O_RDWR);
        return  gpriv(uid);
}

/* Update details for given user only.  */

void  putspuentry(struct spdet *item)
{
        lockit(F_WRLCK);
        insertu(item);
        unlockit();
}

/* This routine is used by getspulist via uloop_over */

static void gu(char *arg, int_ugid_t uid)
{
        struct  spdet  **rp = (struct spdet **) arg;
        readu(uid, *rp);
        ++*rp;                                  /* pointer to rbuf - advance to next item */
}

static  int  sort_id(struct spdet *a, struct spdet *b)
{
        return  (ULONG) a->spu_user < (ULONG) b->spu_user ? -1: (ULONG) a->spu_user == (ULONG) b->spu_user? 0: 1;
}

struct spdet *getspulist()
{
        struct  spdet   *result, *rbuf;

        /* If we haven't got list of users yet, better get it */

        if  (Npwusers == 0)
                rpwfile();

        if  (spuf_fid < 0)  {
                open_file(O_RDWR);
                lockit(F_RDLCK);
        }
        else  {
                /* Check it hasn't changed */
                struct  stat  ufst;
                lockit(F_RDLCK);
                fstat(spuf_fid, &ufst);
                if  (ufst.st_mtime != last_mod_time)  {
                        close_file();
                        open_file(O_RDWR);
                        lockit(F_RDLCK);
                }
        }

        result = (struct spdet *) malloc(Npwusers * sizeof(struct spdet));
        if  (!result)
                nomem();
        rbuf = result;
        uloop_over(gu, (char *) &rbuf);
        unlockit();
        qsort(QSORTP1 result, Npwusers, sizeof(struct spdet), QSORTP4 sort_id);
        return  result;
}

void  putspuhdr()
{
        lockit(F_WRLCK);
        lseek(spuf_fid, 0L, 0);
        Ignored_error = write(spuf_fid, (char *) &Spuhdr, sizeof(Spuhdr));
        unlockit();
}

/* Save list. This always rewrites the header and
   probably is the last thing to be called before we quit */

void  putspulist(struct spdet *list)
{
        struct  spdet  *sp, *ep;
#ifndef HAVE_FTRUNCATE
        char    *fname;
#endif

        lockit(F_WRLCK);
#ifdef  HAVE_FTRUNCATE
        Ignored_error = ftruncate(spuf_fid, 0L);
        lseek(spuf_fid, 0L, 0);
#else
        close(spuf_fid);
        fname = envprocess(SPUFILE);
        spuf_fid = open(fname, O_RDWR|O_TRUNC);
        free(fname);
        fcntl(spuf_fid, F_SETFD, 1);
        lockit(F_WRLCK);
#endif
        Spuhdr.sph_lastp = 0;                   /* Force new format */
        Ignored_error = write(spuf_fid, (char *) &Spuhdr, sizeof(Spuhdr));

        ep = &list[Npwusers];
        for  (sp = list;  sp < ep;  sp++)  {
                if (sp->spu_user == ROOTID  || (ULONG) sp->spu_user == (ULONG) Daemuid)
                        sp->spu_flgs = ALLPRIVS;
                if  (!issame_defs(sp))
                        Ignored_error = write(spuf_fid, (char *) sp, sizeof(struct spdet));
        }
        unlockit();
}
