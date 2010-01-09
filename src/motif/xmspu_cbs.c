/* xmspu_cbs.c -- xmspuser callback routines

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
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#include <X11/cursorfont.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include "defaults.h"
#include "spuser.h"
#include "files.h"
#include "ecodes.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "xmspu_ext.h"
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

Widget	workw[23];
static	Widget	ccws[32];

/* Positions (in the motif sense) of users we are thinking of mangling
   If null/zero then we are looking at the default list.  */

static	int	pendunum,
		*pendulist;

Widget	GetTopShell(Widget w)
{
	while  (w && !XtIsWMShell(w))
		w = XtParent(w);
	return  w;
}

Widget	FindWidget(Widget w)
{
	while  (w && !XtIsWidget(w))
		w = XtParent(w);
	return  w;
}

static char *	makebigvec(char ** mat)
{
	unsigned  totlen = 0, len;
	char	**ep, *newstr, *pos;

	for  (ep = mat;  *ep;  ep++)
		totlen += strlen(*ep) + 1;

	newstr = malloc((unsigned) totlen);
	if  (!newstr)
		nomem();
	pos = newstr;
	for  (ep = mat;  *ep;  ep++)  {
		len = strlen(*ep);
		strcpy(pos, *ep);
		free(*ep);
		pos += len;
		*pos++ = '\n';
	}
	pos[-1] = '\0';
	free((char *) mat);
	return  newstr;
}

void	dohelp(Widget wid, int helpcode)
{
	char	**evec = helpvec(helpcode, 'H'), *newstr;
	Widget		ew;
	if  (!evec[0])  {
		disp_arg[0] = helpcode;
		free((char *) evec);
		evec = helpvec($E{Missing help code}, 'E');
	}
	ew = XmCreateInformationDialog(FindWidget(wid), "help", NULL, 0);
	XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_HELP_BUTTON));
	newstr = makebigvec(evec);
	XtVaSetValues(ew,
		      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		      XtVaTypedArg, XmNmessageString, XmRString, newstr, strlen(newstr),
		      NULL);
	free(newstr);
	XtManageChild(ew);
	XtPopup(XtParent(ew), XtGrabNone);
}

void	doerror(Widget wid, int errnum)
{
	char	**evec = helpvec(errnum, 'E'), *newstr;
	Widget		ew;
	if  (!evec[0])  {
		disp_arg[0] = errnum;
		free((char *) evec);
		evec = helpvec($E{Missing error code}, 'E');
	}
	ew = XmCreateErrorDialog(FindWidget(wid), "error", NULL, 0);
	XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_CANCEL_BUTTON));
	XtAddCallback(ew, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(errnum));
	newstr = makebigvec(evec);
	XtVaSetValues(ew,
		      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		      XtVaTypedArg, XmNmessageString, XmRString, newstr, strlen(newstr),
		      NULL);
	free(newstr);
	XtManageChild(ew);
	XtPopup(XtParent(ew), XtGrabNone);
}

void	displaybusy(const int on)
{
	static	Cursor	cursor;
	XSetWindowAttributes	attrs;
	if  (!cursor)
		cursor = XCreateFontCursor(dpy, XC_watch);
	attrs.cursor = on? cursor: None;
	XChangeWindowAttributes(dpy, XtWindow(toplevel), CWCursor, &attrs);
	XFlush(dpy);
}

static void	response(Widget w, int * answer, XmAnyCallbackStruct * cbs)
{
	switch  (cbs->reason)  {
	case  XmCR_OK:
		*answer = 1;
		break;
	case  XmCR_CANCEL:
		*answer = 0;
		break;
	}
	XtDestroyWidget(w);
}

int	Confirm(Widget parent, int code)
{
	Widget	dlg;
	static	int	answer;
	char	*msg;
	XmString	text;

	dlg = XmCreateQuestionDialog(FindWidget(parent), "Confirm", NULL, 0);
	XtVaSetValues(dlg,
		      XmNdialogStyle,		XmDIALOG_FULL_APPLICATION_MODAL,
		      NULL);
	XtAddCallback(dlg, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(code));
	XtAddCallback(dlg, XmNokCallback, (XtCallbackProc) response, &answer);
	XtAddCallback(dlg, XmNcancelCallback, (XtCallbackProc) response, &answer);
	answer = -1;
	msg = gprompt(code);
	text = XmStringCreateLocalized(msg);
	free(msg);
	XtVaSetValues(dlg,
		      XmNmessageString,		text,
		      XmNdefaultButtonType,	XmDIALOG_CANCEL_BUTTON,
		      NULL);
	XmStringFree(text);
	XtManageChild(dlg);
	XtPopup(XtParent(dlg), XtGrabNone);
	while  (answer < 0)
		XtAppProcessEvent(app, XtIMAll);
	return  answer;
}

static int	getselectedusers(const int moan)
{
	int	nu, *nulist;

	if  (XmListGetSelectedPos(uwid, &nulist, &nu))  {
		if  (nu <= 0)  {
			XtFree((char *) nulist);
			pendunum = 0;
			pendulist = (int *) 0;
			if  (moan)
				doerror(uwid, $EH{No users selected});
			return  0;
		}
		pendunum = nu;
		pendulist = nulist;
		return  1;
	}
	else  {
		if  (moan)
			doerror(uwid, $EH{No users selected});
		pendunum = 0;
		pendulist = (int *) 0;
		return  0;
	}
}

/* Create the stuff at the beginning of a dialog */

