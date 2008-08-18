/* xmspq.c -- xmspq main routine

   Copyright 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
static	char	rcsid2[] = "@(#) $Revision: 1.1 $";
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef	USING_FLOCK
#include <sys/sem.h>
#endif
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/CascadeB.h>
#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include <Xm/SeparatoGP.h>
#include <Xm/TextF.h>
#include "incl_sig.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "xfershm.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xmsq_ext.h"
#include "xmmenu.h"
#include "displayopt.h"

void	jdisplay(void);
void	openjfile(void);
void	openpfile(void);
void	pdisplay(void);
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
void	allocate_colours(void);
#endif

FILE	*Cfile;
char	Confvarname[] = "XMSPQCONF";

#define	IPC_MODE	0600
int	Ctrl_chan = -1;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;

DEF_DISPOPTS;

unsigned	Pollinit,	/* Initial polling */
		Pollfreq;	/* Current polling frequency */

uid_t	Daemuid,
	Realuid,
	Effuid;

char	scrkeep,
	confabort;

int	arr_rtime,
	arr_rint;

char	*Realuname;

struct	spdet	*mypriv;

struct	spr_req	jreq,
		preq,
		oreq;
struct	spq	JREQ;
struct	spptr	PREQ;

char	*ptdir,
	*spdir,
	*Curr_pwd;

/* X Stuff */

XtAppContext	app;
Display		*dpy;

Widget	toplevel,	/* Main window */
	jtitwid,	/* Job title */
	ptitwid,	/* Ptr title */
	jwid,		/* Job scroll list */
	pwid;		/* Printer scroll list */

static	Widget	panedw,		/* Paned window to stick rest in */
		menubar,	/* Menu */
		jpopmenu,	/* Job popup menu */
		ppopmenu,	/* Ptr popup menu */
		toolbar;	/* Optional toolbar */

XtIntervalId	Ptimeout;

typedef	struct	{
	Boolean	toolbar_pres;
	Boolean jobtit_pres;
	Boolean ptrtit_pres;
	Boolean footer_pres;
	Boolean keep_jscroll;
	Boolean nopagecount;
	Boolean localonly;
	Boolean	sortptrs;
	int	unprintedonly;
	int	incjobs;
	String	confirmabort;
	String	onlyuser;
	String	onlyprinter;
	String	onlytitle;
	long	classcode;
	int	pollfreq;
	int	rtime, rint;
}  vrec_t;

static void	cb_about(void);
static void	cb_quit(Widget, int);
static void	cb_saveopts(Widget);

static	XtResource	resources[] = {
	{ "toolbarPresent", "ToolbarPresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, toolbar_pres), XtRImmediate, False },
	{ "jtitlePresent", "JtitlePresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, jobtit_pres), XtRImmediate, False },
	{ "ptitlePresent", "PtitlePresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, ptrtit_pres), XtRImmediate, False },
	{ "footerPresent", "FooterPresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, footer_pres), XtRImmediate, False },
	{ "keepJobScroll", "KeepJobScroll", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, keep_jscroll), XtRImmediate, False },
	{ "noPageCount", "NoPageCount", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, nopagecount), XtRImmediate, False },
	{ "localOnly", "LocalOnly", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, localonly), XtRImmediate, False },
	{ "sortPtrs", "SortPtrs", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, sortptrs), XtRImmediate, False },
	{ "unPrintedOnly", "UnPrintedOnly", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, unprintedonly), XtRImmediate, (XtPointer) 0 },
	{ "inclJobs", "InclJobs", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, incjobs), XtRImmediate, (XtPointer) 1 },

	{ "confirmAbort", "ConfirmAbort", XtRString, sizeof(String),
		  XtOffsetOf(vrec_t, confirmabort), XtRString, "" }, /* Default unprinted */
	{ "onlyUser", "OnlyUser", XtRString, sizeof(String),
		  XtOffsetOf(vrec_t, onlyuser), XtRString, "" },
	{ "onlyPrinter", "OnlyPrinter", XtRString, sizeof(String),
		  XtOffsetOf(vrec_t, onlyprinter), XtRString, "" },
	{ "onlyTitle", "OnlyTitle", XtRString, sizeof(String),
		  XtOffsetOf(vrec_t, onlytitle), XtRString, "" },
	{ "onlyClasscode", "OnlyClasscode", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, classcode), XtRImmediate, (XtPointer) 0xFFFFFFFFL },
	{ "pollFreq", "PollFreq", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, pollfreq), XtRImmediate, (XtPointer) DEFAULT_REFRESH },
	{ "repeatTime", "RepeatTime", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, rtime), XtRImmediate, (XtPointer) 500 },
	{ "repeatInt", "RepeatInt", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, rint), XtRImmediate, (XtPointer) 100 }};

