// jobdoc.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "spqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "jobview.h"
#include "formdlg.h"
#include "pagedlg.h"
#include "userdlg.h"
#include "retndlg.h"
#include "secdlg22.h"
#include "jpsearch.h"
#include "jdatadoc.h"
#include <string.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#if	_MSC_VER == 700
#define UNIXTODOSTIME	((70UL * 365UL + 68/4 + 1) * 24UL * 3600UL)
#else
#define	UNIXTODOSTIME	0
#endif

BOOL  Smstr(const CString &, const char *, const BOOL = FALSE);

/////////////////////////////////////////////////////////////////////////////
// CJobdoc

IMPLEMENT_DYNCREATE(CJobdoc, CDocument)

CJobdoc::CJobdoc()
{
	m_sformtype = m_sjtitle = m_sprinter = m_suser = TRUE;
	m_swraparound = FALSE;
	CSpqwApp *mw = (CSpqwApp *)AfxGetApp();
	CMainFrame	*mf = (CMainFrame *) mw->m_pMainWnd;
	m_wrestrict = mf->m_restrictlist;
	revisejobs(mw->m_appjoblist);
}

CJobdoc::~CJobdoc()
{
}

BEGIN_MESSAGE_MAP(CJobdoc, CDocument)
	//{{AFX_MSG_MAP(CJobdoc)
	ON_COMMAND(ID_JOBS_FORMANDCOPIES, OnJobsFormandcopies)
	ON_COMMAND(ID_JOBS_CLASSCODES, OnJobsClasscodes)
	ON_COMMAND(ID_ACTION_ANOTHERCOPY, OnActionAnothercopy)
	ON_COMMAND(ID_ACTION_ABORTJOB, OnActionAbortjob)
	ON_COMMAND(ID_JOBS_PAGES, OnJobsPages)
	ON_COMMAND(ID_JOBS_RETENTION, OnJobsRetention)
	ON_COMMAND(ID_JOBS_USERANDMAIL, OnJobsUserandmail)
	ON_COMMAND(ID_JOBS_VIEWJOB, OnJobsViewjob)
	ON_COMMAND(ID_SEARCH_SEARCHBACKWARDS, OnSearchSearchbackwards)
	ON_COMMAND(ID_SEARCH_SEARCHFOR, OnSearchSearchfor)
	ON_COMMAND(ID_SEARCH_SEARCHFORWARD, OnSearchSearchforward)
	ON_COMMAND(ID_WINDOW_WINDOWOPTIONS, OnWindowWindowoptions)
	ON_COMMAND(ID_JOBS_UNQUEUEJOB, OnJobsUnqueuejob)
	ON_COMMAND(ID_JOBS_COPYJOB, OnJobsCopyjob)
	ON_COMMAND(ID_JOBS_COPYOPTIONSINJOB, OnJobsCopyoptionsinjob)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void	CJobdoc::revisejobs(joblist &src)
{
	m_joblist.clear();                  
	for  (unsigned cnt = 0; cnt < src.number(); cnt++)  {
		spq	*j = src[cnt];
		if  (m_wrestrict.visible(*j))
			m_joblist.append(j);
	}
	UpdateAllViews(NULL);
}
		 
spq	*CJobdoc::GetSelectedJob(const BOOL moan)
{                 
	POSITION	p = GetFirstViewPosition();
	CJobView	*aview = (CJobView *)GetNextView(p);
	if  (aview)  {
		int	cr = aview->GetActiveRow();
		if  (cr >= 0)  {
			spq	 *result = (*this)[cr];
			if  (moan  &&  strcmp(((CSpqwApp *) AfxGetApp())->m_username, result->spq_uname) != 0)  {
				spdet  &mypriv = ((CSpqwApp *) AfxGetApp())->m_mypriv;
				if  (!mypriv.ispriv(PV_OTHERJ))  {
					AfxMessageBox(IDP_NOTYOURJ, MB_OK|MB_ICONSTOP);
					return  NULL;
				}
			}
			return  result;
		}
	}
	if  (moan)
		AfxMessageBox(IDP_NOJOBSELECTED, MB_OK|MB_ICONEXCLAMATION);
	return  NULL;
}			

void CJobdoc::OnJobsFormandcopies()
{   
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;
	CFormdlg	fdlg;
	fdlg.m_copies = cj->spq_cps;
	fdlg.m_formtype = cj->get_spq_form();
	fdlg.m_header = cj->get_spq_file();
	fdlg.m_printer = cj->get_spq_ptr();
	fdlg.m_priority = cj->spq_pri;
	fdlg.m_supph = cj->spq_jflags & SPQ_NOH? TRUE: FALSE;
	if  (fdlg.DoModal() == IDOK)  {
		spq		newj = *cj;
		newj.spq_cps = fdlg.m_copies;
		strcpy(newj.spq_form, fdlg.m_formtype);
		strcpy(newj.spq_file, fdlg.m_header);
		strcpy(newj.spq_ptr, fdlg.m_printer);
		newj.spq_pri = fdlg.m_priority;
		if  (fdlg.m_supph)
			newj.spq_jflags |= SPQ_NOH;
		else
			newj.spq_jflags &= ~SPQ_NOH;
		Jobs().chgjob(newj);
	}		
}	

