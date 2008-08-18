/* xmspuser.c -- xmspuser main routine

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
static	char	rcsid1[] = "@(#) $Id: xmspuser.c,v 1.1 2008/08/18 16:25:54 jmc Exp $";		/* We use these in the about message */
static	char	rcsid2[] = "@(#) $Revision: 1.1 $";
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
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
#include "spuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xmspu_ext.h"
#include "xmmenu.h"
#ifdef	SHAREDLIBS
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"
#include "displayopt.h"
#endif

FILE	*Cfile;

uid_t	Daemuid,
	Realuid,
	Effuid;

#ifdef	SHAREDLIBS
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
DEF_DISPOPTS;
#endif

int	arr_rtime,
	arr_rint;

int	hchanges,	/* Had changes to default */
	uchanges;	/* Had changes to user(s) */

char		alphsort;
unsigned	Nusers;
extern	struct	sphdr	Spuhdr;
struct	spdet	*ulist;
static	char		*defhdr,
			*s_class,
			*ns_class,
			*lt_class,
			*gt_class,
			*s_perm,
			*ns_perm,
			*lt_perm,
			*gt_perm;

static	char	*urestrict;	/* Restrict  */

#define	USNAM_COL	0
#define	DEFPRI_COL	8
#define	MINPRI_COL	12
#define	MAXPRI_COL	16
#define	COPIES_COL	20
#define	DEFFORM_COL	24
#define	DEFPTR_COL	42
#define	CLASS_COL	58
#define	PRIV_COL	70

/* X Stuff */

XtAppContext	app;
Display		*dpy;

Widget	toplevel,	/* Main window */
	dwid,		/* Default list */
	uwid;		/* User scroll list */

static	Widget	panedw,		/* Paned window to stick rest in */
		menubar;	/* Menu */

typedef	struct	{
	Boolean tit_pres;
	Boolean footer_pres;
	Boolean	sort_alpha;
	String	onlyuser;
	int	rtime, rint;
}  vrec_t;

static void	cb_about(void);
static void	cb_quit(Widget, int);
static void	cb_saveopts(Widget);

static	XtResource	resources[] = {
	{ "titlePresent", "TitlePresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, tit_pres), XtRImmediate, False },
	{ "footerPresent", "FooterPresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, footer_pres), XtRImmediate, False },
	{ "sortAlpha", "SortAlpha", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, sort_alpha), XtRImmediate, False },
	{ "onlyUser", "OnlyUser", XtRString, sizeof(String),
		  XtOffsetOf(vrec_t, onlyuser), XtRString, "" },
	{ "repeatTime", "RepeatTime", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, rtime), XtRImmediate, (XtPointer) 500 },
	{ "repeatInt", "RepeatInt", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, rint), XtRImmediate, (XtPointer) 100 }};

static	casc_button
opt_casc[] = {
	{	ITEM,	"Disporder",	cb_disporder,	0	},
	{	ITEM,	"Saveopts",	cb_saveopts,	0	},
	{	DSEP	},
	{	ITEM,	"Quit",		cb_quit,	0	}},
def_casc[] = {
	{	ITEM,	"dpri",		cb_pris,	0	},
	{	ITEM,	"dform",	cb_formetc,	FORMETC_DFORM	},
	{	ITEM,	"dptr",		cb_formetc,	FORMETC_DPTR	},
	{	ITEM,	"dforma",	cb_formetc,	FORMETC_DFORMA	},
	{	ITEM,	"dptra",	cb_formetc,	FORMETC_DPTRA	},
	{	SEP	},
	{	ITEM,	"dclass",	cb_class,	0	},
	{	ITEM,	"dpriv",	cb_priv,	0	},
	{	SEP	},
	{	ITEM,	"defcpy",	cb_copydef,	0	}},
