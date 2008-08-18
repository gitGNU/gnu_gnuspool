// sprsedoc.cpp : implementation of the CSprservDoc class
//

#include "stdafx.h"
#include "pages.h"
#include "monfile.h"
#include "xtini.h"
#include "sprserv.h"
#include "sprsedoc.h"
#include "sprsevw.h"
#include "mfdlg.h"
#include "dfdlg.h"
#include "formdlg.h"
#include "pagedlg.h"
#include "retndlg.h"
#include "retnabsdlg.h"
#include "secdlg22.h"
#include "userdlg.h"
#include "sizelim.h"
#include <limits.h>
#include <string.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#if	_MSC_VER == 700
#define UNIXTODOSTIME	((70UL * 365UL + 68/4 + 1) * 24UL * 3600UL)
#else
#define	UNIXTODOSTIME	0
#endif

/////////////////////////////////////////////////////////////////////////////
// CSprservDoc

IMPLEMENT_DYNCREATE(CSprservDoc, CDocument)

BEGIN_MESSAGE_MAP(CSprservDoc, CDocument)
	//{{AFX_MSG_MAP(CSprservDoc)
	ON_COMMAND(ID_LIST_ADDNEWFILE, OnListAddnewfile)
	ON_COMMAND(ID_LIST_CHANGEFILENAME, OnListChangefilename)
	ON_COMMAND(ID_LIST_DELETEFILEFROMLIST, OnListDeletefilefromlist)
	ON_COMMAND(ID_OPTIONS_FORMHEADERCOPIES, OnOptionsFormheadercopies)
	ON_COMMAND(ID_OPTIONS_PAGE, OnOptionsPage)
	ON_COMMAND(ID_OPTIONS_RESTOREDEFAULTS, OnOptionsRestoredefaults)
	ON_COMMAND(ID_OPTIONS_RETAIN, OnOptionsRetain)
	ON_COMMAND(ID_OPTIONS_SECURITY, OnOptionsSecurity)
	ON_COMMAND(ID_OPTIONS_USERANDMAIL, OnOptionsUserandmail)
	ON_COMMAND(ID_LIST_OK, OnListOk)
	ON_COMMAND(ID_LIST_WRITETOWININI, OnListWritetowinini)
	ON_COMMAND(ID_OPTIONS_SIZELIMIT, OnOptionsSizelimit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSprservDoc construction/destruction

CSprservDoc::CSprservDoc()
{
	m_timerset = FALSE;
}

CSprservDoc::~CSprservDoc()
{
}

BOOL CSprservDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSprservDoc serialization

void CSprservDoc::Serialize(CArchive& ar)
{
	m_flist.Serialize(ar);
	if  (ar.IsLoading())
		reschedule();
}


/////////////////////////////////////////////////////////////////////////////
// CSprservDoc diagnostics

#ifdef _DEBUG
void CSprservDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSprservDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSprservDoc commands

static	CString	dircat(CString inp, int defdrive, char *defdir)
{
	CString	result;
	
	if  (inp[1] != ':')  {
		result += char(defdrive + 'A' - 1);
		result += ':';
		if  (inp[0] != '\\')  {
			result += defdir;            
			result += '\\';
		}
		result += inp;
	}
	else  if  (inp[2] != '\\')  {
		result += inp[0];
		result += ':';
		result += '\\';
		result += inp.Right(inp.GetLength() - 2);
	}
	else
		result = inp;
	result.MakeUpper();
	return  result;
}			

void CSprservDoc::OnListAddnewfile()
{
	CMFDlg	mdlg;
	CSprservApp *ma = ((CSprservApp *)AfxGetApp());
	char	fullname[_MAX_PATH];
	wsprintf(fullname, "%c:%s\\", ma->m_defdrive + 'A' - 1, (char *) ma->m_defdir);
	mdlg.m_filename = fullname;
	mdlg.m_filename.MakeLower();
	mdlg.m_polltime = MON_INTERVAL; 
	mdlg.m_notyet = TRUE;
	if  (mdlg.DoModal() == IDOK)  {
		CString	resf = dircat(mdlg.m_filename, ma->m_defdrive, ma->m_defdir);
		CMonFile  *newf = new CMonFile((const char *)resf, ma->m_options.qparams, ma->m_options.spr_options.pfe, ma->m_options.spr_options.delimiter, mdlg.m_polltime);
		if  (!mdlg.m_notyet)
			newf->setok(TRUE);
		m_flist.addfile(newf);
		UpdateAllViews(NULL);
		SetModifiedFlag(TRUE);
		reschedule();
	}	
}

// Similarly for dropped files.

extern	void	parsecmdline(const char FAR *);

void	CSprservDoc::NewDropped(const char *fname)
{
	const  char	*sp = strrchr(fname, '.');
	if  (sp)  {
		sp++;
		if  (_stricmp(sp, "xtc") == 0  ||  _stricmp(sp, "xtj") == 0  ||  _stricmp(sp, "bat") == 0)  {
			parsecmdline(fname);                                                               
			return;
		}
	}
//	CDFDlg	mdlg;
	CSprservApp *ma = ((CSprservApp *)AfxGetApp());
//	mdlg.m_filename = fname;
//	mdlg.m_polltime = MON_INTERVAL; 
//	mdlg.m_notyet = TRUE;
//	mdlg.m_options = ma->m_options;
//	if  (mdlg.DoModal() == IDOK)  {
//		CString	resf = dircat(mdlg.m_filename, ma->m_defdrive, ma->m_defdir);
	CString resf = dircat(fname, ma->m_defdrive, ma->m_defdir);
//		CMonFile  *newf = new CMonFile((const char *)resf, mdlg.m_options.qparams, mdlg.m_options.spr_options.pfe, mdlg.m_options.spr_options.delimiter, mdlg.m_polltime);
	CMonFile *newf = new CMonFile((const char *) resf, ma->m_options.qparams, ma->m_options.spr_options.pfe, ma->m_options.spr_options.delimiter, MON_INTERVAL);
//		if  (!mdlg.m_notyet)
//			newf->setok(TRUE);
		m_flist.addfile(newf);
		UpdateAllViews(NULL);
		SetModifiedFlag(TRUE);
//	}	
}
	
int 	CSprservDoc::GetSelectedFile()
{                 
	POSITION	p = GetFirstViewPosition();
	CSprservView	*aview = (CSprservView *)GetNextView(p);
	if  (aview)  {
		int	 cr = aview->GetActiveRow();
		if  (cr >= 0)
			return  cr;
	}		
	AfxMessageBox(IDP_NOFILESELECTED, MB_OK|MB_ICONEXCLAMATION);
	return  -1;
}	

void CSprservDoc::OnListChangefilename()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CMonFile	*currf = m_flist[curr];
	CMFDlg	mdlg;
	mdlg.m_filename = currf->get_file();
	mdlg.m_filename.MakeLower();
	mdlg.m_polltime = currf->get_time();
	mdlg.m_notyet = !currf->isok();
	if  (mdlg.DoModal() == IDOK)  {
		CSprservApp *ma = ((CSprservApp *)AfxGetApp());
		CString	resf = dircat(mdlg.m_filename, ma->m_defdrive, ma->m_defdir);
		CMonFile  *newf = new CMonFile((const char *) resf, currf, mdlg.m_polltime);
		newf->setok(!mdlg.m_notyet);
		m_flist.repfile(unsigned(curr), newf);
		UpdateAllViews(NULL);
		SetModifiedFlag(TRUE);
		reschedule();
	}	
}
		
