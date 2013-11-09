/* network.h -- structures for keeping track of connections

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

#include "incl_net.h"

struct  remote  {
        /* NB we assume in various places that HOSTNSIZE > UIDSIZE */
        char    hostname[HOSTNSIZE];    /* Actual host name (alternatively user name) */
        char    alias[HOSTNSIZE];       /* Alias for within Xi-Text */
        SHORT   sockfd;                 /* Socket fd to talk to it */
        netid_t hostid;                 /* Host id in network byte order */
        struct  remote  *hash_next;     /* Other remotes in same hash bucket see sh_network.c */
        USHORT          ht_flags;       /* Host-type flags */
#define HT_PROBEFIRST   (1 << 1)        /* Probe connection first */
#define HT_MANUAL       (1 << 2)        /* Manual connection only */
#define HT_DOS          (1 << 3)        /* System is only client, don't attempt to connect to it (historic name) */
#define HT_PWCHECK      (1 << 4)        /* Check password of user */
#define HT_ROAMUSER     (1 << 5)        /* Roaming user */
#define HT_TRUSTED      (1 << 6)        /* Trusted host */
        USHORT          stat_flags;     /* State flags in scheduler */
#define SF_ISCLIENT     (1 << 0)        /* Set to indicate "I" am client */
#define SF_PROBED       (1 << 1)        /* Set to indicate probe sent */
#define SF_CONNECTED    (1 << 2)        /* Connection complete */
#define SF_NOTSERVER    (1 << 3)        /* Not a server so don't expect printers etc */
        enum  sync_state { NSYNC_NONE = 0, NSYNC_REQ = 1, NSYNC_OK = 2 } is_sync;
        USHORT  ht_timeout;             /* Timeout value (seconds) */
        time_t  lastwrite;              /* When last done */
        USHORT  ht_seqto;               /* Sequence TO */
        USHORT  ht_seqfrom;             /* Sequence From */
};

struct  feeder  {
        char    fdtype;                 /* Type of file required */
        char    resvd[3];               /* Pad out to 4 bytes */
        slotno_t  jobslot;              /* Job slot net byte order */
        jobno_t  jobno;                 /* Jobnumber net byte order */
};

/* General messages */

struct  netmsg  {
        USHORT          code;   /* Code number */
        USHORT          seq;    /* Sequence */
        netid_t         hostid; /* Host id net byte order */
        LONG            arg;    /* Argument */
};
struct  nihdr   {               /* Incoming messages, see what in a minute */
        USHORT          code;   /* Code number */
        USHORT          seq;    /* Sequence */
};

/* Size of hash table used in various places */

#define NETHASHMOD      97              /* Not very big prime number */

/* Fields in /etc/gnuspool-hosts lines */

#define HOSTF_HNAME     0
#define HOSTF_ALIAS     1
#define HOSTF_FLAGS     2
#define HOSTF_TIMEOUT   3

/* Define these here to save extensive ifdefs */

#define FEED_SP         0       /* Feed spool file */
#define FEED_NPSP       1       /* Feed spool file, don't bother with pages */
#define FEED_ER         2       /* Feed error file */
#define FEED_PF         3       /* Feed page file */

/* Functions for manipulating host file */

extern  void  end_hostfile();
extern  struct remote *get_hostfile();
extern  void  hash_hostfile();

extern  char            *get_myhostname();                                      /* Get my hostname which we standardise */
extern  char            *look_host(const netid_t);                              /* Look up external host id */
extern  char            *look_int_host(const netid_t);                  /* Look up internal host id */
extern  const  char    *host_prefix_str(const netid_t, const char *);   /* Generate prefix with hostid:name */
extern  const  char    *host_prefix_long(const netid_t, const LONG);    /* Generate prefix with hostid:value */

extern  netid_t         look_hostname(const char *);                            /* Look up host name and return external id */
extern  netid_t         look_int_hostname(const char *);                        /* Look up host name and reteurn internal id */

/* We now have routines for this to provide for IPv6 conversion later */

extern  netid_t  sockaddr2int_netid_t(struct sockaddr_in *);
extern  netid_t  ext2int_netid_t(const netid_t);
extern  netid_t  int2ext_netid_t(const netid_t);

extern  netid_t myhostid;
