// dfdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDFDlg dialog

class CDFDlg : public CDialog
{
// Construction
public:
	CDFDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDFDlg)
	enum { IDD = IDD_DROPPEDFILE };
	BOOL	m_notyet;
	UINT	m_polltime;
	CString	m_filename;
	//}}AFX_DATA
	xtini	m_options;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDFDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDFDlg)
	afx_msg void OnClickedSetform();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
