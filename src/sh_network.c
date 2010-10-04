/* sh_network.c -- spshed net monitor process

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
#ifdef	NETWORK_VERSION
#include "incl_sig.h"
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#include <sys/ipc.h>
#ifdef	OS_LINUX
#define	__USE_GNU	1
#endif
#include <sys/msg.h>
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include "errnums.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "spq.h"
#define	UCONST
#include "q_shm.h"
#include "xfershm.h"
#include "files.h"
#include "incl_unix.h"
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "services.h"

/* It seems to take more than one attempt to set up a UDP port at
   times, so....  */

#define	UDP_TRIES	3

#ifdef	BUGGY_SIGCLD
extern	int	nchild;
#endif

SHORT	listsock,
	viewsock,
	probesock;
USHORT	lportnum,		/* These are all htons-ified */
	vportnum,
	pportnum;
int	Netsync_req;

static	unsigned	lumpsize = DEF_LUMPSIZE,
			lumpwait = DEF_LUMPWAIT,
			closedelay = DEF_CLOSEDELAY;

static	SHORT	tcpproto, udpproto;

#define	INC_REMOTES	4

struct	rem_list  {
	int	rl_nums,		/* Number on list */
		rl_max;			/* Number allocated */
	struct	remote	**list;
};

struct	rem_list	possible,
			probed,
			connected,
			roamers;

enum	hip_type  {	HIP_POSS,		/* Possible */
			HIP_REMOTE,		/* Remote we are actually talking to */
			HIP_PROBE,		/* Remote we are "probing" */
			HIP_ROAM };		/* On roam list */

struct	haship  {
	struct	haship	*next;		/* Next in hash chain */
	struct	remote	*remp;		/* Remote structure */
	enum  hip_type	type;		/* Type of thing */
};

static	struct	haship	*hashtab[NETHASHMOD];

PIDTYPE	Netm_pid;	/* Process id of net monitor */

extern	int	Ctrl_chan;

extern	unsigned  calcnhash(const netid_t);
extern  void  check_jmoved();
extern  void  check_pmoved();
extern  void  feed_req();
extern	void  job_pack(struct spq *, struct spq *);
extern	void  net_jclear(const netid_t);
extern	void  net_pclear(const netid_t);
extern	void  ptr_pack(struct spptr *, struct spptr *);
extern	void  nfreport(const int);
extern	void  report(const int);
extern	void  unpack_job(struct spq *, struct spq *);
extern	void  unpack_ptr(struct spptr *, struct spptr *);
extern	slotno_t  find_pslot(const netid_t, const slotno_t);
extern	Hashspq   *ver_job(const slotno_t, const jobno_t);
extern	Hashspptr *ver_ptr(const slotno_t);
extern	Hashspptr *ver_remptr(const slotno_t, const netid_t);

/* Allocate a remote structure and copy.  */

static struct remote *new_remote(const struct remote *rp)
{
	struct	remote  *result;

	if  (!(result = (struct remote *) malloc(sizeof(struct remote))))
		nomem();

	*result = *rp;
	return  result;
}

/* Allocate a new hash structure.  */

static struct haship *alloc_haship(struct remote *rp, enum hip_type type)
{
	struct haship *result = (struct haship *) malloc(sizeof(struct haship));
	if  (!result)
		nomem();
	result->next = (struct haship *) 0;
	result->remp = rp;
	result->type = type;
	return  result;
}

/* Return the place which points to where a hash structure is or would go */

static  struct  haship **get_pp_haship(const netid_t ipaddr)
{
	struct	haship	**rpp, *rp;

	for  (rpp = &hashtab[calcnhash(ipaddr)];  (rp = *rpp);  rpp = &rp->next)
		if  (rp->remp->hostid == ipaddr)
			break;

	/* Even if we don't find it, we return where it 'would' go.  */

	return  rpp;
}

/* Allocate and free members of various lists.  */

static  void  add_remlist(struct rem_list *rl, struct remote *rp)
{
	if  (rl->rl_nums >= rl->rl_max)  {
		rl->rl_max += INC_REMOTES;
		if  (rl->list)
			rl->list = (struct remote **) realloc((char *) rl->list, (unsigned)(sizeof(struct remote *) * rl->rl_max));
		else
			rl->list = (struct remote **) malloc((unsigned)(sizeof(struct remote *) * rl->rl_max));
		if  (!rl->list)
			nomem();
	}
	rl->list[rl->rl_nums++] = rp;
}

static void	free_remlist(struct rem_list *rl, struct remote *rp, const int errcode)
{
	int	cnt;

	for  (cnt = rl->rl_nums - 1;  cnt >= 0;  cnt--)
		if  (rl->list[cnt] == rp)  {
			if  (--rl->rl_nums != cnt)
				rl->list[cnt] = rl->list[rl->rl_nums];
			return;
		}

	if  (errcode != 0)  {
		disp_str = rp->hostname;
		nfreport(errcode);
	}
}

static void  push_msg(struct msgbuf *msg, const unsigned mlng)
{
	for  (;;)  {
		do  if  (msgsnd(Ctrl_chan, msg, mlng, 0) >= 0)
			return;
		while  (errno == EINTR);
		report($E{Network IPC send fail});
	}
}

static void  push_jmsg(struct spr_req *req, struct spq *jp)
{
	int	ret;

	for  (;;)  {
		if  ((ret = wjmsg(req, jp)) == 0)
			return;
		if  (ret != $E{IPC msg q full})
			report(ret);
	}
}

static void  push_pmsg(struct spr_req *req, struct spptr *pp)
{
	int	ret;

	for  (;;)  {
		if  ((ret = wpmsg(req, pp)) == 0)
			return;
		if  (ret != $E{IPC msg q full})
			report(ret);
	}
}

struct remote *alloc_roam(const netid_t roamip, const char *u_name)
{
	struct	remote	*rp;
	struct	haship	**hipp, *hip;
	struct	remote	buf;

	/* Might be renaming of existing person.  The existing person
	   might still have a connection on the go - probably
	   invalid though.  */

	hipp = get_pp_haship(roamip);
	if  ((hip = *hipp))  {
		if  (hip->remp->ht_flags & HT_ROAMUSER)
			strcpy(hip->remp->hostname, u_name);
		return  hip->remp;
	}

	/* New person...  */

	BLOCK_ZERO(&buf, sizeof(buf));
	strcpy(buf.hostname, u_name);
	buf.sockfd = -1;
	buf.hostid = roamip;
	buf.ht_flags = HT_DOS|HT_ROAMUSER;
	rp = new_remote(&buf);
	*hipp = alloc_haship(rp, HIP_ROAM);
	add_remlist(&roamers, rp);
	return  rp;
}

static struct remote *look4(const netid_t netid, const enum hip_type type)
{
	struct	haship  **hipp = get_pp_haship(netid);
	struct	haship	*hip = *hipp;
	return  (hip  &&  hip->type == type)? hip->remp: (struct remote *)  0;
}

struct	remote *find_connected(const netid_t netid)
{
	return  look4(netid, HIP_REMOTE);
}

struct remote *find_probe(const netid_t netid)
{
	return  look4(netid, HIP_PROBE);
}

/* Inline versions for this module */

inline struct remote *inl_find_connected(const netid_t netid)
{
	return  look4(netid, HIP_REMOTE);
}

inline struct remote *inl_find_probe(const netid_t netid)
{
	return  look4(netid, HIP_PROBE);
}

/* Reassign contents of remote structure on connection.  */

static struct remote *reass_hip(struct haship *hip)
{
	struct	remote	*rp = hip->remp;

