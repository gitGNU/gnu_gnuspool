// sprsevw.h : interface of the CSprsetwView class
//
/////////////////////////////////////////////////////////////////////////////

class CSprsetwView : public CScrollView
{
protected: // create from serialization only
	CSprsetwView();
	DECLARE_DYNCREATE(CSprsetwView)

// Attributes
public:
	int	m_nCharWidth;			// width of characters
	int m_nRowWidth;            // width of row in current device units
	int m_nRowHeight;           // height of row in current device units
	int m_nSelectedRow;			// record of selected row
	CSprsetwDoc* GetDocument();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSprsetwView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnUpdate(CView*, LPARAM = 0L, CObject* = NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	int		GetActiveRow()	{	return  m_nSelectedRow;	}
	void 	OnDrawRow(CDC*, int, int, BOOL);

// Implementation
public:
	virtual ~CSprsetwView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CSprsetwView)
	afx_msg void OnNetworkAddnewhost();
	afx_msg void OnNetworkDeletehost();
	afx_msg void OnNetworkChangehost();
	afx_msg void OnNetworkSetasserver();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnOptionsRestoredefaults();
	afx_msg void OnOptionsFormandcopies();
	afx_msg void OnOptionsPage();
	afx_msg void OnOptionsRetain();
	afx_msg void OnOptionsSecurity();
	afx_msg void OnOptionsUserandmail();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnOptionsProgramoptions();
	afx_msg void OnProgramPortsettings();
	afx_msg void OnOptionsSizelimit();
	afx_msg void OnProgramLoginorlogout();
	afx_msg void OnUpdateOptionsFormandcopies(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOptionsPage(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOptionsRetain(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOptionsSizelimit(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOptionsUserandmail(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOptionsSecurity(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void	UpdateScrollSizes();
};

#ifndef _DEBUG	// debug version in sprsevw.cpp
inline CSprsetwDoc* CSprsetwView::GetDocument()
   { return (CSprsetwDoc*) m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
