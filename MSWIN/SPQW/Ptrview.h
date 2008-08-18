class CPtrView : public CRowView
{
	DECLARE_DYNCREATE(CPtrView)
public:
	CPtrView();
                         
private:
	pident	curr_ptr;
                         
// Attributes
public:
	CPtrdoc* GetDocument()  {
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPtrdoc)));
		return (CPtrdoc*) m_pDocument;
	}

// Overrides of CView
	void OnUpdate(CView* pSender, LPARAM lHint = 0L, CObject* pHint = NULL);

// Overrides of CRowView
	void       GetRowWidthHeight(CDC* pDC, int& nRowWidth, int& nRowHeight);
	int        GetActiveRow();
	unsigned   GetRowCount();
	void       OnDrawRow(CDC* pDC, int nRowNo, int y, BOOL bSelected);
	void       ChangeSelectionToRow(int nRow);
	void	   PtrAllChange(const BOOL);
	void	   PtrChgPtr(const unsigned);
	void	   Initformats();
	void	   SaveWidthSettings();
	void	   Dopopupmenu(CPoint pos);

// Implementation
protected:
	virtual ~CPtrView() {}
};
