/* xtnet_ua.c -- xtnetserv user access routines
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
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include "incl_sig.h"
#include <errno.h>
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
#include "xtnet_ext.h"
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

struct  udp_conn  {
        struct  udp_conn  *next;                /* Next in hash chain */
        time_t          lastop;                 /* Time of last op */
        netid_t         hostid;                 /* Hostid involved */
        int_ugid_t      host_uid;               /* User id to distinguish UNIX clients or UNKNOWN_UID */
        int_ugid_t      uid;                    /* User id using */
        char            *username;              /* UNIX user name */
};

static  struct  udp_conn        *conn_hash[NETHASHMOD];

extern  unsigned  timeouts;

extern unsigned  calcnhash(const netid_t); /* Defined in look_host.c */

/* These are mostly all to do with pending jobs from the old version of the
   Windows client, however we do try to keep track of clients from UNIX hosts.
   In those cases we track the user id (on the host) to try to distinguish different
   users on the machine. Call find_conn with UNKNOWN_UID if we don't need to worry. */

static  struct  udp_conn  *find_conn(const netid_t hostid, const int_ugid_t uid)
{
        struct  udp_conn  *fp;

        for  (fp = conn_hash[calcnhash(hostid)];  fp;  fp = fp->next)
                if  (fp->hostid == hostid  &&  (uid == UNKNOWN_UID || fp->host_uid == uid))
                        return  fp;
        return  (struct udp_conn *) 0;
}

static  struct  udp_conn  *add_conn(const netid_t hostid)
{
        struct  udp_conn *fp = malloc(sizeof(struct udp_conn));
        unsigned  hashv = calcnhash(hostid);

        if  (!fp)
                nomem();

        fp->next = conn_hash[hashv];
        conn_hash[hashv] = fp;
        fp->hostid = hostid;
        fp->host_uid = UNKNOWN_UID;                       /* Fix this later if not Windows */
        return  fp;
}

static  void    kill_conn(struct udp_conn *fp)
{
        struct  udp_conn  **fpp, *np;

        fpp = &conn_hash[calcnhash(fp->hostid)];

        while  ((np = *fpp))  {
                if  (np == fp)  {
                        *fpp = fp->next;
                        free(fp->username);
                        free((char *) fp);
                        return;
                }
                fpp = &np->next;
        }

        /* "We cannot get here" */
        exit(E_SETUP);
}

struct  pend_job *add_pend(const netid_t whofrom)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                if  (!pend_list[cnt].out_f)  {
                        pend_list[cnt].clientfrom = whofrom;
                        pend_list[cnt].prodsent = 0;
                        return  &pend_list[cnt];
                }
        return  (struct pend_job *) 0;
}

struct  pend_job *find_pend(const netid_t whofrom)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                if  (pend_list[cnt].clientfrom == whofrom)
                        return  &pend_list[cnt];
        return  (struct pend_job *) 0;
}

struct  pend_job *find_j_by_jno(const jobno_t jobno)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                if  (pend_list[cnt].out_f  &&  pend_list[cnt].jobn == jobno)
                        return  &pend_list[cnt];
        return  (struct pend_job *) 0;
}

/* Clean up after decaying job.  */

void  abort_job(struct pend_job *pj)
{
        if  (!pj  ||  !pj->out_f)
                return;
        fclose(pj->out_f);
        pj->out_f = (FILE *) 0;
        if  (pj->delim)  {
                free(pj->delim);
                pj->delim = (char *) 0;
        }
        unlink(pj->tmpfl);
        unlink(pj->pgfl);
        pj->clientfrom = 0;
}

/* Pack up a spdet structure */

void  spdet_pack(struct spdet *to, struct spdet *from)
{
        to->spu_isvalid = from->spu_isvalid;
        strncpy(to->spu_form, from->spu_form, MAXFORM);
        strncpy(to->spu_formallow, from->spu_formallow, ALLOWFORMSIZE);
        strncpy(to->spu_ptr, from->spu_ptr, PTRNAMESIZE);
        strncpy(to->spu_ptrallow, from->spu_ptrallow, JPTRNAMESIZE);
        to->spu_form[MAXFORM] = '\0';
        to->spu_formallow[ALLOWFORMSIZE] = '\0';
        to->spu_ptr[PTRNAMESIZE] = '\0';
        to->spu_ptrallow[JPTRNAMESIZE] = '\0';
        to->spu_user = htonl((ULONG) from->spu_user);
        to->spu_minp = from->spu_minp;
        to->spu_maxp = from->spu_maxp;
        to->spu_defp = from->spu_defp;
        to->spu_cps = from->spu_cps;
        to->spu_flgs = htonl(from->spu_flgs);
        to->spu_class = htonl(from->spu_class);
}

