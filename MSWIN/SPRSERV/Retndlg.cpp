// retndlg.cpp : implementation file
//

#include "stdafx.h"
#ifdef	SPRSERV
#include "pages.h"
#include "monfile.h"
#include "xtini.h"
#include "sprserv.h"
#include "Sprserv.hpp"
#endif
#ifdef	SPRSETW
#include "sprsetw.h"
#include "Sprsetw.hpp"
#endif
#include "retndlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRetndlg dialog

CRetndlg::CRetndlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRetndlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRetndlg)
	m_retain = FALSE;
	m_dip = 0;
	m_dinp = 0;
	m_hold = FALSE;
	//}}AFX_DATA_INIT
}

void CRetndlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRetndlg)
	DDX_Check(pDX, IDC_RETAIN, m_retain);
	DDX_Text(pDX, IDC_DIP, m_dip);
	DDV_MinMaxUInt(pDX, m_dip, 0, 32767);
	DDX_Text(pDX, IDC_DINP, m_dinp);
	DDV_MinMaxUInt(pDX, m_dinp, 0, 32767);
	DDX_Check(pDX, IDC_HOLD, m_hold);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRetndlg, CDialog)
	//{{AFX_MSG_MAP(CRetndlg)
	ON_BN_CLICKED(IDC_HOLD, OnClickedHold)
	ON_WM_HELPINFO()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_DELHOURS, OnDeltaposScrDelhours)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_DELMINS, OnDeltaposScrDelmins)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_DELSECS, OnDeltaposScrDelsecs)
	ON_EN_CHANGE(IDC_DELHOURS, OnChangeDelhours)
	ON_EN_CHANGE(IDC_DELMINS, OnChangeDelmins)
	ON_EN_CHANGE(IDC_DELSECS, OnChangeDelsecs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRetndlg message handlers

BOOL CRetndlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	if  (m_hold)
		Enablehtime(TRUE);
	return TRUE;
}

void CRetndlg::OnOK()
{
	UINT	prin = GetDlgItemInt(IDC_DIP, NULL, FALSE);
	UINT	nprin = GetDlgItemInt(IDC_DINP, NULL, FALSE);
	if  (prin > nprin  &&  AfxMessageBox(IDP_PRINGNPRIN, MB_OKCANCEL+MB_ICONQUESTION) == IDCANCEL)  {
		CEdit	*ew = (CEdit *) GetDlgItem(IDC_DINP);
		ew->SetSel(0, -1);
		ew->SetFocus();
		return;
	}
	CDialog::OnOK();
}

void CRetndlg::OnClickedHold()
{
	if  (((CButton *)GetDlgItem(IDC_HOLD))->GetCheck())  {
		m_holdtime = 5 * 60;
		Enablehtime(TRUE);
	}                     
	else
		Enablehtime(FALSE);
}
  