static	casc_button
opt_casc[] = {
	{	ITEM,	"Viewopts",	cb_viewopt,	0	},
	{	SEP	},
	{	ITEM,	"Saveopts",	cb_saveopts,	0	},
	{	DSEP	},
	{	ITEM,	"Syserror",	cb_syserr,	0	},
	{	DSEP	},
	{	ITEM,	"Quit",		cb_quit,	0	}},
act_casc[] = {
	{	ITEM,	"Abortj",	cb_jact,	SO_AB	},
	{	ITEM,	"Onemore",	cb_onemore,	0	},
	{	SEP	},
	{	ITEM,	"Go",		cb_pact,	SO_PGO	},
	{	ITEM,	"Heoj",		cb_pact,	SO_PHLT	},
	{	ITEM,	"Halt",		cb_pact,	SO_PSTP	},
	{	ITEM,	"Ok",		cb_pact,	SO_OYES	},
	{	ITEM,	"Nok",		cb_pact,	SO_ONO	}},
job_casc[] = {
	{	ITEM,	"View",		cb_view,	0	},
	{	ITEM,	"Formj",	cb_jform,	0	},
	{	ITEM,	"Pages",	cb_jpages,	0	},
	{	ITEM,	"User",		cb_juser,	0	},
	{	ITEM,	"Retain",	cb_jretain,	0	},
	{	ITEM,	"Classj",	cb_jclass,	0	},
	{	SEP	},
	{	ITEM,	"Unqueue",	cb_unqueue,	0	}},
ptr_casc[] = {
	{	ITEM,	"Interrupt",	cb_pact,	SO_INTER	},
	{	ITEM,	"Abortp",	cb_pact,	SO_PJAB	},
	{	ITEM,	"Restart",	cb_pact,	SO_RSP	},
	{	SEP	},
	{	ITEM,	"Formp",	cb_pform,	0	},
	{	ITEM,	"Classp",	cb_pclass,	0	},
	{	ITEM,	"Devicep",	cb_pdev,	0	},
	{	SEP	},
	{	ITEM,	"Add",		cb_padd,	0	},
	{	ITEM,	"Delete",	cb_pact,	SO_DELP	}},
search_casc[] = {
	{	ITEM,	"Search",	cb_srchfor,	0	},
	{	ITEM,	"Searchforw",	cb_rsrch,	0	},
	{	ITEM,	"Searchback",	cb_rsrch,	1	}},
help_casc[] = {
	{	ITEM,	"Help",		dohelp,		$H{xmspq help main screen}	},
	{	ITEM,	"Helpon",	cb_chelp,	0	},
	{	SEP	},
	{	ITEM,	"About",	cb_about,	0	}},
jobpop_casc[] = {
	{	ITEM,	"Abortj",	cb_jact,	SO_AB	},
	{	SEP	},
	{	ITEM,	"Onemore",	cb_onemore,	0	},
	{	ITEM,	"Formj",	cb_jform,	0	},
	{	SEP	},
	{	ITEM,	"View",		cb_view,	0	},
	{	ITEM,	"Unqueue",	cb_unqueue,	0	}},
ptrpop_casc[] = {
	{	ITEM,	"Go",		cb_pact,	SO_PGO	},
	{	ITEM,	"Interrupt",	cb_pact,	SO_INTER},
	{	ITEM,	"Restart",	cb_pact,	SO_RSP	},
	{	ITEM,	"Abortp",	cb_pact,	SO_PJAB	},
	{	SEP	},
	{	ITEM,	"Heoj",		cb_pact,	SO_PHLT	},
	{	ITEM,	"Halt",		cb_pact,	SO_PSTP	},
	{	SEP	},
	{	ITEM,	"Formp",	cb_pform,	0	},
	{	SEP	},
	{	ITEM,	"Ok",		cb_pact,	SO_OYES	},
	{	ITEM,	"Nok",		cb_pact,	SO_ONO	}};

