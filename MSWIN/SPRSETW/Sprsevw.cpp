// sprsevw.cpp : implementation of the CSprsetwView class
//

#include "stdafx.h"
#include "sprsetw.h"
#include "sprsedoc.h"
#include "sprsevw.h"
#include "hostdlg.h"
#include "portnums.h"
#include "formdlg.h"
#include "pagedlg.h"
#include "userdlg.h"
#include "retndlg.h"
#include "retnabsdlg.h"
#include "secdlg22.h"
#include "sizelim.h"
#include "propts.h"
#include <ctype.h>
#include "loginhost.h"
#include "loginout.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#if	_MSC_VER == 700
#define UNIXTODOSTIME	((70UL * 365UL + 68/4 + 1) * 24UL * 3600UL)
#else
#define	UNIXTODOSTIME	0
#endif

#define	HOST_COL	0
#define	HOST_LEN	HOSTNSIZE
#define	ALIAS_COL	(HOST_COL+HOST_LEN+1)
#define	ALIAS_LEN	HOSTNSIZE
#define	INET_COL	(ALIAS_COL+ALIAS_LEN+1)
#define	INET_LEN	15
#define	PROBE_COL	(INET_COL+INET_LEN+1)
#define	PROBE_LEN	7
#define	SERVER_COL	(PROBE_COL+PROBE_LEN+1)
#define	SERVER_LEN	8
#define	TIMEOUT_COL	(SERVER_COL+SERVER_LEN)
#define	TIMEOUT_LEN	5
#define	ROW_WIDTH	(TIMEOUT_COL+TIMEOUT_LEN)

/////////////////////////////////////////////////////////////////////////////
// CSprsetwView

IMPLEMENT_DYNCREATE(CSprsetwView, CScrollView)

BEGIN_MESSAGE_MAP(CSprsetwView, CScrollView)
	//{{AFX_MSG_MAP(CSprsetwView)
	ON_COMMAND(ID_NETWORK_ADDNEWHOST, OnNetworkAddnewhost)
	ON_COMMAND(ID_NETWORK_DELETEHOST, OnNetworkDeletehost)
	ON_COMMAND(ID_NETWORK_CHANGEHOST, OnNetworkChangehost)
	ON_COMMAND(ID_NETWORK_SETASSERVER, OnNetworkSetasserver)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_OPTIONS_RESTOREDEFAULTS, OnOptionsRestoredefaults)
	ON_COMMAND(ID_OPTIONS_FORMANDCOPIES, OnOptionsFormandcopies)
	ON_COMMAND(ID_OPTIONS_PAGE, OnOptionsPage)
	ON_COMMAND(ID_OPTIONS_RETAIN, OnOptionsRetain)
	ON_COMMAND(ID_OPTIONS_SECURITY, OnOptionsSecurity)
	ON_COMMAND(ID_OPTIONS_USERANDMAIL, OnOptionsUserandmail)
	ON_WM_SIZE()
	ON_COMMAND(ID_OPTIONS_PROGRAMOPTIONS, OnOptionsProgramoptions)
	ON_COMMAND(ID_PROGRAM_PORTSETTINGS, OnProgramPortsettings)
	ON_COMMAND(ID_OPTIONS_SIZELIMIT, OnOptionsSizelimit)
	ON_COMMAND(ID_PROGRAM_LOGINORLOGOUT, OnProgramLoginorlogout)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_FORMANDCOPIES, OnUpdateOptionsFormandcopies)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_PAGE, OnUpdateOptionsPage)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_RETAIN, OnUpdateOptionsRetain)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SIZELIMIT, OnUpdateOptionsSizelimit)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_USERANDMAIL, OnUpdateOptionsUserandmail)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SECURITY, OnUpdateOptionsSecurity)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSprsetwView construction/destruction

CSprsetwView::CSprsetwView()
{
	m_nSelectedRow = -1;
}

CSprsetwView::~CSprsetwView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSprsetwView drawing

