/* xmsq_cbs.c -- xmspq misc callbacks

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
#include <errno.h>
#include <X11/cursorfont.h>
#include <Xm/DialogS.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
#include <Xm/ComboBox.h>
#endif
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "xmsq_ext.h"
#include "displayopt.h"

classcode_t	copyclasscode;

Widget	workw[11];
static	Widget	ccws[32];

static	unsigned  char	copyconfabort, copyrestrunp, copyjinclude;

static	char	matchcase, wraparound, sbackward, searchptrs;
static	char	searchtit = 1, searchform = 1, searchuser = 1, searchptr = 1, searchdev = 1;
static	char	*matchtext;

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

static	char *	makebigvec(char ** mat)
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
		disp_arg[9] = errnum;
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

static void	response(Widget w, int *answer, XmAnyCallbackStruct *cbs)
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
	XtAddCallback(dlg, XmNokCallback, (XtCallbackProc) response, (XtPointer) &answer);
	XtAddCallback(dlg, XmNcancelCallback, (XtCallbackProc) response, (XtPointer) &answer);
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

static void	ulist_cb(Widget w, int nullok, XmSelectionBoxCallbackStruct *cbs)
{
	char	*value;

	XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
	if  (!value[0]  &&  !nullok)  {
		doerror(w, $EH{Null user});
		XtFree(value);
		return;
	}
	XmTextSetString(workw[WORKW_UTXTW], value);
	XtFree(value);
	XtDestroyWidget(w);
}

static int	sort_u(char **a, char **b)
{
	return  strcmp(*a, *b);
}

static void	getusersel(Widget w, int nullok)
{
	Widget		dw;
	int		nusers, cnt;
	char		*existing;
	char		**ulist = gen_ulist((char *) 0, 0);
	XmString	existstr;

	XtVaGetValues(workw[WORKW_UTXTW], XmNvalue, &existing, NULL);
	existstr = XmStringCreateLocalized(existing);
	XtFree(existing);
	count_hv(ulist, &nusers, &cnt);
	dw = XmCreateSelectionDialog(FindWidget(w), "uselect", NULL, 0);

	if  (nusers <= 0)  {
		XtVaSetValues(dw,
			      XmNlistItemCount,	0,
			      XmNtextString,	existstr,
			      NULL);
	}
	else  {
		XmString  *strlist = (XmString *) XtMalloc(nusers * sizeof(XmString));
		qsort(QSORTP1 ulist, nusers, sizeof(char *), QSORTP4 sort_u);
		for  (cnt = 0;  cnt < nusers;  cnt++)  {
			strlist[cnt] = XmStringCreateLocalized(ulist[cnt]);
			free(ulist[cnt]);
		}
		free((char *) ulist);
		XtVaSetValues(dw,
			      XmNlistItems,	strlist,
			      XmNlistItemCount,	nusers,
			      XmNtextString,	existstr,
			      NULL);
		for  (cnt = 0;  cnt < nusers;  cnt++)
			XmStringFree(strlist[cnt]);
		XtFree((XtPointer) strlist);
	}
	XmStringFree(existstr);
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_APPLY_BUTTON));
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON));
	XtAddCallback(dw, XmNokCallback, (XtCallbackProc) ulist_cb, INT_TO_XTPOINTER(nullok));
	XtManageChild(dw);
}

#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
static void	plist_cb(Widget w, int nullok, XmSelectionBoxCallbackStruct *cbs)
{
	char	*value;

	XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
	if  (!value[0]  &&  !nullok)  {
		doerror(w, $EH{Null printer});
		XtFree(value);
		return;
	}
	XmTextSetString(workw[WORKW_PTXTW], value);
	XtFree(value);
	XtDestroyWidget(w);
}

static void	newpnamehelp(Widget w, int helpcode)
{
	FILE	*nfile;

	if  ((nfile = hexists(ptdir, (char *) 0)))  {
		char	**hv = makefvec(nfile), *newstr;
		Widget	ew = XmCreateInformationDialog(FindWidget(w), "help", NULL, 0);
		XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_HELP_BUTTON));
		newstr = makebigvec(hv);
		XtVaSetValues(ew,
			      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
			      XtVaTypedArg, XmNmessageString, XmRString, newstr, strlen(newstr),
			      NULL);
		free(newstr);
		XtManageChild(ew);
		XtPopup(XtParent(ew), XtGrabNone);
	}
	else
		dohelp(w, helpcode);
}

void	getptrsel(Widget w, int nullok)
{
	Widget		dw;
	int		nptrs, cnt;
	char		*existing;
	char		**plist = wotjprin();
	XmString	existstr;

	XtVaGetValues(workw[WORKW_PTXTW], XmNvalue, &existing, NULL);
	existstr = XmStringCreateLocalized(existing);
	XtFree(existing);
	count_hv(plist, &nptrs, &cnt);
	dw = XmCreateSelectionDialog(FindWidget(w), "pselect", NULL, 0);

	if  (nptrs <= 0)  {
		XtVaSetValues(dw,
			      XmNlistItemCount,	0,
			      XmNtextString,	existstr,
			      NULL);
		free((char *) plist);
	}
	else  {
		XmString  *strlist = (XmString *) XtMalloc(nptrs * sizeof(XmString));
		for  (cnt = 0;  cnt < nptrs;  cnt++)  {
			strlist[cnt] = XmStringCreateLocalized(plist[cnt]);
			free(plist[cnt]);
		}
		free((char *) plist);
		XtVaSetValues(dw,
			      XmNlistItems,	strlist,
			      XmNlistItemCount,	nptrs,
			      XmNtextString,	existstr,
			      NULL);
		for  (cnt = 0;  cnt < nptrs;  cnt++)
			XmStringFree(strlist[cnt]);
		XtFree((XtPointer) strlist);
	}
	XmStringFree(existstr);
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_APPLY_BUTTON));
	XtAddCallback(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON),
		      XmNactivateCallback,
		      (XtCallbackProc) newpnamehelp,
		      (XtPointer) $H{Printer name or null});
	XtAddCallback(dw, XmNokCallback, (XtCallbackProc) plist_cb, (XtPointer) nullok);
	XtManageChild(dw);
}

void	getnewptrsel(Widget w)
{
	Widget		dw;
	int		nptrs, cnt;
	char		*existing;
	char		**plist = wotpprin();
	XmString	existstr;

	XtVaGetValues(workw[WORKW_PTXTW], XmNvalue, &existing, NULL);
	existstr = XmStringCreateLocalized(existing);
	XtFree(existing);
	count_hv(plist, &nptrs, &cnt);
	dw = XmCreateSelectionDialog(FindWidget(w), "pselect", NULL, 0);

	if  (nptrs <= 0)  {
		XtVaSetValues(dw,
			      XmNlistItemCount,	0,
			      XmNtextString,	existstr,
			      NULL);
		free((char *) plist);
	}
	else  {
		XmString  *strlist = (XmString *) XtMalloc(nptrs * sizeof(XmString));
		for  (cnt = 0;  cnt < nptrs;  cnt++)  {
			strlist[cnt] = XmStringCreateLocalized(plist[cnt]);
			free(plist[cnt]);
		}
		free((char *) plist);
		XtVaSetValues(dw,
			      XmNlistItems,	strlist,
			      XmNlistItemCount,	nptrs,
			      XmNtextString,	existstr,
			      NULL);
		for  (cnt = 0;  cnt < nptrs;  cnt++)
			XmStringFree(strlist[cnt]);
		XtFree((XtPointer) strlist);
	}
	XmStringFree(existstr);
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_APPLY_BUTTON));
	XtAddCallback(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON),
		      XmNactivateCallback,
		      (XtCallbackProc) newpnamehelp,
		      (XtPointer) $H{Please enter printer name});
	XtAddCallback(dw, XmNokCallback, (XtCallbackProc) plist_cb, (XtPointer) 0);
	XtManageChild(dw);
}
#endif

static void	dlist_cb(Widget w, int nullok, XmSelectionBoxCallbackStruct * cbs)
{
	char	*value;

	XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
	if  (!value[0])  {
		doerror(w, $EH{Null device name});
		XtFree(value);
		return;
	}
	XmTextSetString(workw[WORKW_DEVTXTW], value);
	XtFree(value);
	XtDestroyWidget(w);
}

void	getdevsel(Widget w)
{
	Widget		dw;
	int		ndevs, cnt;
	char		*existing;
	char		**plist = wottty();
	XmString	existstr;

	XtVaGetValues(workw[WORKW_DEVTXTW], XmNvalue, &existing, NULL);
	existstr = XmStringCreateLocalized(existing);
	XtFree(existing);
	count_hv(plist, &ndevs, &cnt);
	dw = XmCreateSelectionDialog(FindWidget(w), "dselect", NULL, 0);

	if  (ndevs <= 0)  {
		XtVaSetValues(dw,
			      XmNlistItemCount,	0,
			      XmNtextString,	existstr,
			      NULL);
		free((char *) plist);
	}
	else  {
		XmString  *strlist = (XmString *) XtMalloc(ndevs * sizeof(XmString));
		for  (cnt = 0;  cnt < ndevs;  cnt++)  {
			strlist[cnt] = XmStringCreateLocalized(plist[cnt]);
			free(plist[cnt]);
		}
		free((char *) plist);
		XtVaSetValues(dw,
			      XmNlistItems,	strlist,
			      XmNlistItemCount,	ndevs,
			      XmNtextString,	existstr,
			      NULL);
		for  (cnt = 0;  cnt < ndevs;  cnt++)
			XmStringFree(strlist[cnt]);
		XtFree((XtPointer) strlist);
	}
	XmStringFree(existstr);
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_APPLY_BUTTON));
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON));
	XtAddCallback(dw, XmNokCallback, (XtCallbackProc) dlist_cb, (XtPointer) 0);
	XtManageChild(dw);
}

static void	flist_cb(Widget w, int nullok, XmSelectionBoxCallbackStruct * cbs)
{
	char	*value;

	XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
	if  (!value[0])  {
		doerror(w, $EH{Null form name});
		XtFree(value);
		return;
	}
	XmTextSetString(workw[WORKW_FTXTW], value);
	XtFree(value);
	XtDestroyWidget(w);
}

void	pformhelp(Widget w, int isold)
{
	FILE	*nfile;
	char	*pname;

	if  (isold)
		pname = PREQ.spp_ptr;
	else
		XtVaGetValues(workw[WORKW_PTXTW], XmNvalue, &pname, NULL);

	if  (pname  &&  pname[0]  &&  (nfile = hexists(ptdir, pname)))  {
		char	**hv = makefvec(nfile), *newstr;
		Widget	ew = XmCreateInformationDialog(FindWidget(w), "help", NULL, 0);
		XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_HELP_BUTTON));
		newstr = makebigvec(hv);
		XtVaSetValues(ew,
			      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
			      XtVaTypedArg, XmNmessageString, XmRString, newstr, strlen(newstr),
			      NULL);
		free(newstr);
		XtManageChild(ew);
		XtPopup(XtParent(ew), XtGrabNone);
	}
	else
		dohelp(w, $H{Form type selection});
	if  (!isold)
		XtFree(pname);
}

void	getformsel(Widget w, int isptr)
{
	Widget		dw;
	int		nforms, cnt;
	char		*existing;
	char		**flist = wotjform();
	XmString	existstr;

	XtVaGetValues(workw[WORKW_FTXTW], XmNvalue, &existing, NULL);
	existstr = XmStringCreateLocalized(existing);
	XtFree(existing);
	count_hv(flist, &nforms, &cnt);
	dw = XmCreateSelectionDialog(FindWidget(w), "fselect", NULL, 0);

	if  (nforms <= 0)  {
		XtVaSetValues(dw,
			      XmNlistItemCount,	0,
			      XmNtextString,	existstr,
			      NULL);
		free((char *) flist);
	}
	else  {
		XmString  *strlist = (XmString *) XtMalloc(nforms * sizeof(XmString));
		for  (cnt = 0;  cnt < nforms;  cnt++)  {
			strlist[cnt] = XmStringCreateLocalized(flist[cnt]);
			free(flist[cnt]);
		}
		free((char *) flist);
		XtVaSetValues(dw,
			      XmNlistItems,	strlist,
			      XmNlistItemCount,	nforms,
			      XmNtextString,	existstr,
			      NULL);
		for  (cnt = 0;  cnt < nforms;  cnt++)
			XmStringFree(strlist[cnt]);
		XtFree((XtPointer) strlist);
	}
	XmStringFree(existstr);
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_APPLY_BUTTON));
	if  (isptr)
		XtAddCallback(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON),
			      XmNactivateCallback, (XtCallbackProc) pformhelp, (XtPointer) 0);
	else
		XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON));
	XtAddCallback(dw, XmNokCallback, (XtCallbackProc) flist_cb, (XtPointer) 0);
	XtManageChild(dw);
}

static void	setclear(Widget w, int val)
{
	int	cnt;
	if  (val)  {		/* Setting */
		if  (mypriv->spu_flgs & PV_COVER)  {
			if  (copyclasscode == mypriv->spu_class)
				copyclasscode = U_MAX_CLASS;
			else
				copyclasscode = mypriv->spu_class;
		}
		else
			copyclasscode |= mypriv->spu_class;
	}
	else  {			/* Clearing */
		if  (mypriv->spu_flgs & PV_COVER)  {
			if  ((copyclasscode & ~mypriv->spu_class) != 0)
				copyclasscode = mypriv->spu_class;
			else
				copyclasscode = 0;
		}
		else
			copyclasscode &= ~mypriv->spu_class;
	}
	for  (cnt = 0;  cnt < 32;  cnt++)
		XmToggleButtonGadgetSetState(ccws[cnt], (copyclasscode & (1 << cnt))? True: False, False);
}

