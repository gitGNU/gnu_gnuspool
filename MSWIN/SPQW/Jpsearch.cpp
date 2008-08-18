// jpsearch.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "spqw.h"
#include "jpsearch.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJPSearch dialog

CJPSearch::CJPSearch(CWnd* pParent /*=NULL*/)
	: CDialog(CJPSearch::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJPSearch)
	m_sstring = "";
	m_sforward = -1;
	m_sdevice = FALSE;
	m_sformtype = FALSE;
	m_sjtitle = FALSE;
	m_sprinter = FALSE;
	m_suser = FALSE;
	m_swraparound = FALSE;
	//}}AFX_DATA_INIT
}

void CJPSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJPSearch)
	DDX_Text(pDX, IDC_LOOKFOR, m_sstring);
	DDV_MaxChars(pDX, m_sstring, 128);
	DDX_Radio(pDX, IDC_SEARCH_FORWARD, m_sforward);
	DDX_Check(pDX, IDC_SRCH_DEVICE, m_sdevice);
	DDX_Check(pDX, IDC_SRCH_FORMTYPE, m_sformtype);
	DDX_Check(pDX, IDC_SRCH_JTITLE, m_sjtitle);
	DDX_Check(pDX, IDC_SRCH_PRINTER, m_sprinter);
	DDX_Check(pDX, IDC_SRCH_USER, m_suser);
	DDX_Check(pDX, IDC_WRAPROUND, m_swraparound);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJPSearch, CDialog)
	//{{AFX_MSG_MAP(CJPSearch)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJPSearch message handlers

void CJPSearch::OnOK()
{
	char	ttext[20];
	if  (GetDlgItemText(IDC_LOOKFOR, ttext, sizeof(ttext)) == 0)  {
		AfxMessageBox(IDP_JPNOSEARCHS, MB_OK|MB_ICONEXCLAMATION);
		CEdit  *ew = (CEdit *) GetDlgItem(IDC_LOOKFOR);
		ew->SetSel(0, -1);
		ew->SetFocus();
		return;
	}
	for  (int cnt = IDC_SRCH_USER;  cnt <= IDC_SRCH_PRINTER;  cnt++)
		if  (((CButton *)GetDlgItem(cnt))->GetCheck())
			goto  ok;
	AfxMessageBox(IDP_JPNOITEM, MB_OK|MB_ICONEXCLAMATION);
	return;
ok:
	CDialog::OnOK();
}

BOOL CJPSearch::OnInitDialog()
{
	CDialog::OnInitDialog();              
	if  (m_which == IDD_JOBSRCH)
		GetDlgItem(IDC_SRCH_DEVICE)->EnableWindow(FALSE);
	else  {
		GetDlgItem(IDC_SRCH_JTITLE)->EnableWindow(FALSE);
		GetDlgItem(IDC_SRCH_USER)->EnableWindow(FALSE);
	}	                                         
	
	return TRUE;
}

const DWORD a115HelpIDs[] = {
	IDC_LOOKFOR,	IDH_115_263,	// Search for job/printer strings 
	IDC_SEARCH_FORWARD,	IDH_115_264,	// Search for job/printer strings Forward
	IDC_SEARCH_BACK,	IDH_115_265,	// Search for job/printer strings Backward
	IDC_WRAPROUND,	IDH_115_266,	// Search for job/printer strings Wrap Around
	IDC_SRCH_USER,	IDH_115_268,	// Search for job/printer strings User
	IDC_SRCH_JTITLE,	IDH_115_269,	// Search for job/printer strings Job title
	IDC_SRCH_FORMTYPE,	IDH_115_270,	// Search for job/printer strings Formtype
	IDC_SRCH_DEVICE,	IDH_115_271,	// Search for job/printer strings Device
	IDC_SRCH_PRINTER,	IDH_115_272,	// Search for job/printer strings Printer
	0, 0
};

BOOL CJPSearch::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a115HelpIDs[cnt] != 0;  cnt += 2)
		if  (a115HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a115HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
