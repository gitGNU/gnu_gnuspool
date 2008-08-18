// mainfrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h" 
#include "jobdoc.h"
#include "ptrdoc.h"
#include "jdatadoc.h"
#include "spqw.h"
#include "opdlg22.h"
#include "poptsdlg.h"
#include "formatcode.h"
#include "fmtdef.h"
#include "netstat.h" 
#include "uperm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_WINDOW_NEWJOBWINDOW, OnWindowNewjobwindow)
	ON_COMMAND(ID_WINDOW_NEWPRINTERWINDOW, OnWindowNewprinterwindow)
	ON_WM_TIMER()
	ON_COMMAND(ID_FILE_PROGRAMOPTIONS, OnFileProgramoptions)
	ON_MESSAGE(WM_NETMSG_ARRIVED, OnNMArrived)
	ON_MESSAGE(WM_NETMSG_NEWCONN, OnNMNewconn)
	ON_MESSAGE(WM_NETMSG_PROBERCV, OnNMProbercv)
	ON_COMMAND(ID_FILE_SAVETOFILE, OnFileSavetofile)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVETOFILE, OnUpdateFileSavetofile)
	ON_COMMAND(ID_FILE_DISPLAYOPTIONS, OnFileDisplayoptions)
	ON_COMMAND(ID_WINDOW_SETJOBLISTFORMAT, OnWindowSetjoblistformat)
	ON_COMMAND(ID_WINDOW_SETPRINTERLISTFORMAT, OnWindowSetprinterlistformat)
	ON_COMMAND(ID_FILE_NETWORKSTATS, OnFileNetworkstats)
	ON_COMMAND(ID_FILE_USERPERMISSIONS, OnFileUserpermissions)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_STARTP, OnUpdateStartp)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_ENDP, OnUpdateEndp)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_HATP, OnUpdateHatp)
	ON_COMMAND(ID_FILE_PCOLOUR_AWOPER, OnFilePcolourAwoper)
	ON_COMMAND(ID_FILE_PCOLOUR_ERROR, OnFilePcolourError)
	ON_COMMAND(ID_FILE_PCOLOUR_HALTED, OnFilePcolourHalted)
	ON_COMMAND(ID_FILE_PCOLOUR_IDLE, OnFilePcolourIdle)
	ON_COMMAND(ID_FILE_PCOLOUR_OFFLINE, OnFilePcolourOffline)
	ON_COMMAND(ID_FILE_PCOLOUR_PRINTING, OnFilePcolourPrinting)
	ON_COMMAND(ID_FILE_PCOLOUR_SHUTD, OnFilePcolourShutd)
	ON_COMMAND(ID_FILE_PCOLOUR_STARTUP, OnFilePcolourStartup)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_INDEX, CMDIFrameWnd::OnHelpIndex)
	ON_COMMAND(ID_HELP_USING, CMDIFrameWnd::OnHelpUsing)
	ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpIndex)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars

static UINT BASED_CODE indicators[] =
{
	ID_SEPARATOR,			// status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
	ID_INDICATOR_STARTP,
	ID_INDICATOR_ENDP,
	ID_INDICATOR_HATP
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_dispenab = m_dispstart = m_dispend = m_disphat = FALSE;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE("Failed to create toolbar\n");
		return -1;		// fail to create
	}

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))  {
		TRACE("Failed to create status bar\n");
		return -1;		// fail to create
	}

	return 0;
}