static void	cctoggle(Widget w, int bitnum, XmToggleButtonCallbackStruct * cbs)
{
	if  (cbs->set)
		copyclasscode |= 1 << bitnum;
	else
		copyclasscode &= ~(1 << bitnum);
}

static void	conftoggle(Widget w, int cnt, XmToggleButtonCallbackStruct * cbs)
{
	if  (cbs->set)
		copyconfabort = (unsigned char) cnt;
}

static void	restrptoggle(Widget w, int cnt, XmToggleButtonCallbackStruct * cbs)
{
	if  (cbs->set)
		copyrestrunp = (unsigned char) cnt;
}

static void	jincltoggle(Widget w, int cnt, XmToggleButtonCallbackStruct * cbs)
{
	if  (cbs->set)
		copyjinclude = (unsigned char) cnt;
}

/* Set up a form selection pane and dialog */

#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
Widget CreateFselDialog(Widget mainform, Widget	prevabove, char *existing, XtCallbackProc cb, int nullok)
#else
Widget CreateFselDialog(Widget mainform, Widget	prevabove, char *existing, char **formlist)
#endif
{
	Widget	ftitw;
#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
	Widget  fselb;
#else
	int	nrows, ncols, cnt;
	XmStringTable	st;

	count_hv(formlist, &nrows, &ncols);

	if  (!(st = (XmStringTable) XtMalloc((unsigned) ((nrows+1) * sizeof(XmString *)))))
		nomem();
	for  (cnt = 0;  cnt < nrows;  cnt++)
		st[cnt] = XmStringCreateLocalized(formlist[cnt]);
#endif

	ftitw = XtVaCreateManagedWidget("form",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
	workw[WORKW_FTXTW] = XtVaCreateManagedWidget("fname",
						     xmTextFieldWidgetClass,	mainform,
						     XmNmaxWidth,		MAXFORM,
						     XmNcursorPositionVisible,	False,
						     XmNcolumns,		MAXFORM,
						     XmNtopAttachment,		XmATTACH_WIDGET,
						     XmNtopWidget,		prevabove,
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		ftitw,
						     NULL);
	if  (existing)
		XmTextSetString(workw[WORKW_FTXTW], existing);

	fselb = XtVaCreateManagedWidget("fselect",
					xmPushButtonWidgetClass,	mainform,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			workw[WORKW_FTXTW],
					NULL);

	XtAddCallback(fselb, XmNactivateCallback, cb, (XtPointer) nullok);
#else
	workw[WORKW_FTXTW] = XtVaCreateManagedWidget("fname",
						     xmComboBoxWidgetClass,	mainform,
						     XmNcomboBoxType,		XmDROP_DOWN_COMBO_BOX,
						     XmNitemCount,		nrows,
						     XmNitems,			st,
						     XmNvisibleItemCount,	nrows <= 0? 1: nrows > 15? 15: nrows,
						     XmNcolumns,		MAXFORM,
						     XmNtopAttachment,		XmATTACH_WIDGET,
						     XmNtopWidget,		prevabove,
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		ftitw,
						     NULL);

	if  (existing)  {
		XmString	existstr = XmStringCreateLocalized(existing);
		XtVaSetValues(workw[WORKW_FTXTW], XmNselectedItem, existstr, NULL);
		XmStringFree(existstr);
	}
	for  (cnt = 0;  cnt < nrows;  cnt++)
		XmStringFree(st[cnt]);
	XtFree((XtPointer) st);
#endif
	return  workw[WORKW_FTXTW];
}

/* Set up a printer selection pane and dialog */

#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
Widget CreatePselDialog(Widget mainform, Widget	prevabove, char *existing, XtCallbackProc cb, int nullok)
#else
Widget CreatePselDialog(Widget mainform, Widget	prevabove, char *existing, char **printerslist, int nullok, int namesize)
#endif
{
	Widget	ptitw;
#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
	Widget  pselb;
#else
	int	nrows, ncols, actrows, cnt, rcnt;
	XmStringTable	st;

	count_hv(printerslist, &nrows, &ncols);
	actrows = nrows;
	if  (nullok)
		actrows++;

	if  (!(st = (XmStringTable) XtMalloc((unsigned) ((actrows + 1) * sizeof(XmString *)))))
		nomem();
	rcnt = 0;
	if  (nullok)
		st[rcnt++] = XmStringCreateLocalized("-");
	for  (cnt = 0;  cnt < nrows;  cnt++)
		st[rcnt++] = XmStringCreateLocalized(printerslist[cnt]);
#endif

	ptitw = XtVaCreateManagedWidget("ptrname",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
	workw[WORKW_PTXTW] = XtVaCreateManagedWidget("ptr",
						     xmTextFieldWidgetClass,	mainform,
						     XmNcolumns,		JPTRNAMESIZE,
						     XmNmaxWidth,		JPTRNAMESIZE,
						     XmNcursorPositionVisible,	False,
						     XmNtopAttachment,		XmATTACH_WIDGET,
						     XmNtopWidget,		prevabove,
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		ptitw,
						     NULL);

	if  (existing)
		XmTextSetString(workw[WORKW_PTXTW], existing);

	pselb = XtVaCreateManagedWidget("pselect",
					xmPushButtonWidgetClass,	mainform,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			workw[WORKW_PTXTW],
					NULL);

	XtAddCallback(pselb, XmNactivateCallback, cb, (XtPointer) nullok);
#else
	workw[WORKW_PTXTW] = XtVaCreateManagedWidget("ptr",
						     xmComboBoxWidgetClass,	mainform,
						     XmNcolumns,		namesize,
						     XmNcomboBoxType,		XmDROP_DOWN_COMBO_BOX,
						     XmNitemCount,		actrows,
						     XmNitems,			st,
						     XmNvisibleItemCount,	actrows <= 0? 1: actrows > 15? 15: actrows,
						     XmNtopAttachment,		XmATTACH_WIDGET,
						     XmNtopWidget,		prevabove,
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		ptitw,
						     NULL);

	if  (existing)  {
		XmString	existstr = XmStringCreateLocalized(existing);
		XtVaSetValues(workw[WORKW_PTXTW], XmNselectedItem, existstr, NULL);
		XmStringFree(existstr);
	}

	for  (cnt = 0;  cnt < actrows;  cnt++)
		XmStringFree(st[cnt]);
	XtFree((XtPointer) st);
#endif

	return  workw[WORKW_PTXTW];
}

/* Set up a user selection pane and dialog */

Widget CreateUselDialog(Widget mainform, Widget	prevabove, char *existing, int nullok)
{
	Widget	utitw, uselb;

	utitw = XtVaCreateManagedWidget("username",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	workw[WORKW_UTXTW] = XtVaCreateManagedWidget("user",
						     xmTextFieldWidgetClass,	mainform,
						     XmNcolumns,		UIDSIZE,
						     XmNmaxWidth,		UIDSIZE,
						     XmNcursorPositionVisible,	False,
						     XmNtopAttachment,		XmATTACH_WIDGET,
						     XmNtopWidget,		prevabove,
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		utitw,
						     NULL);

	if  (existing)
		XmTextSetString(workw[WORKW_UTXTW], existing);

	uselb = XtVaCreateManagedWidget("uselect",
					xmPushButtonWidgetClass,	mainform,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			workw[WORKW_UTXTW],
					NULL);

	XtAddCallback(uselb, XmNactivateCallback, (XtCallbackProc) getusersel, INT_TO_XTPOINTER(nullok));
	return  workw[WORKW_UTXTW];
}

/* Set up a class code dialog */

Widget CreateCCDialog(Widget mainform, Widget prevabove, classcode_t eclass, Widget *pleft)
{
	Widget	ccmainrc, seta, cleara;
	int	cnt;

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

		name[0] = cnt < 16? (char)('A' + cnt): (char) (cnt + 'a' - 16);
		name[1] = '\0';
		ccws[cnt] = w = XtVaCreateManagedWidget(name, xmToggleButtonGadgetClass, ccmainrc, NULL);

		if  (copyclasscode & (1 << cnt))
			XmToggleButtonGadgetSetState(w, True, False);

		XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) cctoggle, INT_TO_XTPOINTER(cnt));

		if  (!(mypriv->spu_flgs & PV_COVER)  &&  !(mypriv->spu_class & (1 << cnt)))
			XtSetSensitive(w, False);
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
	if  (pleft)
		*pleft = cleara;
	return  ccmainrc;
}

/* Create the stuff at the beginning of a dialog */

void  CreateEditDlg(Widget parent, char *dlgname, Widget *dlgres, Widget *paneres, Widget *formres, const int nbutts)
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
	*formres = XtVaCreateWidget("form",
				    xmFormWidgetClass,		*dlgres,
				    XmNfractionBase,		2 * nbutts,
				    NULL);
}

