// Refreshconn.cpp : implementation file
//

#include "stdafx.h"
#include "pages.h"
#include "xtini.h"
#include "sprserv.h"
#include "Refreshconn.h"
#include "Sprserv.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRefreshconn dialog


CRefreshconn::CRefreshconn(CWnd* pParent /*=NULL*/)
	: CDialog(CRefreshconn::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRefreshconn)
	m_servname = _T("");
	m_action = -1;
	//}}AFX_DATA_INIT
}


void CRefreshconn::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRefreshconn)
	DDX_Text(pDX, IDC_SERVNAME, m_servname);
	DDX_Radio(pDX, IDC_REFRESHCONN, m_action);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRefreshconn, CDialog)
	//{{AFX_MSG_MAP(CRefreshconn)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRefreshconn message handlers

const DWORD a110HelpIDs[] = {
	IDC_SERVNAME,	IDH_110_242,	// Connection Died 
	IDC_REFRESHCONN,	IDH_110_243,	// Connection Died Refresh it
	IDC_REFRDONTASK,	IDH_110_244,	// Connection Died Refresh it and don't ask again
	IDC_TERMINATE,	IDH_110_245,	// Connection Died Terminate
	0, 0
};

BOOL CRefreshconn::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a110HelpIDs[cnt] != 0;  cnt += 2)
		if  (a110HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a110HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	return CDialog::OnHelpInfo(pHelpInfo);
}
