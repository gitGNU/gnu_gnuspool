// formdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFormdlg dialog

class CFormdlg : public CDialog
{
// Construction
public:
	CFormdlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CFormdlg)
	enum { IDD = IDD_FORMDLG };
	UINT	m_copies;
	CString	m_header;
	BOOL	m_supph;
	CString	m_formtype;
	CString	m_printer;
	UINT	m_priority;
	//}}AFX_DATA
	BOOL	m_formok;				// may change form
	BOOL	m_ptrok;				// may change ptr
	CString	m_allowform;
	CString	m_allowptr;
	int		m_minp, m_maxp, m_maxcps;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFormdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFormdlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