void CRetndlg::Enablehtime(const BOOL enab)
{
	if  (enab)  {
		int  cnt;
		for  (cnt = IDC_DELHOURS; cnt <= IDC_SCR_DELSECS;  cnt++)
			GetDlgItem(cnt)->EnableWindow(TRUE);
		GetDlgItem(IDC_COLON2)->EnableWindow(TRUE);
		GetDlgItem(IDC_COLON3)->EnableWindow(TRUE);
		fillindelay();
	}
	else  {
		int	 cnt;
		SetDlgItemText(IDC_DELHOURS, "");
		SetDlgItemText(IDC_DELMINS, "");
		SetDlgItemText(IDC_DELSECS, "");
		for  (cnt = IDC_DELHOURS; cnt <= IDC_SCR_DELSECS;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
		GetDlgItem(IDC_COLON2)->EnableWindow(FALSE);
		GetDlgItem(IDC_COLON3)->EnableWindow(FALSE);
	}
}

void	CRetndlg::fillindelay()
{
	long	del = m_holdtime;
	SetDlgItemInt(IDC_DELHOURS, del/3600);
	del %= 3600;
	SetDlgItemInt(IDC_DELMINS, del/60);
	del %= 60;
	SetDlgItemInt(IDC_DELSECS, del);
}

#ifdef	SPRSERV
const DWORD a106HelpIDs[] = {
	IDC_RETAIN,	IDH_106_181,	// Retention options Retain on queue after printing
	IDC_DIP,	IDH_106_182,	// Retention options 
	IDC_SCR_DIP,	IDH_106_183,	// Retention options Spin1
	IDC_DINP,	IDH_106_184,	// Retention options 
	IDC_SCR_DINP,	IDH_106_185,	// Retention options Spin2
	IDC_HOLD,	IDH_106_186,	// Retention options Delay print for...
	IDC_DELHOURS,	IDH_106_246,	// Retention options 
	IDC_SCR_DELHOURS,	IDH_106_249,	// Retention options Spin1
	IDC_DELMINS,	IDH_106_247,	// Retention options 
	IDC_SCR_DELMINS,	IDH_106_250,	// Retention options Spin1
	IDC_DELSECS,	IDH_106_248,	// Retention options 
	IDC_SCR_DELSECS,	IDH_106_251,	// Retention options Spin1
	0, 0
};
#endif
#ifdef	SPRSETW
const DWORD a108HelpIDs[] = {
	IDC_RETAIN,	IDH_108_204,	// Retention options Retain on queue after printing
	IDC_DIP,	IDH_108_205,	// Retention options 
	IDC_SCR_DIP,	IDH_108_206,	// Retention options Spin1
	IDC_DINP,	IDH_108_207,	// Retention options 
	IDC_SCR_DINP,	IDH_108_208,	// Retention options Spin2
	IDC_HOLD,	IDH_108_209,	// Retention options Delay print for...
	IDC_DELHOURS,	IDH_108_246,	// Retention options 
	IDC_SCR_DELHOURS,	IDH_108_249,	// Retention options Spin1
	IDC_DELMINS,	IDH_108_247,	// Retention options 
	IDC_SCR_DELMINS,	IDH_108_250,	// Retention options Spin1
	IDC_DELSECS,	IDH_108_248,	// Retention options 
	IDC_SCR_DELSECS,	IDH_108_251,	// Retention options Spin1
	0, 0
};
#endif

BOOL CRetndlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
#ifdef	SPRSERV
	for  (int cnt = 0;  a106HelpIDs[cnt] != 0;  cnt += 2)
		if  (a106HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a106HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
#endif
#ifdef	SPRSETW
	for  (int cnt = 0;  a108HelpIDs[cnt] != 0;  cnt += 2)
		if  (a108HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a108HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
#endif
}

void CRetndlg::OnDeltaposScrDelhours(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	long  newtime = m_holdtime;
	if  (pNMUpDown->iDelta >= 0)
		newtime += 60 * 60;
	else
		newtime -= 60 * 60;
	if  (newtime > 0)  {
		m_holdtime = newtime;
		fillindelay();
	}
	*pResult = 0;
}

void CRetndlg::OnDeltaposScrDelmins(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	long  newtime = m_holdtime;
	if  (pNMUpDown->iDelta >= 0)
		newtime += 60;
	else
		newtime -= 60;
	if  (newtime > 0)  {
		m_holdtime = newtime;
		fillindelay();
	}
	*pResult = 0;
}

void CRetndlg::OnDeltaposScrDelsecs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	long  newtime = m_holdtime;
	if  (pNMUpDown->iDelta >= 0)
		newtime++;
	else
		newtime--;
	if  (newtime > 0)  {
		m_holdtime = newtime;
		fillindelay();
	}
	*pResult = 0;
}

void CRetndlg::OnChangeDelhours() 
{
	long newh = GetDlgItemInt(IDC_DELHOURS, NULL, FALSE);
	m_holdtime = m_holdtime % 3600 + newh * 3600;
}

void CRetndlg::OnChangeDelmins() 
{
	long newm = GetDlgItemInt(IDC_DELMINS, NULL, FALSE);
	m_holdtime = (m_holdtime / 3600) * 3600 + newm * 60 + m_holdtime % 60;
}

void CRetndlg::OnChangeDelsecs() 
{
	long news = GetDlgItemInt(IDC_DELSECS, NULL, FALSE);
	m_holdtime = (m_holdtime / 60) * 60 + news;
}
