// opdlg22.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"
#include "opdlg22.h"
#include "clientif.h"
#include "ulist.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
                  
extern	BOOL	repattok(const  CString  &);

/////////////////////////////////////////////////////////////////////////////
// COPdlg22 dialog


COPdlg22::COPdlg22(CWnd* pParent /*=NULL*/)
	: CDialog(COPdlg22::IDD, pParent)
{
	//{{AFX_DATA_INIT(COPdlg22)
	m_punpjobs = -1;
	m_onlyprin = "";
	m_onlyu = "";
	m_jinclude = -1;
	m_onlytitle = "";
	//}}AFX_DATA_INIT
}

void COPdlg22::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COPdlg22)
	DDX_Radio(pDX, IDC_ALLJOBS, m_punpjobs);
	DDX_CBString(pDX, IDC_OPTR, m_onlyprin);
	DDX_CBString(pDX, IDC_OUSER, m_onlyu);
	DDX_Radio(pDX, IDC_JINCNONULL, m_jinclude);
	DDX_Text(pDX, IDC_OTITLE, m_onlytitle);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COPdlg22, CDialog)
	//{{AFX_MSG_MAP(COPdlg22)
	ON_BN_CLICKED(IDC_SETALL, OnSetall)
	ON_BN_CLICKED(IDC_CLEARALL, OnClearall)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// COPdlg22 message handlers

BOOL COPdlg22::OnInitDialog()
{
	CDialog::OnInitDialog();
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_OPTR);
	plist	&appl = ((CSpqwApp *)AfxGetApp())->m_appptrlist;
	for  (unsigned  cnt = 0;  cnt < appl.number();  cnt++)
		uP->AddString(appl[cnt]->get_spp_ptr());
	uP->AddString("");
	uP = (CComboBox *)GetDlgItem(IDC_OUSER);
	uP->AddString("");
	if  (((CSpqwApp *)AfxGetApp())->noulist())
		uP->AddString(((CSpqwApp *)AfxGetApp())->m_username);
	else  {
    	UUserList	unixusers;
		const  char  FAR *nu;
		while  (nu = unixusers.nextuser())
			uP->AddString(nu);
	}
	checkclass();
	if  (!m_mayoverride)  {
		for  (int idn = 0;  idn <= IDC_CC_P - IDC_CC_A; idn++)
			if  (!(m_maxclass & (1L << idn)))
				GetDlgItem(idn+IDC_CC_A)->EnableWindow(FALSE);	
		for  (idn = 0;  idn <= IDC_CC_P2 - IDC_CC_A2; idn++)
			if  (!(m_maxclass & (1L << (idn+16))))
				GetDlgItem(idn+IDC_CC_A2)->EnableWindow(FALSE);	
	}
	return TRUE;
}

BOOL	COPdlg22::checkpattern(const int item)
{
	char	buff[100];
	GetDlgItemText(item, buff, sizeof(buff));
	if  (repattok(buff))
		return  TRUE;
	CEdit	*ew = (CEdit *) GetDlgItem(item);
	ew->SetSel(0, -1);
	ew->SetFocus();
    return  FALSE;
}
	
void COPdlg22::OnOK()
{
	scanclass();
	if  (!m_classc)
		AfxMessageBox(IDP_ZEROCLASS, MB_OK|MB_ICONEXCLAMATION);
	else  if  (checkpattern(IDC_OUSER) && checkpattern(IDC_OPTR) && checkpattern(IDC_OTITLE))
		CDialog::OnOK();
}

void COPdlg22::OnSetall()
{
	if  (m_mayoverride)  {
		scanclass();
		if  (m_classc == m_maxclass)
			m_classc = U_MAX_CLASS;
		else
			m_classc = m_maxclass;
	}
	else
		m_classc = m_maxclass;
	checkclass();
}

void COPdlg22::OnClearall()
{
	//  If overriding class, reduce class to standard as a first step.
	if  (m_mayoverride)  {
		scanclass();
		if  (m_classc & ~m_maxclass)
			m_classc = m_maxclass;
		else
			m_classc = 0;
	}
	else
		m_classc = 0;
	checkclass();
}

void COPdlg22::checkclass()
{
	for  (int idn = 0;  idn <= IDC_CC_P - IDC_CC_A; idn++)
		((CButton *)GetDlgItem(idn + IDC_CC_A))->SetCheck(m_classc & (1L << idn) ? 1: 0);
	for  (idn = 0;  idn <= IDC_CC_P2 - IDC_CC_A2; idn++)
		((CButton *)GetDlgItem(idn + IDC_CC_A2))->SetCheck(m_classc & (1L << (idn+16)) ? 1: 0);
}

