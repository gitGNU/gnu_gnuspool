// sprsetw.cpp : Defines the class behaviors for the application.
//

#pragma comment (exestr, "@(#) $Revision: 1.1 $")

#include "stdafx.h"
#include "sprsetw.h"
#include "files.h"
#include "mainfrm.h"
#include "sprsedoc.h"
#include "sprsevw.h"
#include <direct.h>
#include <ctype.h>
#include <limits.h>
#include "getregdata.h"
#include "loginhost.h"

#define	LOTSANDLOTS	99999999L

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern	UINT		winsockstart();
extern	void		winsockend();
extern	char		*translate_delim(const char *, unsigned &);
extern	char		*untranslate_delim(const char *, const unsigned);

/////////////////////////////////////////////////////////////////////////////
// CSprsetwApp

BEGIN_MESSAGE_MAP(CSprsetwApp, CWinApp)
	//{{AFX_MSG_MAP(CSprsetwApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSprsetwApp construction

CSprsetwApp::CSprsetwApp()
{
	m_needsave = FALSE;
	m_isvalid = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSprsetwApp object

CSprsetwApp NEAR theApp;

const  char poptname[] = "Program options";
const  char	qdefname[] = "Queue defaults";
const  char	portsname[] = "UDP Ports";

char  FAR	basedir[_MAX_PATH];

/////////////////////////////////////////////////////////////////////////////
// CSprsetwApp initialization

BOOL CSprsetwApp::InitInstance()
{
	_getcwd(basedir, sizeof(basedir));
	strcat(basedir, "\\");

	if  (!getenv("TZ"))
			_putenv(DEFAULT_TZ);
	_tzset();
	LPSTR  cmdline = m_lpCmdLine;
	while  (*cmdline  &&  *cmdline != '/')
		cmdline++;
	if  (stricmp(cmdline, "/noulist") == 0)
		m_noulist = TRUE;
	SetDialogBkColor(RGB(255,255,255));

	// Register the application's document templates.  Document templates
	// serve as the connection between documents, frame windows and views.

	AddDocTemplate(new CSingleDocTemplate(IDR_MAINFRAME,
			RUNTIME_CLASS(CSprsetwDoc),
			RUNTIME_CLASS(CMainFrame),     // main SDI frame window
			RUNTIME_CLASS(CSprsetwView)));

	//  We only allow one of these things to happen at once.

	if  (m_hPrevInstance != NULL)  {
		AfxMessageBox(IDP_PREVINSTANCE, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}

	load_options();
	
	//  If we cannot initialise sockets, give up now.

	int  ret = winsockstart();
    if  (ret != 0)  {
		if  (ret == IDP_NO_HOSTFILE)  {
			if  (AfxMessageBox(ret, MB_ICONWARNING|MB_YESNO) != IDYES)
				return  FALSE;
			m_isvalid = FALSE;
		}
		else  if  (ret == IDP_NO_SERVER)  {
    		AfxMessageBox(ret, MB_OK|MB_ICONWARNING);
			m_isvalid = FALSE;
		}
		else  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			m_isvalid = FALSE;
		}
    }

    if  (m_isvalid && (ret = initenqsocket(Locparams.servid)) != 0)  {
    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}
    
	//  New logic - first run enquiry.
	//  If that works, run getspuser to get details etc.
	//  If enquiry doesn't work, then ask for password

#ifdef	REGSTRING
	m_winuser = GetRegString("Network\\Logon\\username");
	m_winmach = GetRegString("System\\CurrentControlSet\\Services\\VxD\\VNETSUP\\ComputerName");
#else
	GetUserAndComputerNames(m_winuser, m_winmach);
#endif
	if  (!m_isvalid)
		goto  nogood;

	if  ((ret = xt_enquire(m_winuser, m_winmach, m_username)) != 0)  {
		if  (ret != IDP_XTENQ_PASSREQ)  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			m_isvalid = FALSE;
			goto  madeit;            
		}
		CLoginHost	dlg;
		dlg.m_unixhost = look_host(Locparams.servid);
		dlg.m_clienthost = m_winmach;
		dlg.m_username = m_winuser;
		int	cnt = 0;
		for  (;;)  {
			if  (dlg.DoModal() != IDOK)  {
				m_isvalid = FALSE;
				goto  madeit;
			}
			if  ((ret = xt_login(dlg.m_username, m_winmach, (const char *) dlg.m_passwd, m_username)) == 0)
				break;
			if  (ret != IDP_XTENQ_BADPASSWD || cnt >= 2)  {
		    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
				m_isvalid = FALSE;
				goto  madeit;
			}
			if  (AfxMessageBox(ret, MB_RETRYCANCEL|MB_ICONQUESTION) == IDCANCEL)  {
				m_isvalid = FALSE;
				goto  madeit;
			}
			cnt++;
		}
	}

	//  And now do the business...

	if  ((ret = getspuser(m_mypriv, m_username)) != 0)  {
		AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
		m_isvalid = FALSE;
   	}

madeit:
	if  (m_isvalid)
   		prune_class();
nogood:
	OnFileNew();
	return TRUE;
}

