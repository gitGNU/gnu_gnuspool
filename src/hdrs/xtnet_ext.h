/* xtnet_ext.h -- declarations for xtnetserv

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

struct  pend_job        {
        netid_t   clientfrom;           /* Who is responsible (or 0) */
        FILE    *out_f;                 /* Output file */
        char    *delim;                 /* Delimiter if appropriate */
        char    tmpfl[NAMESIZE+1];      /* Temporary file */
        char    pgfl[NAMESIZE+1];       /* Page file */
        unsigned  char  prodsent;       /* Sent a prod */
        USHORT          penddelim;      /* Number of pending delimiter bytes */
        time_t  lastaction;             /* Last message received (for timeout) */
        jobno_t jobn;                   /* Job number we are creating */
        struct  spq     jobout;         /* Job details pending */
        struct  pages   pageout;        /* Page file details if appropriate */
};

/* Structure used to hash host ids, we now only worry about whether the caller is a client */

struct  hhash   {
        struct  hhash   *hn_next;       /* Next in hash chain */
        netid_t         hostid;         /* IP address */
        int             isme;           /* Is local host */
        int             isclient;       /* Is Windows client */
};

/* Structure used to map Windows user names to UNIX ones */

struct  winuhash  {
        struct  winuhash  *next;                /* Next in hash chain */
        char            *winname;               /* Windows name */
        char            *unixname;              /* UNIX name */
        int_ugid_t      uuid;                   /* UNIX user id */
};

/* Structure to hold details of auto-login hosts (this should probably be removed) */

struct  alhash  {
        struct  alhash  *next;                  /* Next in hash chain */
        netid_t         hostid;                 /* Host id */
        char            *unixname;              /* Unix user name */
        int_ugid_t      uuid;                   /* Unix uid */
};

#ifndef _NFILE
#define _NFILE  64
#endif

#define MAX_PEND_JOBS   (_NFILE / 3)

#define MAXTRIES        5
#define TRYTIME         20

/* It seems to take more than one attempt to set up a UDP port at times, so....  */

#define UDP_TRIES       3

#define JN_INC  80000           /*  Add this to job no if clashes */
#define JOB_MOD 60000           /*  Modulus of job numbers */

extern  char    *Defaultuser;
extern  int_ugid_t  Defaultuid;
extern  SHORT   qsock, uasock, apirsock;
extern  SHORT   qportnum, uaportnum, apirport, apipport;
#ifndef USING_FLOCK
extern  struct  sembuf  jr[], ju[], pr[], pu[];
#endif
extern  struct  spr_req sp_req;
extern  struct  pend_job        pend_list[];
extern  int     hadrfresh;

extern void  abort_job(struct pend_job *);

extern void  process_api();
extern void  process_ua();
extern void  unpack_job(struct spq *, struct spq *);

extern int  scan_job(struct pend_job *);
extern int  tcp_serv_accept(const int, netid_t *);
extern int  validate_job(struct spq *);
extern int  checkpw(const char *, const char *);

extern struct hhash *lookup_hhash(const netid_t);
extern struct winuhash *lookup_winu(const char *);
extern struct winuhash *lookup_winoruu(const char *);
extern struct alhash *find_autoconn(const netid_t);

extern unsigned  process_alarm();

extern struct   pend_job *add_pend(const netid_t);
extern struct   pend_job *find_j_by_jno(const jobno_t);
extern FILE     *goutfile(jobno_t *, char *, char *, const int);