void  CreateEditDlg(Widget parent, char *dlgname, Widget *dlgres, Widget	*paneres, Widget *formres, const int nbutts)
{
	int	n = 0;
	Arg	arg[7];
	static	XmString oks, cancs, helps;

	if  (!oks)  {
		oks = XmStringCreateLocalized("Ok");
		cancs = XmStringCreateLocalized("Cancel");
		helps = XmStringCreateLocalized("Help");
	}
        XtSetArg(arg[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
	XtSetArg(arg[n], XmNdeleteResponse, XmDESTROY); n++;
	XtSetArg(arg[n], XmNokLabelString, oks);	n++;
	XtSetArg(arg[n], XmNcancelLabelString, cancs);	n++;
	XtSetArg(arg[n], XmNhelpLabelString, helps);	n++;
	XtSetArg(arg[n], XmNautoUnmanage, False);	n++;
        *dlgres = *paneres = XmCreateTemplateDialog(GetTopShell(parent), dlgname, arg, n);
	*formres = XtVaCreateWidget("form", xmFormWidgetClass, *dlgres, XmNfractionBase, 2 * nbutts, NULL);
}

/* Create the stuff at the end of a dialog.  */

void  CreateActionEndDlg(Widget shelldlg, Widget panew, XtCallbackProc endrout, int helpcode)
{
	XtAddCallback(shelldlg, XmNokCallback, endrout, INT_TO_XTPOINTER(1));
        XtAddCallback(shelldlg, XmNcancelCallback, endrout, INT_TO_XTPOINTER(0));
        XtAddCallback(shelldlg, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(helpcode));
        XtManageChild(shelldlg);
}

/* Create the stuff at the beginning a dialog which can relate to a
   default value or a (group of) user(s).  */

static Widget CreateUEditDlg(Widget parent, char *dlgname,  Widget *dlgres, Widget *paneres, Widget *formres)
{
	char	resname[UIDSIZE*2 + 10];

	sprintf(resname, "%s%s", pendunum > 0? "u": "def", dlgname);
	CreateEditDlg(parent, resname, dlgres, paneres, formres, 3);
	if  (pendunum > 0)  {
		Widget	tit, result;
		tit = XtVaCreateManagedWidget("useredit",
					      xmLabelWidgetClass,		*formres,
					      XmNtopAttachment,			XmATTACH_FORM,
					      XmNleftAttachment,		XmATTACH_FORM,
					      XmNborderWidth,			0,
					      NULL);
		result = XtVaCreateManagedWidget("userlist",
						 xmTextFieldWidgetClass,	*formres,
						 XmNcursorPositionVisible,	False,
						 XmNtopAttachment,		XmATTACH_FORM,
						 XmNleftAttachment,		XmATTACH_WIDGET,
						 XmNleftWidget,			tit,
						 XmNrightAttachment,		XmATTACH_FORM,
						 XmNeditable,			False,
						 XmNborderWidth,		0,
						 NULL);
		if  (pendunum > 1)
			sprintf(resname, "%s - %s",
				       prin_uname((uid_t) ulist[pendulist[0] - 1].spu_user),
				       prin_uname((uid_t) ulist[pendulist[pendunum-1] - 1].spu_user));
		else
			sprintf(resname, "%s",
				       prin_uname((uid_t) ulist[pendulist[0] - 1].spu_user));
		XmTextSetString(result, resname);
		return  result;
	}
	else
		return  XtVaCreateManagedWidget("defedit",
						xmLabelWidgetClass,		*formres,
						XmNtopAttachment,		XmATTACH_FORM,
						XmNleftAttachment,		XmATTACH_FORM,
						XmNborderWidth,			0,
						NULL);
}

static void	enddispopt(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		int	ret;
		ret = XmToggleButtonGadgetGetState(workw[WORKW_SORTA])? 1: 0;
		if  (ret != alphsort)  {
			alphsort = (char) ret;
			if  (alphsort)
				qsort(QSORTP1 ulist, Nusers, sizeof(struct spdet), QSORTP4 sort_u);
			else
				qsort(QSORTP1 ulist, Nusers, sizeof(struct spdet), QSORTP4 sort_id);
			udisplay(0, (int *) 0);
		}
	}
	XtDestroyWidget(GetTopShell(w));
}

void	cb_disporder(Widget parent)
{
	Widget	disp_shell, paneview, mainform, butts, sortu;

	CreateEditDlg(parent, "Disporder", &disp_shell, &paneview, &mainform, 3);
	butts = XtVaCreateManagedWidget("butts",
					xmRowColumnWidgetClass, mainform,
					XmNtopAttachment,	XmATTACH_FORM,
					XmNleftAttachment,	XmATTACH_FORM,
					XmNrightAttachment,	XmATTACH_FORM,
					XmNpacking,		XmPACK_COLUMN,
					XmNnumColumns,		1,
					XmNisHomogeneous,	True,
					XmNentryClass,		xmToggleButtonGadgetClass,
					XmNradioBehavior,	True,
					NULL);

	sortu = XtVaCreateManagedWidget("sortuid",
					xmToggleButtonGadgetClass,
					butts,
					NULL);
	workw[WORKW_SORTA] = XtVaCreateManagedWidget("sortname",
						     xmToggleButtonGadgetClass,
						     butts,
						     NULL);

	XmToggleButtonGadgetSetState(alphsort? workw[WORKW_SORTA]: sortu, True, False);
	XtManageChild(mainform);
	CreateActionEndDlg(disp_shell, paneview, (XtCallbackProc) enddispopt, $H{xmspuser action help});
}

/* Handle "linked" privileges */

static void	changepriv(Widget w, int wpriv, XmToggleButtonCallbackStruct * cbs)
{
	if  (cbs->set)  {
		int	cnt;
		switch  (wpriv)  {
		case  PV_ANYPRIO:
			XmToggleButtonGadgetSetState(workw[WORKW_CPRIO], True, False);
			break;
		case  PV_ADDDEL:
			XmToggleButtonGadgetSetState(workw[WORKW_HALTGO], True, False);
		case  PV_HALTGO:
			XmToggleButtonGadgetSetState(workw[WORKW_PRINQ], True, False);
			break;
		case  PV_ADMIN:
			for  (cnt = 1;  cnt < NUM_PRIVS;  cnt++)
				XmToggleButtonGadgetSetState(workw[cnt + WORKW_ADMIN], True, False);
			break;
		case  PV_OTHERJ:
			XmToggleButtonGadgetSetState(workw[WORKW_VOTHERJ], True, False);
			break;
		case  PV_FREEZEOK:
			XmToggleButtonGadgetSetState(workw[WORKW_ACCESSOK], True, False);
			break;
		}
	}
	else  {
		switch  (wpriv)  {
		case  PV_CPRIO:
			XmToggleButtonGadgetSetState(workw[WORKW_ANYPRIO], False, False);
			break;
		case  PV_PRINQ:
			XmToggleButtonGadgetSetState(workw[WORKW_HALTGO], False, False);
		case  PV_HALTGO:
			XmToggleButtonGadgetSetState(workw[WORKW_ADDDEL], False, False);
			break;
		case  PV_VOTHERJ:
			XmToggleButtonGadgetSetState(workw[WORKW_OTHERJ], False, False);
			break;
		case  PV_ACCESSOK:
			XmToggleButtonGadgetSetState(workw[WORKW_FREEZEOK], False, False);
			break;
		}
	}
}

static void	endprio(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		int		cmin, cmax, cdef, ccps;
		ULONG	pchanges = 0;
		XmScaleGetValue(workw[WORKW_MINPW], &cmin);
		XmScaleGetValue(workw[WORKW_MAXPW], &cmax);
		XmScaleGetValue(workw[WORKW_DEFPW], &cdef);
		XmScaleGetValue(workw[WORKW_CPSW], &ccps);
		if  (cmin > cmax)  {
			doerror(w, $EH{xmspuser min gt max});
			return;
		}
		if  (cdef < cmin)  {
			doerror(w, $EH{xmspuser def lt min});
			return;
		}
		if  (cdef > cmax)  {
			doerror(w, $EH{xmspuser def gt max});
			return;
		}
		if  (XmToggleButtonGadgetGetState(workw[WORKW_CPRIO]))
		     pchanges |= PV_CPRIO;
		if  (XmToggleButtonGadgetGetState(workw[WORKW_ANYPRIO]))
		     pchanges |= PV_ANYPRIO;
		if  (XmToggleButtonGadgetGetState(workw[WORKW_CDEFLT]))
		     pchanges |= PV_CDEFLT;
		if  (pendunum > 0)  {
			int	ucnt;
			struct	spdet	*uitem;
			uchanges++;
			for  (ucnt = 0;  ucnt < pendunum;  ucnt++)  {
				uitem = &ulist[pendulist[ucnt]-1];
				uitem->spu_minp = (unsigned char) cmin;
				uitem->spu_maxp = (unsigned char) cmax;
				uitem->spu_defp = (unsigned char) cdef;
				uitem->spu_cps = (unsigned char) ccps;
				uitem->spu_flgs &= ~(PV_CPRIO|PV_ANYPRIO|PV_CDEFLT);
				uitem->spu_flgs |= pchanges;
			}
			udisplay(pendunum, pendulist);
		}
		else  {
			ULONG oldflags = Spuhdr.sph_flgs;
			hchanges++;
			Spuhdr.sph_minp = (unsigned char) cmin;
			Spuhdr.sph_maxp = (unsigned char) cmax;
			Spuhdr.sph_defp = (unsigned char) cdef;
			Spuhdr.sph_cps = (unsigned char) ccps;
			Spuhdr.sph_flgs &= ~(PV_CPRIO|PV_ANYPRIO|PV_CDEFLT);
			Spuhdr.sph_flgs |= pchanges;
			if  (oldflags != Spuhdr.sph_flgs)
				udisplay(0, (int *) 0);
			defdisplay();
		}
	}
	XtDestroyWidget(GetTopShell(w));
}

