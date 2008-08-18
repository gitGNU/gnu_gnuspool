// formdlg.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"
#include "formdlg.h"
#include "Spqw.hpp"

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
	m_formtype = "";
	m_header = "";
	m_printer = "";
	m_priority = 0;
	m_supph = FALSE;
	//}}AFX_DATA_INIT
}

void CFormdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFormdlg)
	DDX_Text(pDX, IDC_COPIES, m_copies);
	DDV_MinMaxUInt(pDX, m_copies, 0, 255);
	DDX_CBString(pDX, IDC_FORMTYPE, m_formtype);
	DDV_MaxChars(pDX, m_formtype, 34);
	DDX_Text(pDX, IDC_HEADER, m_header);
	DDV_MaxChars(pDX, m_header, 30);
	DDX_CBString(pDX, IDC_PRINTER, m_printer);
	DDV_MaxChars(pDX, m_printer, 58);
	DDX_Text(pDX, IDC_PRIORITY, m_priority);
	DDV_MinMaxUInt(pDX, m_priority, 1, 255);
	DDX_Check(pDX, IDC_SUPPH, m_supph);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFormdlg, CDialog)
	//{{AFX_MSG_MAP(CFormdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static	void	condadd(CComboBox *uF, char *form)
{
	if  (uF->FindStringExact(-1, form) < 0)
		uF->AddString(form);
}
		                           
void	formadd(CComboBox *uF, char *form)
{
	condadd(uF, form);  
	char  *cp;
	if  (cp = strpbrk(form, ".-"))  {
		unsigned  lng = cp - form;		
		char  nform[MAXFORM + 1];
		strncpy(nform, form, lng);
		nform[lng] = '\0';
		condadd(uF, nform);
	}
}		
		
/////////////////////////////////////////////////////////////////////////////
// CFormdlg message handlers

BOOL CFormdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	spdet  &mypriv = ((CSpqwApp *) AfxGetApp())->m_mypriv;
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_COPIES))->SetRange(0, int(mypriv.ispriv(PV_ANYPRIO)? 255: mypriv.spu_cps));
	if  (mypriv.ispriv(PV_ANYPRIO))
		((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_PRIORITY))->SetRange(1, 255);
	else	
		((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_PRIORITY))->SetRange(int(mypriv.spu_minp), int(mypriv.spu_maxp));	
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_PRINTER);
	CComboBox  *uF = (CComboBox *)GetDlgItem(IDC_FORMTYPE);
	plist	&appl = ((CSpqwApp *)AfxGetApp())->m_appptrlist;
	for  (unsigned  cnt = 0;  cnt < appl.number();  cnt++)  {
		uP->AddString(appl[cnt]->get_spp_ptr());             
		formadd(uF, appl[cnt]->get_spp_form());
	}
	joblist &apjl = ((CSpqwApp *)AfxGetApp())->m_appjoblist;
	for  (cnt = 0;  cnt < apjl.number();  cnt++)
		formadd(uF, apjl[cnt]->get_spq_form());
	uP->AddString("");                           
	return TRUE;
}

void CFormdlg::OnOK()
{
	spdet  &mypriv = ((CSpqwApp *) AfxGetApp())->m_mypriv;
	if  (!mypriv.ispriv(PV_FORMS))  {
		char	newform[MAXFORM+1];
		GetDlgItemText(IDC_FORMTYPE, newform, MAXFORM+1);
		if  (!qmatch(CString(mypriv.spu_formallow), newform))  {
			AfxMessageBox(IDP_WRONGFORM, MB_OK|MB_ICONEXCLAMATION);
			CEdit	*ew = (CEdit *) GetDlgItem(IDC_FORMTYPE);
			ew->SetSel(0, -1);
			ew->SetFocus();
			return;
		}
	}		
	if  (!mypriv.ispriv(PV_OTHERP))  {
		char	newform[JPTRNAMESIZE+1];
		GetDlgItemText(IDC_PRINTER, newform, JPTRNAMESIZE+1);
		if  (!issubset(CString(mypriv.spu_ptrallow), CString(newform)))  {
			AfxMessageBox(IDP_WRONGPTR, MB_OK|MB_ICONEXCLAMATION);
			CEdit	*ew = (CEdit *) GetDlgItem(IDC_PRINTER);
			ew->SetSel(0, -1);
			ew->SetFocus();
			return;
		}
	}		
	CDialog::OnOK();
}

const DWORD a105HelpIDs[] = {
	IDC_FORMTYPE,	IDH_105_163,	// Form Header Printer Copies Priority 
	IDC_HEADER,	IDH_105_164,	// Form Header Printer Copies Priority 
	IDC_SUPPH,	IDH_105_165,	// Form Header Printer Copies Priority Suppress
	IDC_PRINTER,	IDH_105_166,	// Form Header Printer Copies Priority 
	IDC_COPIES,	IDH_105_167,	// Form Header Printer Copies Priority 
	IDC_SCR_COPIES,	IDH_105_168,	// Form Header Printer Copies Priority Spin2
	IDC_PRIORITY,	IDH_105_169,	// Form Header Printer Copies Priority 
	IDC_SCR_PRIORITY,	IDH_105_170,	// Form Header Printer Copies Priority Spin1
	0, 0
};

BOOL CFormdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a105HelpIDs[cnt] != 0;  cnt += 2)
		if  (a105HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a105HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