void CSprservDoc::OnListOk()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CMonFile	*currf = m_flist[curr];
	currf->setok(!currf->isok());
	UpdateAllViews(NULL);
	SetModifiedFlag(TRUE);
	if  (currf->isok())
		reschedule();
}

void CSprservDoc::OnListDeletefilefromlist()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	m_flist.delfile(unsigned(curr));
	UpdateAllViews(NULL);
	SetModifiedFlag(TRUE);
}

void CSprservDoc::OnOptionsFormheadercopies()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CMonFile	*currf = m_flist[curr];
	spq		&pars = currf->qpar();
	CFormdlg	fdlg;
	fdlg.m_formtype = pars.spq_form;
	fdlg.m_printer = pars.spq_ptr;
	fdlg.m_header = pars.spq_file;
	fdlg.m_copies = pars.spq_cps;
	fdlg.m_priority = pars.spq_pri;
	fdlg.m_supph = pars.spq_jflags & SPQ_NOH? TRUE: FALSE;
	spdet	&mypriv = ((CSprservApp *) AfxGetApp())->m_mypriv;
	fdlg.m_formok = mypriv.ispriv(PV_FORMS)? TRUE: FALSE;
	fdlg.m_ptrok = mypriv.ispriv(PV_OTHERP)? TRUE: FALSE;
	fdlg.m_minp = int(mypriv.spu_minp);
	fdlg.m_maxp = int(mypriv.spu_maxp);
	fdlg.m_maxcps = int(mypriv.ispriv(PV_ANYPRIO)? 255: mypriv.spu_cps);
	fdlg.m_allowform = mypriv.spu_formallow;
	fdlg.m_allowptr = mypriv.spu_ptrallow;
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
		UpdateAllViews(NULL);
		SetModifiedFlag(TRUE);
	}	
}

