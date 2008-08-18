// hmsg.cpp : implementation file
//

#include "stdafx.h"
#include "files.h"
#include "pages.h"
#include "xtini.h"
#include "monfile.h"
#include "sprserv.h"
#include "hmsg.h"
#include "Sprserv.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHmsg dialog


CHmsg::CHmsg(CWnd* pParent /*=NULL*/)
	: CDialog(CHmsg::IDD, pParent)
{
	Create(IDD);
	//{{AFX_DATA_INIT(CHmsg)
	m_fromhost = "";
	//}}AFX_DATA_INIT
}

CHmsg::CHmsg(const char *fromhost, const char *msg)
	: CDialog(CHmsg::IDD, NULL)
{
	m_fromhost = fromhost;
	m_inmsg = msg;
	Create(IDD);
}

void CHmsg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHmsg)
	DDX_Text(pDX, IDC_FROMHOST, m_fromhost);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CHmsg, CDialog)
	//{{AFX_MSG_MAP(CHmsg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHmsg message handlers

BOOL CHmsg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CListBox	*lb = (CListBox *) GetDlgItem(IDC_MESSAGE);
	CString	str = m_inmsg;
	int	 np;
	while  ((np = str.Find('\n')) >= 0)  {
		CString	bit = str.Left(np);
		int	lng = bit.GetLength() - 1;
		if  (lng >= 0  &&  bit[lng] == '\r')
			bit = bit.Left(lng);
		lb->AddString(bit);
		str = str.Right(str.GetLength() - np - 1);
	}
	if  (str.GetLength() > 0)	
			lb->AddString(str);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CHmsg::OnOK()
{
	DestroyWindow();
}

void	CHmsg::PostNcDestroy()
{
	delete  this;
}

const DWORD a103HelpIDs[] = {
	IDC_FROMHOST,	IDH_103_167,	// Xi-Text Job Message from host system 
	IDC_MESSAGE,	IDH_103_168,	// Xi-Text Job Message from host system 
	0, 0
};

BOOL CHmsg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a103HelpIDs[cnt] != 0;  cnt += 2)
		if  (a103HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a103HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
