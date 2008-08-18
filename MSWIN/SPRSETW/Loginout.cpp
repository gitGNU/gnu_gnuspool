// Loginout.cpp : implementation file
//

#include "stdafx.h"
#include "sprsetw.h"
#include "Loginout.h"
#include "Sprsetw.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginout dialog


CLoginout::CLoginout(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginout::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginout)
	m_winuser = _T("");
	m_unixuser = _T("");
	//}}AFX_DATA_INIT
}


void CLoginout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginout)
	DDX_Text(pDX, IDC_WINUSER, m_winuser);
	DDX_Text(pDX, IDC_UNIXUSER, m_unixuser);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginout, CDialog)
	//{{AFX_MSG_MAP(CLoginout)
	ON_BN_CLICKED(IDC_LOGIN, OnLogin)
	ON_BN_CLICKED(IDC_LOGOUT, OnLogout)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginout message handlers

void CLoginout::OnLogin() 
{
	EndDialog(IDC_LOGIN);
}

void CLoginout::OnLogout() 
{
	EndDialog(IDC_LOGOUT);
}

const DWORD a104HelpIDs[] = {
	IDC_UNIXUSER,	IDH_104_167,	// Login as new user or log out 
	IDC_WINUSER,	IDH_104_168,	// Login as new user or log out 
	IDC_LOGIN,	IDH_104_169,	// Login as new user or log out Login as new user
	IDC_LOGOUT,	IDH_104_170,	// Login as new user or log out Log out
	0, 0
};

BOOL CLoginout::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a104HelpIDs[cnt] != 0;  cnt += 2)
		if  (a104HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a104HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
