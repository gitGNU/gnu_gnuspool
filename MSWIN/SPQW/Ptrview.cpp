#include "stdafx.h"
#include "jobdoc.h"
#include "spqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "ptrdoc.h"
#include "ptrview.h"
#include "mainfrm.h"
#include <stdlib.h>
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern	void	prin_size(char FAR *, const long);
extern	int		calc_fmt_length(CString  CSpqwApp ::*);
extern	void	save_fmt_widths(CString  CSpqwApp ::*, const Formatrow *, const int);

IMPLEMENT_DYNCREATE(CPtrView, CRowView)

/////////////////////////////////////////////////////////////////////////////
CPtrView::CPtrView()
{
}

void CPtrView::OnUpdate(CView*, LPARAM lHint, CObject* pHint)
{   
	Invalidate();
	for  (int fcnt = 0;  fcnt < m_nformats;  fcnt++)
		m_formats[fcnt].f_maxwidth = 0;
}

void CPtrView::GetRowWidthHeight(CDC* pDC, int& nRowWidth, int& nRowHeight)
{
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	nRowWidth = tm.tmAveCharWidth * calc_fmt_length(&CSpqwApp::m_pfmt);
	nRowHeight = tm.tmHeight;
	m_avecharwidth = tm.tmAveCharWidth;
}

int CPtrView::GetActiveRow()
{
	return  ((CPtrdoc*)GetDocument())->pindex(curr_ptr);
}

unsigned CPtrView::GetRowCount()
{
	return  ((CPtrdoc*)GetDocument())->number();
}

void CPtrView::ChangeSelectionToRow(int nRow)
{
	CPtrdoc* pDoc = GetDocument();
	spptr  *p = (*pDoc)[nRow];
	if  (p)
		curr_ptr = pident(p);
	else
		curr_ptr = pident(netid_t(0));
}               

static	CString	result;

static	int  fmt_ab(const spptr &pp, const int fwidth)
{
	if  (pp.spp_dflags & SPP_HADAB)
		result.LoadString(IDS_PHADAB);
	return  result.GetLength();
}

static	int  fmt_class(const spptr &pp, const int fwidth)
{
	for  (int i = 0;  i < 16;  i++)
		result += char((pp.spp_class & (1L << i))? 'A' + i: '.');
	for  (i = 0;  i < 16;  i++)
		result += char((pp.spp_class & (1L << (i+16)))? 'a' + i: '.');
	return	32;
}

static	int  fmt_device(const spptr &pp, const int fwidth)
{
	if  (pp.spp_netflags & SPP_LOCALNET)  {
		result = '[';
		result += pp.spp_dev;
		result += ']';
	}
	else
		result = pp.spp_dev;
	return  result.GetLength();
}

static	int	fmt_descr(const spptr &pp, const int fwidth)
{
	result = pp.spp_comment;
	return  result.GetLength();
}

static	int  fmt_form(const spptr &pp, const int fwidth)
{
	result = pp.spp_form;
	return  result.GetLength();
}

static	int  fmt_heoj(const spptr &pp, const int fwidth)
{
	if  (pp.spp_sflags & SPP_HEOJ)
		result.LoadString(IDS_PHEOJ);
	return  result.GetLength();
}

static	int  fmt_pid(const spptr &pp, const int fwidth)
{
	if  (pp.spp_netid  ||  pp.spp_state < SPP_PROC)
		return  0;
	char	fmt[10];
	wsprintf(fmt, "%%%dld", fwidth > 19? 19: fwidth);
	char	obuf[20];
	wsprintf(obuf, fmt, long(pp.spp_pid));
	result = obuf;
	return  result.GetLength();
}

