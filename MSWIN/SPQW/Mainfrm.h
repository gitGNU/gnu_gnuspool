// mainfrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:
	enum PageMarkers { Start, End, Halted_at };
	CMultiDocTemplate	*m_dtjob;
	CMultiDocTemplate	*m_dtptr;
	CMultiDocTemplate	*m_dtjdata;
	restrictdoc			m_restrictlist;
	BOOL	m_dispenab, m_dispstart, m_dispend, m_disphat;
	CString	m_jlastsearch;
	CString	m_plastsearch;
	unsigned		m_pollfreq;

// Operations
public:
	void SetPageMarker(const PageMarkers, const BOOL);
	void EnableMarkers(const BOOL on = FALSE) { m_dispenab = on; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual	void AssertValid() const;
	virtual	void Dump(CDumpContext& dc) const;
#endif


protected:	// control bar embedded members
	CStatusBar	m_wndStatusBar;
	CToolBar	m_wndToolBar;
                     
public:                   
	void InitWins();
	BOOL DoRstrDlg(restrictdoc &);
	void OnNewJDWin(spq *);

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowNewjobwindow();
	afx_msg void OnWindowNewprinterwindow();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnFileProgramoptions();
	afx_msg LRESULT OnNMArrived(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMNewconn(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMProbercv(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFileSavetofile();
	afx_msg void OnUpdateFileSavetofile(CCmdUI* pCmdUI);
	afx_msg void OnFileDisplayoptions();
	afx_msg void OnWindowSetjoblistformat();
	afx_msg void OnWindowSetprinterlistformat();
	afx_msg void OnFileNetworkstats();
	afx_msg void OnFileUserpermissions();
	afx_msg void OnUpdateStartp(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEndp(CCmdUI* pCmdUI);
	afx_msg void OnUpdateHatp(CCmdUI* pCmdUI);
	afx_msg void OnFilePcolourAwoper();
	afx_msg void OnFilePcolourError();
	afx_msg void OnFilePcolourHalted();
	afx_msg void OnFilePcolourIdle();
	afx_msg void OnFilePcolourOffline();
	afx_msg void OnFilePcolourPrinting();
	afx_msg void OnFilePcolourShutd();
	afx_msg void OnFilePcolourStartup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void RunColourDlg(const unsigned n);
};

/////////////////////////////////////////////////////////////////////////////
