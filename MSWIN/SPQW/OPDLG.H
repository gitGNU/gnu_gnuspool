// opdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COPdlg dialog

class COPdlg : public CDialog
{
// Construction
public:
	COPdlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(COPdlg)
	enum { IDD = IDD_ORESTRICT };
	CString	m_onlyprin;
	CString	m_onlyu;
	int		m_punpjobs;
	//}}AFX_DATA
	unsigned m_classc;
	unsigned m_maxclass;
	BOOL	m_mayoverride;

// Implementation         
private:
	void	checkclass();
	void	scanclass();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(COPdlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedSetall();
	afx_msg void OnClickedClearall();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
