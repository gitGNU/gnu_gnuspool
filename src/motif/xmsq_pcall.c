/* xmsq_pcall.c -- xmspq callbacks for printers

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
#include <sys/types.h>
#include <Xm/ArrowB.h>
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
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include "defaults.h"
#include "files.h"
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

static const struct spptr *getselectedptr(ULONG perm, int code)
{
	int	*plist, pcnt;

	if  (XmListGetSelectedPos(pwid, &plist, &pcnt)  &&  pcnt > 0)  {
		const  Hashspptr  *result = Ptr_seg.pp_ptrs[plist[0] - 1];
		const  struct  spptr	*rpt = &result->p;
		XtFree((char *) plist);
		if  (perm  &&  !(mypriv->spu_flgs & perm))  {
			disp_str = rpt->spp_ptr;
			disp_str2 = rpt->spp_dev;
			doerror(pwid, code);
			return  NULL;
		}
		if  (rpt->spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))  {
			doerror(pwid, $EH{No remote ptr priv});
			return  NULL;
		}
		PREQ = *rpt;
		OREQ = PREQS = result - Ptr_seg.plist;
		return  rpt;
	}
	doerror(pwid, Ptr_seg.nptrs != 0? $EH{No printer selected}: $EH{No printer to select});
	return  NULL;
}

/* Printer actions.
   SO_PGO, SO_PHLT, SO_PSTP, SO_OYES, SO_ONO,
   SO_INTER, SO_PJAB, SO_RSP, SO_DELP */

void  cb_pact(Widget wid, int msg)
{
	const  struct	spptr	*pp;

	switch  (msg)  {
	default:
		return;
	case  SO_PGO:
		pp = getselectedptr(PV_HALTGO, $EH{No stop start priv});
		if  (!pp)
			return;
		if  (pp->spp_state >= SPP_PROC  &&  !(pp->spp_sflags & SPP_HEOJ))  {
			doerror(pwid, $EH{Printer is running});
			return;
		}
		break;
	case  SO_PHLT:
	case  SO_PSTP:
		pp = getselectedptr(PV_HALTGO, $EH{No stop start priv});
		if  (!pp)
			return;
		if  (pp->spp_state < SPP_PROC)  {
			doerror(pwid, $EH{Printer not running});
			return;
		}
		break;
	case  SO_OYES:
	case  SO_ONO:
		pp = getselectedptr(PV_PRINQ, $EH{No printer list access});
		if  (!pp)
			return;
		if  (pp->spp_state != SPP_OPER)  {
			if  (pp->spp_state != SPP_WAIT)  {
				doerror(pwid, $EH{Printer not aw oper});
				return;
			}
			disp_str = pp->spp_ptr;
			if  (!Confirm(pwid, msg == SO_OYES? $PH{xmspq bypass align}: $PH{xmspq reinstate align}))
				return;
		}
		break;
	case  SO_INTER:
		pp = getselectedptr(PV_HALTGO, $EH{No stop start priv});
		if  (!pp)
			return;
		if  (pp->spp_state != SPP_RUN)  {
			doerror(pwid, $EH{Printer not running});
			return;
		}
		break;
	case  SO_PJAB:
	case  SO_RSP:
		pp = getselectedptr(PV_PRINQ, $EH{No printer list access});
		if  (!pp)
			return;
		if  (pp->spp_state != SPP_RUN)  {
			doerror(pwid, $EH{Printer not running});
			return;
		}
		break;
	case  SO_DELP:
		pp = getselectedptr(PV_ADDDEL, $EH{No delete priv});
		if  (!pp)
			return;
		if  (pp->spp_netid)  {
			disp_str = look_host(pp->spp_netid);
			doerror(pwid, $EH{Printer is remote});
			return;
		}
		disp_str = pp->spp_ptr;
		if  (!Confirm(pwid, $PH{xmspq confirm delete printer}))
			return;
		if  (pp->spp_state >= SPP_PROC)  {
			doerror(pwid, $EH{Printer is running});
			return;
		}
		break;
	}
	womsg(msg);
}