/* Create the stuff at the end of a dialog.  */

void CreateActionEndDlg(Widget shelldlg, Widget panew, XtCallbackProc endrout, int helpcode)
{
	XtAddCallback(shelldlg, XmNokCallback, endrout, (XtPointer) 1);
        XtAddCallback(shelldlg, XmNcancelCallback, endrout, (XtPointer) 0);
        XtAddCallback(shelldlg, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(helpcode));
        XtManageChild(shelldlg);
}

static  void  endviewopt(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		char	*txt;
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
		XmString	ptrtxt;
#endif
		if  (copyclasscode == 0)  {
			doerror(jwid, $EH{Xmspq null class});
			return;
		}
		XtVaGetValues(workw[WORKW_UTXTW], XmNvalue, &txt, NULL);
		if  (Displayopts.opt_restru)
			free(Displayopts.opt_restru);
		Displayopts.opt_restru = txt[0]? stracpy(txt): (char *) 0;
		XtFree(txt);
		if  (Displayopts.opt_restrp)
			free(Displayopts.opt_restrp);
#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
		XtVaGetValues(workw[WORKW_PTXTW], XmNvalue, &txt, NULL);
		Displayopts.opt_restrp = txt[0]? stracpy(txt): (char *) 0;
#else
		XtVaGetValues(workw[WORKW_PTXTW], XmNselectedItem, &ptrtxt, NULL);
		XmStringGetLtoR(ptrtxt, XmSTRING_DEFAULT_CHARSET, &txt);
		XmStringFree(ptrtxt);
		Displayopts.opt_restrp = txt[0]  &&  txt[0] != '-'? stracpy(txt): (char *) 0;
#endif
		XtFree(txt);
		XtVaGetValues(workw[WORKW_HTXTW], XmNvalue, &txt, NULL);
		if  (Displayopts.opt_restrt)
			free(Displayopts.opt_restrt);
		Displayopts.opt_restrt = txt[0]? stracpy(txt): (char *) 0;
		XtFree(txt);
		Displayopts.opt_localonly = XmToggleButtonGadgetGetState(workw[WORKW_LOCO]) ? NRESTR_LOCALONLY: NRESTR_NONE;
		Displayopts.opt_sortptrs = XmToggleButtonGadgetGetState(workw[WORKW_SORTPW]) ? SORTP_BYNAME: SORTP_NONE;
		Displayopts.opt_classcode = copyclasscode;
		confabort = copyconfabort;
		Displayopts.opt_jprindisp = (enum jrestrict_t) copyrestrunp;
		Displayopts.opt_jinclude = (enum jincl_t) copyjinclude;
	}
	XtDestroyWidget(GetTopShell(w));
	Job_seg.Last_ser = Ptr_seg.Last_ser = 0;
	pdisplay();
	jdisplay();
}

