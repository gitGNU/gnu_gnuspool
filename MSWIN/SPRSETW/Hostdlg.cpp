// hostdlg.cpp : implementation file
//

#include "stdafx.h"
#include "sprsetw.h"
#include "hostdlg.h"
#include <limits.h>
#include <ctype.h>
#include "Sprsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHostdlg dialog

CHostdlg::CHostdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHostdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHostdlg)
	m_probefirst = FALSE;
	m_timeout = 0;
	m_hostname = "";
	m_aliasname = "";
	//}}AFX_DATA_INIT
}

void CHostdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHostdlg)
	DDX_Check(pDX, IDC_PROBEFIRST, m_probefirst);
	DDX_Text(pDX, IDC_TIMEOUT, m_timeout);
	DDV_MinMaxUInt(pDX, m_timeout, 1, 32767);
	DDX_Text(pDX, IDC_HOSTNAME, m_hostname);
	DDV_MaxChars(pDX, m_hostname, 14);
	DDX_Text(pDX, IDC_ALIASNAME, m_aliasname);
	DDV_MaxChars(pDX, m_aliasname, 14);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CHostdlg, CDialog)
	//{{AFX_MSG_MAP(CHostdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHostdlg message handlers

BOOL CHostdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}

void CHostdlg::Refocus(const int nCtrl)
{                                      
	CEdit	*ew = (CEdit *) GetDlgItem(nCtrl);
	ew->SetSel(0, -1);
	ew->SetFocus();
}	
	
void CHostdlg::OnOK()
{
	char	hostname[HOSTNSIZE + 10], aliasname[HOSTNSIZE+10];
	
	GetDlgItemText(IDC_HOSTNAME, hostname, sizeof(hostname)-1);
	GetDlgItemText(IDC_ALIASNAME, aliasname, sizeof(aliasname)-1);
	hostname[sizeof(hostname)-1] = aliasname[sizeof(aliasname)-1] = '\0';

	if  (hostname[0] == '\0')  {
		AfxMessageBox(IDP_NOHOSTNAME, MB_OK|MB_ICONEXCLAMATION);
		Refocus(IDC_HOSTNAME);
		return;
	}	
	if  (isdigit(hostname[0])  &&  (aliasname[0] == '\0' || !isalpha(aliasname[0])))  {
		AfxMessageBox(IDP_NOALIASNAME, MB_OK|MB_ICONEXCLAMATION);
		Refocus(IDC_ALIASNAME);
		return;
	}	
	if  (strcmp(hostname, aliasname) == 0)  {
		AfxMessageBox(IDP_ALIASSAMEASHOST, MB_OK|MB_ICONEXCLAMATION);
		Refocus(IDC_ALIASNAME);
		return;
	}
	if  (isdigit(hostname[0]))  {
		netid_t	hid = inet_addr(hostname);
		if  (hid == INADDR_NONE)  {
			AfxMessageBox(IDP_BADINETADDR, MB_OK|MB_ICONEXCLAMATION); 
			Refocus(IDC_HOSTNAME);
			return;
		}
		if  (clashcheck(hid)) {
			AfxMessageBox(IDP_HIDCLASH, MB_OK|MB_ICONEXCLAMATION);
			Refocus(IDC_HOSTNAME);
			return;
		}	
		if  (hid == Locparams.myhostid)  {
			AfxMessageBox(IDP_HIDCLASHMINE, MB_OK|MB_ICONEXCLAMATION);
			Refocus(IDC_HOSTNAME);
			return;
		}	
		if  (clashcheck(aliasname))  {
			AfxMessageBox(IDP_ALIASCLASH, MB_OK|MB_ICONEXCLAMATION);
			Refocus(IDC_ALIASNAME);
			return;
		}	        
		m_hid = hid;
	}
	else  {
		if  (clashcheck(hostname))  {
			AfxMessageBox(IDP_HOSTCLASH, MB_OK|MB_ICONEXCLAMATION);	
			Refocus(IDC_HOSTNAME);
			return;
		}
		if  (aliasname[0]  &&  clashcheck(aliasname))  {
			AfxMessageBox(IDP_ALIASCLASH, MB_OK|MB_ICONEXCLAMATION);
			Refocus(IDC_ALIASNAME);
			return;
		}
		hostent	FAR *hp = gethostbyname(hostname);
		netid_t	nid;
		if  (!hp  ||  (nid = *(netid_t FAR *) hp->h_addr) == 0L  ||  nid == -1L)  {
			AfxMessageBox(IDP_UNKNOWNHOSTNAME, MB_OK|MB_ICONEXCLAMATION);	
			Refocus(IDC_HOSTNAME);
			return;
		}
		if  (nid == Locparams.myhostid)  {
			AfxMessageBox(IDP_MYHOSTNAME, MB_OK|MB_ICONEXCLAMATION);	
			Refocus(IDC_HOSTNAME);
			return;
		}
		m_hid = nid;
	}
		
	CDialog::OnOK();
}

const DWORD a102HelpIDs[] = {
	IDC_HOSTNAME,	IDH_102_158,	// New Host Name 
	IDC_ALIASNAME,	IDH_102_159,	// New Host Name 
	IDC_TIMEOUT,	IDH_102_160,	// New Host Name 
	IDC_SCR_TIMEOUT,	IDH_102_161,	// New Host Name Spin1
	IDC_PROBEFIRST,	IDH_102_162,	// New Host Name Probe first
	0, 0
};

BOOL CHostdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a102HelpIDs[cnt] != 0;  cnt += 2)
		if  (a102HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a102HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