/* Truncate file if neccessary.
   The file is truncated with a truncate call if available,
   otherwise we have to do it by hand.  */

static int  dotrunc(struct pend_job *pj, const LONG size)
{
#ifdef  HAVE_FTRUNCATE
        if  (ftruncate(fileno(pj->out_f), size) < 0)
                return  1;
        pj->jobout.spq_size = size;
#else
        char    *tnam = mkspid("TMP", pj->jobout.spq_job);
        int     ch, fd;
        LONG    cnt = 0;
        FILE    *tf;
        if  ((fd = open(tnam, O_RDWR|O_CREAT|O_TRUNC|O_EXCL, 0400)) < 0  ||  !(tf = fdopen(fd, "w+")))
                return  1;
        rewind(pj->out_f);
        while  ((ch = getc(pj->out_f)) != EOF  &&  cnt < size)  {
                putc(ch, tf);
                cnt++;
        }
        pj->jobout.spq_size = cnt;
        fclose(pj->out_f);
        pj->out_f = tf;
        rewind(tf);
        if  (unlink(pj->tmpfl) < 0  ||  link(tnam, pj->tmpfl) < 0  ||  unlink(tnam) < 0)
                return  1;
#endif
        return  0;
}

/* At end of UDP or API job, scan for pages
   Return 0 (or XTNR_WARN_LIMIT) if ok otherwise error code.  */

