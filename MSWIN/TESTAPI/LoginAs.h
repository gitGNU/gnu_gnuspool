#if !defined(AFX_LOGINAS_H__280A9725_67D6_11D1_BA1D_00C0DF501B60__INCLUDED_)
#define AFX_LOGINAS_H__280A9725_67D6_11D1_BA1D_00C0DF501B60__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LoginAs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoginAs dialog

class CLoginAs : public CDialog
{
// Construction
public:
	CLoginAs(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLoginAs)
	enum { IDD = IDD_LOGIN };
	CString	m_username;
	CString	m_machine;
	CString	m_password;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginAs)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLoginAs)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGINAS_H__280A9725_67D6_11D1_BA1D_00C0DF501B60__INCLUDED_)