void CSprsetwView::UpdateScrollSizes()
{
	CClientDC dc(this);
	TEXTMETRIC tm;
	CRect rectClient;
	GetClientRect(&rectClient);
	dc.GetTextMetrics(&tm);
	m_nRowHeight = tm.tmHeight;
	m_nCharWidth = tm.tmAveCharWidth + 1;
	m_nRowWidth = ROW_WIDTH * m_nCharWidth;
	CSize sizePage(m_nRowWidth/5, max(m_nRowHeight, ((rectClient.bottom/m_nRowHeight)-1)*m_nRowHeight));
	CSize sizeLine(m_nRowWidth/20, m_nRowHeight);
	SetScrollSizes(MM_TEXT, CSize(m_nRowWidth, int(num_hosts()) * m_nRowHeight), sizePage, sizeLine);
}	

void CSprsetwView::OnUpdate(CView*v, LPARAM lHint, CObject* pHint)
{   
	Invalidate();
}

void CSprsetwView::OnDrawRow(CDC* pDC, int nRow, int y, BOOL bSelected)
{
	CBrush brushBackground;
	COLORREF crOldText = 0;
	COLORREF crOldBackground = 0;

	if  (bSelected) {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		crOldBackground = pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
		crOldText = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else  {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
		pDC->SetBkMode(TRANSPARENT);
	}
	
	CRect rectSelection;
	pDC->GetClipBox(&rectSelection);
	rectSelection.top = y;
	rectSelection.bottom = y + m_nRowHeight;
	pDC->FillRect(&rectSelection, &brushBackground);

	remote	*ch = get_nth(unsigned(nRow));

	if  (ch)  {	
		TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);                                 
		const  char	*name = ch->hostname();
		pDC->TextOut(HOST_COL*tm.tmAveCharWidth, y, name, strlen(name));
		if  (name = ch->aliasname())
			pDC->TextOut(ALIAS_COL*tm.tmAveCharWidth, y, name, strlen(name));
		in_addr sin;
		sin.s_addr = ch->hostid;
		char  FAR *na = inet_ntoa(sin);
		CString  outs;
		if  (na)
			outs = na;
		else
			outs.LoadString(IDS_UNKNOWN);
		pDC->TextOut(INET_COL*tm.tmAveCharWidth, y, outs);
		char	timeb[TIMEOUT_LEN + 4];
		wsprintf(timeb, "%5u", ch->ht_timeout);
		pDC->TextOut(TIMEOUT_COL*tm.tmAveCharWidth, y, timeb, strlen(timeb));
		if  (ch->ht_flags & HT_PROBEFIRST)  {
			outs.LoadString(IDS_PROBE);
			pDC->TextOut(PROBE_COL*tm.tmAveCharWidth, y, outs);
		}	
		if  (ch->ht_flags & HT_SERVER)  {
			outs.LoadString(IDS_SERVER);
			pDC->TextOut(SERVER_COL*tm.tmAveCharWidth, y, outs);
		}	
	}   

	// Restore the DC.
	if (bSelected)	{
		pDC->SetBkColor(crOldBackground);
		pDC->SetTextColor(crOldText);
	}
}

void CSprsetwView::OnDraw(CDC* pDC)
{
	if (num_hosts() == 0)
		return;
	int nFirstRow, nLastRow;
	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	nFirstRow = rectClip.top / m_nRowHeight;
	nLastRow = min(unsigned(rectClip.bottom / m_nRowHeight) + 1, num_hosts());
	int nRow, y;
	for (nRow = nFirstRow, y = m_nRowHeight * nFirstRow; nRow < nLastRow; nRow++, y += m_nRowHeight)
		OnDrawRow(pDC, nRow, y, nRow == m_nSelectedRow);
}

/////////////////////////////////////////////////////////////////////////////
// CSprsetwView diagnostics

#ifdef _DEBUG
void CSprsetwView::AssertValid() const
{
	CView::AssertValid();
}

void CSprsetwView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSprsetwDoc* CSprsetwView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSprsetwDoc)));
	return (CSprsetwDoc*) m_pDocument;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSprsetwView message handlers

