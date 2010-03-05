#include "stdafx.h"
#include "mainfrm.h"
#include "formatcode.h"
#include "netmsg.h"
#include "spqw.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//  Stuff to pack details of jobs doing appropriate byte swaps
//  and unpack at the other end.

void	job_pack(spq &dest, const spq &src)
{
	dest.spq_job = htonl(long(src.spq_job));
	dest.spq_orighost = src.spq_orighost;
	dest.spq_netid = 0L;
	dest.spq_rslot = 0L;
	dest.spq_pslot = htonl(-1L);
	dest.spq_time = htonl(long(src.spq_time));
	dest.spq_hold = htonl(long(src.spq_hold));
	dest.spq_size = htonl(long(src.spq_size));
	dest.spq_posn = htonl(long(src.spq_posn));
	dest.spq_pagec = htonl(long(src.spq_pagec));
	dest.spq_start = htonl(long(src.spq_start));
	dest.spq_end = htonl(long(src.spq_end));
	dest.spq_npages = htonl(long(src.spq_npages));
	dest.spq_haltat = htonl(long(src.spq_haltat));

	dest.spq_jflags = htons(src.spq_jflags);
	dest.spq_nptimeout = htons(src.spq_nptimeout);
	dest.spq_ptimeout = htons(src.spq_ptimeout);
	dest.spq_uid = htonl(src.spq_uid);
	dest.spq_class = htonl(src.spq_class);
	dest.spq_extrn = 0;
	dest.spq_pglim = 0;
	dest.spq_proptime = 0L;
	dest.spq_starttime = 0L;
	dest.spq_wpri = htons(short(src.spq_wpri));

	dest.spq_cps = src.spq_cps;
	dest.spq_pri = src.spq_pri;
	dest.spq_sflags = src.spq_sflags;
	dest.spq_dflags = src.spq_dflags;

	strncpy(dest.spq_uname, src.spq_uname, UIDSIZE+1);
	strncpy(dest.spq_puname, src.spq_puname, UIDSIZE+1);
	strncpy(dest.spq_file, src.spq_file, MAXTITLE+1);
	strncpy(dest.spq_form, src.spq_form, MAXFORM+1);
	strncpy(dest.spq_ptr, src.spq_ptr, JPTRNAMESIZE+1);
	strncpy(dest.spq_flags, src.spq_flags, MAXFLAGS+1);
}

void	unpack_job(spq &dest, const spq &src)
{
	dest.spq_job = ntohl(long(src.spq_job));
	dest.spq_orighost = src.spq_orighost;
	dest.spq_netid = 0L;
	dest.spq_rslot = 0L;
	dest.spq_pslot = ntohl(-1L);
	dest.spq_time = ntohl(long(src.spq_time));
	dest.spq_hold = ntohl(long(src.spq_hold));
	dest.spq_size = ntohl(long(src.spq_size));
	dest.spq_posn = ntohl(long(src.spq_posn));
	dest.spq_pagec = ntohl(long(src.spq_pagec));
	dest.spq_start = ntohl(long(src.spq_start));
	dest.spq_end = ntohl(long(src.spq_end));
	dest.spq_npages = ntohl(long(src.spq_npages));
	dest.spq_haltat = ntohl(long(src.spq_haltat));

	dest.spq_jflags = ntohs(src.spq_jflags);
	dest.spq_nptimeout = ntohs(src.spq_nptimeout);
	dest.spq_ptimeout = ntohs(src.spq_ptimeout);
	dest.spq_uid = ntohl(src.spq_uid);
	dest.spq_class = ntohl(src.spq_class);
	dest.spq_extrn = ntohs(src.spq_extrn);
	dest.spq_pglim = 0;
	dest.spq_proptime = 0L;
	dest.spq_starttime = ntohl(src.spq_starttime);
	dest.spq_wpri = ntohs(short(src.spq_wpri));

	dest.spq_cps = src.spq_cps;
	dest.spq_pri = src.spq_pri;
	dest.spq_sflags = src.spq_sflags;
	dest.spq_dflags = src.spq_dflags;

	strncpy(dest.spq_uname, src.spq_uname, UIDSIZE+1);
	strncpy(dest.spq_puname, src.spq_puname, UIDSIZE+1);
	strncpy(dest.spq_file, src.spq_file, MAXTITLE+1);
	strncpy(dest.spq_form, src.spq_form, MAXFORM+1);
	strncpy(dest.spq_ptr, src.spq_ptr, JPTRNAMESIZE+1);
	strncpy(dest.spq_flags, src.spq_flags, MAXFLAGS+1);
}

