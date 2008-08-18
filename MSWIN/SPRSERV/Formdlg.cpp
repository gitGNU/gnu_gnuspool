// formdlg.cpp : implementation file
//

#include "stdafx.h"
#include "defaults.h"
#include "resource.h"
#include "formdlg.h"
#include "Sprserv.hpp"

extern	BOOL	qmatch(CString &, const char FAR *);
extern	BOOL	issubset(CString &patterna, CString &patternb);

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFormdlg dialog

CFormdlg::CFormdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFormdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFormdlg)
	m_copies = 0;
	m_header = "";
	m_supph = FALSE;
	m_formtype = "";
	m_printer = "";
	m_priority = 0;
	//}}AFX_DATA_INIT
}

void CFormdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFormdlg)
	DDX_Text(pDX, IDC_COPIES, m_copies);
	DDV_MinMaxUInt(pDX, m_copies, 0, 255);
	DDX_Text(pDX, IDC_HEADER, m_header);
	DDX_Check(pDX, IDC_SUPPH, m_supph);
	DDX_Text(pDX, IDC_FORMTYPE, m_formtype);
	DDX_Text(pDX, IDC_PRINTER, m_printer);
	DDX_Text(pDX, IDC_PRIORITY, m_priority);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFormdlg, CDialog)
	//{{AFX_MSG_MAP(CFormdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFormdlg message handlers

BOOL CFormdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_COPIES))->SetRange(0, m_maxcps);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_PRIORITY))->SetRange(m_minp, m_maxp);	
	return TRUE;
}

void CFormdlg::OnOK()
{
	if  (!m_formok)  {
		char	newform[MAXFORM+1];
		GetDlgItemText(IDC_FORMTYPE, newform, MAXFORM+1);
		if  (!qmatch(m_allowform, newform))  {
			AfxMessageBox(IDP_WRONGFORM, MB_OK|MB_ICONEXCLAMATION);
			CEdit	*ew = (CEdit *) GetDlgItem(IDC_FORMTYPE);
			ew->SetSel(0, -1);
			ew->SetFocus();
			return;
		}
	}		
	if  (!m_ptrok)  {
		char	newptr[JPTRNAMESIZE+1];
		GetDlgItemText(IDC_PRINTER, newptr, JPTRNAMESIZE+1);
		if  (!issubset(m_allowptr, CString(newptr)))  {
			AfxMessageBox(IDP_WRONGPTR, MB_OK|MB_ICONEXCLAMATION);
			CEdit	*ew = (CEdit *) GetDlgItem(IDC_PRINTER);
			ew->SetSel(0, -1);
			ew->SetFocus();
			return;
		}
	}		
	CDialog::OnOK();
}

const DWORD a102HelpIDs[] = {
	IDC_FORMTYPE,	IDH_102_155,	// Form Header Printer Copies Priority 
	IDC_HEADER,	IDH_102_156,	// Form Header Printer Copies Priority 
	IDC_SUPPH,	IDH_102_157,	// Form Header Printer Copies Priority Suppress
	IDC_PRINTER,	IDH_102_158,	// Form Header Printer Copies Priority 
	IDC_COPIES,	IDH_102_159,	// Form Header Printer Copies Priority 
	IDC_SCR_COPIES,	IDH_102_160,	// Form Header Printer Copies Priority Spin1
	IDC_PRIORITY,	IDH_102_161,	// Form Header Printer Copies Priority 
	IDC_SCR_PRIORITY,	IDH_102_162,	// Form Header Printer Copies Priority Spin2
	0, 0
};

BOOL CFormdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a102HelpIDs[cnt] != 0;  cnt += 2)
		if  (a102HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a102HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
