#include "stdafx.h"
#include "netmsg.h"
#include "spqw.h"

int		joblist::jindex(const jident &wi) const
{
	for  (unsigned cnt = 0;	cnt < njobs;  cnt++)
		if  (jident(list[cnt]) == wi)
			return  cnt;
	return  -1;
}		

int		joblist::jindex(const spq &wj) const
{
	return  jindex(jident(&wj));
}

spq		*joblist::operator [] (const unsigned ind) const
{
	if  (ind >= njobs)
		return  NULL;
	return  list[ind];
}

spq		*joblist::operator [] (const jident &wi) const
{
	int	ret = jindex(wi);
	if  (ret < 0)
		return  NULL;
	return  list[ret];
}	

void	joblist::checkgrow()
{
	if  (njobs >= maxjobs)  {
		if  (list)  {
			spq  **oldlist = list;
			list = new spq *[maxjobs + INCJOBS];
			VERIFY(list != NULL);
			memcpy((void *)list, (void *) oldlist, maxjobs * sizeof(spq *));
			delete [] oldlist;
			maxjobs += INCJOBS;
		}
		else  {
			maxjobs = INITJOBS;
			list = new spq *[INITJOBS];
			VERIFY(list != NULL);
		}
    }
}    
	
void	joblist::append(spq *j)
{
	checkgrow();
	list[njobs++] = j;
}
			  
void	joblist::insert(spq *j, const unsigned where)
{
	checkgrow();
	if  (where >= njobs)
		list[njobs] = j;
	else  {
		for  (int cnt = njobs - 1; cnt >= int(where); cnt--)  {
			list[cnt+1] = list[cnt];                           
			Printers().resetass(cnt, cnt+1);
		}	
		list[where] = j;
	}   
	njobs++;
}	

void	joblist::remove(const unsigned where)
{
	njobs--;
	delete list[where];
	for  (unsigned  cnt = where;  cnt < njobs;  cnt++)  {
		list[cnt] = list[cnt+1];                         
		Printers().resetass(cnt+1, cnt);
	}	
	if  (njobs == 0)  {
		delete [] list;
		list = NULL;
		maxjobs = 0;
	}		
}                                                            

int	joblist::move(const unsigned from, const unsigned to)
{
	if  (from == to)
		return 0;  
	spq  *j = list[from];
	if  (from < to)
		for  (unsigned cnt = from; cnt < to;  cnt++)  {
			list[cnt] = list[cnt+1];                   
			Printers().resetass(cnt+1, cnt);
		}	
	else
		for  (unsigned cnt = from; cnt > to;  cnt--)  {
			list[cnt] = list[cnt-1];				   
			Printers().resetass(cnt-1, cnt);
		}	
	list[to] = j;
	return  1;
}

//  These routines are called in response to network operations

static	void	poke(const UINT msg, const unsigned indx = 0)
{
	CWnd	*maw = AfxGetApp()->m_pMainWnd;
	if  (maw)
		maw->SendMessageToDescendants(msg, indx);
}	
		
void	joblist::net_jclear(const netid_t hostid)
{
	unsigned  cnt = 0;
	int		  changes = 0;
	while  (cnt < njobs)
	    if  (list[cnt]->spq_netid == hostid)  {
	        remove(cnt);
	        changes++;                         
	    }               
	    else
	    	cnt++;
	if	(changes)
		poke(WM_NETMSG_JREVISED);
}

void	joblist::addjob(const jident &ji, const spq &jq)
{
	spq  *newj = new spq;
	VERIFY(newj != NULL);
	memcpy((void *) newj, (void *) &jq, sizeof(spq));
	newj->spq_wpri = jq.spq_pri;
	newj->spq_rslot = ji.remote_slot;
	newj->spq_pslot = -1;
	newj->spq_netid = ji.remote_id;
    
	for  (int  where = njobs - 1;  where >= 0;  where--)  {
		if  (int(list[where]->spq_pri) >= newj->spq_wpri)
			break;
		if  (list[where]->spq_time < newj->spq_time)
			newj->spq_wpri--;
	}                                 
	insert(newj, unsigned(where + 1));
	poke(WM_NETMSG_JADD);
}

