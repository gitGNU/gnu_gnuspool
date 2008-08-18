// opdlg.cpp : implementation file
//

#include "stdafx.h"
#include "defaults.h"
#include "winnetwk.h"
#include "spq.h"
#include "restrict.h"
#include "rowview.h"
#include "jident.h"
#include "joblist.h"
#include "pident.h"
#include "plist.h"
#include "spqw.h"
#include "opdlg.h"
#include "clientif.h"
#include "ulist.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COPdlg dialog

COPdlg::COPdlg(CWnd* pParent /*=NULL*/)
	: CDialog(COPdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COPdlg)
	m_onlyprin = "";
	m_onlyu = "";
	m_punpjobs = -1;
	//}}AFX_DATA_INIT
}

void COPdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COPdlg)
	DDX_CBString(pDX, IDC_OPTR, m_onlyprin);
	DDX_CBString(pDX, IDC_OUSER, m_onlyu);
	DDX_Radio(pDX, IDC_ALLJOBS, m_punpjobs);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COPdlg, CDialog)
	//{{AFX_MSG_MAP(COPdlg)
	ON_BN_CLICKED(IDC_SETALL, OnClickedSetall)
	ON_BN_CLICKED(IDC_CLEARALL, OnClickedClearall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void COPdlg::OnOK()
{   
	scanclass();
	if  (!m_classc)
		AfxMessageBox(IDP_ZEROCLASS, MB_OK|MB_ICONEXCLAMATION);
	else
		CDialog::OnOK();
}

BOOL COPdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_OPTR);
	plist	&appl = ((CSpqwApp *)AfxGetApp())->m_appptrlist;
	for  (unsigned  cnt = 0;  cnt < appl.number();  cnt++)
		uP->AddString(appl[cnt]->get_spp_ptr());
	uP->AddString("");
	uP->SelectString(-1, m_onlyprin);
	
	uP = (CComboBox *)GetDlgItem(IDC_OUSER);
	                  
	uP->AddString("");	            
	UUserList	unixusers;
	const  char  __far *nu;
	while  (nu = unixusers.nextuser())
		uP->AddString(nu);
	uP->SelectString(-1, m_onlyu);
	checkclass();
	if  (!m_mayoverride)
		for  (int idn = 0;  idn <= IDC_CC_P - IDC_CC_A; idn++)
			if  (!(m_maxclass & (1 << idn)))
				GetDlgItem(idn+IDC_CC_A)->EnableWindow(FALSE);	
	return TRUE;
}

void COPdlg::OnClickedSetall()
{
	if  (m_mayoverride)  {
		scanclass();
		if  (m_classc == m_maxclass)
			m_classc = 0xFFFF;
		else
			m_classc = m_maxclass;
	}
	else
		m_classc = m_maxclass;
	checkclass();
}

void COPdlg::OnClickedClearall()
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

void COPdlg::checkclass()
{
	for  (int idn = 0;  idn <= IDC_CC_P - IDC_CC_A; idn++)
		((CButton *)GetDlgItem(idn + IDC_CC_A))->SetCheck(m_classc & (1 << idn) ? 1: 0);
}

void COPdlg::scanclass()
{
	m_classc = 0;
	for  (int  idn = 0;  idn <= IDC_CC_P - IDC_CC_A;  idn++)
		if  (((CButton *)GetDlgItem(idn + IDC_CC_A))->GetCheck())
			m_classc |= (1 << idn);
}