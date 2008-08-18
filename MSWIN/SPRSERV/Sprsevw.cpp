// sprsevw.cpp : implementation of the CSprservView class
//

#include "stdafx.h"
#include "pages.h"
#include "monfile.h"
#include "xtini.h"
#include "sprserv.h"
#include "sprsedoc.h"
#include "sprsevw.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define	CHECK_COL	0
#define	CHECK_LEN	2
#define	TIM_COL		CHECK_LEN
#define	TIM_LEN		6
#define	FILE_COL	(TIM_COL+TIM_LEN+1)
#define	FILE_LEN	50
#define	FORM_COL	(FILE_COL+FILE_LEN+1)
#define	FORM_LEN	MAXFORM
#define	ROW_WIDTH	(FORM_COL+FORM_LEN)

/////////////////////////////////////////////////////////////////////////////
// CSprservView

IMPLEMENT_DYNCREATE(CSprservView, CScrollView)

BEGIN_MESSAGE_MAP(CSprservView, CScrollView)
	//{{AFX_MSG_MAP(CSprservView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSprservView construction/destruction

CSprservView::CSprservView()
{
	m_nRowHeight = m_nRowWidth = 0;
	m_nSelectedRow = -1;        
}

CSprservView::~CSprservView()
{
}

void CSprservView::UpdateScrollSizes()
{
	CClientDC dc(this);
	TEXTMETRIC tm;
	CRect rectClient;
	GetClientRect(&rectClient);
	dc.GetTextMetrics(&tm);
	m_nRowHeight = tm.tmHeight;
	m_nCharWidth = tm.tmAveCharWidth + 1;
	m_nRowWidth = ROW_WIDTH * m_nCharWidth;
	CSize sizePage(m_nRowWidth/5, max(m_nRowHeight, ((rectClient.bottom/m_nRowHeight)-1)*m_nRowHeight));
	CSize sizeLine(m_nRowWidth/20, m_nRowHeight);
	SetScrollSizes(MM_TEXT, CSize(m_nRowWidth, int(GetDocument()->GetDocSize()) * m_nRowHeight), sizePage, sizeLine);
}	

void CSprservView::OnUpdate(CView*v, LPARAM lHint, CObject* pHint)
{   
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CSprservView drawing

void CSprservView::OnDrawRow(CDC* pDC, int nRow, int y, BOOL bSelected)
{
	CBrush brushBackground;
	COLORREF crOldText = 0;
	COLORREF crOldBackground = 0;

	if  (bSelected) {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		crOldBackground = pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
		crOldText = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else  {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
		pDC->SetBkMode(TRANSPARENT);
	}
	
	CRect rectSelection;
	pDC->GetClipBox(&rectSelection);
	rectSelection.top = y;
	rectSelection.bottom = y + m_nRowHeight;
	pDC->FillRect(&rectSelection, &brushBackground);

	CMonFile	*cf = GetDocument()->m_flist[nRow];
	if  (cf)  {	
		spq		&cj = cf->qpar();
    	TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);
		if  (cf->isok())
			pDC->TextOut(CHECK_COL*tm.tmAveCharWidth, y, "*", 1);
		char	timeb[TIM_LEN + 4];
		wsprintf(timeb, "%u", cf->get_time());
		pDC->TextOut(TIM_COL*tm.tmAveCharWidth, y, timeb, strlen(timeb));
		pDC->TextOut(FILE_COL*tm.tmAveCharWidth, y, cf->get_file(), strlen(cf->get_file()));
		pDC->TextOut(FORM_COL*tm.tmAveCharWidth, y, cj.spq_form, strlen(cj.spq_form));
	}   

	// Restore the DC.
	if (bSelected)	{
		pDC->SetBkColor(crOldBackground);
		pDC->SetTextColor(crOldText);
	}
}

void CSprservView::OnDraw(CDC* pDC)
{
	CSprservDoc* pDoc = GetDocument();
	if (pDoc->GetDocSize() == 0)
		return;
	int nFirstRow, nLastRow;
	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	nFirstRow = rectClip.top / m_nRowHeight;
	nLastRow = min(unsigned(rectClip.bottom / m_nRowHeight) + 1, pDoc->GetDocSize());
	int nRow, y;
	for (nRow = nFirstRow, y = m_nRowHeight * nFirstRow; nRow < nLastRow; nRow++, y += m_nRowHeight)
		OnDrawRow(pDC, nRow, y, nRow == m_nSelectedRow);
}

/////////////////////////////////////////////////////////////////////////////
// CSprservView diagnostics

#ifdef _DEBUG
void CSprservView::AssertValid() const
{
	CView::AssertValid();
}

void CSprservView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSprservDoc* CSprservView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSprservDoc)));
	return (CSprservDoc*) m_pDocument;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSprservView message handlers

void CSprservView::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);
	UpdateScrollSizes();
}

void CSprservView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);
	CRect rect(point, CSize(1,1));
	int nFirstRow;
	nFirstRow = rect.top / m_nRowHeight;
	if (unsigned(nFirstRow) <= (GetDocument()->GetDocSize() - 1))  {
		m_nSelectedRow = nFirstRow;              
		GetDocument()->UpdateAllViews(NULL);
	}	
}

void CSprservView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	GetDocument()->PublicChfilename();
}

void CSprservView::OnRButtonDown(UINT nFlags, CPoint point)
{
	GetDocument()->PublicForm();
}

void CSprservView::STimer(UINT nIDEvent, UINT nElapse)
{
 	SetTimer(nIDEvent, nElapse, NULL);
} 	                                  

void CSprservView::OnTimer(UINT nIDEvent)
{
	KillTimer(nIDEvent);
	GetDocument()->OnTimer(nIDEvent);
}

#ifdef	DROPINTOVIEW
void CSprservView::OnDropFiles(HDROP hDropInfo)
{
	UINT	fnum = ::DragQueryFile(hDropInfo, UINT(-1), NULL, 0);
	for  (UINT  cnt = 0;  cnt < fnum;  cnt++)  {
		char	fpath[_MAX_PATH];
		::DragQueryFile(hDropInfo, cnt, fpath, _MAX_PATH);
		GetDocument()->NewDropped(fpath);
	}
	::DragFinish(hDropInfo);
	GetDocument()->reschedule();
}

int CSprservView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;
	DragAcceptFiles(TRUE);
	return 0;
}
#endif
