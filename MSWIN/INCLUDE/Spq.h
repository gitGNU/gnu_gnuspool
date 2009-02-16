/* Spq.h -- spool queue format

   Copyright 2009 Free Software Foundation, Inc.

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

/*APISTART - beginning of section copied for API*/
struct	spq	{	/*  Entry in spool queue  */
	jobno_t		spq_job;	/*  Job number  */
	netid_t		spq_netid;	/*  Network id (local byte order) 0 local */
	netid_t		spq_orighost;	/*  Who it came from 0 if this machine */
	slotno_t  	spq_rslot;	/*  Slot number remote machine */
	time_t		spq_time;	/*  When submitted  */
#if	XITEXT_VN >= 22
	time_t		spq_proptime;	/*  Proposal (etc) time in cases messages go astray */
#endif
	time_t		spq_starttime;	/*  When (first) started */
	time_t		spq_hold;	/*  Hold until ... */
	unsigned  short	spq_nptimeout;	/*  Timeout if not printed (hours) */
	unsigned  short	spq_ptimeout;	/*  Timeout if printed (hours)  */
	long		spq_size;	/*  Size of job in bytes  */
	long		spq_posn;	/*  Position in job in bytes  */
	long		spq_pagec;	/*  Page count in job */

#if	XITEXT_VN < 22
	int_ugid_t	spq_uid; 	/*  Originating user (binary) */
	char		spq_uname[UIDSIZE+1];	/*  Originating user  */
	char		spq_puname[UIDSIZE+1];	/*  User to post output to  */
#else
	unsigned  long	spq_npages;	/*  Number of pages  */
#endif

	unsigned  char  spq_cps;	/*  Copies  */
	unsigned  char  spq_pri;	/*  Priority  */
#if	XITEXT_VN < 22
	classcode_t		spq_class;	/*  Class */
#endif
	short			spq_wpri;	/*  Working priority  */

	unsigned short	spq_jflags; /*  Job flags */
#define	SPQ_NOH		(1 << 0)	/*  Suppress heading  */
#define	SPQ_WRT		(1 << 1)	/*  Send message to users terminal  */
#define	SPQ_MAIL	(1 << 2)	/*  Mail message to user  */
#define	SPQ_RETN	(1 << 3)	/*  Retain in Q  */
#define	SPQ_ODDP	(1 << 4)	/*  Omit Odd pages */
#define	SPQ_EVENP	(1 << 5)	/*  Omit Even pages */
#define	SPQ_REVOE	(1 << 6)	/*  Reverse odd/even */
#define	SPQ_MATTN	(1 << 7)	/*  Mail attention message */
#define	SPQ_WATTN	(1 << 8)	/*  Write attention message */
#define	SPQ_LOCALONLY	(1 << 9)	/*  Local only */

	unsigned char	spq_sflags;	/*  Flags set by scheduler */
#define	SPQ_ASSIGN	(1 << 0)	/*  Assigned to printer  */
#define	SPQ_WARNED	(1 << 1)	/*  Warned about top of queue */
#define	SPQ_PROPOSED	(1 << 2)	/*  Proposed non-local print */
#define	SPQ_ABORTJ	(1 << 3)	/*  Set for job abort */

	unsigned char	spq_dflags;	/*  Flags set by spd */
#define	SPQ_PQ		(1 << 0)	/*  Being printed  */
#define	SPQ_PRINTED	(1 << 1)	/*  Printed it at least once */
#define	SPQ_STARTED	(1 << 2) 	/*  Job has been started sometime */
#define	SPQ_PAGEFILE	(1 << 3)/*  Has a page delimiter file */
#if	XITEXT_VN >= 23
#define	SPQ_ERRLIMIT	(1 << 4)/*  Error if exceeds upper bound */
#define	SPQ_PGLIMIT	(1 << 5)	/*  Limit is by pages */
#endif

#if	XITEXT_VN >= 22
	unsigned  short	spq_extrn;	/*  External job index 0=Xi-Text */
#if	XITEXT_VN < 23
	unsigned  short	spq_resvd; 	/*  Reserved to pad to 4 bytes*/
#else
	unsigned  short	spq_pglim;	/*  K byte limit or number of pages (enqueue only) */
#endif
	classcode_t		spq_class;
#endif

	slotno_t	spq_pslot;		/*  Printer slot if printing or -1
					    			also -1 if printing by unknown remote */
					    		