	switch  (hip->type)  {
	case  HIP_POSS:			/* Possible now "realised" */
		free_remlist(&possible, rp, $E{Hash function error free_poss});
		add_remlist(&connected, rp);
		hip->type = HIP_REMOTE;
		break;

	case  HIP_ROAM:
		free_remlist(&roamers, rp, 0);
		add_remlist(&connected, rp);
		hip->type = HIP_REMOTE;
		break;

	case  HIP_PROBE:
		free_remlist(&probed, rp, $E{Hash function error free_probe});
		add_remlist(&connected, rp);
		hip->type = HIP_REMOTE;
		break;

	case  HIP_REMOTE:
		if  (rp->ht_flags & HT_ROAMUSER)  {
			struct	hostent	*hp;
			struct	in_addr	sina;
			sina.s_addr = rp->hostid;
			hp = gethostbyaddr((char *) &rp->hostid, sizeof(netid_t), AF_INET);
			disp_str = hp? hp->h_name: inet_ntoa(sina);
		}
		else
			disp_str = rp->hostname;
		nfreport($E{Reconnection whilst still connected});
		close(rp->sockfd);
		if  (rp->is_sync != NSYNC_OK)
			Netsync_req--;
		break;
	}

	return  rp;
}

/* Try to attach to remote machine which may already be running.  */