static	pull_button
	opt_button = {
		"Options",	XtNumber(opt_casc),	$H{xmspq options help},	opt_casc	},
	act_button = {
		"Action",	XtNumber(act_casc),	$H{xmspq action help},	act_casc	},
	job_button = {
		"Jobs",		XtNumber(job_casc),	$H{xmspq jobs help},	job_casc	},
	ptr_button = {
		"Printers",	XtNumber(ptr_casc),	$H{xmspq ptr help},	ptr_casc	},
	srch_button = {
		"Search",	XtNumber(search_casc),	$H{xmspq search help},	search_casc	},
	help_button = {
		"Help",		XtNumber(help_casc),	$H{xmspq help help},	help_casc,	1	};

static	pull_button	*menlist[] = {
	&opt_button,	&act_button,	&job_button,
	&ptr_button,	&srch_button,	&help_button
};

typedef	struct	{
	char	*name;
	void	(*callback)();
	int	callback_data;
	int	helpnum;
}  tool_button;

static	tool_button	toollist[] = {
	{	"Abortjt",	cb_jact,	SO_AB,		$H{xmspq abort buthelp}	},
	{	"Onemore",	cb_onemore,	0,		$H{xmspq onemore buthelp}},
	{	"Form",		cb_jform,	0,		$H{xmspq jform buthelp}	},
	{	"View",		cb_view,	0,		$H{xmspq view buthelp}	},
	{	"Pform",	cb_pform,	0,		$H{xmspq pform buthelp}	},
	{	"Go",		cb_pact,	SO_PGO,		$H{xmspq pgo buthelp}	},
	{	"Heoj",		cb_pact,	SO_PHLT,	$H{xmspq pheoj buthelp}	},
	{	"Halt",		cb_pact,	SO_PSTP,	$H{xmspq pstp buthelp}	},
	{	"Ok",		cb_pact,	SO_OYES,	$H{xmspq pok buthelp}	},
	{	"Nok",		cb_pact,	SO_ONO,		$H{xmspq pnok buthelp}	}};

#if	defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY)

/* Define our own bcopy and bzero because X uses these in places and
   we don't want to include some -libucb which pulls in funny
   sprintfs etc */

void	bcopy(void *from, void *to, unsigned count)
{
	memcpy(to, from, count);
}

void	bzero(void *to, unsigned count)
{
	memset(to, '\0', count);
}
#endif

void	exit_cleanup(void)
{
	if  (Ctrl_chan >= 0)
		womsg(SO_DMON);
}

/* Don't put exit as a callback or we'll get some weird exit code
   based on a Widget pointer.  */

static void	cb_quit(Widget w, int n)
{
#ifndef	HAVE_ATEXIT
	exit_cleanup();
#endif
	exit(n);
}

static void	dumpbool(FILE *tf, char *name, const int value)
{
	fprintf(tf, "%s.%s:\t%s\n", progname, name, value? "True": "False");
}

