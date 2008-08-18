// jdsearch.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "spqw.h"
#include "jdsearch.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJDSearch dialog

CJDSearch::CJDSearch(CWnd* pParent /*=NULL*/)
	: CDialog(CJDSearch::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJDSearch)
	m_ignorecase = FALSE;
	m_lookfor = "";
	m_wrapround = FALSE;
	m_sforward = -1;
	//}}AFX_DATA_INIT
}

void CJDSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJDSearch)
	DDX_Check(pDX, IDC_IGNORECASE, m_ignorecase);
	DDX_Text(pDX, IDC_LOOKFOR, m_lookfor);
	DDX_Check(pDX, IDC_WRAPROUND, m_wrapround);
	DDX_Radio(pDX, IDC_SEARCH_FORWARD, m_sforward);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJDSearch, CDialog)
	//{{AFX_MSG_MAP(CJDSearch)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJDSearch message handlers

const DWORD a114HelpIDs[] = {
	IDC_LOOKFOR,	IDH_114_263,	// Search job data 
	IDC_SEARCH_FORWARD,	IDH_114_264,	// Search job data Forward
	IDC_SEARCH_BACK,	IDH_114_265,	// Search job data Backward
	IDC_WRAPROUND,	IDH_114_266,	// Search job data Wrap Around
	IDC_IGNORECASE,	IDH_114_267,	// Search job data Ignore Case
	0, 0
};

BOOL CJDSearch::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a114HelpIDs[cnt] != 0;  cnt += 2)
		if  (a114HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a114HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
