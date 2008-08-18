// pagedlg.cpp : implementation file
//

#include "stdafx.h"
#ifdef	SPRSERV
#include "pages.h"
#include "xtini.h"
#endif
#include "resource.h"
#include "pagedlg.h"
#include <limits.h>
#include "Sprserv.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern	char		*translate_delim(const char *, unsigned &);
extern	char		*untranslate_delim(const char *, const unsigned);

/////////////////////////////////////////////////////////////////////////////
// CPagedlg dialog

CPagedlg::CPagedlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPagedlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPagedlg)
	m_delimnum = 0;
	m_swapoe = FALSE;
	m_ppflags = "";
	//}}AFX_DATA_INIT
}

void CPagedlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPagedlg)
	DDX_Text(pDX, IDC_DELIMNUM, m_delimnum);
	DDX_Check(pDX, IDC_SWAPOE, m_swapoe);
	DDX_Text(pDX, IDC_PPFLAGS, m_ppflags);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPagedlg, CDialog)
	//{{AFX_MSG_MAP(CPagedlg)
	ON_BN_CLICKED(IDC_ALLP, OnClickedAllp)
	ON_BN_CLICKED(IDC_EVENP, OnClickedEvenp)
	ON_BN_CLICKED(IDC_ODDP, OnClickedOddp)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_ENDP, OnDeltaposScrEndp)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

const	char	endpmk[] = "(end)";


/////////////////////////////////////////////////////////////////////////////
// CPagedlg message handlers

BOOL CPagedlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	SetDlgItemInt(IDC_STARTP, int(m_startp) + 1, FALSE);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_STARTP))->SetRange(SHRT_MAX, 1);
	if  (m_endp >= UINT(SHRT_MAX - 1))
		SetDlgItemText(IDC_ENDP, endpmk);
	else
		SetDlgItemInt(IDC_ENDP, int(m_endp) + 1, FALSE);

	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_DELIMNUM))->SetRange(1, SHRT_MAX);
    
    CString	dstring;
    mkdelimstring(dstring);
	SetDlgItemText(IDC_DELIMITER, dstring);

	if  (m_jflags & SPQ_ODDP)  {
		((CButton *)GetDlgItem(IDC_EVENP))->SetCheck(1);
		CButton *sw = (CButton *)GetDlgItem(IDC_SWAPOE);
		sw->EnableWindow();
		if  (m_jflags & SPQ_REVOE)
			sw->SetCheck(1);
	}
	else  if  (m_jflags & SPQ_EVENP)  {
		((CButton *)GetDlgItem(IDC_ODDP))->SetCheck(1);
		CButton *sw = (CButton *)GetDlgItem(IDC_SWAPOE);
		sw->EnableWindow();
		if  (m_jflags & SPQ_REVOE)
			sw->SetCheck(1);
	}
	else
		((CButton *)GetDlgItem(IDC_ALLP))->SetCheck(1);

	return TRUE;
}

void CPagedlg::OnOK()
{
	if  (!procdelimstring())  {
		AfxMessageBox(IDP_BADDELIMITER, MB_OK+MB_ICONEXCLAMATION);
		CEdit	*ew = (CEdit *) GetDlgItem(IDC_DELIMITER);
		ew->SetSel(0, -1);
		ew->SetFocus();
		return;
	}

	unsigned  long  sp, ep;
	char	buf[20];
	int  res = GetDlgItemText(IDC_ENDP, buf, sizeof(buf));
	if  (res <= 0  ||  res >= sizeof(buf) || _stricmp(buf, endpmk) == 0)
		ep = LONG_MAX - 1L;
	else
		ep = atol(buf) - 1;
	res = GetDlgItemText(IDC_STARTP, buf, sizeof(buf));
	sp = atol(buf) - 1;
	if  (sp > ep)  {
		AfxMessageBox(IDP_STARTPGENDP, MB_OK+MB_ICONEXCLAMATION);
		CEdit	*ew = (CEdit *) GetDlgItem(IDC_ENDP);
		ew->SetSel(0, -1);
		ew->SetFocus();
		return;
	}               
	m_startp = sp;
	m_endp = ep;
	m_jflags &= ~(SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);
	if  (((CButton *)GetDlgItem(IDC_EVENP))->GetCheck())  {
		m_jflags |= SPQ_ODDP;
		if  (((CButton *)GetDlgItem(IDC_SWAPOE))->GetCheck())
			m_jflags |= SPQ_REVOE;
	}
	else  if  (((CButton *)GetDlgItem(IDC_ODDP))->GetCheck())  {
		m_jflags |= SPQ_EVENP;
		if  (((CButton *)GetDlgItem(IDC_SWAPOE))->GetCheck())
			m_jflags |= SPQ_REVOE;
	}
	CDialog::OnOK();
}

