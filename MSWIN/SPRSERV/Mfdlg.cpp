// mfdlg.cpp : implementation file
//

#include "stdafx.h"
#include "pages.h"
#include "xtini.h"
#include "sprserv.h"
#include "mfdlg.h"             
#include <limits.h>
#include "Sprserv.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMFDlg dialog

CMFDlg::CMFDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMFDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMFDlg)
	m_polltime = 0;
	m_notyet = FALSE;
	m_filename = _T("");
	//}}AFX_DATA_INIT
}

void CMFDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMFDlg)
	DDX_Text(pDX, IDC_POLLTIME, m_polltime);
	DDV_MinMaxUInt(pDX, m_polltime, 1, 32767);
	DDX_Check(pDX, IDC_NOTYET, m_notyet);
	DDX_Text(pDX, IDC_MONFILE, m_filename);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMFDlg, CDialog)
	//{{AFX_MSG_MAP(CMFDlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMFDlg message handlers

BOOL CMFDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}

const DWORD a104HelpIDs[] = {
	IDC_MONFILE,	IDH_104_150,	// File to monitor 
	IDC_POLLTIME,	IDH_104_152,	// File to monitor 
	IDC_SCR_POLLTIME,	IDH_104_153,	// File to monitor Spin1
	IDC_NOTYET,	IDH_104_154,	// File to monitor Not yet
	0, 0
};

BOOL CMFDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a104HelpIDs[cnt] != 0;  cnt += 2)
		if  (a104HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a104HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
