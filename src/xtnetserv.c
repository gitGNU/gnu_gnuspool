/* xtnetserv.c -- server process for remote submission, MS windos clients and API

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
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_sig.h"
#include <errno.h>
#ifdef  HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "spuser.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "xfershm.h"
#include "ecodes.h"
#include "client_if.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xtapi_int.h"
#include "xtnet_ext.h"
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "services.h"

SHORT   qsock,                  /* TCP Socket for accepting queued jobs on */
        uasock,                 /* Datagram socket for user access enquiries */
        apirsock;               /* API Request socket */

SHORT   qportnum,               /* Port number for TCP network byte order */
        uaportnum,              /* Port number for UDP network byte order */
        apirport,               /* Port number for API requests */
        apipport;               /* UDP port number for prompt messages to API */

const   char    Sname[] = GSNETSERV_PORT,
                ASrname[] = API_DEFAULT_SERVICE,
                ASmname[] = API_MON_SERVICE;

int     hadrfresh;

#define IPC_MODE        0600

#ifndef USING_FLOCK
/* These don't have SEM_UNDO in any more */

struct  sembuf  jr[2] = {       { JQ_FIDDLE,    0,      0 },
                                { JQ_READING,   1,      0 }},
                ju[1] = {       { JQ_READING,   -1,     0 }},
                pr[2] = {       { PQ_FIDDLE,    0,      0 },
                                { PQ_READING,   1,      0 }},
                pu[1] = {       { PQ_READING,   -1,     0 }};
#endif

struct  spr_req sp_req;
struct  spq     SPQ;

#define JN_INC  80000           /*  Add this to job no if clashes */
#define JOB_MOD 60000           /*  Modulus of job numbers */

struct  pages   pfe = { 1, 1, 0 };
char    *spdir;
char    tmpfl[NAMESIZE + 1], pgfl[NAMESIZE + 1];

int_ugid_t      Defaultuid;
char            *Defaultuser;

struct  hhash   *nhashtab[NETHASHMOD];
struct  winuhash *winuhashtab[NETHASHMOD];
struct  alhash  *alhashtab[NETHASHMOD];

extern  char    dosuser[];

struct  pend_job  pend_list[MAX_PEND_JOBS];/* List of pending UDP jobs */

unsigned timeouts = NETTICKLE;

extern unsigned  calcnhash(const netid_t);

void  nomem()
{
        fprintf(stderr, "Run out of memory\n");
        exit(E_NOMEM);
}

/* Hash function for windows names limited to WUIDSIZE chars */

unsigned  calc_winu_hash(const char *name)
{
        unsigned  sum = 0;
        int     cnt = WUIDSIZE;
        while  (*name  &&  cnt > 0)  {
                sum += tolower(*name);
                name++;
                cnt--;
        }
        return  sum % NETHASHMOD;
}

/* Look up windows user name in hash table */

struct  winuhash  *lookup_winu(const char *wuname)
{
        struct  winuhash  *wp;

        for  (wp = winuhashtab[calc_winu_hash(wuname)];  wp;  wp = wp->next)
                if  (ncstrncmp(wuname, wp->winname, WUIDSIZE) == 0)
                        return  wp;
        return  (struct winuhash *) 0;
}

/* Allocate a new windows user name structure and return it */

struct  winuhash  *add_winu(const char *wuname)
{
        unsigned  hashv = calc_winu_hash(wuname);
        struct  winuhash  *wp = (struct winuhash *) malloc(sizeof(struct winuhash));

        if  (!wp)
                nomem();

        wp->next = winuhashtab[hashv];
        winuhashtab[hashv] = wp;
        return  wp;
}

/* Look up a windows name and if it isn't there take it as a UNIX name and try that.
   That's to save lots of fred=fred type entries in the user map file */

struct  winuhash *lookup_winoruu(const char *wuname)
{
        struct  winuhash *wp;
        int_ugid_t  uid;

        if  ((wp = lookup_winu(wuname)))
                return  wp;

        uid = lookup_uname(wuname);
        if  (uid == UNKNOWN_UID)
                return  (struct winuhash *) 0;

        wp = add_winu(wuname);
        wp->uuid = uid;
        wp->unixname = stracpy(wuname);
        wp->winname = stracpy(wuname);
        return  wp;
}

/* Parse the Windows user name file */

void    parse_winu_file()
{
        FILE    *fp = fopen(WINUSER_MAP, "r");
        char    lbuf[200];

        if  (!fp)
                return;

        while  (fgets(lbuf, sizeof(lbuf), fp))  {
                struct  winuhash  *wp;
                char  *sp = &lbuf[strlen(lbuf)-1], *cp;
                int_ugid_t  uid;

                /* Zap trailing spaces at the end */

                while  (sp >= &lbuf[0] &&  isspace(*sp))
                        sp--;
                *++sp = '\0';   /* Steo back to first space */

                /* Skip over leading spaces at beginning */

                sp = &lbuf[0];
                while  (isspace(*sp))
                        sp++;

                /* Forget lines starting with a # or which don't have a colon in */

                if  (*sp == '#')
                        continue;
                if  (!(cp = strchr(sp, ':')))
                        continue;

                /* Zap the colon, cp points to windows name, sp to UNIX name.
                   Forget ones with unknown UNIX name */

                *cp++ = '\0';
                uid = lookup_uname(sp);
                if  (uid == UNKNOWN_UID)
                        continue;

                /* If we saw Windows name before, just update details */

                if  (!(wp = lookup_winu(cp)))
                        wp = add_winu(cp);
                wp->winname = stracpy(cp);
                wp->unixname = stracpy(sp);
                wp->uuid = uid;
        }

        fclose(fp);
}