/* Priorities which = 0 to set default, 1 to selected users */

void	cb_pris(Widget parent, int which)
{
	Widget	pri_shell, paneview, mainform, prevabove;
	int	cmin, cmax, cdef, ccps;
	ULONG	cchangep, canyprio, cchdeflt;

	if  (pendunum > 0)  {
		pendunum = 0;
		XtFree((char *) pendulist);
		pendulist = (int *) 0;
	}

	if  (which)  {
		struct	spdet	*fu;
		if  (!getselectedusers(1))
			return;
		fu = &ulist[pendulist[0]-1];
		cmin = fu->spu_minp;
		cmax = fu->spu_maxp;
		cdef = fu->spu_defp;
		ccps = fu->spu_cps;
		cchangep = fu->spu_flgs & PV_CPRIO;
		canyprio = fu->spu_flgs & PV_ANYPRIO;
		cchdeflt = fu->spu_flgs & PV_CDEFLT;
	}
	else  {
		cmin = Spuhdr.sph_minp;
		cmax = Spuhdr.sph_maxp;
		cdef = Spuhdr.sph_defp;
		ccps = Spuhdr.sph_cps;
		cchangep = Spuhdr.sph_flgs & PV_CPRIO;
		canyprio = Spuhdr.sph_flgs & PV_ANYPRIO;
		cchdeflt = Spuhdr.sph_flgs & PV_CDEFLT;
	}

	prevabove = CreateUEditDlg(parent, "pri", &pri_shell, &paneview, &mainform);

	prevabove = XtVaCreateManagedWidget("min",
					    xmLabelGadgetClass,	mainform,
					    XmNtopAttachment,	XmATTACH_WIDGET,
					    XmNtopWidget,	prevabove,
					    XmNleftAttachment,	XmATTACH_FORM,
					    NULL);

	prevabove =
		workw[WORKW_MINPW] =
			XtVaCreateManagedWidget("Minp",
						xmScaleWidgetClass,	mainform,
						XmNtopAttachment,	XmATTACH_WIDGET,
						XmNtopWidget,		prevabove,
						XmNleftAttachment,	XmATTACH_FORM,
						XmNrightAttachment,	XmATTACH_FORM,
						XmNorientation,		XmHORIZONTAL,
						XmNminimum,		1,
						XmNmaximum,		255,
						XmNvalue,		cmin,
						XmNshowValue,		True,
						NULL);

	prevabove = XtVaCreateManagedWidget("def",
					    xmLabelGadgetClass,	mainform,
					    XmNtopAttachment,	XmATTACH_WIDGET,
					    XmNtopWidget,	prevabove,
					    XmNleftAttachment,	XmATTACH_FORM,
					    NULL);

	prevabove =
		workw[WORKW_DEFPW] =
			XtVaCreateManagedWidget("Defp",
						xmScaleWidgetClass,	mainform,
						XmNtopAttachment,	XmATTACH_WIDGET,
						XmNtopWidget,		prevabove,
						XmNleftAttachment,	XmATTACH_FORM,
						XmNrightAttachment,	XmATTACH_FORM,
						XmNorientation,		XmHORIZONTAL,
						XmNminimum,		1,
						XmNmaximum,		255,
						XmNvalue,		cdef,
						XmNshowValue,		True,
						NULL);

	prevabove = XtVaCreateManagedWidget("max",
					    xmLabelGadgetClass,	mainform,
					    XmNtopAttachment,	XmATTACH_WIDGET,
					    XmNtopWidget,	prevabove,
					    XmNleftAttachment,	XmATTACH_FORM,
					    NULL);

	prevabove =
		workw[WORKW_MAXPW] =
			XtVaCreateManagedWidget("Maxp",
						xmScaleWidgetClass,	mainform,
						XmNtopAttachment,	XmATTACH_WIDGET,
						XmNtopWidget,		prevabove,
						XmNleftAttachment,	XmATTACH_FORM,
						XmNrightAttachment,	XmATTACH_FORM,
						XmNorientation,		XmHORIZONTAL,
						XmNminimum,		1,
						XmNmaximum,		255,
						XmNvalue,		cmax,
						XmNshowValue,		True,
						NULL);

	prevabove = XtVaCreateManagedWidget("copies",
					    xmLabelGadgetClass,	mainform,
					    XmNtopAttachment,	XmATTACH_WIDGET,
					    XmNtopWidget,	prevabove,
					    XmNleftAttachment,	XmATTACH_FORM,
					    NULL);

	workw[WORKW_CPSW] = XtVaCreateManagedWidget("cps",
						    xmScaleWidgetClass,		mainform,
						    XmNtopAttachment,		XmATTACH_WIDGET,
						    XmNtopWidget,		prevabove,
						    XmNleftAttachment,		XmATTACH_FORM,
						    XmNrightAttachment,		XmATTACH_FORM,
						    XmNorientation,		XmHORIZONTAL,
						    XmNminimum,			0,
						    XmNmaximum,			255,
						    XmNvalue,			ccps,
						    XmNshowValue,		True,
						    NULL);

	workw[WORKW_CPRIO] = XtVaCreateManagedWidget("changep",
						     xmToggleButtonGadgetClass,		mainform,
						     XmNtopAttachment,			XmATTACH_WIDGET,
						     XmNtopWidget,			workw[WORKW_CPSW],
						     XmNleftAttachment,			XmATTACH_FORM,
						     XmNborderWidth,			0,
						     NULL);

	workw[WORKW_ANYPRIO] = XtVaCreateManagedWidget("anyprio",
						      xmToggleButtonGadgetClass,	mainform,
						      XmNtopAttachment,			XmATTACH_WIDGET,
						      XmNtopWidget,			workw[WORKW_CPRIO],
						      XmNleftAttachment,		XmATTACH_FORM,
						      XmNborderWidth,			0,
						      NULL);

	if  (cchangep)
		XmToggleButtonGadgetSetState(workw[WORKW_CPRIO], True, False);
	if  (canyprio)
		XmToggleButtonGadgetSetState(workw[WORKW_ANYPRIO], True, False);

	XtAddCallback(workw[WORKW_CPRIO], XmNvalueChangedCallback, (XtCallbackProc) changepriv, INT_TO_XTPOINTER(PV_CPRIO));
	XtAddCallback(workw[WORKW_ANYPRIO], XmNvalueChangedCallback, (XtCallbackProc) changepriv, INT_TO_XTPOINTER(PV_ANYPRIO));

	workw[WORKW_CDEFLT] = XtVaCreateManagedWidget("cdefltp",
						     xmToggleButtonGadgetClass,		mainform,
						     XmNtopAttachment,			XmATTACH_WIDGET,
						     XmNtopWidget,			workw[WORKW_ANYPRIO],
						     XmNleftAttachment,			XmATTACH_FORM,
						     XmNborderWidth,			0,
						     NULL);
	if  (cchdeflt)
		XmToggleButtonGadgetSetState(workw[WORKW_CDEFLT], True, False);
	XtManageChild(mainform);
	CreateActionEndDlg(pri_shell, paneview, (XtCallbackProc) endprio, which ? $H{xmspuser user pris}: $H{xmspuser system pris});
}

/* Provide lists of forms and printers (This would be a wonderful use
   of C++ pointers to members - sigh) */

