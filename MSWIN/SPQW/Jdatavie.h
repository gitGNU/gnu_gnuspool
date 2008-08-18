// jdatavie.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJdataview view

class CJdataview : public CScrollView
{
	DECLARE_DYNCREATE(CJdataview)
protected:
	CJdataview();			// protected constructor used by dynamic creation

// Attributes
public:     
	int  m_nRowHeight, m_nRowWidth, m_nCharWidth;
	char	*m_buffer;		// Buffer for current row
	CString	m_lastlook;
	BOOL	m_ignorecase;
	BOOL	m_wrapround;
	BOOL	m_isstart, m_isend, m_ishat;	//  Markers for start/end/halt
	unsigned  long  m_startp, m_endp;

// Operations
public:	                                              
	char  *GetRow(const int row) { return ((CJdatadoc *)GetDocument())->GetRow(row, m_buffer); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJdataview)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual	void OnInitialUpdate();		// first time after construct
	virtual void OnUpdate(CView *, LPARAM, CObject *);
	//}}AFX_VIRTUAL

// Implementation

private:
	unsigned	GetNumRows();
	unsigned	GetNumCols();
	unsigned	Current_Page();
	void	UpdateScrollSizes();
	void	UpdatePageMarkers();
	void	set_page(void (CJdatadoc::*)(const long));
	int	 	matches(char *, CString, BOOL);
	int		lookback(const int, const int, int &);
	int		lookforw(const int, const int, int &);
	void	ExecuteSearch(const int = 0);

protected:
	virtual ~CJdataview();

	// Generated message map functions
	//{{AFX_MSG(CJdataview)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSearchSearchforward();
	afx_msg void OnSearchSearchbackwards();
	afx_msg void OnSearchSearchfor();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSearchSetstartpage();
	afx_msg void OnSearchSetendpage();
	afx_msg void OnSearchSethaltedatpage();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