//	Ditto for printers

void	ptr_pack(spptr &dest, const spptr &src)
{
	dest.spp_job = htonl(long(src.spp_job));
	dest.spp_rjhostid = src.spp_rjhostid;
	dest.spp_rjslot = htonl(long(src.spp_rjslot));
	dest.spp_jslot = htonl(long(src.spp_jslot));
	dest.spp_minsize = htonl(long(src.spp_minsize));
	dest.spp_maxsize = htonl(long(src.spp_maxsize));
	dest.spp_netid = htonl(long(src.spp_netid));
	dest.spp_rslot = htonl(long(src.spp_rslot));
	dest.spp_pid = htonl(long(src.spp_pid));

	dest.spp_resvd = 0;
#if	XITEXT_VN < 22
	dest.spp_class = htons(short(src.spp_class));
#else
	dest.spp_class = htonl(long(src.spp_class));
	dest.spp_extrn = 0;
#endif

	dest.spp_state = src.spp_state;
	dest.spp_sflags = src.spp_sflags;
	dest.spp_dflags = src.spp_dflags;
	dest.spp_netflags = src.spp_netflags;

	strncpy(dest.spp_dev, src.spp_dev, LINESIZE+1);
	strncpy(dest.spp_form, src.spp_form, MAXFORM+1);
	strncpy(dest.spp_ptr, src.spp_ptr, PTRNAMESIZE+1);
	strncpy(dest.spp_feedback, src.spp_feedback, PFEEDBACK+1);
	strncpy(dest.spp_comment, src.spp_comment, COMMENTSIZE+1);
}

void	unpack_ptr(spptr &dest, const spptr &src)
{
	dest.spp_job = ntohl(long(src.spp_job));
	dest.spp_rjhostid = src.spp_rjhostid;
	dest.spp_rjslot = ntohl(long(src.spp_rjslot));
	dest.spp_jslot = ntohl(long(src.spp_jslot));
	dest.spp_minsize = ntohl(long(src.spp_minsize));
	dest.spp_maxsize = ntohl(long(src.spp_maxsize));
	dest.spp_netid = ntohl(long(src.spp_netid));
	dest.spp_rslot = ntohl(long(src.spp_rslot));
	dest.spp_pid = ntohl(long(src.spp_pid));

	dest.spp_resvd = 0;
#if	XITEXT_VN < 22
	dest.spp_class = ntohs(short(src.spp_class));
#else
	dest.spp_class = ntohl(long(src.spp_class));
	dest.spp_extrn = ntohs(short(src.spp_extrn));
#endif

	dest.spp_state = src.spp_state;
	dest.spp_sflags = src.spp_sflags;
	dest.spp_dflags = src.spp_dflags;
	dest.spp_netflags = src.spp_netflags;

	strncpy(dest.spp_dev, src.spp_dev, LINESIZE+1);
	strncpy(dest.spp_form, src.spp_form, MAXFORM+1);
	strncpy(dest.spp_ptr, src.spp_ptr, PTRNAMESIZE+1);
	strncpy(dest.spp_feedback, src.spp_feedback, PFEEDBACK+1);
	strncpy(dest.spp_comment, src.spp_comment, COMMENTSIZE+1);
}

void    endsync(const netid_t);                                         

inline	HWND	mainhWnd()
{
	return  AfxGetApp()->m_pMainWnd->m_hWnd;
}                                  

