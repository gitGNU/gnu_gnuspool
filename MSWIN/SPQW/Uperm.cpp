// uperm.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h" 
#include "jobdoc.h"
#include "ptrdoc.h"
#include "jdatadoc.h"
#include "spqw.h"
#include "uperm.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUperm dialog


CUperm::CUperm(CWnd* pParent /*=NULL*/)
	: CDialog(CUperm::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUperm)
	//}}AFX_DATA_INIT
}

void CUperm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUperm)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CUperm, CDialog)
	//{{AFX_MSG_MAP(CUperm)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CUperm message handlers

BOOL CUperm::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CSpqwApp	&ma = *((CSpqwApp *)AfxGetApp());
	spdet		&mypriv = ma.m_mypriv;
	
	SetDlgItemText(IDC_USER, ma.m_username);
	SetDlgItemText(IDC_FORMTYPE, mypriv.spu_form);
	SetDlgItemText(IDC_DEFPRINTER, mypriv.spu_ptr);
	SetDlgItemText(IDC_FORMALLOW, mypriv.spu_formallow);
	SetDlgItemText(IDC_PTRALLOW, mypriv.spu_ptrallow);
	
	char	nbuf[10];
	(void) sprintf(nbuf, "%d", int(mypriv.spu_minp));
	SetDlgItemText(IDC_MINP, nbuf);
	(void) sprintf(nbuf, "%d", int(mypriv.spu_defp));
	SetDlgItemText(IDC_DEFP, nbuf);
	(void) sprintf(nbuf, "%d", int(mypriv.spu_maxp));
	SetDlgItemText(IDC_MAXP, nbuf);
	(void) sprintf(nbuf, "%d", int(mypriv.spu_cps));
	SetDlgItemText(IDC_COPIES, nbuf);
	
	((CButton *)GetDlgItem(IDC_ADMIN))->SetCheck(mypriv.ispriv(PV_ADMIN));
	((CButton *)GetDlgItem(IDC_SSTOP))->SetCheck(mypriv.ispriv(PV_SSTOP));
	((CButton *)GetDlgItem(IDC_FORMS))->SetCheck(mypriv.ispriv(PV_FORMS));
	((CButton *)GetDlgItem(IDC_CPRIO))->SetCheck(mypriv.ispriv(PV_CPRIO));
	((CButton *)GetDlgItem(IDC_OTHERJ))->SetCheck(mypriv.ispriv(PV_OTHERJ));
	((CButton *)GetDlgItem(IDC_PRINQ))->SetCheck(mypriv.ispriv(PV_PRINQ));
	((CButton *)GetDlgItem(IDC_HALTGO))->SetCheck(mypriv.ispriv(PV_HALTGO));
	((CButton *)GetDlgItem(IDC_ANYPRIO))->SetCheck(mypriv.ispriv(PV_ANYPRIO));
	((CButton *)GetDlgItem(IDC_CDEFLT))->SetCheck(mypriv.ispriv(PV_CDEFLT));
	((CButton *)GetDlgItem(IDC_ADDDEL))->SetCheck(mypriv.ispriv(PV_ADDDEL));
	((CButton *)GetDlgItem(IDC_COVER))->SetCheck(mypriv.ispriv(PV_COVER));
	((CButton *)GetDlgItem(IDC_UNQUEUE))->SetCheck(mypriv.ispriv(PV_UNQUEUE));
	((CButton *)GetDlgItem(IDC_VOTHERJ))->SetCheck(mypriv.ispriv(PV_VOTHERJ));
	((CButton *)GetDlgItem(IDC_REMOTEJ))->SetCheck(mypriv.ispriv(PV_REMOTEJ));
	((CButton *)GetDlgItem(IDC_REMOTEP))->SetCheck(mypriv.ispriv(PV_REMOTEP));
	((CButton *)GetDlgItem(IDC_OTHERP))->SetCheck(mypriv.ispriv(PV_OTHERP));
	((CButton *)GetDlgItem(IDC_ACCESSOK))->SetCheck(mypriv.ispriv(PV_ACCESSOK));
	((CButton *)GetDlgItem(IDC_FREEZEOK))->SetCheck(mypriv.ispriv(PV_FREEZEOK));
	
	for  (int idn = 0;  idn <= IDC_CC_P - IDC_CC_A; idn++)
		((CButton *)GetDlgItem(idn + IDC_CC_A))->SetCheck(mypriv.spu_class & (1L << idn) ? 1: 0);
	for  (idn = 0;  idn <= IDC_CC_P2 - IDC_CC_A2; idn++)
		((CButton *)GetDlgItem(idn + IDC_CC_A2))->SetCheck(mypriv.spu_class & (1L << (idn+16)) ? 1: 0);

	return TRUE;
}