void	CMainFrame::InitWins()
{
	OnWindowNewprinterwindow();
	OnWindowNewjobwindow();
    MDITile(MDITILE_HORIZONTAL);
}    

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL	CMainFrame::DoRstrDlg(restrictdoc &r)
{
#if	XITEXT_VN < 22
	COPdlg	pdlg(this);
	unsigned  short classc;
#else
	COPdlg22	pdlg(this);
	unsigned  long	classc;
#endif
	int		risp, jincl;
	r.getrestrict(risp, classc, pdlg.m_onlyu, pdlg.m_onlyprin, pdlg.m_onlytitle, jincl); 
	pdlg.m_punpjobs = risp;
	pdlg.m_classc = classc;
	pdlg.m_jinclude = jincl;
	CSpqwApp *ma = (CSpqwApp *) AfxGetApp();
	pdlg.m_maxclass = ma->m_mypriv.spu_class;
	pdlg.m_mayoverride = ma->m_mypriv.ispriv(PV_COVER);
	if  (pdlg.DoModal() != IDOK)
		return  FALSE;
	r.setclass(pdlg.m_classc);		
	r.setonlyunprinted(pdlg.m_punpjobs > 1? restrictdoc::ONLYPRINTED: pdlg.m_punpjobs? restrictdoc::ONLYUNPRINTED: restrictdoc::ALLITEMS);
	r.setuser(pdlg.m_onlyu);
	r.setprinter(pdlg.m_onlyprin);
	r.settitle(pdlg.m_onlytitle);
	r.setjincl(pdlg.m_jinclude);
	return  TRUE;
}	

void CMainFrame::OnWindowNewjobwindow()
{
	CJobdoc *doc = (CJobdoc *) m_dtjob->CreateNewDocument();
	static int count = 0;
	char	tbuf[20];
	wsprintf(tbuf, "Job List %d", ++count);
	doc->SetTitle(tbuf);
	doc->m_wrestrict = m_restrictlist;
	CFrameWnd *frm = m_dtjob->CreateNewFrame(doc, NULL);
	m_dtjob->InitialUpdateFrame(frm, doc);
}

void CMainFrame::OnWindowNewprinterwindow()
{
	CPtrdoc *doc = (CPtrdoc *) m_dtptr->CreateNewDocument();
	static int count = 0;
	char	tbuf[26];
	wsprintf(tbuf, "Printer List %d", ++count);
	doc->SetTitle(tbuf);
	doc->m_wrestrict = m_restrictlist;
	CFrameWnd *frm = m_dtptr->CreateNewFrame(doc, NULL);
	m_dtptr->InitialUpdateFrame(frm, doc);
}

void CMainFrame::OnNewJDWin(spq *cj)
{
	CJdatadoc *doc = (CJdatadoc *) m_dtjdata->CreateNewDocument();
	doc->setjob(*cj);
	if  (!doc->loaddoc())
		doc->m_invalid = TRUE;
	CFrameWnd *frm = m_dtjdata->CreateNewFrame(doc, NULL);
	m_dtjdata->InitialUpdateFrame(frm, doc);	
}	

void CMainFrame::SetPageMarker(const PageMarkers which, const BOOL on)
{
	switch  (which)  {
	default:
		m_dispstart = on;
		break;
	case  End:
		m_dispend = on;
		break;
	case  Halted_at:
		m_disphat = on;
		break;
	}
}

void	CMainFrame::OnUpdateStartp(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_dispenab  &&  m_dispstart);
}

void	CMainFrame::OnUpdateEndp(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_dispenab  &&  m_dispend);
}

void	CMainFrame::OnUpdateHatp(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_dispenab  &&  m_disphat);
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	refreshconn();
}

void CMainFrame::OnFileProgramoptions()
{
	CPOptsdlg	dlg;
	spqopts	 &qopts = ((CSpqwApp *) AfxGetApp())->m_options.spq_options;
	dlg.m_confabort = qopts.confabort;
	dlg.m_probewarn = qopts.probewarn;
	dlg.m_polltime = qopts.Pollinit;
	dlg.m_sjext = qopts.batext;
	dlg.m_abshold = qopts.abshold != 0;
	if  (dlg.m_confabort < 0)
		dlg.m_confabort = 0;
	else  if  (dlg.m_confabort > 2)
		dlg.m_confabort = 2;              
	if  (dlg.m_probewarn < 0)
		dlg.m_probewarn = 0;
	else  if  (dlg.m_probewarn > 1)
		dlg.m_probewarn = 1;
	if  (dlg.DoModal() == IDOK)  {
		qopts.confabort = dlg.m_confabort;
		qopts.probewarn = dlg.m_probewarn;
		qopts.batext = dlg.m_sjext;
		qopts.abshold = dlg.m_abshold? 1: 0;
		qopts.Pollinit = m_pollfreq = dlg.m_polltime;
		((CSpqwApp *) AfxGetApp())->dirty();
	}
}		

// Accept stuff arriving from network

