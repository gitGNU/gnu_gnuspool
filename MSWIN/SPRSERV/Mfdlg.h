// mfdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMFDlg dialog

class CMFDlg : public CDialog
{
// Construction
public:
	CMFDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMFDlg)
	enum { IDD = IDD_MONFILE };
	UINT	m_polltime;
	BOOL	m_notyet;
	CString	m_filename;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMFDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMFDlg)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