void CJobdoc::OnJobsClasscodes()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;
	CSecdlg22 sdlg;
	sdlg.m_classc = cj->spq_class;
	spdet	&mypriv = ((CSpqwApp *)AfxGetApp())->m_mypriv;
	sdlg.m_maxclass = mypriv.spu_class;
	sdlg.m_mayoverride = mypriv.ispriv(PV_COVER);	
	sdlg.m_localonly = cj->spq_jflags & SPQ_LOCALONLY? TRUE: FALSE;
	if  (sdlg.DoModal() == IDOK)  {
		spq		newj = *cj;
		newj.spq_class = sdlg.m_classc;
		if  (sdlg.m_localonly)
			newj.spq_jflags |= SPQ_LOCALONLY;
		else
			newj.spq_jflags &= ~SPQ_LOCALONLY;
		Jobs().chgjob(newj);
	}
}

void CJobdoc::OnActionAnothercopy()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;
	spdet	&mypriv = ((CSpqwApp *)AfxGetApp())->m_mypriv;
	if  (cj->spq_cps < mypriv.ispriv(PV_ANYPRIO)? 255: mypriv.spu_cps)	{
		spq		newj = *cj;
		newj.spq_cps++;
		Jobs().chgjob(newj);
	}
}		

void CJobdoc::OnActionAbortjob()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;
	int  confirm = ((CSpqwApp *) AfxGetApp())->m_options.spq_options.confabort;
	UINT  msgcode = 0;
	if  (confirm > 1)
		msgcode = cj->spq_dflags & SPQ_PRINTED? IDP_ACONFA_PRINTED: IDP_ACONFA_NPRINTED;
	else  if  (confirm > 0  &&  !(cj->spq_dflags & SPQ_PRINTED))
		msgcode = IDP_CCONFA_NPRINTED;
	if  (msgcode == 0  ||  AfxMessageBox(msgcode, MB_OKCANCEL|MB_ICONQUESTION) == IDOK)
		Jobs().opjob(SO_AB, jident(cj));
}

void CJobdoc::OnJobsPages()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;     
	CPagedlg	pdlg;
	pdlg.m_ppflags = cj->get_spq_flags();
	pdlg.m_startp = cj->spq_start;
	pdlg.m_endp = cj->spq_end;
	pdlg.m_hatp = cj->spq_haltat;
	pdlg.m_jflags = cj->spq_jflags;
	if  (pdlg.DoModal() == IDOK)  {
		spq  newj = *cj;
		strcpy(newj.spq_flags, pdlg.m_ppflags);
		newj.spq_start = pdlg.m_startp;
		newj.spq_end = pdlg.m_endp;
		newj.spq_haltat = pdlg.m_hatp;
		newj.spq_jflags = pdlg.m_jflags;
		Jobs().chgjob(newj);
	}
}

void CJobdoc::OnJobsRetention()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;
	CRetndlg	rdlg;
	rdlg.m_dinp = cj->spq_nptimeout;
	rdlg.m_dip = cj->spq_ptimeout;
	rdlg.m_hold = cj->spq_hold != 0;
	rdlg.m_printed = cj->spq_dflags & SPQ_PRINTED? TRUE: FALSE;
	rdlg.m_retain = cj->spq_jflags & SPQ_RETN? TRUE: FALSE;	
	rdlg.m_holdtime = cj->spq_hold + UNIXTODOSTIME;
	if  (rdlg.DoModal() == IDOK)  {
		spq	newj = *cj;
	    newj.spq_nptimeout = rdlg.m_dinp;
	    newj.spq_ptimeout = rdlg.m_dip;
	    if  (rdlg.m_hold)
	    	newj.spq_hold = rdlg.m_holdtime - UNIXTODOSTIME;
	    else
	    	newj.spq_hold = 0;
	    if  (rdlg.m_printed)
	    	newj.spq_dflags |= SPQ_PRINTED;
	    else
	    	newj.spq_dflags &= ~SPQ_PRINTED;
	    if  (rdlg.m_retain)
	    	newj.spq_jflags |= SPQ_RETN;	
	    else	
	    	newj.spq_jflags &= ~SPQ_RETN;
		Jobs().chgjob(newj);
	}	
}

void CJobdoc::OnJobsUserandmail()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;     
	CUserdlg  udlg;
	udlg.m_mail = cj->spq_jflags & SPQ_MAIL? TRUE: FALSE;
	udlg.m_write = cj->spq_jflags & SPQ_WRT? TRUE: FALSE;
	udlg.m_mattn = cj->spq_jflags & SPQ_MATTN? TRUE: FALSE;
	udlg.m_wattn = cj->spq_jflags & SPQ_WATTN? TRUE: FALSE;
	udlg.m_user = strcmp(cj->spq_uname, cj->spq_puname) == 0? "" : cj->spq_puname;
	if  (udlg.DoModal() == IDOK)  { 
		spq		newj = *cj;
		if  (udlg.m_user.IsEmpty())
			strcpy(newj.spq_puname, newj.spq_uname);
		else
			strcpy(newj.spq_puname, udlg.m_user);
		newj.spq_jflags &= ~(SPQ_MAIL|SPQ_WRT|SPQ_MATTN|SPQ_WATTN);
		if  (udlg.m_mail)
			newj.spq_jflags |= SPQ_MAIL;
		if  (udlg.m_write)
			newj.spq_jflags |= SPQ_WRT;
		if  (udlg.m_mattn)
			newj.spq_jflags |= SPQ_MATTN;
		if  (udlg.m_wattn)
			newj.spq_jflags |= SPQ_WATTN;		
		Jobs().chgjob(newj);
	}
}
	
