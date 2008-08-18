// vmdlg.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"
#include "vmdlg.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVMdlg dialog

CVMdlg::CVMdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVMdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVMdlg)
	m_isstart = FALSE;
	m_isend = FALSE;
	m_ishat = FALSE;
	//}}AFX_DATA_INIT
}

void CVMdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVMdlg)
	DDX_Check(pDX, IDC_SETSTART, m_isstart);
	DDX_Check(pDX, IDC_SETEND, m_isend);
	DDX_Check(pDX, IDC_SETHAT, m_ishat);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVMdlg, CDialog)
	//{{AFX_MSG_MAP(CVMdlg)
	ON_BN_CLICKED(IDC_QUITDATA, OnClickedQuitdata)
	ON_BN_CLICKED(IDC_SETEND, OnClickedSetend)
	ON_BN_CLICKED(IDC_SETHAT, OnClickedSethat)
	ON_BN_CLICKED(IDC_SETSTART, OnClickedSetstart)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVMdlg message handlers

void CVMdlg::OnClickedQuitdata()
{
	EndDialog(IDC_QUITDATA);
}

void CVMdlg::OnClickedSetend()
{
	m_isend = TRUE;
	GetDlgItem(IDC_SETEND)->EnableWindow(FALSE);
}

void CVMdlg::OnClickedSethat()
{
	m_ishat = TRUE;	
	GetDlgItem(IDC_SETHAT)->EnableWindow(FALSE);		
}

void CVMdlg::OnClickedSetstart()
{
	m_isstart = TRUE;	
	GetDlgItem(IDC_SETSTART)->EnableWindow(FALSE);
}

BOOL CVMdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
    if  (m_isstart || !m_enabstart)
		GetDlgItem(IDC_SETSTART)->EnableWindow(FALSE);
	if  (m_isend || !m_enabend)
		GetDlgItem(IDC_SETEND)->EnableWindow(FALSE);
	if  (m_ishat || !m_enabhat)
		GetDlgItem(IDC_SETHAT)->EnableWindow(FALSE);		
	return TRUE;
}

const DWORD a119HelpIDs[] = {
	IDC_SETSTART,	IDH_119_301,	// View Options Start page
	IDC_SETEND,	IDH_119_302,	// View Options End page
	IDC_SETHAT,	IDH_119_303,	// View Options Set 
	IDC_QUITDATA,	IDH_119_304,	// View Options Quit view
	0, 0
};

BOOL CVMdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a119HelpIDs[cnt] != 0;  cnt += 2)
		if  (a119HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a119HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
