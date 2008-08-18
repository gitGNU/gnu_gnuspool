// sprsetw.h : main header file for the SPRSETW application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
/////////////////////////////////////////////////////////////////////////////
// CSprsetwApp:
// See sprsetw.cpp for the implementation of this class
//

class CSprsetwApp : public CWinApp
{
protected:
	BOOL	m_noulist;

public:
	CSprsetwApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSprsetwApp)
	public:
	virtual BOOL InitInstance();
    virtual int  ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	xtini	m_options;
	spdet	m_mypriv;
	CString	m_winmach;
	CString	m_winuser;
	CString	m_username;
	BOOL	m_needsave;
	BOOL	m_isvalid;
	void	load_options();
	void	save_options();
	void	prune_class();
	BOOL	noulist() { return m_noulist; }
	BOOL	appvalid() { return m_isvalid; }

	//{{AFX_MSG(CSprsetwApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