	unsigned  long	spq_start,
				    spq_end; 	/*  Record to start/finish at  */

#if	XITEXT_VN < 22
	unsigned  long	spq_npages;	/*  Number of pages  */
	unsigned  long	spq_haltat;	/*  Page number we were halted at */
#else
	unsigned  long	spq_haltat;	/*  Page number we were halted at */
	int_ugid_t		spq_uid; 	/*  Originating user (binary) */
	char			spq_uname[UIDSIZE+1];	/*  Originating user  */
	char			spq_puname[UIDSIZE+1];	/*  User to post output to  */
#endif	
	char  			spq_file[MAXTITLE+1];	/*  File name  */
	char  			spq_form[MAXFORM+1];	/*  Paper type  */
#if	XITEXT_VN < 23
	char  			spq_ptr[NAMESIZE+1];	/*  Printer type */
#else
	char  			spq_ptr[JPTRNAMESIZE+1];	/*  Printer type */
#endif
	char  			spq_flags[MAXFLAGS+1];	/*  Flags to use for filter */
#ifdef	__cplusplus
	char  *get_spq_uname() 	{  return  spq_uname;	}
	char  *get_spq_puname() 	{  return  spq_puname;	}
	char  *get_spq_file() 		{  return  spq_file; }
	char  *get_spq_form() 		{  return  spq_form; }
	char  *get_spq_ptr() 		{  return  spq_ptr; }
	char  *get_spq_flags() 	{  return  spq_flags; }
#ifndef	TRUE
#define	TRUE	1
#endif
	void	unqueue(const BOOL = TRUE);
	void	optcopy();
#endif
};

struct	spptr	{	/*  Details of printer  */
#if	XITEXT_VN >= 22
	netid_t		spp_netid;	/*  Network id 0 local */
	slotno_t  	spp_rslot;	/*  Slot number on remote machine */
	int_pid_t	spp_pid;	/*  Process id of spd process if applicable  */
#endif
	jobno_t		spp_job;	/*  Current job  */
	netid_t		spp_rjhostid; 	/*  Machine owning job being printed or 0 */
	slotno_t	spp_rjslot; 	/*  Slot on remote of job being printed or same as spp_jslot */
	slotno_t 	spp_jslot;	/*  Slot in Jlist on this machine or -1  */
	char		spp_state;	/*  Process state  */
	unsigned  char	spp_sflags; 	/*  Scheduler flags */
#define	SPP_SELECT	(1 << 0) 	/*  Selected for printing */
#define	SPP_INTER	(1 << 1) 	/*  Set for job interrupt */
#define	SPP_HEOJ	(1 << 2)	/*  Set for halt end of job */
#define	SPP_PROPOSED	(1 << 3)	/*  Proposed */
	unsigned  char	spp_dflags; 	/*  Flags set by spd */
#define	SPP_HADAB	(1 << 0)	/*  Had abort message  */
#define	SPP_REQALIGN	(1 << 1)	/*  Requires alignment */
	unsigned  char	spp_netflags; 	/*  Network printer flags */
#define	SPP_LOCALNET	(1 << 0)	/*  Access by network= stuff */
#define	SPP_LOCALONLY	(1 << 1)	/*  local printer only */

#if	XITEXT_VN < 22
	classcode_t		spp_class;		/*  Class  */
	unsigned  short	spp_resvd;		/*  Reserved */
	int_pid_t		spp_pid;		/*  Process id of spooler  */
	netid_t  		spp_netid;		/*  Network id (local byte order) 0 local */
	slotno_t  		spp_rslot;		/*  Slot number on remote machine */
#else
	classcode_t		spp_class;		/*  Class  */
#endif

	unsigned  long	spp_minsize; 	/*  Minimum size we'll accept */
	unsigned  long	spp_maxsize; 	/*  Maximum size we'll accept */

#if	XITEXT_VN >= 22
	unsigned  short	spp_extrn;		/*  External printer index 0=Xi-Text */
	unsigned  short	spp_resvd;		/*  Reserved */
#endif

	char	spp_dev[LINESIZE+1];	/*  Device  */
	char	spp_form[MAXFORM+1];	/*  Paper type  */
#if	XITEXT_VN < 23
	char	spp_ptr[NAMESIZE+1];	/*  Printer type  */
#else
	char	spp_ptr[PTRNAMESIZE+1];	/*  Printer type  */
#endif
#if	XITEXT_VN >= 20
	char	spp_feedback[PFEEDBACK+1]; /* Feedback from terminal server */
#endif
#if	XITEXT_VN >= 23
	char	spp_comment[COMMENTSIZE+1];/* Description of printer */
#endif
#ifdef	__cplusplus
	char	*get_spp_dev()		{  return  spp_dev;  }
	char	*get_spp_form()		{  return  spp_form; }
	char	*get_spp_ptr()		{  return  spp_ptr;  }
#if	XITEXT_VN >= 23
	char	*get_spp_comment() {  return  spp_comment;	}
#endif
#endif
};
/*APIEND - end of section copied for API */

/*
 *	Flags for scheduler to set on printers
 */

