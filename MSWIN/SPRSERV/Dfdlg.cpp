// dfdlg.cpp : implementation file
//

#include "stdafx.h"
#include "pages.h"
#include "xtini.h"
#include "sprserv.h"
#include "dfdlg.h"             
#include "formdlg.h"
#include <limits.h>
#include "Sprserv.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDFDlg dialog

CDFDlg::CDFDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDFDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDFDlg)
	m_notyet = FALSE;
	m_polltime = 0;
	m_filename = "";
	//}}AFX_DATA_INIT
}

void CDFDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDFDlg)
	DDX_Check(pDX, IDC_NOTYET, m_notyet);
	DDX_Text(pDX, IDC_POLLTIME, m_polltime);
	DDV_MinMaxUInt(pDX, m_polltime, 1, 32767);
	DDX_Text(pDX, IDD_MONFILE, m_filename);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDFDlg, CDialog)
	//{{AFX_MSG_MAP(CDFDlg)
	ON_BN_CLICKED(IDC_SETFORM, OnClickedSetform)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFDlg message handlers

BOOL CDFDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}

void CDFDlg::OnClickedSetform()
{
	spq		&pars = m_options.qparams;
	CFormdlg	fdlg;
	fdlg.m_formtype = pars.spq_form;
	fdlg.m_printer = pars.spq_ptr;
	fdlg.m_header = pars.spq_file;
	fdlg.m_copies = pars.spq_cps;
	fdlg.m_priority = pars.spq_pri;
	fdlg.m_supph = pars.spq_jflags & SPQ_NOH? TRUE: FALSE;
	spdet	&mypriv = ((CSprservApp *) AfxGetApp())->m_mypriv;
	fdlg.m_formok = mypriv.ispriv(PV_FORMS)? TRUE: FALSE;
	fdlg.m_ptrok = mypriv.ispriv(PV_OTHERP)? TRUE: FALSE;
	fdlg.m_minp = int(mypriv.spu_minp);
	fdlg.m_maxp = int(mypriv.spu_maxp);
	fdlg.m_maxcps = int(mypriv.ispriv(PV_ANYPRIO)? 255: mypriv.spu_cps);
	fdlg.m_allowform = mypriv.spu_formallow;
	fdlg.m_allowptr = mypriv.spu_ptrallow;
	if  (fdlg.DoModal() == IDOK)  {
		strcpy(pars.spq_form, fdlg.m_formtype);
		strcpy(pars.spq_ptr, fdlg.m_printer);
		strcpy(pars.spq_file, fdlg.m_header);
		pars.spq_cps = fdlg.m_copies;
		pars.spq_pri = fdlg.m_priority;
		if  (fdlg.m_supph)
			pars.spq_jflags |= SPQ_NOH;
		else
			pars.spq_jflags &= ~SPQ_NOH;
	}	
}

const DWORD a101HelpIDs[] = {
	IDC_MONFILE,	IDH_101_150,	// Dropped file 
	IDC_POLLTIME,	IDH_101_152,	// Dropped file 
	IDC_SCR_POLLTIME,	IDH_101_153,	// Dropped file Spin1
	IDC_NOTYET,	IDH_101_154,	// Dropped file Not yet
	IDC_SETFORM,	IDH_101_151,	// Dropped file Form type
	0, 0
};

BOOL CDFDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a101HelpIDs[cnt] != 0;  cnt += 2)
		if  (a101HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a101HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
