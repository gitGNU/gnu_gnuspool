// LoginHost.cpp : implementation file
//

#include "stdafx.h"
#include "spqw.h"
#include "LoginHost.h"
#include "Spqw.hpp"

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

const DWORD a107HelpIDs[] = {
	IDC_UNIXHOST,	IDH_107_171,	// Please log in to Unix Host.... 
	IDC_CLIENTHOST,	IDH_107_172,	// Please log in to Unix Host.... 
	IDC_USER,	IDH_107_173,	// Please log in to Unix Host.... 
	IDC_PASSWD,	IDH_107_174,	// Please log in to Unix Host.... 
};

BOOL CLoginHost::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a107HelpIDs[cnt] != 0;  cnt += 2)
		if  (a107HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a107HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