/* Clear hash table for Windows users on SIGHUP */

void    clear_winuhash()
{
        unsigned  hashv;

        for  (hashv = 0;  hashv < NETHASHMOD;  hashv++)  {
                struct  winuhash  *wp = winuhashtab[hashv];
                while  (wp)  {
                        struct  winuhash  *nxt = wp->next;
                        free(wp->winname);
                        free(wp->unixname);
                        free((char *) wp);
                        wp = nxt;
                }
                winuhashtab[hashv] = (struct winuhash *) 0;
        }

        /* Forget auto login stuff whilst we are there */
        for  (hashv = 0;  hashv < NETHASHMOD;  hashv++)  {
                struct  alhash  *al = alhashtab[hashv];
                while  (al)  {
                        struct  alhash  *nxt = al->next;
                        free(al->unixname);
                        free((char *) al);
                        al = nxt;
                }
                alhashtab[hashv] = (struct alhash *) 0;
        }
}

struct  alhash  *find_autoconn(const netid_t h)
{
        struct  alhash  *al;

        for  (al = alhashtab[calcnhash(h)];  al;  al = al->next)
                if  (al->hostid == h)
                        return  al;
        return  (struct alhash *)  0;
}

struct  alhash  *add_autoconn(const netid_t h, const char *uname, const int_ugid_t uid)
{
        struct  alhash  *al = find_autoconn(h);

        if  (al)
                free(al->unixname);
        else  {
                unsigned  hashv = calcnhash(h);
                al = (struct alhash *) malloc(sizeof(struct alhash));
                if  (!al)
                        nomem();
                al->next = alhashtab[hashv];
                alhashtab[hashv] = al;
                al->hostid = h;
        }
        al->unixname = stracpy(uname);
        al->uuid = uid;
        return  al;
}

struct  hhash  *lookup_hhash(const netid_t h)
{
        struct  hhash  *hp;

        for  (hp = nhashtab[calcnhash(h)];  hp;  hp = hp->hn_next)
                if  (hp->hostid == h)
                        return  hp;
        return  (struct hhash *)  0;
}

struct  hhash  *add_hhash(const netid_t h)
{
        struct  hhash  *hp = (struct hhash *) malloc(sizeof(struct hhash));
        unsigned  hashv = calcnhash(h);

        if  (!hp)
                nomem();
        hp->hn_next = nhashtab[hashv];
        nhashtab[hashv] = hp;
        hp->hostid = h;
        hp->isme = 0;
        hp->isclient = 0;
        return  hp;
}

void    forget_hosts()
{
        unsigned  hashv;

        for  (hashv = 0;  hashv < NETHASHMOD;  hashv++)  {
                struct  hhash  *hp, *nxt;
                hp = nhashtab[hashv];
                while  (hp)  {
                        nxt = hp->hn_next;
                        free((char *) hp);
                        hp = nxt;
                }
                nhashtab[hashv] = (struct hhash *) 0;
        }
}

/* Add IP address representing "me" to table for benefit of APIs on local host */

static void  addme(const netid_t mid)
{
        struct  hhash  *hp = add_hhash(mid);
        hp->isme = 1;
}

/* Read in hosts file and build up interesting stuff */

void  process_hfile()
{
        struct  remote  *rp;
        struct  hhash   *hp;
        extern  char    hostf_errors;

        hostf_errors = 0;

        while  ((rp = get_hostfile()))  {

                /* If it is telling us about a possible client user, note the details as with the user map
                   file, however this overrides whatever was in there and possibly provides for a machine
                   at which a user doesn't need to supply a password */

                if  (rp->ht_flags & HT_ROAMUSER)  {
                        struct  winuhash  *wp;
                        int_ugid_t  uid = lookup_uname(rp->hostname);

                        if  (uid == UNKNOWN_UID)
                                continue;

                        if  (!(wp = lookup_winu(rp->alias)))
                                wp = add_winu(rp->alias);

                        wp->uuid = uid;
                        wp->winname = stracpy(rp->alias);
                        wp->unixname = stracpy(rp->hostname);

                        /* Don't worry about usual machines if we are checking anyhow or no machine given */

                        if  (!(rp->ht_flags & HT_PWCHECK)  &&  dosuser[0])  {
                                netid_t  defhost = look_hostname(dosuser);
                                if  (defhost)
                                        add_autoconn(defhost, rp->hostname, uid);
                        }
                        continue;
                }

                /* It might be telling us about a client with a fixed IP address. */

                if  (rp->ht_flags & HT_DOS)  {
                        int_ugid_t  uid;
                        hp = lookup_hhash(rp->hostid);
                        if  (!hp)
                                hp = add_hhash(rp->hostid);
                        hp->isclient = 1;
                        if  (!(rp->ht_flags & HT_PWCHECK)  &&  dosuser[0]  &&  (uid = lookup_uname(dosuser)) != UNKNOWN_UID)
                                add_autoconn(rp->hostid, dosuser, uid);
                        continue;
                }

                /* We now don't bother with "trusted" */

                hp = lookup_hhash(rp->hostid);
                if  (!hp)
                        hp = add_hhash(rp->hostid);
        }

        end_hostfile();

        /* Create entries for "me" to allow for API connections from local hosts */

        addme(myhostid);
        addme(htonl(INADDR_LOOPBACK));

        /* This may be a good place to warn people about errors in the
           host file.  */

        if  (hostf_errors)
                print_error($E{Warn errors in host file});
}

