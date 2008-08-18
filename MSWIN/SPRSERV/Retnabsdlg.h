#if !defined(AFX_RETNABSDLG_H__C7FA7AE0_C456_11D4_9542_00E09872E940__INCLUDED_)
#define AFX_RETNABSDLG_H__C7FA7AE0_C456_11D4_9542_00E09872E940__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Retnabsdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRetnabsdlg dialog

class CRetnabsdlg : public CDialog
{
// Construction
public:
	CRetnabsdlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRetnabsdlg)
	enum { IDD = IDD_RETAINABS };
	BOOL	m_retain;
	UINT	m_dip;
	UINT	m_dinp;
	BOOL	m_hold;
	//}}AFX_DATA
	long	m_holdtime;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRetnabsdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void	Enablehtime(const BOOL = TRUE);
public:
	void	fillintime();

protected:

	// Generated message map functions
	//{{AFX_MSG(CRetnabsdlg)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHold();
	afx_msg void OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrWday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMon(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RETNABSDLG_H__C7FA7AE0_C456_11D4_9542_00E09872E940__INCLUDED_)
