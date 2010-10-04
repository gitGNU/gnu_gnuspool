/* xmsq_jcall.c -- xmspq callbacks for jobs

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
#include <sys/stat.h>
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <Xm/ArrowB.h>
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
#ifdef HAVE_XM_SPINB_H
#include <Xm/SpinB.h>
#endif
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include "defaults.h"
#include "network.h"
#include "files.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "xmsq_ext.h"

#define	SECSPERDAY	(24L * 60L * 60L)
#define	MAXLONG	0x7fffffffL	/*  Change this?  */

XtIntervalId	arrow_timer;
#ifndef HAVE_XM_SPINB_H
unsigned	arrow_min,
		arrow_max,
		arrow_lng;
#endif

static	char	*emsg;		/* "End page" message */

static	char	*daynames[7],
		*monnames[12];

#ifdef HAVE_XM_SPINB_H
static	XmStringTable	timezerof,
			stdaynames,
			stmonnames;

#endif
static	char	*Last_unqueue_dir;

static	int	longest_day = 0,
		longest_mon = 0;

Widget	holdspin;

#ifndef HAVE_XM_SPINB_H
/* Generic callback for increment arrows.  id is NULL the first time
   to get a different timeout The passed parameter is the Widget
   we are mangling */

void  arrow_incr(Widget w, XtIntervalId *id)
{
	unsigned newval;
	char	*txt, newtxt[20];

	XtVaGetValues(w, XmNvalue, &txt, NULL);
	newval = atoi(txt) + 1;
	XtFree(txt);
	if  (newval <= arrow_max)  {
		sprintf(newtxt, "%*u", arrow_lng, newval);
		XmTextSetString(w, newtxt);
	}
	arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) arrow_incr, (XtPointer) w);
}

/* Ditto for decrement.  */

void  arrow_decr(Widget w, XtIntervalId *id)
{
	unsigned	newval;
	char	*txt, newtxt[20];

	XtVaGetValues(w, XmNvalue, &txt, NULL);
	newval = atoi(txt);
	XtFree(txt);
	if  (newval > arrow_min)  {
		newval--;
		sprintf(newtxt, "%*u", arrow_lng, newval);
		XmTextSetString(w, newtxt);
	}
	arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) arrow_decr, (XtPointer) w);
}

/* Increment priorities */

static void  priup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)  {
		arrow_max = mypriv->spu_flgs & PV_ANYPRIO? 255: mypriv->spu_maxp;
		arrow_lng = 3;
		arrow_incr(workw[subj], NULL);
	}
	else
		XtRemoveTimeOut(arrow_timer);
}

/* Decrement priorities */

static void  pridn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)  {
		arrow_min = mypriv->spu_flgs & PV_ANYPRIO? 1: mypriv->spu_minp;
		arrow_lng = 3;
		arrow_decr(workw[subj], NULL);
	}
	else
		XtRemoveTimeOut(arrow_timer);
}

/* Increment copies */

static void  cpsup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)  {
		arrow_max = mypriv->spu_flgs & PV_ANYPRIO? 255: mypriv->spu_cps;
		arrow_lng = 3;
		arrow_incr(workw[subj], NULL);
	}
	else
		XtRemoveTimeOut(arrow_timer);
}

/* Decrement copies */

static void  cpsdn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)  {
		arrow_min = 0;
		arrow_lng = 3;
		arrow_decr(workw[subj], NULL);
	}
	else
		XtRemoveTimeOut(arrow_timer);
}

#endif /* ! HAVE_XM_SPINB_H */

/* Increment page count, displaying "end" at last page */

static void  pg_incr(Widget w, XtIntervalId *id)
{
	LONG	newval;
	char	*txt, newtxt[20];

	XtVaGetValues(w, XmNvalue, &txt, NULL);
	if  (strcmp(txt, emsg) == 0)  {
		XtFree(txt);
		return;
	}
	newval = atol(txt);	/* Already displayed as 1 more */
	XtFree(txt);
	if  (newval > 1  &&  newval >= JREQ.spq_npages)
		strcpy(newtxt, emsg);
	else
		sprintf(newtxt, "%10ld", (long) (newval+1));
	XmTextSetString(w, newtxt);
	arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) pg_incr, (XtPointer) w);
}

/* Decrement page count.  */

static void  pg_decr(Widget w, XtIntervalId *id)
{
	LONG	newval;
	char	*txt, newtxt[20];

	XtVaGetValues(w, XmNvalue, &txt, NULL);
	if  (strcmp(txt, emsg) == 0)
		newval = JREQ.spq_npages - 1;
	else
		newval = atol(txt) - 2;
	XtFree(txt);
	if  (newval >= 0)  {
		sprintf(newtxt, "%10ld", (long) (newval+1));
		XmTextSetString(w, newtxt);
	}
	arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) pg_decr, (XtPointer) w);
}

/* Callout routines for up and down pages.  */

static void  pgup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)
		pg_decr(workw[subj], NULL);
	else
		XtRemoveTimeOut(arrow_timer);
}

static void  pgdn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)
		pg_incr(workw[subj], NULL);
	else
		XtRemoveTimeOut(arrow_timer);
}

#ifndef HAVE_XM_SPINB_H

/* Callbacks for "delete if printed/not printed" times */

static void  pup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)  {
		arrow_max = 0x7FFF;
		arrow_lng = 5;
		arrow_incr(workw[subj], NULL);
	}
	else
		XtRemoveTimeOut(arrow_timer);
}

static void  pdn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)  {
		arrow_min = 1;
		arrow_lng = 5;
		arrow_decr(workw[subj], NULL);
	}
	else
		XtRemoveTimeOut(arrow_timer);
}
#endif /* ! HAVE_XM_SPINB_H */

static void  fillintimes(int on)
{
	if  (on)  {
		time_t	ht = JREQ.spq_hold;
		struct	tm  *tp = localtime(&ht);
#ifdef HAVE_XM_SPINB_H
		XtVaSetValues(workw[WORKW_HOURTW], XmNposition, tp->tm_hour, (XtPointer) 0);
		XtVaSetValues(workw[WORKW_MINTW], XmNposition, tp->tm_min, (XtPointer) 0);
		XtVaSetValues(workw[WORKW_DOWTW], XmNposition, tp->tm_wday, (XtPointer) 0);
		XtVaSetValues(workw[WORKW_MONTW], XmNposition, tp->tm_mon, (XtPointer) 0);
#ifdef	BROKEN_SPINBOX
		XtVaSetValues(workw[WORKW_DOMTW], XmNposition, tp->tm_mday-1, (XtPointer) 0);
		XtVaSetValues(workw[WORKW_YEARTW], XmNposition, tp->tm_year, (XtPointer) 0);
#else
		XtVaSetValues(workw[WORKW_DOMTW], XmNposition, tp->tm_mday, (XtPointer) 0);
		XtVaSetValues(workw[WORKW_YEARTW], XmNposition, tp->tm_year+1900, (XtPointer) 0);
#endif
#else /* ! HAVE_XM_SPINB_H */
		char	nbuf[6];
		sprintf(nbuf, "%.2d", tp->tm_hour);
		XmTextSetString(workw[WORKW_HOURTW], nbuf);
		sprintf(nbuf, "%.2d", tp->tm_min);
		XmTextSetString(workw[WORKW_MINTW], nbuf);
		XmTextSetString(workw[WORKW_DOWTW], daynames[tp->tm_wday]);
		sprintf(nbuf, "%.2d", tp->tm_mday);
		XmTextSetString(workw[WORKW_DOMTW], nbuf);
		XmTextSetString(workw[WORKW_MONTW], monnames[tp->tm_mon]);
		sprintf(nbuf, "%d", tp->tm_year+1900);
		XmTextSetString(workw[WORKW_YEARTW], nbuf);
#endif /* HAVE_XM_SPINB_H */
		XtManageChild(holdspin);
	}
	else
		XtUnmanageChild(holdspin);
}

#ifdef HAVE_XM_SPINB_H
static void  holdt_cb(Widget w, XtPointer wh, XmSpinBoxCallbackStruct *cbs)
{
	time_t	newtime = JREQ.spq_hold;
	Widget	whichw = cbs->widget;
	int	incr, pos;
	static	unsigned  char	month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	switch  (cbs->reason)  {
	default:
		return;
	case  XmCR_SPIN_NEXT:
		incr = 1;
		break;
	case  XmCR_SPIN_PRIOR:
		incr = -1;
		break;
	case  XmCR_SPIN_FIRST:
		incr = cbs->crossed_boundary? 1: -1;
		break;
	case  XmCR_SPIN_LAST:
		incr = cbs->crossed_boundary? -1: 1;
		break;
	}

	if  (whichw == workw[WORKW_MINTW])
		newtime += 60L * incr;
	else  if  (whichw == workw[WORKW_HOURTW])
		newtime += 3600L * incr;
	else  if  (whichw == workw[WORKW_DOWTW] || whichw == workw[WORKW_DOMTW])
		newtime += SECSPERDAY * incr;
	else  if  (whichw == workw[WORKW_MONTW])  {
		struct	tm  *tp = localtime(&newtime);
		if  (incr > 0)  {
			newtime += month_days[tp->tm_mon] * SECSPERDAY;
			if  (tp->tm_year % 4 == 0 && tp->tm_mon == 1)
				newtime += SECSPERDAY;
		}
		else  {
			newtime -= month_days[(tp->tm_mon + 11) % 12] * SECSPERDAY;
			if  (tp->tm_year % 4 == 0 && tp->tm_mon == 2)
				newtime -= SECSPERDAY;
		}
	}
	else  {			/* Must be year */
		struct	tm  *tp = localtime(&newtime);
		if  (incr > 0)  {
			newtime += 365 * SECSPERDAY;
			if  ((tp->tm_year % 4 == 0  &&  tp->tm_mon <= 1)  ||  (tp->tm_year % 4 == 3  &&  tp->tm_mon > 1))
				newtime += SECSPERDAY;
		}
		else  {
			newtime -= 365 * SECSPERDAY;
			if  ((tp->tm_year % 4 == 1  &&  tp->tm_mon <= 1)  ||  (tp->tm_year % 4 == 0  &&  tp->tm_mon > 1))
				newtime -= SECSPERDAY;
		}
	}

	if  (newtime >= time((time_t *) 0))  {
		JREQ.spq_hold = (LONG) newtime;
		fillintimes(1);
	}
	XtVaGetValues(whichw, XmNposition, &pos, (XtPointer) 0);
	cbs->position = pos;
}