void	cb_viewopt(Widget parent, int data)
{
	int	cnt;
	Widget	view_shell, paneview, mainform, prevabove, butts, loconly, confbuts, allh, jdisp_but;
	Widget	save_but, prevleft, restrpw, jinclw;
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
	char	**ptrlist = wotjprin();
#endif

	CreateEditDlg(parent, "Viewopts", &view_shell, &paneview, &mainform, 3);
	prevabove = XtVaCreateManagedWidget("viewtitle",
					    xmLabelWidgetClass,		mainform,
					    XmNtopAttachment,		XmATTACH_FORM,
					    XmNleftAttachment,		XmATTACH_FORM,
					    XmNborderWidth,		0,
					    NULL);
	prevabove = CreateUselDialog(mainform, prevabove, Displayopts.opt_restru? Displayopts.opt_restru: "", 1);
#if  	!defined(HAVE_XM_COMBOBOX_H)  ||  defined(BROKEN_COMBOBOX)
	prevabove = CreatePselDialog(mainform, prevabove, Displayopts.opt_restrp? Displayopts.opt_restrp: "", (XtCallbackProc) getptrsel, 1);
#else
	prevabove = CreatePselDialog(mainform, prevabove, Displayopts.opt_restrp, ptrlist, 1, JPTRNAMESIZE);
	freehelp(ptrlist);
#endif

	butts = XtVaCreateManagedWidget("tittitle",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	prevabove = workw[WORKW_HTXTW] = XtVaCreateManagedWidget("title",
								 xmTextFieldWidgetClass,	mainform,
								 XmNcolumns,			MAXTITLE,
								 XmNmaxWidth,			MAXTITLE,
								 XmNcursorPositionVisible,	False,
								 XmNtopAttachment,		XmATTACH_WIDGET,
								 XmNtopWidget,			prevabove,
								 XmNleftAttachment,		XmATTACH_WIDGET,
								 XmNleftWidget,			butts,
								 NULL);

	XmTextSetString(workw[WORKW_HTXTW], Displayopts.opt_restrt? Displayopts.opt_restrt: "");

	butts = XtVaCreateManagedWidget("butts",
					xmRowColumnWidgetClass, mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					XmNrightAttachment,	XmATTACH_FORM,
					XmNpacking,		XmPACK_COLUMN,
					XmNnumColumns,		2,
					NULL);

	loconly = XtVaCreateManagedWidget("loconly",
					  xmRowColumnWidgetClass, butts,
					  XmNpacking,		XmPACK_COLUMN,
					  XmNnumColumns,	1,
					  XmNisHomogeneous,	True,
					  XmNentryClass,	xmToggleButtonGadgetClass,
					  XmNradioBehavior,	True,
					  NULL);

	confbuts = XtVaCreateManagedWidget("confbuts",
					   xmRowColumnWidgetClass, butts,
					   XmNpacking,		XmPACK_COLUMN,
					   XmNnumColumns,	1,
					   XmNisHomogeneous,	True,
					   XmNentryClass,	xmToggleButtonGadgetClass,
					   XmNradioBehavior,	True,
					   NULL);

	allh = XtVaCreateManagedWidget("allhosts",
				       xmToggleButtonGadgetClass,
				       loconly,
				       NULL);
	workw[WORKW_LOCO] = XtVaCreateManagedWidget("localonly",
						    xmToggleButtonGadgetClass,
						    loconly,
						    NULL);

	XmToggleButtonGadgetSetState(Displayopts.opt_localonly != NRESTR_NONE? workw[WORKW_LOCO]: allh, True, False);

	for  (cnt = 0;  cnt < 3;  cnt++)  {
		Widget	w;
		char	name[10];
		sprintf(name, "confb%d", cnt+1);
		w = XtVaCreateManagedWidget(name, xmToggleButtonGadgetClass, confbuts, NULL);
		XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) conftoggle, INT_TO_XTPOINTER(cnt));
		if  (cnt == confabort)
			XmToggleButtonGadgetSetState(w, True, False);
	}
	copyconfabort = confabort;
	prevabove = CreateCCDialog(mainform, butts, Displayopts.opt_classcode, &prevleft);
	restrpw = XtVaCreateManagedWidget("restrp",
					  xmRowColumnWidgetClass,	mainform,
					  XmNtopAttachment,		XmATTACH_WIDGET,
					  XmNtopWidget,			butts,
					  XmNleftAttachment,		XmATTACH_WIDGET,
					  XmNleftWidget,		prevleft,
					  XmNpacking,			XmPACK_COLUMN,
					  XmNnumColumns,		1,
					  XmNisHomogeneous,		True,
					  XmNentryClass,		xmToggleButtonGadgetClass,
					  XmNradioBehavior,		True,
					  NULL);
	for  (cnt = 0;  cnt < 3;  cnt++)  {
		Widget	w;
		char	name[10];
		sprintf(name, "restrp%d", cnt+1);
		w = XtVaCreateManagedWidget(name, xmToggleButtonGadgetClass, restrpw, NULL);
		XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) restrptoggle, INT_TO_XTPOINTER(cnt));
		if  (cnt == (int) Displayopts.opt_jprindisp)
			XmToggleButtonGadgetSetState(w, True, False);
	}
	copyrestrunp = (unsigned char) Displayopts.opt_jprindisp;

	jinclw = XtVaCreateManagedWidget("jincl",
					 xmRowColumnWidgetClass,	mainform,
					 XmNtopAttachment,		XmATTACH_WIDGET,
					 XmNtopWidget,			restrpw,
					 XmNleftAttachment,		XmATTACH_WIDGET,
					 XmNleftWidget,			prevleft,
					 XmNpacking,			XmPACK_COLUMN,
					 XmNnumColumns,			1,
					 XmNisHomogeneous,		True,
					 XmNentryClass,			xmToggleButtonGadgetClass,
					 XmNradioBehavior,		True,
					 NULL);
	for  (cnt = 0;  cnt < 3;  cnt++)  {
		Widget	w;
		char	name[10];
		sprintf(name, "jincl%d", cnt+1);
		w = XtVaCreateManagedWidget(name, xmToggleButtonGadgetClass, jinclw, NULL);
		XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) jincltoggle, INT_TO_XTPOINTER(cnt));
		if  (cnt == (int) Displayopts.opt_jinclude)
			XmToggleButtonGadgetSetState(w, True, False);
	}
	copyjinclude = (unsigned char) Displayopts.opt_jinclude;

	workw[WORKW_SORTPW] = XtVaCreateManagedWidget("sortptrs",
						      xmToggleButtonGadgetClass, mainform,
						      XmNtopAttachment,		 XmATTACH_WIDGET,
						      XmNtopWidget,		 jinclw,
						      XmNleftAttachment,	 XmATTACH_WIDGET,
						      XmNleftWidget,		 prevleft,
						      NULL);
	if  (Displayopts.opt_sortptrs != SORTP_NONE)
		XmToggleButtonGadgetSetState(workw[WORKW_SORTPW], True, False);

	jdisp_but = XtVaCreateManagedWidget("jdispfmt",
					    xmPushButtonWidgetClass,	mainform,
					    XmNtopAttachment,		XmATTACH_WIDGET,
					    XmNtopWidget,		prevabove,
					    XmNleftAttachment,		XmATTACH_FORM,
					    NULL);

	XtAddCallback(jdisp_but, XmNactivateCallback, (XtCallbackProc) cb_setjdisplay, (XtPointer) 0);
	jdisp_but = XtVaCreateManagedWidget("pdispfmt",
					    xmPushButtonWidgetClass,	mainform,
					    XmNtopAttachment,		XmATTACH_WIDGET,
					    XmNtopWidget,		prevabove,
					    XmNleftAttachment,		XmATTACH_WIDGET,
					    XmNleftWidget,		jdisp_but,
					    NULL);

	XtAddCallback(jdisp_but, XmNactivateCallback, (XtCallbackProc) cb_setpdisplay, (XtPointer) 0);

	if  (mypriv->spu_flgs & PV_FREEZEOK)  {
		save_but = XtVaCreateManagedWidget("savehome",
						   xmPushButtonWidgetClass,	mainform,
						   XmNtopAttachment,		XmATTACH_WIDGET,
						   XmNtopWidget,		jdisp_but,
						   XmNleftAttachment,		XmATTACH_FORM,
						   NULL);

		XtAddCallback(save_but, XmNactivateCallback, (XtCallbackProc) cb_saveformats, (XtPointer) 1);

		save_but = XtVaCreateManagedWidget("savecurr",
						   xmPushButtonWidgetClass,	mainform,
						   XmNtopAttachment,		XmATTACH_WIDGET,
						   XmNtopWidget,		jdisp_but,
						   XmNleftAttachment,		XmATTACH_WIDGET,
						   XmNleftWidget,		save_but,
						   NULL);

		XtAddCallback(save_but, XmNactivateCallback, (XtCallbackProc) cb_saveformats, (XtPointer) 0);
	}
	XtManageChild(mainform);
	CreateActionEndDlg(view_shell, paneview, (XtCallbackProc) endviewopt, $H{xmspq dopts help});
}

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

