// LoginHost.cpp : implementation file
//

#include "stdafx.h"
#include "sprsetw.h"
#include "LoginHost.h"
#include "sprsetw.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginHost dialog


CLoginHost::CLoginHost(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginHost::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginHost)
	m_unixhost = _T("");
	m_clienthost = _T("");
	m_username = _T("");
	m_passwd = _T("");
	//}}AFX_DATA_INIT
}


void CLoginHost::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginHost)
	DDX_Text(pDX, IDC_UNIXHOST, m_unixhost);
	DDX_Text(pDX, IDC_CLIENTHOST, m_clienthost);
	DDX_Text(pDX, IDC_USER, m_username);
	DDV_MaxChars(pDX, m_username, 11);
	DDX_Text(pDX, IDC_PASSWD, m_passwd);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginHost, CDialog)
	//{{AFX_MSG_MAP(CLoginHost)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginHost message handlers

const DWORD a103HelpIDs[] = {
	IDC_UNIXHOST,	IDH_103_163,	// Please log in to Unix Host.... 
	IDC_CLIENTHOST,	IDH_103_164,	// Please log in to Unix Host.... 
	IDC_USER,	IDH_103_165,	// Please log in to Unix Host.... 
	IDC_PASSWD,	IDH_103_166,	// Please log in to Unix Host.... 
	0, 0
};

BOOL CLoginHost::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a103HelpIDs[cnt] != 0;  cnt += 2)
		if  (a103HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a103HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