user_casc[] = {
	{	ITEM,	"upri",		cb_pris,	1	},
	{	ITEM,	"uform",	cb_formetc,	FORMETC_UFORM	},
	{	ITEM,	"uptr",		cb_formetc,	FORMETC_UPTR	},
	{	ITEM,	"uforma",	cb_formetc,	FORMETC_UFORMA	},
	{	ITEM,	"uptra",	cb_formetc,	FORMETC_UPTRA	},
	{	SEP	},
	{	ITEM,	"uclass",	cb_class,	1	},
	{	ITEM,	"upriv",	cb_priv,	1	},
	{	SEP	},
	{	ITEM,	"ucpy",		cb_copydef,	1	}},
chrg_casc[] = {
	{	ITEM,	"Display",	cb_cdisplay,	0	},
	{	SEP	},
	{	ITEM,	"Zero",		cb_zeroc,	0	},
	{	ITEM,	"Zeroall",	cb_zeroc,	1	},
	{	ITEM,	"Impose",	cb_impose,	0	}},
search_casc[] = {
	{	ITEM,	"Search",	cb_srchfor,	0	},
	{	ITEM,	"Searchforw",	cb_rsrch,	0	},
	{	ITEM,	"Searchback",	cb_rsrch,	1	}},
help_casc[] = {
	{	ITEM,	"Help",		dohelp,		$H{xmspuser main screen help}	},
	{	ITEM,	"Helpon",	cb_chelp,	0	},
	{	SEP	},
	{	ITEM,	"About",	cb_about,	0	}};

static	pull_button
	opt_button = {
		"Options",	XtNumber(opt_casc),	$H{xmspuser options help},	opt_casc	},
	def_button = {
		"Defaults",	XtNumber(def_casc),	$H{xmspuser defaults menu help},def_casc	},
	user_button = {
		"Users",	XtNumber(user_casc),	$H{xmspuser users menu help},	user_casc	},
	chrg_button = {
		"Charges",	XtNumber(chrg_casc),	$H{xmspuser charges help},	chrg_casc	},
	srch_button = {
		"Search",	XtNumber(search_casc),	$H{xmspuser search help},	search_casc	},
	help_button = {
		"Help",		XtNumber(help_casc),	$H{xmspuser help help},	help_casc,	1	};

static	pull_button	*menlist[] = {
	&opt_button,	&def_button,	&user_button,
	&chrg_button,	&srch_button,	&help_button
};

#if	defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY)

/* Define our own bcopy and bzero because X uses these in places and
   we don't want to include some -libucb which pulls in funny
   sprintfs etc */

void	bcopy(void * from, void * to, unsigned count)
{
	memcpy(to, from, count);
}

void	bzero(void *to, unsigned count)
{
	memset(to, '\0', count);
}
#endif

/* Different sorts of sorts (of sorts) */

int	sort_u(struct spdet *a, struct spdet *b)
{
	return  strcmp(prin_uname((uid_t) a->spu_user), prin_uname((uid_t) b->spu_user));
}

int	sort_id(struct spdet *a, struct spdet *b)
{
	return  (ULONG) a->spu_user < (ULONG) b->spu_user ? -1: (ULONG) a->spu_user == (ULONG) b->spu_user? 0: 1;
}

/* Don't put exit as a callback or we'll get some weird exit code
   based on a Widget pointer.  */