void CPagedlg::OnClickedAllp()
{
	CWnd  *cw = GetDlgItem(IDC_SWAPOE);
	((CButton *)cw)->SetCheck(0);
	cw->EnableWindow(FALSE);
}

void CPagedlg::OnClickedEvenp()
{
	GetDlgItem(IDC_SWAPOE)->EnableWindow();
}

void CPagedlg::OnClickedOddp()
{
	GetDlgItem(IDC_SWAPOE)->EnableWindow();
}
                                                 
                                                 
void	CPagedlg::mkdelimstring(CString &res)
{                                           
	char	*result = untranslate_delim((char *) m_delimiter, m_deliml);
	res = result? result: "\\f";
}	

BOOL	CPagedlg::procdelimstring()	                                                 
{
	char	instring[200];
	if  (GetDlgItemText(IDC_DELIMITER, instring, sizeof(instring)) <= 0)
		return  FALSE;
	char  *result = translate_delim(instring, m_deliml);
	if  (!result)
		return  FALSE;
	if  (m_delimiter)
		delete  m_delimiter;
	m_delimiter = result;
	return  TRUE;
}			

void CPagedlg::OnDeltaposScrEndp(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	unsigned  long  ep;
	char	buf[20];
	int  res = GetDlgItemText(IDC_ENDP, buf, sizeof(buf));
	if  (res <= 0  ||  res >= sizeof(buf) || _stricmp(buf, endpmk) == 0)
		ep = LONG_MAX - 1L;
	else
		ep = atol(buf) - 1;
	if  (pNMUpDown->iDelta >= 0)  {
		ep += pNMUpDown->iDelta;
		if  (ep >= LONG_MAX - 1L)
			ep = 0;
		else  if  (ep >= SHRT_MAX)
			ep = LONG_MAX - 1L;
	}
	else  {
		if  (ep < (unsigned long) - pNMUpDown->iDelta)
			ep = 0;
		else  if  (ep >= LONG_MAX - 2L)
			ep = SHRT_MAX - 1;
		else
			ep += pNMUpDown->iDelta;
	}
	if  (ep >= SHRT_MAX)
		SetDlgItemText(IDC_ENDP, endpmk);
	else
		SetDlgItemInt(IDC_ENDP, int(ep+1), FALSE);
 	*pResult = 0;
}

const DWORD a105HelpIDs[] = {
	IDC_STARTP,	IDH_105_169,	// Page Options 
	IDC_SCR_STARTP,	IDH_105_170,	// Page Options Spin1
	IDC_ENDP,	IDH_105_171,	// Page Options 
	IDC_SCR_ENDP,	IDH_105_172,	// Page Options Spin2
	IDC_ALLP,	IDH_105_173,	// Page Options All pages
	IDC_ODDP,	IDH_105_174,	// Page Options Odd pages
	IDC_EVENP,	IDH_105_175,	// Page Options Even pages
	IDC_SWAPOE,	IDH_105_176,	// Page Options Swap odd/even
	IDC_DELIMNUM,	IDH_105_177,	// Page Options 
	IDC_SCR_DELIMNUM,	IDH_105_178,	// Page Options Spin3
	IDC_DELIMITER,	IDH_105_179,	// Page Options 
	IDC_PPFLAGS,	IDH_105_180,	// Page Options 
	0, 0
};

BOOL CPagedlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a105HelpIDs[cnt] != 0;  cnt += 2)
		if  (a105HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a105HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
