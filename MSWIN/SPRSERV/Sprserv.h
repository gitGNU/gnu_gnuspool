// sprserv.h : main header file for the SPRSERV application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CSprservApp:
// See sprserv.cpp for the implementation of this class
//

class CSprservApp : public CWinApp
{
public:
	int		m_defdrive;
	char	m_defdir[_MAX_DIR];
	xtini	m_options;
	spdet	m_mypriv;
	CString	m_username, m_winuser, m_winmach;
	BOOL	m_shutdown;
	BOOL	m_dontask;

protected:
	BOOL	m_noulist;

public:
	CSprservApp();
	void	load_options();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSprservApp)
	public:
	virtual BOOL InitInstance();
	virtual	int	ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	BOOL	noulist() { return m_noulist; }

	//{{AFX_MSG(CSprservApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