void	joblist::changedjob(const jident &ji, const spq &jq)
{
	int	jnum = jindex(ji);
	if  (jnum < 0)
		return;
	spq	*jp = list[jnum];
	unsigned  oldpri = jp->spq_pri;
	slotno_t  oldslot = jp->spq_pslot;
	int	oldwpri = jp->spq_wpri;
	*jp = jq;
	jp->spq_wpri = oldwpri;           
	jp->spq_pslot = oldslot;
	jp->spq_netid = ji.remote_id;
	jp->spq_rslot = ji.remote_slot;

	//  If priority has changed, jiggle job about

	int  pdiff = int(jp->spq_pri) - int(oldpri);
	if  (pdiff == 0)  {
		poke(WM_NETMSG_JCHANGE, unsigned(jnum));
		return;                                  
	}
		
	jp->spq_wpri += pdiff;
	int  changes;
	if  (pdiff > 0)  {	//  Going up........
		for  (int  where = jnum - 1;  where >= 0;  where--)  {
			if  (int(list[where]->spq_pri) >= jp->spq_wpri)
				break;
			jp->spq_wpri--;
		}
		changes = move(unsigned(jnum), unsigned(where + 1));
	}
	else  {		//  Going down....
		for  (unsigned	where = jnum + 1;  where < njobs;  where++)  {
			if  (int(list[where]->spq_pri) <= jp->spq_wpri)
				break;
			jp->spq_wpri++;
		}
		changes = move(unsigned(jnum), where - 1);
	}
	if  (changes)
		poke(WM_NETMSG_JREVISED);
}

void	joblist::unassign(const jident &ji, const spq &job)
{
	int	jnum = jindex(ji);
	if  (jnum < 0)
		return;
	spq	*jp = list[jnum];
	jp->spq_cps = job.spq_cps;
	jp->spq_jflags = job.spq_jflags;
	jp->spq_dflags = job.spq_dflags;
	jp->spq_sflags &= ~(SPQ_ASSIGN|SPQ_PROPOSED|SPQ_ABORTJ);
	jp->spq_haltat = job.spq_haltat;
	jp->spq_pslot = -1;
    poke(WM_NETMSG_JCHANGE, unsigned(jnum));
}

void	joblist::deljob(const jident &ji)
{
	int  where  =  jindex(ji);
	if  (where < 0)
		return;
	remove(unsigned(where));
	poke(WM_NETMSG_JDEL);
}

void	joblist::locpassign(const jident &ji)
{
	int  where = jindex(ji);
	if  (where < 0)
		return;
	spq  *jp = list[where];
	jp->spq_sflags |= SPQ_ASSIGN;
	jp->spq_pslot = -1;
    poke(WM_NETMSG_JCHANGE, unsigned(where));
}

//  These routines are "transmit" rather than "receive"

void	joblist::chgjob(const spq &job)
{
	int	jnum = jindex(job);
	if  (jnum >= 0)
		job_sendupdate(list[jnum], &job, SJ_CHNG);
}

void	joblist::opjob(const int op, const jident &jobref)
{
	int	jnum = jindex(jobref);
	if  (jnum >= 0)
		job_message(jobref.remote_id, list[jnum], op, 0L, 0L);
}

void	joblist::assign(const int jind, const int pind)
{
	spq	*jp = list[jind];
	if  (jp)  {
		jp->spq_pslot = pind;
		jp->spq_sflags |= SPQ_ASSIGN;
		jp->spq_dflags |= SPQ_PQ;	// Cheat really
		poke(WM_NETMSG_JCHANGE, unsigned(jind));
	}	
}

void	joblist::pageupdate(const spq &job)
{
	int	jnum = jindex(job);
	if  (jnum < 0)
		return;
	spq	ljob = *list[jnum];			//  Copy in case something else changed
	ljob.spq_start = job.spq_start;
	ljob.spq_end = job.spq_end;
	ljob.spq_haltat = job.spq_haltat;
	job_sendupdate(list[jnum], &ljob, SJ_CHNG);
}	