static void	cb_quit(Widget w, int n)
{
	if  (uchanges || hchanges)  {
		if  (alphsort)
			qsort(QSORTP1 ulist, Nusers, sizeof(struct spdet), QSORTP4 sort_id);
		putspulist(ulist, Nusers, hchanges);
	}
	exit(n);
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
	fprintf(tf, "%s.sortAlpha:\t%s\n", progname, alphsort? "True": "False");
	if  (urestrict)
		fprintf(tf, "%s.onlyUser:\t%s\n", progname, urestrict);
	XtVaGetValues(uwid, XmNwidth, &wid, XmNvisibleItemCount, &items, NULL);
	fprintf(tf, "%s*ulist.width: %ld\n", progname, (long) wid);
	fprintf(tf, "%s*ulist.visibleItemCount: %ld\n", progname, (long) items);
	SWAP_TO(Realuid);
	oldumask = umask(0);
	umask(oldumask & ~0444);
	if  (!(inf = fopen(hf, "w")))  {
		SWAP_TO(Daemuid);
		doerror(w, $EH{xmspq app-default cannot open});
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
	exit(E_NOMEM);
}

static void	cb_about(void)
{
	Widget		dlg;
	char	buf[sizeof(rcsid1) + sizeof(rcsid2) + 2];
	sprintf(buf, "%s\n%s", rcsid1, rcsid2);
	dlg = XmCreateInformationDialog(uwid, "about", NULL, 0);
	XtVaSetValues(dlg,
		      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		      XtVaTypedArg, XmNmessageString, XmRString, buf, strlen(buf),
		      NULL);
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
	XtManageChild(dlg);
	XtPopup(XtParent(dlg), XtGrabNone);
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
				XtAddCallback(button, XmNactivateCallback, cb->callback, INT_TO_XTPOINTER(cb->callback_data));
			continue;
		}
	}
	if  (item->ishelp)
		return  cascade;
	return  NULL;
}

static void setup_macros(Widget	menub, const int helpcode, const int helpbase, char *pullname, XtCallbackProc macroproc)
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

	setup_macros(menubar, $H{xmspuser macro help}, $H{Job or User macro}, "usermacro", (XtCallbackProc) cb_macrou);
	XtManageChild(menubar);
}

static void	maketitle(char * tname)
{
	Widget			labv;
	XtWidgetGeometry	size;

	labv = XtVaCreateManagedWidget(tname, xmLabelWidgetClass, panedw, NULL);
	size.request_mode = CWHeight;
	XtQueryGeometry(labv, NULL, &size);
	XtVaSetValues(labv, XmNpaneMaximum, size.height, XmNpaneMinimum, size.height, NULL);
}

#include "xmspuser.bm"

static void	wstart(int argc, char **argv)
{
	vrec_t	vrec;
	Pixmap	bitmap;
	XtWidgetGeometry	size;

	toplevel = XtVaAppInitialize(&app, "GSPOOL", NULL, 0, &argc, argv, NULL, NULL);
	if  (argc > 1  &&  strcmp(argv[1], "*") != 0)
		urestrict = stracpy(argv[1]);
	XtGetApplicationResources(toplevel, &vrec, resources, XtNumber(resources), NULL, 0);
	bitmap = XCreatePixmapFromBitmapData(dpy = XtDisplay(toplevel),
					     RootWindowOfScreen(XtScreen(toplevel)),
					     xmspuser_bits, xmspuser_width, xmspuser_height, 1, 0, 1);
	XtVaSetValues(toplevel, XmNiconPixmap, bitmap, NULL);

	/* Set up parameters from resources */

	alphsort = vrec.sort_alpha;
	if  (vrec.onlyuser[0]  &&  !urestrict)
		urestrict = stracpy(vrec.onlyuser);
	arr_rtime = vrec.rtime;
	arr_rint = vrec.rint;

	/* Now to create all the bits of the application */

	panedw = XtVaCreateWidget("layout", xmPanedWindowWidgetClass, toplevel, NULL);

	setup_menus();

	dwid = XtVaCreateManagedWidget("dlist",
				       xmListWidgetClass,	panedw,
				       XmNvisibleItemCount,	1,
				       XmNselectionPolicy,	XmSINGLE_SELECT,
				       NULL);
	size.request_mode = CWHeight;
	XtQueryGeometry(dwid, NULL, &size);
	XtVaSetValues(dwid, XmNpaneMaximum, size.height, XmNpaneMinimum, size.height, NULL);
	XtAddCallback(dwid, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER($H{xmspuser defaults help}));
	if  (vrec.tit_pres)
		maketitle("utitle");

	uwid = XmCreateScrolledList(panedw, "ulist", NULL, 0);
	XtVaSetValues(uwid,
		      XmNselectionPolicy,	XmEXTENDED_SELECT,
		      NULL);
	XtAddCallback(uwid, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER($H{xmspuser users help}));
	XtManageChild(uwid);

	if  (vrec.footer_pres)
		maketitle("footer");

	XtManageChild(panedw);
	XtRealizeWidget(toplevel);
	s_class = gprompt($P{Class std});
	ns_class = gprompt($P{Non std class});
	lt_class = gprompt($P{Class less than});
	gt_class = gprompt($P{Class greater than});
	s_perm = gprompt($P{Perm std});
	ns_perm = gprompt($P{Non std perm});
	lt_perm = gprompt($P{Perm less than});
	gt_perm = gprompt($P{Perm greater than});
	defhdr = gprompt($P{Spuser default string});
}

