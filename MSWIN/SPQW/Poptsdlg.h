// poptsdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPOptsdlg dialog

class CPOptsdlg : public CDialog
{
// Construction
public:
	CPOptsdlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CPOptsdlg)
	enum { IDD = IDD_PROGOPTS };
	int		m_confabort;
	int		m_probewarn;
	UINT	m_polltime;
	int		m_sjext;
	BOOL	m_abshold;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPOptsdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPOptsdlg)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
