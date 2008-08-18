#include "stdafx.h"
#include <string.h>
#include <sys/types.h>
#ifdef	SPQW
#include "spqw.h"
#endif
#ifdef	SPRSETW
#include "sprsetw.h"
#endif
#ifdef	SPRSERV
#include "pages.h"
#include "xtini.h"
#include "sprserv.h"
#include "refreshconn.h"
#endif
#include "clientif.h"
#include "ulist.h"
#include "loginhost.h"

#define UDP_WAITTIME       5

extern	sockaddr_in     serv_addr;

static  void     unpack_spdet(spdet FAR &dest, const spdet &src)
{
	dest.spu_isvalid = src.spu_isvalid;
	dest.spu_class = ntohl(src.spu_class);
	dest.spu_flgs = ntohl(src.spu_flgs);
	dest.spu_user = ntohl((unsigned long) src.spu_user);
	dest.spu_minp = src.spu_minp;
	dest.spu_maxp = src.spu_maxp;
	dest.spu_defp = src.spu_defp;
	strncpy((char FAR *) dest.spu_form, src.spu_form, MAXFORM);
	strncpy((char FAR *) dest.spu_formallow, src.spu_formallow, ALLOWFORMSIZE);
	strncpy((char FAR *) dest.spu_ptr, src.spu_ptr, PTRNAMESIZE);
	strncpy((char FAR *) dest.spu_ptrallow, src.spu_ptrallow, JPTRNAMESIZE);
	dest.spu_form[MAXFORM] = '\0';
	dest.spu_formallow[ALLOWFORMSIZE] = '\0';
	dest.spu_ptr[PTRNAMESIZE] = '\0';
	dest.spu_ptrallow[JPTRNAMESIZE] = '\0';
	dest.spu_cps = src.spu_cps;
}