/* Copy but avoid copying trailing null */

#define	movein(to, from)	BLOCK_COPY(to, from, strlen(from))
#ifdef	HAVE_MEMCPY
#define	BLOCK_SET(to, n, ch)	memset((void *) to, ch, (unsigned) n)
#else
static void	BLOCK_SET(char * to, unsigned n, const char ch)
{
	while  (n != 0)
		*to++ = ch;
}
#endif

void	defdisplay(void)
{
	int	outl;
	XmString	str;
	char	obuf[100], nbuf[16];

	BLOCK_SET(obuf, sizeof(obuf), ' ');
	movein(&obuf[USNAM_COL], defhdr);
	sprintf(nbuf, "%3d", (int) Spuhdr.sph_defp);
	movein(&obuf[DEFPRI_COL], nbuf);
	sprintf(nbuf, "%3d", (int) Spuhdr.sph_minp);
	movein(&obuf[MINPRI_COL], nbuf);
	sprintf(nbuf, "%3d", (int) Spuhdr.sph_maxp);
	movein(&obuf[MAXPRI_COL], nbuf);
	sprintf(nbuf, "%3d", (int) Spuhdr.sph_cps);
	movein(&obuf[COPIES_COL], nbuf);
	movein(&obuf[DEFFORM_COL], Spuhdr.sph_form);
	movein(&obuf[DEFPTR_COL], Spuhdr.sph_ptr);

	/* Trim trailing spaces */

	for  (outl = sizeof(obuf) - 1;  outl >= 0  &&  obuf[outl] == ' ';  outl--)
		;
	obuf[outl+1] = '\0';
	str = XmStringCreateLocalized(obuf);
	XmListDeleteAllItems(dwid);
	XmListAddItem(dwid, str, 0);
	XmStringFree(str);
}

static	XmString	fillbuffer(
	 char * buff, 
	 unsigned buffsize, 
	 struct spdet * uitem)
{
	int	outl;
	ULONG	exclp;
	classcode_t	exclc;
	char	*msg;
	char	nbuf[20];

	BLOCK_SET(buff, buffsize, ' ');
	movein(&buff[USNAM_COL], prin_uname((uid_t) uitem->spu_user));
	sprintf(nbuf, "%3d", (int) uitem->spu_defp);
	movein(&buff[DEFPRI_COL], nbuf);
	sprintf(nbuf, "%3d", (int) uitem->spu_minp);
	movein(&buff[MINPRI_COL], nbuf);
	sprintf(nbuf, "%3d", (int) uitem->spu_maxp);
	movein(&buff[MAXPRI_COL], nbuf);
	sprintf(nbuf, "%3d", (int) uitem->spu_cps);
	movein(&buff[COPIES_COL], nbuf);
	movein(&buff[DEFFORM_COL], uitem->spu_form);
	movein(&buff[DEFPTR_COL], uitem->spu_ptr);
	msg = s_class;
	exclc = uitem->spu_class ^ Spuhdr.sph_class;
	if  (exclc != 0)  {
		msg = ns_class;
		if  ((exclc & Spuhdr.sph_class) == 0)
			msg = gt_class;
		else  if  ((exclc & ~Spuhdr.sph_class) == 0)
			msg = lt_class;
	}
	movein(&buff[CLASS_COL], msg);
	msg = s_perm;
	exclp = uitem->spu_flgs ^ Spuhdr.sph_flgs;
	if  (exclp != 0)  {
		msg = ns_perm;
		if  ((exclp & Spuhdr.sph_flgs) == 0)
			msg = gt_perm;
		else  if  ((exclp & ~Spuhdr.sph_flgs) == 0)
			msg = lt_perm;
	}
	movein(&buff[PRIV_COL], msg);
	for  (outl = buffsize - 1;  outl >= 0  &&  buff[outl] == ' ';  outl--)
		;
	buff[outl+1] = '\0';
	return  XmStringCreateLocalized(buff);
}