#else /* ! HAVE_XM_SPINB_H */

/* Timeout routine for bits of dates.  The "amount" is the widget
   number of the relevant bit if we are incrementing the time,
   otherwise - the widget number */

static void  time_adj(int amount, XtIntervalId *id)
{
	static	unsigned  char	month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	time_t	newtime = JREQ.spq_hold;
	struct	tm	*tp;

	switch  (amount)  {
	default:
	case  WORKW_MINTW:
		newtime += 60L;
		break;
	case  -WORKW_MINTW:
		newtime -= 60L;
		break;
	case  WORKW_HOURTW:
		newtime += 60*60L;
		break;
	case  -WORKW_HOURTW:
		newtime -= 60*60L;
		break;
	case  WORKW_DOWTW:
	case  WORKW_DOMTW:
		newtime += SECSPERDAY;
		break;
	case  -WORKW_DOWTW:
	case  -WORKW_DOMTW:
		newtime -= SECSPERDAY;
		break;
	case  WORKW_MONTW:
		tp = localtime(&newtime);
		newtime += month_days[tp->tm_mon] * SECSPERDAY;
		if  (tp->tm_year % 4 == 0 && tp->tm_mon == 1)
			newtime += SECSPERDAY;
		break;
	case  -WORKW_MONTW:
		tp = localtime(&newtime);
		newtime -= month_days[(tp->tm_mon + 11) % 12] * SECSPERDAY;
		if  (tp->tm_year % 4 == 0 && tp->tm_mon == 2)
			newtime -= SECSPERDAY;
		break;
	case  WORKW_YEARTW:
		tp = localtime(&newtime);
		newtime += 365 * SECSPERDAY;
		if  ((tp->tm_year % 4 == 0  &&  tp->tm_mon <= 1)  ||
		     (tp->tm_year % 4 == 3  &&  tp->tm_mon > 1))
			newtime += SECSPERDAY;
		break;
	case  -WORKW_YEARTW:
		tp = localtime(&newtime);
		newtime -= 365 * SECSPERDAY;
		if  ((tp->tm_year % 4 == 1  &&  tp->tm_mon <= 1)  ||
		     (tp->tm_year % 4 == 0  &&  tp->tm_mon > 1))
			newtime -= SECSPERDAY;
		break;
	}

	if  (newtime >= time((time_t *) 0))  {
		JREQ.spq_hold = (LONG) newtime;
		fillintimes(1);
	}
	arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) time_adj, (XtPointer) amount);
}

/* Callback for arrows on bits of dates.  "Amount" is as described above.  */

static void  time_cb(Widget w, int amount, XmArrowButtonCallbackStruct *cbs)
{
	/* If we aren't doing a hold time, ignore it.  */

	if  (!XmToggleButtonGadgetGetState(workw[WORKW_HOLDBW]))
	     return;

	if  (cbs->reason == XmCR_ARM)
		time_adj(amount, NULL);
	else
		XtRemoveTimeOut(arrow_timer);
}
#endif

/* Create pair of up/down arrows with appropriate callbacks.  */

Widget CreateArrowPair(char *name, Widget formw, Widget	topw, Widget leftw, XtCallbackProc upcall, XtCallbackProc dncall, int updata, int dndata, int sensitive)
{
	Widget	uparrow, dnarrow;
	char	fullname[10];

	sprintf(fullname, "%sup", name);
	if  (topw)  {
		uparrow = XtVaCreateManagedWidget(fullname,
						  xmArrowButtonWidgetClass,	formw,
						  XmNtopAttachment,		XmATTACH_WIDGET,
						  XmNtopWidget,			topw,
						  XmNleftAttachment,		XmATTACH_WIDGET,
						  XmNleftWidget,		leftw,
						  XmNarrowDirection,		XmARROW_UP,
						  XmNborderWidth,		0,
						  XmNrightOffset,		0,
						  NULL);

		sprintf(fullname, "%sdn", name);

		dnarrow = XtVaCreateManagedWidget(fullname,
						  xmArrowButtonWidgetClass,	formw,
						  XmNtopAttachment,		XmATTACH_WIDGET,
						  XmNtopWidget,			topw,
						  XmNleftAttachment,		XmATTACH_WIDGET,
						  XmNleftWidget,		uparrow,
						  XmNarrowDirection,		XmARROW_DOWN,
						  XmNborderWidth,		0,
						  XmNleftOffset,		0,
						  NULL);
	}
	else  {
		uparrow = XtVaCreateManagedWidget(fullname,
						  xmArrowButtonWidgetClass,	formw,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNleftAttachment,		XmATTACH_WIDGET,
						  XmNleftWidget,		leftw,
						  XmNarrowDirection,		XmARROW_UP,
						  XmNborderWidth,		0,
						  XmNrightOffset,		0,
						  NULL);

		sprintf(fullname, "%sdn", name);

		dnarrow = XtVaCreateManagedWidget(fullname,
						  xmArrowButtonWidgetClass,	formw,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNleftAttachment,		XmATTACH_WIDGET,
						  XmNleftWidget,		uparrow,
						  XmNarrowDirection,		XmARROW_DOWN,
						  XmNborderWidth,		0,
						  XmNleftOffset,		0,
						  NULL);
	}

	if  (!sensitive)  {
		XtSetSensitive(uparrow, False);
		XtSetSensitive(dnarrow, False);
	}
	else  {
		XtAddCallback(uparrow, XmNarmCallback, (XtCallbackProc) upcall, INT_TO_XTPOINTER(updata));
		XtAddCallback(dnarrow, XmNarmCallback, (XtCallbackProc) dncall, INT_TO_XTPOINTER(dndata));
		XtAddCallback(uparrow, XmNdisarmCallback, (XtCallbackProc) upcall, (XtPointer) 0);
		XtAddCallback(dnarrow, XmNdisarmCallback, (XtCallbackProc) dncall, (XtPointer) 0);
	}
	return  dnarrow;
}

/* Get the job the user is pointing at.  Winge if no such job, or user can't get at it.  */

const struct spq *getselectedjob(const ULONG priv)
{
	int	*plist, pcnt;

	if  (XmListGetSelectedPos(jwid, &plist, &pcnt) && pcnt > 0)  {
		const  Hashspq  *result = Job_seg.jj_ptrs[plist[0] - 1];
		const  struct  spq  *rj = &result->j;
		XtFree((XtPointer) plist);
		if  (!(mypriv->spu_flgs & priv)  &&  strcmp(Realuname, rj->spq_uname) != 0)  {
			disp_str = rj->spq_file;
			disp_str2 = rj->spq_uname;
			doerror(jwid, priv == PV_VOTHERJ? $EH{xmspq job not readable}: $EH{spq job not yours});
			return  NULL;
		}
		if  (rj->spq_netid  &&  !(mypriv->spu_flgs & PV_REMOTEJ))  {
			doerror(jwid, $EH{spq no remote job priv});
			return  NULL;
		}
		JREQ = *rj;
		JREQS = result - Job_seg.jlist;
		return  rj;
	}
	doerror(jwid, Job_seg.njobs != 0? $EH{No job selected}: $EH{No jobs to select});
	return  NULL;
}

Widget  CreateJtitle(Widget formw)
{
	Widget		titw1, titw2;
	int	lng;
	char	nbuf[HOSTNSIZE+20];
	titw1 =	XtVaCreateManagedWidget("jobnotitle",
					xmLabelWidgetClass,	formw,
					XmNtopAttachment,	XmATTACH_FORM,
					XmNleftAttachment,	XmATTACH_FORM,
					XmNborderWidth,		0,
					NULL);
	if  (JREQ.spq_netid)
		sprintf(nbuf, "%s:%ld", look_host(JREQ.spq_netid), (long) JREQ.spq_job);
	else
		sprintf(nbuf, "%ld", (long) JREQ.spq_job);
	lng = strlen(nbuf);

	titw2 = XtVaCreateManagedWidget("jobno",
					xmTextFieldWidgetClass,	formw,
					XmNcolumns,		lng,
					XmNcursorPositionVisible,	False,
					XmNeditable,		False,
					XmNtopAttachment,	XmATTACH_FORM,
					XmNleftAttachment,	XmATTACH_WIDGET,
					XmNleftWidget,		titw1,
					NULL);
	XmTextSetString(titw2, nbuf);
	return  titw2;
}

/* Initial part of job dialog.  Return widget of tallest part of title.  */

static Widget CreateJeditDlg(Widget parent, char *dlgname, Widget *dlgres, Widget *paneres, Widget *formres)
{
	CreateEditDlg(parent, dlgname, dlgres, paneres, formres, 3);
	return  CreateJtitle(*formres);
}

/* Callback for one more copy.  */