static int	jobmatch(int n)
{
	const  struct	spq	*jp = &Job_seg.jj_ptrs[n]->j;

	if  (searchtit  &&  smstr(jp->spq_file, 0))
		return  1;
	if  (searchform  &&  smstr(jp->spq_form, 0))
		return  1;
	if  (searchptr  &&  smstr(jp->spq_ptr, 0))
		return  1;
	if  (searchuser  &&  smstr(jp->spq_uname, 1))
		return  1;
	return  0;
}

static int	ptrmatch(int n)
{
	const  struct	spptr	*pp = &Ptr_seg.pp_ptrs[n]->p;

	if  (searchform  &&  smstr(pp->spp_form, 0))
		return  1;
	if  (searchptr  &&  smstr(pp->spp_ptr, 0))
		return  1;
	if  (searchdev  &&  smstr(pp->spp_dev, 1))
		return  1;
	return  0;
}

static void	execute_search(void)
{
	int	*plist, pcnt, cline, nline;
	int	topitem, visibleitem;

	if  (searchptrs)  {
		if  (XmListGetSelectedPos(pwid, &plist, &pcnt))  {
			cline = plist[0] - 1;
			XtFree((char *) plist);
		}
		else
			cline = sbackward? (int) Ptr_seg.nptrs: -1;
		if  (sbackward)  {
			for  (nline = cline - 1;  nline >= 0;  nline--)
				if  (ptrmatch(nline))
					goto  gotptr;
			if  (wraparound)
				for  (nline = Ptr_seg.nptrs-1;  nline > cline;  nline--)
					if  (ptrmatch(nline))
						goto  gotptr;
			doerror(pwid, $EH{Rsearch ptr not found});
		}
		else  {
			for  (nline = cline + 1;  nline < Ptr_seg.nptrs;  nline++)
				if  (ptrmatch(nline))
					goto  gotptr;
			if  (wraparound)
				for  (nline = 0;  nline < cline;  nline++)
					if  (ptrmatch(nline))
						goto  gotptr;
			doerror(pwid, $EH{Fsearch ptr not found});
		}
		return;
	gotptr:
		nline++;
		XmListSelectPos(pwid, nline, False);
		XtVaGetValues(pwid, XmNtopItemPosition, &topitem, XmNvisibleItemCount, &visibleitem, NULL);
		if  (nline < topitem)
			XmListSetPos(pwid, nline);
		else  if  (nline >= topitem+visibleitem)
			XmListSetBottomPos(pwid, nline);
	}
	else  {
		if  (XmListGetSelectedPos(jwid, &plist, &pcnt))  {
			cline = plist[0] - 1;
			XtFree((char *) plist);
		}
		else
			cline = sbackward? (int) Job_seg.njobs: -1;
		if  (sbackward)  {
			for  (nline = cline - 1;  nline >= 0;  nline--)
				if  (jobmatch(nline))
					goto  gotjob;
			if  (wraparound)
				for  (nline = Job_seg.njobs-1;  nline > cline;  nline--)
					if  (jobmatch(nline))
						goto  gotjob;
			doerror(jwid, $EH{Rsearch job not found});
		}
		else  {
			for  (nline = cline + 1;  nline < Job_seg.njobs;  nline++)
				if  (jobmatch(nline))
					goto  gotjob;
			if  (wraparound)
				for  (nline = 0;  nline < cline;  nline++)
					if  (jobmatch(nline))
						goto  gotjob;
			doerror(jwid, $EH{Fsearch job not found});
		}
		return;
	gotjob:
		nline++;
		XmListSelectPos(jwid, nline, False);
		XtVaGetValues(jwid, XmNtopItemPosition, &topitem, XmNvisibleItemCount, &visibleitem, NULL);
		if  (nline < topitem)
			XmListSetPos(jwid, nline);
		else  if  (nline >= topitem+visibleitem)
			XmListSetBottomPos(jwid, nline);
	}
}

