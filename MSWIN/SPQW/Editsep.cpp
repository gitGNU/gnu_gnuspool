// editsep.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "spqw.h"
#include "editsep.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditsep dialog


CEditsep::CEditsep(CWnd* pParent /*=NULL*/)
	: CDialog(CEditsep::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditsep)
	m_sepvalue = "";
	//}}AFX_DATA_INIT
}

void CEditsep::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditsep)
	DDX_Text(pDX, IDC_SEPVALUE, m_sepvalue);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditsep, CDialog)
	//{{AFX_MSG_MAP(CEditsep)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEditsep message handlers

const DWORD a103HelpIDs[] = {
	IDC_SEPVALUE,	IDH_103_155,	// New separator string 
	0, 0
};

BOOL CEditsep::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a103HelpIDs[cnt] != 0;  cnt += 2)
		if  (a103HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a103HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