void    read_hfiles()
{
        struct  winuhash  *wp;
        int_ugid_t  uid;

        parse_winu_file();
        process_hfile();
        if  (!(wp = lookup_winu("default"))  || (uid = lookup_uname(wp->unixname)) == UNKNOWN_UID)  {
                Defaultuid = Daemuid;
                Defaultuser = SPUNAME;
        }
        else  {
                Defaultuser = wp->unixname;
                Defaultuid = wp->uuid;
        }
}

/* Catch hangup signals and re-read hosts file */

static  RETSIGTYPE  catchhup(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(n, SIG_IGN);
#endif
        /* Forget what we learned before and re-read it */

        clear_winuhash();
        forget_hosts();
        read_hfiles();

#ifdef  UNSAFE_SIGNALS
        signal(n, catchhup);
#endif
}

static  char    sigstocatch[] = { SIGINT, SIGQUIT, SIGTERM };

/* On a signal, remove file (TCP connection) */

RETSIGTYPE  catchdel(int n)
{
        unlink(tmpfl);
        unlink(pgfl);
        exit(E_SIGNAL);
}

/* Main path - remove files pending for UDP */

RETSIGTYPE  catchabort(int n)
{
        int     cnt;
#ifdef  UNSAFE_SIGNALS
        signal(n, SIG_IGN);
#endif
        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                abort_job(&pend_list[cnt]);
        exit(E_SIGNAL);
}

/* This notes signals from (presumably) the scheduler. This is only used by the API. */

RETSIGTYPE      sh_markit(int sig)
{
#ifdef  UNSAFE_SIGNALS
        signal(sig, sh_markit);
#endif
        hadrfresh++;
}

void    catchsigs(RETSIGTYPE (*catchfn)(int))
{
        int     i;

#ifdef  STRUCT_SIG
        struct  sigstruct_name  zc, oldsig;
        zc.sighandler_el = catchfn;
        sigmask_clear(zc);
        zc.sigflags_el = SIGVEC_INTFLAG;
        for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
                sigact_routine(sigstocatch[i], &zc, &oldsig);
                if  (oldsig.sighandler_el == SIG_IGN)
                        sigact_routine(sigstocatch[i], &oldsig, (struct sigstruct_name *) 0);
        }
#else
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                if  (signal(sigstocatch[i], catchfn) == SIG_IGN)
                        signal(sigstocatch[i], SIG_IGN);
#endif
}

void  openrfile()
{
        int     ret;

        /* If message queue does not exist, then the spooler isn't
           running. I don't think that we want to randomly start it.  */

        if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
                print_error($E{Spooler not running});
                exit(E_NOTRUN);
        }

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
                print_error($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif

        if  ((ret = init_xfershm(1)))  {
                print_error(ret);
                exit(E_SETUP);
        }
#ifndef USING_FLOCK
        set_xfer_server();      /* Don't want SEM_UNDO */
#endif

        if  (!jobshminit(1))  {
                print_error($E{Cannot open jshm});
                exit(E_JOBQ);
        }
        if  (!ptrshminit(1))  {
                print_error($E{Cannot open pshm});
                exit(E_PRINQ);
        }
}

/* "Log myself in" with spshed. This is only used by the API  */

void  lognprocess()
{
        struct  spr_req nmsg;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
        z.sighandler_el = sh_markit;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(QRFRESH, &z, (struct sigstruct_name *) 0);
#else
        /* signal is #defined as sigset on suitable systems */
        signal(QRFRESH, sh_markit);
#endif
        nmsg.spr_mtype = MT_SCHED;
        nmsg.spr_un.n.spr_act = SON_XTNATT;
        nmsg.spr_un.n.spr_seq = 0;
        nmsg.spr_un.n.spr_pid = getpid();
        BLOCK_ZERO(&nmsg.spr_un.n.spr_n, sizeof(nmsg.spr_un.n.spr_n));
        msgsnd(Ctrl_chan, (struct msgbuf *) &nmsg, sizeof(struct sp_nmsg), 0); /* Wait until it goes */
}

/* Unpack a job and see what British Hairyways has broken this time */

void  unpack_job(struct spq *to, struct spq *from)
{
        to->spq_job = ntohl((ULONG) from->spq_job);
        to->spq_netid = 0L;
        to->spq_orighost = 0L;  /* For now */
        to->spq_rslot = 0L;
        to->spq_time = ntohl((ULONG) from->spq_time);
        to->spq_proptime = 0L;
        to->spq_starttime = ntohl((ULONG) from->spq_starttime);
        to->spq_hold = ntohl((ULONG) from->spq_hold);
        to->spq_nptimeout = ntohs(from->spq_nptimeout);
        to->spq_ptimeout = ntohs(from->spq_ptimeout);
        to->spq_size = ntohl((ULONG) from->spq_size);
        to->spq_posn = ntohl((ULONG) from->spq_posn);
        to->spq_pagec = ntohl((ULONG) from->spq_pagec);
        to->spq_npages = ntohl((ULONG) from->spq_npages);

        to->spq_cps = from->spq_cps;
        to->spq_pri = from->spq_pri;
        to->spq_wpri = ntohs((USHORT) from->spq_wpri);

        to->spq_jflags = ntohs(from->spq_jflags);
        to->spq_sflags = from->spq_sflags;
        to->spq_dflags = from->spq_dflags;

        to->spq_extrn = ntohs((USHORT) from->spq_extrn);
        to->spq_pglim = ntohs((USHORT) from->spq_pglim);

        to->spq_class = ntohl(from->spq_class);

        to->spq_pslot = ntohl(-1L);

        to->spq_start = ntohl((ULONG) from->spq_start);
        to->spq_end = ntohl((ULONG) from->spq_end);
        to->spq_haltat = ntohl((ULONG) from->spq_haltat);

        to->spq_uid = ntohl(from->spq_uid);

        strncpy(to->spq_uname, from->spq_uname, UIDSIZE+1); /* May change later */
        strncpy(to->spq_puname, from->spq_puname, UIDSIZE+1);
        strncpy(to->spq_file, from->spq_file, MAXTITLE+1);
        strncpy(to->spq_form, from->spq_form, MAXFORM+1);
        strncpy(to->spq_ptr, from->spq_ptr, JPTRNAMESIZE+1);
        strncpy(to->spq_flags, from->spq_flags, MAXFLAGS+1);
}

