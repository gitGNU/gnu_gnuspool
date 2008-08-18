// testadoc.h : interface of the CTestapiDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CTestapiDoc : public CDocument
{
protected: // create from serialization only
	CTestapiDoc();
	DECLARE_DYNCREATE(CTestapiDoc)

// Attributes
public:
	int	m_numptrs;
	slotno_t	FAR * m_ptrslots;
	apispptr	FAR * m_ptrs;
	
// Operations
public:

// Implementation
public:
	virtual ~CTestapiDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual BOOL OnNewDocument();

// Generated message map functions
protected:
	//{{AFX_MSG(CTestapiDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
