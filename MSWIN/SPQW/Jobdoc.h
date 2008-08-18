class CJobdoc : public CDocument
{
	DECLARE_DYNCREATE(CJobdoc)
protected:
	CJobdoc();			// protected constructor used by dynamic creation

// Job list we are supporting and are interested in

	joblist		m_joblist;		//  List of jobs in window
public:
	restrictdoc	m_wrestrict;	//  Restricted view
	BOOL	m_sformtype, m_sjtitle, m_sprinter, m_suser, m_swraparound;

public:                        
	void	setrestrict(const restrictdoc &r) { m_wrestrict = r; }
	void	revisejobs(joblist &);
	unsigned	number()	{  return  m_joblist.number();	}
	int		jindex(const jident &ind) { return m_joblist.jindex(ind); }
	int		jindex(const spq &j) { return m_joblist.jindex(j); }
	spq		*operator [] (const unsigned ind) { return m_joblist[ind]; }
	spq		*operator [] (const jident &ind) { return m_joblist[ind];  }
 		
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJobdoc)
	public:
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CJobdoc();
	spq		*GetSelectedJob(const BOOL = TRUE);
	void	DoSearch(const CString, const BOOL);
	BOOL	Smatches(const CString, const int);

	// Generated message map functions
protected:
	//{{AFX_MSG(CJobdoc)
	afx_msg void OnJobsFormandcopies();
	afx_msg void OnJobsClasscodes();
	afx_msg void OnActionAnothercopy();
	afx_msg void OnActionAbortjob();
	afx_msg void OnJobsPages();
	afx_msg void OnJobsRetention();
	afx_msg void OnJobsUserandmail();
	afx_msg void OnJobsViewjob();
	afx_msg void OnSearchSearchbackwards();
	afx_msg void OnSearchSearchfor();
	afx_msg void OnSearchSearchforward();
	afx_msg void OnWindowWindowoptions();
	afx_msg void OnJobsUnqueuejob();
	afx_msg void OnJobsCopyjob();
	afx_msg void OnJobsCopyoptionsinjob();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
