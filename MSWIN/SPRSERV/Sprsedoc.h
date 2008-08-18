// sprsedoc.h : interface of the CSprservDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CSprservDoc : public CDocument
{
protected: // create from serialization only
	CSprservDoc();
	DECLARE_DYNCREATE(CSprservDoc)

// Attributes

private:
	BOOL	m_timerset;
public:
	CMFList	m_flist;

// Operations
public:
	unsigned	GetDocSize() {	return m_flist.number();	}
	const  CMonFile	*GetRow(unsigned nrow)	{	return  m_flist[nrow];	}
	void	OnTimer(UINT);
	void	NewDropped(const char *);
	void	reschedule();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSprservDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
private:
	int		GetSelectedFile();

public:
	virtual ~CSprservDoc();
#ifdef _DEBUG
	virtual	void AssertValid() const;
	virtual	void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CSprservDoc)
	afx_msg void OnListAddnewfile();
	afx_msg void OnListChangefilename();
	afx_msg void OnListDeletefilefromlist();
	afx_msg void OnOptionsFormheadercopies();
	afx_msg void OnOptionsPage();
	afx_msg void OnOptionsRestoredefaults();
	afx_msg void OnOptionsRetain();
	afx_msg void OnOptionsSecurity();
	afx_msg void OnOptionsUserandmail();
	afx_msg void OnListOk();
	afx_msg void OnListWritetowinini();
	afx_msg void OnOptionsSizelimit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	void	PublicChfilename()	{ OnListChangefilename();	}
	void	PublicForm()		{ OnOptionsFormheadercopies();	}
};

/////////////////////////////////////////////////////////////////////////////