static XmString *	uhelpform(int * cnt)
{
	unsigned	ucnt;
	XmString	*result, *rp;

	*cnt = 0;

	/* There cannot be more form types than the number of users
	   can there....  */

	result = (XmString *) malloc((Nusers + 2) * sizeof(XmString));
	if  (!result)
		return  result;
	rp = result;
	*rp++ = XmStringCreateLocalized(Spuhdr.sph_form);
	++*cnt;
	for  (ucnt = 0;  ucnt < Nusers;  ucnt++)  {
		XmString	*prev, curr;
		curr = XmStringCreateLocalized(ulist[ucnt].spu_form);
		for  (prev = result;  prev < rp;  prev++)
			if  (XmStringCompare(*prev, curr))  {
				XmStringFree(curr);
				goto  gotit;
			}
		*rp++ = curr;
		++*cnt;
	gotit:
		;
	}
	*rp++ = (XmString) 0;
	return  result;
}

static XmString *	uhelpforma(int * cnt)
{
	unsigned	ucnt;
	XmString	*result, *rp;

	*cnt = 0;

	/* There cannot be more form types than the number of users
	   can there....  */

	result = (XmString *) malloc((Nusers + 2) * sizeof(XmString));
	if  (!result)
		return  result;
	rp = result;
	*rp++ = XmStringCreateLocalized(Spuhdr.sph_formallow);
	++*cnt;
	for  (ucnt = 0;  ucnt < Nusers;  ucnt++)  {
		XmString	*prev, curr;
		curr = XmStringCreateLocalized(ulist[ucnt].spu_formallow);
		for  (prev = result;  prev < rp;  prev++)
			if  (XmStringCompare(*prev, curr))  {
				XmStringFree(curr);
				goto  gotit;
			}
		*rp++ = curr;
		++*cnt;
	gotit:
		;
	}
	*rp++ = (XmString) 0;
	return  result;
}

static XmString *	uhelpptr(int * cnt)
{
	unsigned	ucnt;
	XmString	*result, *rp;

	*cnt = 0;

	/* There cannot be more ptr types than the number of users can
	   there....  */

	result = (XmString *) malloc((Nusers + 2) * sizeof(XmString));
	if  (!result)
		return  result;
	rp = result;
	*rp++ = XmStringCreateLocalized(Spuhdr.sph_ptr);
	++*cnt;
	for  (ucnt = 0;  ucnt < Nusers;  ucnt++)  {
		XmString	*prev, curr;
		curr = XmStringCreateLocalized(ulist[ucnt].spu_ptr);
		for  (prev = result;  prev < rp;  prev++)
			if  (XmStringCompare(*prev, curr))  {
				XmStringFree(curr);
				goto  gotit;
			}
		*rp++ = curr;
		++*cnt;
	gotit:
		;
	}
	*rp++ = (XmString) 0;
	return  result;
}

static XmString *	uhelpptra(int * cnt)
{
	unsigned	ucnt;
	XmString	*result, *rp;

	*cnt = 0;

	/* There cannot be more ptr types than the number of users can
	   there....  */

	result = (XmString *) malloc((Nusers + 2) * sizeof(XmString));
	if  (!result)
		return  result;
	rp = result;
	*rp++ = XmStringCreateLocalized(Spuhdr.sph_ptrallow);
	++*cnt;
	for  (ucnt = 0;  ucnt < Nusers;  ucnt++)  {
		XmString	*prev, curr;
		curr = XmStringCreateLocalized(ulist[ucnt].spu_ptrallow);
		for  (prev = result;  prev < rp;  prev++)
			if  (XmStringCompare(*prev, curr))  {
				XmStringFree(curr);
				goto  gotit;
			}
		*rp++ = curr;
		++*cnt;
	gotit:
		;
	}
	*rp++ = (XmString) 0;
	return  result;
}

static void	form_cb(Widget w, XtPointer cldata, XmSelectionBoxCallbackStruct * cbs)
{
	char	*value;
	int	ucnt;

	XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
	if  (XTPOINTER_TO_INT(cldata) <= FORMETC_UFORMA  &&  !value[0])  {
		doerror(w, $EH{xmspuser null form type});
		XtFree(value);
		return;
	}
	switch  (XTPOINTER_TO_INT(cldata))  {
	case  FORMETC_DFORM:
		hchanges++;
		strncpy(Spuhdr.sph_form, value, MAXFORM);
		defdisplay();
		break;
	case  FORMETC_DFORMA:
		hchanges++;
		strncpy(Spuhdr.sph_formallow, value, ALLOWFORMSIZE);
		break;
	case  FORMETC_UFORM:
		uchanges++;
		for  (ucnt = 0;  ucnt < pendunum;  ucnt++)
			strncpy(ulist[pendulist[ucnt]-1].spu_form, value, MAXFORM);
		udisplay(pendunum, pendulist);
		break;
	case  FORMETC_UFORMA:
		uchanges++;
		for  (ucnt = 0;  ucnt < pendunum;  ucnt++)
			strncpy(ulist[pendulist[ucnt]-1].spu_formallow, value, ALLOWFORMSIZE);
		break;
	case  FORMETC_DPTR:
		hchanges++;
		strncpy(Spuhdr.sph_ptr, value, PTRNAMESIZE);
		defdisplay();
		break;
	case  FORMETC_DPTRA:
		hchanges++;
		strncpy(Spuhdr.sph_ptrallow, value, JPTRNAMESIZE);
		break;
	case  FORMETC_UPTR:
		uchanges++;
		for  (ucnt = 0;  ucnt < pendunum;  ucnt++)
			strncpy(ulist[pendulist[ucnt]-1].spu_ptr, value, PTRNAMESIZE);
		udisplay(pendunum, pendulist);
		break;
	case  FORMETC_UPTRA:
		uchanges++;
		for  (ucnt = 0;  ucnt < pendunum;  ucnt++)
			strncpy(ulist[pendulist[ucnt]-1].spu_ptrallow, value, JPTRNAMESIZE);
		break;
	}
	XtFree(value);
	XtDestroyWidget(w);
}

void	cb_formetc(Widget parent, int which)
{
	Widget		dw;
	int		rows, mcode = 0;
	char		*eform = (char *) 0, *dname = (char *) 0;
	XmString	*flist = (XmString *) 0, existing;

	if  (pendunum > 0)  {
		pendunum = 0;
		XtFree((char *) pendulist);
		pendulist = (int *) 0;
	}

	switch  (which)  {
	case  FORMETC_DFORM:
		eform = Spuhdr.sph_form;
		flist = uhelpform(&rows);
		mcode = $H{xmspuser system forms};
		dname = "dforms";
		break;
	case  FORMETC_DFORMA:
		eform = Spuhdr.sph_formallow;
		flist = uhelpforma(&rows);
		mcode = $H{xmspuser system forms allowed};
		dname = "dformall";
		break;
	case  FORMETC_UFORM:
		if  (!getselectedusers(1))
			return;
		eform = ulist[pendulist[0]-1].spu_form;
		flist = uhelpform(&rows);
		mcode = $H{xmspuser user forms};
		dname = "uforms";
		break;
	case  FORMETC_UFORMA:
		if  (!getselectedusers(1))
			return;
		eform = ulist[pendulist[0]-1].spu_formallow;
		flist = uhelpforma(&rows);
		mcode = $H{xmspuser user forms allowed};
		dname = "uformall";
		break;
	case  FORMETC_DPTR:
		eform = Spuhdr.sph_ptr;
		flist = uhelpptr(&rows);
		mcode = $H{xmspuser system ptrs};
		dname = "dptrs";
		break;
	case  FORMETC_DPTRA:
		eform = Spuhdr.sph_ptrallow;
		flist = uhelpptra(&rows);
		mcode = $H{xmspuser system ptrs allowed};
		dname = "dptrall";
		break;
	case  FORMETC_UPTR:
		if  (!getselectedusers(1))
			return;
		eform = ulist[pendulist[0]-1].spu_ptr;
		flist = uhelpptr(&rows);
		mcode = $H{xmspuser user ptrs};
		dname = "uptrs";
		break;
	case  FORMETC_UPTRA:
		if  (!getselectedusers(1))
			return;
		eform = ulist[pendulist[0]-1].spu_ptrallow;
		flist = uhelpptra(&rows);
		mcode = $H{xmspuser user ptrs allowed};
		dname = "uptrall";
		break;
	}

	dw = XmCreateSelectionDialog(FindWidget(parent), dname, NULL, 0);
	existing = XmStringCreateLocalized(eform);

	if  (rows <= 0)
		XtVaSetValues(dw,
			      XmNlistItemCount,	0,
			      XmNtextString,	existing,
			      NULL);
	else  {
		int	cnt;
		XtVaSetValues(dw,
			      XmNlistItems,	flist,
			      XmNlistItemCount,	rows,
			      XmNtextString,	existing,
			      NULL);
		for  (cnt = 0;  cnt < rows;  cnt++)
			XmStringFree(flist[cnt]);
		XtFree((char *) flist);
	}
	XmStringFree(existing);
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_APPLY_BUTTON));
	XtAddCallback(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON),
		      XmNactivateCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(mcode));
	XtAddCallback(dw, XmNokCallback, (XtCallbackProc) form_cb, INT_TO_XTPOINTER(which));
	XtManageChild(dw);
}

