#include "stdafx.h"
#include "netmsg.h"
#include "spqw.h"

int		plist::pindex(const pident &wi) const FAR
{
	for  (unsigned cnt = 0;	cnt < nptrs;  cnt++)  {
		spptr  *pp = list[cnt];
		if  (pp->spp_netid == wi.remote_id  &&
			 pp->spp_rslot == wi.remote_slot)
			return  cnt;
	}
	return  -1;
}		

int		plist::pindex(const spptr &wj) const FAR
{
	return  pindex(pident(&wj));
}

spptr		*plist::printing(const jident &ji) const FAR
{
	for  (unsigned cnt = 0; cnt < nptrs; cnt++)  {
		spptr	*pp = list[cnt];
		if  (jident(pp->spp_job, pp->spp_rjhostid, pp->spp_rjslot) == ji)
			return  pp;
	}
	return  NULL;
}

spptr		*plist::operator [] (const unsigned ind) const FAR
{
	if  (ind >= nptrs)
		return  NULL;
	return  list[ind];
}

spptr		*plist::operator [] (const pident &wi) const FAR
{
	int	ret = pindex(wi);
	if  (ret < 0)
		return  NULL;
	return  list[ret];
}	

void	plist::checkgrow()
{
	if  (nptrs >= maxptrs)  {
		if  (list)  {
			spptr  **oldlist = list;
			list = new spptr *[maxptrs + INCPTRS];
			VERIFY(list != NULL);
			memcpy((void *)list, (void *) oldlist, maxptrs * sizeof(spptr *));
			delete [] oldlist;
			maxptrs += INCPTRS;
		}
		else  {
			maxptrs = INITPTRS;
			list = new spptr *[INITPTRS];
			VERIFY(list != NULL);
		}
    }
}    
	
void	plist::append(spptr  *p)
{
	checkgrow();
	int  first = 0, last = nptrs;
	while  (first < last)  {
		int  middle = (first + last) / 2;
		spptr  *mp = list[middle];
		if  (mp->spp_netid < p->spp_netid)
			first = middle + 1;
		else  if  (mp->spp_netid > p->spp_netid)
			last = middle;
		else  {
			if  (strcmp(p->spp_ptr, mp->spp_ptr) < 0)
				last = middle;
			else
				first = middle + 1;
		}
	}
	for  (last = nptrs;  last > first;  last--)
		list[last] = list[last-1];
	list[first] = p;
	nptrs++;
}			  

void	plist::remove(const unsigned where)
{
	nptrs--;
	delete list[where];
	for  (unsigned  cnt = where;  cnt < nptrs;  cnt++)
		list[cnt] = list[cnt+1];
	if  (nptrs == 0)  {
		delete [] list;
		list = NULL;
		maxptrs = 0;
	}		
}                                                            

//  These routines are called in response to network operations

static	void	poke(const UINT msg, const unsigned indx = 0)
{
	CWnd	*maw = AfxGetApp()->m_pMainWnd;
	if  (maw)
		maw->SendMessageToDescendants(msg, indx);
}	
		
void	plist::net_pclear(const netid_t hostid)
{
	unsigned  cnt = 0;
	int		  changes = 0;
	while  (cnt < nptrs)
	    if  (list[cnt]->spp_netid == hostid)  {
	        remove(cnt);
	        changes++;                         
	    }               
	    else
	    	cnt++;
	if	(changes)
		poke(WM_NETMSG_PREVISED);
}

void	plist::addptr(const pident &pi, const spptr &pq)
{
	spptr  *newp = new spptr;
	*newp = pq;
	newp->spp_jslot = -1;
	newp->spp_netid = pi.remote_id;
	newp->spp_rslot = pi.remote_slot;
	append(newp);
	poke(WM_NETMSG_PADD);
}

void	plist::changedptr(const pident &pi, const spptr &pq)
{
	int	pnum = pindex(pi);
	if  (pnum < 0)
		return;
	ASSERT(list == Printers().list);
	spptr	*pp = list[pnum];
	slotno_t  origslot = pp->spp_jslot;
	*pp = pq;                   
	ASSERT(list == Printers().list);
	pp->spp_jslot = origslot;
	pp->spp_netid = pi.remote_id;
	pp->spp_rslot = pi.remote_slot;
	ASSERT(list == Printers().list);
	poke(WM_NETMSG_PCHANGE, unsigned(pnum));
}

void	plist::unassign_ptr(const pident &pi, const spptr &pq)
{
	int	pnum = pindex(pi);
	if  (pnum < 0)
		return;
	spptr	*pp = list[pnum];
	pp->spp_rjslot = pp->spp_jslot = -1;
	pp->spp_rjhostid = 0;
	pp->spp_job = 0;
	pp->spp_state = pq.spp_state;
	pp->spp_sflags = pq.spp_sflags;
	pp->spp_dflags = pq.spp_dflags;
	poke(WM_NETMSG_PCHANGE, unsigned(pnum));
}

void	plist::delptr(const pident &pi)
{
	int  where  =  pindex(pi);
	if  (where < 0)
		return;
	remove(unsigned(where));
	poke(WM_NETMSG_PDEL);	
}

// These are "transmitted" messages

void	plist::chgptr(const spptr &ptr)
{
	int	pnum = pindex(ptr);
	if  (pnum >= 0)
		ptr_sendupdate(list[pnum], &ptr, SP_CHGP);
}

void	plist::opptr(const int op, const spptr *ptr)
{
	ptr_message(ptr, op);
}

void	plist::assign(const int jind, const int pind, const jobno_t jobn)
{
	spptr	*cp = list[pind];
	if  (cp)  {
		cp->spp_jslot = jind;
		cp->spp_job = jobn;
		const  spq	 *jp = Jobs()[jind];
		cp->spp_rjslot = jp->spq_rslot;
		cp->spp_rjhostid = jp->spq_netid;
		cp->spp_sflags = SPP_SELECT;
		poke(WM_NETMSG_PCHANGE, unsigned(pind));
	}
}		
		