void  conn_attach(struct remote *prp)
{
	int	sk;
	struct	remote	*rp;
	struct	haship	**hipp, *hip;
	struct	sockaddr_in	sin;
	if  ((sk = socket(PF_INET, SOCK_STREAM, tcpproto)) < 0)
		return;

	sin.sin_family = AF_INET;
	sin.sin_port = lportnum;
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	BLOCK_COPY(&sin.sin_addr, &prp->hostid, sizeof(netid_t));

	if  (connect(sk, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(sk);
		return;
	}

	/* Successful connection...
	   Now deal with what we knew before */

	hipp = get_pp_haship(prp->hostid);

	if  ((hip = *hipp))			/* Heard of it before */
		rp = reass_hip(hip);
	else  {
		/*   New case (from attach_hosts). */
		rp = new_remote(prp);
		*hipp = alloc_haship(rp, HIP_REMOTE);
		add_remlist(&connected, rp);
	}

	/* Now set fields in remote structure.  */

	rp->sockfd = (SHORT) sk;
	rp->ht_flags |= HT_ISCLIENT;
	rp->is_sync = NSYNC_NONE;
	rp->lastwrite = time((time_t *) 0);
	rp->ht_seqto = rp->ht_seqfrom = 0;
	Netsync_req++;
}

static	int  probe_send(const netid_t hostid, struct netmsg *pmsg)
{
	int	sockfd, tries;
	struct	sockaddr_in	serv_addr, cli_addr;

	BLOCK_ZERO(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = hostid;
	serv_addr.sin_port = pportnum;

	BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cli_addr.sin_port = 0;

	/* We don't really need the cli_addr but we are obliged to bind something.
	   The remote uses our "pportnum".  */

	for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
		if  ((sockfd = socket(AF_INET, SOCK_DGRAM, udpproto)) < 0)  {
			disp_arg[0] = ntohs(pportnum);
			nfreport($E{Cannot create probe socket});
			return  0;
		}
		if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
			disp_arg[0] = ntohs(pportnum);
			nfreport($E{Cannot bind probe socket});
			close(sockfd);
			return  0;
		}
		if  (sendto(sockfd, (char *) pmsg, sizeof(struct netmsg), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0)
			goto  doneok;
		close(sockfd);
	}

	/* Failed....  */

	disp_arg[0] = ntohs(pportnum);
	nfreport($E{Cannot send probe socket});
	return  0;
 doneok:
	close(sockfd);
	return  1;
}

/* Initiate connection by doing UDP probe first.
   The net monitor process deals with the reply, or "nettickle" discovers that
   it's not worth bothering about.  */

void  probe_attach(struct remote *prp)
{
	struct	netmsg	pmsg;

	pmsg.code = htons(SON_CONNECT);
	pmsg.seq = 0;
	pmsg.hostid = myhostid;
	pmsg.arg = 0;

	if  (probe_send(prp->hostid, &pmsg))  {
		struct	haship	**hipp, *hip;
		struct	remote	*rp;

		hipp = get_pp_haship(prp->hostid);

		if  ((hip = *hipp))  {
			rp = hip->remp;

			switch  (hip->type)  {
			case  HIP_REMOTE:
			case  HIP_PROBE:
				rp->lastwrite = time((time_t *) 0);
				return;

			case  HIP_POSS:
				free_remlist(&possible, rp, $E{Hash function error free_poss});
				add_remlist(&probed, rp);
				hip->type = HIP_PROBE;
				break;

			case  HIP_ROAM:
				free_remlist(&roamers, rp, 0);
				add_remlist(&probed, rp);
				hip->type = HIP_PROBE;
				break;
			}
		}
		else  {
			rp = new_remote(prp);
			*hipp = alloc_haship(rp, HIP_PROBE);
			add_remlist(&probed, rp);
		}

		rp->is_sync = NSYNC_NONE;
		rp->ht_flags |= HT_ISCLIENT;
		rp->lastwrite = time((time_t *) 0);
	}
}

void  reply_probe()
{
	netid_t		whofrom;
	struct	remote	*rp;
	struct	netmsg	pmsg;
	SOCKLEN_T		repl = sizeof(struct sockaddr_in);
	struct	sockaddr_in	reply_addr;
	if  (recvfrom(probesock, (char *) &pmsg, sizeof(pmsg), 0, (struct sockaddr *) &reply_addr, &repl) < 0)
		return;

	if  ((whofrom = pmsg.hostid) == 0L  ||  whofrom == myhostid)
		return;

	switch  (ntohs(pmsg.code))  {
	default:
		return;		/* Forget it */

	case  SON_CONNECT:

		/* Probe connect - just bounce it back We can't check
		   the validity of the hosts here because this is
		   run by netmonitor process which might not know
		   all the facts.  */

		pmsg.code = htons(SON_CONNOK);
		pmsg.seq = 0;
		pmsg.hostid = myhostid;
		pmsg.arg = 0;
		probe_send(whofrom, &pmsg);
		return;

	case  SON_CONNOK:

		/* Connection ok - forget it if we weren't interested
		   in that processor (possibly because it's ancient).
		   Otherwise we send a message to the scheduler
		   process proper and exit to be regenerated.  */

		if  ((rp = inl_find_probe(whofrom)))  {
			struct	spr_req	nmsg;
			nmsg.spr_mtype = MT_SCHED;
			nmsg.spr_un.n.spr_act = SON_CONNOK;
			nmsg.spr_un.n.spr_seq = 0;
			nmsg.spr_un.n.spr_pid = Netm_pid;
			nmsg.spr_un.n.spr_n = *rp;
			push_msg((struct msgbuf *) &nmsg, sizeof(struct sp_nmsg));
			exit(0);
		}
		return;
	}
}

/* Attach remote, either immediately, or by doing probe operation
   first Return 1 if we got through immediately, 0 otherwise.  */

int  rattach(struct remote *prp)
{
	if  (prp->hostid == 0L  ||  prp->hostid == myhostid)
		return  0;
	if  (prp->ht_flags & HT_PROBEFIRST)  {
		probe_attach(prp);
		return  0;
	}
	else  {
		conn_attach(prp);
		return  1;
	}
}

/* Accept connection from new machine */

void  newhost()
{
	int	newsock;
	struct	remote	*rp;
	struct	haship	**hipp, *hip;
	struct	sockaddr_in	sin;
	SOCKLEN_T	sinl;

	sinl = sizeof(sin);
	if  ((newsock = accept(listsock, (struct sockaddr *) &sin, &sinl)) < 0)
		return;

	hipp = get_pp_haship((netid_t) sin.sin_addr.s_addr);

	if  (!(hip = *hipp))  {		/* Unknown host */
		struct	netmsg		rq;
		struct  hostent  *hp;
		rq.code = htons(SN_SHUTHOST);
		rq.hostid = myhostid;
		rq.arg = 0;
		Ignored_error = write(newsock, (char *) &rq, sizeof(rq));
		hp = gethostbyaddr((char *) &sin.sin_addr.s_addr, sizeof(netid_t), AF_INET);
		disp_str = hp? hp->h_name: inet_ntoa(sin.sin_addr);
		nfreport($E{Connection attempt from unknown host});
		close(newsock);
		return;
	}

	/* The client is responsible for broadcasting job and printer
	   details, so we'll say it's sync'ed.  It can ask for my
	   stuff when it's ready.  */

	rp = reass_hip(hip);
	rp->is_sync = NSYNC_OK;
	rp->ht_flags &= ~HT_ISCLIENT;
	rp->sockfd = (SHORT) newsock;
	rp->lastwrite = time((time_t *) 0);
	rp->ht_seqto = rp->ht_seqfrom = 0;
}

/* The following are service names so that we can find a suitable port number.
   WARNING: We must run as root if we want to use privileged port numbers < 1024.  */

static	char	*servnames[] = {
	CONNECTPORT_NAME1,
	CONNECTPORT_NAME2,
	CONNECTPORT_NAME3,
	CONNECTPORT_NAME4
};
static	char	*vservnames[] = {
	VIEWPORT_NAME1,
	VIEWPORT_NAME2
};

/* Attach hosts if possible */

void  attach_hosts()
{
	struct	servent	*sp;
	struct	protoent  *pp;
	struct	remote	*rp;
	char	*tcp_protoname, *udp_protoname, *ep;
	int	si;
	extern	char	hostf_errors;

	if  ((ep = envprocess(LUMPSIZE))  &&  (lumpsize = (unsigned) atoi(ep)) == 0)
		lumpsize = DEF_LUMPSIZE;
	if  ((ep = envprocess(LUMPWAIT))  &&  (lumpwait = (unsigned) atoi(ep)) == 0)
		lumpwait = DEF_LUMPWAIT;
	if  ((ep = envprocess(CLOSEDELAY))  &&  (closedelay = (unsigned) atoi(ep)) == 0)
		closedelay = DEF_CLOSEDELAY;

	/* Get TCP/UDP protocol names */

	if  (!((pp = getprotobyname("tcp"))  || (pp = getprotobyname("TCP"))))
		return;
	tcp_protoname = stracpy(pp->p_name);
	tcpproto = pp->p_proto;
	if  (!((pp = getprotobyname("udp"))  || (pp = getprotobyname("UDP"))))  {
		free(tcp_protoname);
		return;
	}
	udp_protoname = stracpy(pp->p_name);
	udpproto = pp->p_proto;
	endprotoent();

	/* Get port number for this caper */

	for  (si = 0;  si < sizeof(servnames)/sizeof(char *);  si++)
		if  ((sp = getservbyname(servnames[si], tcp_protoname)))
			goto  found_serv;
	free(tcp_protoname);
	free(udp_protoname);
	endservent();
	return;

 found_serv:

	/* Shhhhhh....  I know this should be network byte order, but
	   lets leave it alone for now.  */

	lportnum = pportnum = sp->s_port;

	/* Get port number for probe port, if not found use the same as above.  */

	for  (si = 0;  si < sizeof(servnames)/sizeof(char *);  si++)
		if  ((sp = getservbyname(servnames[si], udp_protoname)))  {
			pportnum = sp->s_port;
			break;
		}

	for  (si = 0;  si < sizeof(vservnames)/sizeof(char *);  si++)
		if  ((sp = getservbyname(vservnames[si], tcp_protoname)))  {
			vportnum = sp->s_port;
			goto  found_vserv;
		}

	/* Do this in 2 steps as some ntohs etc are asm calls...
	   (including the one I first tried it on) */

	vportnum = ntohs((USHORT) lportnum) + 1;
	vportnum = htons(vportnum);
 found_vserv:
	free(tcp_protoname);
	free(udp_protoname);
	endservent();

	/* Now set up "listening" socket */

	if  ((listsock = socket(PF_INET, SOCK_STREAM, tcpproto)) >= 0)  {
		struct	sockaddr_in	sin;
#ifdef	SO_REUSEADDR
		int	on = 1;
		setsockopt(listsock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
		sin.sin_family = AF_INET;
		sin.sin_port = lportnum;
		BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
		sin.sin_addr.s_addr = INADDR_ANY;
		if  (bind(listsock, (struct sockaddr *) &sin, sizeof(sin)) < 0  ||  listen(listsock, 5) < 0)  {
			close(listsock);
			listsock = -1;
			disp_arg[0] = ntohs(lportnum);
			report($E{Network connect socket error});
		}
	}

	/* Now set up "viewing/feeding" socket */

	if  ((viewsock = socket(PF_INET, SOCK_STREAM, tcpproto)) >= 0)  {
		struct	sockaddr_in	sin;
#ifdef	SO_REUSEADDR
		int	on = 1;
		setsockopt(viewsock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
		sin.sin_family = AF_INET;
		sin.sin_port = vportnum;
		BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
		sin.sin_addr.s_addr = INADDR_ANY;
		if  (bind(viewsock, (struct sockaddr *) &sin, sizeof(sin)) < 0  ||  listen(viewsock, 5) < 0)  {
			close(viewsock);
			viewsock = -1;
			disp_arg[0] = ntohs(vportnum);
			report($E{Network feeder socket error});
		}
	}

	/* Now set up Datagram probe socket */

	if  ((probesock = socket(PF_INET, SOCK_DGRAM, udpproto)) >= 0)  {
		struct	sockaddr_in	sin;

		sin.sin_family = AF_INET;
		sin.sin_port = pportnum;
		BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
		sin.sin_addr.s_addr = INADDR_ANY;
		if  (bind(probesock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
			close(probesock);
			probesock = -1;
			disp_arg[0] = ntohs(pportnum);
			report($E{Network probe socket error});
		}
	}

	/* See comment about this line in createjfile().
	   We make it work whatever order this routine and createjfile() are
	   called in.  */

	Job_seg.dptr->js_viewport = vportnum;

	/* Now parse host file and attach as appropriate */

	while  ((rp = get_hostfile()))  {

		/* Ignore "roaming user" entries - xtnetserv tells us
		   about them and they find their way into
		   alloc_roam().  */

		if  (rp->ht_flags & HT_ROAMUSER)
			continue;

		if  (rp->ht_flags & (HT_MANUAL|HT_DOS))  {
			struct	remote  *newrp = new_remote(rp);
			struct	haship	**hipp = get_pp_haship(rp->hostid);
			if  (!*hipp)	/* Otherwise I'm really confused */
				*hipp = alloc_haship(newrp, HIP_POSS);
			add_remlist(&possible, newrp);
			continue;
		}

		/* Otherwise set it up as a new connection.  */

		rattach(rp);
	}
	end_hostfile();
	if  (hostf_errors)
		nfreport($E{Warn errors in host file});
}

/* Remove traces of jobs and printers on our machine associated with
   the dying machine */

static	void  deallochost(struct remote *rp)
{
	struct	haship	**hipp, *hip;

	net_jclear(rp->hostid);
	net_pclear(rp->hostid);

	hipp = get_pp_haship(rp->hostid);

	if  ((hip = *hipp))  {

		switch  (hip->type)  {
		case  HIP_POSS:
		case  HIP_ROAM:
			return;

		case  HIP_PROBE:
			free_remlist(&probed, rp, $E{Hash function error free_probe});
			goto  deallrest;

		case  HIP_REMOTE:
			close(rp->sockfd);
			if  (rp->is_sync != NSYNC_OK)
				Netsync_req--;
			free_remlist(&connected, rp, 0);
		deallrest:
			if  (rp->ht_flags & HT_ROAMUSER)  {
				hip->type = HIP_ROAM;
				add_remlist(&roamers, rp);
			}
			else  {
				hip->type = HIP_POSS;
				add_remlist(&possible, rp);
			}
		}
	}
}

/* Write to socket, but if we get some error, treat connection as
   down.  Return 0 in that case so we can break out of loops.  */

static	int  chk_write(struct remote *rp, char *buffer, unsigned length)
{
	int	nbytes;
	while  (length != 0)  {
		if  ((nbytes = write(rp->sockfd, buffer, length)) < 0)  {
			/* If write failed, we kill off network monitor process. */
			kill(Netm_pid, NETSHUTSIG);
			deallochost(rp);
			return  0;
		}
		buffer += nbytes;
		length -= nbytes;
	}
	rp->lastwrite = time((time_t *) 0);
	rp->ht_seqto++;
	return  1;
}

void  clearhost(const netid_t netid)
{
	struct	haship	**hipp = get_pp_haship(netid);
	struct	haship  *hip = *hipp;
	if  (hip)
		deallochost(hip->remp);
}

/* Send a random sort of message to a host.
   Currently this is used only for "delete job error file".  */

void  net_xmit(const netid_t netid, const int code, const LONG arg)
{
	struct	remote		*rp;

	if  ((rp = inl_find_connected(netid)))  {
		struct	netmsg		rq;
		rq.code = htons((USHORT) code);
		rq.seq = htons(rp->ht_seqto);
		rq.hostid = myhostid;
		rq.arg = htonl((ULONG) arg);
		chk_write(rp, (char *) &rq, sizeof(rq));
	}
}

/* Read from TCP socket and join together the bits which things
   sometimes get split into.  */

static	void  read_sock(struct remote *rp, char *rqb, unsigned size)
{
	int	nbytes;
	struct	spr_req	msg;

	for  (;;)  {
		while  ((nbytes = read(rp->sockfd, rqb, size)) > 0)  {
			if  (nbytes == size)
				return;
			size -= nbytes;
			rqb += nbytes;
		}
		if  (errno != EINTR)
			break;
	}
	disp_str = look_host(rp->hostid);
	nfreport($E{Truncated TCP message});
	msg.spr_mtype = MT_SCHED;
	msg.spr_un.o.spr_act = SN_SHUTHOST;
	msg.spr_un.o.spr_netid = rp->hostid;
	push_msg((struct msgbuf *) &msg, sizeof(struct sp_omsg));
	exit(0);
}

#define	READ_SOCK(rp, rqb)  read_sock(rp, sizeof(struct nihdr)+(char *) &rqb, sizeof(rqb)-sizeof(struct nihdr))

/* Receive such a message. This is either the "other end" of the
   routine above, or a broadcast message from a machine to say
   we're going down.  */

void  net_recv(const struct nihdr *ni, struct remote *rp)
{
	USHORT		act = ntohs(ni->code);
	struct	netmsg		nrq;
	struct	spr_req		reply;

	READ_SOCK(rp, nrq);

	switch  (act)  {
	case  SN_TICKLE:	/* Still alive */
		return;

	case  SN_DELERR:

		/* Delete an error file.
		   This may not necessarily be the same as the job file */

		unlink(mkspid(ERNAM, (jobno_t) ntohl(nrq.arg)));
		break;

	case  SN_REQSYNC:
	case  SN_ENDSYNC:
	case  SN_SHUTHOST:

		reply.spr_mtype = MT_SCHED;
		reply.spr_un.o.spr_act = act;
		reply.spr_un.o.spr_netid = nrq.hostid;
		reply.spr_un.o.spr_pid = Netm_pid;
		reply.spr_un.o.spr_seq = 0;
		reply.spr_un.o.spr_jpslot = 0;
		reply.spr_un.o.spr_jobno = 0;
		reply.spr_un.o.spr_arg1 = 0;
		reply.spr_un.o.spr_arg2 = 0;
		push_msg((struct msgbuf *) &reply, sizeof(struct sp_omsg));

		if  (act == SN_SHUTHOST)
			/* Exit - we are regenerated by the main path
			   with new smaller tables.  */
			exit(0);
		break;
	}
}

/* Broadcast a message to all known hosts */

void  broadcast(char *msg, const unsigned size)
{
	int	cnt;

	/* We do this loop backwards so if one of them is found to be
	   dead, the "chk_write" logic will move the end one
	   which we've done over the top of the one we just tried
	   to do.  */

	for  (cnt = connected.rl_nums - 1;  cnt >= 0;  cnt--)  {
		struct  remote  *rp = connected.list[cnt];
		((struct netmsg *) msg)->seq = htons(rp->ht_seqto);
		chk_write(rp, msg, size);
	}
}

/* Tell our friends byebye */

void  netshut()
{
	unsigned		snore;
	struct	netmsg		rq;

	kill(Netm_pid, NETSHUTSIG);

	rq.code = htons(SN_SHUTHOST);
	rq.hostid = myhostid;
	rq.arg = 0;
	broadcast((char *) &rq, sizeof(rq));
	for  (snore = closedelay;  snore != 0;  snore = sleep(snore))
		;
}

/* Tell one host goodbye */

void  shut_host(const netid_t hostid)
{
	struct	remote	*rp;

	if  ((rp = inl_find_connected(hostid)))  {
		unsigned	snore;
		struct	netmsg	rq;
		rq.code = htons(SN_SHUTHOST);
		rq.seq = htons(rp->ht_seqto);
		rq.hostid = myhostid;
		rq.arg = 0;
		chk_write(rp, (char *) &rq, sizeof(rq));
		for  (snore = closedelay;  snore != 0;  snore = sleep(snore))
			;
	}
}

/* Keep connections alive */

unsigned  nettickle()
{
	int		cnt;
	unsigned	result;
	time_t		now;
	struct	netmsg	rq;

	if  (connected.rl_nums <= 0)
		return  0;

	now = time((time_t *) 0);
	result = 0;

	rq.code = htons(SN_TICKLE);
	rq.hostid = myhostid;
	rq.arg = 0;

	for  (cnt = connected.rl_nums - 1;  cnt >= 0;  cnt--)  {
		struct	remote	*rp = connected.list[cnt];
		unsigned  tleft = now - rp->lastwrite;

		/* If it was last written twice as long ago, send it a
		   null 'tickle' message.  If it was less than
		   that, but more than the "tickle" time, set
		   result to the shortest time of any connection
		   up to that time */

		if  (tleft >= rp->ht_timeout * 2)  {
			rq.seq = htons(rp->ht_seqto);
			chk_write(rp, (char *) &rq, sizeof(rq));
		}
		else  if  (tleft > rp->ht_timeout)  {
			tleft = rp->ht_timeout*2 - tleft;
			if  (result == 0  ||  tleft < result)
				result = tleft;
		}
	}

	/* For any pending operations, abandon any which have not had
	   a reply up to the timeout.  */

	for  (cnt = probed.rl_nums - 1;  cnt >= 0;  cnt--)  {
		struct  remote  *rp = probed.list[cnt];
		unsigned  tleft = now - rp->lastwrite;
		if  (tleft > rp->ht_timeout)
			deallochost(rp);
	}

	return  result;
}

/* Broadcast information about one of my jobs */

void  job_broadcast(Hashspq *jp, const int code)
{
	if  (connected.rl_nums > 0)  {
		struct	sp_jmsg	rq;
		rq.spr_act = htons((USHORT) code);
		rq.spr_pid = 0;
		rq.spr_netid = myhostid;
		rq.spr_jslot = htonl((ULONG) (jp - Job_seg.jlist));
		job_pack(&rq.spr_q, &jp->j);
		broadcast((char *) &rq, sizeof(rq));
	}
}

/* Accept a broadcast message about a job on some other machine sent
   by that machine.  */

void  job_recvbcast(const struct nihdr *ni, struct remote *rp)
{
	USHORT			act = ntohs(ni->code);
	struct	spr_req		rq;
	struct	sp_jmsg		jrq;
	struct	spq		injob;

	READ_SOCK(rp, jrq);

	switch  (act)  {
	case  SJ_ENQ:
		rq.spr_mtype = MT_SCHED;
		rq.spr_un.j.spr_act = SJ_ENQ;
		rq.spr_un.j.spr_pid = Netm_pid;
		rq.spr_un.j.spr_seq = 0;
		rq.spr_un.j.spr_netid = rp->hostid;
		unpack_job(&injob, &jrq.spr_q);
		injob.spq_rslot = ntohl(jrq.spr_jslot);
		push_jmsg(&rq, &injob);
		break;

	case  SJ_CHANGEDJOB:
	case  SJ_JUNASSIGNED:
		rq.spr_mtype = MT_SCHED;
		rq.spr_un.j.spr_act = act;
		rq.spr_un.j.spr_pid = Netm_pid;
		rq.spr_un.j.spr_seq = 0;
		rq.spr_un.j.spr_netid = rp->hostid;
		unpack_job(&injob, &jrq.spr_q);
		rq.spr_un.j.spr_jslot = ntohl(jrq.spr_jslot);
		push_jmsg(&rq, &injob);
		break;

	case  SO_DEQUEUED:
	case  SO_LOCASSIGN:
		rq.spr_mtype = MT_SCHED;
		rq.spr_un.o.spr_act = act;
		rq.spr_un.o.spr_pid = Netm_pid;
		rq.spr_un.o.spr_seq = 0;
		rq.spr_un.o.spr_netid = rp->hostid;
		rq.spr_un.o.spr_jpslot = ntohl(jrq.spr_jslot);
		push_msg((struct msgbuf *) &rq, sizeof(struct sp_omsg));
	}
}

/* Broadcast stuff about one of my printers */

void  ptr_broadcast(Hashspptr *pp, const int code)
{
	if  (connected.rl_nums > 0)  {
		struct	sp_pmsg		rq;
		rq.spr_act = htons((USHORT) code);
		rq.spr_pid = 0;
		rq.spr_netid = myhostid;
		rq.spr_pslot = htonl((ULONG) (pp - Ptr_seg.plist));
		ptr_pack(&rq.spr_p, &pp->p);
		broadcast((char *) &rq, sizeof(rq));
	}
}

/* Pick above message up at the other end */

void  ptr_recvbcast(const struct nihdr *ni, struct remote *rp)
{
	slotno_t	lslot;
	USHORT		act = ntohs(ni->code);
	struct	spr_req		rq;
	struct	sp_pmsg		prq;
	struct	spptr		inptr;

	READ_SOCK(rp, prq);

	switch  (act)  {
	case  SP_ADDP:
		rq.spr_mtype = MT_SCHED;
		rq.spr_un.p.spr_act = SP_ADDP;
		rq.spr_un.p.spr_pid = Netm_pid;
		rq.spr_un.p.spr_seq = 0;
		rq.spr_un.p.spr_netid = rp->hostid;
		unpack_ptr(&inptr, &prq.spr_p);
		inptr.spp_rslot = ntohl(prq.spr_pslot);
		push_pmsg(&rq, &inptr);
		break;

	case  SP_CHANGEDPTR:
	case  SP_PUNASSIGNED:
		rq.spr_mtype = MT_SCHED;
		rq.spr_un.p.spr_act = act;
		rq.spr_un.p.spr_pid = Netm_pid;
		rq.spr_un.p.spr_seq = 0;
		rq.spr_un.p.spr_netid = rp->hostid;
		unpack_ptr(&inptr, &prq.spr_p);
		if  ((lslot = find_pslot(rp->hostid, ntohl(prq.spr_pslot))) < 0)
			return;
		rq.spr_un.p.spr_pslot = lslot;
		push_pmsg(&rq, &inptr);
		break;

	case  SO_DELP:
		rq.spr_mtype = MT_SCHED;
		rq.spr_un.o.spr_act = SO_DELP;
		rq.spr_un.o.spr_pid = Netm_pid;
		rq.spr_un.o.spr_seq = 0;
		rq.spr_un.o.spr_netid = rp->hostid;
		if  ((lslot = find_pslot(rp->hostid, ntohl(prq.spr_pslot))) < 0)
			return;
		rq.spr_un.o.spr_jpslot = lslot;
		push_msg((struct msgbuf *) &rq, sizeof(struct sp_omsg));
		break;
	}
}

/* This says something about a job belonging to a remote machine to
   the machine, except for confirming a job for remote printing,
   in which case we are talking about this machine's job.  */

void	job_message(const netid_t netid, struct spq *jp, const int code, const ULONG arg1, const ULONG arg2)
{
	struct	remote		*rp;
	struct	sp_omsg		rq;

	if  (!(rp = inl_find_connected(netid)))
		return;
	rq.spr_act = htons((USHORT) code);
	rq.spr_pid = 0;
	rq.spr_seq = htons(rp->ht_seqto);
	rq.spr_jobno = htonl((ULONG) jp->spq_job);
	rq.spr_jpslot = htonl((ULONG) jp->spq_rslot);
	rq.spr_netid = myhostid;
	rq.spr_arg1 = htonl(arg1);
	rq.spr_arg2 = htonl(arg2);
	chk_write(rp, (char *) &rq, sizeof(rq));
}

/* Unravel that lot at the other end */

void  job_recvmsg(const struct nihdr *ni, struct remote *rp)
{
	USHORT		act = ntohs(ni->code);
	Hashspq		*hjp;
	struct	spr_req		rq;
	struct	sp_omsg		orq;

	READ_SOCK(rp, orq);

	rq.spr_mtype = MT_SCHED;
	rq.spr_un.o.spr_pid = Netm_pid;
	rq.spr_un.o.spr_seq = 0;
	rq.spr_un.o.spr_arg1 = ntohl(orq.spr_arg1);
	rq.spr_un.o.spr_arg2 = ntohl(orq.spr_arg2);
	rq.spr_un.o.spr_jobno = ntohl(orq.spr_jobno);
	rq.spr_un.o.spr_netid = rp->hostid;

	switch  (rq.spr_un.o.spr_act = act)  {
	default:
		if  (!(hjp = ver_job((slotno_t) ntohl(orq.spr_jpslot), (jobno_t) ntohl(orq.spr_jobno))))
			return;
		rq.spr_un.o.spr_jpslot = hjp - Job_seg.jlist;
		break;

		/* Bug fix 3/7/01 - allow through "proposals" for jobs which have been deleted
		   in case job gets printed and deleted by machine A before the request arrives
		   from machine B. Machine B won't ever get the proposal rejection and its
		   printer will be "stuck". */

	case  SO_PROPOSE:
		rq.spr_un.o.spr_jpslot = ntohl(orq.spr_jpslot);
		break;

	case  SO_PROP_OK:
	case  SO_PROP_NOK:
	case  SO_PROP_DEL:

		/* Arg1 contains the job slot number on this machine.
		   At least I hope it does!
		   Again, we rely on the confirm_print code to check*/

		rq.spr_un.o.spr_jpslot = ntohl(orq.spr_arg1);
		break;
	}
	push_msg((struct msgbuf *) &rq, sizeof(struct sp_omsg));
}

/* This says something about a printer belonging to a remote machine
   to the machine.  */

void  ptr_message(struct spptr *pp, const int code)
{
	struct	remote		*rp;
	struct	sp_omsg		rq;

	if  (!(rp = inl_find_connected(pp->spp_netid)))
		return;
	rq.spr_act = htons((USHORT) code);
	rq.spr_pid = 0;
	rq.spr_seq = htons(rp->ht_seqto);
	rq.spr_jobno = 0;
	rq.spr_arg1 = 0;
	rq.spr_arg2 = 0;
	rq.spr_netid = myhostid;
	rq.spr_jpslot = htonl(pp->spp_rslot);
	chk_write(rp, (char *) &rq, sizeof(rq));
}

/* Unravel that lot at the other end */

void  ptr_recvmsg(const struct nihdr *ni, struct remote *rp)
{
	USHORT		act = ntohs(ni->code);
	Hashspptr		*pp;
	struct	spr_req		rq;
	struct	sp_omsg		orq;

	READ_SOCK(rp, orq);
	if  (!(pp = ver_ptr(ntohl(orq.spr_jpslot))))
		return;
	rq.spr_mtype = MT_SCHED;
	rq.spr_un.o.spr_act = act;
	rq.spr_un.o.spr_pid = Netm_pid;
	rq.spr_un.o.spr_seq = 0;
	rq.spr_un.o.spr_jobno = 0;
	rq.spr_un.o.spr_netid = rp->hostid;
	rq.spr_un.o.spr_jpslot = pp - Ptr_seg.plist;
	push_msg((struct msgbuf *) &rq, sizeof(struct sp_omsg));
}

/* Tell a machine about a change I've made to one of its jobs */

void  job_sendupdate(struct spq *jp, struct spq *newj, const int code)
{
	struct	remote		*rp;
	struct	sp_jmsg		rq;

	if  (!(rp = inl_find_connected(jp->spq_netid)))
		return;
	rq.spr_act = htons((USHORT) code);
	rq.spr_pid = 0;
	rq.spr_seq = htons(rp->ht_seqto);
	rq.spr_netid = myhostid;
	rq.spr_jslot = htonl(jp->spq_rslot);
	job_pack(&rq.spr_q, newj);
	chk_write(rp, (char *) &rq, sizeof(rq));
}

/* Unravel that lot at the other end */

void  job_recvupdate(const struct nihdr *ni, struct remote *rp)
{
	USHORT		act = ntohs(ni->code);
	Hashspq			*hjp;
	struct	spr_req		rq;
	struct	sp_jmsg		jrq;
	struct	spq		injob;

	READ_SOCK(rp, jrq);
	if  (!(hjp = ver_job((slotno_t) ntohl(jrq.spr_jslot), (jobno_t) ntohl(jrq.spr_q.spq_job))))
		return;
	rq.spr_mtype = MT_SCHED;
	rq.spr_un.j.spr_act = act;
	rq.spr_un.j.spr_pid = Netm_pid;
	rq.spr_un.j.spr_seq = 0;
	rq.spr_un.j.spr_netid = 0;
	rq.spr_un.j.spr_jslot = hjp - Job_seg.jlist;
	unpack_job(&injob, &jrq.spr_q);
	push_jmsg(&rq, &injob);
}

/* Tell a machine about a change I've made to one of its printers */

void  ptr_sendupdate(struct spptr *pp, struct spptr *newp, const int code)
{
	struct	remote		*rp;

	if  ((rp = inl_find_connected(pp->spp_netid)))  {
		struct	sp_pmsg		rq;
		rq.spr_act = htons((USHORT) code);
		rq.spr_pid = 0;
		rq.spr_seq = htons(rp->ht_seqto);
		rq.spr_netid = myhostid;
		rq.spr_pslot = htonl(pp->spp_rslot);
		ptr_pack(&rq.spr_p, newp);
		chk_write(rp, (char *) &rq, sizeof(rq));
	}
}

/* Unravel that lot */

void  ptr_recvupdate(const struct nihdr *ni, struct remote *rp)
{
	Hashspptr		*pp;
	struct	spr_req		rq;
	struct	sp_pmsg		prq;
	struct	spptr		inptr;

	READ_SOCK(rp, prq);
	if  (!(pp = ver_ptr(ntohl(prq.spr_pslot))))
		return;
	rq.spr_mtype = MT_SCHED;
	rq.spr_un.p.spr_act = SP_CHGP;
	rq.spr_un.p.spr_pid = Netm_pid;
	rq.spr_un.p.spr_seq = 0;
	rq.spr_un.p.spr_netid = 0;
	rq.spr_un.p.spr_pslot = pp - Ptr_seg.plist;
	unpack_ptr(&inptr, &prq.spr_p);
	push_pmsg(&rq, &inptr);
}

/* Send message about assigning given job to printer.  We need to
   retain the slot numbers correctly. We are always calling this
   from the machine with the printer on.  */

void  ptr_assxmit(Hashspq *jp, Hashspptr *pp)
{
	if  (connected.rl_nums > 0)  {
		struct	sp_omsg	rq;

		rq.spr_act = htons(SO_ASSIGN);
		rq.spr_pid = 0;
		rq.spr_netid = myhostid;
		rq.spr_jpslot = htonl((ULONG) (pp - Ptr_seg.plist));
		rq.spr_jobno = htonl((ULONG) jp->j.spq_job);
		if  (jp->j.spq_netid)  {
			rq.spr_arg1 = htonl((ULONG) jp->j.spq_rslot);
			rq.spr_arg2 = jp->j.spq_netid;
		}
		else  {
			rq.spr_arg1 = htonl((ULONG) (jp - Job_seg.jlist));
			rq.spr_arg2 = myhostid;
		}
		broadcast((char *) &rq, sizeof(rq));
	}
}

/* Receive notice of assignment.  */

void  ptr_assrecv(const struct nihdr *ni, struct remote *rp)
{
	USHORT			act = ntohs(ni->code);
	Hashspq			*jp;
	Hashspptr		*pp;
	struct	spr_req		rq;
	struct	sp_omsg		orq;

	READ_SOCK(rp, orq);
	if  (!(pp = ver_remptr(ntohl(orq.spr_jpslot), (ULONG) orq.spr_netid)))
		return;

	rq.spr_mtype = MT_SCHED;
	rq.spr_un.o.spr_act = act;
	rq.spr_un.o.spr_pid = Netm_pid;
	rq.spr_un.o.spr_seq = 0;
	rq.spr_un.o.spr_jobno = ntohl(orq.spr_jobno);
	rq.spr_un.o.spr_netid = rp->hostid;
	rq.spr_un.o.spr_jpslot = pp - Ptr_seg.plist;

	if  (orq.spr_arg2 == myhostid)  {
		jp = ver_job((slotno_t) ntohl(orq.spr_arg1), (jobno_t) ntohl(orq.spr_jobno));
		if  (!jp)
			return;
		rq.spr_un.o.spr_arg1 = jp - Job_seg.jlist;
		rq.spr_un.o.spr_arg2 = 0L;
	}
	else  {
		rq.spr_un.o.spr_arg1 = ntohl(orq.spr_arg1);
		rq.spr_un.o.spr_arg2 = orq.spr_arg2;
	}
	push_msg((struct msgbuf *) &rq, sizeof(struct sp_omsg));
}

/* Send notification information about a job to a remote machine */

void  job_sendnote(Hashspq *jp, Hashspptr *pp, const int code, const jobno_t errf, const int past)
{
	struct	remote		*rp;
	struct	sp_omsg		orq;
	slotno_t		pnum;

	if  (!(rp = inl_find_connected(jp->j.spq_netid)))
		return;
	if  (past)
		orq.spr_act = htons(SO_PNOTIFY);
	else
		orq.spr_act = htons(SO_NOTIFY);
	orq.spr_pid = htonl((ULONG) code);
	orq.spr_seq = htons(rp->ht_seqto);
	orq.spr_jobno = htonl((ULONG) jp->j.spq_job);
	orq.spr_netid = myhostid;
	orq.spr_jpslot = htonl(jp->j.spq_rslot);
	orq.spr_arg1 = htonl(errf);
	pnum = pp? pp - Ptr_seg.plist: -1L;
	orq.spr_arg2 = htonl((ULONG) pnum);
	chk_write(rp, (char *) &orq, sizeof(orq));
}

/* For unravelling the above at the other end */

void  job_recvnote(const struct nihdr *ni, struct remote *rp)
{
	USHORT			act = ntohs(ni->code);
	Hashspq			*jp;
	slotno_t		pnum;
	struct	spr_req		rq;
	struct	sp_omsg		orq;

	READ_SOCK(rp, orq);
	if  (!(jp = ver_job((slotno_t) ntohl(orq.spr_jpslot), (jobno_t) ntohl(orq.spr_jobno))))
		return;
	rq.spr_mtype = MT_SCHED;
	rq.spr_un.o.spr_act = act;
	rq.spr_un.o.spr_arg1 = ntohl(orq.spr_pid);
	pnum = (slotno_t) ntohl(orq.spr_arg2);
	rq.spr_un.o.spr_arg2 = (ULONG) pnum;
	if  (pnum >= 0)  {
		Hashspptr	*pp = ver_remptr(pnum, (ULONG) orq.spr_netid);
		if  (pp)
			rq.spr_un.o.spr_arg2 = (ULONG) (pp - Ptr_seg.plist);
	}
	rq.spr_un.o.spr_pid = Netm_pid;
	rq.spr_un.o.spr_seq = 0;
	rq.spr_un.o.spr_jobno = ntohl(orq.spr_arg1);	/* This isn't necessarily the same as jp->j.spq_job */
	rq.spr_un.o.spr_netid = rp->hostid;
	rq.spr_un.o.spr_jpslot = jp - Job_seg.jlist;
	push_msg((struct msgbuf *) &rq, sizeof(struct sp_omsg));
}

/* For unravelling charges the other end
   Support receipt of messages for compatibility but do nothing */

void  chrg_recv(const struct nihdr *ni, struct remote *rp)
{
	struct	sp_cmsg		crq;
	READ_SOCK(rp, crq);			/* Just soak it up */
}

/* Tell existing machines about all our luvly jobs and printers
   after initial startup.  */

void	net_initsync()
{
	unsigned  lumpcount = 0;
	LONG	indx;

	indx = Job_seg.dptr->js_q_head;
	while  (indx >= 0L)  {
		Hashspq	*jp = &Job_seg.jlist[indx];
		if  (jp->j.spq_netid == 0  &&  !(jp->j.spq_jflags & SPQ_LOCALONLY))  {
			job_broadcast(jp, SJ_ENQ);
			if  ((++lumpcount % lumpsize) == 0)
				sleep(lumpwait);
		}
		indx = jp->q_nxt;
	}

	indx = Ptr_seg.dptr->ps_l_head;
	while  (indx >= 0L)  {
		Hashspptr *pp = &Ptr_seg.plist[indx];
		if  (pp->p.spp_state != SPP_NULL  &&  pp->p.spp_netid == 0  &&  !(pp->p.spp_netflags & SPP_LOCALONLY))  {
			ptr_broadcast(pp, SP_ADDP);
			if  ((++lumpcount % lumpsize) == 0)
				sleep(lumpwait);
		}
		indx = pp->l_nxt;
	}
}

/* Look around for remote machines we haven't got details of jobs or printers for */

void  netsync()
{
	int	rpcnt;

	for  (rpcnt = connected.rl_nums - 1;  rpcnt >= 0;  rpcnt--)  {
		struct  remote  *rp = connected.list[rpcnt];
		if  (rp->is_sync == NSYNC_NONE)  {
			struct	netmsg	rq;
			rq.code = htons(SN_REQSYNC);
			rq.seq = htons(rp->ht_seqto);
			rq.hostid = myhostid;
			rq.arg = 0;
			chk_write(rp, (char *) &rq, sizeof(rq));
			rp->is_sync = NSYNC_REQ;
			/* Return so we don't get indigestion with
			   everyone talking at once.  */
			return;
		}
	}
}

void  sendsync(const netid_t netid)
{
	unsigned  lumpcount = 0;
	LONG	  indx;
	struct	remote		*rp;
	struct	sp_jmsg		jrq;
	struct	sp_pmsg		prq;

	if  (!(rp = inl_find_connected(netid)))
		return;

	/* Set up common stuff */

	jrq.spr_act = htons(SJ_ENQ);
	jrq.spr_pid = 0;
	jrq.spr_netid = myhostid;
	prq.spr_act = htons(SP_ADDP);
	prq.spr_pid = 0;
	prq.spr_netid = myhostid;

	indx = Job_seg.dptr->js_q_head;
	while  (indx >= 0L)  {
		Hashspq	*jp = &Job_seg.jlist[indx];
		if  (jp->j.spq_netid == 0  &&  !(jp->j.spq_jflags & SPQ_LOCALONLY))  {
			jrq.spr_jslot = htonl((ULONG) indx);
			job_pack(&jrq.spr_q, &jp->j);
			jrq.spr_seq = htons(rp->ht_seqto);
			if  (!chk_write(rp, (char *) &jrq, sizeof(jrq)))
				return;
			if  ((++lumpcount % lumpsize) == 0)
				sleep(lumpwait);
		}
		indx = jp->q_nxt;
	}

	indx = Ptr_seg.dptr->ps_l_head;
	while  (indx >= 0L)  {
		Hashspptr *pp = &Ptr_seg.plist[indx];
		if  (pp->p.spp_state != SPP_NULL  &&  pp->p.spp_netid == 0  &&  !(pp->p.spp_netflags & SPP_LOCALONLY))  {
			prq.spr_pslot = htonl((ULONG) indx);
			ptr_pack(&prq.spr_p, &pp->p);
			prq.spr_seq = htons(rp->ht_seqto);
			if  (!chk_write(rp, (char *) &prq, sizeof(prq)))
				return;
			if  ((++lumpcount % lumpsize) == 0)
				sleep(lumpwait);
		}
		indx = pp->l_nxt;
	}
}

void  endsync(const netid_t netid)
{
	struct	remote	*rp;

	if  ((rp = inl_find_connected(netid))  &&  rp->is_sync != NSYNC_OK)  {
		rp->is_sync = NSYNC_OK;
		Netsync_req--;
	}
	if  (Netsync_req > 0)
		netsync();
}

void  remote_recv(struct remote *rp)
{
	struct	nihdr	nih;

	read_sock(rp, (char *) &nih, sizeof(nih));

	switch  (ntohs(nih.code))  {
	default:
		disp_arg[0] = ntohs(nih.code);
		nfreport($E{Unknown network packet});
		break;
	case  SJ_ENQ:
	case  SJ_CHANGEDJOB:
	case  SO_DEQUEUED:
	case  SJ_JUNASSIGNED:
	case  SO_LOCASSIGN:
		job_recvbcast(&nih, rp);
		break;

	case  SJ_CHNG:
	case  SJ_JUNASSIGN:
		job_recvupdate(&nih, rp);
		break;

	case  SP_ADDP:
	case  SP_CHANGEDPTR:
	case  SP_PUNASSIGNED:
	case  SO_DELP:
		ptr_recvbcast(&nih, rp);
		break;

	case  SP_CHGP:
		ptr_recvupdate(&nih, rp);
		break;

	case  SPD_CHARGE:
		chrg_recv(&nih, rp);
		break;

	case  SO_AB:
	case  SO_ABNN:
	case  SO_PROPOSE:
	case  SO_PROP_OK:
	case  SO_PROP_NOK:
	case  SO_PROP_DEL:
		job_recvmsg(&nih, rp);
		break;

	case  SO_ASSIGN:
		ptr_assrecv(&nih, rp);
		break;

	case  SO_RSP:
	case  SO_PHLT:
	case  SO_PSTP:
	case  SO_PGO:
	case  SO_OYES:
	case  SO_ONO:
	case  SO_INTER:
	case  SO_PJAB:
		ptr_recvmsg(&nih, rp);
		break;

	case  SO_NOTIFY:
	case  SO_PNOTIFY:
		job_recvnote(&nih, rp);
		break;

	case  SN_DELERR:
		net_recv(&nih, rp);
		/*	Don't check for sequence number with this because
			message is sent in separate process in sh_misc.c */
		return;
	case  SN_TICKLE:
	case  SN_REQSYNC:
	case  SN_ENDSYNC:
	case  SN_SHUTHOST:
		net_recv(&nih, rp);
		break;
	}

	/* Fall through and check sequence number */

	if  (ntohs(nih.seq) != rp->ht_seqfrom)  {
		if  (rp->ht_seqfrom != 0)  { /* Forget the initial case after fork */
			disp_arg[0] = rp->ht_seqfrom;
			disp_arg[1] = ntohs(nih.seq);
			disp_arg[2] = ntohs(nih.code);
			disp_str = look_host(rp->hostid);
			nfreport($E{Message synch lost});
		}
		rp->ht_seqfrom = ntohs(nih.seq);
	}
	rp->ht_seqfrom++;
}

/* Catch termination signals sent when we think something has gone down.  */

static	int	count_catch_shut = 0;

void  exec_catchshut()
{
	struct	spr_req	reply;
	reply.spr_mtype = MT_SCHED;
	reply.spr_un.o.spr_act = SN_ABORTHOST;
	push_msg((struct msgbuf *) &reply, sizeof(struct sp_omsg));
	exit(0);
}

RETSIGTYPE  catchshut(int n)
{
#ifdef	UNSAFE_SIGNALS
	signal(n, SIG_IGN);
#endif
	count_catch_shut++;
}

static RETSIGTYPE  stop_mon(int signum)
{
	struct	spr_req	msg;
#ifdef	STRUCT_SIG
	struct	sigstruct_name  z;
	z.sighandler_el = SIG_IGN;
	sigmask_clear(z);
	z.sigflags_el = 0;
	sigact_routine(signum, &z, (struct sigstruct_name *) 0);
#else
	signal(signum, SIG_IGN);
#endif
	msg.spr_mtype = MT_SCHED;
	msg.spr_un.o.spr_pid = getpid();
	msg.spr_un.o.spr_act = SO_SSTOP;
	msg.spr_un.o.spr_arg1 = signum == SIGTERM? $E{Sched killed}: $E{Netmon program fault};
	push_msg((struct msgbuf *) &msg, sizeof(struct sp_omsg));
	exit(0);
}

static	int	hadrfresh = 0;

/* This notes signals relating to remaps.  */

RETSIGTYPE	sh_markit(int sig)
{
#ifdef	UNSAFE_SIGNALS
	signal(sig, sh_markit);
#endif
	hadrfresh++;
}

/* This monitors incoming data from various remote hosts by forking
   off a process and listening on the various hosts.  If it finds
   a suitable candidate, the main scheduler path gets sent a
   message on the message channel. In the case where a new
   machine arrives we exit because our forked-off process has
   garbage hosts in it, so we can be born again another day.
   Hopefully this won't happen too often.  */

void  netmonitor()
{
	int	nret, rpcnt;
	int	highfd;
	fd_set	setupset, ready, excptset;
#ifdef	STRUCT_SIG
	struct	sigstruct_name  zmark;
	sigmask_clear(zmark);
	zmark.sigflags_el = SIGVEC_INTFLAG;
#endif

	if  ((Netm_pid = fork()) != 0)  {
#ifdef	BUGGY_SIGCLD
		PIDTYPE	pid2;
#endif
		if  (Netm_pid < 0)
			report($E{Internal cannot fork});
#ifdef	BUGGY_SIGCLD
		/* Contortions to avoid leaving zombie processes */

		if  ((pid2 = fork()) != 0)  {
			if  (pid2 < 0)  {
				kill(Netm_pid, SIGKILL);
				report($E{Internal cannot fork});
			}
			exit(0);
		}

		/* The main spshed process is now a sibling of the net
		   monitor process...  */

		nchild = 0;
#endif
		return;
	}

	Netm_pid = getpid();

#ifdef	STRUCT_SIG
	zmark.sighandler_el = sh_markit;
	sigact_routine(QRFRESH, &zmark, (struct sigstruct_name *) 0);
	zmark.sighandler_el = stop_mon;
	sigact_routine(SIGTERM, &zmark, (struct sigstruct_name *) 0);
#ifndef	DEBUG
	sigact_routine(SIGBUS, &zmark, (struct sigstruct_name *) 0);
	sigact_routine(SIGSEGV, &zmark, (struct sigstruct_name *) 0);
#ifdef	SIGSYS
	sigact_routine(SIGSYS, &zmark, (struct sigstruct_name *) 0);
#endif /* DEBUG */
#endif /* SIGSYS */
#else  /* HAVE_SIGVEC or unsafe sigs */
	signal(QRFRESH, sh_markit);
	signal(SIGTERM, stop_mon);
#ifndef	DEBUG
	signal(SIGBUS, stop_mon);
	signal(SIGSEGV, stop_mon);
	signal(SIGILL, stop_mon);
#ifdef	SIGSYS
	signal(SIGSYS, stop_mon);
#endif /* SIGSYS */
#endif /* DEBUG */
#endif

	highfd = listsock;
	if  (viewsock > highfd)
		highfd = viewsock;
	if  (probesock > highfd)
		highfd = probesock;
	FD_ZERO(&setupset);
	FD_SET(listsock, &setupset);
	FD_SET(viewsock, &setupset);
	FD_SET(probesock, &setupset);

	for  (rpcnt = 0;  rpcnt < connected.rl_nums;  rpcnt++)  {
		int	sock = connected.list[rpcnt]->sockfd;
		if  (sock > 0)  {
			if  (sock > highfd)
				highfd = sock;
			FD_SET(sock, &setupset);
		}
	}

	/* Set signal to note shutdown messages */

#ifdef	STRUCT_SIG
	zmark.sighandler_el = catchshut;
	sigact_routine(NETSHUTSIG, &zmark, (struct sigstruct_name *) 0);
#else
	signal(NETSHUTSIG, catchshut);
#endif

	/* We are now in the (possibly painfully) created net monitor process.
	   We slurp up messages and put on message queue.  */

	for  (;;)  {
		ready = setupset;
		excptset = setupset;

		for  (;;)  {
			while  (hadrfresh)  {
				hadrfresh = 0;
				check_jmoved();
				check_pmoved();
			}

			if  (count_catch_shut > 0)
				exec_catchshut();

			if  ((nret = select(highfd+1, &ready, (fd_set *) 0, &excptset, (struct timeval *) 0)) >= 0)
				break;
			if  (errno != EINTR)
				report($E{Poll or select error});
		}

		for  (rpcnt = 0;  rpcnt < connected.rl_nums;  rpcnt++)  {
			struct	remote	*rp = connected.list[rpcnt];
			if  (rp->sockfd >= 0  &&  FD_ISSET(rp->sockfd, &excptset))  {
				struct	spr_req	msg;
				nfreport($E{exception on TCP connection});
				msg.spr_mtype = MT_SCHED;
				msg.spr_un.o.spr_act = SN_SHUTHOST;
				msg.spr_un.o.spr_netid = rp->hostid;
				push_msg((struct msgbuf *) &msg, sizeof(struct sp_omsg));
				exit(0);
			}
		}

		if  (FD_ISSET(viewsock, &ready))  {
			feed_req();
			if  (--nret <= 0)
				continue;
		}

		for  (rpcnt = 0;  rpcnt < connected.rl_nums;  rpcnt++)  {
			struct	remote	*rp = connected.list[rpcnt];
			if  (rp->sockfd >= 0  &&  FD_ISSET(rp->sockfd, &ready))  {
				remote_recv(rp);
				--nret;
			}
		}

		if  (nret <= 0)
			continue;

		if  (FD_ISSET(probesock, &ready))  {
			reply_probe();
			if  (--nret <= 0)
				continue;
		}
		if  (FD_ISSET(listsock, &ready))  {
			struct	spr_req	reply;
			reply.spr_mtype = MT_SCHED;
			reply.spr_un.o.spr_act = SN_NEWHOST;
			push_msg((struct msgbuf *) &reply, sizeof(struct sp_omsg));
			exit(0);
		}
	}
}

#else	/* !NETWORK_VERSION */

/* This "routine" isn't strictly necessary but some C compilers winge
   if they are given a .c file with no code so here is some....  */

void	netmonitor()
{
	return;
}

#endif	/* !NETWORK_VERSION */
