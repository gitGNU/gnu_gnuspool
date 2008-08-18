// propts.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPropts dialog

class CPropts : public CDialog
{
// Construction
public:
	CPropts(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CPropts)
	enum { IDD = IDD_PROPTS };
	BOOL	m_interpolate;
	BOOL	m_textmode;
	BOOL	m_verbose;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropts)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPropts)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
