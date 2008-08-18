// jdatavie.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"          
#include "mainfrm.h" 
#include "jdatadoc.h"
#include "jdatavie.h"
#include "jdsearch.h"
#include "vmdlg.h"
#include <ctype.h>
#include <limits.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJdataview

IMPLEMENT_DYNCREATE(CJdataview, CScrollView)


CJdataview::CJdataview()
{
	m_nRowHeight = m_nRowWidth = 0;
	m_buffer = NULL;
	m_isstart = m_isend = m_ishat = FALSE;
	m_startp = 0;
	m_endp = UINT_MAX;
}

CJdataview::~CJdataview()
{
	if  (m_buffer)
		delete [] m_buffer;
}


BEGIN_MESSAGE_MAP(CJdataview, CScrollView)
	//{{AFX_MSG_MAP(CJdataview)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_SEARCH_SEARCHFORWARD, OnSearchSearchforward)
	ON_COMMAND(ID_SEARCH_SEARCHBACKWARDS, OnSearchSearchbackwards)
	ON_COMMAND(ID_SEARCH_SEARCHFOR, OnSearchSearchfor)
	ON_WM_SIZE()
	ON_COMMAND(ID_SEARCH_SETSTARTPAGE, OnSearchSetstartpage)
	ON_COMMAND(ID_SEARCH_SETENDPAGE, OnSearchSetendpage)
	ON_COMMAND(ID_SEARCH_SETHALTEDATPAGE, OnSearchSethaltedatpage)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJdataview drawing 

void CJdataview::UpdateScrollSizes()
{
	if  (!m_buffer)  {
		m_buffer = new char[GetNumCols() + 1];
		if  (!m_buffer)  {
			AfxMessageBox(IDP_NOMEMFORJOB, MB_OK|MB_ICONEXCLAMATION);
			((CJdatadoc *)GetDocument())->m_invalid = TRUE;
		}		
	}	

	CClientDC dc(this);
	TEXTMETRIC tm;
	dc.GetTextMetrics(&tm);
    
    long	total;
	m_nRowHeight = tm.tmHeight;
	m_nCharWidth = tm.tmAveCharWidth + 1;
	total = long(GetNumCols()) * long(m_nCharWidth);
	if  (total > INT_MAX)  {
		AfxMessageBox(IDP_JOBTOOWIDE, MB_OK|MB_ICONEXCLAMATION);
		((CJdatadoc *)GetDocument())->m_invalid = TRUE;
		total = INT_MAX;
	}	
	m_nRowWidth = int(total);
	total = long(GetNumRows()) * long(m_nRowHeight);
	if  (total > INT_MAX)  {
		AfxMessageBox(IDP_JOBTOOLONG, MB_OK|MB_ICONEXCLAMATION);
		((CJdatadoc *)GetDocument())->m_invalid = TRUE;
		total = INT_MAX;
	}	
	SetScrollSizes(MM_TEXT, CSize(m_nRowWidth, int(total)));
}	

void CJdataview::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();	
	UpdateScrollSizes();
	UpdatePageMarkers();
}                           

void	CJdataview::OnUpdate(CView *pSender, LPARAM lhint, CObject *pHint)
{
	if  (lhint == JD_LHINT_PAGES)
		UpdatePageMarkers();
	else
		CScrollView::OnUpdate(pSender, lhint, pHint);
}

				
void CJdataview::OnDraw(CDC* pDC)
{
	pDC->SelectStockObject(SYSTEM_FIXED_FONT);
	pDC->SetBkMode(TRANSPARENT);
	if  (((CJdatadoc *)GetDocument())->m_invalid)  {
		CString	minv;
		minv.LoadString(IDP_INVALIDDOC);
		pDC->TextOut(0, 0, minv);
		return;
	}	
	CRect rect;
	pDC->GetClipBox(&rect);			 // Get the invalidated region.
	int nFirstRow = rect.top / m_nRowHeight;
	int nLastRow = min(unsigned(rect.bottom) / m_nRowHeight + 1, GetNumRows());
	int nRow, y;
	for (nRow = nFirstRow, y = m_nRowHeight * nFirstRow;  nRow < nLastRow;  nRow++, y += m_nRowHeight)  {
		char  *rw = GetRow(nRow);
		pDC->TextOut(0, y, rw, strlen(rw));
	}	
	UpdatePageMarkers();
}