void CSprsetwView::OnNetworkAddnewhost()
{
	CHostdlg	hdlg;
	hdlg.m_probefirst = TRUE;
	hdlg.m_timeout = NETTICKLE;
	if  (hdlg.DoModal() == IDOK)  {
		remote	*rp = new remote(hdlg.m_hid, hdlg.m_hostname, hdlg.m_aliasname, hdlg.m_probefirst? HT_PROBEFIRST: 0, hdlg.m_timeout);
		rp->addhost();
		Invalidate();
		((CSprsetwApp *) AfxGetApp())->m_needsave = TRUE;
	}	
}

void CSprsetwView::OnNetworkDeletehost()
{   
	if  (num_hosts() == 0)  {
		AfxMessageBox(IDP_NOHOSTSYET, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
	if  (GetActiveRow() < 0)  {
		AfxMessageBox(IDP_NOTSELECTED, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
		
	if  (AfxMessageBox(IDP_SUREDEL, MB_YESNO|MB_ICONQUESTION) == IDYES)  {
		remote	*rp = get_nth(unsigned(GetActiveRow()));
		rp->delhost();
		delete  rp;
		m_nSelectedRow = -1;
		Invalidate();
		((CSprsetwApp *) AfxGetApp())->m_needsave = TRUE;
	}
}		

void CSprsetwView::OnNetworkChangehost()
{
	if  (num_hosts() == 0)  {
		AfxMessageBox(IDP_NOHOSTSYET, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
	if  (GetActiveRow() < 0)  {
		AfxMessageBox(IDP_NOTSELECTED, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
	remote	*rp = get_nth(unsigned(GetActiveRow()));
	rp->delhost();
	CHostdlg	hdlg;
	unsigned  char	is_serv = (unsigned char) (rp->ht_flags & HT_SERVER);
	hdlg.m_probefirst = rp->ht_flags & HT_PROBEFIRST? TRUE: FALSE;
	hdlg.m_timeout = rp->ht_timeout;
	hdlg.m_hostname = rp->hostname();
	hdlg.m_aliasname = rp->aliasname();
	if  (hdlg.DoModal() == IDOK)  {
		remote	*nrp = new remote(hdlg.m_hid, hdlg.m_hostname, hdlg.m_aliasname, hdlg.m_probefirst? HT_PROBEFIRST|is_serv: is_serv, hdlg.m_timeout);
		nrp->addhost();
		delete  rp;
		((CSprsetwApp *) AfxGetApp())->m_needsave = TRUE;
	}
	else
		rp->addhost();
	Invalidate();
}

static	BOOL	dologin(CString unixh, const BOOL enqfirst)
{
	CSprsetwApp  &ma = *((CSprsetwApp *) AfxGetApp());
	int	ret;
	CString	newname;

	if  (enqfirst) {
		if  ((ret = xt_enquire(ma.m_winuser, ma.m_winmach, newname)) == 0)
			return  TRUE;
		if  (ret != IDP_XTENQ_PASSREQ)  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			return  FALSE;
		}
	}
	
	CLoginHost	dlg;
	dlg.m_unixhost = unixh;
	dlg.m_clienthost = ma.m_winmach;
	dlg.m_username = ma.m_winuser;
	int	cnt = 0;
	for  (;;)  {
		if  (dlg.DoModal() != IDOK)
			return  FALSE;
		if  ((ret = xt_login(dlg.m_username, ma.m_winmach, (const char *) dlg.m_passwd, newname)) == 0)
			break;
		if  (ret != IDP_XTENQ_BADPASSWD || cnt >= 2)  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			return  FALSE;
		}
		if  (AfxMessageBox(ret, MB_RETRYCANCEL|MB_ICONQUESTION) == IDCANCEL)
			return  FALSE;
		cnt++;
	}
	
	return  TRUE;
}

void CSprsetwView::OnNetworkSetasserver()
{
	if  (num_hosts() == 0)  {
		AfxMessageBox(IDP_NOHOSTSYET, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
	if  (GetActiveRow() < 0)  {
		AfxMessageBox(IDP_NOTSELECTED, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	netid_t	oldserver = Locparams.servid;
	remote	*newserv = get_nth(unsigned(GetActiveRow()));
	Locparams.servid = newserv->hostid;
    if  (initenqsocket(Locparams.servid) != 0)  {
    	AfxMessageBox(IDP_NOTALIVE, MB_OK|MB_ICONSTOP);
giveup:
		Locparams.servid = oldserver;
		initenqsocket(Locparams.servid);
		return;
	}
	int	ret;    
	spdet	newspu;
	CString	newname;
	CSprsetwApp  &ma = *((CSprsetwApp *) AfxGetApp());
	
	if  (!dologin(newserv->hostname(), TRUE))
		goto  giveup;

   	if  ((ret = getspuser(newspu, newname)) != 0)  {
		AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
		goto  giveup;
   	}

	ma.m_mypriv = newspu;
	ma.m_username = newname;
	ma.prune_class();
	unsigned  cnt = 0;
	remote	*rp;
	while  (rp = get_nth(cnt))  {
		rp->ht_flags &= ~HT_SERVER;
		cnt++;
	}		                                               
	newserv->ht_flags |= HT_SERVER;
	Invalidate();
	ma.m_needsave = TRUE;
	ma.m_isvalid = TRUE;
}

void CSprsetwView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);
	CRect rect(point, CSize(1,1));
	int nFirstRow;
	nFirstRow = rect.top / m_nRowHeight;
	if (unsigned(nFirstRow) < num_hosts())  {
		m_nSelectedRow = nFirstRow;              
		GetDocument()->UpdateAllViews(NULL);
	}	
}

void CSprsetwView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	OnLButtonDown(nFlags, point);
	OnNetworkChangehost();
}

void CSprsetwView::OnOptionsRestoredefaults()
{
	if  (AfxMessageBox(IDP_RESTOREOK, MB_YESNO|MB_ICONQUESTION) != IDYES)
		return;
	CSprsetwApp	*ma = (CSprsetwApp *) AfxGetApp();
	xtini	newxtini;
	if  (ma->m_options.spr_options.delimiter)
		delete [] ma->m_options.spr_options.delimiter; 
	ma->m_options = newxtini;
	ma->m_needsave = TRUE;
}

void CSprsetwView::OnOptionsFormandcopies()
{
	CSprsetwApp	*ma = (CSprsetwApp *) AfxGetApp();
	CFormdlg	fdlg;
	spq		&pars = ma->m_options.qparams;
	fdlg.m_formtype = pars.spq_form;
	fdlg.m_printer = pars.spq_ptr;
	fdlg.m_header = pars.spq_file;
	fdlg.m_copies = pars.spq_cps;
	fdlg.m_priority = pars.spq_pri;
	fdlg.m_supph = pars.spq_jflags & SPQ_NOH? TRUE: FALSE;
	spdet	&mypriv = ma->m_mypriv;
	fdlg.m_formok = mypriv.ispriv(PV_FORMS)? TRUE: FALSE;
	fdlg.m_ptrok = mypriv.ispriv(PV_OTHERP)? TRUE: FALSE;
	fdlg.m_minp = int(mypriv.spu_minp);
	fdlg.m_maxp = int(mypriv.spu_maxp);
	fdlg.m_maxcps = int(mypriv.ispriv(PV_ANYPRIO)? 255: mypriv.spu_cps);
	fdlg.m_allowform = mypriv.spu_formallow;
	fdlg.m_allowptr = mypriv.spu_ptrallow;
	if  (fdlg.m_formtype.IsEmpty())
		fdlg.m_formtype = mypriv.spu_form;
	if  (fdlg.m_printer.IsEmpty())
		fdlg.m_printer = mypriv.spu_ptr;
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
		ma->m_needsave = TRUE;
	}	
}

void CSprsetwView::OnOptionsPage()
{
	CSprsetwApp	*ma = (CSprsetwApp *) AfxGetApp();
	CPagedlg	pdlg;
	spq		&pars = ma->m_options.qparams;
	pages	&pgs = ma->m_options.spr_options.pfe;
	char  *&delimp = ma->m_options.spr_options.delimiter;
	pdlg.m_jflags = pars.spq_jflags;
	pdlg.m_startp = pars.spq_start;
	pdlg.m_endp = pars.spq_end;
	pdlg.m_deliml = unsigned(pgs.deliml);
	pdlg.m_delimiter = delimp;
	pdlg.m_delimnum = unsigned(pgs.delimnum); 
	pdlg.m_ppflags = pars.spq_flags;
	if  (pdlg.DoModal() == IDOK)  {
		pars.spq_jflags = pdlg.m_jflags;
		pars.spq_start = pdlg.m_startp;
		pars.spq_end = pdlg.m_endp;
		pgs.deliml = pdlg.m_deliml;
		pgs.delimnum = pdlg.m_delimnum;
		delimp = pdlg.m_delimiter;
		strncpy(pars.spq_flags, (const char FAR *) pdlg.m_ppflags, MAXFLAGS);
		ma->m_needsave = TRUE;
	}
}

void CSprsetwView::OnOptionsRetain()
{
	CSprsetwApp	*ma = (CSprsetwApp *) AfxGetApp();
	spq		&pars = ma->m_options.qparams;
	if  (ma->m_options.spq_options.abshold)  {
		CRetnabsdlg	rdlg;
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
			ma->m_needsave = TRUE;
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
			ma->m_needsave = TRUE;
		}
	}
}

void CSprsetwView::OnOptionsSecurity()
{
	CSprsetwApp	*ma = (CSprsetwApp *) AfxGetApp();
	CSecdlg22 sdlg;
	spq		&pars = ma->m_options.qparams;
	sdlg.m_classc = pars.spq_class;
	spdet	&mypriv = ((CSprsetwApp *)AfxGetApp())->m_mypriv;
	sdlg.m_maxclass = mypriv.spu_class;
	sdlg.m_mayoverride = mypriv.ispriv(PV_COVER);	
	sdlg.m_localonly = pars.spq_jflags & SPQ_LOCALONLY? TRUE: FALSE;
	if  (sdlg.DoModal() == IDOK)  {
		pars.spq_class = sdlg.m_classc;
		if  (sdlg.m_localonly)
			pars.spq_jflags |= SPQ_LOCALONLY;
		else
			pars.spq_jflags &= ~SPQ_LOCALONLY;
		ma->m_needsave = TRUE;
	}
}

void CSprsetwView::OnOptionsUserandmail()
{
	CSprsetwApp	*ma = (CSprsetwApp *) AfxGetApp();
	CUserdlg	udlg;
	spq		&pars = ma->m_options.qparams;
	udlg.m_mail = pars.spq_jflags & SPQ_MAIL? TRUE: FALSE;
	udlg.m_write = pars.spq_jflags & SPQ_WRT? TRUE: FALSE;
	udlg.m_mattn = pars.spq_jflags & SPQ_MATTN? TRUE: FALSE;
	udlg.m_wattn = pars.spq_jflags & SPQ_WATTN? TRUE: FALSE;
	udlg.m_postuser = strcmp(pars.spq_uname, pars.spq_puname) == 0? "" : pars.spq_puname;
	if  (udlg.DoModal() == IDOK)  { 
		if  (udlg.m_postuser.IsEmpty())
			strcpy(pars.spq_puname, pars.spq_uname);
		else
			strcpy(pars.spq_puname, (const char FAR *) udlg.m_postuser);
		pars.spq_jflags &= ~(SPQ_MAIL|SPQ_WRT|SPQ_MATTN|SPQ_WATTN);
		if  (udlg.m_mail)
			pars.spq_jflags |= SPQ_MAIL;
		if  (udlg.m_write)
			pars.spq_jflags |= SPQ_WRT;
		if  (udlg.m_mattn)
			pars.spq_jflags |= SPQ_MATTN;
		if  (udlg.m_wattn)
			pars.spq_jflags |= SPQ_WATTN;		
		ma->m_needsave = TRUE;
	}	
}

void CSprsetwView::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);
	UpdateScrollSizes();
}

void CSprsetwView::OnOptionsProgramoptions()
{
	CPropts	dlg;
	CSprsetwApp	*ma = (CSprsetwApp *) AfxGetApp();
	spropts	&mo = ma->m_options.spr_options;
	dlg.m_verbose = mo.verbose;
	dlg.m_interpolate = mo.interpolate;
	dlg.m_textmode = mo.textmode;
	if  (dlg.DoModal() == IDOK)  {
		mo.verbose = dlg.m_verbose;
		mo.interpolate = dlg.m_interpolate;
		mo.textmode = dlg.m_textmode;
		ma->m_needsave = TRUE;
	}		
}

void CSprsetwView::OnProgramPortsettings()
{
	CPortnums	dlg;
	dlg.DoModal();
}

void CSprsetwView::OnOptionsSizelimit()
{
	CSprsetwApp	*ma = (CSprsetwApp *) AfxGetApp();
	spq		&pars = ma->m_options.qparams;
	CSizelim	dlg;
	dlg.m_limittype = pars.spq_dflags & SPQ_PGLIMIT? 1: 0;
	dlg.m_errlimit = (pars.spq_dflags & SPQ_ERRLIMIT) != 0;
	dlg.m_limit = pars.spq_pglim;
    if  (dlg.DoModal() == IDOK)  {
    	pars.spq_dflags &= ~(SPQ_PGLIMIT|SPQ_ERRLIMIT);
    	if  (pars.spq_pglim = dlg.m_limit)  {
    		if  (dlg.m_limittype > 0)
    			pars.spq_dflags |= SPQ_PGLIMIT;
    		if  (dlg.m_errlimit)
    			pars.spq_dflags |= SPQ_ERRLIMIT;
    	}
		ma->m_needsave = TRUE;
	}		
}

void CSprsetwView::OnProgramLoginorlogout() 
{
	CLoginout	lodlg;
	CSprsetwApp	&ma = *((CSprsetwApp *) AfxGetApp());
	lodlg.m_winuser = ma.m_winuser;

	if  (ma.m_isvalid)
		lodlg.m_unixuser = ma.m_username;
	else
		lodlg.m_unixuser.LoadString(IDS_NOTLOGGED);

	int	ret = lodlg.DoModal();

	if  (ret == IDC_LOGOUT)  {
		if  ((ret = xt_logout()) != 0)
			AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
		ma.m_isvalid = FALSE;
		return;
	}

	if  (ret != IDC_LOGIN)
		return;

	if  (dologin(look_host(Locparams.servid), FALSE))  {
		spdet	newspu;
		CString	newname;
	   	if  ((ret = getspuser(newspu, newname)) != 0)  {
			AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			ma.m_isvalid = FALSE;
			return;
		}
		ma.m_mypriv = newspu;
		ma.m_username = newname;
		ma.prune_class();
		ma.m_isvalid = TRUE;
	}
	else
		ma.m_isvalid = FALSE;
}

void CSprsetwView::OnUpdateOptionsFormandcopies(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CSprsetwApp *)AfxGetApp())->appvalid());
}

void CSprsetwView::OnUpdateOptionsPage(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CSprsetwApp *)AfxGetApp())->appvalid());
}

void CSprsetwView::OnUpdateOptionsRetain(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CSprsetwApp *)AfxGetApp())->appvalid());
}

void CSprsetwView::OnUpdateOptionsSizelimit(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CSprsetwApp *)AfxGetApp())->appvalid());
}

void CSprsetwView::OnUpdateOptionsUserandmail(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CSprsetwApp *)AfxGetApp())->appvalid());
}

void CSprsetwView::OnUpdateOptionsSecurity(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CSprsetwApp *)AfxGetApp())->appvalid());
}
