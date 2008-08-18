// testavw.h : interface of the CTestapiView class
//
/////////////////////////////////////////////////////////////////////////////

class CTestapiView : public CView
{
protected: // create from serialization only
	CTestapiView();
	DECLARE_DYNCREATE(CTestapiView)

// Attributes
public:
	CTestapiDoc* GetDocument();

// Operations
public:

// Implementation
public:
	virtual ~CTestapiView();
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTestapiView)
	afx_msg void OnSplurgeCreatejob();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in testavw.cpp
inline CTestapiDoc* CTestapiView::GetDocument()
   { return (CTestapiDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
