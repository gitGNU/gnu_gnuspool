// hmsg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHmsg dialog

class CHmsg : public CDialog
{
// Construction
public:
	CHmsg(CWnd* pParent = NULL);	// standard constructor
	CHmsg(const char *, const char *);

// Dialog Data
	//{{AFX_DATA(CHmsg)
	enum { IDD = IDD_MESSG };
	CString	m_fromhost;
	//}}AFX_DATA
	CString	m_inmsg;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHmsg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHmsg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
