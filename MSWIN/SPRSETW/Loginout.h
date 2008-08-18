#if !defined(AFX_LOGINOUT_H__F36E7741_6643_11D1_BA1D_00C0DF501B60__INCLUDED_)
#define AFX_LOGINOUT_H__F36E7741_6643_11D1_BA1D_00C0DF501B60__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Loginout.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoginout dialog

class CLoginout : public CDialog
{
// Construction
public:
	CLoginout(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLoginout)
	enum { IDD = IDD_LOGINOUT };
	CString	m_winuser;
	CString	m_unixuser;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginout)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLoginout)
	afx_msg void OnLogin();
	afx_msg void OnLogout();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGINOUT_H__F36E7741_6643_11D1_BA1D_00C0DF501B60__INCLUDED_)
