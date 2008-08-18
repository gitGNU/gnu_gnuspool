// LoginAs.cpp : implementation file
//

#include "stdafx.h"
#include "testapi.h"
#include "LoginAs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginAs dialog


CLoginAs::CLoginAs(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginAs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginAs)
	m_username = _T("");
	m_machine = _T("");
	m_password = _T("");
	//}}AFX_DATA_INIT
}


void CLoginAs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginAs)
	DDX_Text(pDX, IDC_USERNAME, m_username);
	DDX_Text(pDX, IDC_MACHINE, m_machine);
	DDX_Text(pDX, IDC_PASSWD, m_password);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginAs, CDialog)
	//{{AFX_MSG_MAP(CLoginAs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginAs message handlers