static void	cb_saveopts(Widget w)
{
	char	*srfile = (char *) 0, *hf;
	FILE	*inf, *tf;
	int	ch;
	unsigned	oldumask;
	Dimension	wid;
	SHORT	items;
	time_t	now;
	struct	tm	*tp;

	if  (!Confirm(w, $PH{xm save options}))
		return;

	hf = envprocess("$HOME/GSPOOL");
	if  (!(inf = fopen(hf, "r")))  {
		srfile = XtResolvePathname(dpy, "app-defaults", NULL, NULL, NULL, NULL, 0, NULL);
		if  (!(inf = fopen(srfile, "r")))  {
			doerror(w, $EH{xmspq app-default cannot open});
			XtFree(srfile);
			return;
		}
		XtFree(srfile);
	}
	tf = tmpfile();
	while  ((ch = getc(inf)) != EOF)
		putc(ch, tf);
	fclose(inf);
	time(&now);
	tp = localtime(&now);
	fprintf(tf, "\n!! %s User-defined options %.2d:%.2d:%.2d %.2d/%.2d/%.2d\n\n",
		       progname,
		       tp->tm_hour, tp->tm_min, tp->tm_sec,
		       tp->tm_year % 100, tp->tm_mon+1, tp->tm_mday);
	dumpbool(tf, "keepJobScroll", scrkeep);
	dumpbool(tf, "localOnly", Displayopts.opt_localonly != NRESTR_NONE);
	dumpbool(tf, "sortPtrs", Displayopts.opt_sortptrs != SORTP_NONE);
	fprintf(tf, "%s.unPrintedOnly:\t%d\n", progname, (int) Displayopts.opt_jprindisp);
	fprintf(tf, "%s.inclJobs:\t%d\n", progname, (int) Displayopts.opt_jinclude);
	fprintf(tf, "%s.onlyClasscode:\t%ld\n", progname, (long) Displayopts.opt_classcode);
	fprintf(tf, "%s.confirmAbort:\t%s\n", progname, confabort == 0? "Never": confabort > 1? "Always": "Unprinted");
	fprintf(tf, "%s.onlyUser:\t%s\n", progname, Displayopts.opt_restru? Displayopts.opt_restru: "");
	fprintf(tf, "%s.onlyPrinter:\t%s\n", progname, Displayopts.opt_restrp? Displayopts.opt_restrp: "");
	fprintf(tf, "%s.onlyTitle:\t%s\n", progname, Displayopts.opt_restrt? Displayopts.opt_restrt: "");

	XtVaGetValues(jwid, XmNwidth, &wid, XmNvisibleItemCount, &items, NULL);
	fprintf(tf, "%s*jlist.width: %ld\n", progname, (long) wid);
	fprintf(tf, "%s*jlist.visibleItemCount: %ld\n", progname, (long) items);
	XtVaGetValues(pwid, XmNvisibleItemCount, &items, NULL);
	fprintf(tf, "%s*plist.visibleItemCount: %ld\n", progname, (long) items);
	SWAP_TO(Realuid);
	oldumask = umask(0);
	umask(oldumask & ~0444);
	if  (!(inf = fopen(hf, "w")))  {
		SWAP_TO(Daemuid);
		doerror(w, $EH{xmspq app-default cannot create});
		if  (srfile)
			free(srfile);
		return;
	}
	umask(oldumask);
	SWAP_TO(Daemuid);
	free(hf);
	rewind(tf);
	while  ((ch = getc(tf)) != EOF)
		putc(ch, inf);
	fclose(tf);
	fclose(inf);
}

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
#ifndef	HAVE_ATEXIT
	exit_cleanup();
#endif
	exit(E_NOMEM);
}

/* If we get a message error die appropriately */

static void	msg_error(const int ret)
{
	doerror(jwid, ret);
	exit(E_SETUP);
}

/* Write messages to scheduler.  */

void	womsg(const int act)
{
	oreq.spr_un.o.spr_act = (USHORT) act;
	if  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)
		msg_error(errno == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error});
}

void	my_wjmsg(const int act)
{
	int	ret;
	jreq.spr_un.j.spr_act = (USHORT) act;
	if  ((ret = wjmsg(&jreq, &JREQ)))
		msg_error(ret);
}

void	my_wpmsg(const int act)
{
	int	ret;
	preq.spr_un.p.spr_act = (USHORT) act;
	if  ((ret = wpmsg(&preq, &PREQ)))
		msg_error(ret);
}

static void	cb_about(void)
{
	Widget		dlg;
	char	buf[sizeof(rcsid1) + sizeof(rcsid2) + 2];
	sprintf(buf, "%s\n%s", rcsid1, rcsid2);
	dlg = XmCreateInformationDialog(jwid, "about", NULL, 0);
	XtVaSetValues(dlg,
		      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		      XtVaTypedArg, XmNmessageString, XmRString, buf, strlen(buf),
		      NULL);
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
	XtManageChild(dlg);
	XtPopup(XtParent(dlg), XtGrabNone);
}

/* This deals with alarm calls whilst polling.  */

