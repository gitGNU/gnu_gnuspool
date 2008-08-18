// secdlg22.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "secdlg22.h"
#include "Sprsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSecdlg22 dialog


CSecdlg22::CSecdlg22(CWnd* pParent /*=NULL*/)
	: CDialog(CSecdlg22::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSecdlg22)
	m_localonly = FALSE;
	//}}AFX_DATA_INIT
	m_classc = 0xFFFFFFFF;
}

void CSecdlg22::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSecdlg22)
	DDX_Check(pDX, IDC_LOCALONLY, m_localonly);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSecdlg22, CDialog)
	//{{AFX_MSG_MAP(CSecdlg22)
	ON_BN_CLICKED(IDC_SETALL, OnSetall)
	ON_BN_CLICKED(IDC_CLEARALL, OnClearall)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSecdlg22 message handlers

BOOL CSecdlg22::OnInitDialog()
{
	CDialog::OnInitDialog();
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

void CSecdlg22::OnOK()
{
	scanclass();
	if  (!m_classc)
		AfxMessageBox(IDP_ZEROCLASS, MB_OK|MB_ICONEXCLAMATION);
	else
		CDialog::OnOK();
}

void CSecdlg22::OnSetall()
{
	if  (m_mayoverride)  {
		scanclass();
		if  (m_classc == m_maxclass)
			m_classc = 0xFFFFFFFFL;
		else
			m_classc = m_maxclass;
	}
	else
		m_classc = m_maxclass;
	checkclass();
}

void CSecdlg22::OnClearall()
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

void CSecdlg22::checkclass()
{
	for  (int idn = 0;  idn <= IDC_CC_P - IDC_CC_A; idn++)
		((CButton *)GetDlgItem(idn + IDC_CC_A))->SetCheck(m_classc & (1L << idn) ? 1: 0);
	for  (idn = 0;  idn <= IDC_CC_P2 - IDC_CC_A2; idn++)
		((CButton *)GetDlgItem(idn + IDC_CC_A2))->SetCheck(m_classc & (1L << (idn+16)) ? 1: 0);
}

void CSecdlg22::scanclass()
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
	IDC_SETALL,	IDH_109_221,	// Security options Set All
	IDC_CLEARALL,	IDH_109_222,	// Security options Clear all
	IDC_CC_A,	IDH_109_223,	// Security options A
	IDC_CC_B,	IDH_109_224,	// Security options B
	IDC_CC_C,	IDH_109_225,	// Security options C
	IDC_CC_D,	IDH_109_226,	// Security options D
	IDC_CC_E,	IDH_109_227,	// Security options E
	IDC_CC_F,	IDH_109_228,	// Security options F
	IDC_CC_G,	IDH_109_229,	// Security options G
	IDC_CC_H,	IDH_109_230,	// Security options H
	IDC_CC_I,	IDH_109_231,	// Security options I
	IDC_CC_J,	IDH_109_232,	// Security options J
	IDC_CC_K,	IDH_109_233,	// Security options K
	IDC_CC_L,	IDH_109_234,	// Security options L
	IDC_CC_M,	IDH_109_235,	// Security options M
	IDC_CC_N,	IDH_109_236,	// Security options N
	IDC_CC_O,	IDH_109_237,	// Security options O
	IDC_CC_P,	IDH_109_238,	// Security options P
	IDC_CC_A2,	IDH_109_239,	// Security options a
	IDC_CC_B2,	IDH_109_240,	// Security options b
	IDC_CC_C2,	IDH_109_241,	// Security options c
	IDC_CC_D2,	IDH_109_242,	// Security options d
	IDC_CC_E2,	IDH_109_243,	// Security options e
	IDC_CC_F2,	IDH_109_244,	// Security options f
	IDC_CC_G2,	IDH_109_245,	// Security options g
	IDC_CC_H2,	IDH_109_246,	// Security options h
	IDC_CC_I2,	IDH_109_247,	// Security options i
	IDC_CC_J2,	IDH_109_248,	// Security options j
	IDC_CC_K2,	IDH_109_249,	// Security options k
	IDC_CC_L2,	IDH_109_250,	// Security options l
	IDC_CC_M2,	IDH_109_251,	// Security options m
	IDC_CC_N2,	IDH_109_252,	// Security options n
	IDC_CC_O2,	IDH_109_253,	// Security options o
	IDC_CC_P2,	IDH_109_254,	// Security options p
	IDC_LOCALONLY,	IDH_109_255,	// Security options Local only
	0, 0
};

BOOL CSecdlg22::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a109HelpIDs[cnt] != 0;  cnt += 2)
		if  (a109HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a109HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
