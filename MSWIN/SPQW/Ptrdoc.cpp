// ptrdoc.cpp : implementation file
//

#include "stdafx.h"
#include "ptrdoc.h"
#include "formatcode.h"
#include "spqw.h"
#include "rowview.h"
#include "ptrview.h"
#include "mainfrm.h"
#include "secdlg22.h"
#include "prform.h"
#include "jpsearch.h"
#include <string.h>

BOOL  Smstr(const CString &, const char *, const BOOL = FALSE);

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPtrdoc

IMPLEMENT_DYNCREATE(CPtrdoc, CDocument)

CPtrdoc::CPtrdoc()
{
	m_sdevice = m_sformtype = m_sprinter = TRUE;
	m_swraparound = FALSE;
	CSpqwApp *mw = (CSpqwApp *)AfxGetApp();
	CMainFrame	*mf = (CMainFrame *) mw->m_pMainWnd;
	m_wrestrict = mf->m_restrictlist;
	m_ptrcolours = mw->m_appcolours;
	reviseptrs(mw->m_appptrlist);
}

CPtrdoc::~CPtrdoc()
{
}

BEGIN_MESSAGE_MAP(CPtrdoc, CDocument)
	//{{AFX_MSG_MAP(CPtrdoc)
	ON_COMMAND(ID_ACTION_DISAPPROVEALIGNMENT, OnActionDisapporvealignment)
	ON_COMMAND(ID_PRINTERS_CLASSCODES, OnPrintersClasscodes)
	ON_COMMAND(ID_ACTION_GOPRINTER, OnActionGoprinter)
	ON_COMMAND(ID_ACTION_HALTATONCE, OnActionHaltatonce)
	ON_COMMAND(ID_ACTION_HALTEOJ, OnActionHalteoj)
	ON_COMMAND(ID_ACTION_OKALIGNMENT, OnActionOkalignment)
	ON_COMMAND(ID_PRINTERS_FORMTYPE, OnPrintersFormtype)
	ON_COMMAND(ID_PRINTERS_INTERRUPT, OnPrintersInterrupt)
	ON_COMMAND(ID_SEARCH_SEARCHFOR, OnSearchSearchfor)
	ON_COMMAND(ID_SEARCH_SEARCHFORWARD, OnSearchSearchforward)
	ON_COMMAND(ID_SEARCH_SEARCHBACKWARDS, OnSearchSearchbackwards)
	ON_COMMAND(ID_WINDOW_WINDOWOPTIONS, OnWindowWindowoptions)
	ON_COMMAND(ID_FILE_WPCOLOUR_AWOPER, OnFileWpcolourAwoper)
	ON_COMMAND(ID_FILE_WPCOLOUR_ERROR, OnFileWpcolourError)
	ON_COMMAND(ID_FILE_WPCOLOUR_HALTED, OnFileWpcolourHalted)
	ON_COMMAND(ID_FILE_WPCOLOUR_IDLE, OnFileWpcolourIdle)
	ON_COMMAND(ID_FILE_WPCOLOUR_OFFLINE, OnFileWpcolourOffline)
	ON_COMMAND(ID_FILE_WPCOLOUR_PRINTING, OnFileWpcolourPrinting)
	ON_COMMAND(ID_FILE_WPCOLOUR_SHUTD, OnFileWpcolourShutd)
	ON_COMMAND(ID_FILE_WPCOLOUR_STARTUP, OnFileWpcolourStartup)
	ON_COMMAND(ID_FILE_PCOLOUR_COPYTOPROG, OnFilePcolourCopytoprog)
	ON_COMMAND(ID_FILE_PCOLOUR_COPYTOWIN, OnFilePcolourCopytowin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
    
static	int	verboten(const unsigned perm, const UINT msg)
{
	if  (((CSpqwApp *)AfxGetApp())->m_mypriv.ispriv(perm))
		return  0;
	AfxMessageBox(msg, MB_OK|MB_ICONSTOP);
	return  1;
}
	
void	CPtrdoc::reviseptrs(plist FAR &src)
{
	m_ptrlist.clear();                  
	for  (unsigned cnt = 0; cnt < src.number(); cnt++)  {
		spptr	*p = src[cnt];
		if  (m_wrestrict.visible(*p))
			m_ptrlist.append(p);
	}
	UpdateAllViews(NULL);
}

spptr	*CPtrdoc::GetSelectedPtr(const BOOL moan)
{                 
	POSITION	p = GetFirstViewPosition();
	CPtrView	*aview = (CPtrView *)GetNextView(p);
	if  (aview)  {
		int	cr = aview->GetActiveRow();
		if  (cr >= 0)  {
			spptr	*result = (*this)[cr];
			if  (moan)  {
				spdet  &mypriv = ((CSpqwApp *) AfxGetApp())->m_mypriv;
				if  (!mypriv.ispriv(PV_PRINQ))  {
					AfxMessageBox(IDP_NOPRINQ, MB_OK|MB_ICONSTOP);
					return  NULL;
				}
			}
			return  result;
		}	
	}
	if  (moan)
		AfxMessageBox(IDP_NOPTRSELECTED, MB_OK|MB_ICONEXCLAMATION);
	return  NULL;
}			

void CPtrdoc::OnActionDisapporvealignment()
{
	spptr	*cp = GetSelectedPtr();
	if  (!cp)
		return;
    if  (cp->spp_state != SPP_OPER)  {
    	if  (cp->spp_state != SPP_WAIT)
    		return;
    	if  (AfxMessageBox(IDP_CONFIRM_REINSTALIGN, MB_OKCANCEL|MB_ICONQUESTION) != IDOK)
    		return;                                                     
    }
    Printers().opptr(SO_ONO, cp);   	
}

void CPtrdoc::OnPrintersClasscodes()
{
	spptr	*cp = GetSelectedPtr();
	if  (!cp)
		return;
	if  (verboten(PV_ADDDEL, IDP_PRIN_CANTCCC))
		return;
	if  (cp->spp_state >= SPP_PROC)  {
		AfxMessageBox(IDP_PRIN_RUNNING, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	CSecdlg22 sdlg;
	sdlg.m_classc = cp->spp_class;
	spdet  &mypriv = ((CSpqwApp *)AfxGetApp())->m_mypriv;
	sdlg.m_maxclass = mypriv.spu_class;
	sdlg.m_mayoverride = mypriv.ispriv(PV_COVER);	
	sdlg.m_localonly = cp->spp_netflags & SPP_LOCALONLY? TRUE: FALSE;
	if  (sdlg.DoModal() == IDOK)  {
		spptr  newp = *cp;
		newp.spp_class = sdlg.m_classc;
		if  (sdlg.m_localonly)
			newp.spp_netflags |= SPP_LOCALONLY;
		else
			newp.spp_netflags &= ~SPP_LOCALONLY;
		Printers().chgptr(newp);
	}
}

void CPtrdoc::OnActionGoprinter()
{
	spptr	*cp = GetSelectedPtr();
	if  (!cp)
		return;
	if  (verboten(PV_HALTGO, IDP_PRIN_CANTGO))
		return;
	if  (cp->spp_state >= SPP_PROC)  {
		AfxMessageBox(IDP_PRIN_RUNNING, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	Printers().opptr(SO_PGO, cp);
}	

void CPtrdoc::OnActionHaltatonce()
{
	spptr	*cp = GetSelectedPtr();
	if  (!cp)
		return;
	if  (verboten(PV_HALTGO, IDP_PRIN_CANTHALT))
		return;
	if  (cp->spp_state < SPP_PROC)  {
		AfxMessageBox(IDP_PRIN_NOTRUNNING, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	Printers().opptr(SO_PSTP, cp);
}

void CPtrdoc::OnActionHalteoj()
{
	spptr	*cp = GetSelectedPtr();
	if  (!cp)
		return;
	if  (verboten(PV_HALTGO, IDP_PRIN_CANTHALT))
		return;
	if  (cp->spp_state < SPP_PROC)  {
		AfxMessageBox(IDP_PRIN_NOTRUNNING, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	Printers().opptr(SO_PHLT, cp);
}

void CPtrdoc::OnActionOkalignment()
{
	spptr	*cp = GetSelectedPtr();
	if  (!cp)
		return;
    if  (cp->spp_state != SPP_OPER)  {
    	if  (cp->spp_state != SPP_WAIT)
    		return;
    	if  (AfxMessageBox(IDP_CONFIRM_BYPASSALIGN, MB_OKCANCEL|MB_ICONQUESTION) != IDOK)
    		return;                                                     
    }
    Printers().opptr(SO_OYES, cp);
}

void CPtrdoc::OnPrintersFormtype()
{
	spptr	*cp = GetSelectedPtr();
	if  (!cp)
		return;
	if  (cp->spp_state >= SPP_PROC)  {
		AfxMessageBox(IDP_PRIN_RUNNING, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	CPrform	fdlg;
	fdlg.m_formtype = cp->get_spp_form();
	if  (fdlg.DoModal() == IDOK)  {
		spptr	newp = *cp;
		strcpy(newp.spp_form, fdlg.m_formtype);
		Printers().chgptr(newp);
	}
}

void CPtrdoc::OnPrintersInterrupt()
{
	spptr	*cp = GetSelectedPtr();
	if  (!cp)
		return;
	if  (verboten(PV_HALTGO, IDP_PRIN_CANTINT))
		return;
	if  (cp->spp_state != SPP_RUN)  {
		AfxMessageBox(IDP_PRIN_NOTRUNNING, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	Printers().opptr(SO_INTER, cp);
}
	
BOOL CPtrdoc::Smatches(const CString str, const int ind)
{
	spptr *pt = (*this)[ind];
	if  (m_sformtype && Smstr(str, pt->spp_form))
		return  TRUE;
	if  (m_sdevice && Smstr(str, pt->spp_dev, TRUE))
		return  TRUE;
	if  (m_sprinter && Smstr(str, pt->spp_ptr))
		return  TRUE;
	return FALSE;
}	

void CPtrdoc::DoSearch(const CString str, const BOOL forward)
{             
	int	cnt;
	spptr  *pt = GetSelectedPtr(FALSE);
	if  (forward)  {
		int	cwhich = pt? pindex(pt): -1;
		for  (cnt = cwhich + 1;  cnt < int(number());  cnt++)
			if  (Smatches(str, cnt))
				goto  gotit;
		if  (m_swraparound)
			for  (cnt = 0;  cnt < cwhich;  cnt++)
				if  (Smatches(str, cnt))
					goto  gotit;
	}
	else  {
		int  cwhich = pt? pindex(pt): number();
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
	CPtrView	*aview = (CPtrView *)GetNextView(p);
	if  (aview)  {
		aview->ChangeSelectionToRow(cnt);		
		aview->UpdateRow(cnt);
	    UpdateAllViews(NULL);
	}    
}	

void CPtrdoc::OnSearchSearchfor()
{
	CJPSearch	sdlg;
	sdlg.m_sstring = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_plastsearch;
	sdlg.m_sdevice = m_sdevice;
	sdlg.m_sformtype = m_sformtype;
	sdlg.m_sjtitle = FALSE;
	sdlg.m_sprinter = m_sprinter;
	sdlg.m_suser = FALSE;
	sdlg.m_swraparound = m_swraparound;
	sdlg.m_which = CJPSearch::IDD_PRINSRCH;
	sdlg.m_sforward = 0;
	if  (sdlg.DoModal() != IDOK)
		return;
	((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_plastsearch = sdlg.m_sstring;
	m_sdevice = sdlg.m_sdevice;
	m_sformtype = sdlg.m_sformtype;
	m_sprinter = sdlg.m_sprinter;
	m_swraparound = sdlg.m_swraparound;
	DoSearch(sdlg.m_sstring, sdlg.m_sforward == 0);
}

void CPtrdoc::OnSearchSearchforward()
{
	CString  sstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_plastsearch;
	if  (sstr.IsEmpty())  {
		AfxMessageBox(IDP_NOSEARCHSTR, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	DoSearch(sstr, TRUE);	
}

void CPtrdoc::OnSearchSearchbackwards()
{
	CString  sstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_plastsearch;
	if  (sstr.IsEmpty())  {
		AfxMessageBox(IDP_NOSEARCHSTR, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	DoSearch(sstr, FALSE);	
}

void CPtrdoc::OnWindowWindowoptions()
{
	CSpqwApp *mw = (CSpqwApp *) AfxGetApp();
	CMainFrame	*mf = (CMainFrame *) mw->m_pMainWnd;
	if  (mf->DoRstrDlg(m_wrestrict))
		reviseptrs(mw->m_appptrlist);
}

void CPtrdoc::OnFileWpcolourAwoper() 
{
	RunColourDlg(SPP_OPER);
}

void CPtrdoc::OnFileWpcolourError() 
{
	RunColourDlg(SPP_ERROR);
}

void CPtrdoc::OnFileWpcolourHalted() 
{
	RunColourDlg(SPP_HALT);
}

void CPtrdoc::OnFileWpcolourIdle() 
{
	RunColourDlg(SPP_WAIT);
}

void CPtrdoc::OnFileWpcolourOffline() 
{
	RunColourDlg(SPP_OFFLINE);
}

void CPtrdoc::OnFileWpcolourPrinting() 
{
	RunColourDlg(SPP_RUN);
}

void CPtrdoc::OnFileWpcolourShutd() 
{
	RunColourDlg(SPP_SHUTD);
}

void CPtrdoc::OnFileWpcolourStartup() 
{
	RunColourDlg(SPP_INIT);
}

void CPtrdoc::RunColourDlg(const unsigned int n)
{
	COLORREF	&mc = m_ptrcolours[n];
	CColorDialog	cdlg;
	cdlg.m_cc.Flags |= CC_RGBINIT|CC_SOLIDCOLOR;
	cdlg.m_cc.rgbResult = mc;
	if  (cdlg.DoModal() == IDOK)  {
		mc = cdlg.m_cc.rgbResult;
		UpdateAllViews(NULL);
	}
}

void CPtrdoc::OnFilePcolourCopytoprog() 
{
	CSpqwApp *mw = (CSpqwApp *)AfxGetApp();
	mw->m_appcolours = m_ptrcolours;
	mw->dirty();
}

void CPtrdoc::OnFilePcolourCopytowin() 
{
	m_ptrcolours = ((CSpqwApp *)AfxGetApp())->m_appcolours;
	UpdateAllViews(NULL);
}
