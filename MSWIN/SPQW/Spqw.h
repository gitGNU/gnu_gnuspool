// spqw.h : main header file for the SPQW application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "pages.h"
#include "spuser.h"
#include "xtini.h"

/////////////////////////////////////////////////////////////////////////////
// CSpqwApp:
// See spqw.cpp for the implementation of this class
//

class CSpqwApp : public CWinApp
{
private:
	BOOL	m_dirty;
	
protected:
	BOOL	m_noulist;

public:
	CSpqwApp();
	BOOL	isdirty() { return m_dirty; }
	void	dirty()	{ m_dirty = TRUE;	}
	void	clean()	{ m_dirty = FALSE;	}
	void	load_options();
	void	save_options();
	xtini	m_options;
	joblist	m_appjoblist;
	plist	m_appptrlist;
	spdet	m_mypriv;
	CString	m_username, m_winuser, m_winmach;
	CString	m_jfmt, m_pfmt;
	CPtrcolours	m_appcolours;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpqwApp)
	public:
	virtual BOOL InitInstance();
	virtual int	 ExitInstance();
	virtual BOOL OnIdle(LONG);
	//}}AFX_VIRTUAL

// Implementation

	BOOL	noulist() { return m_noulist; }
	//{{AFX_MSG(CSpqwApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
	
inline	joblist	&Jobs()
{
	return  ((CSpqwApp *)AfxGetApp())->m_appjoblist;
}

inline	plist	&Printers()
{
	return  ((CSpqwApp *)AfxGetApp())->m_appptrlist;
}			
