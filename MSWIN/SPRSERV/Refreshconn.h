#if !defined(AFX_REFRESHCONN_H__1B1A62A0_2632_11D2_A14F_00C0DF50793E__INCLUDED_)
#define AFX_REFRESHCONN_H__1B1A62A0_2632_11D2_A14F_00C0DF50793E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Refreshconn.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRefreshconn dialog

class CRefreshconn : public CDialog
{
// Construction
public:
	CRefreshconn(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRefreshconn)
	enum { IDD = IDD_REFRESHCONN };
	CString	m_servname;
	int		m_action;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRefreshconn)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRefreshconn)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REFRESHCONN_H__1B1A62A0_2632_11D2_A14F_00C0DF50793E__INCLUDED_)
