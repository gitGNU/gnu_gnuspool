// retndlg.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"
#include "retndlg.h"
#include "Spqw.hpp"

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
	m_dinp = 0;
	m_dip = 0;
	m_hold = FALSE;
	m_printed = FALSE;
	m_retain = FALSE;
	//}}AFX_DATA_INIT
	m_inittime = time((time_t *) 0);
}

void CRetndlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRetndlg)
	DDX_Text(pDX, IDC_DINP, m_dinp);
	DDV_MinMaxUInt(pDX, m_dinp, 1, 32767);
	DDX_Text(pDX, IDC_DIP, m_dip);
	DDV_MinMaxUInt(pDX, m_dip, 1, 32767);
	DDX_Check(pDX, IDC_HOLD, m_hold);
	DDX_Check(pDX, IDC_PRINTED, m_printed);
	DDX_Check(pDX, IDC_RETAIN, m_retain);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRetndlg, CDialog)
	//{{AFX_MSG_MAP(CRetndlg)
	ON_BN_CLICKED(IDC_HOLD, OnClickedHold)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_HOUR, OnDeltaposScrHour)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MIN, OnDeltaposScrMin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_WDAY, OnDeltaposScrWday)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MDAY, OnDeltaposScrMday)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MON, OnDeltaposScrMon)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRetndlg message handlers

void CRetndlg::OnClickedHold()
{
	if  (((CButton *)GetDlgItem(IDC_HOLD))->GetCheck())  {
		m_holdtime = m_inittime + 5 * 60;
		Enablehtime(TRUE);
	}                     
	else
		Enablehtime(FALSE);
}		

void CRetndlg::OnOK()
{
	UINT	prin = GetDlgItemInt(IDC_DIP, NULL, FALSE);
	UINT	nprin = GetDlgItemInt(IDC_DINP, NULL, FALSE);
	if  (prin > nprin  &&  AfxMessageBox(IDP_PRINGTNPRIN, MB_YESNO|MB_ICONQUESTION) != IDYES)  {
		CEdit	*ew = (CEdit *) GetDlgItem(IDC_DINP);
		ew->SetSel(0, -1);
		ew->SetFocus();
		return;
	}
	
	CDialog::OnOK();
}

BOOL CRetndlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	if  (m_hold)
		Enablehtime(TRUE);
	return TRUE;
}
     
void CRetndlg::Enablehtime(const BOOL enab)
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

void	CRetndlg::fillintime()
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

void CRetndlg::OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CRetndlg::OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CRetndlg::OnDeltaposScrWday(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CRetndlg::OnDeltaposScrMday(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CRetndlg::OnDeltaposScrMon(NMHDR* pNMHDR, LRESULT* pResult) 
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

const DWORD a113HelpIDs[] = {
	IDC_RETAIN,	IDH_113_245,	// Retention Options Retain on queue after printing
	IDC_PRINTED,	IDH_113_246,	// Retention Options Printed
	IDC_DIP,	IDH_113_247,	// Retention Options 
	IDC_SCR_DIP,	IDH_113_248,	// Retention Options Spin1
	IDC_DINP,	IDH_113_249,	// Retention Options 
	IDC_SCR_DINP,	IDH_113_250,	// Retention Options Spin2
	IDC_HOLD,	IDH_113_251,	// Retention Options Do not print before......
	IDC_HOUR,	IDH_113_252,	// Retention Options 
	IDC_SCR_HOUR,	IDH_113_257,	// Retention Options Spin3
	IDC_MIN,	IDH_113_253,	// Retention Options 
	IDC_SCR_MIN,	IDH_113_258,	// Retention Options Spin3
	IDC_WDAY,	IDH_113_254,	// Retention Options 
	IDC_SCR_WDAY,	IDH_113_259,	// Retention Options Spin3
	IDC_MDAY,	IDH_113_255,	// Retention Options 
	IDC_SCR_MDAY,	IDH_113_260,	// Retention Options Spin3
	IDC_MON,	IDH_113_256,	// Retention Options 
	IDC_SCR_MON,	IDH_113_261,	// Retention Options Spin3
	0, 0
};

BOOL CRetndlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a113HelpIDs[cnt] != 0;  cnt += 2)
		if  (a113HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a113HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
