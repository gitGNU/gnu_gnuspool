// prform.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"
#include "prform.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//. In formdlg.cpp

extern	void	formadd(CComboBox *, char *);

/////////////////////////////////////////////////////////////////////////////
// CPrform dialog

CPrform::CPrform(CWnd* pParent /*=NULL*/)
	: CDialog(CPrform::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrform)
	m_formtype = "";
	//}}AFX_DATA_INIT
}

void CPrform::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrform)
	DDX_CBString(pDX, IDC_FORMTYPE, m_formtype);
	DDV_MaxChars(pDX, m_formtype, 34);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPrform, CDialog)
	//{{AFX_MSG_MAP(CPrform)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrform message handlers

BOOL CPrform::OnInitDialog()
{
	CDialog::OnInitDialog();	

	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_FORMTYPE);
	joblist	&appl = ((CSpqwApp *)AfxGetApp())->m_appjoblist;
	for  (unsigned  cnt = 0;  cnt < appl.number();  cnt++)
		formadd(uP, appl[cnt]->get_spq_form());
	plist	&applp = ((CSpqwApp *)AfxGetApp())->m_appptrlist;
	for  (cnt = 0;  cnt < applp.number();  cnt++)
		formadd(uP, applp[cnt]->get_spp_form());
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

const DWORD a111HelpIDs[] = {
	IDC_FORMTYPE,	IDH_111_163,	// Set printer form type 
	0, 0
};

BOOL CPrform::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a111HelpIDs[cnt] != 0;  cnt += 2)
		if  (a111HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a111HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