static void  pform_cb(Widget w, XtPointer cldata, XmSelectionBoxCallbackStruct *cbs)
{
	char	*value;

	XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
	if  (!value[0])  {
		doerror(w, $EH{Null form name});
		XtFree(value);
		return;
	}
	if  (strcmp(PREQ.spp_form, value) != 0)  {
		strncpy(PREQ.spp_form, value, MAXFORM);
		my_wpmsg(SP_CHGP);
	}
	XtFree(value);
	XtDestroyWidget(w);
}

void  cb_pform()
{
	const  struct  spptr  *pp = getselectedptr(PV_PRINQ, $EH{No printer list access});
	char	**flist;
	XmString	*strlist, existing;
	Widget	dw;
	int	rows, cnt;
	if  (!pp)
		return;

	if  (pp->spp_state >= SPP_PROC)  {
		doerror(pwid, $EH{Printer is running});
		return;
	}

	flist = wotpform();
	count_hv(flist, &rows, &cnt);
	dw = XmCreateSelectionDialog(pwid, "pforms", NULL, 0);
	existing = XmStringCreateLocalized((char *) pp->spp_form);
	if  (rows <= 0)  {
		XtVaSetValues(dw,
			      XmNlistItemCount,	0,
			      XmNtextString,	existing,
			      NULL);
	}
	else  {
		strlist = (XmString *) XtMalloc(rows * sizeof(XmString));
		for  (cnt = 0;  cnt < rows;  cnt++)  {
			strlist[cnt] = XmStringCreateLocalized(flist[cnt]);
			free(flist[cnt]);
		}
		free((char *) flist);
		XtVaSetValues(dw,
			      XmNlistItems,	strlist,
			      XmNlistItemCount,	rows,
			      XmNtextString,	existing,
			      NULL);
		for  (cnt = 0;  cnt < rows;  cnt++)
			XmStringFree(strlist[cnt]);
		XtFree((char *) strlist);
	}
	XmStringFree(existing);
	XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_APPLY_BUTTON));
	XtAddCallback(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON),
		      XmNactivateCallback, (XtCallbackProc) pformhelp, (XtPointer) 1);
	XtAddCallback(dw, XmNokCallback, (XtCallbackProc) pform_cb, (XtPointer) 0);
	XtManageChild(dw);
}

static void  endpclass(Widget w, int data)
{
	if  (data)  {
		if  (PREQ.spp_minsize != 0  &&  PREQ.spp_maxsize != 0  &&  PREQ.spp_minsize > PREQ.spp_maxsize)  {
			doerror(w, $EH{Minprint gt maxprint});
			return;
		}
		if  (copyclasscode == 0)  {
			doerror(w, $EH{xmspq setting zero class ptr});
			return;
		}
		if  (XmToggleButtonGadgetGetState(workw[WORKW_LOCO]))
			PREQ.spp_netflags |= SPP_LOCALONLY;
		else
			PREQ.spp_netflags &= ~SPP_LOCALONLY;

		PREQ.spp_class = copyclasscode;
		my_wpmsg(SP_CHGP);
	}
	XtDestroyWidget(GetTopShell(w));
}

static void  maxmin_incr(int w, XtIntervalId *id)
{
	ULONG	*wp;

	wp = w == WORKW_PMINW? &PREQ.spp_minsize: &PREQ.spp_maxsize;

	*wp += 1000L;
	XmTextSetString(workw[w], prin_size(*wp));
	arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) maxmin_incr, INT_TO_XTPOINTER(w));
}

static void  maxmin_decr(int w, XtIntervalId *id)
{
	ULONG	*wp;

	wp = w == WORKW_PMINW? &PREQ.spp_minsize: &PREQ.spp_maxsize;

	if  (*wp < 1000L)
		*wp = 0L;
	else
		*wp -= 1000L;
	XmTextSetString(workw[w], prin_size(*wp));
	arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) maxmin_decr, INT_TO_XTPOINTER(w));
}

static void  upmaxmin(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)
		maxmin_incr(subj, NULL);
	else
		XtRemoveTimeOut(arrow_timer);
}

