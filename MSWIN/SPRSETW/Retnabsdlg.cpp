// Retnabsdlg.cpp : implementation file
//

#include "stdafx.h"
#include "sprsetw.h"
#include "Retnabsdlg.h"
#include "Sprsetw.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRetnabsdlg dialog


CRetnabsdlg::CRetnabsdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRetnabsdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRetnabsdlg)
	m_retain = FALSE;
	m_dip = 0;
	m_dinp = 0;
	m_hold = FALSE;
	//}}AFX_DATA_INIT
}


void CRetnabsdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRetnabsdlg)
	DDX_Check(pDX, IDC_RETAIN, m_retain);
	DDX_Text(pDX, IDC_DIP, m_dip);
	DDV_MinMaxUInt(pDX, m_dip, 0, 32767);
	DDX_Text(pDX, IDC_DINP, m_dinp);
	DDV_MinMaxUInt(pDX, m_dinp, 0, 32767);
	DDX_Check(pDX, IDC_HOLD, m_hold);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRetnabsdlg, CDialog)
	//{{AFX_MSG_MAP(CRetnabsdlg)
	ON_WM_HELPINFO()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_HOUR, OnDeltaposScrHour)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MIN, OnDeltaposScrMin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_WDAY, OnDeltaposScrWday)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MDAY, OnDeltaposScrMday)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MON, OnDeltaposScrMon)
	ON_BN_CLICKED(IDC_HOLD, OnHold)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRetnabsdlg message handlers

void CRetnabsdlg::OnOK() 
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

BOOL CRetnabsdlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if  (m_hold)
		Enablehtime(TRUE);
	return TRUE;
}

const DWORD a108HelpIDs[]=
{
	IDC_SCR_DINP,	IDH_108_208,	// Retention options: "Spin2" (msctls_updown32)
	IDC_HOLD,	IDH_108_209,	// Retention options: "Do not print before......" (Button)
	IDC_HOUR,	IDH_108_210,	// Retention options: "" (Edit)
	IDC_MIN,	IDH_108_210,	// Retention options: "" (Edit)
	IDC_WDAY,	IDH_108_210,	// Retention options: "" (Edit)
	IDC_MDAY,	IDH_108_210,	// Retention options: "" (Edit)
	IDC_MON,	IDH_108_210,	// Retention options: "" (Edit)
	IDC_SCR_HOUR,	IDH_108_210,	// Retention options: "Spin3" (msctls_updown32)
	IDC_SCR_MIN,	IDH_108_210,	// Retention options: "Spin3" (msctls_updown32)
	IDC_SCR_WDAY,	IDH_108_210,	// Retention options: "Spin3" (msctls_updown32)
	IDC_SCR_MDAY,	IDH_108_210,	// Retention options: "Spin3" (msctls_updown32)
	IDC_SCR_MON,	IDH_108_210,	// Retention options: "Spin3" (msctls_updown32)
	IDC_RETAIN,	IDH_108_204,	// Retention options: "Retain on queue after printing" (Button)
	IDC_DIP,	IDH_108_205,	// Retention options: "0" (Edit)
	IDC_SCR_DIP,	IDH_108_206,	// Retention options: "Spin1" (msctls_updown32)
	IDC_DINP,	IDH_108_207,	// Retention options: "0" (Edit)
	0, 0
};

BOOL CRetnabsdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a108HelpIDs[cnt] != 0;  cnt += 2)
		if  (a108HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a108HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}

void CRetnabsdlg::OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_holdtime;
	if  (pNMUpDown->iDelta >= 0)
		newtime += 60 * 60;
	else
		newtime -= 60 * 60;
	if  (newtime > time(NULL))  {
		m_holdtime = newtime;
		fillintime();
	}
	*pResult = 0;
}

void CRetnabsdlg::OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_holdtime;
	if  (pNMUpDown->iDelta >= 0)
		newtime += 60;
	else
		newtime -= 60;
	if  (newtime > time(NULL))  {
		m_holdtime = newtime;
		fillintime();
	}
	*pResult = 0;
}

void CRetnabsdlg::OnDeltaposScrWday(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_holdtime;
	if  (pNMUpDown->iDelta >= 0)
		newtime += 60 * 60 * 24L;
	else
		newtime -= 60 * 60 * 24L;
	if  (newtime > time(NULL))  {
		m_holdtime = newtime;
		fillintime();
	}
	*pResult = 0;
}

void CRetnabsdlg::OnDeltaposScrMday(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_holdtime;
	if  (pNMUpDown->iDelta >= 0)
		newtime += 60 * 60 * 24L;
	else
		newtime -= 60 * 60 * 24L;
	if  (newtime > time(NULL))  {
		m_holdtime = newtime;
		fillintime();
	}
	*pResult = 0;
}

void CRetnabsdlg::OnDeltaposScrMon(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_holdtime;
	tm	*lt = localtime(&newtime);
	const  unsigned  char  mdays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if  (pNMUpDown->iDelta >= 0)  {
		unsigned  ndays = mdays[lt->tm_mon];
		if  (lt->tm_mon == 1  &&  lt->tm_year % 4 == 0)
			ndays++;
		newtime += ndays * 60L * 60L * 24L;
	}	
	else  {
		int	mon = lt->tm_mon - 1;
		if  (mon < 0)
			mon = 11;
		unsigned  ndays = mdays[mon];
		if  (mon == 1  &&  lt->tm_year % 4 == 0)
			ndays++;
		newtime -= ndays * 60L * 60L * 24L;
	}	
	if  (newtime > time(NULL))  {
		m_holdtime = newtime;
		fillintime();
	}
	*pResult = 0;
}

void CRetnabsdlg::OnHold() 
{
	if  (((CButton *)GetDlgItem(IDC_HOLD))->GetCheck())  {
		m_holdtime = time(NULL) + 5 * 60;
		Enablehtime(TRUE);
	}                     
	else
		Enablehtime(FALSE);
}

void CRetnabsdlg::Enablehtime(const BOOL enab)
{
	if  (enab)  {
		int	cnt;
		for  (cnt = IDC_HOUR;  cnt <= IDC_MON;  cnt++)
			GetDlgItem(cnt)->EnableWindow(TRUE);
		for  (cnt = IDC_SCR_HOUR;  cnt <= IDC_COLON;  cnt++)
			GetDlgItem(cnt)->EnableWindow(TRUE);
		fillintime();
	}
	else  {
		int	cnt;
		for  (cnt = IDC_HOUR;  cnt <= IDC_MON;  cnt++)  {
			SetDlgItemText(cnt, "");
			GetDlgItem(cnt)->EnableWindow(FALSE);        
		}
		for  (cnt = IDC_SCR_HOUR;  cnt <= IDC_COLON;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
	}		
}

void	CRetnabsdlg::fillintime()
{
	tm	*tp = localtime(&m_holdtime);
	char	tdigs[4];
	wsprintf(tdigs, "%.2d", tp->tm_hour);
	SetDlgItemText(IDC_HOUR, tdigs);
	wsprintf(tdigs, "%.2d", tp->tm_min);
	SetDlgItemText(IDC_MIN, tdigs);
	CString	wday;
	wday.LoadString(IDS_SUNDAY + tp->tm_wday);
	SetDlgItemText(IDC_WDAY, wday);
	SetDlgItemInt(IDC_MDAY, tp->tm_mday);
	CString mon;
	mon.LoadString(IDS_JANUARY + tp->tm_mon);
	SetDlgItemText(IDC_MON, mon);
}	

