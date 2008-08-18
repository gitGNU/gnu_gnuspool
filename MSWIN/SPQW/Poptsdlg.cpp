// poptsdlg.cpp : implementation file
//

#include "stdafx.h"
#include "spqw.h"
#include "poptsdlg.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPOptsdlg dialog

CPOptsdlg::CPOptsdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPOptsdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPOptsdlg)
	m_confabort = -1;
	m_probewarn = -1;
	m_polltime = 0;
	m_sjext = -1;
	m_abshold = FALSE;
	//}}AFX_DATA_INIT
}

void CPOptsdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPOptsdlg)
	DDX_Radio(pDX, IDC_NEVER, m_confabort);
	DDX_Radio(pDX, IDC_PROBE_IGNORE, m_probewarn);
	DDX_Text(pDX, IDC_POLLTIME, m_polltime);
	DDV_MinMaxUInt(pDX, m_polltime, 1, 32767);
	DDX_Radio(pDX, IDC_SJXBC, m_sjext);
	DDX_Check(pDX, IDC_ABSHOLD, m_abshold);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPOptsdlg, CDialog)
	//{{AFX_MSG_MAP(CPOptsdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPOptsdlg message handlers

BOOL CPOptsdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}

const DWORD a112HelpIDs[] = {
	IDC_NEVER,	IDH_112_236,	// Program options Never
	IDC_UNPRINTED,	IDH_112_237,	// Program options On unprinted jobs
	IDC_ALWAYS,	IDH_112_238,	// Program options Always
	IDC_PROBE_IGNORE,	IDH_112_239,	// Program options Ignore
	IDC_PROBE_WARN,	IDH_112_240,	// Program options Warn
	IDC_SJXBC,	IDH_112_241,	// Program options XTC
	IDC_SJBAT,	IDH_112_242,	// Program options BAT
	IDC_ABSHOLD,	IDH_112_305,	// Program options Save hold times as dates
	IDC_POLLTIME,	IDH_112_243,	// Program options 
	IDC_SCR_POLLTIME,	IDH_112_244,	// Program options Spin1
	0, 0
};

BOOL CPOptsdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a112HelpIDs[cnt] != 0;  cnt += 2)
		if  (a112HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a112HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
