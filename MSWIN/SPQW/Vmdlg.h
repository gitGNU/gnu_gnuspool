// vmdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVMdlg dialog

class CVMdlg : public CDialog
{
// Construction
public:
	CVMdlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVMdlg)
	enum { IDD = IDD_VIEWMENU };
	BOOL	m_isstart;
	BOOL	m_isend;
	BOOL	m_ishat;
	//}}AFX_DATA
	BOOL	m_enabstart, m_enabend, m_enabhat;	// Enable page changes

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVMdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVMdlg)
	afx_msg void OnClickedQuitdata();
	afx_msg void OnClickedSetend();
	afx_msg void OnClickedSethat();
	afx_msg void OnClickedSetstart();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