void	pollit(int n, XtIntervalId id)
{
	Ptimeout = (XtIntervalId) 0;

	/* If the job queue has not changed, then halve the frequency
	   of polling.  If it has changed, double it.  */

	if  (Ptr_seg.Last_ser != Ptr_seg.dptr->ps_serial)
		pdisplay();

	if  (Job_seg.Last_ser == Job_seg.dptr->js_serial)  {
		Pollfreq <<= 1;
		if  (Pollfreq > POLLMAX)
			Pollfreq = POLLMAX;
	}
	else  {
		Pollfreq >>= 1;
		if  (Pollfreq < POLLMIN)
			Pollfreq = POLLMIN;
		jdisplay();
	}

	/* We'll add a timeout anyhow as we are unsure about signals
	   getting through */

	Ptimeout = XtAppAddTimeOut(app, Pollfreq * 1000, (XtTimerCallbackProc) pollit, (XtPointer) 0);
}

/* This notes signals from (presumably) the scheduler.  */

static RETSIGTYPE	markit(int sig)
{
#ifdef	UNSAFE_SIGNALS
	signal(sig, markit);
#endif
	if  (sig != QRFRESH)  {
#ifndef	HAVE_ATEXIT
		exit_cleanup();
#endif
		exit(E_SIGNAL);
	}
	if  (Ptimeout)  {
		XtRemoveTimeOut(Ptimeout);
		Ptimeout = (XtIntervalId) 0;
	}
	Ptimeout = XtAppAddTimeOut(app, 10, (XtTimerCallbackProc) pollit, (XtPointer) 0);
}

/* Other signals are errors Suppress final message....  */

static RETSIGTYPE	sigerr(int n)
{
	Ctrl_chan = -1;
	exit(E_SIGNAL);
}

Widget	BuildPulldown(Widget menub, pull_button *item)
{
	int	cnt;
	Widget	pulldown, cascade, button;

	pulldown = XmCreatePulldownMenu(menub, "pulldown", NULL, 0);
	cascade = XtVaCreateManagedWidget(item->pull_name, xmCascadeButtonWidgetClass, menub,
					  XmNsubMenuId, pulldown, NULL);

	if  (item->helpnum != 0)
		XtAddCallback(cascade, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(item->helpnum));

	for  (cnt = 0;  cnt < item->nitems;  cnt++)  {
		char	sname[20];
		casc_button	*cb = &item->items[cnt];
		switch  (cb->type)  {
		case  SEP:
			sprintf(sname, "separator%d", cnt);
			button = XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, pulldown, NULL);
			continue;
		case  DSEP:
			sprintf(sname, "separator%d", cnt);
			button = XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, pulldown,
							 XmNseparatorType, XmDOUBLE_LINE, NULL);
			continue;
		case  ITEM:
			button = XtVaCreateManagedWidget(cb->name, xmPushButtonGadgetClass, pulldown, NULL);
			if  (cb->callback)
				XtAddCallback(button, XmNactivateCallback, (XtCallbackProc) cb->callback, INT_TO_XTPOINTER(cb->callback_data));
			continue;
		}
	}
	if  (item->ishelp)
		return  cascade;
	return  NULL;
}

static void  setup_macros(Widget menub, const int helpcode, const int helpbase, char *pullname, XtCallbackProc macroproc)
{
	int	cnt, had = 0;
	Widget	pulldown, cascade, button;
	char	*macroprmpt[10];

	for  (cnt = 0;  cnt < 10;  cnt++)
		if  ((macroprmpt[cnt] = helpprmpt(helpbase+cnt)))
			had++;

	if  (had <= 0)
		return;

	pulldown = XmCreatePulldownMenu(menub, pullname, NULL, 0);
	cascade = XtVaCreateManagedWidget(pullname, xmCascadeButtonWidgetClass, menub, XmNsubMenuId, pulldown, NULL);
	XtAddCallback(cascade, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(helpcode));
	for  (cnt = 0;  cnt < 10;  cnt++)  {
		char	sname[20];
		if  (!macroprmpt[cnt])
			continue;
		free(macroprmpt[cnt]);
		sprintf(sname, "macro%d", cnt);
		button = XtVaCreateManagedWidget(sname, xmPushButtonGadgetClass, pulldown, NULL);
		XtAddCallback(button, XmNactivateCallback, macroproc, INT_TO_XTPOINTER(cnt));
	}
}