static void  dnmaxmin(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)
		maxmin_decr(subj, NULL);
	else
		XtRemoveTimeOut(arrow_timer);
}

void  cb_pclass(Widget parent)
{
	Widget	pc_shell, panew, mainform, titw;
	const  struct	spptr	*pp = getselectedptr(PV_ADDDEL, $EH{No perm change class});
	char	nbuf[HOSTNSIZE+PTRNAMESIZE];
	XmString	tit;

	if  (!pp)
		return;

	if  (pp->spp_state >= SPP_PROC)  {
		doerror(pwid, $EH{Printer is running});
		return;
	}

	if  (pp->spp_netid)
		sprintf(nbuf, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
	else
		sprintf(nbuf, "%s", pp->spp_ptr);

	CreateEditDlg(parent, "Pclass", &pc_shell, &panew, &mainform, 3);

	titw = XtVaCreateManagedWidget("ptrnametitle",
				xmLabelWidgetClass,	mainform,
				XmNtopAttachment,	XmATTACH_FORM,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNborderWidth,		0,
				NULL);

	tit = XmStringCreateLocalized(nbuf);
	titw = XtVaCreateManagedWidget("ptrname",
				       xmLabelWidgetClass,	mainform,
				       XmNlabelString,		tit,
				       XmNtopAttachment,	XmATTACH_FORM,
				       XmNleftAttachment,	XmATTACH_WIDGET,
				       XmNleftWidget,		titw,
				       XmNborderWidth,		0,
				       NULL);
	XmStringFree(tit);

	titw = CreateCCDialog(mainform, titw, pp->spp_class, (Widget *) 0);
	workw[WORKW_LOCO] = XtVaCreateManagedWidget("loconly",
						    xmToggleButtonGadgetClass,	mainform,
						    XmNtopAttachment,		XmATTACH_WIDGET,
						    XmNtopWidget,		titw,
						    XmNleftAttachment,		XmATTACH_FORM,
						    NULL);
	if  (pp->spp_netflags & SPP_LOCALONLY)
		XmToggleButtonGadgetSetState(workw[WORKW_LOCO], True, False);

	titw = XtVaCreateManagedWidget("min",
				       xmLabelWidgetClass,	mainform,
				       XmNtopAttachment,	XmATTACH_WIDGET,
				       XmNtopWidget,		workw[WORKW_LOCO],
				       XmNleftAttachment,	XmATTACH_FORM,
				       NULL);

	workw[WORKW_PMINW] = XtVaCreateManagedWidget("mins",
						     xmTextFieldWidgetClass,	mainform,
						     XmNcolumns,		5,
						     XmNmaxWidth,		5,
						     XmNcursorPositionVisible,	False,
						     XmNeditable,		False,
						     XmNtopAttachment,		XmATTACH_WIDGET,
						     XmNtopWidget,		workw[WORKW_LOCO],
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		titw,
						     NULL);

	XmTextSetString(workw[WORKW_PMINW], prin_size(pp->spp_minsize));
	CreateArrowPair("min",				mainform,
			workw[WORKW_LOCO],		workw[WORKW_PMINW],
			(XtCallbackProc) upmaxmin,	(XtCallbackProc) dnmaxmin,
			WORKW_PMINW,		WORKW_PMINW,	1);

	titw = XtVaCreateManagedWidget("max",
				       xmLabelWidgetClass,	mainform,
				       XmNtopAttachment,	XmATTACH_WIDGET,
				       XmNtopWidget,		workw[WORKW_PMINW],
				       XmNleftAttachment,	XmATTACH_FORM,
				       NULL);

	workw[WORKW_PMAXW] = XtVaCreateManagedWidget("maxs",
						     xmTextFieldWidgetClass,	mainform,
						     XmNcolumns,		5,
						     XmNmaxWidth,		5,
						     XmNcursorPositionVisible,	False,
						     XmNeditable,		False,
						     XmNtopAttachment,		XmATTACH_WIDGET,
						     XmNtopWidget,		workw[WORKW_PMINW],
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		titw,
						     NULL);

	XmTextSetString(workw[WORKW_PMAXW], prin_size(pp->spp_maxsize));
	CreateArrowPair("max",				mainform,
			workw[WORKW_PMINW],		workw[WORKW_PMAXW],
			(XtCallbackProc) upmaxmin,	(XtCallbackProc) dnmaxmin,
			WORKW_PMAXW,		WORKW_PMAXW,	1);

	XtManageChild(mainform);
	CreateActionEndDlg(pc_shell, panew, (XtCallbackProc) endpclass, $H{Ptr class dlg});
}

static void  endnewp(Widget w, int data)
{
	char		*txt;
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
	XmString	ptrtxt;
#endif
	if  (!data)  {
		XtDestroyWidget(GetTopShell(w));
		return;
	}
	if  (copyclasscode == 0)  {
		doerror(w, $EH{Ptr null class});
		return;
	}
	BLOCK_ZERO(&PREQ, sizeof(PREQ));
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
	XtVaGetValues(workw[WORKW_PTXTW], XmNselectedItem, &ptrtxt, NULL);
	XmStringGetLtoR(ptrtxt, XmSTRING_DEFAULT_CHARSET, &txt);
	XmStringFree(ptrtxt);
	if  (strlen(txt) == 0  ||  txt[0] == '-')  {
		XtFree(txt);
		doerror(pwid, $EH{Null ptr name});
		return;
	}
#else
	XtVaGetValues(workw[WORKW_PTXTW], XmNvalue, &txt, NULL);
	if  (strlen(txt) == 0)  {
		XtFree(txt);
		doerror(pwid, $EH{Null ptr name});
		return;
	}
#endif
	strncpy(PREQ.spp_ptr, txt, PTRNAMESIZE);
	XtFree(txt);
	XtVaGetValues(workw[WORKW_FTXTW], XmNvalue, &txt, NULL);
	if  (strlen(txt) == 0)  {
		XtFree(txt);
		doerror(pwid, $EH{Null form name});
		return;
	}
	strncpy(PREQ.spp_form, txt, MAXFORM);
	XtFree(txt);
	XtVaGetValues(workw[WORKW_DEVTXTW], XmNvalue, &txt, NULL);
	if  (strlen(txt) == 0)  {
		XtFree(txt);
		doerror(pwid, $EH{Null device name});
		return;
	}
	strncpy(PREQ.spp_dev, txt, LINESIZE);
	XtFree(txt);
	XtVaGetValues(workw[WORKW_PDESCRW], XmNvalue, &txt, NULL);
	strncpy(PREQ.spp_comment, txt, COMMENTSIZE);
	XtFree(txt);
	if  (XmToggleButtonGadgetGetState(workw[WORKW_DEVNETW]))
		PREQ.spp_netflags |= SPP_LOCALNET;
	else  {
		int	valcode;
		valcode = validatedev(PREQ.spp_dev);
		if  (valcode != 0  &&  !Confirm(pwid, valcode))
			return;
	}
	if  (XmToggleButtonGadgetGetState(workw[WORKW_LOCO]))
		PREQ.spp_netflags |= SPP_LOCALONLY;
	PREQ.spp_class = copyclasscode;
	PREQ.spp_extrn = 0;
	my_wpmsg(SP_ADDP);
	XtDestroyWidget(GetTopShell(w));
}

void  cb_padd(Widget parent)
{
	Widget	padd_shell, panew, mainform, prevabove,
		dtitw,		dselb,
		ftitw,		formb;
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
	char	**ptrlist;
#endif

	if  (!(mypriv->spu_flgs & PV_ADDDEL))  {
		doerror(pwid, $EH{No add priv});
		return;
	}

	CreateEditDlg(parent, "Padd", &padd_shell, &panew, &mainform, 3);

	prevabove = XtVaCreateManagedWidget("newprinter",
					    xmLabelWidgetClass,	mainform,
					    XmNtopAttachment,	XmATTACH_FORM,
					    XmNleftAttachment,	XmATTACH_FORM,
					    XmNborderWidth,	0,
					    NULL);

#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
	ptrlist = wotpprin();
	prevabove = CreatePselDialog(mainform, prevabove, (char *) 0, ptrlist, 0, PTRNAMESIZE);
	freehelp(ptrlist);
#else
	prevabove = CreatePselDialog(mainform, prevabove, (char *) 0, (XtCallbackProc) getnewptrsel, 0);
#endif

	dtitw = XtVaCreateManagedWidget("Dev",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	workw[WORKW_DEVTXTW] = XtVaCreateManagedWidget("device",
						       xmTextFieldWidgetClass,	mainform,
						       XmNcolumns,		LINESIZE,
						       XmNmaxWidth,		LINESIZE,
						       XmNcursorPositionVisible,False,
						       XmNtopAttachment,	XmATTACH_WIDGET,
						       XmNtopWidget,		prevabove,
						       XmNleftAttachment,	XmATTACH_WIDGET,
						       XmNleftWidget,		dtitw,
						       NULL);

	workw[WORKW_DEVNETW] = XtVaCreateManagedWidget("netfilt",
						       xmToggleButtonGadgetClass, mainform,
						       XmNtopAttachment,	XmATTACH_WIDGET,
						       XmNtopWidget,		prevabove,
						       XmNleftAttachment,	XmATTACH_WIDGET,
						       XmNleftWidget,		workw[WORKW_DEVTXTW],
						       XmNborderWidth,		0,
						       NULL);

	dselb = XtVaCreateManagedWidget("dselect",
					xmPushButtonWidgetClass,	mainform,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			workw[WORKW_DEVNETW],
					NULL);

	XtAddCallback(dselb, XmNactivateCallback, (XtCallbackProc) getdevsel, (XtPointer) 0);

	ftitw = XtVaCreateManagedWidget("Form",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		workw[WORKW_DEVTXTW],
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	prevabove =
		workw[WORKW_FTXTW] =
			XtVaCreateManagedWidget("fname",
						xmTextFieldWidgetClass,		mainform,
						XmNcolumns,			MAXFORM,
						XmNmaxWidth,			MAXFORM,
						XmNcursorPositionVisible,	False,
						XmNtopAttachment,		XmATTACH_WIDGET,
						XmNtopWidget,			workw[WORKW_DEVTXTW],
						XmNleftAttachment,		XmATTACH_WIDGET,
						XmNleftWidget,			ftitw,
						NULL);

	formb = XtVaCreateManagedWidget("fselect",
					xmPushButtonWidgetClass,	mainform,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			workw[WORKW_DEVTXTW],
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			workw[WORKW_FTXTW],
					NULL);

	XtAddCallback(formb, XmNactivateCallback, (XtCallbackProc) getformsel, (XtPointer) 1);

	dtitw = XtVaCreateManagedWidget("Descr",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	prevabove = workw[WORKW_PDESCRW] = XtVaCreateManagedWidget("descr",
								   xmTextFieldWidgetClass,	mainform,
								   XmNcolumns,			COMMENTSIZE,
								   XmNmaxWidth,			COMMENTSIZE,
								   XmNcursorPositionVisible,	False,
								   XmNtopAttachment,		XmATTACH_WIDGET,
								   XmNtopWidget,		prevabove,
								   XmNleftAttachment,		XmATTACH_WIDGET,
								   XmNleftWidget,		dtitw,
								   NULL);

	prevabove = CreateCCDialog(mainform, prevabove, Displayopts.opt_classcode, (Widget *) 0);

	workw[WORKW_LOCO] = XtVaCreateManagedWidget("loconly",
						    xmToggleButtonGadgetClass, mainform,
						    XmNtopAttachment,		XmATTACH_WIDGET,
						    XmNtopWidget,		prevabove,
						    XmNleftAttachment,		XmATTACH_FORM,
						    NULL);

	XtManageChild(mainform);
	CreateActionEndDlg(padd_shell, panew, (XtCallbackProc) endnewp, $H{Ptr add dlg});
}

static void  pmacroexec(char *str, const struct spptr *pp)
{
	static	char	*execprog;
	PIDTYPE	pid;
	int	status;

	if  (!execprog)
		execprog = envprocess(EXECPROG);

	if  ((pid = fork()) == 0)  {
		char	*argbuf[3];
		char	nbuf[PTRNAMESIZE+HOSTNSIZE+2];
		argbuf[0] = str;
		if  (pp)  {
			if  (pp->spp_netid)  {
				sprintf(nbuf, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
				argbuf[1] = nbuf;
			}
			else
				argbuf[1] = (char *) pp->spp_ptr;
			argbuf[2] = (char *) 0;
		}
		else
			argbuf[1] = (char *) 0;
		chdir(Curr_pwd);
		execv(execprog, argbuf);
		exit(255);
	}
	if  (pid < 0)  {
		doerror(pwid, $EH{Macro fork failed});
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
			doerror(pwid, $EH{Macro command gave signal});
		}
		else  {
			disp_arg[0] = (status >> 8) & 255;
			doerror(pwid, $EH{Macro command error});
		}
	}
}

static void  endpmacro(Widget w, int data)
{
	if  (data)  {
		char	*txt;
		XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &txt, NULL);
		if  (txt[0])
			pmacroexec(txt, &PREQ);
		XtFree(txt);
	}
	XtDestroyWidget(GetTopShell(w));
}

void  cb_macrop(Widget parent, int data)
{
	char	*prompt = helpprmpt(data + $P{Printer macro});
	int	*plist, pcnt;
	const  struct	spptr	*pp = (const struct spptr *) 0;
	Widget	pc_shell, panew, mainform, labw;

	if  (!prompt)  {
		disp_arg[0] = data + $P{Printer macro};
		doerror(pwid, $EH{Macro error});
		return;
	}

	if  (XmListGetSelectedPos(pwid, &plist, &pcnt) && pcnt > 0)  {
		pp = &Ptr_seg.pp_ptrs[plist[0] - 1]->p;
		XtFree((XtPointer) plist);
	}

	if  (data != 0)  {
		pmacroexec(prompt, pp);
		return;
	}

	PREQ = *pp;

	CreateEditDlg(parent, "ptrcmd", &pc_shell, &panew, &mainform, 3);
	labw = XtVaCreateManagedWidget("cmdtit",
				       xmLabelGadgetClass,	mainform,
				       XmNtopAttachment,	XmATTACH_FORM,
				       XmNleftAttachment,	XmATTACH_FORM,
				       NULL);
	workw[WORKW_STXTW] = XtVaCreateManagedWidget("cmd",
				       xmTextFieldWidgetClass,	mainform,
				       XmNcolumns,		20,
				       XmNcursorPositionVisible,False,
				       XmNtopAttachment,	XmATTACH_FORM,
				       XmNleftAttachment,	XmATTACH_WIDGET,
				       XmNleftWidget,		labw,
				       NULL);
	XtManageChild(mainform);
	CreateActionEndDlg(pc_shell, panew, (XtCallbackProc) endpmacro, $H{Printer macro});
}

static void  endpdev(Widget w, int data)
{
	char	*txt;
	if  (!data)  {
		XtDestroyWidget(GetTopShell(w));
		return;
	}
	XtVaGetValues(workw[WORKW_DEVTXTW], XmNvalue, &txt, NULL);
	if  (strlen(txt) == 0)  {
		XtFree(txt);
		doerror(pwid, $EH{Null device name});
		return;
	}
	strncpy(PREQ.spp_dev, txt, LINESIZE);
	XtFree(txt);
	if  (XmToggleButtonGadgetGetState(workw[WORKW_DEVNETW]))
		PREQ.spp_netflags |= SPP_LOCALNET;
	else  {
		int	valcode;
		valcode = validatedev(PREQ.spp_dev);
		if  (valcode != 0  &&  !Confirm(pwid, valcode))
			return;
	}
	XtVaGetValues(workw[WORKW_PDESCRW], XmNvalue, &txt, NULL);
	strncpy(PREQ.spp_comment, txt, COMMENTSIZE);
	XtFree(txt);
	my_wpmsg(SP_CHGP);
	XtDestroyWidget(GetTopShell(w));
}

void  cb_pdev(Widget w)
{
	Widget	pd_shell, panew, mainform, titw, dtitw, dselb;
	const  struct	spptr	*pp = getselectedptr(PV_ADDDEL, $EH{No perm change dev});
	char	nbuf[HOSTNSIZE+PTRNAMESIZE];
	XmString	tit;

	if  (!pp)
		return;

	if  (pp->spp_state >= SPP_PROC)  {
		doerror(pwid, $EH{Printer is running});
		return;
	}

	if  (pp->spp_netid)  {
		doerror(pwid, $EH{Cannot change dev remote});
		return;
	}

	sprintf(nbuf, "%s", pp->spp_ptr);

	CreateEditDlg(w, "Pdev", &pd_shell, &panew, &mainform, 3);

	titw = XtVaCreateManagedWidget("ptrnametitle",
				xmLabelWidgetClass,	mainform,
				XmNtopAttachment,	XmATTACH_FORM,
				XmNleftAttachment,	XmATTACH_FORM,
				XmNborderWidth,		0,
				NULL);

	tit = XmStringCreateLocalized(nbuf);
	titw = XtVaCreateManagedWidget("ptrname",
				       xmLabelWidgetClass,	mainform,
				       XmNlabelString,		tit,
				       XmNtopAttachment,	XmATTACH_FORM,
				       XmNleftAttachment,	XmATTACH_WIDGET,
				       XmNleftWidget,		titw,
				       XmNborderWidth,		0,
				       NULL);
	XmStringFree(tit);

	dtitw = XtVaCreateManagedWidget("Dev",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		titw,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	workw[WORKW_DEVTXTW] = XtVaCreateManagedWidget("device",
						       xmTextFieldWidgetClass,	mainform,
						       XmNcolumns,		LINESIZE,
						       XmNmaxWidth,		LINESIZE,
						       XmNcursorPositionVisible,False,
						       XmNtopAttachment,	XmATTACH_WIDGET,
						       XmNtopWidget,		titw,
						       XmNleftAttachment,	XmATTACH_WIDGET,
						       XmNleftWidget,		dtitw,
						       NULL);

	XmTextSetString(workw[WORKW_DEVTXTW], (char *) pp->spp_dev);

	workw[WORKW_DEVNETW] = XtVaCreateManagedWidget("netfilt",
						       xmToggleButtonGadgetClass, mainform,
						       XmNtopAttachment,	XmATTACH_WIDGET,
						       XmNtopWidget,		titw,
						       XmNleftAttachment,	XmATTACH_WIDGET,
						       XmNleftWidget,		workw[WORKW_DEVTXTW],
						       XmNborderWidth,		0,
						       NULL);

	dselb = XtVaCreateManagedWidget("dselect",
					xmPushButtonWidgetClass,	mainform,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			titw,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			workw[WORKW_DEVNETW],
					NULL);

	XtAddCallback(dselb, XmNactivateCallback, (XtCallbackProc) getdevsel, (XtPointer) 0);

	dtitw = XtVaCreateManagedWidget("Descr",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		workw[WORKW_DEVNETW],
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	workw[WORKW_PDESCRW] = XtVaCreateManagedWidget("descr",
						       xmTextFieldWidgetClass,	mainform,
						       XmNcolumns,		COMMENTSIZE,
						       XmNmaxWidth,		COMMENTSIZE,
						       XmNcursorPositionVisible,False,
						       XmNtopAttachment,	XmATTACH_WIDGET,
						       XmNtopWidget,		workw[WORKW_DEVNETW],
						       XmNleftAttachment,	XmATTACH_WIDGET,
						       XmNleftWidget,		dtitw,
						       NULL);

	XmTextSetString(workw[WORKW_PDESCRW], (char *) pp->spp_comment);

	XtManageChild(mainform);
	CreateActionEndDlg(pd_shell, panew, (XtCallbackProc) endpdev, $H{Ptr pdev dlg});
}