BOOL CJobdoc::Smatches(const CString str, const int ind)
{
	spq  *jb = (*this)[ind];
	if  (m_sformtype && Smstr(str, jb->spq_form))
		return  TRUE;
	if  (m_sjtitle && Smstr(str, jb->spq_file))
		return  TRUE;
	if  (m_sprinter && Smstr(str, jb->spq_ptr))
		return  TRUE;
	if  (m_suser && Smstr(str, jb->spq_uname, TRUE))
		return  TRUE;
	return FALSE;
}	

void CJobdoc::DoSearch(const CString str, const BOOL forward)
{             
	int	cnt;
	spq  *cj = GetSelectedJob(FALSE);
	if  (forward)  {
		int	cwhich = cj? jindex(cj): -1;
		for  (cnt = cwhich + 1;  cnt < int(number());  cnt++)
			if  (Smatches(str, cnt))
				goto  gotit;
		if  (m_swraparound)
			for  (cnt = 0;  cnt < cwhich;  cnt++)
				if  (Smatches(str, cnt))
					goto  gotit;
	}
	else  {
		int  cwhich = cj? jindex(cj): number();
		for  (cnt = cwhich - 1;  cnt >= 0;  cnt--)
			if  (Smatches(str, cnt))
				goto  gotit;
		if  (m_swraparound)
			for  (cnt = number() - 1;  cnt > cwhich;  cnt--)
				if  (Smatches(str, cnt))
					goto  gotit;
	}
	AfxMessageBox(IDP_NOTFOUND, MB_OK|MB_ICONEXCLAMATION);
	return;
	
gotit:							
	POSITION	p = GetFirstViewPosition();
	CJobView	*aview = (CJobView *)GetNextView(p);
	if  (aview)  {
		aview->ChangeSelectionToRow(cnt);		
		aview->UpdateRow(cnt);
	    UpdateAllViews(NULL);
	}    
}	
	
void CJobdoc::OnSearchSearchbackwards()
{
	CString  sstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_jlastsearch;
	if  (sstr.IsEmpty())  {
		AfxMessageBox(IDP_NOSEARCHSTR, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	DoSearch(sstr, FALSE);
}

void CJobdoc::OnSearchSearchfor()
{
	CJPSearch	sdlg;
	sdlg.m_sstring = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_jlastsearch;
	sdlg.m_sdevice = FALSE;
	sdlg.m_sformtype = m_sformtype;
	sdlg.m_sjtitle = m_sjtitle;
	sdlg.m_sprinter = m_sprinter;
	sdlg.m_suser = m_suser;
	sdlg.m_which = CJPSearch::IDD_JOBSRCH;
	sdlg.m_sforward = 0;
	sdlg.m_swraparound = m_swraparound;
	if  (sdlg.DoModal() != IDOK)
		return;
	((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_jlastsearch = sdlg.m_sstring;
	m_sformtype = sdlg.m_sformtype;
	m_sjtitle = sdlg.m_sjtitle;
	m_sprinter = sdlg.m_sprinter;
	m_suser = sdlg.m_suser;
	m_swraparound = sdlg.m_swraparound;
	DoSearch(sdlg.m_sstring, sdlg.m_sforward == 0);
}

void CJobdoc::OnSearchSearchforward()
{
	CString  sstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_jlastsearch;
	if  (sstr.IsEmpty())  {
		AfxMessageBox(IDP_NOSEARCHSTR, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	DoSearch(sstr, TRUE);		
}

void CJobdoc::OnJobsViewjob()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;
	CSpqwApp *mw = (CSpqwApp *)AfxGetApp();
	CMainFrame	*mf = (CMainFrame *) mw->m_pMainWnd;
	mf->OnNewJDWin(cj);
}

void CJobdoc::OnWindowWindowoptions()
{
	CSpqwApp *mw = (CSpqwApp *)AfxGetApp();
	CMainFrame	*mf = (CMainFrame *) mw->m_pMainWnd;
	if  (mf->DoRstrDlg(m_wrestrict))
		revisejobs(mw->m_appjoblist);
}

void CJobdoc::OnJobsUnqueuejob()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;
	spq	jcopy = *cj;
	jcopy.unqueue();
}

void CJobdoc::OnJobsCopyjob()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;   
	spq	jcopy = *cj;
	jcopy.unqueue(FALSE);
}

void CJobdoc::OnJobsCopyoptionsinjob()
{
	spq	*cj = GetSelectedJob();
	if  (!cj)
		return;        
	cj->optcopy();
}