const DWORD a118HelpIDs[] = {
	IDC_USER,	IDH_118_173,	// User permissions from server 
	IDC_FORMTYPE,	IDH_118_163,	// User permissions from server 
	IDC_MINP,	IDH_118_277,	// User permissions from server 
	IDC_DEFP,	IDH_118_278,	// User permissions from server 
	IDC_MAXP,	IDH_118_279,	// User permissions from server 
	IDC_COPIES,	IDH_118_167,	// User permissions from server 
	IDC_FORMALLOW,	IDH_118_280,	// User permissions from server 
	IDC_DEFPRINTER,	IDH_118_281,	// User permissions from server 
	IDC_PTRALLOW,	IDH_118_282,	// User permissions from server 
	IDC_REMOTEJ,	IDH_118_283,	// User permissions from server Access remote jobs
	IDC_FORMS,	IDH_118_284,	// User permissions from server Select alternative forms
	IDC_CDEFLT,	IDH_118_285,	// User permissions from server Change own default priority
	IDC_CPRIO,	IDH_118_286,	// User permissions from server Change priority on queue
	IDC_ANYPRIO,	IDH_118_287,	// User permissions from server Set any priority
	IDC_UNQUEUE,	IDH_118_288,	// User permissions from server Unqueue
	IDC_OTHERJ,	IDH_118_289,	// User permissions from server Change other users' jobs
	IDC_VOTHERJ,	IDH_118_290,	// User permissions from server View other users' jobs
	IDC_SSTOP,	IDH_118_291,	// User permissions from server Stop scheduler
	IDC_COVER,	IDH_118_292,	// User permissions from server Override class
	IDC_ADMIN,	IDH_118_293,	// User permissions from server System administrator
	IDC_PRINQ,	IDH_118_294,	// User permissions from server Select printer list
	IDC_REMOTEP,	IDH_118_295,	// User permissions from server Access remote printers
	IDC_HALTGO,	IDH_118_296,	// User permissions from server Stop/start printers
	IDC_ADDDEL,	IDH_118_297,	// User permissions from server Add and delete printers
	IDC_OTHERP,	IDH_118_298,	// User permissions from server Unrestricted printers
	IDC_ACCESSOK,	IDH_118_299,	// User permissions from server May access secondary fields
	IDC_FREEZEOK,	IDH_118_300,	// User permissions from server May save defaults
	IDC_CC_A,	IDH_118_192,	// User permissions from server A
	IDC_CC_B,	IDH_118_193,	// User permissions from server B
	IDC_CC_C,	IDH_118_194,	// User permissions from server C
	IDC_CC_D,	IDH_118_195,	// User permissions from server D
	IDC_CC_E,	IDH_118_196,	// User permissions from server E
	IDC_CC_F,	IDH_118_197,	// User permissions from server F
	IDC_CC_G,	IDH_118_198,	// User permissions from server G
	IDC_CC_H,	IDH_118_199,	// User permissions from server H
	IDC_CC_I,	IDH_118_200,	// User permissions from server I
	IDC_CC_J,	IDH_118_201,	// User permissions from server J
	IDC_CC_K,	IDH_118_202,	// User permissions from server K
	IDC_CC_L,	IDH_118_203,	// User permissions from server L
	IDC_CC_M,	IDH_118_204,	// User permissions from server M
	IDC_CC_N,	IDH_118_205,	// User permissions from server N
	IDC_CC_O,	IDH_118_206,	// User permissions from server O
	IDC_CC_P,	IDH_118_207,	// User permissions from server P
	IDC_CC_A2,	IDH_118_208,	// User permissions from server a
	IDC_CC_B2,	IDH_118_209,	// User permissions from server b
	IDC_CC_C2,	IDH_118_210,	// User permissions from server c
	IDC_CC_D2,	IDH_118_211,	// User permissions from server d
	IDC_CC_E2,	IDH_118_212,	// User permissions from server e
	IDC_CC_F2,	IDH_118_213,	// User permissions from server f
	IDC_CC_G2,	IDH_118_214,	// User permissions from server g
	IDC_CC_H2,	IDH_118_215,	// User permissions from server h
	IDC_CC_I2,	IDH_118_216,	// User permissions from server i
	IDC_CC_J2,	IDH_118_217,	// User permissions from server j
	IDC_CC_K2,	IDH_118_218,	// User permissions from server k
	IDC_CC_L2,	IDH_118_219,	// User permissions from server l
	IDC_CC_M2,	IDH_118_220,	// User permissions from server m
	IDC_CC_N2,	IDH_118_221,	// User permissions from server n
	IDC_CC_O2,	IDH_118_222,	// User permissions from server o
	IDC_CC_P2,	IDH_118_223,	// User permissions from server p
	0, 0
};

BOOL CUperm::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a118HelpIDs[cnt] != 0;  cnt += 2)
		if  (a118HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a118HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