/////////////////////////////////////////////////////////////////////////////
// CJdataview message handlers

void	CJdataview::set_page(void (CJdatadoc::*w)(const long))
{
	(((CJdatadoc *)GetDocument())->*w)(Current_Page());
}		
	
void	CJdataview::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CVMdlg	dlg;
	dlg.m_isstart = m_isstart;
	dlg.m_isend = m_isend;
	dlg.m_ishat = m_ishat;
	dlg.m_enabhat = ((CJdatadoc *)GetDocument())->gethat() != 0;
	unsigned  cp = Current_Page();
	dlg.m_enabend = cp >= m_startp;
	dlg.m_enabstart = cp <= m_endp;
		
	switch  (dlg.DoModal())  {
	default:
	case  IDOK:
		if  (dlg.m_isstart &&  !m_isstart)
			set_page(&CJdatadoc::setstart);
		if  (dlg.m_isend && !m_isend)
		    set_page(&CJdatadoc::setend);
		if  (dlg.m_ishat && !m_ishat)
			set_page(&CJdatadoc::sethat);
		UpdatePageMarkers();
		return;
	case  IDC_QUITDATA:
		GetParent()->DestroyWindow();
		return;
	}
}

void CJdataview::OnRButtonDown(UINT nFlags, CPoint point)
{
	OnSearchSearchfor();
}

int	 CJdataview::matches(char *str, CString mstr, BOOL ignorecase)
{
	int	slen = strlen(str);
	int mlen = mstr.GetLength();
	for  (int  i = 0;  i <= slen - mlen;  i++)  {
		int  xpos;
		for  (int  j = 0;  j < mlen;  j++)  {
			int  sch = str[i+j];
			int  mch = mstr[j];
			if  (ignorecase)  {
				sch = toupper(sch);
				mch = toupper(mch);
			}
			if  (sch != mch && mch != '.')
				goto  ns;
		}
		//  Found it but adjust position appropriately
		xpos = 0;
		for  (j = 0;  j < i;  j++)  {
			xpos++;
			int  ch = str[j];
			if  (ch & 0x80)  {
				ch &= ~0x80;
				xpos += 2;  
			}				
			if  (!isprint(ch))  {
				if  (ch == '\t')
					while  (xpos & 3)
						xpos++;
				else
					xpos++;
			}		
		}
		return  xpos;
	ns:
		;
	}
	return  -1;
}

int	CJdataview::lookback(const int from, const int to, int &which)
{
	CJdatadoc *doc = (CJdatadoc *)GetDocument();
	int	xpos;
	for  (int matchrow = from;  matchrow >= to;  matchrow--)  {
		char	*str = doc->FindRow(matchrow);
		if  ((xpos = matches(str, m_lastlook, m_ignorecase)) >= 0)  {
			delete [] str;
			which = matchrow;
			return  xpos;
		}                 
		delete [] str;
	}
	return  -1;
}			
	
int	CJdataview::lookforw(const int from, const int to, int &which)
{
	CJdatadoc *doc = (CJdatadoc *)GetDocument();
	int	xpos;
	for  (int matchrow = from;  matchrow < to;  matchrow++)  {
		char	*str = doc->FindRow(matchrow);
		if  ((xpos = matches(str, m_lastlook, m_ignorecase)) >= 0)  {
			delete [] str;
			which = matchrow;
			return  xpos;
		}                 
		delete [] str;
	}       
	return  -1;
}			
			