static	int  fmt_jobno(const spptr &pp, const int fwidth)
{
	if  (pp.spp_state < SPP_PREST)
		return  0;
	spq	*cj = Jobs()[unsigned(pp.spp_jslot)];
	if  (cj)  {
		char	obuf[HOSTNSIZE+1+20];
		wsprintf(obuf, "%s:%ld", (char FAR *) look_host(cj->spq_netid), pp.spp_job);
		result = obuf;
		if  (result.GetLength() <= fwidth)
			return  result.GetLength();			
	}
	char	fmt[10];
	wsprintf(fmt, "%%%dld", fwidth > 19? 19: fwidth);
	char	obuf[20];
	wsprintf(obuf, fmt, long(pp.spp_job));
	result = obuf;
	return  result.GetLength();
}

static	int  fmt_localonly(const spptr &pp, const int fwidth)
{
	if  (pp.spp_netflags & SPP_LOCALONLY)
		result.LoadString(IDS_LOCALONLY);
	return  result.GetLength();
}

static	int  fmt_message(const spptr &pp, const int fwidth)
{
	if  (pp.spp_state < SPP_HALT)
		result = pp.spp_feedback;
	return  result.GetLength();
}

static	int  fmt_na(const spptr &pp, const int fwidth)
{
	if  (pp.spp_dflags & SPP_REQALIGN)
		result.LoadString(IDS_PNALIGN);
	return  result.GetLength();
}

static	int  fmt_printer(const spptr &pp, const int fwidth)
{
	result = look_host(pp.spp_netid);
	result += ':';
	result += pp.spp_ptr;
	return  result.GetLength();
}

static	int  fmt_state(const spptr &pp, const int fwidth)
{
	int	staten = pp.spp_state;
	result.LoadString(IDS_PSTATUS_NULL + staten);
	if  (staten < SPP_HALT  &&  pp.spp_feedback[0])  {
		result += ':';
		result += pp.spp_feedback;
	}
	return  result.GetLength();
}

static	int  fmt_ostate(const spptr &pp, const int fwidth)
{
	result.LoadString(IDS_PSTATUS_NULL + pp.spp_state);
	return  result.GetLength();
}

static	int  fmt_user(const spptr &pp, const int fwidth)
{
	if  (pp.spp_state < SPP_PREST)
		return  0;
	spq	*cj = Jobs()[unsigned(pp.spp_jslot)];
	if  (!cj)
		return  0;
	result = cj->spq_uname;
	return  result.GetLength();
}

