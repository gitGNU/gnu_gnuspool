// userdlg.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"
#include "userdlg.h"
#include "clientif.h"
#include "ulist.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserdlg dialog

CUserdlg::CUserdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUserdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUserdlg)
	m_mattn = FALSE;
	m_user = "";
	m_wattn = FALSE;
	m_write = FALSE;
	m_mail = FALSE;
	//}}AFX_DATA_INIT
}

void CUserdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserdlg)
	DDX_Check(pDX, IDC_MATTN, m_mattn);
	DDX_CBString(pDX, IDC_USER, m_user);
	DDX_Check(pDX, IDC_WATTN, m_wattn);
	DDX_Check(pDX, IDC_WRITE, m_write);
	DDX_Check(pDX, IDC_MAIL, m_mail);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CUserdlg, CDialog)
	//{{AFX_MSG_MAP(CUserdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserdlg message handlers

BOOL CUserdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_USER);
	if  (((CSpqwApp *)AfxGetApp())->noulist())
		uP->AddString(((CSpqwApp *)AfxGetApp())->m_username);
	else  {
		UUserList	unixusers;
		const  char  FAR *nu;
        while  (nu = unixusers.nextuser())
			uP->AddString(nu);
	}
	return TRUE;
}

const DWORD a117HelpIDs[] = {
	IDC_USER,	IDH_117_173,	// User and mail options 
	IDC_MAIL,	IDH_117_273,	// User and mail options Mail completion
	IDC_WRITE,	IDH_117_274,	// User and mail options Write completion
	IDC_MATTN,	IDH_117_275,	// User and mail options Mail attention
	IDC_WATTN,	IDH_117_276,	// User and mail options Write attention
	0, 0
};

BOOL CUserdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a117HelpIDs[cnt] != 0;  cnt += 2)
		if  (a117HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a117HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