// Get hold of connection to given TCP port.
// Port number is already htons-ified

static  SOCKET  tcp_connect(const netid_t h, const short port)
{
	SOCKET	sk;
	sockaddr_in	sin;
	
	if  ((sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		return INVALID_SOCKET;

	sin.sin_family = AF_INET;
	sin.sin_port = port;
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = h;

	if  (connect(sk, (sockaddr *) &sin, sizeof(sin)) != 0)  {
		int  code = WSAGetLastError();
		char	bleah[50];
		wsprintf(bleah, "conn error %d", code);
		AfxMessageBox(bleah, MB_OK|MB_ICONSTOP);
		closesocket(sk);
		return INVALID_SOCKET;
	}
	return  sk;
}

//  Send a probe - if we don't succeed generate a string ID ready for
//  a message box.                 

#define	UDP_TRIES	3

static	UINT	probe_send(const netid_t h, netmsg &msg)
{
	short	tries;
	SOCKET	sockfd;
	sockaddr_in	serv_addr, cli_addr;

	memset((void *) &serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = h;
	serv_addr.sin_port = Locparams.pportnum;

	memset((void *)&cli_addr, '\0', sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//	We don't really need the cli_addr but we are obliged to bind something.
	//	The remote uses our "pportnum".

	for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
		if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
			return  IDP_PROBE_SOCKERR;
		if  (bind(sockfd, (sockaddr *) &cli_addr, sizeof(cli_addr)) != 0)  {
			closesocket(sockfd);
			return	IDP_PROBE_BINDERR;
		}
		if  (sendto(sockfd, (char FAR *) &msg, sizeof(msg), 0, (sockaddr FAR *) &serv_addr, sizeof(serv_addr)) >= 0)  {
			closesocket(sockfd);
			return  0;
		}
		closesocket(sockfd);
	}
    
    if  (((CSpqwApp *) AfxGetApp())->m_options.spq_options.probewarn)
		return  IDP_PROBE_SENDERR;
	return  0;
}                               

// Receive probe from a Unix host

static	int	probe_recv(netmsg &msg)
{
	if  (recvfrom(Locparams.probesock, (char FAR *) &msg, sizeof(netmsg), 0, (sockaddr FAR *) 0, (int FAR *) 0) <= 0)
		return  -1;
	return  0;
}

//	Try to attach to remote machine which may already be running.

void	remote::conn_attach()
{
	SOCKET	sk;
	int	code;
	if  ((sk = tcp_connect(hostid, Locparams.lportnum)) != INVALID_SOCKET)  {
		current_q.alloc(this);
		if  (WSAAsyncSelect(sk, mainhWnd(), WM_NETMSG_ARRIVED, FD_READ) != 0)  {
			int  code = WSAGetLastError();
			AfxMessageBox(IDP_ASYNCSEL, MB_OK|MB_ICONSTOP);
		}	
		sockfd = sk;
		ht_flags |= HT_ISCLIENT;
		is_sync = NSYNC_NONE;
		lastwrite = time((time_t *) 0);
		ht_seqto = ht_seqfrom = 0;
		Locparams.Netsync_req++;
	}
	else  {
		code = WSAGetLastError();
		char	rbuf[80];
		wsprintf(rbuf, "Connect to %s failed on port %d error %d", (char FAR *)look_host(hostid),
				int(ntohs(Locparams.lportnum)), code);
		AfxMessageBox(rbuf, MB_OK|MB_ICONSTOP);
	}
}

//	Initiate connection by doing UDP probe first.

void	remote::probe_attach()
{
	netmsg	pmsg(htons(SON_CONNECT), Locparams.myhostid);
	UINT	pres = probe_send(hostid, pmsg);
	if  (pres != 0)  {
		AfxMessageBox(pres, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	pending_q.alloc(this);
	is_sync = NSYNC_NONE;
	ht_flags |= HT_ISCLIENT;
	lastwrite = time((time_t *) 0);
}

//  If someone else speaks to us, generate an appropriate response.

void  reply_probe()
{
	netmsg	inmsg;
	if  (probe_recv(inmsg) != 0)
		return;
	netid_t  whofrom = inmsg.hostid;
	if  (whofrom == 0L  ||	whofrom == Locparams.myhostid)
		return;

	switch  (ntohs(inmsg.code))  {
	default:
		return;		// Forget it

	case  SON_CONNECT:

		//	Probe connect - just bounce it back
	{
		netmsg	outmsg(htons(SON_CONNOK), Locparams.myhostid);
		probe_send(whofrom, outmsg);
		return;
	}

	case  SON_CONNOK:

		//	Connection ok - forget it if we weren't interested
		//	in that processor (possibly because it's ancient).

		remote	*rp;
		if  (rp = pending_q.find(whofrom))  {
			rp->conn_attach();
			pending_q.free(rp);
			netsync();                                                                                                                    
		}	
		return;
	}
}

//	Attach remote, either immediately, or by doing probe operation first

void	remote::rattach()
{
	if  (hostid == 0L  ||  hostid == Locparams.myhostid)
		return;
	if  (ht_flags & HT_PROBEFIRST)
		probe_attach();
	else
		conn_attach();
}

//	Accept connection from new machine

void  newhost()
{
#ifdef	ACCEPTCONNS
	netid_t  hostid;
	sockaddr_in	sin;                                                   
	int		sinl;
	SOCKET  newsock = accept(Locparams.listsock, (sockaddr FAR *) &sin, (int FAR *) &sinl);
	if  (newsock == INVALID_SOCKET)
		return;
	hostid = sin.sin_addr.s_addr;
	remote	*rp;
	if  (rp = pending_q.find(hostid))
		pending_q.free(rp);		//	Zap any pending connection	
	if  (!(rp = find_host(hostid)))	//  Ignore people we don't know about
		return;
	current_q.alloc(rp);
	rp->sockfd = newsock;
	if  (WSAAsyncSelect(newsock, mainhWnd(), WM_NETMSG_ARRIVED, FD_READ) != 0)  {
		int  code = WSAGetLastError();
		AfxMessageBox(IDP_ASYNCSEL, MB_OK|MB_ICONSTOP);
	}	
	rp->ht_flags = 0;
	rp->lastwrite = time((time_t *) 0);
	rp->is_sync = NSYNC_OK;
#endif
}

//	Attach hosts if possible

void	attach_hosts()
{
	extern	remote	*nhashtab[];
	for  (unsigned  cnt = 0;  cnt < HASHMOD;  cnt++)
		for  (remote *np = nhashtab[cnt];  np;  np = np->hn_next)
			np->rattach();
}

static	void	deallochost(remote  *rp)
{
	Jobs().net_jclear(rp->hostid);
	Printers().net_pclear(rp->hostid);
//	WSAAsyncSelect(rp->sockfd, mainhWnd(), 0, 0);
	closesocket(rp->sockfd);
	if  (rp->is_sync != NSYNC_OK)
		Locparams.Netsync_req--;
	current_q.free(rp);
}

//	Write to socket, but if we get some error, treat connection
//	as down.  Return 0 in that case so we can break out of loops.

static	int	chk_write(remote *rp, char FAR *buffer, const int length)
{
	int	 lleft = length, nbytes;
	do  {
		if  ((nbytes = send(rp->sockfd, buffer, lleft, 0)) < 0)  {
			int  code = WSAGetLastError();
			deallochost(rp);
			return  0;
		}
		lleft -= nbytes;
		buffer += nbytes;
	}  while  (lleft > 0);
	rp->lastwrite = time((time_t *) 0);
	rp->ht_seqto++;
	return  1;
}

void	clearhost(const netid_t netid)
{
	remote	*rp;
	if  (rp = pending_q.find(netid))
		pending_q.free(rp);
	if  (rp = current_q.find(netid))
		deallochost(rp);
}

//	Send a random sort of message to a host
//	Currently this is used only for "delete job error file" and "endsync".

void	net_xmit(const netid_t netid, const short code, const long arg)
{
	remote  *rp;
	if  (rp = current_q.find(netid))  {
		netmsg	outmsg(htons(code), Locparams.myhostid, htons(rp->ht_seqto), htonl(arg));
		chk_write(rp, (char FAR *) &outmsg, sizeof(outmsg));
	}	
}

//	Receive such a message. This is either the "other end" of the
//	routine above, or a broadcast message from a machine to say we're
//	going down.

void	net_recv(remote *rp)
{
	netmsg		&nrq = *(netmsg *) rp->buffer;

	switch  (ntohs(nrq.code))  {
	case  SN_TICKLE:	// Still alive
	case  SN_DELERR:
		//	Delete an error file.
		//	No such thing on this machine.
		return;

	case  SN_REQSYNC:
		//	Don't do a sendsync because we haven't got any jobs or
		//	printers.
		net_xmit(nrq.hostid, SN_ENDSYNC, 0L);
		return;

	case  SN_ENDSYNC:
		endsync(nrq.hostid);
		return;

	case  SN_SHUTHOST:
		clearhost(nrq.hostid);
		return;
	}
}

//	Broadcast a message to all known hosts

void	broadcast(char FAR *msg, const int size)
{
	current_q.setlast();
	remote	*rp;
	while  (rp = current_q.prev())  {
		((struct netmsg FAR *)msg)->seq = htons(rp->ht_seqto);
		chk_write(rp, msg, size);    
	}
}

//	Tell our friends byebye

void	netshut()
{
	netmsg  rq(htons(SN_SHUTHOST), Locparams.myhostid);
	broadcast((char FAR *) &rq, sizeof(rq));
	current_q.setlast();
	remote	*rp;
	while  (rp = current_q.prev())
		deallochost(rp);
}

//	Tell one host goodbye

void	shut_host(netid_t hostid)
{
	remote	*rp;
	if  (rp = current_q.find(hostid))  {
		netmsg	rq(htons(SN_SHUTHOST), Locparams.myhostid, htons(rp->ht_seqto));
		chk_write(rp, (char FAR *) &rq, sizeof(rq));
	}	
}

//	Accept a broadcast message about a job on some other machine
//	sent by that machine.

void	job_recvbcast(remote *rp)
{
	sp_jmsg  	&jrq = *(sp_jmsg *) rp->buffer;
	spq			jq;
	
	switch  (ntohs(jrq.spr_act))  {
	case  SJ_ENQ:
		unpack_job(jq, jrq.spr_q);
		Jobs().addjob(jident(jq.spq_job, rp->hostid, ntohl(jrq.spr_jslot)), jq);
		break;

	case  SJ_CHANGEDJOB:
		unpack_job(jq, jrq.spr_q);
		Jobs().changedjob(jident(jq.spq_job, rp->hostid, ntohl(jrq.spr_jslot)), jq);
		break;
		
	case  SJ_JUNASSIGNED:
		unpack_job(jq, jrq.spr_q);
		Jobs().unassign(jident(jq.spq_job, rp->hostid, ntohl(jrq.spr_jslot)), jq);
		break;

	case  SO_DEQUEUED:
		Jobs().deljob(jident(ntohl(jrq.spr_q.spq_job), rp->hostid, ntohl(jrq.spr_jslot)));
		break;
		
 	case  SO_LOCASSIGN:
		Jobs().locpassign(jident(ntohl(jrq.spr_q.spq_job), rp->hostid, ntohl(jrq.spr_jslot)));
		break;
	}
}

//	Pick message about printers up at the other end

void	ptr_recvbcast(remote  *rp)
{
	sp_pmsg		&prq = *(sp_pmsg *) rp->buffer;
	spptr		pq;

	switch  (ntohs(prq.spr_act))  {
	case  SP_ADDP:
		unpack_ptr(pq, prq.spr_p);
		Printers().addptr(pident(rp->hostid, ntohl(prq.spr_pslot)), pq);		
		break;

	case  SP_CHANGEDPTR:
		unpack_ptr(pq, prq.spr_p);
		Printers().changedptr(pident(rp->hostid, ntohl(prq.spr_pslot)), pq);
		break;
		
	case  SP_PUNASSIGNED:
		unpack_ptr(pq, prq.spr_p);
		Printers().unassign_ptr(pident(rp->hostid, ntohl(prq.spr_pslot)), pq);
		break;

	case  SO_DELP:
		Printers().delptr(pident(rp->hostid, ntohl(prq.spr_pslot)));
		break;
	}
}

//	This says something about a job belonging to a remote machine
//	to the machine.

void	job_message(const netid_t netid, const spq *jp, const int code, const unsigned long arg1, const unsigned long arg2)
{
	remote		*rp;

	if  (!(rp = current_q.find(netid)))
		return;
	sp_omsg		rq;
	rq.spr_act = htons((unsigned short) code);
	rq.spr_pid = 0;
	rq.spr_seq = htons(rp->ht_seqto);
	rq.spr_jobno = htonl((unsigned long) jp->spq_job);
	rq.spr_jpslot = htonl((unsigned long) jp->spq_rslot);
	rq.spr_netid = Locparams.myhostid;
	rq.spr_arg1 = htonl(arg1);
	rq.spr_arg2 = htonl(arg2);
	chk_write(rp, (char FAR *) &rq, sizeof(rq));
}

//	Unravel that lot at the other end
//	Currently we dont use this routine but we implement it
//	to soak up mesages.

void	job_recvmsg(remote *rp)
{
}

//	This says something about a printer belonging to a remote machine
//	to the machine.

void	ptr_message(const spptr *pp, const int code)
{
	remote		*rp;

	if  (!(rp = current_q.find(pp->spp_netid)))
		return;
	sp_omsg  rq;
	memset((void *) &rq, '\0', sizeof(rq));
	rq.spr_act = htons((unsigned short) code);
	rq.spr_netid = Locparams.myhostid;
	rq.spr_jpslot = htonl(pp->spp_rslot);
	rq.spr_seq = htons(rp->ht_seqto);
	chk_write(rp, (char FAR *) &rq, sizeof(rq));
}

//	Unravel that lot at the other end (dummy routine)

void	ptr_recvmsg(remote *rp)
{
}

//	Tell a machine about a change I've made to one of its jobs

void	job_sendupdate(const spq *jp, const spq *newj, const int code)
{
	remote		*rp;
	if  (!(rp = current_q.find(jp->spq_netid)))
		return;
	sp_jmsg		rq;
	rq.spr_act = htons((unsigned short) code);
	rq.spr_pid = 0;
	rq.spr_seq = htons(rp->ht_seqto);
	rq.spr_netid = Locparams.myhostid;
	rq.spr_jslot = htonl(jp->spq_rslot);
	job_pack(rq.spr_q, *newj);
	chk_write(rp, (char FAR *) &rq, sizeof(rq));
}

//	Unravel that lot at the other end (dummy routine)

void	job_recvupdate(remote *rp)
{
}

//	Tell a machine about a change I've made to one of its printers

void	ptr_sendupdate(const spptr *pp, const spptr *newp, const int code)
{
	remote		*rp;
	if  (!(rp = current_q.find(pp->spp_netid)))
		return;
	sp_pmsg		rq;
	ptr_pack(rq.spr_p, *newp);	
	rq.spr_act = htons((unsigned short) code);
	rq.spr_pid = 0;
	rq.spr_seq = htons(rp->ht_seqto);
	rq.spr_netid = Locparams.myhostid;
	rq.spr_pslot = htonl(pp->spp_rslot);
	chk_write(rp, (char FAR *) &rq, sizeof(rq));
}

//	Unravel that lot (dummy routine)

void	ptr_recvupdate(remote *rp)
{
}

//	Receive notice of assignment

void	ptr_assrecv(remote *rp)
{
	sp_omsg		&orq = *(sp_omsg *) rp->buffer;
	int	jind = Jobs().jindex(jident(jobno_t(ntohl(orq.spr_jobno)), orq.spr_arg2, slotno_t(ntohl(orq.spr_arg1))));
	int pind = Printers().pindex(pident(orq.spr_netid, slotno_t(ntohl(orq.spr_jpslot))));
	if  (jind >= 0  &&  pind >= 0)  {
		Jobs().assign(jind, pind);
		Printers().assign(jind, pind, jobno_t(ntohl(orq.spr_jobno)));
	}	
}

//	dummy routine for receiving job notification

void	job_recvnote(remote *rp)
{
}

//	For unravelling charges (dummy routine)

void	chrg_recv(remote *rp)
{
}

//	Look around for remote machines we haven't got details of jobs
//	or printers for

void	netsync()
{
	remote	*rp;

	current_q.setfirst();
	while  (rp = current_q.next())
		if  (rp->is_sync == NSYNC_REQ)
			return;
	current_q.setfirst();
	while  (rp = current_q.next())  {
		if  (rp->is_sync == NSYNC_NONE)  {
			netmsg	rq(htons(SN_REQSYNC), Locparams.myhostid, htons(rp->ht_seqto));
			if  (chk_write(rp, (char FAR *) &rq, sizeof(rq)))
				rp->is_sync = NSYNC_REQ;
			//	Return so we don't get indigestion
		 	//	with everyone talking at once.
			return;
		}
	}
}

void	endsync(const netid_t netid)
{
	remote	*rp;

	if  ((rp = current_q.find(netid))  &&  rp->is_sync != NSYNC_OK)  {
		rp->is_sync = NSYNC_OK;
		Locparams.Netsync_req--;
	}
	if  (Locparams.Netsync_req > 0)
		netsync();
}

void	remote_recv(remote *rp)
{
	//  Read in the message code which may come in several bits (unlikely for 2 bytes!)
	
	if  (rp->byteoffset < sizeof(unsigned short))  {
		if  (rp->bytesleft == 0)
			rp->bytesleft = sizeof(unsigned short);
		int	nbytes = recv(rp->sockfd, &rp->buffer[rp->byteoffset], (int) rp->bytesleft, 0);
		if  (nbytes <= 0)  {
			char	winge[80];
			wsprintf(winge, "read error %d from %s", WSAGetLastError(), (char FAR *) rp->namefor());
			AfxMessageBox(winge, MB_OK|MB_ICONSTOP);
			return;                                 
		}
		rp->bytesleft -= nbytes;
		rp->byteoffset += nbytes;
		return;
	}
	
	//  We previously read the header, so read the rest of the message
	//  and dispose of it when it is complete.
	
	unsigned  short  code = ntohs(*(unsigned short *)rp->buffer);
	if  (rp->bytesleft == 0)
		switch  (code)  {
		default:
		case  SJ_ENQ:
		case  SJ_CHANGEDJOB:
		case  SO_DEQUEUED:
		case  SJ_JUNASSIGNED:
		case  SO_LOCASSIGN:
		case  SJ_CHNG:
		case  SJ_JUNASSIGN:
			rp->bytesleft = sizeof(sp_jmsg) - sizeof(unsigned short);
			break;

		case  SP_ADDP:
		case  SP_CHANGEDPTR:
		case  SP_PUNASSIGNED:
		case  SO_DELP:
			rp->bytesleft = sizeof(sp_pmsg) - sizeof(unsigned short);
			break;

		case  SPD_CHARGE:
			rp->bytesleft = sizeof(sp_cmsg) - sizeof(unsigned short);
			break;
			
		case  SO_AB:
		case  SO_ABNN:
		case  SO_PROPOSE:
		case  SO_PROP_OK:
		case  SO_PROP_NOK:
		case  SO_PROP_DEL:
		case  SO_ASSIGN:
		case  SO_RSP:
		case  SO_PHLT:
		case  SO_PSTP:
		case  SO_PGO:
		case  SO_OYES:
		case  SO_ONO:
		case  SO_INTER:
		case  SO_PJAB:
		case  SO_NOTIFY:
		case  SO_PNOTIFY:
			rp->bytesleft = sizeof(sp_omsg) - sizeof(unsigned short);
			break;

		case  SN_TICKLE:
		case  SN_DELERR:
		case  SN_REQSYNC:
		case  SN_ENDSYNC:      
		case  SN_SHUTHOST:
			rp->bytesleft = sizeof(netmsg) - sizeof(unsigned short);
			break;
		}
	
	int	nbytes = recv(rp->sockfd, &rp->buffer[rp->byteoffset], (int) rp->bytesleft, 0);
	if  (nbytes <= 0)  {
		char	winge[80];
		wsprintf(winge, "read data error %d from %s", WSAGetLastError(), (char FAR *) rp->namefor());
		AfxMessageBox(winge, MB_OK|MB_ICONSTOP);
		return;                                 
	}
	rp->bytesleft -= nbytes;
	rp->byteoffset += nbytes;
	if  (rp->bytesleft != 0)
		return;					//  And get another message
    rp->byteoffset = 0;			//  Before we forget

	switch  (code)  {
	default:
	case  SJ_ENQ:
	case  SJ_CHANGEDJOB:
	case  SO_DEQUEUED:
	case  SJ_JUNASSIGNED:
	case  SO_LOCASSIGN:
		job_recvbcast(rp);
		return;

	case  SJ_CHNG:
	case  SJ_JUNASSIGN:
		job_recvupdate(rp);
		return;

	case  SP_ADDP:
	case  SP_CHANGEDPTR:
	case  SP_PUNASSIGNED:
	case  SO_DELP:
		ptr_recvbcast(rp);
		return;

	case  SP_CHGP:
		ptr_recvupdate(rp);
		return;

	case  SPD_CHARGE:
		chrg_recv(rp);
		return;

	case  SO_AB:
	case  SO_ABNN:
	case  SO_PROPOSE:
	case  SO_PROP_OK:
	case  SO_PROP_NOK:
	case  SO_PROP_DEL:
		job_recvmsg(rp);
		return;

	case  SO_ASSIGN:
		ptr_assrecv(rp);
		return;

	case  SO_RSP:
	case  SO_PHLT:
	case  SO_PSTP:
	case  SO_PGO:
	case  SO_OYES:
	case  SO_ONO:
	case  SO_INTER:
	case  SO_PJAB:
		ptr_recvmsg(rp);
		return;

	case  SO_NOTIFY:
	case  SO_PNOTIFY:
		job_recvnote(rp);
		return;

	case  SN_TICKLE:
	case  SN_DELERR:
	case  SN_REQSYNC:
	case  SN_ENDSYNC:
	case  SN_SHUTHOST:
		net_recv(rp);
		return;
	}
}

// Accept messages passed on from mainframe.cpp

void	net_recvmsg(WPARAM sockfd, LPARAM code)
{
	unsigned  eventcode = WSAGETSELECTEVENT(code);
	if  (eventcode & FD_CLOSE)  {
		remote	*rp = find_host(SOCKET(sockfd));
		if  (rp)
			deallochost(rp);
		return;
	}
	if  (eventcode & FD_READ)  {
		remote  *rp;
		if	(rp = find_host(SOCKET(sockfd)))
				remote_recv(rp);
	}
}			

void	net_recvconn(WPARAM, LPARAM)
{
	newhost();
}                

void	net_recvprobe(WPARAM, LPARAM)
{
#ifdef	FOOOO   
	fd_set	fds;
	timeval  tot;
	memset((void *) &tot, '\0', sizeof(tot));
	FD_ZERO(&fds);
	FD_SET(Locparams.probesock, &fds);
	do  {
#endif
		reply_probe();
#ifdef	FOOOO
		FD_ZERO(&fds);
		FD_SET(Locparams.probesock, &fds);
	}  while  (select(FD_SETSIZE, &fds, NULL, NULL, &tot) > 0);	
#endif
}
	