void	CSprsetwApp::prune_class()
{
   	if  (!m_mypriv.ispriv(PV_COVER))
   		 m_options.qparams.spq_class &= m_mypriv.spu_class;
	if  (m_options.qparams.spq_class == 0)
   		 m_options.qparams.spq_class = m_mypriv.spu_class;	
	if  (m_options.qparams.spq_pri == 0  ||
		 m_options.qparams.spq_pri < m_mypriv.spu_minp  ||
		 m_options.qparams.spq_pri > m_mypriv.spu_maxp)
		m_options.qparams.spq_pri = m_mypriv.spu_defp;
	if  (m_options.qparams.spq_form[0] == '\0')
		strcpy(m_options.qparams.spq_form, m_mypriv.spu_form);
	if  (m_options.qparams.spq_ptr[0] == '\0')
		strcpy(m_options.qparams.spq_ptr, m_mypriv.spu_ptr);
	if  (m_options.qparams.spq_puname[0] == '\0')
		strcpy(m_options.qparams.spq_puname, m_username);
}

int	CSprsetwApp::ExitInstance()
{
	winsockend();
	return  0;
}
	
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CSprsetwApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void	CSprsetwApp::load_options()
{
	spropts	&opt_spr = m_options.spr_options;
	spq		&qpopt = m_options.qparams;

	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, INIFILE);

	opt_spr.verbose =
		(unsigned char) ::GetPrivateProfileInt(poptname, "Verbose", 0, pfilepath);
	opt_spr.interpolate =
		(unsigned char ) ::GetPrivateProfileInt(poptname, "Interpolate", 0, pfilepath);
	opt_spr.textmode =
		(unsigned char ) ::GetPrivateProfileInt(poptname, "Textmode", 0, pfilepath);
	opt_spr.pfe.delimnum =
		(unsigned char ) ::GetPrivateProfileInt(qdefname, "Delimnum", 1, pfilepath);
	m_options.spq_options.abshold =
		(unsigned char ) ::GetPrivateProfileInt(poptname, "Abshold", 0, pfilepath);
	char	cbuf[100];
	::GetPrivateProfileString(qdefname, "Delimiter", "\\f", cbuf, sizeof(cbuf), pfilepath);
	unsigned  deliml;
	char	*newdelim = translate_delim(cbuf, deliml);
	if  (newdelim)  {
		delete opt_spr.delimiter;
		opt_spr.delimiter = newdelim;
		opt_spr.pfe.deliml = deliml;
	}
	
	//  Get straight job parameters

	qpopt.spq_jflags = 0;
	qpopt.spq_dflags = 0;
	
	if  (::GetPrivateProfileInt(qdefname, "Nohdr", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_NOH;
	if  (::GetPrivateProfileInt(qdefname, "Retain", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_RETN;
	if  (::GetPrivateProfileInt(qdefname, "Localonly", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_LOCALONLY;
	if  (::GetPrivateProfileInt(qdefname, "Write", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_WRT;
	if  (::GetPrivateProfileInt(qdefname, "Mail", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_MAIL;
	if  (::GetPrivateProfileInt(qdefname, "Wattn", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_WATTN;
	if  (::GetPrivateProfileInt(qdefname, "Mattn", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_MATTN;
	if  (::GetPrivateProfileInt(qdefname, "Skipodd", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_ODDP;
	if  (::GetPrivateProfileInt(qdefname, "Skipeven", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_EVENP;
	if  (::GetPrivateProfileInt(qdefname, "Swapoe", 0, pfilepath))
		qpopt.spq_jflags |= SPQ_REVOE;

	qpopt.spq_pglim = (unsigned short) ::GetPrivateProfileInt(qdefname, "Sizelim", 0, pfilepath);
	if  (::GetPrivateProfileInt(qdefname, "Pglim", 0, pfilepath))
		qpopt.spq_dflags |= SPQ_PGLIMIT;
	if  (::GetPrivateProfileInt(qdefname, "Errlim", 0, pfilepath))
		qpopt.spq_dflags |= SPQ_ERRLIMIT;
		
	qpopt.spq_cps =
		(unsigned char) ::GetPrivateProfileInt(qdefname, "Copies", 1, pfilepath);
	
	// Set priority to invalid value (0) if not defined here
	// so we can fix it later
	
	qpopt.spq_pri =
		(unsigned char) ::GetPrivateProfileInt(qdefname, "Priority", 0, pfilepath);
	
	::GetPrivateProfileString(qdefname, "Qclass", "0xFFFFFFFF", cbuf, sizeof(cbuf), pfilepath);
	qpopt.spq_class = (unsigned long) strtoul(cbuf, (char **) 0, 0);
                                                  
	//  Ditto-ish for form
	                                                  
	::GetPrivateProfileString(qdefname, "Form", "", cbuf, sizeof(cbuf), pfilepath);
	strncpy(qpopt.spq_form, cbuf, MAXFORM);
	::GetPrivateProfileString(qdefname, "Printer", "", cbuf, sizeof(cbuf), pfilepath);
	strncpy(qpopt.spq_ptr, cbuf, JPTRNAMESIZE);
	
	//  Ditto-ish for user name

	::GetPrivateProfileString(qdefname, "Puser", "", cbuf, sizeof(cbuf), pfilepath);
	strncpy(qpopt.spq_puname, cbuf, UIDSIZE);
	::GetPrivateProfileString(qdefname, "Title", "", cbuf, sizeof(cbuf), pfilepath);
	strncpy(qpopt.spq_file, cbuf, MAXTITLE);

	::GetPrivateProfileString(qdefname, "Spage", "1", cbuf, sizeof(cbuf), pfilepath);
	qpopt.spq_start = atol(cbuf) - 1L;
	::GetPrivateProfileString(qdefname, "Epage", "", cbuf, sizeof(cbuf), pfilepath);
	if  (isdigit(cbuf[0]))
		qpopt.spq_end = atol(cbuf) - 1L;
	else
		qpopt.spq_end = LONG_MAX - 1L;
		
	::GetPrivateProfileString(qdefname, "PPflags", "", cbuf, sizeof(cbuf), pfilepath);
	strncpy(qpopt.spq_flags, cbuf, MAXFLAGS);
	
	qpopt.spq_ptimeout =
		(unsigned short) ::GetPrivateProfileInt(qdefname, "Pdeltime", 24, pfilepath);			
	qpopt.spq_nptimeout =
		(unsigned short) ::GetPrivateProfileInt(qdefname, "NPdeltime", 7*24, pfilepath);			

	::GetPrivateProfileString(qdefname, "Holdtime", "0", cbuf, sizeof(cbuf), pfilepath);
	time_t	ntime = atol(cbuf);
	if  (ntime > 0  &&  m_options.spq_options.abshold)
		ntime += time(NULL);
	qpopt.spq_hold = ntime;		

	//  Do port number.
	//  Read defaults from services file if available.
	
	Locparams.uaportnum = htons(::GetPrivateProfileInt(portsname, "Printout", DEF_SERVPORTNUM, pfilepath));
}

inline	void	WritePrivateProfileBool(const char *section,
								  	    const char *field,
								  	    const unsigned long value,
								  	    const char *pfilename)
{
	::WritePrivateProfileString(section, field, value? "1":"0", pfilename);
}

inline	void	WritePrivateProfileInt(const char *section,
								  	   const char *field,
								  	   unsigned long value,
								  	   const char *pfilename)
{
	char	dbuf[20];
	wsprintf(dbuf, "%lu", value);
	::WritePrivateProfileString(section, field, dbuf, pfilename);
}

inline	void	WritePrivateProfileHex(const char *section,
								  	   const char *field,
								  	   unsigned long value,
								  	   const char *pfilename)
{
	char	dbuf[20];
	wsprintf(dbuf, "0x%lx", value);
	::WritePrivateProfileString(section, field, dbuf, pfilename);
}

void	CSprsetwApp::save_options()
{
	spropts	&opt_spr = m_options.spr_options;
	spq		&qpopt = m_options.qparams;

	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, INIFILE);

	WritePrivateProfileInt(poptname, "Verbose", opt_spr.verbose, pfilepath);
	WritePrivateProfileInt(poptname, "Interpolate", opt_spr.interpolate, pfilepath);
	WritePrivateProfileInt(poptname, "Textmode", opt_spr.textmode, pfilepath);

	//  Get straight job parameters

	WritePrivateProfileInt(qdefname, "Delimnum", opt_spr.pfe.delimnum, pfilepath);
	char	*newdelim = untranslate_delim(opt_spr.delimiter, unsigned(opt_spr.pfe.deliml));
	::WritePrivateProfileString(qdefname, "Delimiter", newdelim, pfilepath);
	delete [] newdelim;
	
	WritePrivateProfileBool(qdefname, "Nohdr", qpopt.spq_jflags & SPQ_NOH, pfilepath);
	WritePrivateProfileBool(qdefname, "Retain", qpopt.spq_jflags & SPQ_RETN, pfilepath);
	WritePrivateProfileBool(qdefname, "Localonly", qpopt.spq_jflags & SPQ_LOCALONLY, pfilepath);
	WritePrivateProfileBool(qdefname, "Write", qpopt.spq_jflags & SPQ_WRT, pfilepath);
	WritePrivateProfileBool(qdefname, "Mail", qpopt.spq_jflags & SPQ_MAIL, pfilepath);
	WritePrivateProfileBool(qdefname, "Wattn", qpopt.spq_jflags & SPQ_WATTN, pfilepath);
	WritePrivateProfileBool(qdefname, "Mattn", qpopt.spq_jflags & SPQ_MATTN, pfilepath);
	WritePrivateProfileBool(qdefname, "Skipodd", qpopt.spq_jflags & SPQ_ODDP, pfilepath);
	WritePrivateProfileBool(qdefname, "Skipeven", qpopt.spq_jflags & SPQ_EVENP, pfilepath);
	WritePrivateProfileBool(qdefname, "Swapoe", qpopt.spq_jflags & SPQ_REVOE, pfilepath);

	WritePrivateProfileInt(qdefname, "Sizelim", qpopt.spq_pglim, pfilepath);
	WritePrivateProfileBool(qdefname, "Pglim", qpopt.spq_dflags & SPQ_PGLIMIT, pfilepath);
	WritePrivateProfileBool(qdefname, "Errlim", qpopt.spq_dflags & SPQ_ERRLIMIT, pfilepath);

	WritePrivateProfileInt(qdefname, "Copies", qpopt.spq_cps, pfilepath);
	WritePrivateProfileInt(qdefname, "Priority", qpopt.spq_pri, pfilepath);
	WritePrivateProfileHex(qdefname, "Qclass", qpopt.spq_class, pfilepath);
	::WritePrivateProfileString(qdefname, "Form", qpopt.spq_form, pfilepath);
	::WritePrivateProfileString(qdefname, "Printer", qpopt.spq_ptr, pfilepath);
	::WritePrivateProfileString(qdefname, "Puser", qpopt.spq_puname, pfilepath);
	::WritePrivateProfileString(qdefname, "Title", qpopt.spq_file, pfilepath);
	WritePrivateProfileInt(qdefname, "Spage", (unsigned long) (qpopt.spq_start+1L), pfilepath);
	WritePrivateProfileInt(qdefname,
							  "Epage",
							  qpopt.spq_end > LOTSANDLOTS? 0:
							  (unsigned long) (qpopt.spq_end+1L), pfilepath);
	::WritePrivateProfileString(qdefname, "PPflags", qpopt.spq_flags, pfilepath);
	WritePrivateProfileInt(qdefname, "Pdeltime", qpopt.spq_ptimeout, pfilepath);
	WritePrivateProfileInt(qdefname, "NPdeltime", qpopt.spq_nptimeout, pfilepath);
	WritePrivateProfileInt(qdefname, "NPdeltime", qpopt.spq_nptimeout, pfilepath);
	long ntime = qpopt.spq_hold;
	if  (ntime > 0  &&  m_options.spq_options.abshold)
		ntime -= time(NULL);
	WritePrivateProfileInt(qdefname, "Holdtime", ntime, pfilepath);
	m_needsave = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CSprsetwApp commands
