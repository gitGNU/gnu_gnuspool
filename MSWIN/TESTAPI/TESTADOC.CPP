// testadoc.cpp : implementation of the CTestapiDoc class
//

#include "stdafx.h"
#include "xtapi.h"
#include "testapi.h"
#include "testadoc.h"
#include <winsock.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTestapiDoc

IMPLEMENT_DYNCREATE(CTestapiDoc, CDocument)

BEGIN_MESSAGE_MAP(CTestapiDoc, CDocument)
	//{{AFX_MSG_MAP(CTestapiDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestapiDoc construction/destruction

CTestapiDoc::CTestapiDoc()
{
	// TODO: add one-time construction code here
}

CTestapiDoc::~CTestapiDoc()
{
}

BOOL CTestapiDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	if  (xt_ptrlist(((CTestapiApp *)AfxGetApp())->m_apifd, XTAPI_FLAG_IGNORESEQ, &m_numptrs, &m_ptrslots) != XTAPI_OK)  {
		char	msg[50];
		int	num = WSAGetLastError();
		wsprintf(msg, "Printer list error %d", num);
		AfxMessageBox(msg, MB_ICONSTOP|MB_OK);
		AfxMessageBox("Cant read printer list", MB_ICONSTOP|MB_OK);
		m_numptrs = -1;
	}
	else  if  (m_numptrs > 0)  {
		m_ptrs = new FAR apispptr [m_numptrs];
		for  (int cnt = 0;  cnt < m_numptrs;  cnt++)
			if  (xt_ptrread(((CTestapiApp *)AfxGetApp())->m_apifd, XTAPI_FLAG_IGNORESEQ, m_ptrslots[cnt], &m_ptrs[cnt]) != XTAPI_OK)
				AfxMessageBox("Trouble reading a printer", MB_ICONSTOP|MB_OK);
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTestapiDoc serialization

void CTestapiDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTestapiDoc diagnostics

#ifdef _DEBUG
void CTestapiDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CTestapiDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTestapiDoc commands