LRESULT CMainFrame::OnNMArrived(WPARAM wParam, LPARAM lParam)
{
	net_recvmsg(wParam, lParam);
	return  0;
}

LRESULT CMainFrame::OnNMNewconn(WPARAM wParam, LPARAM lParam)
{
	net_recvconn(wParam, lParam);
	return  0;
}

LRESULT CMainFrame::OnNMProbercv(WPARAM wParam, LPARAM lParam)
{             
	net_recvprobe(wParam, lParam);
	return  0;
}

void CMainFrame::OnFileSavetofile()
{
	((CSpqwApp *) AfxGetApp())->save_options();	
}

void CMainFrame::OnUpdateFileSavetofile(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(((CSpqwApp *) AfxGetApp())->isdirty());
}

void CMainFrame::OnFileDisplayoptions()
{
	if  (DoRstrDlg(m_restrictlist))  {
		m_restrictlist.saverestrict();
		((CSpqwApp *) AfxGetApp())->dirty();
	}	
}

void CMainFrame::OnWindowSetjoblistformat()
{
	CSpqwApp	&app = *((CSpqwApp *) AfxGetApp());
	CFmtdef	dlg;
	dlg.m_fmtstring = app.m_jfmt;
	dlg.m_defcode = IDS_DEF_JFMT;
	dlg.m_what4 = IDS_EDITINGJFMT;
	dlg.m_uppercode = IDS_JFORMAT_A;
	dlg.m_lowercode = IDS_JFORMAT_aa;
	if  (dlg.DoModal() == IDOK)  {
		app.m_jfmt = dlg.m_fmtstring;
		app.dirty();
		app.m_pMainWnd->SendMessageToDescendants(WM_NETMSG_JREVISED, 0, 1);
	}
}

void CMainFrame::OnWindowSetprinterlistformat()
{
	CSpqwApp	&app = *((CSpqwApp *) AfxGetApp());
	CFmtdef	dlg;
	dlg.m_fmtstring = app.m_pfmt;
	dlg.m_defcode = IDS_DEF_PFMT;
	dlg.m_what4 = IDS_EDITINGPFMT;
	dlg.m_uppercode = 0;
	dlg.m_lowercode = IDS_PFORMAT_aa;
	if  (dlg.DoModal() == IDOK)  {
		app.m_pfmt = dlg.m_fmtstring;
		app.dirty();
		app.m_pMainWnd->SendMessageToDescendants(WM_NETMSG_PREVISED, 0, 1);
	}
}

void CMainFrame::OnFileNetworkstats()
{
	CNetstat	dlg;
	dlg.DoModal();
}

void CMainFrame::OnFileUserpermissions()
{
	CUperm	dlg;
	dlg.DoModal();
}

void CMainFrame::OnFilePcolourAwoper() 
{
	RunColourDlg(SPP_OPER);
}

void CMainFrame::OnFilePcolourError() 
{
	RunColourDlg(SPP_ERROR);
}

void CMainFrame::OnFilePcolourHalted() 
{
	RunColourDlg(SPP_HALT);
}

void CMainFrame::OnFilePcolourIdle() 
{
	RunColourDlg(SPP_WAIT);
}

void CMainFrame::OnFilePcolourOffline() 
{
	RunColourDlg(SPP_OFFLINE);
}

void CMainFrame::OnFilePcolourPrinting() 
{
	RunColourDlg(SPP_RUN);
}

void CMainFrame::OnFilePcolourShutd() 
{
	RunColourDlg(SPP_SHUTD);
}

void CMainFrame::OnFilePcolourStartup() 
{
	RunColourDlg(SPP_INIT);
}

void CMainFrame::RunColourDlg(const unsigned int n)
{
	CSpqwApp	&app = *((CSpqwApp *) AfxGetApp());
	COLORREF	&mc = app.m_appcolours[n];
	CColorDialog	cdlg;
	cdlg.m_cc.Flags |= CC_RGBINIT|CC_SOLIDCOLOR;
	cdlg.m_cc.rgbResult = mc;
	if  (cdlg.DoModal() == IDOK)  {
		mc = cdlg.m_cc.rgbResult;
		app.dirty();
	}
}