static	classcode_t	 copyclasscode;

static void	setclear(Widget w, int val)
{
	int	cnt;
	if  (val)  {		/* Setting */
		copyclasscode = U_MAX_CLASS;
		for  (cnt = 0;  cnt < 32;  cnt++)
			XmToggleButtonGadgetSetState(ccws[cnt], True, False);
	}
	else  {			/* Clearing */
		copyclasscode = 0;
		for  (cnt = 0;  cnt < 32;  cnt++)
			XmToggleButtonGadgetSetState(ccws[cnt], False, False);
	}
}

static void	cctoggle(Widget w, int bitnum, XmToggleButtonCallbackStruct * cbs)
{
	if  (cbs->set)
		copyclasscode |= 1 << bitnum;
	else
		copyclasscode &= ~(1 << bitnum);
}

static void	endclass(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		if  (copyclasscode == 0)  {
			doerror(w, $EH{xmspuser zero class});
			return;
		}
		if  (pendunum > 0)  {
			int	ucnt, setcover;
			struct	spdet	*uitem;
			setcover = XmToggleButtonGadgetGetState(workw[WORKW_COVER]);
			uchanges++;
			for  (ucnt = 0;  ucnt < pendunum;  ucnt++)  {
				uitem = &ulist[pendulist[ucnt]-1];
				uitem->spu_class = copyclasscode;
				if  (setcover)
					uitem->spu_flgs |= PV_COVER;
				else
					uitem->spu_flgs &= ~PV_COVER;
			}
			udisplay(pendunum, pendulist);
		}
		else  {
			ULONG	oldflags = Spuhdr.sph_flgs;
			classcode_t	oldclass = Spuhdr.sph_class;
			hchanges++;
			Spuhdr.sph_class = copyclasscode;
			if  (XmToggleButtonGadgetGetState(workw[WORKW_COVER]))
				Spuhdr.sph_flgs |= PV_COVER;
			else
				Spuhdr.sph_flgs &= ~PV_COVER;
			defdisplay();
			if  (Spuhdr.sph_flgs != oldflags  ||  Spuhdr.sph_class != oldclass)
				udisplay(0, (int *) 0);
		}
	}
	XtDestroyWidget(GetTopShell(w));
}

void	cb_class(Widget parent, int which)
{
	Widget	class_shell, paneview, mainform, prevabove, ccmainrc, seta, cleara;
	int	cnt;
	classcode_t	eclass;
	ULONG	cover;

	if  (pendunum > 0)  {
		pendunum = 0;
		XtFree((char *) pendulist);
		pendulist = (int *) 0;
	}

	if  (which)  {
		if  (!getselectedusers(1))
			return;
		eclass = ulist[pendulist[0]-1].spu_class;
		cover = ulist[pendulist[0]-1].spu_flgs & PV_COVER;
	}
	else  {
		eclass = Spuhdr.sph_class;
		cover = Spuhdr.sph_flgs & PV_COVER;
	}

	prevabove = CreateUEditDlg(parent, "class", &class_shell, &paneview, &mainform);

	ccmainrc = XtVaCreateManagedWidget("ccmain",
					   xmRowColumnWidgetClass,	mainform,
					   XmNtopAttachment,		XmATTACH_WIDGET,
					   XmNtopWidget,		prevabove,
					   XmNleftAttachment,		XmATTACH_FORM,
					   XmNpacking,			XmPACK_COLUMN,
					   XmNnumColumns,		4,
					   NULL);
	copyclasscode = eclass;
	for  (cnt = 0;  cnt < 32;  cnt++)  {
		Widget	w;
		char	name[2];

		name[0] = cnt < 16? (char) ('A' + cnt): (char) (cnt + 'a' - 16);
		name[1] = '\0';
		ccws[cnt] = w = XtVaCreateManagedWidget(name, xmToggleButtonGadgetClass, ccmainrc, NULL);

		if  (copyclasscode & (1 << cnt))
			XmToggleButtonGadgetSetState(w, True, False);

		XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) cctoggle, INT_TO_XTPOINTER(cnt));
	}

	seta = XtVaCreateManagedWidget("Setall",
				       xmPushButtonGadgetClass,		mainform,
				       XmNtopAttachment,		XmATTACH_WIDGET,
				       XmNtopWidget,			prevabove,
				       XmNleftAttachment,		XmATTACH_WIDGET,
				       XmNleftWidget,			ccmainrc,
				       NULL);

	cleara = XtVaCreateManagedWidget("Clearall",
					 xmPushButtonGadgetClass,	mainform,
					 XmNtopAttachment,		XmATTACH_WIDGET,
					 XmNtopWidget,			seta,
					 XmNleftAttachment,		XmATTACH_WIDGET,
					 XmNleftWidget,			ccmainrc,
					 NULL);

	XtAddCallback(seta, XmNactivateCallback, (XtCallbackProc) setclear, (XtPointer) 1);
	XtAddCallback(cleara, XmNactivateCallback, (XtCallbackProc) setclear, (XtPointer) 0);

	workw[WORKW_COVER] = XtVaCreateManagedWidget("override",
						     xmToggleButtonGadgetClass,		mainform,
						     XmNtopAttachment,			XmATTACH_WIDGET,
						     XmNtopWidget,			ccmainrc,
						     XmNleftAttachment,			XmATTACH_FORM,
						     XmNborderWidth,			0,
						     NULL);
	if  (cover)
		XmToggleButtonGadgetSetState(workw[WORKW_COVER], True, False);
	XtManageChild(mainform);
	CreateActionEndDlg(class_shell, paneview, (XtCallbackProc) endclass,
			   which? $H{xmspuser user class}: $H{xmspuser system class});
}

struct	{
	char	*name;
	ULONG	privbit;
}  privbutts[NUM_PRIVS] =
{{	"admin",	PV_ADMIN	},
{	"sstop",	PV_SSTOP	},
{	"forms",	PV_FORMS	},
{	"otherp",	PV_OTHERP	},
{	"changep",	PV_CPRIO	},
{	"otherj",	PV_OTHERJ	},
{	"prinq",	PV_PRINQ	},
{	"haltgo",	PV_HALTGO	},
{	"anyprio",	PV_ANYPRIO	},
{	"cdefltp",	PV_CDEFLT	},
{	"adddel",	PV_ADDDEL	},
{	"override",	PV_COVER	},
{	"unqueue",	PV_UNQUEUE	},
{	"votherj",	PV_VOTHERJ	},
{	"remotej",	PV_REMOTEJ	},
{	"remotep",	PV_REMOTEP	},
{	"accessok",	PV_ACCESSOK	},
{	"freezeok",	PV_FREEZEOK	}
};