void InitsearchDlg(Widget parent, Widget *shellp, Widget *panep, Widget *formp, char *existing)
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
}

void  InitsearchOpts(Widget formw, Widget after, int sback, int matchc, int wrap)
{
	Widget	dirrc, prevleft;

	dirrc = XtVaCreateManagedWidget("dirn",
					xmRowColumnWidgetClass,		formw,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			after,
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

	XmToggleButtonGadgetSetState(sback? prevleft: workw[WORKW_FORWW], True, False);

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
	if  (matchc)
		XmToggleButtonGadgetSetState(workw[WORKW_MATCHW], True, False);

	workw[WORKW_WRAPW] =  XtVaCreateManagedWidget("wrap",
						      xmToggleButtonGadgetClass,	dirrc,
						      XmNborderWidth,			0,
						      NULL);
	if  (wrap)
		XmToggleButtonGadgetSetState(workw[WORKW_WRAPW], True, False);

	XtManageChild(formw);
}

static void	changesearch(Widget w, int unused, XmToggleButtonCallbackStruct * cbs)
{
	if  (cbs->set)  {
		searchptrs = 1;
		XtSetSensitive(workw[WORKW_HTXTW], False);
		XtSetSensitive(workw[WORKW_UTXTW], False);
		XtSetSensitive(workw[WORKW_DBUTW], True);
	}
	else  {
		searchptrs = 0;
		XtSetSensitive(workw[WORKW_DBUTW], False);
		XtSetSensitive(workw[WORKW_HTXTW], True);
		XtSetSensitive(workw[WORKW_UTXTW], True);
	}
}

static void	endsdlg(Widget w, int data)
{
	if  (data)  {
		sbackward = XmToggleButtonGadgetGetState(workw[WORKW_FORWW])? 0: 1;
		matchcase = XmToggleButtonGadgetGetState(workw[WORKW_MATCHW])? 1: 0;
		wraparound = XmToggleButtonGadgetGetState(workw[WORKW_WRAPW])? 1: 0;
		searchtit = XmToggleButtonGadgetGetState(workw[WORKW_HTXTW])? 1: 0;
		searchform = XmToggleButtonGadgetGetState(workw[WORKW_FTXTW])? 1: 0;
		searchptr = XmToggleButtonGadgetGetState(workw[WORKW_PTXTW])? 1: 0;
		searchuser = XmToggleButtonGadgetGetState(workw[WORKW_UTXTW])? 1: 0;
		searchdev = XmToggleButtonGadgetGetState(workw[WORKW_DBUTW])? 1: 0;
		if  (searchptrs)  {
			if  (!(searchptr || searchdev || searchform))  {
				doerror(w, $EH{No ptr crits given});
				return;
			}
		}
		else  if  (!(searchtit || searchform || searchptr || searchuser))  {
			doerror(w, $EH{No job crits given});
			return;
		}
		if  (matchtext)	/* Last time round */
			XtFree(matchtext);
		XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &matchtext, NULL);
		if  (matchtext[0] == '\0')  {
			doerror(w, $EH{Rsearch ptr});
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
	Widget	s_shell, panew, formw, orc, jorp, whp;

	InitsearchDlg(parent, &s_shell, &panew, &formw, matchtext);

	orc = XtVaCreateManagedWidget("sopts",
				      xmRowColumnWidgetClass,		formw,
				      XmNtopAttachment,			XmATTACH_WIDGET,
				      XmNtopWidget,			workw[WORKW_STXTW],
				      XmNleftAttachment,		XmATTACH_FORM,
				      XmNrightAttachment,		XmATTACH_FORM,
				      XmNpacking,			XmPACK_COLUMN,
				      XmNnumColumns,			2,
				      NULL);

	jorp = XtVaCreateManagedWidget("jorp",
				       xmRowColumnWidgetClass,		orc,
				       XmNpacking,			XmPACK_COLUMN,
				       XmNnumColumns,			1,
				       XmNisHomogeneous,		True,
				       XmNentryClass,			xmToggleButtonGadgetClass,
				       XmNradioBehavior,		True,
				       NULL);

	workw[WORKW_JBUTW] = XtVaCreateManagedWidget("jobs",
						     xmToggleButtonGadgetClass,	jorp,
						     XmNborderWidth,		0,
						     NULL);

	workw[WORKW_PBUTW] = XtVaCreateManagedWidget("ptrs",
						     xmToggleButtonGadgetClass,	jorp,
						     XmNborderWidth,		0,
						     NULL);

	XmToggleButtonGadgetSetState(searchptrs? workw[WORKW_PBUTW]: workw[WORKW_JBUTW], True, False);
	XtAddCallback(workw[WORKW_PBUTW], XmNvalueChangedCallback, (XtCallbackProc) changesearch, (XtPointer) 0);

	whp = XtVaCreateManagedWidget("which",
				      xmRowColumnWidgetClass,		orc,
				      XmNpacking,			XmPACK_COLUMN,
				      XmNnumColumns,			1,
				      XmNisHomogeneous,			True,
				      XmNentryClass,			xmToggleButtonGadgetClass,
				      XmNradioBehavior,			False,
				      NULL);

	workw[WORKW_HTXTW] = XtVaCreateManagedWidget("title",
						     xmToggleButtonGadgetClass,	whp,
						     XmNborderWidth,		0,
						     NULL);

	if  (searchtit)
		XmToggleButtonGadgetSetState(workw[WORKW_HTXTW], True, False);

	workw[WORKW_FTXTW] = XtVaCreateManagedWidget("form",
						     xmToggleButtonGadgetClass,	whp,
						     XmNborderWidth,		0,
						     NULL);

	if  (searchform)
		XmToggleButtonGadgetSetState(workw[WORKW_FTXTW], True, False);

	workw[WORKW_PTXTW] = XtVaCreateManagedWidget("ptr",
						     xmToggleButtonGadgetClass,	whp,
						     XmNborderWidth,		0,
						     NULL);

	if  (searchptr)
		XmToggleButtonGadgetSetState(workw[WORKW_PTXTW], True, False);

	workw[WORKW_UTXTW] = XtVaCreateManagedWidget("user",
						     xmToggleButtonGadgetClass,	whp,
						     XmNborderWidth,		0,
						     NULL);

	if  (searchuser)
		XmToggleButtonGadgetSetState(workw[WORKW_UTXTW], True, False);

	workw[WORKW_DBUTW] = XtVaCreateManagedWidget("dev",
						     xmToggleButtonGadgetClass,	whp,
						     XmNborderWidth,		0,
						     NULL);

	if  (searchdev)
		XmToggleButtonGadgetSetState(workw[WORKW_DBUTW], True, False);

	if  (searchptrs)  {
		XtSetSensitive(workw[WORKW_HTXTW], False);
		XtSetSensitive(workw[WORKW_UTXTW], False);
	}
	else
		XtSetSensitive(workw[WORKW_DBUTW], False);

	InitsearchOpts(formw, orc, sbackward, matchcase, wraparound);
	CreateActionEndDlg(s_shell, panew, (XtCallbackProc) endsdlg, $H{xmspq search menu help});
}

void	cb_chelp(Widget w, int data, XmAnyCallbackStruct *cbs)
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