static	int  fmt_minsize(const spptr &pp, const int fwidth)
{
	if  (pp.spp_minsize == 0L)
		return  0;
	char	rbuf[20];
	prin_size(rbuf, pp.spp_minsize);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_maxsize(const spptr &pp, const int fwidth)
{
	if  (pp.spp_maxsize == 0L)
		return  0;
	char	rbuf[20];
	prin_size(rbuf, pp.spp_maxsize);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_limit(const spptr &pp, const int fwidth)
{
	if  (pp.spp_minsize != 0)
		result = '<';
	if  (pp.spp_maxsize != 0)
		result += '>';
	return  result.GetLength();
}

static	int  fmt_shriek(const spptr &pp, const int fwidth)
{
	if  (pp.spp_dflags)
		result.LoadString(pp.spp_dflags & SPP_HADAB? IDS_PHADAB: IDS_PNALIGN);
	return  result.GetLength();
}

typedef	int	(*fmt_fn)(const spptr &, const int);

const	fmt_fn	lowertab[] = {
	fmt_ab,			//  a
	NULL,			//  b
	fmt_class,		//  c
	fmt_device,		//  d
	fmt_descr,		//  e
	fmt_form,		//  f
	NULL,			//  g
	fmt_heoj,		//  h
	fmt_pid,		//  i
	fmt_jobno,		//  j
	NULL,			//  k
	fmt_localonly,	//  l
	fmt_message,	//	m
	fmt_na,			//  n
	NULL,			//  o
	fmt_printer,	//	p
	NULL, NULL,		//  q, r
	fmt_state,		//  s
	fmt_ostate,		//	t
	fmt_user,		//  u
	NULL,			//  v
	fmt_shriek,		//  w
	fmt_limit,		//  x
	fmt_minsize,	//  y
	fmt_maxsize		//  z
};

void	CPtrView::Initformats()
{
	CRowView::Initformats(&CSpqwApp::m_pfmt, NULL, (int (*const*)(...))lowertab, 0, IDS_SPFORMAT_aa, "iyz");
}

void CPtrView::OnDrawRow(CDC* pDC, int nRow, int y, BOOL bSelected)
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
	
	CPtrdoc  *currdoc = ((CPtrdoc *)GetDocument());
	spptr	*pp = (*currdoc)[nRow];
	
	if  (pp)  {

		if  (!bSelected)
			pDC->SetTextColor(currdoc->m_ptrcolours[pp->spp_state]);
	
		Formatrow	*fl = m_formats;
		int	 currplace = -1, nominal_width = 0, fcnt = 0;

		while  (fcnt < m_nformats)  {

			if  (fl->f_issep)  {
				CString	&fld = fl->f_field;
				int	lng = fld.GetLength();
				int	wc;
				for  (wc = 0;  wc < lng;  wc++)
					if  (fld[wc] != ' ')
						break;
				if  (wc < lng)
					pDC->TextOut((nominal_width + wc) * m_avecharwidth, y, fld.Right(lng-wc));
				nominal_width += fl->f_width;
				fl++;
				fcnt++;
				continue;
			}

			int  lastplace = -1;
			int	 nn = fl->f_width;
			if  (fl->f_ltab)
				lastplace = currplace;
			currplace = nominal_width;
			result.Empty();
			int  inlen = (*(fmt_fn)fl->f_fmtfn)(*pp, nn);

			if  (inlen > nn  &&  lastplace >= 0)  {
				nn += currplace - lastplace;
				currplace = lastplace;
			}

			if  (inlen > nn)  {
				if  (fl->f_skipright)  {
					if  (UINT(inlen) > fl->f_maxwidth)
						fl->f_maxwidth = inlen;
					pDC->TextOut(currplace * m_avecharwidth, y, result);
					break;
				}
				result = result.Left(nn);
			}
			
			if  (inlen > 0  &&  result[0] == ' ')  {
				for  (int  wc = 1;  wc < inlen  &&  result[wc] == ' ';  wc++)
					;  
				inlen -= wc;
				pDC->TextOut((currplace + nn - inlen) * m_avecharwidth, y, result.Right(inlen));
			}
			else	
				pDC->TextOut(currplace * m_avecharwidth, y, result);
			if  (UINT(inlen) > fl->f_maxwidth)
				fl->f_maxwidth = inlen;
			nominal_width += fl->f_width;
			fl++;
			fcnt++;
		}
	}

	// Restore the DC.
	if (bSelected)	{
		pDC->SetBkColor(crOldBackground);
		pDC->SetTextColor(crOldText);
	}
}

void	CPtrView::PtrAllChange(const BOOL plushdr)
{
	if  (plushdr)  {
		Initformats();
		SetUpHdr();
	}
	((CPtrdoc *)GetDocument())->reviseptrs(((CSpqwApp *)AfxGetApp())->m_appptrlist);
	UpdateScrollSizes();
}

void	CPtrView::PtrChgPtr(const unsigned ind)
{
	spptr	*pt = (((CSpqwApp *)AfxGetApp())->m_appptrlist)[ind];
	if  (pt)  {
		int  myind = ((CPtrdoc *)GetDocument())->pindex(*pt);
		if  (myind >= 0)
			RedrawRow(myind);
	}		
}

void	CPtrView::SaveWidthSettings()
{
	save_fmt_widths(&CSpqwApp ::m_pfmt, m_formats, m_nformats);
}

void	CPtrView::Dopopupmenu(CPoint point)
{
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_PTRFLOAT));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
}