static void	setup_menus(void)
{
	int			cnt;
	XtWidgetGeometry	size;
	Widget			helpw;

	menubar = XmCreateMenuBar(panedw, "menubar", NULL, 0);

	/* Get rid of resize button for menubar */

	size.request_mode = CWHeight;
	XtQueryGeometry(menubar, NULL, &size);
	XtVaSetValues(menubar, XmNpaneMaximum, size.height*2, XmNpaneMinimum, size.height*2, NULL);

	for  (cnt = 0;  cnt < XtNumber(menlist);  cnt++)
		if  ((helpw = BuildPulldown(menubar, menlist[cnt])))
			XtVaSetValues(menubar, XmNmenuHelpWidget, helpw, NULL);

	setup_macros(menubar,
		     $H{xmspq job macro help},
		     $H{Job or User macro},
		     "jobmacro",
		     (XtCallbackProc) cb_macroj);
	setup_macros(menubar,
		     $H{xmspq ptr macro help},
		     $H{Printer macro},
		     "ptrmacro",
		     (XtCallbackProc) cb_macrop);
	XtManageChild(menubar);
}

static void	Buildpopup(Widget wid, casc_button * list, unsigned nlist)
{
	unsigned  cnt;
	Widget	button;
	char	sname[20];

	for  (cnt = 0;  cnt < nlist;  cnt++)  {
		casc_button	*cb = &list[cnt];
		switch  (cb->type)  {
		case  SEP:
			sprintf(sname, "separator%d", cnt);
			XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, wid, NULL);
			continue;
		case  DSEP:
			sprintf(sname, "separator%d", cnt);
			XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, wid, XmNseparatorType, XmDOUBLE_LINE, NULL);
			continue;
		case  ITEM:
			button = XtVaCreateManagedWidget(cb->name, xmPushButtonGadgetClass, wid, NULL);
			if  (cb->callback)
				XtAddCallback(button, XmNactivateCallback, (XtCallbackProc) cb->callback, INT_TO_XTPOINTER(cb->callback_data));
			continue;
		}
	}
}

static void	setup_popupmenus(void)
{
	jpopmenu = XmCreatePopupMenu(jwid, "jobpopup", NULL, 0);
	Buildpopup(jpopmenu, jobpop_casc, XtNumber(jobpop_casc));
	ppopmenu = XmCreatePopupMenu(pwid, "ptrpopup", NULL, 0);
	Buildpopup(ppopmenu, ptrpop_casc, XtNumber(ptrpop_casc));
}

static void	setup_toolbar(void)
{
	int			cnt;

	toolbar = XtVaCreateManagedWidget("toolbar", xmRowColumnWidgetClass, panedw, XmNorientation, XmHORIZONTAL, XmNpacking, XmPACK_TIGHT, NULL);

	/* Set up buttons */

	for  (cnt = 0;  cnt < XtNumber(toollist);  cnt++)  {
		Widget  w = XtVaCreateManagedWidget(toollist[cnt].name, xmPushButtonWidgetClass, toolbar, NULL);
		if  (toollist[cnt].callback)
			XtAddCallback(w, XmNactivateCallback,
				      (XtCallbackProc) toollist[cnt].callback,
				      INT_TO_XTPOINTER(toollist[cnt].callback_data));
		if  (toollist[cnt].helpnum != 0)
			XtAddCallback(w, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(toollist[cnt].helpnum));
	}
}

static Widget	maketitle(char *tname, char *tstring)
{
	Widget			labv;
	XtWidgetGeometry	size;

	if  (tstring)  {
		XmString  str = XmStringCreateLocalized(tstring);
		labv = XtVaCreateManagedWidget(tname,
					       xmLabelWidgetClass, panedw,
					       XmNlabelString,	str,
					       NULL);
		XmStringFree(str);
	}
	else
		labv = XtVaCreateManagedWidget(tname, xmLabelWidgetClass, panedw, NULL);
	size.request_mode = CWHeight;
	XtQueryGeometry(labv, NULL, &size);
	XtVaSetValues(labv, XmNpaneMaximum, size.height, XmNpaneMinimum, size.height, NULL);
	return  labv;
}

#ifdef	ACCEL_TRANSLATIONS
void do_quit(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	cb_quit(wid, 0);
}

void do_viewopts(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	cb_viewopt(wid, 0);
}

