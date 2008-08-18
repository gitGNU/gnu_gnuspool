// opdlg22.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COPdlg22 dialog

class COPdlg22 : public CDialog
{
// Construction
public:
	COPdlg22(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(COPdlg22)
	enum { IDD = IDD_ORESTRICT22 };
	int		m_punpjobs;
	CString	m_onlyprin;
	CString	m_onlyu;
	int		m_jinclude;
	CString	m_onlytitle;
	//}}AFX_DATA
	unsigned long m_classc;
	unsigned long m_maxclass;
	BOOL	m_mayoverride;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COPdlg22)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void	checkclass();
	void	scanclass();
	BOOL	checkpattern(const int);

protected:

	// Generated message map functions
	//{{AFX_MSG(COPdlg22)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSetall();
	afx_msg void OnClearall();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