const  int	udp_sendrecv(char FAR *sendbuf, char FAR *recvbuf, const int sendsize, const int recvsize)
{
	time(&Locparams.tlastop);
	if  (sendto(Locparams.uasocket, sendbuf, sendsize, 0, (struct sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
		return  IDP_GSPU_NOSEND;
	fd_set	rfds;
	FD_ZERO(&rfds);
	FD_SET(Locparams.uasocket, &rfds);
	timeval	tv;
	tv.tv_sec = UDP_WAITTIME;
	tv.tv_usec = 0;
	if  (select(FD_SETSIZE, &rfds, (fd_set FAR *) 0, (fd_set FAR *) 0, &tv) <= 0)
		return  IDP_GSPU_NORECV;
	if  (recvfrom(Locparams.uasocket, recvbuf, recvsize, 0, (sockaddr FAR *) 0, (int FAR *) 0) <= 0)
		return  IDP_GSPU_NORECV;
	return  0;
}

const	int		xt_enquire(CString &username, CString &mymachname, CString &resname)
{
	ua_login	enq, reply;
	memset(&enq, '\0', sizeof(enq));
	enq.ual_op = UAL_ENQUIRE;
	strncpy(enq.ual_name, (const char *) username, UIDSIZE);
	strncpy(enq.ual_machname, (const char *) mymachname, HOSTNSIZE);
	int  ret = udp_sendrecv((char FAR *) &enq, (char FAR *) &reply, sizeof(enq), sizeof(reply));
	if  (ret != 0)
		return  ret;
	if  (reply.ual_op == UAL_OK)  {
		resname = reply.ual_name;
		return  0;
	}
	switch  (reply.ual_op)  {
	default:					return  IDP_XTENQ_UNKNOWN;
	case  XTNR_NOT_CLIENT:
	case  XTNR_UNKNOWN_CLIENT:	return  IDP_XTENQ_BADCLIENT;
	case  XTNR_NOT_USERNAME:
	case  UAL_INVU:				return  IDP_XTENQ_UNKNOWNU;
	case  UAL_INVP:
	case  UAL_NOK:				return  IDP_XTENQ_PASSREQ;
	}
}

const	int		xt_login(CString &username, CString &mymachname, const char *passwd, CString &resname)
{
	ua_login	enq, reply;
	memset(&enq, '\0', sizeof(enq));
	enq.ual_op = UAL_LOGIN;
	strncpy(enq.ual_name, (const char *) username, UIDSIZE);
	strncpy(enq.ual_machname, (const char *) mymachname, HOSTNSIZE);
	strncpy(enq.ual_passwd, passwd, UA_PASSWDSZ);
	int  ret = udp_sendrecv((char FAR *) &enq, (char FAR *) &reply, sizeof(enq), sizeof(reply));
	if  (ret != 0)
		return  ret;
	if  (reply.ual_op == UAL_OK)  {
		resname = reply.ual_name;
		return  0;
	}
	switch  (reply.ual_op)  {
	default:					return  IDP_XTENQ_UNKNOWN;
	case  XTNR_NOT_CLIENT:
	case  XTNR_UNKNOWN_CLIENT:	return  IDP_XTENQ_BADCLIENT;
	case  XTNR_NOT_USERNAME:
	case  UAL_INVU:				return  IDP_XTENQ_UNKNOWNU;
	case  UAL_INVP:
	case  UAL_NOK:				return  IDP_XTENQ_BADPASSWD;
	}
}

const  int	xt_logout()
{
	ua_login	enq, reply;
	memset(&enq, '\0', sizeof(enq));
	enq.ual_op = UAL_LOGOUT;
	int  ret = udp_sendrecv((char FAR *) &enq, (char FAR *) &reply, sizeof(enq), sizeof(reply));
	if  (ret != 0)
		return  ret;
	if  (reply.ual_op != UAL_OK)
		return  IDP_XTENQ_UNKNOWN;
	return  0;
}

const  int      getspuser(spdet  FAR &mpriv, CString &realuname, const short)
{
	char    abyte = CL_SV_UENQUIRY;
	ua_reply        resp;
	int	ret = udp_sendrecv((char FAR *) &abyte, (char FAR *) &resp, sizeof(abyte), sizeof(resp));
	if  (ret != 0)
		return  ret;
	if  (resp.ua_perm.spu_isvalid)  {
		realuname = resp.ua_uname;
		unpack_spdet(mpriv, resp.ua_perm);
		return  0;
	}

	//  If invalid, the error code is returned in the user id field

	switch  (ntohl(resp.ua_perm.spu_user))  {
	default:					return  IDP_GSPU_NVALID;
	case  XTNR_UNKNOWN_CLIENT:	return  IDP_XTENQ_BADCLIENT;
	case  XTNR_NOT_USERNAME:
	case  UAL_INVU:				return  IDP_XTENQ_UNKNOWNU;
	case  XTNR_NO_PASSWD:
	case  XTNR_PASSWD_INVALID:	return  IDP_XTENQ_PASSREQ;
    }
}

static	BOOL	relogin()
{
#ifdef	SPQW
	CSpqwApp  &ma = *((CSpqwApp *)AfxGetApp());
#endif
#ifdef	SPRSETW
	CSprsetwApp  &ma = *((CSprsetwApp *)AfxGetApp());
#endif
#ifdef	SPRSERV
	CSprservApp  &ma = *((CSprservApp *)AfxGetApp());
#endif
	int	ret;
	CString	newuser;

	if  ((ret = xt_enquire(ma.m_winuser, ma.m_winmach, newuser)) != 0)  {

		if  (ret != IDP_XTENQ_PASSREQ)
			return  FALSE;            

		CLoginHost	dlg;
		dlg.m_unixhost = look_host(Locparams.servid);
		dlg.m_clienthost = ma.m_winmach;
		dlg.m_username = ma.m_winuser;

		int	cnt = 0;
		
		for  (;;)  {
			
			if  (dlg.DoModal() != IDOK)
				return  FALSE;

			if  ((ret = xt_login(dlg.m_username, ma.m_winmach, (const char *) dlg.m_passwd, newuser)) == 0)
				break;

			if  (ret != IDP_XTENQ_BADPASSWD || cnt >= 2)  {
		    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
				return  FALSE;
			}

			if  (AfxMessageBox(ret, MB_RETRYCANCEL|MB_ICONQUESTION) == IDCANCEL)
				return  FALSE;
			cnt++;
		}
	}

    if  (getspuser(ma.m_mypriv, ma.m_username) != 0)
		return  FALSE;
	return  TRUE;
}

BOOL	refreshconn()
{
	if  (time(NULL) < Locparams.tlastop + Locparams.servtimeout/2)
		return  TRUE;

	char	msg = CL_SV_KEEPALIVE, repl;
	int		ret = udp_sendrecv((char FAR *) &msg, (char FAR *) &repl, sizeof(msg), sizeof(repl));
	if  (ret == 0  &&  repl == 0)
		return  TRUE;

	//  He may want to give up now

#ifdef	SPRSERV
	if  (!((CSprservApp *)AfxGetApp())->m_dontask)  {
		MessageBeep(MB_ICONQUESTION); 
		CRefreshconn  dlg;
		dlg.m_servname = look_host(Locparams.servid);
		dlg.m_action = 0;
		if  (dlg.DoModal() != IDOK || dlg.m_action > 1)
			goto  giveup_nomsg;
		if  (dlg.m_action == 1)
			((CSprservApp *)AfxGetApp())->m_dontask = TRUE;
	}
#else
	if  (AfxMessageBox(IDP_CONNDIED, MB_ICONSTOP|MB_YESNO) != IDYES)
		goto  giveup;
#endif
	
	if  (initenqsocket(Locparams.servid) != 0)
		goto  giveup;

	if  (relogin())
		return  TRUE;

giveup:
#ifdef	SPRSERV
	AfxMessageBox(IDP_CONNDIED, MB_ICONSTOP|MB_OK);
giveup_nomsg:
	((CSprservApp *)AfxGetApp())->m_shutdown = TRUE;
#endif
	AfxGetMainWnd()->SendMessage(WM_CLOSE);
	return  FALSE;
}

UUserList::UUserList(const char FAR *prefx)
{
	if  (prefx)  {
		preflen = strlen(prefx);
		strncpy(prefix, prefx, UIDSIZE);
		prefix[UIDSIZE] = '\0';
	}
	else  {
		prefix[0] = '\0';     
		preflen = 0;
	}

	char    abyte[1];
	abyte[0] = CL_SV_ULIST;
	time(&Locparams.tlastop);
	if  (sendto(Locparams.uasocket, abyte, sizeof(abyte), 0, (sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
		nbytes = pos = 0;
	else  {         
		pos = 0;
		nbytes = recvfrom(Locparams.uasocket, buffer, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
	}       
}

const  char  FAR  *UUserList::nextuser()
{
	if  (nbytes <= 0)
		return  NULL;
	for  (;;)  {
		if  (pos >= nbytes)  {
			nbytes = recvfrom(Locparams.uasocket, buffer, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
			pos = 0;
			if  (nbytes <= 0 || buffer[0] == '\0')
				return  NULL;
		}                                                   
		char  *res = &buffer[pos];
		size_t  len = strlen(res) + 1;
		pos += len;
		if  (strncmp(res, prefix, preflen) == 0)
			return  res;
	}       
}