void do_jobop(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	cb_jact(wid, SO_AB);
}

void do_onemore(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	cb_onemore();
}

void do_pact(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	int	op;

	if  (*nargs != 1)
		return;

	op = atoi(args[0]);

	switch  (op)  {
	case  SO_PGO:
	case  SO_PHLT:
	case  SO_PSTP:
	case  SO_OYES:
	case  SO_ONO:
	case  SO_INTER:
	case  SO_PJAB:
	case  SO_RSP:
	case  SO_DELP:
		cb_pact(wid, op);
	}
}

void do_view(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	cb_view(wid);
}

void do_form(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	cb_jform(wid);
}

void do_pform(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	cb_pform();
}

void do_search(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	if  (*nargs != 1)
		return;
	cb_rsrch(wid, atoi(args[0]));
}

void do_help(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	dohelp(wid, wid == jwid? $H{xmspq job list help}: $H{xmspq ptr list help});
}
#endif

void do_jpopup(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	XButtonPressedEvent  *bpe = (XButtonPressedEvent *) xev;
	int	pos = XmListYToPos(jwid, bpe->y);
	if  (pos <= 0)
		return;
	XmListSelectPos(jwid, pos, False);
	XmMenuPosition(jpopmenu, bpe);
	XtManageChild(jpopmenu);
}

void do_ppopup(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
	XButtonPressedEvent  *bpe = (XButtonPressedEvent *) xev;
	int	pos = XmListYToPos(pwid, bpe->y);
	if  (pos <= 0)
		return;
	XmListSelectPos(pwid, pos, False);
	XmMenuPosition(ppopmenu, bpe);
	XtManageChild(ppopmenu);
}

static	XtActionsRec	arecs[] = {
#ifdef	ACCEL_TRANSLATIONS
	{	"do-viewopts",		do_viewopts	},
	{	"do-quit",		do_quit		},
	{	"do-jobop",		do_jobop	},
	{	"do-onemore",		do_onemore	},
	{	"do-pact",		do_pact		},
	{	"do-view",		do_view		},
	{	"do-form",		do_form		},
	{	"do-pform",		do_pform	},
	{	"do-search",		do_search	},
	{	"do-help",		do_help		},
#endif
	{	"do-jobpop",		do_jpopup	},
	{	"do-ptrpop",		do_ppopup	}
};

#include "xmspq.bm"

static void	wstart(int argc, char **argv)
{
	int	nopage;
	vrec_t	vrec;
	Pixmap	bitmap;
	char	*jtit;

	toplevel = XtVaAppInitialize(&app, "GSPOOL", NULL, 0, &argc, argv, NULL, NULL);
	XtAppAddActions(app, arecs, XtNumber(arecs));
	XtGetApplicationResources(toplevel, &vrec, resources, XtNumber(resources), NULL, 0);
	bitmap = XCreatePixmapFromBitmapData(dpy = XtDisplay(toplevel),
					     RootWindowOfScreen(XtScreen(toplevel)),
					     xmspq_bits, xmspq_width, xmspq_height, 1, 0, 1);
	XtVaSetValues(toplevel, XmNiconPixmap, bitmap, NULL);

	/* Set up parameters from resources */

	scrkeep = vrec.keep_jscroll? 1: 0;
	nopage = vrec.nopagecount? 1: 0;
	Displayopts.opt_localonly = vrec.localonly? NRESTR_LOCALONLY: NRESTR_NONE;
	Displayopts.opt_sortptrs = vrec.sortptrs? SORTP_BYNAME: SORTP_NONE;
	Displayopts.opt_jprindisp = (enum jrestrict_t) vrec.unprintedonly;
	Displayopts.opt_jinclude = (enum jincl_t) vrec.incjobs;
	arr_rtime = vrec.rtime;
	arr_rint = vrec.rint;
	Pollinit = Pollfreq = vrec.pollfreq;

	switch  (vrec.confirmabort[0])  {
	case  'n':case  'N':		confabort = 0;  break;
	default:case  'u':case  'U':	confabort = 1;  break;
	case  'a':case  'A':		confabort = 2;	break;
	}

	Displayopts.opt_restru = vrec.onlyuser[0]? stracpy(vrec.onlyuser): (char *) 0;
	Displayopts.opt_restrp = vrec.onlyprinter[0]? stracpy(vrec.onlyprinter): (char *) 0;
	Displayopts.opt_restrt = vrec.onlytitle[0]? stracpy(vrec.onlytitle): (char *) 0;

	Displayopts.opt_classcode = (classcode_t) vrec.classcode;
	if  (!(mypriv->spu_flgs & PV_COVER))
		Displayopts.opt_classcode &= mypriv->spu_class;
	if  (Displayopts.opt_classcode == 0)
		Displayopts.opt_classcode = mypriv->spu_class;

	/* Now to create all the bits of the application */

	panedw = XtVaCreateWidget("layout", xmPanedWindowWidgetClass, toplevel, NULL);

	setup_menus();

	if  (vrec.toolbar_pres)
		setup_toolbar();

	jtit = get_jobtitle(nopage);

	if  (vrec.jobtit_pres)
		jtitwid = maketitle("jtitle", jtit);

	free(jtit);

	jwid = XmCreateScrolledList(panedw, "jlist", NULL, 0);
	XtAddCallback(jwid, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmspq job list help});
	XtManageChild(jwid);

	jtit = get_ptrtitle();

	if  (vrec.ptrtit_pres)
		ptitwid = maketitle("ptitle", jtit);

	free(jtit);

	pwid = XmCreateScrolledList(panedw, "plist", NULL, 0);
	XtAddCallback(pwid, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmspq ptr list help});
	XtManageChild(pwid);
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
	allocate_colours();
