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

#ifdef	NETWORK_VERSION
struct	remote	{
	/* NB we assume in various places that HOSTNSIZE > UIDSIZE */
	char	hostname[HOSTNSIZE];	/* Actual host name (alternatively user name) */
	char	alias[HOSTNSIZE];	/* Alias for within GNUspool */
	SHORT	sockfd;			/* Socket fd to talk to it */
	netid_t hostid;			/* Host id in network byte order */
	unsigned char	is_sync;	/* sync flags */
#define	NSYNC_NONE	0		/* Not done yet */
#define	NSYNC_REQ	1		/* Requested but not complete */
#define	NSYNC_OK	2		/* Completed */
	unsigned  char	ht_flags;	/* Host-type flags */
#define	HT_ISCLIENT	(1 << 0)	/* Set to indicate "I" am client */
#define	HT_PROBEFIRST	(1 << 1)	/* Probe connection first */
#define	HT_MANUAL	(1 << 2)	/* Manual connection only */
#define	HT_DOS		(1 << 3)	/* DOS or external client system */
#define	HT_PWCHECK	(1 << 4)	/* Check password of user */
#define HT_ROAMUSER	(1 << 5)	/* Roaming user */
#define	HT_TRUSTED	(1 << 6)	/* Trusted host */
	USHORT	ht_timeout;		/* Timeout value (seconds) */
	time_t	lastwrite;		/* When last done */
	USHORT	ht_seqto;		/* Sequence TO */
	USHORT	ht_seqfrom;		/* Sequence From */
};

struct	feeder	{
	char	fdtype;			/* Type of file required */
	char	resvd[3];		/* Pad out to 4 bytes */
	slotno_t  jobslot;		/* Job slot net byte order */
	jobno_t	 jobno;			/* Jobnumber net byte order */
};

/* General messages */

struct	netmsg	{
	USHORT		code;	/* Code number */
	USHORT		seq;	/* Sequence */
	netid_t		hostid;	/* Host id net byte order */
	LONG		arg;	/* Argument */
};
struct	nihdr	{		/* Incoming messages, see what in a minute */
	USHORT		code;	/* Code number */
	USHORT		seq;	/* Sequence */
};

/* Size of hash table used in various places */

#define	NETHASHMOD	53		/* Not very big prime number */

/* Fields in /etc/gnuspool-hosts lines */

#define	HOSTF_HNAME	0
#define	HOSTF_ALIAS	1
#define	HOSTF_FLAGS	2
#define	HOSTF_TIMEOUT	3
#endif

/* Define these here to save extensive ifdefs */

#define	FEED_SP		0	/* Feed spool file */
#define	FEED_NPSP	1	/* Feed spool file, don't bother with pages */
#define	FEED_ER		2	/* Feed error file */
#define	FEED_PF		3	/* Feed page file */

extern  void  end_hostfile();
netid_t look_hostname(const char *);
char 	*look_host(const netid_t);
extern  struct remote *  get_hostfile();
extern  void  hash_hostfile();

extern	netid_t	myhostid;
