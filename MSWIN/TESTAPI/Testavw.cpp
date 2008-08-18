// testavw.cpp : implementation of the CTestapiView class
//

#include "stdafx.h"
#include "xtapi.h"
#include "testapi.h"
#include "testadoc.h"
#include "testavw.h"
#include <winsock.h>
#include <fcntl.h>
#include <io.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTestapiView

IMPLEMENT_DYNCREATE(CTestapiView, CView)

BEGIN_MESSAGE_MAP(CTestapiView, CView)
	//{{AFX_MSG_MAP(CTestapiView)
	ON_COMMAND(ID_SPLURGE_CREATEJOB, OnSplurgeCreatejob)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestapiView construction/destruction

CTestapiView::CTestapiView()
{
	// TODO: add construction code here
}

CTestapiView::~CTestapiView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CTestapiView drawing

void CTestapiView::OnDraw(CDC* pDC)
{
	CTestapiDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	TEXTMETRIC textm;
	pDC->GetTextMetrics(&textm);
	int nRowHeight = textm.tmHeight;
	CString	msg;
	char	omsg[50];
	wsprintf(omsg, "Found %d printers", pDoc->m_numptrs);
	msg = omsg;
	pDC->TextOut(0, 0, msg);
	for  (int cnt = 0;  cnt < pDoc->m_numptrs;  cnt++)  {
		hostent	*hp = gethostbyaddr((char FAR *) &pDoc->m_ptrs[cnt].apispp_netid, sizeof(netid_t), PF_INET);
		wsprintf(omsg, "%s:%s", hp? hp->h_name: "unknown", pDoc->m_ptrs[cnt].apispp_ptr);
		msg = omsg;
		pDC->TextOut(10, (cnt+2)*nRowHeight, msg);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTestapiView diagnostics

#ifdef _DEBUG
void CTestapiView::AssertValid() const
{
	CView::AssertValid();
}

void CTestapiView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTestapiDoc* CTestapiView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTestapiDoc)));
	return (CTestapiDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTestapiView message handlers

void CTestapiView::OnSplurgeCreatejob()
{
	apispq	tryit;
	memset((void *) &tryit, '\0', sizeof(tryit));
	tryit.apispq_nptimeout = 199;
	tryit.apispq_ptimeout = 42;
	tryit.apispq_cps = 3;
	tryit.apispq_pri = 149;
	tryit.apispq_jflags = APISPQ_WRT|APISPQ_WATTN;
	tryit.apispq_class = 0xFFFFFFFF;
	tryit.apispq_start = 0;
	tryit.apispq_end = 1233;
	strcpy(tryit.apispq_puname, "wally");
    strcpy(tryit.apispq_file, "fromdosapi");
	strcpy(tryit.apispq_form, "cruddier");
	strcpy(tryit.apispq_ptr, "tattooneedle");
	int	fd = ::open("README.TXT", O_RDONLY);
	char	b1[100];
	int	cnt;
	cnt = read(fd, b1, sizeof(b1));
	lseek(fd, 0L, 0);
	if  (xt_jobadd(((CTestapiApp *)AfxGetApp())->m_apifd, fd, read, &tryit, NULL, 1, 1) == XTAPI_OK)  {
		char	message[80];
		wsprintf(message, "Job added ok, job number %ld", tryit.apispq_job);
		AfxMessageBox(message, MB_OK);
	}
	else
		AfxMessageBox("BLEAH", MB_ICONEXCLAMATION|MB_OK);
	lseek(fd, 0L, 0);
	cnt = read(fd, b1, sizeof(b1));
	close(fd);
}
