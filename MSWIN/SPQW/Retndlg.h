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
	UINT	m_dinp;
	UINT	m_dip;
	BOOL	m_hold;
	BOOL	m_printed;
	BOOL	m_retain;
	//}}AFX_DATA
	time_t	m_inittime;
	time_t	m_holdtime;

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
	void	fillintime();
	
protected:

	// Generated message map functions
	//{{AFX_MSG(CRetndlg)
	afx_msg void OnClickedHold();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrWday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMon(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
