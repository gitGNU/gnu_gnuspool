// mainfrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "pages.h"
#include "xtini.h"
#include "sprserv.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "monfile.h"
#include "sprsedoc.h"
#include "mfdlg.h"
#include "formdlg.h"
#include "pagedlg.h"
#include "retndlg.h"
#include "retnabsdlg.h"
#include "secdlg22.h"
#include "userdlg.h"
#include "hmsg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#if	_MSC_VER == 700
#define UNIXTODOSTIME	((70UL * 365UL + 68/4 + 1) * 24UL * 3600UL)
#else
#define	UNIXTODOSTIME	0
#endif

extern	UINT		initsockets();

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_LIST_FORMHEADERCOPIES, OnListFormheadercopies)
	ON_COMMAND(ID_LIST_PAGE, OnListPage)
	ON_COMMAND(ID_LIST_RETAIN, OnListRetain)
	ON_COMMAND(ID_LIST_SECURITY, OnListSecurity)
	ON_COMMAND(ID_LIST_USERANDMAIL, OnListUserandmail)
	ON_WM_DROPFILES()
	ON_MESSAGE(WM_NETMSG_MESSRCV, OnHostMsg)
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_INDEX, CFrameWnd::OnHelpIndex)
	ON_COMMAND(ID_HELP_USING, CFrameWnd::OnHelpUsing)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpIndex)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars

static UINT BASED_CODE indicators[] =
{
	ID_SEPARATOR,			// status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
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

	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE("Failed to create status bar\n");
		return -1;		// fail to create
	}

	DragAcceptFiles();
	AfxGetApp()->m_pMainWnd = this;
//	SetTimer(WM_NETMSG_TICKLE, Locparams.servtimeout*1000, NULL);
	UINT	 ret = initsockets();
	if  (ret != 0)
		AfxMessageBox(ret, MB_OK|MB_ICONSTOP);    
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnListFormheadercopies()
{
	CSprservApp	*ma = (CSprservApp *)AfxGetApp();
	spq		&pars = ma->m_options.qparams;
	CFormdlg	fdlg;
	fdlg.m_formtype = pars.spq_form;
	fdlg.m_printer = pars.spq_ptr;
	fdlg.m_header = pars.spq_file;
	fdlg.m_copies = pars.spq_cps;
	fdlg.m_priority = pars.spq_pri;
	fdlg.m_supph = pars.spq_jflags & SPQ_NOH? TRUE: FALSE;
	if  (fdlg.DoModal() == IDOK)  {
		strcpy(pars.spq_form, fdlg.m_formtype);
		strcpy(pars.spq_ptr, fdlg.m_printer);
		strcpy(pars.spq_file, fdlg.m_header);
		pars.spq_cps = fdlg.m_copies;
		pars.spq_pri = fdlg.m_priority;
		if  (fdlg.m_supph)
			pars.spq_jflags |= SPQ_NOH;
		else
			pars.spq_jflags &= ~SPQ_NOH;
	}	
}

void CMainFrame::OnListPage()
{
	CSprservApp	*ma = (CSprservApp *)AfxGetApp();
	spq		&pars = ma->m_options.qparams;
	pages	&pgs = ma->m_options.spr_options.pfe;
	CPagedlg	pdlg;
	pdlg.m_jflags = pars.spq_jflags;
	pdlg.m_startp = pars.spq_start;
	pdlg.m_endp = pars.spq_end;
	pdlg.m_deliml = unsigned(pgs.deliml);
	pdlg.m_delimiter = ma->m_options.spr_options.delimiter;
	pdlg.m_delimnum = unsigned(pgs.delimnum); 
	pdlg.m_ppflags = pars.spq_flags;
	if  (pdlg.DoModal() == IDOK)  {
		pars.spq_jflags = pdlg.m_jflags;
		pars.spq_start = pdlg.m_startp;
		pars.spq_end = pdlg.m_endp;
		pgs.deliml = pdlg.m_deliml;
		pgs.delimnum = pdlg.m_delimnum;
		if  (ma->m_options.spr_options.delimiter)
			delete [] ma->m_options.spr_options.delimiter;
		ma->m_options.spr_options.delimiter = new char [pgs.deliml];
		memcpy(ma->m_options.spr_options.delimiter, pdlg.m_delimiter, unsigned(pgs.deliml));
		strncpy(pars.spq_flags, (const char *) pdlg.m_ppflags, MAXFLAGS);
	}
}
void CMainFrame::OnListRetain()
{
	CSprservApp	*ma = (CSprservApp *)AfxGetApp();
	spq		&pars = ma->m_options.qparams;
	if  (ma->m_options.spq_options.abshold)  {
		CRetnabsdlg rdlg;
		rdlg.m_dinp = pars.spq_nptimeout;
		rdlg.m_dip = pars.spq_ptimeout;
		rdlg.m_hold = pars.spq_hold != 0;
		rdlg.m_retain = pars.spq_jflags & SPQ_RETN? TRUE: FALSE;
		rdlg.m_holdtime = pars.spq_hold + UNIXTODOSTIME;
		if  (rdlg.DoModal() == IDOK)  {
		    pars.spq_nptimeout = rdlg.m_dinp;
			pars.spq_ptimeout = rdlg.m_dip;
			if  (rdlg.m_hold)
		    	pars.spq_hold = rdlg.m_holdtime - UNIXTODOSTIME;
		    else
				pars.spq_hold = 0;
			if  (rdlg.m_retain)
	    		pars.spq_jflags |= SPQ_RETN;	
			else	
	    		pars.spq_jflags &= ~SPQ_RETN;
		}	
	}
	else  {
		CRetndlg	rdlg;
		rdlg.m_dinp = pars.spq_nptimeout;
		rdlg.m_dip = pars.spq_ptimeout;
		rdlg.m_hold = pars.spq_hold != 0;
		rdlg.m_retain = pars.spq_jflags & SPQ_RETN? TRUE: FALSE;	
		rdlg.m_holdtime = pars.spq_hold;
		if  (rdlg.DoModal() == IDOK)  {
		    pars.spq_nptimeout = rdlg.m_dinp;
			pars.spq_ptimeout = rdlg.m_dip;
			if  (rdlg.m_hold)
		    	pars.spq_hold = rdlg.m_holdtime;
		    else
				pars.spq_hold = 0;
			if  (rdlg.m_retain)
	    		pars.spq_jflags |= SPQ_RETN;	
			else	
	    		pars.spq_jflags &= ~SPQ_RETN;
		}	
	}
}

