// secdlg22.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSecdlg22 dialog

class CSecdlg22 : public CDialog
{
// Construction
public:
	CSecdlg22(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CSecdlg22)
	enum { IDD = IDD_SECURITY22 };
	BOOL	m_localonly;
	//}}AFX_DATA
	unsigned  long  m_classc;
	unsigned  long	m_maxclass;
	BOOL	  m_mayoverride;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSecdlg22)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void	scanclass();
	void	checkclass();

protected:

	// Generated message map functions
	//{{AFX_MSG(CSecdlg22)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSetall();
	afx_msg void OnClearall();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
