// sizelim.cpp : implementation file
//

#include "stdafx.h"
#ifdef	SPRSETW
#include "sprsetw.h"
#include "Sprsetw.hpp"
#endif
#ifdef	SPRSERV
#include "pages.h"
#include "xtini.h"
#include "sprserv.h"
#include "Sprserv.hpp"
#endif
#include "sizelim.h"
#include <limits.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSizelim dialog


CSizelim::CSizelim(CWnd* pParent /*=NULL*/)
	: CDialog(CSizelim::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSizelim)
	m_limittype = -1;
	m_errlimit = FALSE;
	m_limit = 0;
	//}}AFX_DATA_INIT
}

void CSizelim::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSizelim)
	DDX_Radio(pDX, IDC_LIMITK, m_limittype);
	DDX_Check(pDX, IDC_ERRLIMIT, m_errlimit);
	DDX_Text(pDX, IDC_LIMIT, m_limit);
	DDV_MinMaxUInt(pDX, m_limit, 0, 30000);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSizelim, CDialog)
	//{{AFX_MSG_MAP(CSizelim)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSizelim message handlers

BOOL CSizelim::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}

const DWORD a110HelpIDs[] = {
	IDC_LIMIT,	IDH_110_256,	// Job size limit 
	IDC_SCR_LIMIT,	IDH_110_257,	// Job size limit Spin1
	IDC_LIMITK,	IDH_110_258,	// Job size limit Size in kilobytes
	IDC_LIMITP,	IDH_110_259,	// Job size limit Size in pages
	IDC_ERRLIMIT,	IDH_110_260,	// Job size limit Button
	0, 0
};

BOOL CSizelim::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a110HelpIDs[cnt] != 0;  cnt += 2)
		if  (a110HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a110HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