void CJdataview::ExecuteSearch(const int direction /* 0 forwards 1 back */)
{
	if  (m_lastlook.IsEmpty())  {
		AfxMessageBox(IDP_JDNOSEARCHS, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	              
	int	 matchrow, xpos;
	CClientDC dc(this);
	OnPrepareDC(&dc);
	int nTopRow, nBotRow;
	CRect rectClient;
	GetClientRect(&rectClient);
	dc.DPtoLP(&rectClient);
	nTopRow = rectClient.top / m_nRowHeight;
	nBotRow = rectClient.bottom  / m_nRowHeight;
	if  (direction)  {
		if  ((xpos = lookback(nTopRow - 1, 0, matchrow)) >= 0)
			goto  foundit;
		if  (m_wrapround  &&  (xpos = lookback(GetNumRows()-1, nBotRow+1, matchrow)) >= 0)
			goto  foundit;
	}		
	else  {
		if  ((xpos = lookforw(nBotRow+1, GetNumRows(), matchrow)) >= 0)
			goto  foundit;
		if  (m_wrapround  &&  (xpos = lookforw(0, nTopRow, matchrow)) >= 0)
			goto  foundit;
	}
	
	//  Didn't find it
	
	AfxMessageBox(IDP_JDSNOTFOUND, MB_OK|MB_ICONEXCLAMATION);
	return;

foundit:
	int	nLeftCol = rectClient.left / m_nCharWidth;
	int	nRightCol = rectClient.right / m_nCharWidth;
	int x = nLeftCol;
	if  (xpos < x || xpos >= nRightCol)
		x = xpos;
	ScrollToPosition(CPoint(x * m_nCharWidth, matchrow * m_nRowHeight));
}
	

void CJdataview::OnSearchSearchforward()
{
	ExecuteSearch(0);
}

void CJdataview::OnSearchSearchbackwards()
{
	ExecuteSearch(1);
}

void CJdataview::OnSearchSearchfor()
{
	CJDSearch	sdlg;
	sdlg.m_ignorecase = m_ignorecase;
	sdlg.m_lookfor = m_lastlook.IsEmpty()? "": m_lastlook;
	sdlg.m_wrapround = m_wrapround;
	sdlg.m_sforward = 0;
	int	ret = sdlg.DoModal();
	if  (ret == IDOK || ret == IDCANCEL)  {
		m_lastlook = sdlg.m_lookfor;
		m_wrapround = sdlg.m_wrapround;
		m_ignorecase = sdlg.m_ignorecase;
	}
	if  (ret == IDOK)
		ExecuteSearch(sdlg.m_sforward);
}

void CJdataview::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);
	UpdatePageMarkers();
}

void CJdataview::UpdatePageMarkers()
{
	if  (m_nRowHeight == 0)		// Only if we've set scroll size
		return;
	CMainFrame *mf = (CMainFrame *)AfxGetApp()->m_pMainWnd;
	unsigned  cp = Current_Page();
	CJdatadoc *doc = (CJdatadoc *)GetDocument();		
	m_isstart = (m_startp = doc->getstart()) == cp;
	m_isend = (m_endp = doc->getend()) == cp;
	m_ishat = doc->gethat() == cp && cp != 0;
	mf->SetPageMarker(CMainFrame::Start, m_isstart);
	mf->SetPageMarker(CMainFrame::End, m_isend);
	mf->SetPageMarker(CMainFrame::Halted_at, m_ishat);
}			                                  

unsigned CJdataview::GetNumRows()
{
	return  ((CJdatadoc *)GetDocument())->jdheight();
}

unsigned CJdataview::GetNumCols()
{
	return  ((CJdatadoc *)GetDocument())->jdwidth();
}

unsigned CJdataview::Current_Page()	
{                                         
	CClientDC dc(this);
	OnPrepareDC(&dc);
	int nTopRow;
	CRect rectClient;
	GetClientRect(&rectClient);
	dc.DPtoLP(&rectClient);
	nTopRow = rectClient.top / m_nRowHeight;
	return  ((CJdatadoc *)GetDocument())->whatpage(nTopRow);
}

void CJdataview::OnSearchSetstartpage()
{
	unsigned  cp = Current_Page();
	if  (cp > m_endp)
		AfxMessageBox(IDP_SSTARTPGTENDP, MB_OK|MB_ICONEXCLAMATION);
	else
		set_page(&CJdatadoc::setstart);
}


void CJdataview::OnSearchSetendpage()
{
	unsigned  cp = Current_Page();
	if  (cp < m_startp)
		AfxMessageBox(IDP_SENDPLTSTARTP, MB_OK|MB_ICONEXCLAMATION);
	else
		set_page(&CJdatadoc::setend);
}

void CJdataview::OnSearchSethaltedatpage()
{
	set_page(&CJdatadoc::sethat);
}

void CJdataview::OnSetFocus(CWnd* pOldWnd)
{
	CScrollView::OnSetFocus(pOldWnd);
	((CMainFrame *) AfxGetApp()->m_pMainWnd)->EnableMarkers(TRUE);
}

void CJdataview::OnKillFocus(CWnd* pNewWnd)
{
	CScrollView::OnKillFocus(pNewWnd);
	((CMainFrame *) AfxGetApp()->m_pMainWnd)->EnableMarkers(FALSE);	
}
