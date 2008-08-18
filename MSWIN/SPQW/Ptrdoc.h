class CPtrdoc : public CDocument
{
	DECLARE_DYNCREATE(CPtrdoc)
protected:
	CPtrdoc();			// protected constructor used by dynamic creation

// Printer list we are supporting and are interested in
	plist		m_ptrlist;		//  List of printers in window
public:
	restrictdoc	m_wrestrict;	//  Restricted view
	BOOL	m_sformtype, m_sdevice, m_sprinter, m_swraparound;
	CPtrcolours	m_ptrcolours;


// Attributes
public:
	void	setrestrict(const restrictdoc &r) { m_wrestrict = r; }
	void	reviseptrs(plist FAR &);
	unsigned	number()	{  return  m_ptrlist.number();	}
	int		pindex(const pident &ind) { return m_ptrlist.pindex(ind); }
	int		pindex(const spptr &j) { return m_ptrlist.pindex(j); }
	spptr	*operator [] (const unsigned ind) { return m_ptrlist[ind]; }
	spptr	*operator [] (const pident &ind) { return m_ptrlist[ind];  }

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPtrdoc)
	public:
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CPtrdoc();
	spptr	*GetSelectedPtr(const BOOL = TRUE);
	void	DoSearch(const CString, const BOOL);
	BOOL	Smatches(const CString, const int);

	// Generated message map functions
protected:
	//{{AFX_MSG(CPtrdoc)
	afx_msg void OnActionDisapporvealignment();
	afx_msg void OnPrintersClasscodes();
	afx_msg void OnActionGoprinter();
	afx_msg void OnActionHaltatonce();
	afx_msg void OnActionHalteoj();
	afx_msg void OnActionOkalignment();
	afx_msg void OnPrintersFormtype();
	afx_msg void OnPrintersInterrupt();
	afx_msg void OnSearchSearchfor();
	afx_msg void OnSearchSearchforward();
	afx_msg void OnSearchSearchbackwards();
	afx_msg void OnWindowWindowoptions();
	afx_msg void OnFileWpcolourAwoper();
	afx_msg void OnFileWpcolourError();
	afx_msg void OnFileWpcolourHalted();
	afx_msg void OnFileWpcolourIdle();
	afx_msg void OnFileWpcolourOffline();
	afx_msg void OnFileWpcolourPrinting();
	afx_msg void OnFileWpcolourShutd();
	afx_msg void OnFileWpcolourStartup();
	afx_msg void OnFilePcolourCopytoprog();
	afx_msg void OnFilePcolourCopytowin();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void RunColourDlg(const unsigned n);
};