struct	spchrg	{				/*  Details of charge  */
	long		spc_chars;		/*  No chars  */
	long		spc_cpc;		/*  Charge per char  */
	int_ugid_t	spc_user;		/*  User (was UIDTYPE but we can't trust it) */
	unsigned  char	spc_pri;	/*  Priority  */
};

/*
 *	State codes for printer.
 */

#define	SPP_NULL	0	/*  Null entry  */
#define	SPP_OFFLINE	1	/*  Device is offline  */
#define	SPP_ERROR	2	/*  Some error  */
#define	SPP_HALT	3	/*  Halted  */
#define	SPP_INIT	4	/*  Initialising  */
#define	SPP_WAIT	5	/*  Idle  */
#define	SPP_SHUTD	6	/*  Shutdown */
#define	SPP_RUN		7	/*  Printing something  */
#define	SPP_OPER	8	/*  Awaiting operator  */

#define	SPP_PROC	SPP_INIT /* Process allocated from this state up */
#define	SPP_PREST	SPP_RUN	 /* Print details of job etc */
#define	SPP_NSTATES	9

/*
 *	Message type codes. Now only one type as they get sorted by the
 *	kernel into ascending order which causes confusion if several
 *	messages get sent in quick succession.
 */

#define	MT_SCHED	100

/*
 *	This + printer pid gives printer message type
 */

#define	MT_PMSG		1000000

/*
 *	Format of request packets.
 *	When sending internally "netid" has 0 in otherwise sender's
 *	netid.
 *	Jslot or pslot refers to the offset from the start of the jobs
 *	or printers vector (the slot number).
 *	Within one machine this is a fixed-ish quantity (for jobs we
 *	pass the job number as a double-check).
 *	In the network situation each job or printer is considered to
 *	"belong" to a machine and the slot number used is that on the machine
 *	it belongs to. However the code in sh_network.c is intended to find
 *	out the slot number of the reference copy of non-local jobs to invoke
 *	the scheduler routines with.
 *
 *	Nothing useful is held in the rslot or netid
 *	fields of struct spq or struct spptr within struct jmsg or pmsg.
 *	On the actual job queues or printer lists these fields provide the
 *	necessary information to access remote machines.
 */

struct	sp_jmsg  {
	unsigned  short	spr_act;	/*  Command  */
	unsigned  short	spr_seq;	/*  Sequence */
	int_pid_t	spr_pid;	/*  Originating process  */
	netid_t		spr_netid;	/*  Network id 0 if local */
	slotno_t	spr_jslot;	/*  Slot number in job shm */
	struct	spq	spr_q;
};

struct	sp_pmsg	{
	unsigned  short	spr_act;	/*  Command  */
	unsigned  short	spr_seq;	/*  Sequence */
	int_pid_t	spr_pid;		/*  Originating process  */
	netid_t		spr_netid; 		/*  Network id 0 if local */
	slotno_t	spr_pslot;		/*  Slot number in printer shm */
	struct	spptr	spr_p;
};

struct	sp_omsg	{
	unsigned  short	spr_act;	/*  Command  */
	unsigned  short	spr_seq;	/*  Sequence */
	int_pid_t	spr_pid;	/*  Originating process  */
	netid_t		spr_netid;	/*  Network id 0 if local */
	slotno_t	spr_jpslot; 	/*  Slot we're talking about */
	jobno_t		spr_jobno; 	/*  Job number for double-check */
	unsigned  long	spr_arg1;	/*  Argument  */
	unsigned  long	spr_arg2;	/*  Argument  */
};

struct	sp_cmsg	{
	unsigned  short	spr_act;	/*  Command  */
	unsigned  short	spr_seq;	/*  Sequence */
	slotno_t		spr_pslot;	/*  Slot in ptr shmem */
	int_pid_t		spr_pid;	/*  Originating process  */
#if	XITEXT_VN < 22
	unsigned  		spr_flags;	/*  Other stuff  */
#else
	netid_t			spr_netid;	/*  Network id 0 if local */
	unsigned long	spr_flags;	/*  Other stuff  */
#endif
	struct	spchrg	spr_c;
};

#if  	defined(NETWORK_VERSION) && defined(unix)
struct	sp_nmsg	{
	unsigned  short	spr_act;	/* Command */
	unsigned  short	spr_seq;	/* Sequence */
	int_pid_t	spr_pid;		/* Originating process */
	struct	remote	spr_n;		/* Network description */
};
#endif

#ifdef	unix
#ifdef	__cplusplus
struct	spr_req	{
	long	spr_mtype;		/*  IPC message type see above  */
	union	{
		sp_jmsg	j;
		sp_pmsg	p;
		sp_cmsg c;
		sp_omsg o;
#ifdef	NETWORK_VERSION
		sp_nmsg	n;
#endif
	};
};
#else
struct	spr_req	{
	long	spr_mtype;		/*  IPC message type see above  */
	union	{
		struct	sp_jmsg	j;
		struct	sp_pmsg	p;
		struct	sp_cmsg c;
		struct	sp_omsg o;
#ifdef	NETWORK_VERSION
		struct	sp_nmsg	n;
#endif
	}  spr_un;
};
#endif
#endif

