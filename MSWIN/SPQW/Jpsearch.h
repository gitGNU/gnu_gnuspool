// jpsearch.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJPSearch dialog

class CJPSearch : public CDialog
{
// Construction
public:
	CJPSearch(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CJPSearch)
	enum { IDD = IDD_SEARCHJP };
	CString	m_sstring;
	int		m_sforward;
	BOOL	m_sdevice;
	BOOL	m_sformtype;
	BOOL	m_sjtitle;
	BOOL	m_sprinter;
	BOOL	m_suser;
	BOOL	m_swraparound;
	//}}AFX_DATA
	enum { IDD_JOBSRCH, IDD_PRINSRCH } m_which;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJPSearch)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJPSearch)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