void CMainFrame::OnListSecurity()
{
	CSprservApp	*ma = (CSprservApp *)AfxGetApp();
	spq		&pars = ma->m_options.qparams;
	CSecdlg22 sdlg;
	sdlg.m_classc = pars.spq_class;
	spdet	&mypriv = ((CSprservApp *)AfxGetApp())->m_mypriv;
	sdlg.m_maxclass = mypriv.spu_class;
	sdlg.m_mayoverride = mypriv.ispriv(PV_COVER);	
	sdlg.m_localonly = pars.spq_jflags & SPQ_LOCALONLY? TRUE: FALSE;
	if  (sdlg.DoModal() == IDOK)  {
		pars.spq_class = sdlg.m_classc;
		if  (sdlg.m_localonly)
			pars.spq_jflags |= SPQ_LOCALONLY;
		else
			pars.spq_jflags &= ~SPQ_LOCALONLY;
	}
}

void CMainFrame::OnListUserandmail()
{
	CSprservApp	*ma = (CSprservApp *)AfxGetApp();
	spq		&pars = ma->m_options.qparams;
	CUserdlg  udlg;
	udlg.m_mail = pars.spq_jflags & SPQ_MAIL? TRUE: FALSE;
	udlg.m_write = pars.spq_jflags & SPQ_WRT? TRUE: FALSE;
	udlg.m_mattn = pars.spq_jflags & SPQ_MATTN? TRUE: FALSE;
	udlg.m_wattn = pars.spq_jflags & SPQ_WATTN? TRUE: FALSE;
	udlg.m_postuser = strcmp(pars.spq_uname, pars.spq_puname) == 0? "" : pars.spq_puname;
	if  (udlg.DoModal() == IDOK)  { 
		if  (udlg.m_postuser.IsEmpty())
			strcpy(pars.spq_puname, pars.spq_uname);
		else
			strcpy(pars.spq_puname, (const char *) udlg.m_postuser);
		pars.spq_jflags &= ~(SPQ_MAIL|SPQ_WRT|SPQ_MATTN|SPQ_WATTN);
		if  (udlg.m_mail)
			pars.spq_jflags |= SPQ_MAIL;
		if  (udlg.m_write)
			pars.spq_jflags |= SPQ_WRT;
		if  (udlg.m_mattn)
			pars.spq_jflags |= SPQ_MATTN;
		if  (udlg.m_wattn)
			pars.spq_jflags |= SPQ_WATTN;		
	}
}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	UINT	fnum = ::DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	for  (UINT  cnt = 0;  cnt < fnum;  cnt++)  {
		char	fpath[_MAX_PATH];
		::DragQueryFile(hDropInfo, cnt, fpath, _MAX_PATH);
		((CSprservDoc *)GetActiveDocument())->NewDropped(fpath);
	}
	::DragFinish(hDropInfo);
	((CSprservDoc *)GetActiveDocument())->reschedule();	
}

LPARAM	CMainFrame::OnHostMsg(WPARAM wParam, LPARAM lParam)
{   
	int		inbytes;
	sockaddr_in		sin;
	int		fromlen = sizeof(sin);
	char	inbuf[1024];
	if  ((inbytes = recvfrom(Locparams.probesock, inbuf, sizeof(inbuf)-1, 0, (sockaddr FAR *) &sin, &fromlen)) <= 0)  {
		if  (WSAGetLastError() != WSAEMSGSIZE)
			return  0;
		inbytes = sizeof(inbuf) - 1;
	}                          
	inbuf[inbytes] = '\0';
	CHmsg	*dlg = new  CHmsg(look_host(sin.sin_addr.s_addr), inbuf);
	dlg->UpdateData(FALSE);
	return  0;
}			