#endif

	if  (vrec.footer_pres)
		maketitle("footer", (char *) 0);

	setup_popupmenus();
	XtManageChild(panedw);
	XtRealizeWidget(toplevel);
}

/* Tell the scheduler we are here and do the business The initial
   refresh will fill up the job and printer screens for us.  */

static void	process(void)
{
#ifdef	STRUCT_SIG
	struct	sigstruct_name	z;
	z.sighandler_el = markit;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(QRFRESH, &z, (struct sigstruct_name *) 0);
	z.sighandler_el = sigerr;
	sigact_routine(SIGINT, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGQUIT, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGHUP, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGTERM, &z, (struct sigstruct_name *) 0);
#else
	/* signal is #defined as sigset on suitable systems */
	signal(QRFRESH, markit);
	signal(SIGINT, sigerr);
	signal(SIGQUIT, sigerr);
	signal(SIGHUP, sigerr);
	signal(SIGTERM, sigerr);
#endif
	oreq.spr_un.o.spr_arg1 = Realuid;
	womsg(SO_MON);
	XtAppMainLoop(app);
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	int	ret;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	/* If we haven't got a directory, use the current */

	if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
		Curr_pwd = runpwd();

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile(Confvarname, "xmspq.help");
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	mypriv = getspuser(Realuid);
	SWAP_TO(Realuid);
	Displayopts.opt_classcode = mypriv->spu_class;
	SWAP_TO(Daemuid);
	if  ((mypriv->spu_flgs & (PV_OTHERJ|PV_VOTHERJ)) != (PV_OTHERJ|PV_VOTHERJ))
		Realuname = prin_uname(Realuid);

	spdir = envprocess(SPDIR);

	if  (chdir(spdir) < 0)  {
		print_error($E{Cannot chdir});
		exit(E_NOCHDIR);
	}
	ptdir = envprocess(PTDIR);

	if  ((Ctrl_chan = msgget(MSGID, 0)) < 0)  {
		print_error($E{Spooler not running});
		exit(E_NOTRUN);
	}

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID, SEMNUMS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif

	/* Open the other files. No read yet until the spool scheduler
	   is aware of our existence, which it won't be until we
	   send it a message.  */

	if  ((ret = init_xfershm(1)))  {
		print_error(ret);
		exit(E_SETUP);
	}
	openjfile();
	openpfile();
	oreq.spr_mtype = jreq.spr_mtype = preq.spr_mtype = MT_SCHED;
	oreq.spr_un.o.spr_pid = preq.spr_un.p.spr_pid = jreq.spr_un.j.spr_pid = getpid();
	wstart(argc, argv);
#ifdef	HAVE_ATEXIT
	atexit(exit_cleanup);
#endif
	process();
	return  0;
}