#define	SPF_RETAIN	1	/*  Flag to say retain job  */
#define	SPF_NOCOPIES	2	/*  Flag to say no copies */

/*
 *	Command codes.
 *	SJ_ relate to jobs and use a struct sp_jmsg
 *	SP_ relate to printers and use a sp_pmsg
 *	SPD_ relate to spd processes and use a sp_cmsg
 *	SO_ relate to single arg commands and use a sp_omsg
 */

#define	SJ_ENQ		0	/*  spr->spshed enqueue job  */
#define	SJ_CHNG		1	/*  spq->spshed revise job  */
#define	SJ_CHANGEDJOB	2	/*  network broadcast changed */
#define	SJ_JUNASSIGN	3	/*  Order to unassign */
#define	SJ_JUNASSIGNED	4	/*  Job unassigned */

#define	SP_ADDP		5	/*  add printer */
#define	SP_CHGP		6	/*  change printer */
#define	SP_CHANGEDPTR	7	/*  network broadcast changed */
#define	SP_PUNASSIGNED	8	/*  Printer unassigned */

#define	SP_REQ		9	/*  spshed->spd request  */
#define	SP_FIN		10	/*  spshed->spd terminate */
#define	SP_PAB		11	/*  spshed->spd aborted job a/w oper */
#define	SP_PYES		12	/*  spshed->spd operator continue  */
#define	SP_PNO		13	/*  spshed->spd operator retry  */
#define	SP_REMAP	14	/*  spshed->spd tell it world has changed */
#define	SPD_DONE	15	/*  spd->spshed completed */
#define	SPD_DAB		16	/*  spd->spshed done abort */
#define	SPD_DERR	17	/*  spd->spshed done with error */
#define	SPD_DFIN	18	/*  spd->spshed finished  */
#define	SPD_SCH		19	/*  spd->spshed state change  */
#define	SPD_CHARGE	20	/*  charge record */

#define	SO_AB		21	/*  spq->spshed abort job  */
#define	SO_ABNN		22	/*  spq->spshed abort job no notify  */
#define	SO_DEQUEUED	23	/*  network broadcast delete */
#define	SO_MON		24	/*  spq->spshed	login  */
#define	SO_DMON		25	/*  spq->spshed logout  */
#define	SO_RSP		26	/*  restart printer */
#define	SO_PHLT		27	/*  halt printer at eoj */
#define	SO_PSTP		28	/*  halt printer at once */
#define	SO_PGO		29	/*  start printer */
#define	SO_DELP		30	/*  delete printer */
#define	SO_SSTOP 	31	/*  something->spshed graceful end */
#define	SO_OYES		32	/*  spq->spshed operator continue  */
#define	SO_ONO		33	/*  spq->spshed operator retry  */
#define	SO_INTER 	34	/*  interrupt printer  */
#define	SO_PJAB		35	/*  abort job on printer */

#define	SO_NOTIFY	36	/*  notify remote */
#define	SO_PNOTIFY	37	/*  notify remote past tense */
#define	SO_PROPOSE	38	/*  propose remote print */
#define	SO_PROP_OK	39	/*  ok to print */
#define	SO_PROP_NOK	40	/*  Not ok someone else printing */
#define	SO_PROP_DEL	41	/*  Treat as deleted */
#define	SO_ASSIGN	42	/*  Generalised assign */
#define	SO_LOCASSIGN	43	/*  Printer assigned to local job */

#define	SN_NEWHOST	44	/*  netmonitor->spshed new host noticed */
#define	SN_SHUTHOST	45	/*  Shutdown gracefully */
#define	SN_ABORTHOST	46	/*  Shutdown ungracefully */
#define	SN_DELERR	47	/*  delete error file */
#define	SN_REQSYNC	48	/*  request existing machine to send details */
#define	SN_ENDSYNC	49	/*  define end of list of details */
#define	SN_TICKLE	50	/*  tell other end we're still here */

#define	SON_CONNECT	51	/*  User request to connect remote if poss */
#define	SON_DISCONNECT	52	/*  User request to disconnect remote forcibly */
#define	SON_CONNOK	53	/*  Other end reply to say connect ok */
#define	SON_XTNATT	54	/*  Register process for monitoring DOS clients */

#define	SOU_PWCHANGED	55	/*  spuser->spshed password changed */

/*
 *	Signals to do various things.
 */

#define	DAEMSTOP	SIGTERM
#define	NETSHUTSIG	SIGHUP
#define	DAEMRST		SIGUSR2
#define	QRFRESH		SIGUSR2