static void	pcopydef(void)
{
	int	cnt;
	for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)
		XmToggleButtonGadgetSetState(workw[cnt + WORKW_ADMIN],
					     Spuhdr.sph_flgs & privbutts[cnt].privbit? True: False, False);
}

static void	endpriv(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		ULONG  newflags = 0;
		int	cnt;
		for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)
			if  (XmToggleButtonGadgetGetState(workw[cnt + WORKW_ADMIN]))
				newflags |= privbutts[cnt].privbit;
		if  (pendunum > 0)  {
			uchanges++;
			for  (cnt = 0;  cnt < pendunum;  cnt++)
				ulist[pendulist[cnt]-1].spu_flgs = newflags;
			udisplay(pendunum, pendulist);
		}
		else  {
			if  (Confirm(w, $PH{Copy to everyone but you}))  {
				Spuhdr.sph_flgs = newflags;
				hchanges++;
				uchanges++;
				for  (cnt = 0;  cnt < Nusers;  cnt++)  {
					int_ugid_t uu = ulist[cnt].spu_user;
					if  (uu != Realuid && uu != Daemuid && uu != ROOTID)
						ulist[cnt].spu_flgs = newflags;
				}
				udisplay(0, (int *) 0);
			}
			else  if  (Spuhdr.sph_flgs != newflags)  {
				Spuhdr.sph_flgs = newflags;
				hchanges++;
				udisplay(0, (int *) 0);
			}
		}
	}
	XtDestroyWidget(GetTopShell(w));
}

void	cb_priv(Widget parent, int which)
{
	Widget	priv_shell, paneview, mainform, prevabove;
	ULONG  existing;
	int	cnt;

	if  (pendunum > 0)  {
		pendunum = 0;
		XtFree((char *) pendulist);
		pendulist = (int *) 0;
	}

	if  (which)  {
		if  (!getselectedusers(1))
			return;
		existing = ulist[pendulist[0]-1].spu_flgs;
	}
	else
		existing = Spuhdr.sph_flgs;

	prevabove = CreateUEditDlg(parent, "privs", &priv_shell, &paneview, &mainform);
	for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)  {
		workw[cnt + WORKW_ADMIN] =
			prevabove =
				XtVaCreateManagedWidget(privbutts[cnt].name,
						    xmToggleButtonGadgetClass,		mainform,
						    XmNtopAttachment,			XmATTACH_WIDGET,
						    XmNtopWidget,			prevabove,
						    XmNleftAttachment,			XmATTACH_FORM,
						    XmNborderWidth,			0,
						    NULL);
		if  (existing & privbutts[cnt].privbit)
			XmToggleButtonGadgetSetState(prevabove, True, False);
	}

	XtAddCallback(workw[WORKW_CPRIO], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_CPRIO);
	XtAddCallback(workw[WORKW_ANYPRIO], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_ANYPRIO);
	XtAddCallback(workw[WORKW_PRINQ], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_PRINQ);
	XtAddCallback(workw[WORKW_HALTGO], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_HALTGO);
	XtAddCallback(workw[WORKW_ADDDEL], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_ADDDEL);
	XtAddCallback(workw[WORKW_ADMIN], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_ADMIN);
	XtAddCallback(workw[WORKW_OTHERJ], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_OTHERJ);
	XtAddCallback(workw[WORKW_VOTHERJ], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_VOTHERJ);
	XtAddCallback(workw[WORKW_ACCESSOK], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_ACCESSOK);
	XtAddCallback(workw[WORKW_FREEZEOK], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) PV_FREEZEOK);
	if  (which)  {
		Widget	cdefb = XtVaCreateManagedWidget("cdef",
							xmPushButtonWidgetClass,	mainform,
							XmNtopAttachment,		XmATTACH_WIDGET,
							XmNtopWidget,			prevabove,
							XmNleftAttachment,		XmATTACH_FORM,
							NULL);
		XtAddCallback(cdefb, XmNactivateCallback, (XtCallbackProc) pcopydef, (XtPointer) 0);
	}
	XtManageChild(mainform);
	CreateActionEndDlg(priv_shell, paneview, (XtCallbackProc) endpriv,
			   which? $H{xmspuser user privs}: $H{xmspuser system privs});
}

static void	copyu(struct spdet * n)
{
	n->spu_defp = Spuhdr.sph_defp;
	n->spu_minp = Spuhdr.sph_minp;
	n->spu_maxp = Spuhdr.sph_maxp;
	n->spu_cps = Spuhdr.sph_cps;
	n->spu_class = Spuhdr.sph_class;
	strncpy(n->spu_form, Spuhdr.sph_form, MAXFORM);
	strncpy(n->spu_formallow, Spuhdr.sph_formallow, ALLOWFORMSIZE);
	strncpy(n->spu_ptr, Spuhdr.sph_ptr, PTRNAMESIZE);
	strncpy(n->spu_ptrallow, Spuhdr.sph_ptrallow, JPTRNAMESIZE);
}

void	cb_copydef(Widget parent, int which)
{
	int	cnt;

	if  (pendunum > 0)  {
		pendunum = 0;
		XtFree((char *) pendulist);
		pendulist = (int *) 0;
	}

	if  (which)  {
		if  (!getselectedusers(1))
			return;
		for  (cnt = 0;  cnt < pendunum;  cnt++)
			copyu(&ulist[pendulist[cnt]-1]);
		udisplay(pendunum, pendulist);
	}
	else  {
		for  (cnt = 0;  cnt < Nusers;  cnt++)
			copyu(&ulist[cnt]);
		udisplay(0, (int *) 0);
	}
	uchanges++;
}

static void	endcbdisplay(Widget w, int data)
{
	XtDestroyWidget(GetTopShell(w));
}

void	cb_cdisplay(Widget parent, int notused)
{
	int		cnt, n;
	XmStringTable	str_list;
	Widget		c_shell, cpanes, listw;
	XmString	dones;
	Arg		args[8];

	if  (pendunum > 0)  {
		pendunum = 0;
		XtFree((char *) pendulist);
		pendulist = (int *) 0;
	}

	if  (!getselectedusers(1))
		return;

	displaybusy(1);
	str_list = (XmStringTable) XtMalloc(pendunum * sizeof(XmString *));
	for  (cnt = 0;  cnt < pendunum; cnt++)  {
		int_ugid_t	uu = ulist[pendulist[cnt]-1].spu_user;
		char	buf[UIDSIZE + 20];
		sprintf(buf, "%10.10s %12ld", prin_uname(uu), (long) calccharge(uu));
		str_list[cnt] = XmStringCreateLocalized(buf);
	}
	dones = XmStringCreateLocalized("Done");
	n = 0;
        XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
	XtSetArg(args[n], XmNdeleteResponse, XmDESTROY); n++;
	XtSetArg(args[n], XmNokLabelString, dones);	n++;
        c_shell = cpanes = XmCreateTemplateDialog(GetTopShell(parent), "chlist", args, n);
	XmStringFree(dones);
	listw = XmCreateScrolledList(cpanes, "chargelist", args, 0);
	n = 0;
	XtSetArg(args[n], XmNitems, str_list); n++;
	XtSetArg(args[n], XmNitemCount, pendunum); n++;
	XtSetArg(args[n], XmNvisibleItemCount, pendunum > 30? 30: pendunum); n++;
	XtSetValues(listw, args, n);
	XtManageChild(listw);
	for  (cnt = 0;  cnt < pendunum;  cnt++)
		XmStringFree(str_list[cnt]);
	XtFree((char *) str_list);
	displaybusy(0);
	XtAddCallback(c_shell, XmNokCallback, (XtCallbackProc) endcbdisplay, (XtPointer) 0);
        XtManageChild(c_shell);
}

