// netstat.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"
#include "netstat.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetstat dialog


CNetstat::CNetstat(CWnd* pParent /*=NULL*/)
	: CDialog(CNetstat::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNetstat)
	//}}AFX_DATA_INIT
}

void CNetstat::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetstat)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNetstat, CDialog)
	//{{AFX_MSG_MAP(CNetstat)
	ON_CBN_SELCHANGE(IDC_HOST, OnSelchangeHost)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNetstat message handlers

void CNetstat::OnSelchangeHost()
{
	CComboBox	*cb = (CComboBox *)GetDlgItem(IDC_HOST);	
    int	sel = cb->GetCurSel();
   	((CButton *) GetDlgItem(IDC_NS_PROBESENT))->SetCheck(0);
   	((CButton *) GetDlgItem(IDC_NS_SYNCREQ))->SetCheck(0);
   	((CButton *) GetDlgItem(IDC_NS_SYNCDONE))->SetCheck(0);
    if  (sel < 0)
    	return;
    netid_t	hostid = netid_t(cb->GetItemData(sel));
    remote	*rp;
    if  (rp = pending_q.find(hostid))  {
		((CButton *) GetDlgItem(IDC_NS_PROBESENT))->SetCheck(1);
		return;
	}
	if  (!(rp = current_q.find(hostid)))
		return;
	((CButton *) GetDlgItem(IDC_NS_PROBESENT))->SetCheck(1);
	if  (rp->sockfd == INVALID_SOCKET)
		return;                                     
	((CButton *) GetDlgItem(IDC_CONNECTED))->SetCheck(1);
	if  (rp->is_sync >= NSYNC_REQ)  {
	   	((CButton *) GetDlgItem(IDC_NS_SYNCREQ))->SetCheck(1);
	   	if  (rp->is_sync >= NSYNC_OK)
	   		((CButton *) GetDlgItem(IDC_NS_SYNCDONE))->SetCheck(1);
	}
}

BOOL CNetstat::OnInitDialog()
{
	CDialog::OnInitDialog();
	remote	*rp;
	CComboBox	*cb = (CComboBox *)GetDlgItem(IDC_HOST);	
	current_q.setfirst();
	pending_q.setfirst();
	while  (rp = current_q.next())
		cb->SetItemData(cb->AddString(rp->namefor()), DWORD(rp->hostid));
	while  (rp = pending_q.next())
		cb->SetItemData(cb->AddString(rp->namefor()), DWORD(rp->hostid));
	return TRUE;
}

const DWORD a108HelpIDs[] = {
	IDC_HOST,	IDH_108_175,	// Network Status 
	IDC_NS_PROBESENT,	IDH_108_176,	// Network Status Probe sent (or no probe required)
	IDC_CONNECTED,	IDH_108_177,	// Network Status TCP Connection made
	IDC_NS_SYNCREQ,	IDH_108_178,	// Network Status Sync Requested
	IDC_NS_SYNCDONE,	IDH_108_179,	// Network Status Sync Completed
	0, 0
};

BOOL CNetstat::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a108HelpIDs[cnt] != 0;  cnt += 2)
		if  (a108HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a108HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
