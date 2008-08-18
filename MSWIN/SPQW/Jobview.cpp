#include "stdafx.h"
#include "jobdoc.h"
#include "spqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "jobview.h"
#include "mainfrm.h"
#include <stdlib.h>
#include <ctype.h>

#define	LOTSANDLOTS	99999999L

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
                           
extern	void	prin_size(char FAR *, const long);
extern	int		calc_fmt_length(CString  CSpqwApp ::*);
extern	void	save_fmt_widths(CString  CSpqwApp ::*, const Formatrow *, const int);
                           
IMPLEMENT_DYNCREATE(CJobView, CRowView)

/////////////////////////////////////////////////////////////////////////////
CJobView::CJobView()
{
}

void CJobView::OnUpdate(CView*v, LPARAM lHint, CObject* pHint)
{   
	Invalidate();
	for  (int fcnt = 0;  fcnt < m_nformats;  fcnt++)
		m_formats[fcnt].f_maxwidth = 0;
}

void CJobView::GetRowWidthHeight(CDC* pDC, int& nRowWidth, int& nRowHeight)
{
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	nRowWidth = tm.tmAveCharWidth * calc_fmt_length(&CSpqwApp::m_jfmt);
	nRowHeight = tm.tmHeight;
	m_avecharwidth = tm.tmAveCharWidth;
}

int CJobView::GetActiveRow()
{
	return  ((CJobdoc*)GetDocument())->jindex(curr_job);
}

unsigned CJobView::GetRowCount()
{
	return  ((CJobdoc*)GetDocument())->number();
}

void CJobView::ChangeSelectionToRow(int nRow)
{
	CJobdoc* pDoc = GetDocument();
	spq  FAR *j = (*pDoc)[nRow];
	if  (j)
		curr_job = jident(j);
	else
		curr_job = jident(jobno_t(0));
}               

static	CString	result;
static	CJobdoc *current_doc;

static	int  fmt_wattn(const spq &jb, const int fwidth)
{
	if  (jb.spq_jflags & SPQ_WATTN)
		result.LoadString(IDS_JF_WATTN);
	return  result.GetLength();
}

static	int  fmt_class(const spq &jb, const int fwidth)
{
	for  (int i = 0;  i < 16;  i++)
		result += char((jb.spq_class & (1L << i))? 'A' + i: '.');
	for  (i = 0;  i < 16;  i++)
		result += char((jb.spq_class & (1L << (i+16)))? 'a' + i: '.');
	return  32;
}

static	int  fmt_ppflags(const spq &jb, const int fwidth)
{
	result = jb.spq_flags;
	return  result.GetLength();
}

