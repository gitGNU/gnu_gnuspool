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

struct	pend_job	{
	netid_t   clientfrom;		/* Who is responsible (or 0) */
	FILE	*out_f;			/* Output file */
	char	*delim;			/* Delimiter if appropriate */
	char	tmpfl[NAMESIZE+1];	/* Temporary file */
	char	pgfl[NAMESIZE+1];	/* Page file */
	unsigned  char	prodsent;	/* Sent a prod */
	USHORT		penddelim;	/* Number of pending delimiter bytes */
	USHORT		timeout;	/* Timeout value to decide when to enquire death */
	time_t	lastaction;		/* Last message received (for timeout) */
	jobno_t	jobn;			/* Job number we are creating */
	struct	spq	jobout;		/* Job details pending */
	struct	pages	pageout;	/* Page file details if appropriate */
};

/* Structure used to hash host ids */

struct	hhash	{
	struct	hhash	*hn_next;	/* Next in hash chain */
	struct	remote	rem;		/* Remote structure */
	char		*dosname;	/* DOS user name (W95 user name if "roaming") */
	char		*actname;	/* Actual name (W95 user name if not "roaming") */
	USHORT		timeout;	/* Timeout value for seeing if it is asleep */
	USHORT		flags;		/* Status flags same as UAL_OK etc  */
	time_t		lastaction;	/* Last action for timeout */
};

/* Structure used to hash client user names */

struct	cluhash  {
	struct	cluhash	*next;		/* Next in hash chain */
	struct	cluhash	*alias_next;	/* Next in alias hash chain */
	unsigned	refcnt;		/* Reference count whilst deallocating */
	char		*machname;	/* Machine name "opt" */
	struct	remote	rem;		/* NB not a "*" */
};

#ifndef	_NFILE
#define	_NFILE	64
#endif

#define	MAX_PEND_JOBS	(_NFILE / 3)

#define	MAXTRIES	5
#define	TRYTIME		20

/* It seems to take more than one attempt to set up a UDP port at times, so....  */

#define	UDP_TRIES	3

#define	JN_INC	80000		/*  Add this to job no if clashes */
#define	JOB_MOD	60000		/*  Modulus of job numbers */

extern	netid_t	myhostid, localhostid;
extern	SHORT	qsock, uasock, apirsock;
extern	SHORT	qportnum, uaportnum, apirport, apipport;
extern	SHORT	tcpproto, udpproto;
#ifndef	USING_FLOCK
extern	struct	sembuf	jr[], ju[], pr[], pu[];
#endif
extern	struct	spr_req	sp_req;
extern	struct	pend_job	pend_list[];
extern	int	had_alarm, hadrfresh;
extern	struct	hhash	*nhashtab[];
extern	struct	cluhash	*cluhashtab[];

extern	unsigned tracing;
extern	FILE	*tracefile;

extern void	trace_op(const int_ugid_t, const char *);
extern void	trace_op_res(const int_ugid_t, const char *, const char *);
extern void	client_trace_op(const netid_t, const char *);
extern void	client_trace_op_name(const netid_t, const char *, const char *);

#define	TRACE_SYSOP		(1 << 0)
#define	TRACE_APICONN		(1 << 1)
#define	TRACE_APIOPSTART	(1 << 2)
#define	TRACE_APIOPEND		(1 << 3)
#define	TRACE_CLICONN		(1 << 4)
#define	TRACE_CLIOPSTART	(1 << 5)
#define	TRACE_CLIOPEND		(1 << 6)

extern void	abort_job(struct pend_job *);
extern void	convert_username(struct hhash *, struct spq *);
extern void	process_api(void);
extern void	process_ua(void);
extern void	unpack_job(struct spq *, struct spq *);
extern void	send_askall(void);
extern void	tell_friends(struct hhash *);

extern int	scan_job(struct pend_job *);
extern int	tcp_serv_accept(const int, netid_t *);
extern int	validate_job(struct spq *);
extern int	checkpw(const char *, const char *);

extern unsigned	process_alarm(void);

extern unsigned	calc_clu_hash(const char *);
extern struct	hhash *	find_remote(const netid_t);
extern struct	pend_job *	add_pend(const netid_t);
extern struct pend_job *	find_j_by_jno(const jobno_t);
extern FILE *goutfile(jobno_t *, char *, char *, const int);

extern struct cluhash *update_roam_name(struct hhash *, const char *);
extern struct cluhash *new_roam_name(const netid_t, struct hhash **, const char *);
extern int	update_nonroam_name(struct hhash *, const char *);
