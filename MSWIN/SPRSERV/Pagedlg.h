// pagedlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPagedlg dialog

class CPagedlg : public CDialog
{
// Construction
public:
	CPagedlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CPagedlg)
	enum { IDD = IDD_PAGE };
	UINT	m_delimnum;
	BOOL	m_swapoe;
	CString	m_ppflags;
	//}}AFX_DATA
	unsigned  short  m_jflags;
	unsigned  long	m_startp, m_endp;
	unsigned  m_deliml;
	char	  *m_delimiter;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPagedlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPagedlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickedAllp();
	afx_msg void OnClickedEvenp();
	afx_msg void OnClickedOddp();
	afx_msg void OnDeltaposScrEndp(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()         

private:
	void	mkdelimstring(CString &);
	BOOL	procdelimstring();
};
