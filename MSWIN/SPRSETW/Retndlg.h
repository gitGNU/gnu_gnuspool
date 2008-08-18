// retndlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRetndlg dialog

class CRetndlg : public CDialog
{
// Construction
public:
	CRetndlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CRetndlg)
	enum { IDD = IDD_RETAIN };
	BOOL	m_retain;
	UINT	m_dip;
	UINT	m_dinp;
	BOOL	m_hold;
	//}}AFX_DATA
	long	m_holdtime;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRetndlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void	Enablehtime(const BOOL = TRUE);
public:
	void	fillindelay();
	
protected:
	// Generated message map functions
	//{{AFX_MSG(CRetndlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickedHold();
	afx_msg void OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrWday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMon(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
