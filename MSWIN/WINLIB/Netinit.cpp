/* Netinit.cpp -- initialise network stuff

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

#include "stdafx.h"
#include <stdlib.h>
#include <io.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include "files.h"
#include "resource.h"
#ifdef	SPQW
#include "netmsg.h"
#endif
#ifdef	SPRSERV
#include "netmsg.h"
#endif

remote  *hhashtab[HASHMOD],             // Hash table of host names
		*ahashtab[HASHMOD],				// Hash table of alias names
		*nhashtab[HASHMOD];         	// Hash table of netids

#ifdef  SPQW
remote_queue    current_q,
				pending_q;
short			n_w_probe = 0;			// Number of hosts with probes
#endif

local_params    Locparams;
extern	char	FAR	basedir[];

//  Socket addresses for enquiry port to xtnetserv

sockaddr_in     serv_addr;
		
// Hashing routines for getting net names from ids and vice versa

inline  unsigned  calcnhash(netid_t netid)
{
	unsigned  result = 0;

	for  (int  i = 0;  i < 32;  i += 8)
		result ^= netid >> i;

	return  result % HASHMOD;
}

inline  unsigned  calchhash(const char *hostid)
{
	unsigned  result = 0;
	while  (*hostid)
		result = (result << 1) ^ *hostid++;
	return  result % HASHMOD;
}

inline  char    *stracpy(const char FAR *s)
{
	char    *result = new char [strlen(s)+1];
	strcpy(result, s);
	return  result;
}       

remote::remote(const netid_t nid, const char FAR *name, const char FAR *alias, const unsigned char flags, const unsigned short timeout) :
	   hostid(nid), ht_flags(flags), ht_timeout(timeout)
{
	hh_next = NULL;
	ha_next = NULL;
	hn_next = NULL;
	h_name = stracpy(name);
	if  (alias && strcmp(name, alias) != 0)
		h_alias = stracpy(alias);
	else
		h_alias = NULL;
	sockfd = INVALID_SOCKET;
	is_sync = NSYNC_NONE;
	lastwrite = 0;
#ifdef	SPQW
	bytesleft = 0;
	byteoffset = 0; 
#endif
}                   

void    remote::addhost()
{
	remote  *np, **npp;

	if  (!isdigit(h_name[0]))  {	
		for  (npp = &hhashtab[calchhash(h_name)]; np = *npp; npp = &np->hh_next)
			;            
		*npp = this;
	}
	if  (h_alias)  {
		for  (npp = &ahashtab[calchhash(h_alias)]; np = *npp; npp = &np->ha_next)
			;
		*npp = this;
	}       
	for  (npp = &nhashtab[calcnhash(hostid)]; np = *npp; npp = &np->hn_next)
		;
	*npp = this;
#ifdef  SPRSETW
	Locparams.lportnum++;
#endif
}           

#ifdef SPRSETW
void    remote::delhost()
{
	remote  *np, **npp;
	for  (npp = &hhashtab[calchhash(h_name)];  np = *npp;  npp = &np->hh_next)
		if  (np == this)  {
			*npp = np->hh_next;
			break;
		}
	if  (h_alias)  {
		for  (npp = &ahashtab[calchhash(h_alias)]; np = *npp; npp = &np->ha_next)
		if  (np == this)  {
			*npp = np->ha_next;
			break;
		}
	}
	for  (npp = &nhashtab[calcnhash(hostid)]; np = *npp; npp = &np->hn_next)
		if  (np == this)  {
			*npp = np->hn_next;
			break;
		}                                       
	Locparams.lportnum--;
}

BOOL    clashcheck(const char FAR *name)
{
	remote  *rp;
	for  (rp = hhashtab[calchhash(name)];  rp;  rp = rp->hh_next)
		if  (strcmp(name, rp->h_name) == 0)
			return  TRUE;
	for  (rp = ahashtab[calchhash(name)];  rp;  rp = rp->ha_next)
		if  (strcmp(name, rp->h_alias) == 0)
			return  TRUE;
	return  FALSE;
}       

BOOL    clashcheck(const netid_t hid)
{
	remote  *rp;
	for  (rp = nhashtab[calcnhash(hid)];  rp;  rp = rp->hn_next)
		if  (rp->hostid == hid)
			return  TRUE;
	return  FALSE;
}                                 
#endif  // SPRSETW

const char      *look_host(const netid_t nid)
{
	for  (remote *np = nhashtab[calcnhash(nid)]; np;  np = np->hn_next)
		if  (nid == np->hostid)
			return  np->h_alias? np->h_alias: np->h_name;
	if  (nid == Locparams.myhostid)
		return  "(local)";
	return  "unknown";
}

remote  *find_host(const netid_t nid)
{
	for  (remote *np = nhashtab[calcnhash(nid)];  np;  np = np->hn_next)
		if  (nid == np->hostid)
			return  np;
	return  (remote *) 0;
}

#ifdef	SPQW
remote  *find_host(const SOCKET sock)
{
	return  current_q.find(sock);
}
#endif       

netid_t look_hname(const char *name)
{
	for  (remote *np = hhashtab[calchhash(name)]; np;  np = np->hh_next)
		if  (strcmp(name, np->h_name) == 0)
			return  np->hostid;
	for  (np = ahashtab[calchhash(name)]; np;  np = np->ha_next)
		if  (strcmp(name, np->h_alias) == 0)
			return  np->hostid;
	return  -1L;
}                                                  

#ifdef  SPQW
unsigned        remote_queue::alloc(remote *rp)
{
	for  (unsigned  number = 0;  number < max;  number++)
		if  (!list[number])  {
			list[number] = rp;
			return  number;
		}
	unsigned  oldmax = max;
	max += INC_REMOTES;
	remote  **newlist = new remote * [max];
	if  (oldmax > 0)  {
		memcpy((void *) newlist, (void *) list, oldmax * sizeof(remote *));
		delete [] list;
	}
	memset((void *) &newlist[oldmax], '\0', INC_REMOTES * sizeof(remote *));
	list = newlist;
	list[oldmax] = rp;
	return  oldmax;
}

remote  *remote_queue::find(const netid_t netid)
{
	for  (unsigned  cnt = 0;  cnt < max;  cnt++)
		if  (list[cnt] && list[cnt]->hostid == netid)
			return  list[cnt];
	return  (remote *) 0;
}

remote  *remote_queue::find(const SOCKET sockfd)
{
	for  (unsigned  cnt = 0;  cnt < max;  cnt++)
		if  (list[cnt] && list[cnt]->sockfd == sockfd)
			return  list[cnt];
	return  (remote *) 0;
}

unsigned        remote_queue::index(const remote * const rp)
{
	for  (unsigned  cnt = 0;  cnt < max;  cnt++)
		if  (list[cnt] == rp)
			return  cnt;
	return  max;
}

void    remote_queue::free(const remote * const rp)
{
	unsigned  ind = index(rp);
	if  (ind < max)
		list[ind] = (remote *) 0;       
}

remote  *remote_queue::operator [] (const unsigned ind)
{
	if  (ind < max)
		return  list[ind];
	return  (remote *) 0;
}
#endif

#ifdef  SPRSETW
//  Get nth host in list (assuming list is small)

remote  *get_nth(const unsigned n)
{
	unsigned  reached = 0;
	for  (unsigned  cnt = 0;  cnt < HASHMOD;  cnt++)
		for  (remote *np = nhashtab[cnt];  np;  np = np->hn_next)
			if  (reached++ == n)
				return  np;
	return  (remote *) 0;
}
#endif

//      Maximum number of bits we are prepared to parse \xitext\hosts into.

#define MAXPARSE        6

//      Split string into bits in result using delimiters given.
//      Ignore bits after MAXPARSE-1
//      Assume string is manglable

static  int     spliton(char **result, char *string, const char *delims)
{
	int     parsecnt = 1, resc = 1;

	//      Assumes no leading delimiters

	result[0] = string;
	while  (string = strpbrk(string, delims))  {
		*string++ = '\0';
		while  (strchr(delims, *string))
			string++;
		if  (!*string)
			break;
		result[parsecnt] = string;
		++resc;
		if  (++parsecnt >= MAXPARSE-1)
			break;
	}
	while  (parsecnt < MAXPARSE)
		result[parsecnt++] = (char *) 0;
	return  resc;
}

static  char FAR *shortestalias(hostent FAR *hp)
{
	char    FAR *which = (char FAR *) 0;
	int     minlen = 1000, ln;

	for  (char FAR * FAR *ap = hp->h_aliases; *ap; ap++)
		if  ((ln = strlen(*ap)) < minlen)  {
			minlen = ln;
			which = *ap;
		}
	if  (minlen < int(strlen(hp->h_name)))
		return  which;
	return  (char FAR *) 0;
}

//  Open host file and initialise list.
//  Return 0 - OK
//  Otherwise resource ID of error string.
//  Nasty side effect - bodge Locparams.servid

UINT    loadhostfile()
{
	UINT	lastret = 0;
	ifstream  hfile;
	char	hpath[_MAX_PATH];
	strcpy(hpath, basedir);
	strcat(hpath, HOSTFILE);
	hfile.open(hpath, ios::in | ios::nocreate);
	if  (!hfile.good())
		return  IDP_NO_HOSTFILE;

	//      Loop until we find something interesting

	char    line[100];
	do  {
		hfile.getline(line, sizeof(line));

		//      Ignore leading white space and skip comment lines starting with #

		for  (char  *hostp = line;  isspace(*hostp);  hostp++)
			;
		if  (!*hostp  ||  *hostp == '#')
			continue;

		//      Split line on white space.

		char    *bits[MAXPARSE];                      
		remote  *newrem;
		
		if  (spliton(bits, hostp, " \t") < 2)  {
			hostent  FAR *hp;                         
			netid_t nid;                              
			
			//      Reject entry if it is unknown or is "me"

			if  (!(hp = gethostbyname(bits[HOSTF_HNAME]))  ||
			    (nid = *(netid_t FAR *) hp->h_addr) == 0L  ||
			    nid == -1L  ||
				nid == Locparams.myhostid)  {
				lastret = IDP_HF_INVALIDHOST;
				continue;
			}
			
			newrem = new remote(nid, hp->h_name, shortestalias(hp));
		}       
		else  {
			unsigned  totim = NETTICKLE;
			unsigned  char  serv_flag = 0;
			char    FAR *alias = NULL;

			//      Alias name of - means no alias

			if  (strcmp(bits[HOSTF_ALIAS], "-") != 0)
				alias = bits[HOSTF_ALIAS];

			//      Check timeout time

			if  (bits[HOSTF_TIMEOUT])  {
				if  (!isdigit(bits[HOSTF_TIMEOUT][0]))
					return  lastret = IDP_HF_BADTIMEOUT;
				long  tot = atol(bits[HOSTF_TIMEOUT]);
				if  (tot <= 0 || tot > 30000)  {
					if  (lastret == 0)
						lastret = IDP_HF_BADTIMEOUT;
					continue;
				}
				totim = (unsigned) tot;
			}

			//      Parse flags - currently only "server" and "probe" are supported

			if  (bits[HOSTF_FLAGS])  {
				char    *bitsf[MAXPARSE];
				spliton(bitsf, bits[HOSTF_FLAGS], ",");
				for  (char **fp = bitsf;  *fp;  fp++)
					if  (_stricmp(*fp, "server") == 0)
						serv_flag |= HT_SERVER;
					else  if  (_stricmp(*fp, "probe") == 0)  {
						serv_flag |= HT_PROBEFIRST;
#ifdef	SPQW
						n_w_probe++;
#endif
					}
			}

			hostent  FAR *hp;
			netid_t nid;
			
			//  Allow host name to be given as internet address
			
			if  (isdigit(bits[HOSTF_HNAME][0]))  {
				nid = inet_addr(hostp);
				if  (nid == -1L || nid == 0L || nid == Locparams.myhostid)
					continue;
				hp = gethostbyaddr((char *)&nid, sizeof(long), AF_INET);
				if  (!hp)  {
					if  (!alias)  {
						lastret = IDP_HF_NOALIAS;
						continue;
					}
					newrem = new remote(nid, alias, NULL, serv_flag, totim);
				}
				else
					newrem = new remote(nid, hp->h_name, alias? alias: shortestalias(hp), serv_flag, totim);
			}
			else  {
				//      Reject entry if it is unknown or is "me"

				if  (!(hp = gethostbyname(bits[HOSTF_HNAME]))  ||
				    (nid = *(netid_t FAR *) hp->h_addr) == 0L  ||
				    nid == -1L  ||
					nid == Locparams.myhostid)  {
					lastret = IDP_HF_INVALIDHOST;
					continue;
				}
					
				newrem = new remote(nid, bits[HOSTF_HNAME], alias? alias: shortestalias(hp), serv_flag, totim);
			}          
			
			//  NASTY SIDE EFFECT HERE!!!
			
			if  (serv_flag & HT_SERVER)  {
				Locparams.servid = nid;
				Locparams.servtimeout = newrem->ht_timeout;
			}
		}
		newrem->addhost();
	}  while  (hfile.good());               
				       
	return  lastret;
}

#ifdef SPRSETW
void    savehostfile()
{
	ofstream  hfile;
	char	hpath[_MAX_PATH];
	strcpy(hpath, basedir);
	strcat(hpath, HOSTFILE);
	hfile.open(hpath, ios::out);
	if  (!hfile.good())
		return;
	for  (unsigned  cnt = 0;  cnt < HASHMOD;  cnt++)
		for  (remote *np = nhashtab[cnt];  np;  np = np->hn_next)  {
		hfile << np->hostname() << '\t';
			if  (np->aliasname()[0])
				hfile << np->aliasname();
			else
				hfile << '-';
			hfile << '\t';
			if  (np->ht_flags & HT_PROBEFIRST)  {
				hfile << "probe";
				if  (np->ht_flags & HT_SERVER)
					hfile << ",server";
			}
			else  if (np->ht_flags & HT_SERVER)
				hfile << "server";
			else
				hfile << '-';
			hfile << '\t' << np->ht_timeout << endl;
		}
}

#endif  //SPRSETW

//  Windows sockets doesn't provide this routine...
//  Also get services and protocols don't work.

static  netid_t WSgethostid()
{
	char    myname[20];
	gethostname(myname, sizeof(myname));
	hostent FAR *hp;
	hp = gethostbyname(myname);
	return  hp? *(netid_t FAR *) hp->h_addr : -1L;
}

//  Initialise enquiry socket, but we won't necessarily know that
//  we can't talk until we try.

UINT	initenqsocket(const netid_t hostid)
{
	sockaddr_in	cli_addr;

	if  (Locparams.uasocket != INVALID_SOCKET)  {
		closesocket(Locparams.uasocket);
		Locparams.uasocket = INVALID_SOCKET;
	}
	memset((void *) &serv_addr, '\0', sizeof(serv_addr));
	memset((void *) &cli_addr, '\0', sizeof(cli_addr));
	serv_addr.sin_family = cli_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = hostid;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = Locparams.uaportnum;
	SOCKET  sockfd;
	if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
		return  IDP_GSPU_NOSOCK;
	if  (bind(sockfd, (struct sockaddr FAR *) &cli_addr, sizeof(cli_addr)) < 0)  {
		closesocket(sockfd);
		return  IDP_GSPU_NOBIND;
	}
	Locparams.uasocket = sockfd;
	time(&Locparams.tlastop);
	return  0;
}
	
#ifdef  SPQW
UINT    initsockets()
{
	if  (n_w_probe <= 0)
		return  0;
	sockaddr_in     sin;    
	if  ((Locparams.probesock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return  IDP_SK_CCPROBESOCK;
	sin.sin_family = AF_INET;
	sin.sin_port = Locparams.pportnum;
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;
	if  (bind(Locparams.probesock, (sockaddr *) &sin, sizeof(sin)) < 0)  {
		closesocket(Locparams.probesock);
		Locparams.probesock = INVALID_SOCKET;
		return  IDP_SK_CCPROBESOCK;
	}
//	HWND mw = AfxGetApp()->m_pMainWnd->m_hWnd;
//	if  (WSAAsyncSelect(Locparams.probesock, mw, WM_NETMSG_PROBERCV, FD_READ) != 0)
//		AfxMessageBox(IDP_PROBEASYNCSEL, MB_OK|MB_ICONSTOP);    
	return  0;
}
#endif // SPQW

#ifdef	SPRSERV
UINT	initsockets()
{
	sockaddr_in	sin;
	if  ((Locparams.probesock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)  {
		int	ret = WSAGetLastError();
		return  IDP_SK_CCMSGSOCK;
	}
	sin.sin_family = AF_INET;
	sin.sin_port = Locparams.uaportnum;
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;
	if  (bind(Locparams.probesock, (sockaddr *) &sin, sizeof(sin)) < 0)  {
		int	ret = WSAGetLastError();
		closesocket(Locparams.probesock);
		Locparams.probesock = INVALID_SOCKET;
		return  IDP_SK_CCMSGSOCK;
	}
	HWND mw = AfxGetApp()->m_pMainWnd->m_hWnd;
	if  (WSAAsyncSelect(Locparams.probesock, mw, WM_NETMSG_MESSRCV, FD_READ) != 0)  {
		int	ret = WSAGetLastError();
		closesocket(Locparams.probesock);
		Locparams.probesock = INVALID_SOCKET;
		return  IDP_MSGASYNCSEL;    
	}
	return  0;              
}
#endif	//  SPRSERV

// Initialise windows.
// Return 0 if OK, otherwise return a string resource ID
// ready for plonking into an AfxMessageBox call.

UINT    winsockstart()
{
	WSADATA wd;
	WORD    vr = 0x0101;    //  Major/minor 1/1
	int rets = WSAStartup(vr, &wd);
	if  (rets != 0)  {
		char	moan[80];
		wsprintf(moan, "Startup gave error %d", rets);
		AfxMessageBox(moan, MB_OK|MB_ICONSTOP);
	}
#ifdef SPRSETW
	Locparams.lportnum = 0;         // Used as count
#endif
	Locparams.myhostid = WSgethostid();
	UINT  ret = loadhostfile();
	if  (ret)
		return  ret;
	if  (Locparams.servid == 0)
		return  IDP_NO_SERVER;
	return  0;
}

void    winsockend()
{
	if  (Locparams.probesock != INVALID_SOCKET)  {
		closesocket(Locparams.probesock);       
		Locparams.listsock = Locparams.probesock = INVALID_SOCKET;
	}       
	WSACleanup();
}