void COPdlg22::scanclass()
{
	m_classc = 0;
	for  (int  idn = 0;  idn <= IDC_CC_P - IDC_CC_A;  idn++)
		if  (((CButton *)GetDlgItem(idn + IDC_CC_A))->GetCheck())
			m_classc |= (1L << idn);
	for  (idn = 0;  idn <= IDC_CC_P2 - IDC_CC_A2;  idn++)
		if  (((CButton *)GetDlgItem(idn + IDC_CC_A2))->GetCheck())
			m_classc |= (1L << (idn+16));
}

const DWORD a109HelpIDs[] = {
	IDC_OPTR,	IDH_109_180,	// Restrict display to ..... 
	IDC_OUSER,	IDH_109_181,	// Restrict display to ..... 
	IDC_OTITLE,	IDH_109_182,	// Restrict display to ..... 
	IDC_PUNPRESTR,	IDH_109_183,	// Restrict display to ..... Display
	IDC_ALLJOBS,	IDH_109_184,	// Restrict display to ..... All jobs
	IDC_UNPJOBS,	IDH_109_185,	// Restrict display to ..... Unprinted jobs only
	IDC_PJOBS,	IDH_109_186,	// Restrict display to ..... Printed jobs only
	IDC_JINCNONULL,	IDH_109_187,	// Restrict display to ..... Only for given printer
	IDC_JINCNULL,	IDH_109_188,	// Restrict display to ..... For given printer, plus null
	IDC_JINCALL,	IDH_109_189,	// Restrict display to ..... Regardless of printer
	IDC_SETALL,	IDH_109_190,	// Restrict display to ..... Set All
	IDC_CLEARALL,	IDH_109_191,	// Restrict display to ..... Clear all
	IDC_CC_A,	IDH_109_192,	// Restrict display to ..... A
	IDC_CC_B,	IDH_109_193,	// Restrict display to ..... B
	IDC_CC_C,	IDH_109_194,	// Restrict display to ..... C
	IDC_CC_D,	IDH_109_195,	// Restrict display to ..... D
	IDC_CC_E,	IDH_109_196,	// Restrict display to ..... E
	IDC_CC_F,	IDH_109_197,	// Restrict display to ..... F
	IDC_CC_G,	IDH_109_198,	// Restrict display to ..... G
	IDC_CC_H,	IDH_109_199,	// Restrict display to ..... H
	IDC_CC_I,	IDH_109_200,	// Restrict display to ..... I
	IDC_CC_J,	IDH_109_201,	// Restrict display to ..... J
	IDC_CC_K,	IDH_109_202,	// Restrict display to ..... K
	IDC_CC_L,	IDH_109_203,	// Restrict display to ..... L
	IDC_CC_M,	IDH_109_204,	// Restrict display to ..... M
	IDC_CC_N,	IDH_109_205,	// Restrict display to ..... N
	IDC_CC_O,	IDH_109_206,	// Restrict display to ..... O
	IDC_CC_P,	IDH_109_207,	// Restrict display to ..... P
	IDC_CC_A2,	IDH_109_208,	// Restrict display to ..... a
	IDC_CC_B2,	IDH_109_209,	// Restrict display to ..... b
	IDC_CC_C2,	IDH_109_210,	// Restrict display to ..... c
	IDC_CC_D2,	IDH_109_211,	// Restrict display to ..... d
	IDC_CC_E2,	IDH_109_212,	// Restrict display to ..... e
	IDC_CC_F2,	IDH_109_213,	// Restrict display to ..... f
	IDC_CC_G2,	IDH_109_214,	// Restrict display to ..... g
	IDC_CC_H2,	IDH_109_215,	// Restrict display to ..... h
	IDC_CC_I2,	IDH_109_216,	// Restrict display to ..... i
	IDC_CC_J2,	IDH_109_217,	// Restrict display to ..... j
	IDC_CC_K2,	IDH_109_218,	// Restrict display to ..... k
	IDC_CC_L2,	IDH_109_219,	// Restrict display to ..... l
	IDC_CC_M2,	IDH_109_220,	// Restrict display to ..... m
	IDC_CC_N2,	IDH_109_221,	// Restrict display to ..... n
	IDC_CC_O2,	IDH_109_222,	// Restrict display to ..... o
	IDC_CC_P2,	IDH_109_223,	// Restrict display to ..... p
	0, 0
};

BOOL COPdlg22::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a109HelpIDs[cnt] != 0;  cnt += 2)
		if  (a109HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a109HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