static	int  fmt_hold(const spq &jb, const int fwidth)
{
	time_t	now = time(NULL);
	if  (jb.spq_hold < now)
		return  0;
	tm	*tp = localtime(&jb.spq_hold);
	int	mon = tp->tm_mon+1;
	int	day = tp->tm_mday;
	if  (timezone >= 4L * 60L * 60L)  {
		day = mon;
		mon = tp->tm_mday;
	}
	char	rbuf[16];
	if  (fwidth < 14)  {
		time_t	now = time(NULL);
		if  (jb.spq_hold >= now + 24L * 60L * 60L)
			wsprintf(rbuf, "%.2d/%.2d", day, mon);
		else
			wsprintf(rbuf, "%.2d:%.2d", tp->tm_hour, tp->tm_min);
	}
	else
		wsprintf(rbuf, "%.2d/%.2d/%.2d %.2d:%.2d",
					day, mon, tp->tm_year % 100, tp->tm_hour, tp->tm_min);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_stime(const spq &jb, const int fwidth)
{
	tm	*tp = localtime(&jb.spq_time);
	int	mon = tp->tm_mon+1;
	int	day = tp->tm_mday;
	if  (timezone >= 4L * 60L * 60L)  {
		day = mon;
		mon = tp->tm_mday;
	}
	char	rbuf[16];
	if  (fwidth < 14)  {
		time_t	now = time(NULL);
		if  (jb.spq_time >= now + 24L * 60L * 60L)
			wsprintf(rbuf, "%.2d/%.2d", day, mon);
		else
			wsprintf(rbuf, "%.2d:%.2d", tp->tm_hour, tp->tm_min);
	}
	else
		wsprintf(rbuf, "%.2d/%.2d/%.2d %.2d:%.2d",
					day, mon, tp->tm_year % 100, tp->tm_hour, tp->tm_min);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_sizek(const spq &jb, const int fwidth)
{
	char	rbuf[20];
	prin_size(rbuf, jb.spq_size);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_kreached(const spq &jb, const int fwidth)
{
	if	(!(jb.spq_dflags & SPQ_PQ))
		return  0;
	char	rbuf[20];
	prin_size(rbuf, jb.spq_posn);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_jobno(const spq &jb, const int fwidth)
{
	char	rbuf[HOSTNSIZE+20+6];
	wsprintf(rbuf, "%s:%ld", (char FAR *) look_host(jb.spq_netid), jb.spq_job);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_oddeven(const spq &jb, const int fwidth)
{
	if  (!(jb.spq_jflags & (SPQ_ODDP|SPQ_EVENP)))
		return  0;
	result.LoadString(jb.spq_jflags & SPQ_ODDP? IDS_JF_ODD: IDS_JF_EVEN);
	if  (jb.spq_jflags & SPQ_REVOE)  {
		CString	plus;
		plus.LoadString(IDS_JF_SWAP);
		result += plus;
	}
	return  result.GetLength();
}

static	int  fmt_printer(const spq &jb, const int fwidth)
{
	if  (jb.spq_ptr[0])
		result = jb.spq_ptr;
	else  if  (jb.spq_sflags & SPQ_ASSIGN)  {
		spptr	*cp = Printers()[unsigned(jb.spq_pslot)];
		if  (cp  ||  (cp = Printers().printing(jident(&jb))))
			result = cp->spp_ptr;
		else  
			result.LoadString(IDS_LOCALPTR);
	}
	return  result.GetLength();
}

static	int  fmt_pgreached(const spq &jb, const int fwidth)
{
	if  (!(jb.spq_dflags & SPQ_PQ))
		return  0;
	char	rbuf[20];
	prin_size(rbuf, jb.spq_npages > 1? jb.spq_pagec: jb.spq_posn);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_range(const spq &jb, const int fwidth)
{
	if  (jb.spq_start == 0 && jb.spq_end >= LOTSANDLOTS)
		return  0;
	if  (jb.spq_start != 0)  {
		char	thing[20];
		wsprintf(thing, "%ld", jb.spq_start+1L);
		result = thing;
	}
	result += '-';
	if  (jb.spq_end < LOTSANDLOTS)  {
		char	thing[20];
		wsprintf(thing, "%ld", jb.spq_end+1L);
		result += thing;
	}
	return  result.GetLength();
}

static	int  fmt_szpages(const spq &jb, const int fwidth)
{
	char	res[20];
	if  (jb.spq_npages > 1)  {
		result = '=';          
		prin_size(res, jb.spq_npages);
		for  (int	resl = strlen(res) + 1;  resl < fwidth;  resl++)
			result += ' ';
		result += res;
	}
	else  {
		prin_size(res, jb.spq_size);
		for  (int	resl = strlen(res);  resl < fwidth;  resl++)
			result += ' ';
		result += res;
	}
	return  result.GetLength();
}

static	int  fmt_nptime(const spq &jb, const int fwidth)
{
	char	fld[12];
	wsprintf(fld, "%%%du", fwidth > 19? 19: fwidth);
	char	rbuf[20];
	wsprintf(rbuf, fld, jb.spq_nptimeout);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_user(const spq &jb, const int fwidth)
{
	result = jb.spq_uname;
	return  result.GetLength();
}

static	int  fmt_puser(const spq &jb, const int fwidth)
{
	result = jb.spq_puname;
	return  result.GetLength();
}

static	int  fmt_mattn(const spq &jb, const int fwidth)
{
	if  (jb.spq_jflags & SPQ_MATTN)
		result.LoadString(IDS_JF_MATTN);
	return  result.GetLength();
}

static	int  fmt_cps(const spq &jb, const int fwidth)
{
	char	fld[12];
	wsprintf(fld, "%%%du", fwidth > 19? 19: fwidth);
	char	rbuf[20];
	wsprintf(rbuf, fld, unsigned(jb.spq_cps));
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_form(const spq &jb, const int fwidth)
{
	result = jb.spq_form;
	return  result.GetLength();
}

static	int  fmt_title(const spq &jb, const int fwidth)
{
	result = jb.spq_file;
	return  result.GetLength();
}

static	int  fmt_localonly(const spq &jb, const int fwidth)
{
	if  (jb.spq_jflags & SPQ_LOCALONLY)
		result.LoadString(IDS_JF_LOCALONLY);
	return  result.GetLength();
}

static	int  fmt_mail(const spq &jb, const int fwidth)
{
	if  (jb.spq_jflags & SPQ_MAIL)
		result.LoadString(IDS_JF_MAIL);
	return  result.GetLength();
}

static	int  fmt_prio(const spq &jb, const int fwidth)
{
	char	fld[12];
	wsprintf(fld, "%%%du", fwidth > 19? 19: fwidth);
	char	rbuf[20];
	wsprintf(rbuf, fld, unsigned(jb.spq_pri));
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_retain(const spq &jb, const int fwidth)
{
	if  (jb.spq_jflags & SPQ_RETN)
		result.LoadString(IDS_JF_RETN);
	return  result.GetLength();
}

static	int  fmt_supph(const spq &jb, const int fwidth)
{
	if  (jb.spq_jflags & SPQ_NOH)
		result.LoadString(IDS_JF_NOH);
	return  result.GetLength();
}

static	int  fmt_ptime(const spq &jb, const int fwidth)
{
	char	fld[12];
	wsprintf(fld, "%%%du", fwidth > 19? 19: fwidth);
	char	rbuf[20];
	wsprintf(rbuf, fld, jb.spq_ptimeout);
	result = rbuf;
	return  result.GetLength();
}

static	int  fmt_write(const spq &jb, const int fwidth)
{
	if  (jb.spq_jflags & SPQ_WRT)
		result.LoadString(IDS_JF_WRITE);
	return  result.GetLength();
}

static	int	fmt_seq(const spq &jb, const int fwidth)
{
	char	fld[12];
	wsprintf(fld, "%%%dd", fwidth > 19? 19: fwidth);
	char	rbuf[20];
	wsprintf(rbuf, fld, current_doc->jindex(jb));
	result = rbuf;
	return  result.GetLength();
}

static	int	fmt_orighost(const spq &jb, const int fwidth)
{
	result = look_host(jb.spq_orighost);
	return  result.GetLength();
}

typedef	int	(*fmt_fn)(const spq &, const int);

const	fmt_fn	uppertab[] = {
	fmt_wattn,			//	A
	NULL,				//  B
	fmt_class,			//  C
	NULL,				//  D
	NULL,				//  E
	fmt_ppflags,		//  F
	NULL,				//  G
	fmt_hold,			//  H
	NULL,				//  I
	NULL,				//  J
	fmt_sizek,			//  K
	fmt_kreached,		//  L
	NULL,				//  M
	fmt_jobno,			//  N
	fmt_oddeven,		//  O
	fmt_printer,		//  P
	fmt_pgreached,		//  Q
	fmt_range,			//  R
	fmt_szpages,		//  S
	fmt_nptime,			//  T
	fmt_puser,			//  U
	NULL,               //  V
	fmt_stime,          //  W
	NULL, NULL, NULL	//  X,Y,Z
},  lowertab[] = {
	fmt_mattn,			//  a
	NULL,				//  b
	fmt_cps,			//  c
	NULL, NULL,			//  d,e
	fmt_form,			//  f
	NULL,				//  g
	fmt_title,			//  h
	NULL, NULL, NULL,	//  i,j,k
	fmt_localonly,		//  l
	fmt_mail,			//  m
	fmt_seq,			//  n
	fmt_orighost,		//  o
	fmt_prio,			//  p
	fmt_retain,			//  q
	NULL,				//  r
	fmt_supph,			//  s
	fmt_ptime,			//  t
	fmt_user,			//  u
	NULL,				//  v
	fmt_write,			//  w
	NULL, NULL, NULL	//  x,y,z
};

void	CJobView::Initformats()
{
	CRowView::Initformats(&CSpqwApp::m_jfmt, (int (*const*)(...))uppertab, (int (*const*)(...))lowertab, IDS_SJFORMAT_A, IDS_SJFORMAT_aa, "KLQScnp");
}

void CJobView::OnDrawRow(CDC* pDC, int nRow, int y, BOOL bSelected)
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
	
	current_doc = (CJobdoc *)GetDocument();
	spq	*cj = (*current_doc)[nRow];
	
	if  (cj)  {
	
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
			int  inlen = (*(fmt_fn)fl->f_fmtfn)(*cj, nn);

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

void	CJobView::JobAllChange(const BOOL plushdr)
{
	if  (plushdr)  {
		Initformats();
		SetUpHdr();
	}
	((CJobdoc *)GetDocument())->revisejobs(((CSpqwApp *)AfxGetApp())->m_appjoblist);
	UpdateScrollSizes();
}

void	CJobView::JobChgJob(const unsigned ind)
{
	spq	*jb = (((CSpqwApp *)AfxGetApp())->m_appjoblist)[ind];
	if  (jb)  {
		int  myind = ((CJobdoc *)GetDocument())->jindex(*jb);
		if  (myind >= 0)
			RedrawRow(myind);
	}		
}

void	CJobView::SaveWidthSettings()
{
	save_fmt_widths(&CSpqwApp ::m_jfmt, m_formats, m_nformats);
}

void	CJobView::Dopopupmenu(CPoint point)
{
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_JOBFLOAT));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
}
