// portnums.cpp : implementation file
//

#include "stdafx.h"
#include "sprsetw.h"
#include "portnums.h"
#include "files.h"
#include "Sprsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPortnums dialog


CPortnums::CPortnums(CWnd* pParent /*=NULL*/)
	: CDialog(CPortnums::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPortnums)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CPortnums::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPortnums)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPortnums, CDialog)
	//{{AFX_MSG_MAP(CPortnums)
	ON_BN_CLICKED(IDC_SETDEFAULT, OnSetdefault)
	ON_BN_CLICKED(IDC_SAVESETTINGS, OnSavesettings)
	ON_BN_CLICKED(IDC_APPLYINC, OnApplyinc)
	ON_BN_CLICKED(IDC_APPLYDEC, OnApplydec)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

char	TCP_sect[] = "TCP Ports";
char	UDP_sect[] = "UDP Ports";

struct	pstr	{
	char	*sectname;
	char	*entname;
	int		dlgid;
	int		scr_dlgid;
	int		default_val;
};

struct	pstr	pstr_list[] = {
	{	TCP_sect,	"Connection",	IDC_CONNPORT,	IDC_SCR_CONNPORT,	DEF_CONNPORTNUM	},
	{	TCP_sect,	"Feeder",		IDC_VIEWPORT,	IDC_SCR_VIEWPORT,	DEF_FEEDPORTNUM	},
	{	UDP_sect,	"Probe",		IDC_PROBEPORT,	IDC_SCR_PROBEPORT,	DEF_PROBEPORTNUM },
	{	UDP_sect,	"Printout",		IDC_CLIENTPORT,	IDC_SCR_CLIENTPORT,	DEF_SERVPORTNUM },
	{	TCP_sect,	"API",			IDC_APITCPPORT,	IDC_SCR_APITCPPORT,	DEF_TCPAPIPORTNUM },
	{	UDP_sect,	"API",			IDC_APIUDPPORT,	IDC_SCR_APIUDPPORT,	DEF_UDPAPIPORTNUM }
};

extern	char	FAR	basedir[];

/////////////////////////////////////////////////////////////////////////////
// CPortnums message handlers

BOOL CPortnums::OnInitDialog()
{
	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, INIFILE);
	CDialog::OnInitDialog();
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		SetDlgItemInt(w.dlgid, ::GetPrivateProfileInt(w.sectname, w.entname, w.default_val, pfilepath));
		((CSpinButtonCtrl *) GetDlgItem(w.scr_dlgid))->SetRange(1, 32767);
	}
	SetDlgItemInt(IDC_INCALLBY, 1000);
	SetDlgItemInt(IDC_CLIENTPORT, ntohs(Locparams.uaportnum));
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_INCALLBY))->SetRange(1, 32767);
	return TRUE;
}

void CPortnums::OnSetdefault()
{
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		SetDlgItemInt(w.dlgid, w.default_val);
	}
}

void CPortnums::OnSavesettings()
{
	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, INIFILE);
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		char	nbuf[20];
		wsprintf(nbuf, "%d", GetDlgItemInt(w.dlgid));
		::WritePrivateProfileString(w.sectname, w.entname, nbuf, pfilepath);
	}
}

void CPortnums::OnApplyinc()
{
	int	incr = GetDlgItemInt(IDC_INCALLBY);
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		int		num = GetDlgItemInt(w.dlgid);
		long	result = num + incr;
		if  (result > 32767L)
			return;
	}
	for  (cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		int		num = GetDlgItemInt(w.dlgid);
		SetDlgItemInt(w.dlgid, num + incr);
	}
}

void CPortnums::OnApplydec()
{
	int	incr = GetDlgItemInt(IDC_INCALLBY);
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		int		num = GetDlgItemInt(w.dlgid);
		long	result = num - incr;
		if  (result <= 0l)
			return;
	}
	for  (cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		int		num = GetDlgItemInt(w.dlgid);
		SetDlgItemInt(w.dlgid, num - incr);
	}
}

void CPortnums::OnOK()
{
	Locparams.uaportnum = htons(GetDlgItemInt(IDC_CLIENTPORT));
	CDialog::OnOK();
}

const DWORD a106HelpIDs[] = {
	IDC_SETDEFAULT,	IDH_106_183,	// Port Numbers Set Default
	IDC_INCALLBY,	IDH_106_184,	// Port Numbers 
	IDC_SCR_INCALLBY,	IDH_106_185,	// Port Numbers Spin1
	IDC_APPLYINC,	IDH_106_186,	// Port Numbers Incr
	IDC_APPLYDEC,	IDH_106_187,	// Port Numbers Decr
	IDC_CLIENTPORT,	IDH_106_188,	// Port Numbers 
	IDC_SCR_CLIENTPORT,	IDH_106_189,	// Port Numbers Spin2
	IDC_PROBEPORT,	IDH_106_190,	// Port Numbers 
	IDC_SCR_PROBEPORT,	IDH_106_191,	// Port Numbers Spin2
	IDC_CONNPORT,	IDH_106_192,	// Port Numbers 
	IDC_SCR_CONNPORT,	IDH_106_193,	// Port Numbers Spin2
	IDC_VIEWPORT,	IDH_106_194,	// Port Numbers 
	IDC_SCR_VIEWPORT,	IDH_106_195,	// Port Numbers Spin2
	IDC_APITCPPORT,	IDH_106_196,	// Port Numbers 
	IDC_SCR_APITCPPORT,	IDH_106_197,	// Port Numbers Spin2
	IDC_APIUDPPORT,	IDH_106_198,	// Port Numbers 
	IDC_SCR_APIUDPPORT,	IDH_106_199,	// Port Numbers Spin2
	IDC_SAVESETTINGS,	IDH_106_200,	// Port Numbers Save Settings to INI file
	0, 0
};

BOOL CPortnums::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	for  (int cnt = 0;  a106HelpIDs[cnt] != 0;  cnt += 2)
		if  (a106HelpIDs[cnt] == DWORD(pHelpInfo->iCtrlId))  {
			AfxGetApp()->WinHelp(a106HelpIDs[cnt+1], HELP_CONTEXTPOPUP);
			return  TRUE;
		}
	
	return CDialog::OnHelpInfo(pHelpInfo);
}