void  cb_onemore()
{
	const  struct	spq	*cj = getselectedjob(PV_OTHERJ);
	int	num, maxnum = 255;
	if  (!cj)
		return;
	num = cj->spq_cps + 1;
	if  (!(mypriv->spu_flgs & PV_ANYPRIO))
		maxnum = mypriv->spu_cps;
	if  (num > maxnum)  {
		disp_arg[1] = maxnum;
		doerror(jwid, $EH{Too many copies});
		return;
	}
	JREQ.spq_cps = (unsigned char) num;
	my_wjmsg(SJ_CHNG);
}

/* Job actions, currently only SO_AB */

void  cb_jact(Widget wid, int msg)
{
	const  struct	spq	*jp = getselectedjob(PV_OTHERJ);
	if  (!jp)
		return;
	OREQ = JREQS;
	if  (confabort > 1 || (confabort > 0  &&  !(jp->spq_dflags & SPQ_PRINTED)))  {
		if  (!Confirm(jwid,
			      jp->spq_dflags & SPQ_PRINTED? $PH{Sure about deleting printed job}:
			      $PH{Sure about deleting not printed}))
			return;
	}
	womsg(msg);
}

/* Callback for end of job forms dialog */

static void  endjform(Widget w, int data)
{
	if  (data)  {
		char		*txt;
		int		nres;
#if  	defined(HAVE_XM_SPINB_H)  &&  defined(BROKEN_SPINBOX)
		int	mini;
#endif
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
		XmString	ptrtxt;
		XtVaGetValues(workw[WORKW_FTXTW], XmNselectedItem, &ptrtxt, NULL);
		XmStringGetLtoR(ptrtxt, XmSTRING_DEFAULT_CHARSET, &txt);
		XmStringFree(ptrtxt);
#else
		XtVaGetValues(workw[WORKW_FTXTW], XmNvalue, &txt, NULL);
#endif
		if  (!txt[0])  {
			XtFree(txt);
			return;
		}
		if  (!((mypriv->spu_flgs & PV_FORMS) || qmatch(mypriv->spu_formallow, txt)))  {
			disp_str = txt;
			disp_str2 = mypriv->spu_formallow;
			doerror(jwid, $EH{form type not valid});
			XtFree(txt);
			return;
		}
		strncpy(JREQ.spq_form, txt, MAXFORM);
		XtFree(txt);
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
		XtVaGetValues(workw[WORKW_PTXTW], XmNselectedItem, &ptrtxt, NULL);
		XmStringGetLtoR(ptrtxt, XmSTRING_DEFAULT_CHARSET, &txt);
		XmStringFree(ptrtxt);
		if  (txt[0] == '-')
			txt[0] = '\0';
#else
		XtVaGetValues(workw[WORKW_PTXTW], XmNvalue, &txt, NULL);
#endif
		if  (!((mypriv->spu_flgs & PV_OTHERP)  ||  issubset(mypriv->spu_ptrallow, txt)))  {
			disp_str = txt;
			disp_str2 = mypriv->spu_ptrallow;
			doerror(jwid, $EH{ptr type not valid});
			XtFree(txt);
			return;
		}
		strncpy(JREQ.spq_ptr, txt, JPTRNAMESIZE);
		XtFree(txt);
		XtVaGetValues(workw[WORKW_HTXTW], XmNvalue, &txt, NULL);
		strncpy(JREQ.spq_file, txt, MAXTITLE);
		XtFree(txt);
		if  (XmToggleButtonGadgetGetState(workw[WORKW_SUPPHBW]))
			JREQ.spq_jflags |= SPQ_NOH;
		else
			JREQ.spq_jflags &= ~SPQ_NOH;
#ifdef HAVE_XM_SPINB_H
#ifdef BROKEN_SPINBOX
		XtVaGetValues(workw[WORKW_PRITXTW], XmNposition, &nres, XmNminimumValue, &mini, NULL);
		JREQ.spq_pri = nres+mini;
		XtVaGetValues(workw[WORKW_CPSTXTW], XmNposition, &nres, XmNminimumValue, &mini, NULL);
		JREQ.spq_cps = nres+mini;
#else
		XtVaGetValues(workw[WORKW_PRITXTW], XmNposition, &nres, NULL);
		JREQ.spq_pri = nres;
		XtVaGetValues(workw[WORKW_CPSTXTW], XmNposition, &nres, NULL);
		JREQ.spq_cps = nres;
#endif
#else
		XtVaGetValues(workw[WORKW_PRITXTW], XmNvalue, &txt, NULL);
		JREQ.spq_pri = atoi(txt);
		XtFree(txt);
		XtVaGetValues(workw[WORKW_CPSTXTW], XmNvalue, &txt, NULL);
		JREQ.spq_cps = atoi(txt);
		XtFree(txt);
#endif
		my_wjmsg(SJ_CHNG);
	}
	XtDestroyWidget(GetTopShell(w));
}

/* Create job form dialog */

