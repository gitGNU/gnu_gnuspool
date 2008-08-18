#pragma comment (exestr, "@(#) $Revision: 1.1 $")

#include "stdafx.h"
#include <direct.h>
#include "files.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "formatcode.h"
#include "spqw.h"
#include "rowview.h"
#include "jobdoc.h"
#include "jobview.h"
#include "ptrdoc.h"
#include "ptrview.h"
#include "jdatadoc.h"
#include "jdatavie.h"
#include "getregdata.h"
#include "loginhost.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpqwApp

BEGIN_MESSAGE_MAP(CSpqwApp, CWinApp)
	//{{AFX_MSG_MAP(CSpqwApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpqwApp construction

CSpqwApp::CSpqwApp()
{
	clean();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSpqwApp object

CSpqwApp NEAR theApp;

const  char poptname[] = "Program options";
const  char	ddefname[] = "Display defaults";
const  char	tportsname[] = "TCP Ports";
const  char	portsname[] = "UDP Ports";

char  FAR	basedir[_MAX_PATH];

/////////////////////////////////////////////////////////////////////////////
// CSpqwApp initialization

BOOL CSpqwApp::InitInstance()
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

	SetDialogBkColor(RGB(255,255,255));        // set dialog background color to gray

	CMultiDocTemplate	*dtjob;
	CMultiDocTemplate	*dtptr;
	CMultiDocTemplate	*dtjdata;

	AddDocTemplate(dtjob = new CMultiDocTemplate(IDR_JOBS,
			RUNTIME_CLASS(CJobdoc),
			RUNTIME_CLASS(CMDIChildWnd),        // standard MDI child frame
			RUNTIME_CLASS(CJobView)));
	AddDocTemplate(dtptr = new CMultiDocTemplate(IDR_PTR,
			RUNTIME_CLASS(CPtrdoc),
			RUNTIME_CLASS(CMDIChildWnd),        // standard MDI child frame
			RUNTIME_CLASS(CPtrView)));
	AddDocTemplate(dtjdata = new CMultiDocTemplate(IDR_VIEWJOB,
			RUNTIME_CLASS(CJdatadoc),
			RUNTIME_CLASS(CMDIChildWnd),        // standard MDI child frame
			RUNTIME_CLASS(CJdataview)));

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	pMainFrame->ShowWindow(SW_SHOWMAXIMIZED);
	pMainFrame->UpdateWindow();
	pMainFrame->m_dtjob = dtjob;
	pMainFrame->m_dtptr = dtptr;
	pMainFrame->m_dtjdata = dtjdata;
	
	m_pMainWnd = pMainFrame;
	
	//  We only allow one of these things to happen at once
	
	if  (m_hPrevInstance != NULL)  {
		AfxMessageBox(IDP_PREVINSTANCE, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}

	//  Load options first as the port-number reading
	//  routines in windows sockets don't work so we don't
	//  worry about them.
	
	load_options();

	int  ret = winsockstart();
    if  (ret != 0)  {
    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
		return  FALSE;
    }
    if  ((ret = initenqsocket(Locparams.servid)) != 0)  {
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

	if  ((ret = xt_enquire(m_winuser, m_winmach, m_username)) != 0)  {
		if  (ret != IDP_XTENQ_PASSREQ)  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			winsockend();
			return  FALSE;            
		}
		CLoginHost	dlg;
		dlg.m_unixhost = look_host(Locparams.servid);
		dlg.m_clienthost = m_winmach;
		dlg.m_username = m_winuser;
		int	cnt = 0;
		for  (;;)  {
			if  (dlg.DoModal() != IDOK)  {
				winsockend();
				return  FALSE;
			}
			if  ((ret = xt_login(dlg.m_username, m_winmach, (const char *) dlg.m_passwd, m_username)) == 0)
				break;
			if  (ret != IDP_XTENQ_BADPASSWD || cnt >= 2)  {
		    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
				winsockend();
				return  FALSE;
			}
			if  (AfxMessageBox(ret, MB_RETRYCANCEL|MB_ICONQUESTION) == IDCANCEL)  {
				winsockend();
				return  FALSE;
			}
			cnt++;
		}
	}
	

	//  And now do the business...

	if  ((ret = getspuser(m_mypriv, m_username)) != 0)  {
		AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
   		winsockend();
		return  FALSE;
   	}

	if  ((ret = initsockets()) != 0)  {
   		AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
   		winsockend();
		return  FALSE;
   	}
	
	if  (!m_mypriv.ispriv(PV_COVER))  {
		m_options.spq_options.classcode &= m_mypriv.spu_class;
		if  (m_options.spq_options.classcode == 0)  {
			AfxMessageBox(IDP_FOSSILCC, MB_OK|MB_ICONSTOP);
			m_options.spq_options.classcode = m_mypriv.spu_class;
		}
	}
    pMainFrame->m_pollfreq = m_options.spq_options.Pollinit;
    pMainFrame->m_restrictlist.loadrestrict();
    attach_hosts();
    pMainFrame->SetTimer(WM_NETMSG_TICKLE, Locparams.servtimeout*1000, NULL);
	pMainFrame->InitWins();
	return TRUE;
}                   

int CSpqwApp::ExitInstance()
{
	if  (isdirty()  &&  AfxMessageBox(IDP_SAVEPROPTS, MB_YESNO|MB_ICONQUESTION) == IDYES)
		save_options();
	netshut();
	winsockend();
	return  0;
}	

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
void CSpqwApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

BOOL CSpqwApp::OnIdle(LONG lcount)
{
	unsigned  long	todo;
	if  (ioctlsocket(Locparams.probesock, FIONREAD, &todo) != SOCKET_ERROR  &&  todo != 0)  {
		reply_probe();                                                                
		return  FALSE;
	}
	if  (Locparams.Netsync_req != 0)
		netsync();                  

	return  CWinApp::OnIdle(lcount);	
}

void	CSpqwApp::load_options()
{
	char	pfilepath[_MAX_PATH];
    strcpy(pfilepath, basedir);
    strcat(pfilepath, INIFILE);

	char	cbuf[100];
	
	m_options.spq_options.Pollinit =
		m_options.spq_options.Pollfreq =
			(unsigned short) ::GetPrivateProfileInt(poptname, "Polling", DEFAULT_REFRESH, pfilepath);
	m_options.spq_options.batext = ::GetPrivateProfileInt(poptname, "Batext", 0, pfilepath);
	m_options.spq_options.abshold = ::GetPrivateProfileInt(poptname, "Abshold", 0, pfilepath);
	::GetPrivateProfileString(ddefname, "Dclass", "0xFFFFFFFF", cbuf, sizeof(cbuf), pfilepath);
	m_options.spq_options.classcode = (unsigned long) strtoul(cbuf, (char **) 0, 0);
	m_options.spq_options.confabort =
			(unsigned char) ::GetPrivateProfileInt(ddefname, "Confabort", 1, pfilepath);
	m_options.spq_options.Restrunp =
			(unsigned char) ::GetPrivateProfileInt(ddefname, "OnlyUnprinted", 0, pfilepath);
	m_options.spq_options.probewarn =
			(unsigned char) ::GetPrivateProfileInt(ddefname, "Probewarn", 1, pfilepath);
	::GetPrivateProfileString(ddefname, "OnlyPrinter", "", cbuf, sizeof(cbuf), pfilepath);
    m_options.spq_options.Restrp = cbuf;
	::GetPrivateProfileString(ddefname, "OnlyUser", "", cbuf, sizeof(cbuf), pfilepath);
    m_options.spq_options.Restru = cbuf;
	::GetPrivateProfileString(ddefname, "OnlyTitle", "", cbuf, sizeof(cbuf), pfilepath);
    m_options.spq_options.Restrt = cbuf;

	if  (::GetPrivateProfileString(ddefname, "Jfmt", "", cbuf, sizeof(cbuf), pfilepath) > 0)
		m_jfmt = cbuf;
	else
		m_jfmt.LoadString(IDS_DEF_JFMT);
	if  (::GetPrivateProfileString(ddefname, "Pfmt", "", cbuf, sizeof(cbuf), pfilepath) > 0)
		m_pfmt = cbuf;
	else
		m_pfmt.LoadString(IDS_DEF_PFMT);
	for (unsigned cnt = SPP_OFFLINE;  cnt <= SPP_OPER;  cnt++)  {
		char  nbuf[15];
		wsprintf(nbuf, "pcolour%d", cnt);
		m_appcolours[cnt] = ::GetPrivateProfileInt(ddefname, nbuf, 0, pfilepath);
	}

	//  Do port numbers
	//  Read defaults from services file if available.
	
	Locparams.uaportnum = htons(::GetPrivateProfileInt(portsname, "Printout", DEF_SERVPORTNUM, pfilepath));
	Locparams.pportnum = htons(::GetPrivateProfileInt(portsname, "Probe", DEF_PROBEPORTNUM, pfilepath));
	Locparams.lportnum = htons(::GetPrivateProfileInt(tportsname, "Connection", DEF_CONNPORTNUM, pfilepath));
	Locparams.vportnum = htons(::GetPrivateProfileInt(tportsname, "Feeder", DEF_FEEDPORTNUM, pfilepath));
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

void	CSpqwApp::save_options()
{
	char	pfilepath[_MAX_PATH];
    strcpy(pfilepath, basedir);
    strcat(pfilepath, INIFILE);
	WritePrivateProfileInt(poptname, "Polling", m_options.spq_options.Pollinit, pfilepath);
	WritePrivateProfileBool(poptname, "Batext", m_options.spq_options.batext, pfilepath);
	WritePrivateProfileBool(poptname, "Abshold", m_options.spq_options.abshold, pfilepath);
	WritePrivateProfileHex(ddefname, "Dclass", m_options.spq_options.classcode, pfilepath);
	WritePrivateProfileInt(ddefname, "Confabort", m_options.spq_options.confabort, pfilepath);
	WritePrivateProfileInt(ddefname, "OnlyUnprinted", m_options.spq_options.Restrunp, pfilepath);
	WritePrivateProfileBool(ddefname, "Probewarn", m_options.spq_options.probewarn, pfilepath);
	::WritePrivateProfileString(ddefname, "OnlyPrinter", m_options.spq_options.Restrp, pfilepath);
	::WritePrivateProfileString(ddefname, "OnlyUser", m_options.spq_options.Restru, pfilepath);
	::WritePrivateProfileString(ddefname, "OnlyTitle", m_options.spq_options.Restrt, pfilepath);
	::WritePrivateProfileString(ddefname, "Jfmt", (const char *) ('\"' + m_jfmt + '\"'), pfilepath);
	::WritePrivateProfileString(ddefname, "Pfmt", (const char *) ('\"' + m_pfmt + '\"'), pfilepath);
	for (unsigned cnt = SPP_OFFLINE;  cnt <= SPP_OPER;  cnt++)  {
		char  nbuf[15];
		wsprintf(nbuf, "pcolour%d", cnt);
		WritePrivateProfileInt(ddefname, nbuf, m_appcolours[cnt], pfilepath);
	}

	WritePrivateProfileInt(portsname, "Printout", ntohs(Locparams.uaportnum), pfilepath);
	WritePrivateProfileInt(portsname, "Probe", ntohs(Locparams.pportnum), pfilepath);
	WritePrivateProfileInt(tportsname, "Connection", ntohs(Locparams.lportnum), pfilepath);
	WritePrivateProfileInt(tportsname, "Feeder", ntohs(Locparams.vportnum), pfilepath);
	clean();
}
