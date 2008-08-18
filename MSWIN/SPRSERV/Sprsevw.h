// sprsevw.h : interface of the CSprservView class
//
/////////////////////////////////////////////////////////////////////////////

class CSprservView : public CScrollView
{
protected: // create from serialization only
	CSprservView();
	DECLARE_DYNCREATE(CSprservView)

// Attributes
public:
	int	m_nCharWidth;			// width of characters
	int m_nRowWidth;            // width of row in current device units
	int m_nRowHeight;           // height of row in current device units
	int m_nSelectedRow;			// record of selected row
	CSprservDoc* GetDocument();

// Operations
public:
	int		GetActiveRow()	{	return  m_nSelectedRow;	}
	void 	OnDrawRow(CDC*, int, int, BOOL);
	void	STimer(UINT,UINT);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSprservView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnUpdate(CView*, LPARAM = 0L, CObject* = NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSprservView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CSprservView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void	UpdateScrollSizes();
};

#ifndef _DEBUG	// debug version in sprsevw.cpp
inline CSprservDoc* CSprservView::GetDocument()
   { return (CSprservDoc*) m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
