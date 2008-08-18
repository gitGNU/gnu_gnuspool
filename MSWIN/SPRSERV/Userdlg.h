// userdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUserdlg dialog

class CUserdlg : public CDialog
{
// Construction
public:
	CUserdlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CUserdlg)
	enum { IDD = IDD_USERMAIL };
	CString	m_postuser;
	BOOL	m_mail;
	BOOL	m_write;
	BOOL	m_mattn;
	BOOL	m_wattn;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CUserdlg)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