int  scan_job(struct pend_job *pj)
{
        char    *rcp;
        int     ch, rec_cnt, pgfid = -1, retcode = XTNQ_OK;
        ULONG   klim = 0xffffffffL;
        LONG    plim = 0x7fffffffL, onpage, char_count = 0;
        char    *rcdend;

        pj->jobout.spq_npages = 0;
        onpage = 0;
        rewind(pj->out_f);

        if  (pj->jobout.spq_pglim)  {
                if  (pj->jobout.spq_dflags & SPQ_PGLIMIT)
                        plim = pj->jobout.spq_pglim;
                else
                        klim = (ULONG) pj->jobout.spq_pglim << 10;
        }

        if  ((ULONG) pj->jobout.spq_size > klim)  {
                if  (pj->jobout.spq_dflags & SPQ_ERRLIMIT  ||  dotrunc(pj, (LONG) klim))
                        return  XTNR_PAST_LIMIT;
                retcode = XTNR_WARN_LIMIT;
        }

        if  (!pj->delim  ||  (pj->pageout.deliml == 1  &&  pj->pageout.delimnum == 1 &&  pj->delim[0] == '\f'))  {
                pj->jobout.spq_dflags &= SPQ_ERRLIMIT | SPQ_PGLIMIT; /* I didnt mean a ~ here */

                while  (char_count < pj->jobout.spq_size  &&  (ch = getc(pj->out_f)) != EOF)  {
                        char_count++;
                        onpage++;
                        if  (ch == '\f')  {
                                onpage = 0;
                                if  (++pj->jobout.spq_npages > plim)  {
                                        if  (pj->jobout.spq_dflags & SPQ_ERRLIMIT || dotrunc(pj, char_count))
                                                return  XTNR_PAST_LIMIT;
                                        retcode = XTNR_WARN_LIMIT;
                                        break;
                                }
                        }
                }
                if  (char_count == 0)
                        return  XTNR_EMPTYFILE;

                /* If we hit EOF before we expected then the file
                   system must be full */

                if  (char_count != pj->jobout.spq_size)
                        return  XTNR_FILE_FULL;
                if  (onpage)
                        pj->jobout.spq_npages++;
                return  retcode;
        }

        pj->jobout.spq_dflags |= SPQ_PAGEFILE;

        if  ((pgfid = open(pj->pgfl, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0)
                return  XTNR_CC_PAGEFILE;

#ifdef  RUN_AS_ROOT
#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                fchown(pgfid, Daemuid, getegid());
#else
        if  (Daemuid != ROOTID)
                chown(pj->pgfl, Daemuid, getegid());
#endif
#endif
        pj->pageout.lastpage = 0;       /* Fix this later perhaps */
        Ignored_error = write(pgfid, (char *) &pj->pageout, sizeof(pj->pageout));
        Ignored_error = write(pgfid, pj->delim, (unsigned) pj->pageout.deliml);

        rcp = pj->delim;
        rcdend = pj->delim + pj->pageout.deliml;
        onpage = 0;
        rec_cnt = 0;

        while  (char_count < pj->jobout.spq_size  &&  (ch = getc(pj->out_f)) != EOF)  {
                char_count++;
                onpage++;
                if  (ch == *rcp)  {
                        if  (++rcp >= rcdend)  {
                                if  (++rec_cnt >= pj->pageout.delimnum)  {
                                        if  (write(pgfid, (char *) &char_count, sizeof(LONG)) != sizeof(LONG))
                                                return  XTNR_FILE_FULL;
                                        onpage = 0;
                                        rec_cnt = 0;
                                        if  (++pj->jobout.spq_npages > plim)  {
                                                if  (pj->jobout.spq_dflags & SPQ_ERRLIMIT  ||  dotrunc(pj, char_count))
                                                        return  XTNR_PAST_LIMIT;
                                                retcode = XTNR_WARN_LIMIT;
                                                break;
                                        }
                                }
                                rcp = pj->delim;
                        }
                }
                else  if  (rcp > pj->delim)  {
                        char    *pp, *prcp, *prevpl;
                        prevpl = --rcp; /*  Last one matched  */
                        for  (;  rcp > pj->delim;  rcp--)  {
                                if  (*rcp != ch)
                                        continue;
                                pp = prevpl;
                                prcp = rcp - 1;
                                for  (;  prcp >= pj->delim;  pp--, prcp--)
                                        if  (*pp != *prcp)
                                                goto  rej;
                                rcp++;
                                break;
                        rej:    ;
                        }
                }
        }

        if  (char_count == 0)  {
                close(pgfid);
                return  XTNR_EMPTYFILE;
        }

        /* If we hit EOF before we expected then the file system must
           be full */

        if  (char_count != pj->jobout.spq_size)
                return  XTNR_FILE_FULL;

        /* Store the offset of the end of the file */

        if  (write(pgfid, (char *) &pj->jobout.spq_size, sizeof(LONG)) != sizeof(LONG))
                return  XTNR_FILE_FULL;

        /* Remember how big the last page was */

        if  (onpage > 0)  {
                pj->jobout.spq_npages++;
                if  ((pj->pageout.lastpage = pj->pageout.delimnum - rec_cnt) > 0)  {
                        lseek(pgfid, 0L, 0);
                        Ignored_error = write(pgfid, (char *) &pj->pageout, sizeof(pj->pageout));
                }
        }

        if  (close(pgfid) < 0)
                return  XTNR_FILE_FULL;
        return  retcode;
}


static  void  udp_send_vec(char *vec, const int size, struct sockaddr_in *sinp)
{
        sendto(uasock, vec, size, 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Similar routine for when we are sending from scratch */

static void  udp_send_to(char *vec, const int size, const netid_t whoto)
{
        int     sockfd, tries;
        struct  sockaddr_in     to_sin, cli_addr;

        BLOCK_ZERO(&to_sin, sizeof(to_sin));
        to_sin.sin_family = AF_INET;
        to_sin.sin_port = uaportnum;
        to_sin.sin_addr.s_addr = whoto;

        BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
        cli_addr.sin_family = AF_INET;
        cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        cli_addr.sin_port = 0;

        /* We don't really need the cli_addr but we are obliged to
           bind something.  The remote uses our standard port.  */

        for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
                if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                        return;
                if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
                        close(sockfd);
                        return;
                }
                if  (sendto(sockfd, vec, size, 0, (struct sockaddr *) &to_sin, sizeof(to_sin)) >= 0)  {
                        close(sockfd);
                        return;
                }
                close(sockfd);
        }
}

void  udp_send_ulist(struct sockaddr_in *sinp)
{
        char    **ul;
        char    reply[CL_SV_BUFFSIZE];

        if  ((ul = gen_ulist((char *) 0, 0)))  {
                char    **up;
                int     rp = 0;
                for  (up = ul;  *up;  up++)  {
                        int     lng = strlen(*up) + 1;
                        if  (lng + rp > CL_SV_BUFFSIZE)  {
                                udp_send_vec(reply, rp, sinp);
                                rp = 0;
                        }
                        strcpy(&reply[rp], *up);
                        rp += lng;
                        free(*up);
                }
                if  (rp > 0)
                        udp_send_vec(reply, rp, sinp);
                free((char *) ul);
        }

        /* Mark end with a null */

        reply[0] = '\0';
        udp_send_vec(reply, 1, sinp);
}

int  checkpw(const char *name, const char *passwd)
{
        static  char    *sppwnam;
        int             ipfd[2], opfd[2];
        char            rbuf[1];
        PIDTYPE         pid;

        if  (!sppwnam)
                sppwnam = envprocess(SPPWPROG);

        /* Don't bother with error messages, just say no.  */

        if  (pipe(ipfd) < 0)
                return  0;
        if  (pipe(opfd) < 0)  {
                close(ipfd[0]);
                close(ipfd[1]);
                return  0;
        }

        if  ((pid = fork()) == 0)  {
                close(opfd[1]);
                close(ipfd[0]);
                if  (opfd[0] != 0)  {
                        close(0);
                        Ignored_error = dup(opfd[0]);
                        close(opfd[0]);
                }
                if  (ipfd[1] != 1)  {
                        close(1);
                        Ignored_error = dup(ipfd[1]);
                        close(ipfd[1]);
                }
                execl(sppwnam, sppwnam, name, (char *) 0);
                exit(255);
        }
        close(opfd[0]);
        close(ipfd[1]);
        if  (pid < 0)  {
                close(ipfd[0]);
                close(opfd[1]);
                return  0;
        }
        Ignored_error = write(opfd[1], passwd, strlen(passwd));
        rbuf[0] = '\n';
        Ignored_error = write(opfd[1], rbuf, sizeof(rbuf));
        close(opfd[1]);
        if  (read(ipfd[0], rbuf, sizeof(rbuf)) != sizeof(rbuf))  {
                close(ipfd[0]);
                return  0;
        }
        close(ipfd[0]);
        return  rbuf[0] == '0'? 1: 0;
}

/* Tell scheduler about new user.  */

void  tell_sched_roam(const netid_t netid, const char *unam)
{
        struct  spr_req nmsg;
        if  (netid == 0)
                return;
        BLOCK_ZERO(&nmsg, sizeof(nmsg));
        nmsg.spr_mtype = MT_SCHED;
        nmsg.spr_un.n.spr_act = SON_ROAMUSER;
        nmsg.spr_un.n.spr_pid = getpid();
        nmsg.spr_un.n.spr_n.hostid = netid;
        strncpy(nmsg.spr_un.n.spr_n.hostname, unam, HOSTNSIZE);
        msgsnd(Ctrl_chan, (struct msgbuf *) &nmsg, sizeof(struct sp_nmsg), 0); /* Wait until it goes */
}

/* Bad return from enquiry - we return the errors in the user id field of
   the permissions structure, which is otherwise useless.  */

static  void  enq_badret(struct sockaddr_in *sinp, const int code)
{
        struct  ua_reply  rep;
        BLOCK_ZERO(&rep, sizeof(rep));
        rep.ua_perm.spu_user = htonl(code);
        sendto(uasock, (char *) &rep, sizeof(rep), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

static  void  job_reply(struct sockaddr_in *sinp, const int code)
{
        char    reply[1];
        reply[0] = code;
        sendto(uasock, reply, sizeof(reply), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

static  void  bad_log(struct sockaddr_in *sinp, const int code)
{
        struct  ua_login  reply;
        BLOCK_ZERO(&reply, sizeof(reply));
        reply.ual_op = code;
        sendto(uasock, (char *) &reply, sizeof(reply), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Handle "enquiry" which clients do before login in case the original
   login hasn't timed out. */

static  void  do_enquiry(struct sockaddr_in *sinp, char *pmsg, const int inlength)
{
        struct  ua_reply  rep;
        char    *luser;
        int_ugid_t  luid;

        /* Rspr and UNIX-based clients put the user name they're asking about after the code */

        if  (inlength > 1)  {
                luser = &pmsg[1];
                luid = lookup_uname(luser);
                if  (luid == UNKNOWN_UID)  {
                        enq_badret(sinp, XTNR_NOT_USERNAME);
                        return;
                }
        }
        else  {
                netid_t whofrom = sinp->sin_addr.s_addr;                /* Not expected to be local host */
                struct  udp_conn  *fp;

                if  ((fp = find_conn(whofrom, UNKNOWN_UID)))  { /* A logged-in person */
                        luser = fp->username;
                        luid = fp->uid;
                }
                else  {
                        struct  alhash  *ap = find_autoconn(whofrom);
                        if  (!ap)  {
                                enq_badret(sinp, XTNR_UNKNOWN_CLIENT);
                                return;
                        }

                        luser = ap->unixname;
                        luid = ap->uuid;
                }
        }

        strcpy(rep.ua_uname, luser);
        spdet_pack(&rep.ua_perm, getspuentry(luid));
        sendto(uasock, (char *) &rep, sizeof(rep), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Return OK login and UNIX user name we're in as */

static  void    logret_ok(struct sockaddr_in *sinp, const char *uname)
{
        struct  ua_login  reply;

        BLOCK_ZERO(&reply, sizeof(reply));
        reply.ual_op = UAL_OK;
        strncpy(reply.ual_name, uname, sizeof(reply.ual_name)-1);
        sendto(uasock, (char *) &reply, sizeof(reply), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Enquiry about login status from UNIX host.
   We always allow connection from the current one */

void  udp_uenquire(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn  *fp;
        int_ugid_t  uuid, ruid;

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        ruid = ntohl(inmsg->ua_un.ual_uid);

        if  ((fp = find_conn(whofrom, ruid)))  {

                /* Existing connection.
                   If it's for a different user, reject unless it's on "my" machine. */

                if  (ncstrcmp(fp->username, inmsg->ual_name) != 0)  {

                        if  (whofrom != 0)  {
                                bad_log(sinp, XTNR_NOT_USERNAME);
                                return;
                        }

                        if  ((uuid = lookup_uname(inmsg->ual_name)) == UNKNOWN_UID)  {
                                bad_log(sinp, XTNR_NOT_USERNAME);
                                return;
                        }
                        free(fp->username);
                        fp->username = stracpy(inmsg->ual_name);
                        fp->uid = uuid;
                }

                fp->lastop = time((time_t *) 0);
                logret_ok(sinp, fp->username);
                tell_sched_roam(whofrom, fp->username);         /* Not really needed, scheduler doesn't worry about user name */
                return;
        }

        /* Create standard connection only from "me" */

        if  (whofrom != 0)  {
                bad_log(sinp, XTNR_NO_PASSWD);
                return;
        }

        if  ((uuid = lookup_uname(inmsg->ual_name)) == UNKNOWN_UID)  {
                bad_log(sinp, XTNR_NOT_USERNAME);
                return;
        }

        /* Create connection for that person */

        fp = add_conn(whofrom);
        fp->host_uid = ruid;
        fp->lastop = time((time_t *) 0);
        fp->username = stracpy(inmsg->ual_name);
        fp->uid = uuid;
        logret_ok(sinp, fp->username);
}

void  udp_enquire(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);           /* We don't anticipate this to be "me" */
        struct  udp_conn  *fp;
        struct  alhash  *wp;

        /* NB the size of the login structure may change (longer Win user name) so
           this will cream out old clients */

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        /* See if it is someone we know, if so refresh the access time and return the details */

        if  ((fp = find_conn(whofrom, UNKNOWN_UID)))  {
                fp->lastop = time((time_t *) 0);
                logret_ok(sinp, fp->username);
                return;
        }

        /* See if it is someone we connect automatically */

        if  (!(wp = find_autoconn(whofrom)))  {
                bad_log(sinp, XTNR_NO_PASSWD);
                return;
        }

        /* Create connection for that person */

        fp = add_conn(whofrom);
        fp->lastop = time((time_t *) 0);
        fp->uid = wp->uuid;
        fp->username = stracpy(wp->unixname);
        logret_ok(sinp, fp->username);
        tell_sched_roam(whofrom, fp->username);
}

void  udp_ulogin(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);           /* Might be "me" */
        struct  udp_conn  *fp;
        int_ugid_t  luid, ruid;

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        ruid = ntohl(inmsg->ua_un.ual_uid);

        /* See if someone is already logged in and report success if it is the same person
           as before. Refresh the activity time. NB "whofrom" might be "me". */

        if  ((fp = find_conn(whofrom, ruid))  &&  ncstrcmp(fp->username, inmsg->ual_name) == 0)  {
                fp->lastop = time((time_t *) 0);
                logret_ok(sinp, fp->username);
                return;
        }

        /* Check password, if not OK, kill any existing connection */

        if  ((luid = lookup_uname(inmsg->ual_name)) == UNKNOWN_UID  ||  !checkpw(inmsg->ual_name, inmsg->ual_passwd))  {
                bad_log(sinp, XTNR_PASSWD_INVALID);
                if  (fp)
                        kill_conn(fp);
                return;
        }

        /* Reset user name and ID of existing connection or allocate a new one */

        if  (fp)
                free(fp->username);
        else  {
                fp = add_conn(whofrom);
                fp->host_uid = ruid;
        }
        fp->lastop = time((time_t *) 0);
        fp->uid = luid;
        fp->username = stracpy(inmsg->ual_name);
        logret_ok(sinp, fp->username);
        tell_sched_roam(whofrom, fp->username);
}

/* Log in as a Windows user */

void  udp_login(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn  *fp;
        struct  winuhash  *wn;
        char    *lname;
        int_ugid_t  luid;

        /* First check the message size */

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        /* See what the Unix name is corresponding to that windows name. (Or take it as possible Unix name).
           Use the default user name if needed (should not be a security risk with the passwords set correctly) */

        if  ((wn = lookup_winoruu(inmsg->ual_name)))  {
                lname = wn->unixname;
                luid = wn->uuid;
        }
        else  {
                lname = Defaultuser;
                luid = Defaultuid;
        }

        /* If we were logged in OK as that guy before, just refresh
           and say OK. NB "whofrom" might be me again. */

        if  ((fp = find_conn(whofrom, UNKNOWN_UID)) && fp->uid == luid)  {
                fp->lastop = time((time_t *) 0);
                logret_ok(sinp, fp->username);
                return;
        }

        /* Check the password and cancel existing login if wrong */

        if  (!checkpw(lname, inmsg->ual_passwd))  {
                bad_log(sinp, XTNR_PASSWD_INVALID);
                if  (fp)
                        kill_conn(fp);
                return;
        }

        /* Reset user name and ID of existing connection or allocate a new one */

        if  (fp)
                free(fp->username);
        else
                fp = add_conn(whofrom);
        fp->lastop = time((time_t *) 0);
        fp->uid = luid;
        fp->username = stracpy(lname);
        logret_ok(sinp, fp->username);
        tell_sched_roam(whofrom, fp->username);
}

/* Log out whoever is logged in, no reply */

void  udp_logout(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn  *fp;
        int_ugid_t      ruid = UNKNOWN_UID;     /* Remote user id */

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        /* If this comes from a UNIX host, we have NON null in the filler and
           the uid (at the other end) in the group name field. We do it this way
           as existing Windows clients zero the whole thing and just put in UAL_LOGOUT */

        if  (inmsg->ual_fill != '\0')
                ruid = ntohl(inmsg->ua_un.ual_uid);

        if  ((fp = find_conn(whofrom, ruid)))
                kill_conn(fp);
}

void  udp_job_process(struct sockaddr_in *sinp, char *pmsg, int datalength)
{
        int                     ret, tries;
        netid_t                 whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn        *fp;
        struct  pend_job        *pj;
        time_t                  now = time((time_t *) 0);

        /* If we think this guy isn't connected, reject it */

        if  (!(fp = find_conn(whofrom, UNKNOWN_UID))  ||  fp->host_uid != UNKNOWN_UID)  {
                job_reply(sinp, XTNR_UNKNOWN_CLIENT);
                return;
        }

        pj = find_pend(whofrom);
        fp->lastop = now;

        switch  (pmsg[0])  {
        case  CL_SV_STARTJOB:

                /* Start of new job, delete any pending one. add new one */

                abort_job(pj);
                pj = add_pend(whofrom);
                if  (!pj)  {
                        job_reply(sinp, XTNR_NOMEM_QF);
                        return;
                }
                pj->lastaction = now;

                /* Generate job number and output file vaguely from netid etc.  */

                pj->jobn = (ULONG) (ntohl(whofrom) + now) % JOB_MOD;
                pj->out_f = goutfile(&pj->jobn, pj->tmpfl, pj->pgfl, 1);

                /* Step over to job descriptor and unpack */

                pmsg += sizeof(LONG);
                datalength -= sizeof(LONG);
                if  (datalength < sizeof(struct spq))  {
                        abort_job(pj);
                        job_reply(sinp, SV_CL_BADPROTO);
                        return;
                }
                unpack_job(&pj->jobout, (struct spq *) pmsg);
                pj->jobout.spq_orighost = whofrom;

                /* Check job details */

                pj->jobout.spq_uid = fp->uid;
                strncpy(pj->jobout.spq_uname, fp->username, UIDSIZE);
                if  (lookup_uname(pj->jobout.spq_puname) == UNKNOWN_UID)
                        strncpy(pj->jobout.spq_puname, fp->username, UIDSIZE);

                if  ((ret = validate_job(&pj->jobout)) != 0)  {
                        abort_job(pj);
                        job_reply(sinp, ret);
                        return;
                }

                /* Read optional page descriptor */

                pmsg += sizeof(struct spq);
                datalength -= sizeof(struct spq);
                pj->penddelim = 0;
                if  (pj->jobout.spq_dflags & SPQ_PAGEFILE)  {
                        if  (datalength < sizeof(struct pages))  {
                                abort_job(pj);
                                job_reply(sinp, SV_CL_BADPROTO);
                                return;
                        }
                        BLOCK_COPY(&pj->pageout, pmsg, sizeof(struct pages));
                        pmsg += sizeof(struct pages);
                        datalength -= sizeof(struct pages);
#ifndef WORDS_BIGENDIAN
                        pj->pageout.delimnum = ntohl((ULONG) pj->pageout.delimnum);
                        pj->pageout.deliml = ntohl((ULONG) pj->pageout.deliml);
#endif

                        /* Read page delimiter */

                        if  (pj->pageout.deliml <= 0)  {
                                abort_job(pj);
                                job_reply(sinp, XTNR_BAD_PF);
                                return;
                        }
                        if  (!(pj->delim = malloc((unsigned) pj->pageout.deliml)))  {
                                abort_job(pj);
                                job_reply(sinp, XTNR_NOMEM_PF);
                                return;
                        }
                        if  (pj->pageout.deliml <= datalength)
                                BLOCK_COPY(pj->delim, pmsg, pj->pageout.deliml);
                        else  {
                                pj->penddelim = pj->pageout.deliml - datalength;
                                BLOCK_COPY(pj->delim, pmsg, datalength);
                        }
                }
                pj->jobout.spq_job = pj->jobn;
                pj->jobout.spq_size = 0;
                pj->jobout.spq_time = (LONG) pj->lastaction;
                job_reply(sinp, XTNQ_OK);
                return;

        case  CL_SV_CONTDELIM:

                if  (!pj)  {
                        job_reply(sinp, SV_CL_UNKNOWNJ);
                        return;
                }

                pj->lastaction = time((time_t *) 0);

                /* Might be more of the page delimiter */

                if  (pj->penddelim != 0)  {
                        if  (pj->penddelim <= (unsigned) datalength)  {
                                BLOCK_COPY(pj->delim + (unsigned) pj->pageout.deliml - pj->penddelim, pmsg, pj->penddelim);
                                pj->penddelim = 0;
                        }
                        else  {
                                BLOCK_COPY(pj->delim + (unsigned) pj->pageout.deliml - pj->penddelim, pmsg, datalength);
                                pj->penddelim -= datalength;
                        }
                        job_reply(sinp, XTNQ_OK);
                        return;
                }
                job_reply(sinp, SV_CL_UNKNOWNJ);
                return;

        case  CL_SV_JOBDATA:

                if  (!pj)  {
                        job_reply(sinp, SV_CL_UNKNOWNJ);
                        return;
                }

                pj->lastaction = time((time_t *) 0);

                while  (--datalength > 0)  {
                        pmsg++;
                        if  (putc(*(unsigned char *)pmsg, pj->out_f) == EOF)  {
                                job_reply(sinp, XTNR_FILE_FULL);
                                return;
                        }
                        pj->jobout.spq_size++;
                }
                if  (fflush(pj->out_f) == EOF)  {
                        job_reply(sinp, XTNR_FILE_FULL);
                        return;
                }

                job_reply(sinp, XTNQ_OK);
                return;

        case  CL_SV_ENDJOB:
                if  (!pj)  {
                        job_reply(sinp, SV_CL_UNKNOWNJ);
                        return;
                }
                if  ((ret = scan_job(pj)) != XTNQ_OK  &&  ret != XTNR_WARN_LIMIT)  {
                        abort_job(pj);
                        job_reply(sinp, ret);
                        return;
                }

                /* Copy job to request buffer
                   (NB Child TCP processes initialise their copy of SPQ).  */

                for  (tries = 0;  tries < MAXTRIES;  tries++)  {
                        if  (wjmsg(&sp_req, &pj->jobout) == 0)  {
                                fclose(pj->out_f);
                                pj->out_f = (FILE *) 0;
                                if  (pj->delim)  {
                                        free(pj->delim);
                                        pj->delim = (char *) 0;
                                }
                                pj->clientfrom = 0;
                                job_reply(sinp, XTNQ_OK);
                                return;
                        }
                        sleep(TRYTIME);
                }
                abort_job(pj);
                job_reply(sinp, XTNR_QFULL);
                return;

        case  CL_SV_HANGON:
                if  (!pj)  {
                        job_reply(sinp, SV_CL_UNKNOWNJ);
                        return;
                }
                pj->lastaction = time((time_t *) 0);
                pj->prodsent = 0;
                job_reply(sinp, XTNQ_OK);
                return;
        }
}

/* This is mainly for dosspwrite - find the clients logged in as given user */

static void  answer_asku(struct sockaddr_in *sinp, struct ua_pal *inmsg, const int inlng)
{
        int     nu = 0, cnt;
        struct  udp_conn  *hp;
        struct  ua_asku_rep  reply;

        BLOCK_ZERO(&reply, sizeof(reply));

        if  (inlng != sizeof(struct ua_pal))
                goto  dun;

        for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
                for  (hp = conn_hash[cnt];  hp;  hp = hp->next)
                        if  (strcmp(hp->username, inmsg->uap_name) == 0)  {
                                reply.uau_ips[nu++] = hp->hostid;
                                if  (nu >= UAU_MAXU)
                                        goto  dun;
                        }
        }
 dun:
        reply.uau_n = htons(nu);
        udp_send_vec((char *) &reply, sizeof(reply), sinp);
}

/* Respond to keep alive messages  */

static void  tickle(struct sockaddr_in *sinp)
{
        struct  udp_conn  *fp = find_conn(sockaddr2int_netid_t(sinp), UNKNOWN_UID);
        if  (fp)
                fp->lastop = time((time_t *) 0);
        job_reply(sinp, XTNQ_OK);
}

void  process_ua()
{
        int     datalength;
        LONG    pmsgl[CL_SV_BUFFSIZE/sizeof(LONG)]; /* Force to long */
        char    *pmsg = (char *) pmsgl;
        struct  sockaddr_in     sin;
        SOCKLEN_T               sinl = sizeof(sin);

        while  ((datalength = recvfrom(uasock, pmsg, sizeof(pmsgl), 0, (struct sockaddr *) &sin, &sinl)) < 0)
                if  (errno != EINTR)
                    return;

        /* Deal with job data etc above */

        switch  (pmsg[0])  {
        default:
                job_reply(&sin, SV_CL_UNKNOWNC);
                return;

        case  CL_SV_UENQUIRY:
                do_enquiry(&sin, pmsg, datalength);
                return;

        case  CL_SV_ULIST:
                udp_send_ulist(&sin);
                return;

        case  UAL_UENQUIRE:
                udp_uenquire(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  UAL_ENQUIRE:      /* This is done first */
                udp_enquire(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  UAL_ULOGIN:       /* Log in with UNIX name */
                udp_ulogin(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  UAL_LOGIN:
                udp_login(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  UAL_LOGOUT:
                udp_logout(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  CL_SV_STARTJOB:
        case  CL_SV_CONTDELIM:
        case  CL_SV_JOBDATA:
        case  CL_SV_ENDJOB:
        case  CL_SV_HANGON:
                udp_job_process(&sin, pmsg, datalength);
                return;

        case  SV_SV_LOGGEDU:            /* Don't support these any more */
        case  SV_SV_ASKALL:
                return;

        case  SV_SV_ASKU:
                answer_asku(&sin, (struct ua_pal *) pmsg, datalength);
                return;

        case  UAL_OK:           /* Actually these are redundant */
        case  UAL_NOK:
        case  CL_SV_KEEPALIVE:
                tickle(&sin);
                return;
        }
}

/* Tell the client we think it should wake up */

void  send_prod(struct pend_job *pj)
{
        char    prodit = SV_CL_TOENQ;
        udp_send_to(&prodit, sizeof(prodit), pj->clientfrom);
}

/* See which UDP ports seem to have dried up
   Return time of next alarm (only we now use the timeout in select instead) */

unsigned  process_alarm()
{
        time_t  now = time((time_t *) 0);
        unsigned  mintime = 0, nexttime;
        int     prodtime, killtime, cnt;

        /* Scan list of jobs being added to see half-baked ones */

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                if  (pend_list[cnt].clientfrom != 0L)  {
                        struct  pend_job  *pj = &pend_list[cnt];
                        prodtime = pj->lastaction + timeouts - now;
                        killtime = prodtime + timeouts;
                        if  (killtime < 0)  {
                                abort_job(pj); /* Must have died */
                                continue;
                        }
                        if  (prodtime <= 0)  {
                                send_prod(pj);
                                nexttime = killtime;
                        }
                        else
                                nexttime = prodtime;
                        if  (mintime != 0  &&  (mintime == 0 || mintime > nexttime))
                                mintime = nexttime;
                }

        /* Apply timeouts to stale logins.  */

        for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
                struct  udp_conn        **hpp, *hp;

            restartch:

                /* Have to restart chain at the beginning as freeing hp below clobbers pointer */

                hpp = &conn_hash[cnt];

                while  ((hp = *hpp))  {
                        long    tdiff = (long) (hp->lastop + timeouts) - now;
                        if  (tdiff <= 0)  {
                                *hpp = hp->next;
                                hpp = &hp->next;
                                free(hp->username);
                                free((char *) hp);
                                goto  restartch;
                        }
                        else  {
                                if  (mintime == 0 || mintime > (unsigned) tdiff)
                                        mintime = (unsigned) tdiff;
                                hpp = &hp->next;
                        }
                }
        }
        return  mintime;
}

