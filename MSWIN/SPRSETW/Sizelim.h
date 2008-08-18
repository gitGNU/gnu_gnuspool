// sizelim.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSizelim dialog

class CSizelim : public CDialog
{
// Construction
public:
	CSizelim(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CSizelim)
	enum { IDD = IDD_SIZELIM };
	int		m_limittype;
	BOOL	m_errlimit;
	UINT	m_limit;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSizelim)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSizelim)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