void CSprservDoc::OnOptionsPage()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CPagedlg	pdlg;
	CMonFile	*currf = m_flist[curr];
	spq		&pars = currf->qpar();
	pages	&pgs = currf->pgs();
	pdlg.m_jflags = pars.spq_jflags;
	pdlg.m_startp = pars.spq_start;
	pdlg.m_endp = pars.spq_end;
	pdlg.m_deliml = unsigned(pgs.deliml);
	pdlg.m_delimiter = currf->delim();
	pdlg.m_delimnum = unsigned(pgs.delimnum); 
	pdlg.m_ppflags = pars.spq_flags;
	if  (pdlg.DoModal() == IDOK)  {
		pars.spq_jflags = pdlg.m_jflags;
		pars.spq_start = pdlg.m_startp;
		pars.spq_end = pdlg.m_endp;
		pgs.deliml = pdlg.m_deliml;
		pgs.delimnum = pdlg.m_delimnum;
		currf->delimset(pdlg.m_delimiter);
		strncpy(pars.spq_flags, (const char *) pdlg.m_ppflags, MAXFLAGS);
		SetModifiedFlag(TRUE);
	}
}		

void CSprservDoc::OnOptionsRestoredefaults()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CSprservApp *ma = ((CSprservApp *)AfxGetApp());
	CMonFile	*currf = m_flist[curr];
	spq		&pars = currf->qpar();
	pages	&pgs = currf->pgs();
	pars = ma->m_options.qparams;
	pgs = ma->m_options.spr_options.pfe;
	char  *newdelim = new char [pgs.deliml];
	memcpy((void *) newdelim, ma->m_options.spr_options.delimiter, unsigned(pgs.deliml));
	currf->delimset(newdelim);
	UpdateAllViews(NULL);
	SetModifiedFlag(TRUE);
}

void CSprservDoc::OnOptionsRetain()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CMonFile	*currf = m_flist[curr];
	spq		&pars = currf->qpar();
	if  (((CSprservApp *)AfxGetApp())->m_options.spq_options.abshold)  {
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
			SetModifiedFlag(TRUE);
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
			SetModifiedFlag(TRUE);
		}
	}
}

void CSprservDoc::OnOptionsSecurity()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CMonFile	*currf = m_flist[curr];
	spq		&pars = currf->qpar();
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
		SetModifiedFlag(TRUE);
	}
}

void CSprservDoc::OnOptionsUserandmail()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CMonFile	*currf = m_flist[curr];
	spq		&pars = currf->qpar();
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
		SetModifiedFlag(TRUE);
	}
}

void	CSprservDoc::OnTimer(UINT nIDEvent)
{
	CSprservApp *ma = (CSprservApp *)AfxGetApp();
	if  (!ma  ||  ma->m_shutdown)
		return;
	CMonFile	*w = m_flist[nIDEvent];
	if  (w)  {
		CFile	f;
		unsigned  ntime = w->nexttime(f);
		if  (ntime == 0)  {
			UINT  ret = w->queuejob(ma->m_username, f);
			if  (ret != 0)  {
				AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
				w->setok(FALSE);
				UpdateAllViews(NULL);
			}
		}		
	}
	m_timerset = FALSE;
	reschedule();
}

void	CSprservDoc::reschedule()
{
	if  (m_timerset)
		return;

	if  (!refreshconn())
		return;

restart:
	m_flist.setfirst();
	unsigned  which = 0, minw = UINT_MAX, mint = Locparams.servtimeout*1000;
	CMonFile	*w;
	while  (w = m_flist.next())  {
		CFile	f;
		unsigned  wtime = w->nexttime(f);
	    if  (wtime == 0)  {
	    	UINT  ret = w->queuejob(((CSprservApp *)AfxGetApp())->m_username, f);
			if  (ret != 0)  {
				AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
				w->setok(FALSE);     
				UpdateAllViews(NULL);
			}
	    	goto  restart;
	    }
	    if  (wtime < mint)  {
	    	mint = wtime;
		   	minw = which;
		}
		which++;
	}
	POSITION	p = GetFirstViewPosition();
	CSprservView	*aview = (CSprservView *)GetNextView(p);
	if  (aview)  {
		aview->STimer(minw, mint);
		m_timerset = TRUE;
	}
}	

void CSprservDoc::OnListWritetowinini()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	::WriteProfileString("ports", m_flist[curr]->get_file(), "");
}

void CSprservDoc::OnOptionsSizelimit()
{
	int	 curr = GetSelectedFile();
	if  (curr < 0)
		return;
	CMonFile	*currf = m_flist[curr];
	spq		&pars = currf->qpar();
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
		SetModifiedFlag(TRUE);
	}	
}