/* Open charge file as required */

static int	grab_file(const int omode)
{
	static	char	*file_name;

	if  (!file_name)
		file_name = envprocess(CHFILE);

	return  open(file_name, omode);
}

void	cb_zeroc(Widget parent, int which)
{
	int	cnt, fd;
	struct	spcharge	spc;

	if  (pendunum > 0)  {
		pendunum = 0;
		XtFree((char *) pendulist);
		pendulist = (int *) 0;
	}

	spc.spch_host = 0;		/* Me */
	spc.spch_pri = 0;
	spc.spch_chars = 0;
	spc.spch_cpc = 0;

	if  (!which)  {
		if  (!getselectedusers(1))
			return;
		if  (!Confirm(parent, $PH{xmspuser zero charges}))
			return;
		if  ((fd = grab_file(O_WRONLY|O_APPEND)) < 0)
			return;
		time(&spc.spch_when);
		spc.spch_what = SPCH_ZERO;
		for  (cnt = 0;  cnt < pendunum;  cnt++)  {
			spc.spch_user = ulist[pendulist[cnt]-1].spu_user;
			write(fd, (char *) &spc, sizeof(spc));
		}
	}
	else  {
		if  (!Confirm(parent, $PH{xmspuser zero all charges}))
			return;
		if  ((fd = grab_file(O_WRONLY|O_APPEND)) < 0)
			return;
		time(&spc.spch_when);
		spc.spch_what = SPCH_ZEROALL;
		spc.spch_user = -1;
		write(fd, (char *) &spc, sizeof(spc));
	}
	close(fd);
}

static void	endimp(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		char	*txt, *tp;
		int	cnt, fd;
		LONG	res;
		struct	spcharge	spc;

		XtVaGetValues(workw[WORKW_IMPW], XmNvalue, &txt, NULL);
		for  (tp = txt;  isspace(*tp);  tp++)
			;
		if  (!isdigit(*tp))  {
			doerror(w, $EH{xmspuser invalid charge});
			XtFree(txt);
			return;
		}
		res = atol(txt);
		if  ((fd = grab_file(O_WRONLY|O_APPEND)) >= 0)  {
			time(&spc.spch_when);
			spc.spch_host = 0;		/* Me */
			spc.spch_pri = 0;
			spc.spch_chars = 0;
			spc.spch_cpc = res;
			spc.spch_what = SPCH_FEE;
			for  (cnt = 0;  cnt < pendunum;  cnt++)  {
				spc.spch_user = ulist[pendulist[cnt]-1].spu_user;
				write(fd, (char *) &spc, sizeof(spc));
			}
			close(fd);
		}
	}
	XtDestroyWidget(GetTopShell(w));
}

void	cb_impose(Widget parent, int which)
{
	Widget	imp_shell, paneview, mainform, prevabove, titw;
	char	nbuf[10];

	if  (pendunum > 0)  {
		pendunum = 0;
		XtFree((char *) pendulist);
		pendulist = (int *) 0;
	}

	if  (!getselectedusers(1))
		return;

	prevabove = CreateUEditDlg(parent, "impose", &imp_shell, &paneview, &mainform);

	titw = XtVaCreateManagedWidget("amount",
				       xmLabelGadgetClass,	mainform,
				       XmNtopAttachment,	XmATTACH_WIDGET,
				       XmNtopWidget,		prevabove,
				       XmNleftAttachment,	XmATTACH_FORM,
				       NULL);

	workw[WORKW_IMPW] = XtVaCreateManagedWidget("amt",
						    xmTextFieldWidgetClass,	mainform,
						    XmNcolumns,			8,
						    XmNmaxWidth,		8,
						    XmNcursorPositionVisible,	False,
						    XmNtopAttachment,		XmATTACH_WIDGET,
						    XmNtopWidget,		prevabove,
						    XmNleftAttachment,		XmATTACH_WIDGET,
						    XmNleftWidget,		titw,
						    NULL);

	sprintf(nbuf, "%8d", 0);
	XmTextSetString(workw[WORKW_IMPW], nbuf);
	XtManageChild(mainform);
	CreateActionEndDlg(imp_shell, paneview, (XtCallbackProc) endimp, $H{xmspuser addfee help});
}

void	cb_chelp(Widget w, int data, XmAnyCallbackStruct * cbs)
{
	Widget	help_w;
	Cursor	cursor;

	cursor = XCreateFontCursor(dpy, XC_hand2);
	if  ((help_w = XmTrackingLocate(toplevel, cursor, False))  &&
	     XtHasCallbacks(help_w, XmNhelpCallback) == XtCallbackHasSome)  {
		cbs->reason = XmCR_HELP;
		XtCallCallbacks(help_w, XmNhelpCallback, cbs);
	}
	XFreeCursor(dpy, cursor);
}

static void	umacroexec(char * str)
{
	static	char	*execprog;
	PIDTYPE	pid;
	int	status;

	if  (!execprog)
		execprog = envprocess(EXECPROG);

	if  ((pid = fork()) == 0)  {
		char	**argbuf, **ap;
		int	cnt;
		getselectedusers(0);
		if  (!(argbuf = (char **) malloc((unsigned) pendunum + 2)))
			exit(0);
		ap = argbuf;
		*ap++ = str;
		for  (cnt = 0;  cnt < pendunum;  cnt++)
			*ap++ = prin_uname((uid_t) ulist[pendulist[cnt] - 1].spu_user);
		*ap = (char *) 0;
		execv(execprog, argbuf);
		exit(255);
	}
	if  (pid < 0)  {
		doerror(uwid, $EH{Macro fork failed});
		return;
	}
#ifdef	HAVE_WAITPID
	while  (waitpid(pid, &status, 0) < 0)
		;
#else
	while  (wait(&status) != pid)
		;
#endif
	if  (status != 0)  {
		if  (status & 255)  {
			disp_arg[0] = status & 255;
			doerror(uwid, $EH{Macro command gave signal});
		}
		else  {
			disp_arg[0] = (status >> 8) & 255;
			doerror(uwid, $EH{Macro command error});
		}
	}
}

static void	endumacro(Widget w, int data)
{
	if  (data)  {
		char	*txt;
		XtVaGetValues(workw[WORKW_SORTA], XmNvalue, &txt, NULL);
		if  (txt[0])
			umacroexec(txt);
		XtFree(txt);
	}
	XtDestroyWidget(GetTopShell(w));
}

void	cb_macrou(Widget parent, int data)
{
	char	*prompt = helpprmpt(data + $P{Job or User macro});
	Widget	uc_shell, panew, mainform, labw;

	if  (!prompt)  {
		disp_arg[0] = data + $P{Job or User macro};
		doerror(uwid, $EH{Macro error});
		return;
	}

	if  (data != 0)  {
		umacroexec(prompt);
		return;
	}

	CreateEditDlg(parent, "usercmd", &uc_shell, &panew, &mainform, 3);
	labw = XtVaCreateManagedWidget("cmdtit",
				       xmLabelGadgetClass,	mainform,
				       XmNtopAttachment,	XmATTACH_FORM,
				       XmNleftAttachment,	XmATTACH_FORM,
				       NULL);
	workw[WORKW_SORTA] = XtVaCreateManagedWidget("cmd",
						     xmTextFieldWidgetClass,	mainform,
						     XmNcolumns,		20,
						     XmNcursorPositionVisible,	False,
						     XmNtopAttachment,		XmATTACH_FORM,
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		labw,
						     NULL);
	XtManageChild(mainform);
	CreateActionEndDlg(uc_shell, panew, (XtCallbackProc) endumacro, $H{Job or User macro});
}

