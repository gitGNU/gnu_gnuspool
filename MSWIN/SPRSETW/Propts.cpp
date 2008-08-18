// propts.cpp : implementation file
//

#include "stdafx.h"
#include "sprsetw.h"
#include "propts.h"
#include "Sprsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropts dialog

CPropts::CPropts(CWnd* pParent /*=NULL*/)
	: CDialog(CPropts::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPropts)
	m_interpolate = FALSE;
	m_textmode = FALSE;
	m_verbose = FALSE;
	//}}AFX_DATA_INIT
}

void CPropts::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropts)
	DDX_Check(pDX, IDC_INTERPOLATE, m_interpolate);
	DDX_Check(pDX, IDC_TEXTMODE, m_textmode);
	DDX_Check(pDX, IDC_VERBOSE, m_verbose);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPropts, CDialog)
	//{{AFX_MSG_MAP(CPropts)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropts message handlers

const DWORD a107HelpIDs[] = {
	IDC_VERBOSE,	IDH_107_201,	// Program Options Verbose
	IDC_TEXTMODE,	IDH_107_202,	// Program Options Text Mode
	IDC_INTERPOLATE,	IDH_107_203,	// Program Options Interpolate
	0, 0
};

BOOL CPropts::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a107HelpIDs[cnt] != 0;  cnt += 2)
		if  (a107HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a107HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