void	udisplay(int nu, int *posns)
{
	int		ucnt;
	XmString	str;
	char		obuf[100];

	if  (nu <= 0)  {
		XmListDeleteAllItems(uwid);
		for  (ucnt = 0;  ucnt < Nusers;  ucnt++)  {
			str = fillbuffer(obuf, sizeof(obuf), &ulist[ucnt]);
			XmListAddItem(uwid, str, 0);
			XmStringFree(str);
		}
	}
	else  {
		if  (nu > 1)
			XtVaSetValues(uwid, XmNselectionPolicy,	XmMULTIPLE_SELECT, NULL);
		for  (ucnt = 0;  ucnt < nu;  ucnt++)  {
			str = fillbuffer(obuf, sizeof(obuf), &ulist[posns[ucnt]-1]);
			XmListReplaceItemsPos(uwid, &str, 1, posns[ucnt]);
			XmStringFree(str);
			XmListSelectPos(uwid, posns[ucnt], False);
		}
		if  (nu > 1)
			XtVaSetValues(uwid, XmNselectionPolicy,	XmEXTENDED_SELECT, NULL);
	}
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	struct	spdet	*mypriv;
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	Realuid = getuid();
	Effuid = geteuid();
	INIT_DAEMUID;
	Cfile = open_cfile("XMSPUSERCONF", "xmspuser.help");
	SCRAMBLID_CHECK
	SWAP_TO(Daemuid);
	wstart(argc, argv);
	if  (!(mypriv = getspuentry(Realuid)))  {
		doerror(toplevel, $EH{Not registered yet});
		exit(E_UNOTSETUP);
	}
	if  (!(mypriv->spu_flgs & PV_ADMIN))  {
		doerror(toplevel, $EH{No admin file permission});
		exit(E_NOPRIV);
	}
	if  (spu_needs_rebuild && Confirm(toplevel, $PH{Xmspuser confirm rebuild}))  {
		char  *name = envprocess(DUMPPWFILE);
		int	wuz = access(name, 0);
		if  (wuz >= 0)  {
			un_rpwfile();
			unlink(name);
		}
		free(name);
		displaybusy(1);
		rebuild_spufile();
		if  (wuz >= 0)
			dump_pwfile();
		produser();
		displaybusy(0);
	}
	ulist = getspulist(&Nusers);

	/* Chop down list to ones we want */

	if  (Nusers != 0  &&  urestrict)  {
		unsigned   cnt, nucnt = 0;
		struct  spdet  *cp, *np;
		struct  spdet  *newulist = (struct spdet *) malloc(Nusers * sizeof(struct spdet));
		if  (!newulist)
			nomem();
		cp = ulist;
		np = newulist;
		for  (cnt = 0;  cnt < Nusers;  cp++, cnt++)  {
			char	*un = prin_uname((uid_t) cp->spu_user);
			if  (urestrict && !qmatch(urestrict, un))
				continue;
			*np++ = *cp;
			nucnt++;
		}
		free((char *) ulist);
		ulist = newulist;
		Nusers = nucnt;
	}

	if  (alphsort)
		qsort(QSORTP1 ulist, Nusers, sizeof(struct spdet), QSORTP4 sort_u);
	defdisplay();
	udisplay(0, (int *) 0);
	XtAppMainLoop(app);
	return  0;
}
