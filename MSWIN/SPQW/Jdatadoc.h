#define	JD_LHINT_DOC	0L
#define	JD_LHINT_PAGES	1L

class pagemarker {
public:
	long	pageoffset;		// Offset in file of start of page
	unsigned	linecount;	// Corresponding line count
};	

class CJdatadoc : public CDocument
{
	DECLARE_DYNCREATE(CJdatadoc)
protected:
	CJdatadoc();			// protected constructor used by dynamic creation

// Attributes
	spq		jcopy;				// copy of job (make this a pointer???)
	CFile	jobfile;			// temporary file for job
	CString	tname;				// temporary file name
	pagemarker	*pagebreaks;	// where the page breaks come
	unsigned	charwidth, charheight;	//  Size of job in chars
	unsigned	changes;
public:
	BOOL	m_donttrustpages;	// Dont trust pages markers
	BOOL	m_invalid;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJdatadoc)
	public:
	virtual void DeleteContents();
	//}}AFX_VIRTUAL

// Implementation
private:
	void  setpage(unsigned long spq::*, const long);

// Operations
public:
	void  setjob(const spq &j) { jcopy = j;	}
	int	  loaddoc();                                       
	int   scanbyformfeeds(char *, UINT);
	void  setstart(const long p)	{ setpage(&spq::spq_start, p);	}
	void  setend(const long p)		{ setpage(&spq::spq_end, p);	}
	void  sethat(const long p)		{ setpage(&spq::spq_haltat, p);	}
	const unsigned long  getstart()	const	{	return  jcopy.spq_start;	}
	const unsigned long	getend() const	{   return	jcopy.spq_end;		}
	const unsigned long	gethat()const	{	return  jcopy.spq_haltat;	}
	unsigned  jdwidth()		const	{	return  charwidth;	}
	unsigned  jdheight()	const	{	return  charheight;	}
	UINT  RowLength(const long);
	long  Locrow(const int);
	char  *FindRow(const int);
	char  *GetRow(const int, char *);
	unsigned  whatpage(const int);

// Implementation

public:
	virtual ~CJdatadoc();

	// Generated message map functions
protected:
	//{{AFX_MSG(CJdatadoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

