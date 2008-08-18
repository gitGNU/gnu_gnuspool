// uperm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUperm dialog

class CUperm : public CDialog
{
// Construction
public:
	CUperm(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CUperm)
	enum { IDD = IDD_USERPERM };
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUperm)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUperm)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
