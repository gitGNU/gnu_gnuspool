// pagedlg.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"
#include "pagedlg.h"
#include <limits.h>
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

const	char	endpmk[] = "(end)";


/////////////////////////////////////////////////////////////////////////////
// CPagedlg dialog

CPagedlg::CPagedlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPagedlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPagedlg)
	m_ppflags = "";
	//}}AFX_DATA_INIT
}

void CPagedlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPagedlg)
	DDX_Text(pDX, IDC_PPFLAGS, m_ppflags);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPagedlg, CDialog)
	//{{AFX_MSG_MAP(CPagedlg)
	ON_BN_CLICKED(IDC_ALLP, OnClickedAllp)
	ON_BN_CLICKED(IDC_ODDP, OnClickedOddp)
	ON_BN_CLICKED(IDC_EVENP, OnClickedEvenp)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_ENDP, OnDeltaposScrEndp)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPagedlg message handlers

void CPagedlg::OnClickedAllp()
{
	CWnd  *cw = GetDlgItem(IDC_SWAPOE);
	((CButton *)cw)->SetCheck(0);
	cw->EnableWindow(FALSE);
}

void CPagedlg::OnClickedOddp()
{
	GetDlgItem(IDC_SWAPOE)->EnableWindow();
}

void CPagedlg::OnClickedEvenp()
{
	GetDlgItem(IDC_SWAPOE)->EnableWindow();
}

void CPagedlg::OnOK()
{
	unsigned  long sp, ep;
	char	buf[20];
	int  res = GetDlgItemText(IDC_ENDP, buf, sizeof(buf));
	if  (res <= 0  ||  res >= sizeof(buf) || _stricmp(buf, endpmk) == 0)
		ep = UINT_MAX;
	else
		ep = atol(buf) - 1L;
	res = GetDlgItemText(IDC_STARTP, buf, sizeof(buf));
	sp = atol(buf) - 1L;
	if  (sp > ep)  {
		AfxMessageBox(IDP_STARTPGTENDP, MB_OK|MB_ICONEXCLAMATION);
		CEdit	*ew = (CEdit *) GetDlgItem(IDC_ENDP);
		ew->SetSel(0, -1);
		ew->SetFocus();
		return;
	}               
	m_startp = sp;
	m_endp = ep;
	if  (m_hatp != 0)  {
		unsigned long hp;
		res = GetDlgItemText(IDC_HATP, buf, sizeof(buf));
		hp = atol(buf) - 1L;
		if  (hp > ep)  {
			AfxMessageBox(IDP_HATPGTENDP, MB_OK|MB_ICONEXCLAMATION);
			CEdit  *ew = (CEdit *) GetDlgItem(IDC_HATP);
			ew->SetSel(0, -1);
			ew->SetFocus();
			return;
		}
		m_hatp = hp;
	}		
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

BOOL CPagedlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	SetDlgItemInt(IDC_STARTP, int(m_startp) + 1, FALSE);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_STARTP))->SetRange(SHRT_MAX, 1);
	if  (m_endp >= UINT(SHRT_MAX - 1))
		SetDlgItemText(IDC_ENDP, endpmk);
	else
		SetDlgItemInt(IDC_ENDP, int(m_endp) + 1, FALSE);
	
	if  (m_hatp != 0)  {           
		GetDlgItem(IDC_HATP)->EnableWindow();
		GetDlgItem(IDC_SCR_HATP)->EnableWindow();
		SetDlgItemInt(IDC_HATP, int(m_hatp) + 1, FALSE);
		((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_HATP))->SetRange(SHRT_MAX, 1);
	}	

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
	return	TRUE;
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

const DWORD a110HelpIDs[] = {
	IDC_STARTP,	IDH_110_225,	// Page options 
	IDC_SCR_STARTP,	IDH_110_226,	// Page options Spin1
	IDC_ENDP,	IDH_110_227,	// Page options 
	IDC_SCR_ENDP,	IDH_110_228,	// Page options Spin2
	IDC_HATP,	IDH_110_229,	// Page options 
	IDC_SCR_HATP,	IDH_110_230,	// Page options Spin1
	IDC_ALLP,	IDH_110_231,	// Page options All pages
	IDC_ODDP,	IDH_110_232,	// Page options Odd pages
	IDC_EVENP,	IDH_110_233,	// Page options Even pages
	IDC_SWAPOE,	IDH_110_234,	// Page options Swap odd/even
	IDC_PPFLAGS,	IDH_110_235,	// Page options 
	0, 0
};

BOOL CPagedlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a110HelpIDs[cnt] != 0;  cnt += 2)
		if  (a110HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a110HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