int  tcp_serv_open(SHORT portnum)
{
        int     result;
        struct  sockaddr_in     sin;
#ifdef  SO_REUSEADDR
        int     on = 1;
#endif
        sin.sin_family = AF_INET;
        sin.sin_port = portnum;
        BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
        sin.sin_addr.s_addr = INADDR_ANY;

        if  ((result = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                return  -1;
#ifdef  SO_REUSEADDR
        setsockopt(result, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
        if  (bind(result, (struct sockaddr *) &sin, sizeof(sin)) < 0  ||  listen(result, 5) < 0)  {
                close(result);
                return  -1;
        }
        return  result;
}

/* Accept TCP connection and pass back the author in the second argument.
   Translate "my" hostid to be zero. */

int  tcp_serv_accept(const int msock, netid_t *whofrom)
{
        int     sock;
        SOCKLEN_T       sinl;
        struct  sockaddr_in  sin;

        sinl = sizeof(sin);
        if  ((sock = accept(msock, (struct sockaddr *) &sin, &sinl)) < 0)
                return  -1;
        *whofrom = sockaddr2int_netid_t(&sin);
        return  sock;
}

int  udp_serv_open(SHORT portnum)
{
        int     result;
        struct  sockaddr_in     sin;
        sin.sin_family = AF_INET;
        sin.sin_port = portnum;
        BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
        sin.sin_addr.s_addr = INADDR_ANY;

        /* Open Datagram socket for user access stuff */

        if  ((result = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                return  -1;

        if  (bind(result, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
                close(result);
                return  -1;
        }
        return  result;
}

/* Set up network stuff */

int  init_network()
{
        struct  servent *sp;

        if  (!(sp = getservbyname(Sname, "tcp")))  {
                disp_str = (char *) Sname;
                disp_str2 = "tcp";
                print_error($E{xtnet unknown qserv});
                return  0;
        }

        /* Shhhhhh....  I know this should be network byte order, but
           lets leave it alone for now.  */

        qportnum = sp->s_port;
        if  (!(sp = getservbyname(Sname, "udp")))  {
                disp_str = (char *) Sname;
                disp_str2 = "udp";
                print_error($E{xtnet unknown uaserv});
                return  0;
        }

        uaportnum = sp->s_port;

        if  (!(sp = getservbyname(ASrname, "tcp")))  {
                disp_str = (char *) ASrname;
                disp_str2 = "tcp";
                print_error($E{xtnet unknown apiserv});
                return  0;
        }

        apirport = sp->s_port;
        if  (!(sp = getservbyname(ASmname, "udp")))  {
                disp_str = (char *) ASmname;
                disp_str2 = "udp";
                print_error($E{xtnet unknown apipserv});
                return  0;
        }

        apipport = sp->s_port;
        endservent();

        if  ((qsock = tcp_serv_open(qportnum)) < 0)  {
                disp_arg[0] = ntohs(qportnum);
                print_error($E{xtnet cannot open qsock});
                return  0;
        }

        if  ((uasock = udp_serv_open(uaportnum)) < 0)  {
                disp_arg[0] = ntohs(uaportnum);
                print_error($E{xtnet cannot open uasock});
                return  0;
        }

        if  ((apirsock = tcp_serv_open(apirport)) < 0)  {
                disp_arg[0] = ntohs(apirport);
                print_error($E{xtnet cannot open apisock});
                return  0;
        }

        return  1;
}

/* Generate output file name */

FILE *goutfile(jobno_t *jnp, char *tmpfl, char *pgfl, const int wantreread)
{
        FILE    *res;
        int     fid;

        for  (;;)  {
                strcpy(tmpfl, mkspid(SPNAM, *jnp));
                if  ((fid = open(tmpfl, wantreread? O_RDWR|O_CREAT|O_EXCL: O_WRONLY|O_CREAT|O_EXCL, 0400)) >= 0)
                        break;
                *jnp += JN_INC;
        }
#ifdef  RUN_AS_ROOT
#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                fchown(fid, Daemuid, getegid());
#else
        if  (Daemuid != ROOTID)
                chown(tmpfl, Daemuid, getegid());
#endif
#endif
        if  (!wantreread)
                catchsigs(catchdel);

        if  ((res = fdopen(fid, wantreread? "w+": "w")) == (FILE *) 0)  {
                unlink(tmpfl);
                nomem();
        }

        /* Generate name now, worry later */

        strcpy(pgfl, mkspid(PFNAM, *jnp));
        return  res;
}

static  int  sock_read(const int sock, char *buffer, unsigned nbytes)
{
        while  (nbytes != 0)  {
                int     rbytes = read(sock, buffer, nbytes);
                if  (rbytes <= 0)
                        return  0;
                buffer += rbytes;
                nbytes -= rbytes;
        }
        return  1;
}

/* Copy job data to output file and count pages.
   Version for rspr where we believe the job size etc so we can do it as we go along.
   Return 0 (XTNQ_OK) if OK,
   -1 if network error, otherwise queueing error.  */

int  copyout(int sock, FILE *outf, char *delim)
{
        char    *rcp;
        int     ch, rec_cnt, pgfid = -1, inbytes = 0, incount = 0, retcode = XTNQ_OK;
        unsigned        sequence = 1;
        LONG    onpage, char_count = 0;
        LONG    plim = 0x7fffffffL;
        ULONG   klim = 0xffffffffL;
        char    *rcdend;
        struct  tcp_data  inb;

        if  (SPQ.spq_pglim)  {
                if  (SPQ.spq_dflags & SPQ_PGLIMIT)
                        plim = SPQ.spq_pglim;
                else
                        klim = (ULONG) SPQ.spq_pglim << 10;
        }

        SPQ.spq_npages = 0;
        onpage = 0;

        if  (!delim  ||  (pfe.deliml == 1  &&  pfe.delimnum == 1 &&  delim[0] == '\f'))  {
                SPQ.spq_dflags &= SPQ_ERRLIMIT | SPQ_PGLIMIT; /* I didnt mean a ~ here */

                while  (char_count < SPQ.spq_size)  {
                        if  (inbytes <= incount)  {
                                if  (!sock_read(sock, (char *) &inb, sizeof(inb)))
                                        goto  ioerror;
                                if  (inb.tcp_code != TCP_DATA)
                                        goto  lost_sync;
                                if  (inb.tcp_seq != (sequence & 0xff))
                                        goto  lost_seq;
                                sequence++;
                                inbytes = ntohs(inb.tcp_size);
                                incount = 0;
                        }
                        ch = inb.tcp_buff[incount++];
                        if  ((ULONG) ++char_count > klim)  {
                                SPQ.spq_size = --char_count;
                                retcode = XTNR_WARN_LIMIT;
                                break;
                        }
                        onpage++;
                        if  (putc(ch, outf) == EOF)
                                goto  ffull;
                        if  (ch == '\f')  {
                                onpage = 0;
                                if  (++SPQ.spq_npages > plim)  {
                                        SPQ.spq_size = char_count;
                                        retcode = XTNR_WARN_LIMIT;
                                        break;
                                }
                        }
                }

                if  (retcode == XTNR_WARN_LIMIT)  {
                        for  (;;)  {
                                if  (!sock_read(sock, (char *) &inb, sizeof(inb)))
                                        goto  ioerror;
                                if  (inb.tcp_code == TCP_DATA)
                                        continue;
                                if  (inb.tcp_code == TCP_ENDJOB)
                                        break;
                                goto  lost_sync;
                        }
                        if  (SPQ.spq_dflags & SPQ_ERRLIMIT)
                                goto  toobig;
                }
                else  {
                        if  (char_count != SPQ.spq_size)
                                goto  lost_seq;
                        if  (!sock_read(sock, (char *) &inb, sizeof(inb)))
                                goto  ioerror;
                        if  (inb.tcp_code != TCP_ENDJOB)
                                goto  lost_sync;
                }

                if  (char_count == 0)  {
                        fclose(outf);
                        unlink(tmpfl);
                        return  XTNR_EMPTYFILE;
                }
                if  (onpage)
                        SPQ.spq_npages++;
                if  (fclose(outf) != EOF)
                        return  retcode;
                goto  ffull;
        }

        SPQ.spq_dflags |= SPQ_PAGEFILE;

        if  ((pgfid = open(pgfl, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0)  {
                fclose(outf);
                unlink(tmpfl);
                return  XTNR_CC_PAGEFILE;
        }
#ifdef  RUN_AS_ROOT
#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                fchown(pgfid, Daemuid, getegid());
#else
        if  (Daemuid != ROOTID)
                chown(pgfl, Daemuid, getegid());
#endif
#endif
        pfe.lastpage = 0;       /* Fix this later perhaps */
        Ignored_error = write(pgfid, (char *) &pfe, sizeof(pfe));
        Ignored_error = write(pgfid, delim, (unsigned) pfe.deliml);

        rcp = delim;
        rcdend = delim + pfe.deliml;
        onpage = 0;
        rec_cnt = 0;

        while  (char_count < SPQ.spq_size)  {
                if  (inbytes <= incount)  {
                        if  (!sock_read(sock, (char *) &inb, sizeof(inb)))
                                goto  ioerror;
                        incount = 0;
                        if  (inb.tcp_code != TCP_DATA)
                                goto  lost_sync;
                        if  (inb.tcp_seq != (sequence & 0xff))
                                goto  lost_seq;
                        sequence++;
                        inbytes = ntohs(inb.tcp_size);
                        incount = 0;
                }
                ch = inb.tcp_buff[incount++];
                if  ((ULONG) ++char_count > klim)  {
                        SPQ.spq_size = --char_count;
                        retcode = XTNR_WARN_LIMIT;
                        break;
                }
                onpage++;
                if  (ch == *rcp)  {
                        if  (++rcp >= rcdend)  {
                                if  (++rec_cnt >= pfe.delimnum)  {
                                        if  (write(pgfid, (char *) &char_count, sizeof(LONG)) != sizeof(LONG))
                                                goto  ffull;
                                        onpage = 0;
                                        rec_cnt = 0;
                                        if  (++SPQ.spq_npages > plim)  {
                                                SPQ.spq_size = char_count;
                                                retcode = XTNR_WARN_LIMIT;
                                                break;
                                        }
                                }
                                rcp = delim;
                        }
                }
                else  if  (rcp > delim)  {
                        char    *pp, *prcp, *prevpl;
                        prevpl = --rcp; /*  Last one matched  */
                        for  (;  rcp > delim;  rcp--)  {
                                if  (*rcp != ch)
                                        continue;
                                pp = prevpl;
                                prcp = rcp - 1;
                                for  (;  prcp >= delim;  pp--, prcp--)
                                        if  (*pp != *prcp)
                                                goto  rej;
                                rcp++;
                                break;
                        rej:    ;
                        }
                }
                if  (putc(ch, outf) == EOF)
                        goto  ffull;
        }

        if  (retcode == XTNR_WARN_LIMIT)  {
                for  (;;)  {
                        if  (!sock_read(sock, (char *) &inb, sizeof(inb)))
                                goto  ioerror;
                        if  (inb.tcp_code == TCP_DATA)
                                continue;
                        if  (inb.tcp_code == TCP_ENDJOB)
                                break;
                        goto  lost_sync;
                }
                if  (SPQ.spq_dflags & SPQ_ERRLIMIT)
                        goto  toobig;
        }
        else  {
                if  (char_count != SPQ.spq_size)
                        goto  lost_seq;
                if  (!sock_read(sock, (char *) &inb, sizeof(inb)))
                        goto  ioerror;
                if  (inb.tcp_code != TCP_ENDJOB)
                        goto  lost_sync;
        }

        if  (char_count == 0)  {
                fclose(outf);
                close(pgfid);
                unlink(tmpfl);
                unlink(pgfl);
                return  XTNR_EMPTYFILE;
        }

        /* Store the offset of the end of the file */

        if  (write(pgfid, (char *) &SPQ.spq_size, sizeof(LONG)) != sizeof(LONG))
                goto  ffull;

        /* Remember how big the last page was */

        if  (onpage > 0)  {
                SPQ.spq_npages++;
                if  ((pfe.lastpage = pfe.delimnum - rec_cnt) > 0)  {
                        lseek(pgfid, 0L, 0);
                        Ignored_error = write(pgfid, (char *) &pfe, sizeof(pfe));
                }
        }

        if  (close(pgfid) < 0)
                goto  ffull;
        if  (fclose(outf) != EOF)
                return  retcode;

 ffull:
        unlink(tmpfl);
        unlink(pgfl);
        return  XTNR_FILE_FULL;
 toobig:
        fclose(outf);
        unlink(tmpfl);
        if  (pgfid >= 0)  {
                close(pgfid);
                unlink(pgfl);
        }
        return  XTNR_PAST_LIMIT;
 lost_sync:
        unlink(tmpfl);
        unlink(pgfl);
        return  XTNR_LOST_SYNC;
 lost_seq:
        unlink(tmpfl);
        unlink(pgfl);
        return  XTNR_LOST_SEQ;
 ioerror:
        unlink(tmpfl);
        unlink(pgfl);
        return  -1;
}

void  q_reply(const int sock, const int flag, const jobno_t code)
{
        struct  client_if       result;

        result.resvd[0] = result.resvd[1] = result.resvd[2] = '\0';
        result.flag = (unsigned char) flag;
        result.jobnum = htonl((ULONG) code);
        Ignored_error = write(sock, (char *) &result, sizeof(result));
}

int  validate_job(struct spq * jp)
{
        uid_t   Realuid = jp->spq_uid;
        struct  spdet   *spuser;

        /* Find out the privileges of the specified user.
           If we don't know the posting user, quietly zap it.  */

        spuser = getspuentry(Realuid);
        if  (lookup_uname(jp->spq_puname) == UNKNOWN_UID)
                strcpy(jp->spq_puname, jp->spq_uname);

        /* Validate class code of job, priority, copies */

        if  (!(spuser->spu_flgs & PV_COVER))
                jp->spq_class &= spuser->spu_class;
        if  (jp->spq_class == 0)
                return  XTNR_ZERO_CLASS;
        if  (jp->spq_pri < spuser->spu_minp  ||  jp->spq_pri > spuser->spu_maxp)
                return  XTNR_BAD_PRIORITY;
        if  (!(spuser->spu_flgs & PV_ANYPRIO) && jp->spq_cps > spuser->spu_cps)
                return  XTNR_BAD_COPIES;

        /* Validate form type, if none given plug in standard form.
           Likewise printer.  */

        if  (jp->spq_form[0] == '\0')  {
                strncpy(jp->spq_form, spuser->spu_form, MAXFORM);
                jp->spq_form[MAXFORM] = '\0';
        }
        else  if  (!((spuser->spu_flgs & PV_FORMS) || qmatch(spuser->spu_formallow, jp->spq_form)))
                return  XTNR_BAD_FORM;

        if  (jp->spq_ptr[0] == '\0')  {
                strncpy(jp->spq_ptr, spuser->spu_ptr, JPTRNAMESIZE);
                jp->spq_ptr[JPTRNAMESIZE] = '\0';
        }
        else  if  (!((spuser->spu_flgs & PV_OTHERP) || issubset(spuser->spu_ptrallow, jp->spq_ptr)))
                return  XTNR_BAD_PTR;

        /* Quietly fix anything we don't want to fuss about */

        if  (jp->spq_nptimeout == 0)
                jp->spq_nptimeout = QNPTIMEOUT;
        if  (jp->spq_ptimeout == 0)
                jp->spq_ptimeout = QPTIMEOUT;
        jp->spq_haltat = 0L;
        if  (jp->spq_start > jp->spq_end)  {
                jp->spq_start = 0L;
                jp->spq_end = 0x7ffffffeL;
        }
        if  (jp->spq_hold != 0  &&  (time_t) jp->spq_hold < time((time_t *) 0))
                jp->spq_hold = 0;
        return  0;
}

/* In case where we have a UNIX user name passed to us, convert user name
   appropriately */

void    convert_unix_username(struct spq *jp)
{
        int_ugid_t  realu = lookup_uname(jp->spq_uname);
        int  diffpost = strcmp(jp->spq_puname, jp->spq_uname) != 0;     /* Post user different */

        if  (realu == UNKNOWN_UID)  {
                strcpy(jp->spq_uname, Defaultuser);
                jp->spq_uid = Defaultuid;
        }
        else
                jp->spq_uid = realu;

        /* If we don't know the posting user just copy in the now valid user name in */

        if  (diffpost  &&  lookup_uname(jp->spq_puname) == UNKNOWN_UID)
                strcpy(jp->spq_puname, jp->spq_uname);
}

/* Code executed by child process to continue queueing job.  */

static  int  qrest(const int sock, const netid_t netid)
{
        int     tries, ret;
        struct  hhash   *frp;
        char    *delim = (char *) 0;
        FILE    *outf;
        jobno_t jn;
        int     isclient;

        isclient = (frp = lookup_hhash(netid)) && frp->isclient;
        convert_unix_username(&SPQ);

        if  ((ret = validate_job(&SPQ)) != 0)  {
                q_reply(sock, ret, (jobno_t) 0);
                return  0;
        }

        /* See if the job has a page delimiter.  */

        if  (SPQ.spq_dflags & SPQ_PAGEFILE)  {
                struct  tcp_data  inb;
                struct  pages   *inpg = (struct pages *) inb.tcp_buff;

                if  (!sock_read(sock, (char *) &inb, sizeof(inb)))  {
                        q_reply(sock, XTNR_BAD_PF, (jobno_t) 0);
                        return  0;
                }
                if  (inb.tcp_code != TCP_PAGESPEC)  {
                        q_reply(sock, XTNR_LOST_SYNC, (jobno_t) 0);
                        return  0;
                }

                pfe.delimnum = ntohl((ULONG) inpg->delimnum);
                pfe.deliml = ntohl((ULONG) inpg->deliml);
                pfe.lastpage = ntohl((ULONG) inpg->lastpage);

                if  (pfe.deliml <= 0)  {
                        q_reply(sock, XTNR_BAD_PF, (jobno_t) 0);
                        return  0;
                }
                if  (!(delim = malloc((unsigned) pfe.deliml)))  {
                        q_reply(sock, XTNR_NOMEM_PF, (jobno_t) 0);
                        return  0;
                }
                if  (!sock_read(sock, delim, (unsigned) pfe.deliml))  {
                        q_reply(sock, XTNR_BAD_PF, (jobno_t) 0);
                        return  0;
                }
        }

        SPQ.spq_time = time((time_t *) 0);
        SPQ.spq_orighost = netid;

        /* On DOS machines make up a job number from the current time
           mashed up with the host id.
           On Unix machines Use job number from other machine */

        if  (isclient)  {
                SPQ.spq_jflags |= SPQ_CLIENTJOB;
                jn = (ULONG) (ntohl(netid) + SPQ.spq_time) % JOB_MOD;
        }
        else
                jn = SPQ.spq_job;

        sp_req.spr_un.j.spr_pid = getpid();
        outf = goutfile(&jn, tmpfl, pgfl, 0);
        SPQ.spq_job = jn;               /* goutfile might have reset this if it clashes with something */

        if  ((ret = copyout(sock, outf, delim)) != XTNQ_OK  &&  ret != XTNR_WARN_LIMIT)  {
                if  (delim)
                        free(delim);
                if  (ret >= 0)
                        q_reply(sock, ret, (jobno_t) 0);
                return  ret == XTNR_PAST_LIMIT? 1: 0;
        }

        /* Finished with delimiter */

        if  (delim)
                free(delim);

        for  (tries = 0;  tries < MAXTRIES;  tries++)  {
                if  (wjmsg(&sp_req, &SPQ) == 0)  {
                        q_reply(sock, ret, jn);
                        return  1;
                }
                sleep(TRYTIME);
        }

        unlink(tmpfl);
        unlink(pgfl);
        q_reply(sock, XTNR_QFULL, (jobno_t) 0);
        return  0;
}

static  int  jobthere(int sock)
{
        struct  tcp_data  inb;

        if  (!sock_read(sock, (char *) &inb, sizeof(inb)))
                return  0;

        if  (inb.tcp_code != TCP_STARTJOB  &&  inb.tcp_code != TCP_STARTJOB_U)  {
                if  (inb.tcp_code != TCP_CLOSESOCK)
                        q_reply(sock, XTNR_LOST_SYNC, (jobno_t) 0);
                return  0;
        }
        unpack_job(&SPQ, (struct spq *) inb.tcp_buff);
        return  1;
}

/* Process requests to enqueue file (called from rspr)
   Fixed 15/3/12 don't block signals forever after being called */

void  process_q()
{
        int     sock;
        PIDTYPE pid;
        netid_t whofrom;
#ifdef  HAVE_SIGACTION
        sigset_t        sset;
        sigemptyset(&sset);
        sigaddset(&sset, QRFRESH);
        sigprocmask(SIG_BLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
        int     masked = siggetmask();
        sigblock(masked | sigmask(QRFRESH));
#else
        RETSIGTYPE  (*oldsig)(int);
#ifdef  SIG_HOLD
        oldsig = signal(QRFRESH, SIG_HOLD);
#else
        oldsig = signal(QRFRESH, SIG_IGN);
#endif
#endif
        if  ((sock = tcp_serv_accept(qsock, &whofrom)) < 0)  {
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                return;
        }

        if  ((pid = fork()) < 0)  {
                print_error($E{Internal cannot fork});
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                return;
        }

#ifndef BUGGY_SIGCLD
        if  (pid != 0)  {
                close(sock);
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                return;
        }
#else
        /* Make the process the grandchild so we don't have to worry
           about waiting for it later.  */

        if  (pid != 0)  {
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, (int *) 0, 0) < 0  &&  errno == EINTR)
                        ;
#else
                PIDTYPE wpid;
                while  ((wpid = wait((int *) 0)) != pid  &&  (wpid >= 0 || errno == EINTR))
                        ;
#endif
                close(sock);
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                return;
        }
        if  (fork() != 0)
                exit(0);
#endif

        while  (jobthere(sock)  &&  qrest(sock, whofrom))
                ;
        close(sock);
        exit(0);
}

void  process()
{
        int     highfd;
        fd_set  ready;
        struct  timeval  alrm;

        alrm.tv_usec = 0;

        highfd = qsock;
        if  (uasock > highfd)
                highfd = uasock;
        if  (apirsock > highfd)
                highfd = apirsock;

        for  (;;)  {

                /* If not timeouts in progress, don't bother with alarm */

                struct  timeval  *ap = (struct timeval *) 0;
                int  nret;
                unsigned  nexttime = process_alarm();

                if  (nexttime != 0)  {
                        alrm.tv_sec = nexttime;
                        ap = &alrm;
                }

                FD_ZERO(&ready);
                FD_SET(qsock, &ready);
                FD_SET(uasock, &ready);
                FD_SET(apirsock, &ready);

                while  ((nret = select(highfd+1, &ready, (fd_set *) 0, (fd_set *) 0, ap)) < 0)
                        if  (errno != EINTR)
                                exit(0);

                while  (nret > 0)  {
                        if  (FD_ISSET(apirsock, &ready))  {
                                process_api();
                                nret--;
                                continue;
                        }
                        if  (FD_ISSET(uasock, &ready))  {
                                process_ua();
                                nret--;
                                continue;
                        }
                        if  (FD_ISSET(qsock, &ready))  {
                                process_q();
                                nret--;
                                continue;
                        }
                }
        }
}

/* Ye olde main routine.
   I don't expect any arguments & will ignore any the fool gives me,
   apart from remembering my name.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int_ugid_t      chku;
        char    *trf;
#ifndef DEBUG
        PIDTYPE pid;
#endif
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zch;
#endif

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        Realuid = getuid();
        Effuid = geteuid();
        init_mcfile();

        if  ((Cfile = open_icfile()) == (FILE *) 0)
                exit(E_NOCONFIG);

        if  ((chku = lookup_uname(SPUNAME)) == UNKNOWN_UID)  {
                Defaultuid = Daemuid = ROOTID;
                Defaultuser = prin_uname(ROOTID);
        }
        else  {
                Defaultuid = Daemuid = chku;
                Defaultuser = SPUNAME;
        }

        /* Revert to spooler user (we are setuser to root) */

#ifdef  SCO_SECURITY
#ifdef  RUN_AS_ROOT
        setluid(ROOTID);
        setuid(ROOTID);
#else
        setluid(Daemuid);
        setuid(Daemuid);
#endif
#else
#ifdef  RUN_AS_ROOT
        setuid(ROOTID);
#else
        setuid(Daemuid);
#endif
#endif

        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)
                print_error($E{Cannot chdir});

        /* Set timeout value */

        trf = envprocess(XTTIMEOUTS);
        timeouts = atoi(trf);
        free(trf);
        if (timeouts == 0)
                timeouts = NETTICKLE;

        /* Initial processing of host file */

        read_hfiles();

#ifdef  STRUCT_SIG
        zch.sighandler_el = catchhup;
        sigmask_clear(zch);
        zch.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGHUP, &zch, (struct sigstruct_name *) 0);
#else
        signal(SIGHUP, catchhup);
#endif
        openrfile();

        if  (!init_network())  {
                print_error($E{xtnet aborted});
                exit(E_NETERR);
        }

#ifndef DEBUG

        while  ((pid = fork()) < 0)  {
                print_error($E{Fork waiting});
                sleep(30);
        }
        if  (pid != 0)
                return  0;
#ifdef  SETPGRP_VOID
        setpgrp();
#else
        setpgrp(0, getpid());
#endif
        catchsigs(catchabort);
#endif  /* ! DEBUG */

#ifndef BUGGY_SIGCLD
#ifdef  STRUCT_SIG
        zch.sighandler_el = SIG_IGN;
#ifdef  SA_NOCLDWAIT
        zch.sigflags_el |= SA_NOCLDWAIT;
#endif
        sigact_routine(SIGCLD, &zch, (struct sigstruct_name *) 0);
#else
        signal(SIGCLD, SIG_IGN);
#endif
#endif

        lognprocess();
        sp_req.spr_mtype = MT_SCHED;
        sp_req.spr_un.j.spr_act = SJ_ENQ;
        sp_req.spr_un.j.spr_seq = 0;
        sp_req.spr_un.j.spr_netid = 0;
        sp_req.spr_un.j.spr_jslot = 0;
        sp_req.spr_un.j.spr_pid = getpid();
        process();
        return  0;              /* Not reached */
}