void  cb_jform(Widget parent)
{
	int	maxp = 255, minp = 1, maxcps = 255, curp, curc;
	Widget	jf_shell, panew, mainform, prevabove, htitw, prititw, ctitw;
	const  struct	spq	*jp = getselectedjob(PV_OTHERJ);
#ifdef HAVE_XM_SPINB_H
	Widget	spinb;
#else
	char	nbuf[6];
#endif
#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
	char	**posslist;
#endif

	if  (!jp)
		return;

	curp = jp->spq_pri;
	curc = jp->spq_cps;
	if  (!(mypriv->spu_flgs & PV_ANYPRIO))  {
		maxp = mypriv->spu_maxp;
		minp = mypriv->spu_minp;
		if  (curp < minp)
			curp = minp;
		if  (curp > maxp)
			curp = maxp;
		maxcps = mypriv->spu_cps;
		if  (curc > maxcps)
			curc = maxcps;
	}

#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
	prevabove = CreateJeditDlg(parent, "Jform", &jf_shell, &panew, &mainform);
	posslist = wotjform();
	prevabove = CreateFselDialog(mainform, prevabove, (char *) jp->spq_form, posslist);
	freehelp(posslist);
#else
	prevabove = CreateJeditDlg(parent, "Jform", &jf_shell, &panew, &mainform);
	prevabove = CreateFselDialog(mainform, prevabove, (char *) jp->spq_form, (XtCallbackProc) getformsel, 0);
#endif

	htitw = XtVaCreateManagedWidget("Header",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	workw[WORKW_HTXTW] = XtVaCreateManagedWidget("hdr",
						     xmTextFieldWidgetClass,	mainform,
						     XmNcolumns,		MAXTITLE,
						     XmNmaxWidth,		MAXTITLE,
						     XmNcursorPositionVisible,	False,
						     XmNtopAttachment,		XmATTACH_WIDGET,
						     XmNtopWidget,		prevabove,
						     XmNleftAttachment,		XmATTACH_WIDGET,
						     XmNleftWidget,		htitw,
						     NULL);

	XmTextSetString(workw[WORKW_HTXTW], (char *) jp->spq_file);

	workw[WORKW_SUPPHBW] = XtVaCreateManagedWidget("supph",
						       xmToggleButtonGadgetClass,	mainform,
						       XmNtopAttachment,		XmATTACH_WIDGET,
						       XmNtopWidget,			prevabove,
						       XmNleftAttachment,		XmATTACH_WIDGET,
						       XmNleftWidget,			workw[WORKW_HTXTW],
						       NULL);
	if  (jp->spq_jflags & SPQ_NOH)
		XmToggleButtonGadgetSetState(workw[WORKW_SUPPHBW], True, False);
	prevabove = workw[WORKW_HTXTW];

#if  	defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
	posslist = wotjprin();
	prevabove = CreatePselDialog(mainform, workw[WORKW_HTXTW], (char *) jp->spq_ptr, posslist, 1, JPTRNAMESIZE);
	freehelp(posslist);
#else
	prevabove = CreatePselDialog(mainform, workw[WORKW_HTXTW], (char *) jp->spq_ptr, (XtCallbackProc) getptrsel, 1);
#endif

	prititw = XtVaCreateManagedWidget("Priority",
				xmLabelGadgetClass,	mainform,
				XmNtopAttachment,	XmATTACH_WIDGET,
				XmNtopWidget,		prevabove,
				XmNleftAttachment,	XmATTACH_FORM,
				NULL);

#ifdef HAVE_XM_SPINB_H

	spinb = XtVaCreateManagedWidget("prisp",
					xmSpinBoxWidgetClass,		mainform,
					XmNarrowSensitivity,		mypriv->spu_flgs & PV_CPRIO? XmARROWS_SENSITIVE: XmARROWS_INSENSITIVE,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			prititw,
					NULL);

	workw[WORKW_PRITXTW] = XtVaCreateManagedWidget("pri",
						       xmTextFieldWidgetClass,		spinb,
						       XmNmaximumValue,			maxp,
						       XmNminimumValue,			minp,
						       XmNspinBoxChildType,		XmNUMERIC,
						       XmNcolumns,			3,
						       XmNeditable,			False,
						       XmNcursorPositionVisible,	False,
#ifdef	BROKEN_SPINBOX
						       XmNpositionType,			XmPOSITION_INDEX,
						       XmNposition,			curp - minp,
#else
						       XmNposition,			curp,
#endif
						       NULL);
	prevabove = spinb;

#else  /* ! HAVE_XM_SPINB_H */

	workw[WORKW_PRITXTW] = XtVaCreateManagedWidget("pri",
						       xmTextFieldWidgetClass,		mainform,
						       XmNcolumns,			3,
						       XmNmaxWidth,			3,
						       XmNcursorPositionVisible,	False,
						       XmNtopAttachment,		XmATTACH_WIDGET,
						       XmNtopWidget,			prevabove,
						       XmNleftAttachment,		XmATTACH_WIDGET,
						       XmNleftWidget,			prititw,
						       NULL);

	sprintf(nbuf, "%3d", jp->spq_pri);
	XmTextSetString(workw[WORKW_PRITXTW], nbuf);

	CreateArrowPair("pri",				mainform,
			prevabove,			workw[WORKW_PRITXTW],
			(XtCallbackProc) priup_cb,	(XtCallbackProc) pridn_cb,
			WORKW_PRITXTW,			WORKW_PRITXTW,
			mypriv->spu_flgs & PV_CPRIO);

	if  (!(mypriv->spu_flgs & PV_CPRIO))
		XtSetSensitive(workw[WORKW_PRITXTW], False);

	prevabove = workw[WORKW_PRITXTW];
#endif

	ctitw = XtVaCreateManagedWidget("Copies",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

#ifdef HAVE_XM_SPINB_H

	spinb = XtVaCreateManagedWidget("cpssp",
					xmSpinBoxWidgetClass,		mainform,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			ctitw,
					NULL);

	workw[WORKW_CPSTXTW] = XtVaCreateManagedWidget("cps",
						       xmTextFieldWidgetClass,		spinb,
						       XmNspinBoxChildType,		XmNUMERIC,
						       XmNcolumns,			3,
						       XmNeditable,			False,
						       XmNcursorPositionVisible,	False,
						       XmNmaximumValue,			maxcps,
						       XmNminimumValue,			0,
						       XmNposition,			curc,
						       NULL);
#else

	workw[WORKW_CPSTXTW] = XtVaCreateManagedWidget("cps",
						       xmTextFieldWidgetClass,		mainform,
						       XmNcolumns,			3,
						       XmNmaxWidth,			3,
						       XmNcursorPositionVisible,	False,
						       XmNtopAttachment,		XmATTACH_WIDGET,
						       XmNtopWidget,			prevabove,
						       XmNleftAttachment,		XmATTACH_WIDGET,
						       XmNleftWidget,			ctitw,
						       NULL);
	sprintf(nbuf, "%3d", jp->spq_cps);
	XmTextSetString(workw[WORKW_CPSTXTW], nbuf);
	CreateArrowPair("cps",				mainform,
			prevabove,			workw[WORKW_CPSTXTW],
			(XtCallbackProc) cpsup_cb,	(XtCallbackProc) cpsdn_cb,
			WORKW_CPSTXTW,			WORKW_CPSTXTW,		1);
#endif /* !HAVE_XM_SPINB_H */

	XtManageChild(mainform);
	CreateActionEndDlg(jf_shell, panew, (XtCallbackProc) endjform, $H{xmspq form type menu});
}

static void  endjpage(Widget w, int data)
{
	if  (data)  {
		char	*txt;
		LONG	spage, epage, hpage;
		XtVaGetValues(workw[WORKW_SPTXTW], XmNvalue, &txt, NULL);
		if  (strcmp(txt, emsg) == 0)
			spage = MAXLONG - 1L;
		else
			spage = atol(txt) - 1L;
		if  (spage < 0)
			spage = 0;
		XtFree(txt);
		XtVaGetValues(workw[WORKW_EPTXTW], XmNvalue, &txt, NULL);
		if  (strcmp(txt, emsg) == 0)
			epage = MAXLONG - 1L;
		else
			epage = atol(txt) - 1L;
		if  (epage < 0)
			epage = 0;
		XtFree(txt);
		if  (spage > epage)  {
			doerror(jwid, $EH{end page less than start page});
			return;
		}
		if  (JREQ.spq_haltat != 0)  {
			XtVaGetValues(workw[WORKW_HAPTXTW], XmNvalue, &txt, NULL);
			if  (strcmp(txt, emsg) == 0)
				hpage = MAXLONG - 1L;
			else
				hpage = atol(txt) - 1L;
			if  (hpage < 0)
				hpage = 0;
			XtFree(txt);
			if  (hpage > epage)  {
				doerror(jwid, $EH{end page less than haltat page});
				return;
			}
			JREQ.spq_haltat = hpage;
		}
		JREQ.spq_start = spage;
		JREQ.spq_end = epage;
		XtVaGetValues(workw[WORKW_FLAGSPTXTW], XmNvalue, &txt, NULL);
		strncpy(JREQ.spq_flags, txt, MAXFLAGS);
		XtFree(txt);
		JREQ.spq_jflags &= ~(SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);
		if  (XmToggleButtonGadgetGetState(workw[WORKW_ODDPBTW]))
			JREQ.spq_jflags |= SPQ_EVENP;
		if  (XmToggleButtonGadgetGetState(workw[WORKW_EVENPBTW]))
			JREQ.spq_jflags |= SPQ_ODDP;
		if  (XmToggleButtonGadgetGetState(workw[WORKW_SWOEPBTW]))
			JREQ.spq_jflags |= SPQ_REVOE;
		my_wjmsg(SJ_CHNG);
	}
	XtDestroyWidget(GetTopShell(w));
}

void  cb_jpages(Widget parent)
{
	Widget	jp_shell, panew, mainform, prevabove,
		stitw,	etitw,	htitw,
		pselmainrc, pselradrc,
		ftitw;
	int		cnt;
	const  struct	spq	*jp = getselectedjob(PV_OTHERJ);
	char	nbuf[20];

	if  (!jp)
		return;

	prevabove = CreateJeditDlg(parent, "Jpage", &jp_shell, &panew, &mainform);

	if  (!emsg)
		emsg = gprompt($P{Page eodoc});

	stitw = XtVaCreateManagedWidget("Spage",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	workw[WORKW_SPTXTW] = XtVaCreateManagedWidget("spage",
						      xmTextFieldWidgetClass,	mainform,
						      XmNcolumns,		10,
						      XmNmaxWidth,		10,
						      XmNcursorPositionVisible,	False,
						      XmNtopAttachment,		XmATTACH_WIDGET,
						      XmNtopWidget,		prevabove,
						      XmNleftAttachment,	XmATTACH_WIDGET,
						      XmNleftWidget,		stitw,
						      NULL);

	sprintf(nbuf, "%10ld", jp->spq_start + 1L);
	XmTextSetString(workw[WORKW_SPTXTW], nbuf);
	CreateArrowPair("s",				mainform,
			prevabove,			workw[WORKW_SPTXTW],
			(XtCallbackProc) pgup_cb,	(XtCallbackProc) pgdn_cb,
			WORKW_SPTXTW,			WORKW_SPTXTW,		1);

	prevabove = workw[WORKW_SPTXTW];

	etitw = XtVaCreateManagedWidget("Epage",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	workw[WORKW_EPTXTW] = XtVaCreateManagedWidget("epage",
						      xmTextFieldWidgetClass,	mainform,
						      XmNcolumns,		10,
						      XmNmaxWidth,		10,
						      XmNcursorPositionVisible,	False,
						      XmNtopAttachment,		XmATTACH_WIDGET,
						      XmNtopWidget,		prevabove,
						      XmNleftAttachment,	XmATTACH_WIDGET,
						      XmNleftWidget,		etitw,
						      NULL);

	CreateArrowPair("e",				mainform,
			prevabove,			workw[WORKW_EPTXTW],
			(XtCallbackProc) pgup_cb,	(XtCallbackProc) pgdn_cb,
			WORKW_EPTXTW,			WORKW_EPTXTW,	1);

	if  (jp->spq_end > 0  &&  jp->spq_end + 1L >= jp->spq_npages)
		strcpy(nbuf, emsg);
	else
		sprintf(nbuf, "%10ld", jp->spq_end + 1L);
	XmTextSetString(workw[WORKW_EPTXTW], nbuf);
	prevabove = workw[WORKW_EPTXTW];

	if  (jp->spq_haltat != 0)  {
		htitw = XtVaCreateManagedWidget("Hpage",
						xmLabelGadgetClass,	mainform,
						XmNtopAttachment,	XmATTACH_WIDGET,
						XmNtopWidget,		prevabove,
						XmNleftAttachment,	XmATTACH_FORM,
						NULL);

		workw[WORKW_HAPTXTW] = XtVaCreateManagedWidget("hpage",
							       xmTextFieldWidgetClass,	mainform,
							       XmNcolumns,		10,
							       XmNmaxWidth,		10,
							       XmNcursorPositionVisible,	False,
							       XmNtopAttachment,	XmATTACH_WIDGET,
							       XmNtopWidget,		prevabove,
							       XmNleftAttachment,	XmATTACH_WIDGET,
							       XmNleftWidget,		htitw,
							       NULL);

		CreateArrowPair("h",				mainform,
				prevabove,			workw[WORKW_HAPTXTW],
				(XtCallbackProc) pgup_cb,	(XtCallbackProc) pgdn_cb,
				WORKW_HAPTXTW,			WORKW_HAPTXTW,		1);

		sprintf(nbuf, "%10ld", jp->spq_haltat + 1L);
		XmTextSetString(workw[WORKW_HAPTXTW], nbuf);
		prevabove = workw[WORKW_HAPTXTW];
	}

	/* Row column widget to carry lh pane with radio buttons and rh pane with just swap toggle */

	pselmainrc = XtVaCreateManagedWidget("oeselect",
					     xmRowColumnWidgetClass,	mainform,
					     XmNtopAttachment,		XmATTACH_WIDGET,
					     XmNtopWidget,		prevabove,
					     XmNleftAttachment,		XmATTACH_FORM,
					     XmNrightAttachment,	XmATTACH_FORM,
					     XmNpacking,		XmPACK_COLUMN,
					     XmNnumColumns,		2,
					     NULL);

	pselradrc = XtVaCreateManagedWidget("aoe",
					    xmRowColumnWidgetClass,	pselmainrc,
					    XmNpacking,			XmPACK_COLUMN,
					    XmNnumColumns,		1,
					    XmNisHomogeneous,		True,
					    XmNentryClass,		xmToggleButtonGadgetClass,
					    XmNradioBehavior,		True,
					    XmNborderWidth,		0,
					    NULL);

	workw[WORKW_ALLPBTW] = XtVaCreateManagedWidget("allpages",
						       xmToggleButtonGadgetClass, pselradrc,
						       XmNborderWidth,		0,
						       NULL);

	workw[WORKW_ODDPBTW] = XtVaCreateManagedWidget("oddpages",
						       xmToggleButtonGadgetClass, pselradrc,
						       XmNborderWidth,		0,
						       NULL);

	workw[WORKW_EVENPBTW] = XtVaCreateManagedWidget("evenpages",
							xmToggleButtonGadgetClass, pselradrc,
							XmNborderWidth,		0,
							NULL);

	cnt = jp->spq_jflags & SPQ_ODDP? WORKW_EVENPBTW:
		jp->spq_jflags & SPQ_EVENP? WORKW_ODDPBTW:
		WORKW_ALLPBTW;
	XmToggleButtonGadgetSetState(workw[cnt], True, False);

	workw[WORKW_SWOEPBTW] = XtVaCreateManagedWidget("swapoe",
							xmToggleButtonGadgetClass, pselmainrc,
							XmNborderWidth,		0,
							NULL);
	if  (jp->spq_jflags & SPQ_REVOE)
		XmToggleButtonGadgetSetState(workw[WORKW_SWOEPBTW], True, False);

	prevabove = pselmainrc;

	ftitw = XtVaCreateManagedWidget("Flags",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

	workw[WORKW_FLAGSPTXTW] = XtVaCreateManagedWidget("flags",
							  xmTextFieldWidgetClass,	mainform,
							  XmNcolumns,			MAXFLAGS,
							  XmNmaxWidth,			MAXFLAGS,
							  XmNcursorPositionVisible,	False,
							  XmNtopAttachment,		XmATTACH_WIDGET,
							  XmNtopWidget,			prevabove,
							  XmNleftAttachment,		XmATTACH_WIDGET,
							  XmNleftWidget,		ftitw,
							  NULL);

	XmTextSetString(workw[WORKW_FLAGSPTXTW], (char *) jp->spq_flags);
	XtManageChild(mainform);
	CreateActionEndDlg(jp_shell, panew, (XtCallbackProc) endjpage, $H{xmspq pages menu});
}

static	struct	{
	unsigned  char	widnum;
	USHORT	flag;
	char	*widname;
}  mwwids[] = {
	{	WORKW_MAILW,	SPQ_MAIL,	"mail"	},
	{	WORKW_WRITEW,	SPQ_WRT,	"write"	},
	{	WORKW_MATTNW,	SPQ_MATTN,	"mattn"	},
	{	WORKW_WATTNW,	SPQ_WATTN,	"wattn"	}};

static void  attachmwbutts(Widget form, Widget after)
{
	int	cnt;
	Widget	w;

	for  (cnt = 0;  cnt < XtNumber(mwwids);  cnt++)  {
		w = XtVaCreateManagedWidget(mwwids[cnt].widname,
					    xmToggleButtonGadgetClass, form,
					    XmNtopAttachment,		XmATTACH_WIDGET,
					    XmNtopWidget,		after,
					    XmNleftAttachment,		XmATTACH_FORM,
					    XmNborderWidth,		0,
					    NULL);
		workw[mwwids[cnt].widnum] = w;
		after = w;
		if  (JREQ.spq_jflags & mwwids[cnt].flag)
			XmToggleButtonGadgetSetState(w, True, False);
	}
}

static void  endjuser(Widget w, int data)
{
	if  (data)  {
		char	*txt;
		int	cnt;
		XtVaGetValues(workw[WORKW_UTXTW], XmNvalue, &txt, NULL);
		strncpy(JREQ.spq_puname, txt, UIDSIZE);
		XtFree(txt);
		JREQ.spq_jflags &= ~(SPQ_MAIL|SPQ_WRT|SPQ_MATTN|SPQ_WATTN);
		for  (cnt = 0;  cnt < XtNumber(mwwids);  cnt++)
			if  (XmToggleButtonGadgetGetState(workw[mwwids[cnt].widnum]))
				JREQ.spq_jflags |= mwwids[cnt].flag;
		my_wjmsg(SJ_CHNG);
	}
	XtDestroyWidget(GetTopShell(w));
}

void  cb_juser(Widget parent)
{
	Widget	ju_shell, panew, mainform, prevabove;
	const  struct	spq	*jp = getselectedjob(PV_OTHERJ);
	if  (!jp)
		return;
	prevabove = CreateJeditDlg(parent, "Juser", &ju_shell, &panew, &mainform);
	prevabove = CreateUselDialog(mainform, prevabove, (char *) jp->spq_puname, 0);
	attachmwbutts(mainform, prevabove);
	XtManageChild(mainform);
	CreateActionEndDlg(ju_shell, panew, (XtCallbackProc) endjuser, $H{xmspq user menu});
}

static void  holdt_set(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
	if  (cbs->set)  {
		JREQ.spq_hold = (LONG) time((time_t *) 0);
		fillintimes(1);
	}
	else  {
		JREQ.spq_hold = 0L;
		fillintimes(0);
	}
}

static void  endjret(Widget w, int data)
{
	if  (data)  {
#ifdef HAVE_XM_SPINB_H
		int	nres;
#ifdef	BROKEN_SPINBOX
		int	mini;
#endif
#else
		char	*txt;
#endif
		if  (XmToggleButtonGadgetGetState(workw[WORKW_RETQBW]))
			JREQ.spq_jflags |= SPQ_RETN;
		else
			JREQ.spq_jflags &= ~SPQ_RETN;
		if  (XmToggleButtonGadgetGetState(workw[WORKW_PRINTED]))
			JREQ.spq_dflags |= SPQ_PRINTED;
		else
			JREQ.spq_dflags &= ~SPQ_PRINTED;
#ifdef	HAVE_XM_SPINB_H
#ifdef	BROKEN_SPINBOX
		XtVaGetValues(workw[WORKW_PDELTXTW], XmNposition, &nres, XmNminimumValue, &mini, NULL);
		JREQ.spq_ptimeout = nres+mini;
		XtVaGetValues(workw[WORKW_NPDELTXTW], XmNposition, &nres, XmNminimumValue, &mini, NULL);
		JREQ.spq_nptimeout = nres+mini;
#else
		XtVaGetValues(workw[WORKW_PDELTXTW], XmNposition, &nres, NULL);
		JREQ.spq_ptimeout = nres;
		XtVaGetValues(workw[WORKW_NPDELTXTW], XmNposition, &nres, NULL);
		JREQ.spq_nptimeout = nres;
#endif
#else
		XtVaGetValues(workw[WORKW_PDELTXTW], XmNvalue, &txt, NULL);
		JREQ.spq_ptimeout = atoi(txt);
		if  (JREQ.spq_ptimeout == 0)
			JREQ.spq_ptimeout = 1;
		XtFree(txt);
		XtVaGetValues(workw[WORKW_NPDELTXTW], XmNvalue, &txt, NULL);
		JREQ.spq_nptimeout = atoi(txt);
		if  (JREQ.spq_nptimeout == 0)
			JREQ.spq_nptimeout = 1;
		XtFree(txt);
#endif /* ! HAVE_XM_SPINB_H */
		if  (JREQ.spq_nptimeout < JREQ.spq_ptimeout  &&  !Confirm(jwid, $PH{xmspq not ptd lt ptd}))
			return;
		my_wjmsg(SJ_CHNG);
	}
	XtDestroyWidget(GetTopShell(w));
}

void  cb_jretain(Widget parent)
{
	Widget		jr_shell, panew, mainform, prevabove, prevleft, ptitw, nptitw;
#ifdef HAVE_XM_SPINB_H
	Widget		spinb;
#endif
	int		cnt;
	const  struct	spq	*jp = getselectedjob(PV_OTHERJ);
	time_t		st;
	struct	tm	*tp;
	char		nbuf[50];

	if  (!jp)
		return;

	if  (longest_day == 0)  {
#ifdef HAVE_XM_SPINB_H
		if  (!(timezerof = (XmStringTable) XtMalloc((unsigned) (60 * sizeof(XmString)))))
			nomem();
		if  (!(stdaynames = (XmStringTable) XtMalloc((unsigned) (7 * sizeof(XmString)))))
			nomem();
		if  (!(stmonnames = (XmStringTable) XtMalloc((unsigned) (12 * sizeof(XmString)))))
			nomem();
		for  (cnt = 0;  cnt < 60;  cnt++)  {
			char	buf[3];
			sprintf(buf, "%.2d", cnt);
			timezerof[cnt] = XmStringCreateLocalized(buf);
		}
#endif
		for  (cnt = 0;  cnt < 7;  cnt++)  {
			int	lng;
			daynames[cnt] = gprompt(cnt+$P{Full Sunday});
			if  ((lng = strlen(daynames[cnt])) > longest_day)
				longest_day = lng;
#ifdef HAVE_XM_SPINB_H
			stdaynames[cnt] = XmStringCreateLocalized(daynames[cnt]);
#endif
		}
		for  (cnt = 0;  cnt < 12;  cnt++)  {
			int	lng;
			monnames[cnt] = gprompt(cnt+$P{Full January});
			if  ((lng = strlen(monnames[cnt])) > longest_mon)
				longest_mon = lng;
#ifdef HAVE_XM_SPINB_H
			stmonnames[cnt] = XmStringCreateLocalized(monnames[cnt]);
#endif
		}
	}

	prevabove = CreateJeditDlg(parent, "Jret", &jr_shell, &panew, &mainform);

	prevleft = XtVaCreateManagedWidget("createdon",
					   xmLabelGadgetClass,	mainform,
					   XmNtopAttachment,	XmATTACH_WIDGET,
					   XmNtopWidget,	prevabove,
					   XmNleftAttachment,	XmATTACH_FORM,
					   NULL);

	prevabove = XtVaCreateManagedWidget("ctime",
					    xmTextFieldWidgetClass,	mainform,
					    XmNcursorPositionVisible,	False,
					    XmNeditable,		False,
					    XmNtopAttachment,		XmATTACH_WIDGET,
					    XmNtopWidget,		prevabove,
					    XmNrightAttachment,		XmATTACH_FORM,
					    XmNleftAttachment,		XmATTACH_WIDGET,
					    XmNleftWidget,		prevleft,
					    NULL);
	st = jp->spq_time;
	tp = localtime(&st);
	sprintf(nbuf, "%.2d:%.2d %s %.2d %s %d", tp->tm_hour, tp->tm_min, daynames[tp->tm_wday],
		       tp->tm_mday, monnames[tp->tm_mon], tp->tm_year+1900);
	XmTextSetString(prevabove, nbuf);

	workw[WORKW_RETQBW] = XtVaCreateManagedWidget("retain",
						      xmToggleButtonGadgetClass, mainform,
						      XmNtopAttachment,		XmATTACH_WIDGET,
						      XmNtopWidget,		prevabove,
						      XmNleftAttachment,	XmATTACH_POSITION,
						      XmNleftPosition,		4,
						      NULL);

	if  (jp->spq_jflags & SPQ_RETN)
		XmToggleButtonGadgetSetState(workw[WORKW_RETQBW], True, False);

	workw[WORKW_PRINTED] = XtVaCreateManagedWidget("printed",
						       xmToggleButtonGadgetClass,	mainform,
						       XmNtopAttachment,		XmATTACH_WIDGET,
						       XmNtopWidget,			workw[WORKW_RETQBW],
						       XmNleftAttachment,		XmATTACH_POSITION,
						       XmNleftPosition,			4,
						       NULL);

	if  (jp->spq_dflags & SPQ_PRINTED)
		XmToggleButtonGadgetSetState(workw[WORKW_PRINTED], True, False);

	prevabove = workw[WORKW_PRINTED];

	ptitw = XtVaCreateManagedWidget("pdeltit",
					xmLabelGadgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_FORM,
					NULL);

#ifdef HAVE_XM_SPINB_H

	spinb = XtVaCreateManagedWidget("pdelsp",
					xmSpinBoxWidgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_WIDGET,
					XmNleftWidget,		ptitw,
					NULL);

	workw[WORKW_PDELTXTW] = XtVaCreateManagedWidget("pdel",
							xmTextFieldWidgetClass,		spinb,
							XmNcursorPositionVisible,	False,
							XmNeditable,			False,
							XmNspinBoxChildType,		XmNUMERIC,
							XmNcolumns,			5,
							XmNmaximumValue,		0x7fff,
							XmNminimumValue,		1,
#ifdef	BROKEN_SPINBOX
							XmNpositionType,		XmPOSITION_INDEX,
							XmNposition,			jp->spq_ptimeout-1,
#else
							XmNposition,			jp->spq_ptimeout,
#endif
							NULL);

	prevabove = spinb;

#else /* ! HAVE_XM_SPINB_H */

	workw[WORKW_PDELTXTW] = XtVaCreateManagedWidget("pdel",
							xmTextFieldWidgetClass,		mainform,
							XmNcursorPositionVisible,	False,
							XmNcolumns,			5,
							XmNmaxWidth,			5,
							XmNtopAttachment,		XmATTACH_WIDGET,
							XmNtopWidget,			prevabove,
							XmNleftAttachment,		XmATTACH_WIDGET,
							XmNleftWidget,			ptitw,
							NULL);

	sprintf(nbuf, "%5u", jp->spq_ptimeout);
	XmTextSetString(workw[WORKW_PDELTXTW], nbuf);
	CreateArrowPair("p",				mainform,
			prevabove,			workw[WORKW_PDELTXTW],
			(XtCallbackProc) pup_cb,	(XtCallbackProc) pdn_cb,
			WORKW_PDELTXTW,	WORKW_PDELTXTW,		1);

	prevabove = workw[WORKW_PDELTXTW];

#endif /* !HAVE_XM_SPINB_H */

	nptitw = XtVaCreateManagedWidget("npdeltit",
					 xmLabelGadgetClass,	mainform,
					 XmNtopAttachment,	XmATTACH_WIDGET,
					 XmNtopWidget,		prevabove,
					 XmNleftAttachment,	XmATTACH_FORM,
					 NULL);

#ifdef HAVE_XM_SPINB_H
	spinb = XtVaCreateManagedWidget("npdelsp",
					xmSpinBoxWidgetClass,	mainform,
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		prevabove,
					XmNleftAttachment,	XmATTACH_WIDGET,
					XmNleftWidget,		nptitw,
					NULL);

	workw[WORKW_NPDELTXTW] = XtVaCreateManagedWidget("npdel",
							 xmTextFieldWidgetClass,	spinb,
							 XmNcursorPositionVisible,	False,
							 XmNeditable,			False,
							 XmNcolumns,			5,
							 XmNspinBoxChildType,		XmNUMERIC,
							 XmNmaximumValue,		0x7fff,
							 XmNminimumValue,		1,
#ifdef	BROKEN_SPINBOX
							 XmNpositionType,		XmPOSITION_INDEX,
							 XmNposition,			jp->spq_nptimeout-1,
#else
							 XmNposition,			jp->spq_nptimeout,
#endif
							 NULL);

	prevabove = spinb;

#else

	workw[WORKW_NPDELTXTW] = XtVaCreateManagedWidget("npdel",
							 xmTextFieldWidgetClass,	mainform,
							 XmNmaxWidth,			5,
							 XmNcursorPositionVisible,	False,
							 XmNtopAttachment,		XmATTACH_WIDGET,
							 XmNtopWidget,			prevabove,
							 XmNleftAttachment,		XmATTACH_WIDGET,
							 XmNleftWidget,			nptitw,
							 XmNcolumns,			5,
							 NULL);

	sprintf(nbuf, "%5u", jp->spq_nptimeout);
	XmTextSetString(workw[WORKW_NPDELTXTW], nbuf);
	CreateArrowPair("np",				mainform,
			prevabove,			workw[WORKW_NPDELTXTW],
			(XtCallbackProc) pup_cb,	(XtCallbackProc) pdn_cb,
			WORKW_NPDELTXTW,		WORKW_NPDELTXTW,	1);
	prevabove = workw[WORKW_NPDELTXTW];
#endif /* ! HAVE_XM_SPINB_H */

	prevleft = workw[WORKW_HOLDBW] = XtVaCreateManagedWidget("hold",
								 xmToggleButtonGadgetClass,	mainform,
								 XmNtopAttachment,		XmATTACH_WIDGET,
								 XmNtopWidget,			prevabove,
								 XmNleftAttachment,		XmATTACH_FORM,
								 NULL);

#ifdef HAVE_XM_SPINB_H
	holdspin = XtVaCreateWidget("holdspin",
				    xmSpinBoxWidgetClass,	mainform,
				    XmNtopAttachment,		XmATTACH_WIDGET,
				    XmNtopWidget,		prevabove,
				    XmNleftAttachment,		XmATTACH_WIDGET,
				    XmNleftWidget,		prevleft,
				    NULL);

	workw[WORKW_HOURTW] = XtVaCreateManagedWidget("hour",
						      xmTextFieldWidgetClass,	holdspin,
						      XmNcolumns,		2,
						      XmNmaxWidth,		2,
						      XmNeditable,		False,
						      XmNcursorPositionVisible,	False,
						      XmNspinBoxChildType,	XmSTRING,
						      XmNnumValues,		24,
						      XmNvalues,		timezerof,
						      NULL);

	workw[WORKW_MINTW] = XtVaCreateManagedWidget("min",
						     xmTextFieldWidgetClass,	holdspin,
						     XmNcolumns,		2,
						     XmNmaxWidth,		2,
						     XmNeditable,		False,
						     XmNcursorPositionVisible,	False,
						     XmNspinBoxChildType,	XmSTRING,
						     XmNnumValues,		60,
						     XmNvalues,			timezerof,
						     NULL);

	workw[WORKW_DOWTW] = XtVaCreateManagedWidget("dow",
						     xmTextFieldWidgetClass,	holdspin,
						     XmNcolumns,		longest_day,
						     XmNmaxWidth,		longest_day,
						     XmNeditable,		False,
						     XmNcursorPositionVisible,	False,
						     XmNspinBoxChildType,	XmSTRING,
						     XmNnumValues,		7,
						     XmNvalues,			stdaynames,
						     NULL);

	workw[WORKW_DOMTW] = XtVaCreateManagedWidget("dm",
						     xmTextFieldWidgetClass,	holdspin,
						     XmNcolumns,		2,
						     XmNmaxWidth,		2,
						     XmNeditable,		False,
						     XmNcursorPositionVisible,	False,
						     XmNspinBoxChildType,	XmNUMERIC,
						     XmNmaximumValue,		31,
						     XmNminimumValue,		1,
#ifdef	BROKEN_SPINBOX
						     XmNpositionType,		XmPOSITION_INDEX,
						     XmNposition,		0,
#else
						     XmNposition,		1,
#endif
						     NULL);

	workw[WORKW_MONTW] = XtVaCreateManagedWidget("mon",
						     xmTextFieldWidgetClass,	holdspin,
						     XmNcolumns,		longest_mon,
						     XmNmaxWidth,		longest_mon,
						     XmNeditable,		False,
						     XmNcursorPositionVisible,	False,
						     XmNspinBoxChildType,	XmSTRING,
						     XmNnumValues,		12,
						     XmNvalues,			stmonnames,
						     NULL);

	workw[WORKW_YEARTW] = XtVaCreateManagedWidget("yr",
						      xmTextFieldWidgetClass,	holdspin,
						      XmNcolumns,		4,
						      XmNmaxWidth,		4,
						      XmNeditable,		False,
						      XmNcursorPositionVisible,	False,
						      XmNspinBoxChildType,	XmNUMERIC,
						      XmNmaximumValue,		9999,
						      XmNminimumValue,		1900,
#ifdef	BROKEN_SPINBOX
						      XmNpositionType,		XmPOSITION_INDEX,
						      XmNposition,		0,
#else
						      XmNposition,		1900,
#endif
						      NULL);

	XtAddCallback(holdspin, XmNmodifyVerifyCallback, (XtCallbackProc) holdt_cb, 0);

#else  /* ! HAVE_XM_SPINB_H */

	holdspin = XtVaCreateWidget("holdspin",
				    xmFormWidgetClass,		mainform,
				    XmNtopAttachment,		XmATTACH_WIDGET,
				    XmNtopWidget,		prevleft, /* Did mean that */
				    XmNleftAttachment,		XmATTACH_FORM,
				    NULL);

	workw[WORKW_HOURTW] = XtVaCreateManagedWidget("hour",
						      xmTextFieldWidgetClass,	holdspin,
						      XmNcolumns,		2,
						      XmNmaxWidth,		2,
						      XmNeditable,		False,
						      XmNcursorPositionVisible,	False,
						      XmNtopAttachment,		XmATTACH_FORM,
						      XmNleftAttachment,	XmATTACH_FORM,
						      NULL);

	prevleft = CreateArrowPair("h",	holdspin, 0, workw[WORKW_HOURTW], (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_HOURTW, WORKW_HOURTW, 1);

	prevleft = XtVaCreateManagedWidget(":",
					   xmLabelGadgetClass,	holdspin,
					   XmNtopAttachment,	XmATTACH_FORM,
					   XmNleftAttachment,	XmATTACH_WIDGET,
					   XmNleftWidget,	prevleft,
					   NULL);

	prevleft = workw[WORKW_MINTW] = XtVaCreateManagedWidget("min",
								xmTextFieldWidgetClass,		holdspin,
								XmNcolumns,			2,
								XmNmaxWidth,			2,
								XmNeditable,			False,
								XmNcursorPositionVisible,	False,
								XmNtopAttachment,		XmATTACH_FORM,
								XmNleftAttachment,		XmATTACH_WIDGET,
								XmNleftWidget,			prevleft,
								NULL);

	prevleft = CreateArrowPair("m", holdspin, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_MINTW, WORKW_MINTW, 1);

	prevleft = workw[WORKW_DOWTW] = XtVaCreateManagedWidget("dow",
								xmTextFieldWidgetClass,		holdspin,
								XmNcolumns,			longest_day,
								XmNmaxWidth,			longest_day,
								XmNeditable,			False,
								XmNcursorPositionVisible,	False,
								XmNtopAttachment,		XmATTACH_FORM,
								XmNleftAttachment,		XmATTACH_WIDGET,
								XmNleftWidget,			prevleft,
								NULL);

	prevleft = CreateArrowPair("dw", holdspin, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_DOWTW, WORKW_DOWTW, 1);

	prevleft = workw[WORKW_DOMTW] = XtVaCreateManagedWidget("dm",
								xmTextFieldWidgetClass,		holdspin,
								XmNcolumns,			2,
								XmNmaxWidth,			2,
								XmNeditable,			False,
								XmNcursorPositionVisible,	False,
								XmNtopAttachment,		XmATTACH_FORM,
								XmNleftAttachment,		XmATTACH_WIDGET,
								XmNleftWidget,			prevleft,
								NULL);

	prevleft = CreateArrowPair("dm", holdspin, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_DOMTW, WORKW_DOMTW, 1);

	prevleft = workw[WORKW_MONTW] = XtVaCreateManagedWidget("mon",
								xmTextFieldWidgetClass,		holdspin,
								XmNcolumns,			longest_mon,
								XmNmaxWidth,			longest_mon,
								XmNeditable,			False,
								XmNcursorPositionVisible,	False,
								XmNtopAttachment,		XmATTACH_FORM,
								XmNleftAttachment,		XmATTACH_WIDGET,
								XmNleftWidget,			prevleft,
								NULL);

	prevleft = CreateArrowPair("mon", holdspin, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_MONTW, WORKW_MONTW, 1);

	prevleft = workw[WORKW_YEARTW] = XtVaCreateManagedWidget("yr",
								 xmTextFieldWidgetClass,	holdspin,
								 XmNcolumns,			4,
								 XmNmaxWidth,			4,
								 XmNeditable,			False,
								 XmNcursorPositionVisible,	False,
								 XmNtopAttachment,		XmATTACH_FORM,
								 XmNleftAttachment,		XmATTACH_WIDGET,
								 XmNleftWidget,			prevleft,
								 NULL);

	prevleft = CreateArrowPair("yr", holdspin, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_YEARTW, WORKW_YEARTW, 1);

#endif /* ! HAVE_XM_SPINB_H */

	if  (jp->spq_hold != 0)  {
		XmToggleButtonGadgetSetState(workw[WORKW_HOLDBW], True, False);
		fillintimes(1);
	}

	XtAddCallback(workw[WORKW_HOLDBW], XmNvalueChangedCallback, (XtCallbackProc) holdt_set, (XtPointer) 0);
	XtManageChild(mainform);
	CreateActionEndDlg(jr_shell, panew, (XtCallbackProc) endjret, $H{xmspq retain dlg});
}

static void  endjclass(Widget w, int data)
{
	if  (data)  {
		if  (copyclasscode == 0)  {
			doerror(w, $EH{xmspq setting zero class});
			return;
		}
		if  (XmToggleButtonGadgetGetState(workw[WORKW_LOCO]))
			JREQ.spq_jflags |= SPQ_LOCALONLY;
		else
			JREQ.spq_jflags &= ~SPQ_LOCALONLY;

		JREQ.spq_class = copyclasscode;
		my_wjmsg(SJ_CHNG);
	}
	XtDestroyWidget(GetTopShell(w));
}

void  cb_jclass(Widget parent)
{
	Widget	jc_shell, panew, mainform, prevabove;
	const  struct	spq	*jp = getselectedjob(PV_OTHERJ);

	if  (!jp)
		return;

	prevabove = CreateJeditDlg(parent, "Jclass", &jc_shell, &panew, &mainform);
	prevabove = CreateCCDialog(mainform, prevabove, jp->spq_class, (Widget *) 0);
	workw[WORKW_LOCO] = XtVaCreateManagedWidget("loconly",
						    xmToggleButtonGadgetClass,	mainform,
						    XmNtopAttachment,		XmATTACH_WIDGET,
						    XmNtopWidget,		prevabove,
						    XmNleftAttachment,		XmATTACH_POSITION,
						    XmNleftPosition,		4,
						    NULL);
	if  (jp->spq_jflags & SPQ_LOCALONLY)
		XmToggleButtonGadgetSetState(workw[WORKW_LOCO], True, False);
	XtManageChild(mainform);
	CreateActionEndDlg(jc_shell, panew, (XtCallbackProc) endjclass, $H{xmspq class menu});
}

static void  endjunqueue(Widget w, int data)
{
	if  (data)  {
		int	copyonly;
		char	*dtxt, *jftxt, *cftxt;
		PIDTYPE	pid;
		struct	stat	sbuf;
		char	*arg0, **ap, *argbuf[8];
		char	jobnobuf[40];
		static	char	*udprog;

		if  (!udprog)
			udprog = envprocess(DUMPJOB);
		copyonly = XmToggleButtonGadgetGetState(workw[WORKW_JCONLY]);
		XtVaGetValues(workw[WORKW_JDIRW], XmNvalue, &dtxt, NULL);
		if  (dtxt[0] == '\0')  {
			XtFree(dtxt);
			doerror(w, $EH{xmspq no directory});
			return;
		}
		if  (dtxt[0] != '/')  {
			disp_str = dtxt;
			doerror(w, $EH{Not absolute path});
			XtFree(dtxt);
			return;
		}
		if  (stat(dtxt, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
			disp_str = dtxt;
			doerror(w, $EH{Not a directory});
			XtFree(dtxt);
			return;
		}
		free(Last_unqueue_dir);
		Last_unqueue_dir = stracpy(dtxt);
		XtFree(dtxt);	/* According to spec cannot mix XtFree and free */
		XtVaGetValues(workw[WORKW_JCFW], XmNvalue, &cftxt, NULL);
		if  (cftxt[0] == '\0')  {
			XtFree(cftxt);
			doerror(w, $EH{xmspq no cmd file});
			return;
		}
		XtVaGetValues(workw[WORKW_JJFW], XmNvalue, &jftxt, NULL);
		if  (jftxt[0] == '\0')  {
			XtFree(jftxt);
			XtFree(cftxt);
			doerror(w, $EH{xmspq no job file});
			return;
		}
		XtDestroyWidget(GetTopShell(w));
		if  ((pid = fork()))  {
			int	status;

			XtFree(jftxt);
			XtFree(cftxt);

			if  (pid < 0)  {
				doerror(jwid, $EH{Unqueue no fork});
				return;
			}

#ifdef	HAVE_WAITPID
			while  (waitpid(pid, &status, 0) < 0)
				;
#else
			while  (wait(&status) != pid)
				;
#endif
			if  (status == 0)	/* All ok */
				return;
			if  (status & 0xff)  {
				disp_arg[9] = status & 0xff;
				doerror(jwid, $EH{Unqueue program fault});
				return;
			}
			status = (status >> 8) & 0xff;
			disp_arg[0] = JREQ.spq_job;
			disp_str = JREQ.spq_file;
			switch  (status)  {
			default:
				disp_arg[1] = status;
				doerror(jwid, $EH{Unqueue misc error});
				return;
			case  E_SETUP:
				disp_str = udprog;
				doerror(jwid, $EH{Cannot find unqueue});
				return;
			case  E_JDFNFND:
				doerror(jwid, $EH{Unqueue spool not found});
				return;
			case  E_JDNOCHDIR:
				doerror(jwid, $EH{Unqueue dir not found});
				return;
			case  E_JDFNOCR:
				doerror(jwid, $EH{Unqueue no create});
				return;
			case  E_JDJNFND:
				doerror(jwid, $EH{Unqueue unknown job});
				return;
			}
		}

		/* Child process */

		setuid(Realuid);
		chdir(Curr_pwd);	/* So it picks up the right config file */
		if  (JREQ.spq_netid)
			sprintf(jobnobuf, "%s:%ld", look_host(JREQ.spq_netid), (long) JREQ.spq_job);
		else
			sprintf(jobnobuf, "%ld", (long) JREQ.spq_job);
		ap = argbuf;
		*ap++ = (arg0 = strrchr(udprog, '/'))? arg0 + 1: udprog;
		if  (copyonly)
			*ap++ = "-n";
		*ap++ = jobnobuf;
		*ap++ = Last_unqueue_dir;
		*ap++ = cftxt;
		*ap++ = jftxt;
		*ap++ = (char *) 0;
		execv(udprog, argbuf);
		exit(E_SETUP);
	}
	XtDestroyWidget(GetTopShell(w));
}

static void  enddseld(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
	char	*dirname;

	if  (data)  {
		int	n;
		if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &dirname))
			return;
		if  (!dirname  ||  dirname[0] == '\0')
			return;
		if  (dirname[n = strlen(dirname) - 1] == '/')
			dirname[n] = '\0';
		XmTextSetString(workw[WORKW_JDIRW], dirname);
		XtFree(dirname);
	}
	XtDestroyWidget(w);
}

static void  selectdir(Widget w)
{
	Widget	dseld;
	char	*txt;

	XtVaGetValues(workw[WORKW_JDIRW], XmNvalue, &txt, NULL);
	if  (chdir(txt) < 0)
		chdir(Curr_pwd);
	XtFree(txt);
	dseld = XmCreateFileSelectionDialog(w, "dselb", NULL, 0);
	XtAddCallback(dseld, XmNcancelCallback, (XtCallbackProc) enddseld, (XtPointer) 0);
	XtAddCallback(dseld, XmNokCallback, (XtCallbackProc) enddseld, (XtPointer) 1);
	chdir(spdir);
	XtManageChild(dseld);
}

void  cb_unqueue(Widget parent)
{
	Widget	ju_shell, panew, mainform, prevabove, prevleft, dselb;
	const  struct	spq	*jp = getselectedjob(PV_OTHERJ);
	char	nbuf[12];

	if  (!jp)
		return;
	if  (!(mypriv->spu_flgs & PV_UNQUEUE))  {
		doerror(jwid, $EH{spq cannot unqueue});
		return;
	}

	if  (!Last_unqueue_dir)
		Last_unqueue_dir = stracpy(Curr_pwd);

	prevabove = CreateJeditDlg(parent, "Junqueue", &ju_shell, &panew, &mainform);

	prevabove = XtVaCreateManagedWidget("Directory",
					    xmLabelGadgetClass,	mainform,
					    XmNtopAttachment,	XmATTACH_WIDGET,
					    XmNtopWidget,		prevabove,
					    XmNleftAttachment,	XmATTACH_FORM,
					    NULL);

	prevabove =
		workw[WORKW_JDIRW] =
			XtVaCreateManagedWidget("dir",
						xmTextFieldWidgetClass,		mainform,
						XmNcursorPositionVisible,	False,
						XmNtopAttachment,		XmATTACH_WIDGET,
						XmNtopWidget,			prevabove,
						XmNleftAttachment,		XmATTACH_FORM,
						XmNrightAttachment,		XmATTACH_FORM,
						NULL);

	XmTextSetString(prevabove, Last_unqueue_dir);

	prevabove = dselb = XtVaCreateManagedWidget("dselect",
						    xmPushButtonWidgetClass,	mainform,
						    XmNtopAttachment,		XmATTACH_WIDGET,
						    XmNtopWidget,		prevabove,
						    XmNrightAttachment,		XmATTACH_FORM,
						    NULL);

	XtAddCallback(dselb, XmNactivateCallback, (XtCallbackProc) selectdir, (XtPointer) 0);

	prevabove = workw[WORKW_JCONLY] = XtVaCreateManagedWidget("copyonly",
								  xmToggleButtonGadgetClass,	mainform,
								  XmNtopAttachment,		XmATTACH_WIDGET,
								  XmNtopWidget,			prevabove,
								  XmNleftAttachment,		XmATTACH_FORM,
								  NULL);

	prevleft = XtVaCreateManagedWidget("Cmdfile",
					   xmLabelGadgetClass,	mainform,
					   XmNtopAttachment,	XmATTACH_WIDGET,
					   XmNtopWidget,	prevabove,
					   XmNleftAttachment,	XmATTACH_FORM,
					   NULL);

	workw[WORKW_JCFW] =
		XtVaCreateManagedWidget("cfile",
					xmTextFieldWidgetClass,		mainform,
					XmNcursorPositionVisible,	False,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			prevleft,
					XmNrightAttachment,		XmATTACH_FORM,
					NULL);
	prevabove = workw[WORKW_JCFW];
	sprintf(nbuf, "C%ld", (long) jp->spq_job);
	XmTextSetString(prevabove, nbuf);

	prevleft = XtVaCreateManagedWidget("Jobfile",
					   xmLabelGadgetClass,	mainform,
					   XmNtopAttachment,	XmATTACH_WIDGET,
					   XmNtopWidget,	prevabove,
					   XmNleftAttachment,	XmATTACH_FORM,
					   NULL);

	workw[WORKW_JJFW] =
		XtVaCreateManagedWidget("jfile",
					xmTextFieldWidgetClass,		mainform,
					XmNcursorPositionVisible,	False,
					XmNtopAttachment,		XmATTACH_WIDGET,
					XmNtopWidget,			prevabove,
					XmNleftAttachment,		XmATTACH_WIDGET,
					XmNleftWidget,			prevleft,
					XmNrightAttachment,		XmATTACH_FORM,
					NULL);
	sprintf(nbuf, "J%ld", (long) jp->spq_job);
	XmTextSetString(workw[WORKW_JJFW], nbuf);
	XtManageChild(mainform);
	CreateActionEndDlg(ju_shell, panew, (XtCallbackProc) endjunqueue, $H{xmspq unqueue help});
}

static void  jmacroexec(char *str, const struct spq *jp)
{
	static	char	*execprog;
	PIDTYPE	pid;
	int	status;

	if  (!execprog)
		execprog = envprocess(EXECPROG);

	if  ((pid = fork()) == 0)  {
		char	nbuf[20+HOSTNSIZE];
		char	*argbuf[3];
		argbuf[0] = str;
		if  (jp)  {
			if  (jp->spq_netid)
				sprintf(nbuf, "%s:%ld", look_host(jp->spq_netid), (long) jp->spq_job);
			else
				sprintf(nbuf, "%ld", (long) jp->spq_job);
			argbuf[1] = nbuf;
			argbuf[2] = (char *) 0;
		}
		else
			argbuf[1] = (char *) 0;
		chdir(Curr_pwd);
		execv(execprog, argbuf);
		exit(255);
	}
	if  (pid < 0)  {
		doerror(jwid, $EH{Macro fork failed});
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
			doerror(jwid, $EH{Macro command gave signal});
		}
		else  {
			disp_arg[0] = (status >> 8) & 255;
			doerror(jwid, $EH{Macro command error});
		}
	}
}

static void  endjmacro(Widget w, int data)
{
	if  (data)  {
		char	*txt;
		XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &txt, NULL);
		if  (txt[0])  {
			const  struct	spq	*jp = (const struct spq *) 0;
			int	*plist, pcnt;
			if  (XmListGetSelectedPos(jwid, &plist, &pcnt) && pcnt > 0)  {
				jp = &Job_seg.jj_ptrs[plist[0] - 1]->j;
				XtFree((XtPointer) plist);
			}
			jmacroexec(txt, jp);
		}
		XtFree(txt);
	}
	XtDestroyWidget(GetTopShell(w));
}

void  cb_macroj(Widget parent, int data)
{
	char	*prompt = helpprmpt(data + $P{Job or User macro});
	int	*plist, pcnt;
	const  struct	spq	*jp = (const struct spq *) 0;
	Widget	jc_shell, panew, mainform, labw;

	if  (!prompt)  {
		disp_arg[0] = data + $P{Job or User macro};
		doerror(jwid, $EH{Macro error});
		return;
	}

	if  (XmListGetSelectedPos(jwid, &plist, &pcnt) && pcnt > 0)  {
		jp = &Job_seg.jj_ptrs[plist[0] - 1]->j;
		XtFree((XtPointer) plist);
	}

	if  (data != 0)  {
		jmacroexec(prompt, jp);
		return;
	}

	CreateEditDlg(parent, "jobcmd", &jc_shell, &panew, &mainform, 3);
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
	CreateActionEndDlg(jc_shell, panew, (XtCallbackProc) endjmacro, $H{Job or User macro});
}
