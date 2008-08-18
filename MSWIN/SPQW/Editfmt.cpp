// editfmt.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "spqw.h"
#include "editfmt.h"
#include "Spqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditfmt dialog


CEditfmt::CEditfmt(CWnd* pParent /*=NULL*/)
	: CDialog(CEditfmt::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditfmt)
	m_width = 0;
	m_tableft = FALSE;
	m_skipright = FALSE;
	//}}AFX_DATA_INIT
	m_uppercode = 0;
	m_lowercode = 0;
	m_existing = ' ';
}

void CEditfmt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditfmt)
	DDX_Text(pDX, IDC_WIDTH, m_width);
	DDV_MinMaxUInt(pDX, m_width, 1, 100);
	DDX_Check(pDX, IDC_TABLEFT, m_tableft);
	DDX_Check(pDX, IDC_SKIPRIGHT, m_skipright);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditfmt, CDialog)
	//{{AFX_MSG_MAP(CEditfmt)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEditfmt message handlers

BOOL CEditfmt::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CComboBox	*lb = (CComboBox *) GetDlgItem(IDC_FMTCODE);
	if  (m_uppercode != 0)  {
		CString	str;
		for  (UINT  cnt = 0;  cnt < 26;  cnt++)
			if  (str.LoadString(m_uppercode + cnt))  {
				char  whichch = char(cnt + 'A');
				int	indx;
				lb->SetItemData(indx = lb->AddString((const char *) str), DWORD(whichch));
				if  (whichch == m_existing)
					lb->SetCurSel(indx); 
			}
	}
	if  (m_lowercode != 0)  {
		CString	str;
		for  (UINT  cnt = 0;  cnt < 26;  cnt++)
			if  (str.LoadString(m_lowercode + cnt))  {
				char  whichch = char(cnt + 'a');
				int	indx;
				lb->SetItemData(indx = lb->AddString((const char *) str), DWORD(whichch));
				if  (whichch == m_existing)
					lb->SetCurSel(indx); 
			}
	}
	return TRUE;
}

void CEditfmt::OnOK()
{
	CComboBox	*lb = (CComboBox *) GetDlgItem(IDC_FMTCODE);
	int	 which = lb->GetCurSel();
	if  (which < 0)  {
		AfxMessageBox(IDP_NOCODESEL, MB_OK|MB_ICONEXCLAMATION);
		lb->SetFocus();
		return;
	}
	m_existing = char(lb->GetItemData(which));
	CDialog::OnOK();
}

const DWORD a102HelpIDs[] = {
	IDC_FMTCODE,	IDH_102_150,	// Edit Format Code 
	IDC_WIDTH,	IDH_102_151,	// Edit Format Code 
	IDC_SCR_WIDTH,	IDH_102_152,	// Edit Format Code Spin1
	IDC_TABLEFT,	IDH_102_153,	// Edit Format Code Tab format left if too long
	IDC_SKIPRIGHT,	IDH_102_154,	// Edit Format Code Skip fields to right if too long
	0, 0
};

BOOL CEditfmt::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a102HelpIDs[cnt] != 0;  cnt += 2)
		if  (a102HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a102HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