static	char	matchcase, wraparound, sbackward, *matchtext;

static int	smstr(const char *str, const int exact)
{
	int	 l = strlen(matchtext), cl = strlen(str);

	if  (cl < l)
		return  0;

	if  (exact)  {
		int	cnt;
		if  (cl != l)
			return  0;
		for  (cnt = 0;  cnt < l;  cnt++)  {
			int	mch = matchtext[cnt];
			int	sch = str[cnt];
			if  (!matchcase)  {
				mch = toupper(mch);
				sch = toupper(sch);
			}
			if  (!(mch == sch  ||  mch == '.'))
				return  0;
		}
		return  1;
	}
	else  {
		int	 resid = cl - l, bcnt, cnt;
		for  (bcnt = 0;  bcnt <= resid;  bcnt++)  {
			for  (cnt = 0;  cnt < l;  cnt++)  {
				int	mch = matchtext[cnt];
				int	sch = str[cnt+bcnt];
				if  (!matchcase)  {
					mch = toupper(mch);
					sch = toupper(sch);
				}
				if  (!(mch == sch  ||  mch == '.'))
					goto  notmatch;
			}
			return  1;
		notmatch:
			;
		}
		return  0;
	}
}

/* Written like this for easy e-x-t-e-n-s-i-o-n.  */

static int	usermatch(int n)
{
	return  smstr(prin_uname((uid_t) ulist[n].spu_user), 0);
}

static void	execute_search(void)
{
	int	*plist, pcnt, cline, nline;
	int	topitem, visibleitem;

	if  (XmListGetSelectedPos(uwid, &plist, &pcnt))  {
		cline = plist[0] - 1;
		XtFree((char *) plist);
	}
	else
		cline = sbackward? (int) Nusers: -1;
	if  (sbackward)  {
		for  (nline = cline - 1;  nline >= 0;  nline--)
			if  (usermatch(nline))
				goto  gotuser;
		if  (wraparound)
			for  (nline = Nusers-1;  nline > cline;  nline--)
				if  (usermatch(nline))
					goto  gotuser;
		doerror(uwid, $EH{Rsearch user not found});
	}
	else  {
		for  (nline = cline + 1;  nline < Nusers;  nline++)
			if  (usermatch(nline))
				goto  gotuser;
		if  (wraparound)
			for  (nline = 0;  nline < cline;  nline++)
				if  (usermatch(nline))
					goto  gotuser;
		doerror(uwid, $EH{Fsearch user not found});
	}
	return;
 gotuser:
	nline++;
	XmListSelectPos(uwid, nline, False);
	XtVaGetValues(uwid, XmNtopItemPosition, &topitem, XmNvisibleItemCount, &visibleitem, NULL);
	if  (nline < topitem)
		XmListSetPos(uwid, nline);
	else  if  (nline >= topitem + visibleitem)
		XmListSetBottomPos(uwid, nline);
}

static Widget InitsearchDlg(Widget parent, Widget *shellp, Widget *panep, Widget *formp, char *existing)
{
	Widget	prevleft;

	CreateEditDlg(parent, "Search", shellp, panep, formp, 3);

	prevleft = XtVaCreateManagedWidget("lookfor",
					   xmLabelWidgetClass,	*formp,
					   XmNtopAttachment,	XmATTACH_FORM,
					   XmNleftAttachment,	XmATTACH_FORM,
					   XmNborderWidth,	0,
					   NULL);

	workw[WORKW_STXTW] = XtVaCreateManagedWidget("sstring",
						     xmTextFieldWidgetClass,	*formp,
						     XmNcursorPositionVisible,	False,
						     XmNtopAttachment,		XmATTACH_FORM,
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		prevleft,
						     XmNrightAttachment,	XmATTACH_FORM,
						     NULL);
	if  (existing)
		XmTextSetString(workw[WORKW_STXTW], existing);

	return  workw[WORKW_STXTW];
}

static void	endsdlg(Widget w, int data)
{
	if  (data)  {
		sbackward = XmToggleButtonGadgetGetState(workw[WORKW_FORWW])? 0: 1;
		matchcase = XmToggleButtonGadgetGetState(workw[WORKW_MATCHW])? 1: 0;
		wraparound = XmToggleButtonGadgetGetState(workw[WORKW_WRAPW])? 1: 0;
		if  (matchtext)	/* Last time round */
			XtFree(matchtext);
		XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &matchtext, NULL);
		if  (matchtext[0] == '\0')  {
			doerror(w, $EH{Rsearch user});
			return;
		}
		XtDestroyWidget(GetTopShell(w));
		execute_search();
	}
	else
		XtDestroyWidget(GetTopShell(w));
}

void	cb_rsrch(Widget w, int data)
{
	if  (!matchtext  ||  matchtext[0] == '\0')  {
		doerror(w, $EH{No previous search});
		return;
	}
	sbackward = (char) data;
	execute_search();
}

void	cb_srchfor(Widget parent)
{
	Widget	s_shell, panew, formw, dirrc, prevleft, prevabove;

	prevabove = InitsearchDlg(parent, &s_shell, &panew, &formw, matchtext);

	dirrc = XtVaCreateManagedWidget("dirn",
					xmRowColumnWidgetClass,		formw,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_FORM,
					XmNrightAttachment,		XmATTACH_FORM,
					XmNpacking,			XmPACK_COLUMN,
					XmNnumColumns,			2,
					XmNisHomogeneous,		True,
					XmNentryClass,			xmToggleButtonGadgetClass,
					XmNradioBehavior,		True,
					NULL);

	workw[WORKW_FORWW] = XtVaCreateManagedWidget("forward",
						     xmToggleButtonGadgetClass,	dirrc,
						     XmNborderWidth,		0,
						     NULL);

	prevleft = XtVaCreateManagedWidget("backward",
					   xmToggleButtonGadgetClass,	dirrc,
					   XmNborderWidth,		0,
					   NULL);

	XmToggleButtonGadgetSetState(sbackward? prevleft: workw[WORKW_FORWW], True, False);

	dirrc = XtVaCreateManagedWidget("opts",
					xmRowColumnWidgetClass,		formw,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			dirrc,
					XmNleftAttachment,		XmATTACH_FORM,
					XmNrightAttachment,		XmATTACH_FORM,
					XmNpacking,			XmPACK_COLUMN,
					XmNnumColumns,			2,
					XmNisHomogeneous,		True,
					XmNentryClass,			xmToggleButtonGadgetClass,
					XmNradioBehavior,		False,
					NULL);

	workw[WORKW_MATCHW] =  XtVaCreateManagedWidget("match",
						       xmToggleButtonGadgetClass,	dirrc,
						       XmNborderWidth,			0,
						       NULL);
	if  (matchcase)
		XmToggleButtonGadgetSetState(workw[WORKW_MATCHW], True, False);

	workw[WORKW_WRAPW] =  XtVaCreateManagedWidget("wrap",
						      xmToggleButtonGadgetClass,	dirrc,
						      XmNborderWidth,			0,
						      NULL);
	if  (wraparound)
		XmToggleButtonGadgetSetState(workw[WORKW_WRAPW], True, False);

	XtManageChild(formw);

	CreateActionEndDlg(s_shell, panew, (XtCallbackProc) endsdlg, $H{xmspuser search menu help});
}
