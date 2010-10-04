/* xmsq_plist.c -- xmspq display printer list

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
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
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
#include <Xm/FileSB.h>
#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#ifdef HAVE_XM_SPINB_H
#include <Xm/SpinB.h>
#endif
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include "incl_sig.h"
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

/* Vector of states - assumed given prompt codes consecutively.  */

static	char	*statenames[SPP_NSTATES];

static	char	*halteoj,	/* Halt at end of job message */
		*namsg,		/* Non-aligned message */
		*nsmsg,		/* Network indication start */
		*nemsg,		/* Network indication end */
		*intermsg;

#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
static	char	*statecoloursfg[SPP_NSTATES];
#endif
#ifndef HAVE_XM_SPINB_H
extern	unsigned	arrow_min,
			arrow_max,
			arrow_lng;
extern void  arrow_incr(Widget, XtIntervalId *);
extern void  arrow_decr(Widget, XtIntervalId *);
#endif

int  proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
void  allocate_colours()
{
	Pixel		origfg;
	int		cnt, rendcnt = 0;
	XmRendition	rend[SPP_NSTATES];
	XmRenderTable	ortab;

	/* First get the original foreground and the rendition table */

	XtVaGetValues(pwid, XmNforeground, &origfg, XmNrenderTable, &ortab, NULL);

	for  (cnt = 0;  cnt < SPP_NSTATES;  cnt++)  {
		char	*cnmfg = statecoloursfg[cnt];
		Arg	av[1];
		char	nbuf[20];

		/* If blank, default background, skip */

		if  (!cnmfg)
			continue;

		if  (*cnmfg)  {	/* Non-blank - Gyrations to get pixel value for colour */
			Pixel	newfg;
			XtVaSetValues(pwid, XtVaTypedArg, XmNforeground, XmRString, cnmfg, strlen(cnmfg)+1, NULL);
			XtVaGetValues(pwid, XmNforeground, &newfg, NULL);
			XtSetArg(av[0], XmNrenditionForeground, newfg);
		}
		else
			XtSetArg(av[0], XmNrenditionForeground, origfg);

		sprintf(nbuf, "Statcol%d", cnt);
		rend[rendcnt] = XmRenditionCreate(pwid, nbuf, av, 1);
		rendcnt++;
	}
	if  (rendcnt > 0)  {
		XmRenderTable  ortc = XmRenderTableCopy(ortab, NULL, 0);
		XmRenderTable  rtab = XmRenderTableAddRenditions(ortc, rend, rendcnt, XmMERGE_REPLACE);
		XtVaSetValues(pwid, XmNforeground, origfg, XmNrenderTable, rtab, NULL);
	}

	for  (cnt = 0;  cnt < SPP_NSTATES;  cnt++)
		if  (statecoloursfg[cnt])  {
			free(statecoloursfg[cnt]);
			statecoloursfg[cnt]= (char *) 0;
		}
}
#endif

/* Open print file.  */
void  openpfile()
{
	int	i;

	if  (!ptrshminit(1))  {
		print_error($E{Cannot open pshm});
		exit(E_PRINQ);
	}

	/* Read state names */

	for  (i = 0;  i < SPP_NSTATES;  i++)
		statenames[i] = gprompt($P{Printer status null}+i);
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
	for  (i = 0;  i < SPP_NSTATES;  i++)
		statecoloursfg[i] = helpprmpt($P{Printer scolour null}+i);
#endif
	halteoj = gprompt($P{Printer heoj});
	intermsg = gprompt($P{Printer interrupted});
	namsg = gprompt($P{Printer not aligned});
	nsmsg = gprompt($P{Netdev start str});
	nemsg = gprompt($P{Netdev end str});
}

extern	char	*job_format;
static	char	*ptr_format;
#define	DEFAULT_FORMAT	"%15p %10d %16f %>9s %3x %7j %7u %6w"
static	char		*obuf, *bigbuff;
typedef	int	fmt_t;
#include "inline/pfmt_ab.c"
#include "inline/pfmt_class.c"
#include "inline/pfmt_dev.c"
#include "inline/pfmt_form.c"
#include "inline/pfmt_heoj.c"
#include "inline/pfmt_pid.c"
#include "inline/pfmt_jobno.c"
#include "inline/pfmt_loco.c"
#include "inline/pfmt_msg.c"
#include "inline/pfmt_na.c"
#include "inline/pfmt_ptr.c"
#include "inline/pfmt_state.c"
#include "inline/pfmt_ostat.c"
#include "inline/pfmt_user.c"
#include "inline/pfmt_minsz.c"
#include "inline/pfmt_maxsz.c"
#include "inline/pfmt_limit.c"
#include "inline/pfmt_shrk.c"

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct	formatdef  {
	SHORT	statecode;	/* Code number for heading if applicable */
	SHORT	sugg_width;	/* Suggested width */
	char	*msg;		/* Heading */
	char	*explain;	/* Detailed explanation */
	int	(*fmt_fn)(const struct spptr *, const int);
};

#define	NULLCP	(char *) 0

static	struct	formatdef
	lowertab[] = { /* a-z */
	{	$P{Printer title}+'a'-1,6,		NULLCP, NULLCP,	fmt_ab		},	/* a */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* b */
	{	$P{Printer title}+'c'-1,32,		NULLCP, NULLCP,	fmt_class	},	/* c */
	{	$P{Printer title}+'d'-1,LINESIZE-4,	NULLCP, NULLCP, fmt_device	},	/* d */
	{	$P{Printer title}+'e'-1,COMMENTSIZE-10,	NULLCP, NULLCP,	fmt_comment	},	/* e */
	{	$P{Printer title}+'f'-1,MAXFORM-4,	NULLCP, NULLCP, fmt_form	},	/* f */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* g */
	{	$P{Printer title}+'h'-1,6,		NULLCP, NULLCP,	fmt_heoj	},	/* h */
	{	$P{Printer title}+'i'-1,5,		NULLCP, NULLCP,	fmt_pid		},	/* i */
	{	$P{Printer title}+'j'-1,6,		NULLCP, NULLCP,	fmt_jobno	},	/* j */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* k */
	{	$P{Printer title}+'l'-1,6,		NULLCP, NULLCP,	fmt_localonly	},	/* l */
	{	$P{Printer title}+'m'-1,10,		NULLCP, NULLCP,	fmt_message	},	/* m */
	{	$P{Printer title}+'n'-1,6,		NULLCP, NULLCP,	fmt_na		},	/* n */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* o */
	{	$P{Printer title}+'p'-1,PTRNAMESIZE-4,	NULLCP, NULLCP,	fmt_printer	},	/* p */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* q */
	{	0,			0,		NULLCP, NULLCP, 0		},	/* r */
	{	$P{Printer title}+'s'-1,8,		NULLCP, NULLCP, fmt_state	},	/* s */
	{	$P{Printer title}+'t'-1,8,		NULLCP, NULLCP, fmt_ostate	},	/* t */
	{	$P{Printer title}+'u'-1,UIDSIZE-2,	NULLCP, NULLCP, fmt_user	},	/* u */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* v */
	{	$P{Printer title}+'w'-1,4,		NULLCP, NULLCP,	fmt_shriek	},	/* w */
	{	$P{Printer title}+'x'-1,2,		NULLCP, NULLCP,	fmt_limit	},	/* x */
	{	$P{Printer title}+'y'-1,6,		NULLCP, NULLCP,	fmt_minsize	},	/* y */
	{	$P{Printer title}+'z'-1,6,		NULLCP, NULLCP,	fmt_maxsize	}	/* z */
};

char *get_ptrtitle()
{
	int	nn, obuflen;
	struct	formatdef	*fp;
	char	*cp, *rp, *result, *mp;

	if  (!ptr_format  &&  !(ptr_format = helpprmpt($P{xmspq ptr default format})))
		ptr_format = stracpy(DEFAULT_FORMAT);

	/* Initial pass to discover how much space to allocate */

	obuflen = 1;
	cp = ptr_format;
	while  (*cp)  {
		if  (*cp++ != '%')  {
			obuflen++;
			continue;
		}
		if  (*cp == '<' || *cp == '>')
			cp++;
		nn = 0;
		do  nn = nn * 10 + *cp++ - '0';
		while  (isdigit(*cp));
		obuflen += nn;
		if  (isalpha(*cp))
			cp++;
	}

	/* Allocate space for title result and output buffer */

	result = malloc((unsigned) (obuflen + 1));
	if  (obuf)
		free(obuf);
	if  (bigbuff)
		free(bigbuff);
	obuf = malloc((unsigned) obuflen);
	bigbuff = malloc(4 * obuflen);
	if  (!result ||  !obuf  ||  !bigbuff)
		nomem();

	/* Now set up title Actually this is a waste of time if we
	   aren't actually displaying same, but we needed the buffer.  */

	rp = result;
	*rp++ = ' ';
	cp = ptr_format;
	while  (*cp)  {
		if  (*cp != '%')  {
			*rp++ = *cp++;
			continue;
		}
		cp++;

		/* Get width */

		if  (*cp == '<' || *cp == '>')
			cp++;
		nn = 0;
		do  nn = nn * 10 + *cp++ - '0';
		while  (isdigit(*cp));

		/* Get format char */

		if  (islower(*cp))
			fp = &lowertab[*cp - 'a'];
		else  {
			if  (*cp)
				cp++;
			continue;
		}

		cp++;
		if  (fp->statecode == 0)
			continue;

		/* Get title message if we don't have it Insert into result */

		if  (!fp->msg)
			fp->msg = gprompt(fp->statecode);

		mp = fp->msg;
		while  (nn > 0  &&  *mp)  {
			*rp++ = *mp++;
			nn--;
		}
		while  (nn > 0)  {
			*rp++ = ' ';
			nn--;
		}
	}

	*rp = '\0';

	/* We don't trim trailing spaces so that we have enough room
	   for what comes under the title.  */

	return  result;
}

/* Display contents of printer list.  Don't put it on screen yet.  */

void  pdisplay()
{
	int	topp = 1, cptrpos = -1, newpos = -1, pcnt, plines;
	XmString	*elist;
	netid_t	Chostno = -1;
	slotno_t Cslotno = -1;

	if  (Ptr_seg.nptrs != 0)  {
		int	*plist;

		/* First discover the currently-selected ptr and the
		   scroll position, if any.  */

		XtVaGetValues(pwid,
			      XmNtopItemPosition,	&topp,
			      XmNvisibleItemCount,	&plines,
			      NULL);

		if  (XmListGetSelectedPos(pwid, &plist, &pcnt) && pcnt > 0)  {
			cptrpos = plist[0] - 1;
			XtFree((char *) plist);
			if  ((unsigned) cptrpos < Ptr_seg.nptrs  &&
			     Ptr_seg.pp_ptrs[cptrpos]->p.spp_state != SPP_NULL)  {
				Cslotno = Ptr_seg.pp_ptrs[cptrpos]->p.spp_rslot;
				Chostno = Ptr_seg.pp_ptrs[cptrpos]->p.spp_netid;
			}
		}
		XtVaGetValues(pwid, XmNitems, &elist, NULL);
	}

	XmListDeleteAllItems(pwid);
	readptrlist(1);

	for  (pcnt = 0;  pcnt < Ptr_seg.nptrs;  pcnt++)  {
		XmString	str;
		const  struct  spptr  *pp = &Ptr_seg.pp_ptrs[pcnt]->p;
		struct	formatdef  *fp;
		char		*cp = ptr_format, *rp = obuf, *lbp;
		int		currplace = -1, lastplace, nn, inlen;
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
		char		nbuf[20];
#endif

		if  (pp->spp_netid == Chostno  &&  pp->spp_rslot == Cslotno)
			newpos = pcnt;

		while  (*cp)  {
			int	skiprest = 0;
			if  (*cp != '%')  {
				*rp++ = *cp++;
				continue;
			}
			cp++;
			lastplace = -1;
			if  (*cp == '<')  {
				lastplace = currplace;
				cp++;
			}
			else   if  (*cp == '>')  {
				skiprest = 1;
				cp++;
			}
			nn = 0;
			do  nn = nn * 10 + *cp++ - '0';
			while  (isdigit(*cp));

			/* Get format char */

			if  (islower(*cp))
				fp = &lowertab[*cp - 'a'];
			else  {
				if  (*cp)
					cp++;
				continue;
			}
			cp++;
			if  (!fp->fmt_fn)
				continue;
			currplace = rp - obuf;
			inlen = (fp->fmt_fn)(pp, nn);
			lbp = bigbuff;
			if  (inlen > nn)  {
				if  (skiprest)  {
					do  *rp++ = *lbp++;
					while  (*lbp);
					break;
				}
				if  (lastplace >= 0)  {
					rp = &obuf[lastplace];
					nn = currplace + nn - lastplace;
					inlen = (fp->fmt_fn)(pp, nn);
				}
			}
			while  (inlen > 0  &&  nn > 0)  {
				*rp++ = *lbp++;
				inlen--;
				nn--;
			}
			while  (nn > 0)  {
				*rp++ = ' ';
				nn--;
			}
		}
		*rp = '\0';
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
		sprintf(nbuf, "Statcol%d", pp->spp_state);
		str = XmStringGenerate(obuf, XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, nbuf);
#else
		str = XmStringCreateLocalized(obuf);
#endif
		XmListAddItem(pwid, str, 0);
		XmStringFree(str);
	}

	/* Adjust scrolling.  */

	if  (newpos >= 0)  {	/* Only gets set if we had one */
		XmListSelectPos(pwid, newpos+1, False);
		topp += newpos - cptrpos;
		if  (topp <= 0)
			topp = 1;
	}
	if  (topp+plines > Ptr_seg.nptrs)	/* Shrunk */
		XmListSetBottomPos(pwid, (int) Ptr_seg.nptrs);
	else
		XmListSetPos(pwid, topp);
}

/* Stuff to edit formats */

Widget	sep_valw;
static	Widget	listw, eatw, skipw;
static	int	whicline;
static	unsigned	char	wfld, isinsert;

static void  fillpdisplist()
{
	char	*cp, *lbp;
	int		nn;
	XmString	str;
	struct	formatdef	*fp;

	cp = ptr_format;
	while  (*cp)  {
		lbp = bigbuff;
		if  (*cp != '%')  {
			*lbp++ = '\"';
			do	*lbp++ = *cp++;
			while  (*cp  &&  *cp != '%');
			*lbp++ = '\"';
			*lbp = '\0';
		}
		else  {
			cp++;
			*lbp = ' ';
			if  (*cp == '<' || *cp == '>')
				*lbp = *cp++;
			lbp++;
			*lbp++ = ' ';
			nn = 0;
			do  nn = nn * 10 + *cp++ - '0';
			while  (isdigit(*cp));
			if  (islower(*cp))
				fp = &lowertab[*cp - 'a'];
			else  {
				if  (*cp)
					cp++;
				continue;
			}
			*lbp++ = *cp++;
			if  (fp->statecode == 0)
				continue;
			sprintf(lbp, " %3d ", nn);
			lbp += 5;
			if  (!fp->explain)
				fp->explain = gprompt(fp->statecode+500);
			strcpy(lbp, fp->explain);
		}
		str = XmStringCreateLocalized(bigbuff);
		XmListAddItem(listw, str, 0);
		XmStringFree(str);
	}
}

static void  fld_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
	if  (cbs->set)  {
		struct	formatdef  *fp;
		wfld = (unsigned char) n;
		fp = &lowertab[n - 'a'];
#ifdef HAVE_XM_SPINB_H
#ifdef BROKEN_SPINBOX
		if  (fp->fmt_fn)
			XtVaSetValues(sep_valw, XmNposition, fp->sugg_width-1, NULL);
#else
		if  (fp->fmt_fn)
			XtVaSetValues(sep_valw, XmNposition, fp->sugg_width, NULL);
#endif
#else	/* ! HAVE_XM_SPINB_H */
		if  (fp->fmt_fn)  {
			char	nbuf[6];
			sprintf(nbuf, "%3d", fp->sugg_width);
			XmTextSetString(sep_valw, nbuf);
		}
#endif	/* ! HAVE_XM_SPINB_H */
	}
}

static void  endnewedit(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		char	*lbp;
		int	nn;
		XmString	str;
#ifndef HAVE_XM_SPINB_H
		char	*txt;
#endif
		if  (wfld > 127)
			return;
#ifdef HAVE_XM_SPINB_H
		XtVaGetValues(sep_valw, XmNposition, &nn, NULL);
#ifdef BROKEN_SPINBOX
		nn++;
#endif
#else
		XtVaGetValues(sep_valw, XmNvalue, &txt, NULL);
		nn = atoi(txt);
		XtFree(txt);
		if  (nn <= 0)
			return;
#endif
		lbp = bigbuff;
		*lbp++ = XmToggleButtonGadgetGetState(eatw)? '<': XmToggleButtonGadgetGetState(skipw)? '>': ' ';
		sprintf(lbp, " %c %3d ", (char) wfld, nn);
		lbp += 7;
		strcpy(lbp, lowertab[wfld - 'a'].explain);
		str = XmStringCreateLocalized(bigbuff);
		if  (isinsert)
			XmListAddItem(listw, str, whicline <= 0? 0: whicline);
		else
			XmListReplaceItemsPos(listw, &str, 1, whicline);
		XmStringFree(str);
	}
	XtDestroyWidget(GetTopShell(w));
}

#ifndef HAVE_XM_SPINB_H
void  widup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)  {
		arrow_max = 255;
		arrow_lng = 3;
		arrow_incr(sep_valw, NULL);
	}
	else
		XtRemoveTimeOut(arrow_timer);
}

void  widdn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
	if  (cbs->reason == XmCR_ARM)  {
		arrow_min = 1;
		arrow_lng = 3;
		arrow_decr(sep_valw, NULL);
	}
	else
		XtRemoveTimeOut(arrow_timer);
}
#endif

static void  newrout(Widget w, int isnew)
{
	Widget	ae_shell, panew, formw, prevleft, fldrc;
	int	*plist, cnt, wotc = 255, widsetting = 10, iseat = 0, isskip = 0;
#ifndef HAVE_XM_SPINB_H
	char	nbuf[6];
#endif

	whicline = -1;
	isinsert = isnew;
	if  (XmListGetSelectedPos(listw, &plist, &cnt)  &&  cnt > 0)  {
		whicline = plist[0];
		XtFree((char *) plist);
		if  (!isnew)  {
			XmStringTable	strlist;
			char		*txt, *cp;
			XtVaGetValues(listw, XmNitems, &strlist, NULL);
			XmStringGetLtoR(strlist[whicline-1], XmSTRING_DEFAULT_CHARSET, &txt);
			if  (*txt == '\"')  {
				XtFree(txt);
				return;
			}
			cp = txt;
			if  (*cp == '<')  {
				iseat = 1;
				cp++;
			}
			if  (*cp == '>')  {
				isskip = 1;
				cp++;
			}
			while  (isspace(*cp))
				cp++;
			wotc = *cp++;
			while  (isspace(*cp))
				cp++;
			widsetting = 0;
			do  widsetting = widsetting * 10 + *cp++ - '0';
			while  (isdigit(*cp));
			if  (widsetting <= 0  ||  widsetting > 127)
				widsetting = 10;
			XtFree(txt);
		}
	}
	else  if  (!isnew)
		return;

	CreateEditDlg(w, "pfldedit", &ae_shell, &panew, &formw, 3);
	prevleft = XtVaCreateManagedWidget("width",
					   xmLabelGadgetClass,	formw,
					   XmNtopAttachment,	XmATTACH_FORM,
					   XmNleftAttachment,	XmATTACH_FORM,
					   NULL);
#ifdef HAVE_XM_SPINB_H
	prevleft = XtVaCreateManagedWidget("widsp",
					   xmSpinBoxWidgetClass,	formw,
					   XmNcolumns,			3,
					   XmNtopAttachment,		XmATTACH_FORM,
					   XmNleftAttachment,		XmATTACH_WIDGET,
					   XmNleftWidget,		prevleft,
					   NULL);

	sep_valw = XtVaCreateManagedWidget("wid",
					   xmTextFieldWidgetClass,	prevleft,
					   XmNcolumns,			3,
					   XmNeditable,			False,
					   XmNcursorPositionVisible,	False,
					   XmNspinBoxChildType,		XmNUMERIC,
					   XmNmaximumValue,		127,
					   XmNminimumValue,		1,
#ifdef	BROKEN_SPINBOX
					   XmNpositionType,		XmPOSITION_INDEX,
					   XmNposition,			widsetting-1,
#else
					   XmNposition,			widsetting,
#endif
					   NULL);
#else  /* ! HAVE_XM_SPINB_H */
	prevleft = sep_valw = XtVaCreateManagedWidget("wid",
						      xmTextFieldWidgetClass,		formw,
						      XmNmaxWidth,			3,
						      XmNcursorPositionVisible,		False,
						      XmNcolumns,			3,
						      XmNtopAttachment,			XmATTACH_FORM,
						      XmNleftAttachment,		XmATTACH_WIDGET,
						      XmNleftWidget,			prevleft,
						      NULL);
	prevleft = CreateArrowPair("wid",				formw,
				   (Widget) 0,				prevleft,
				   (XtCallbackProc) widup_cb,		(XtCallbackProc) widdn_cb,
				   1,					1,	1);

	sprintf(nbuf, "%3d", widsetting);
	XmTextSetString(sep_valw, nbuf);
#endif  /* ! HAVE_XM_SPINB_H */

	eatw = XtVaCreateManagedWidget("useleft",
				       xmToggleButtonGadgetClass,	formw,
				       XmNborderWidth,			0,
				       XmNtopAttachment,		XmATTACH_FORM,
				       XmNleftAttachment,		XmATTACH_WIDGET,
				       XmNleftWidget,			prevleft,
				       NULL);

	skipw = XtVaCreateManagedWidget("skipright",
				       xmToggleButtonGadgetClass,	formw,
				       XmNborderWidth,			0,
				       XmNtopAttachment,		XmATTACH_FORM,
				       XmNleftAttachment,		XmATTACH_WIDGET,
				       XmNleftWidget,			eatw,
				       NULL);

	if  (iseat)
		XmToggleButtonGadgetSetState(eatw, True, False);
	if  (isskip)
		XmToggleButtonGadgetSetState(skipw, True, False);

	fldrc = XtVaCreateManagedWidget("fldtype",
					xmRowColumnWidgetClass,		formw,
					XmNtopAttachment,		XmATTACH_WIDGET,
#ifdef HAVE_XM_SPINB_H
					XmNtopWidget,			prevleft,
#else
					XmNtopWidget,			sep_valw,
#endif
					XmNleftAttachment,		XmATTACH_FORM,
					XmNrightAttachment,		XmATTACH_FORM,
					XmNpacking,			XmPACK_COLUMN,
					XmNnumColumns,			2,
					XmNisHomogeneous,		True,
					XmNentryClass,			xmToggleButtonGadgetClass,
					XmNradioBehavior,		True,
					NULL);

	wfld = 255;
	for  (cnt = 0;  cnt < 26;  cnt++)  {
		Widget	wc;
		struct	formatdef  *fp = &lowertab[cnt];
		if  (fp->statecode == 0)
			continue;
		if  (!fp->explain)
			fp->explain = gprompt(fp->statecode + 500);
		sprintf(bigbuff, "%c: %s", cnt + 'a', fp->explain);
		wc = XtVaCreateManagedWidget(bigbuff,
					     xmToggleButtonGadgetClass,	fldrc,
					     XmNborderWidth,		0,
					     NULL);
		if  (cnt + 'a' == wotc)  {
			wfld = cnt + 'a';
			XmToggleButtonGadgetSetState(wc, True, False);
		}
		XtAddCallback(wc, XmNvalueChangedCallback, (XtCallbackProc) fld_turn, INT_TO_XTPOINTER(cnt + 'a'));
	}

	XtManageChild(formw);
	CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endnewedit, $H{xmspq view opts dlg});
}

static void  endnewsepedit(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		char		*txt;
		XmString	str;
		XtVaGetValues(sep_valw, XmNvalue, &txt, NULL);
		sprintf(bigbuff, "\"%s\"", txt[0] == '\0'? " ": txt);
		XtFree(txt);
		str = XmStringCreateLocalized(bigbuff);
		if  (isinsert)
			XmListAddItem(listw, str, whicline <= 0? 0: whicline);
		else
			XmListReplaceItemsPos(listw, &str, 1, whicline);
		XmStringFree(str);
	}
	XtDestroyWidget(GetTopShell(w));
}

static void  newseprout(Widget w, int isnew)
{
	Widget	ae_shell, panew, formw, prevleft;
	int	*plist, pcnt;
	char	*txt;

	whicline = -1;
	isinsert = isnew;

	if  (XmListGetSelectedPos(listw, &plist, &pcnt)  &&  pcnt > 0)  {
		whicline = plist[0];
		XtFree((char *) plist);
		if  (!isnew)  {
			XmStringTable	strlist;
			XtVaGetValues(listw, XmNitems, &strlist, NULL);
			XmStringGetLtoR(strlist[whicline-1], XmSTRING_DEFAULT_CHARSET, &txt);
			if  (*txt != '\"')  {
				XtFree(txt);
				return;
			}
		}
	}
	CreateEditDlg(w, "psepedit", &ae_shell, &panew, &formw, 3);
	prevleft = XtVaCreateManagedWidget("value",
					   xmLabelGadgetClass,	formw,
					   XmNtopAttachment,	XmATTACH_FORM,
					   XmNleftAttachment,	XmATTACH_FORM,
					   NULL);

	prevleft = sep_valw = XtVaCreateManagedWidget("val",
						  xmTextFieldWidgetClass,	formw,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNleftAttachment,		XmATTACH_WIDGET,
						  XmNleftWidget,		prevleft,
						  XmNrightAttachment,		XmATTACH_FORM,
						  NULL);

	if  (!isnew)  {
		char	*cp = txt;
		char	*lbp = bigbuff;
		if  (*cp == '\"')
			cp++;
		do  *lbp++ = *cp++;
		while  (*cp  &&  *cp != '\"'  &&  cp[1]);
		*lbp = '\0';
		XmTextSetString(sep_valw, bigbuff);
		XtFree(txt);
	}

	XtManageChild(formw);
	CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endnewsepedit, $H{xmspq new sep dlg});
}

static void  delrout()
{
	int	*plist, pcnt;

	if  (XmListGetSelectedPos(listw, &plist, &pcnt)  &&  pcnt > 0)  {
		int	which = plist[0];
		XtFree((char *) plist);
		XmListDeletePos(listw, which);
	}
}

static void  endpdisp(Widget w, int data)
{
	if  (data)  {		/* OK Pressed */
		XmStringTable	strlist;
		int	numstrs, cnt;
		char	*cp, *txt, *ip;
		XtVaGetValues(listw, XmNitems, &strlist, XmNitemCount, &numstrs, NULL);
		cp = bigbuff;
		for  (cnt = 0;  cnt < numstrs;  cnt++)  {
			XmStringGetLtoR(strlist[cnt], XmSTRING_DEFAULT_CHARSET, &txt);
			ip = txt;
			if  (*ip == '\"')  {	/* Separator */
				ip++;
				do  *cp++ = *ip++;
				while  (*ip  &&  *ip != '\"'  &&  ip[1]);
			}
			else  {
				int	wf;
				*cp++ = '%';
				if  (*ip == '<' || *ip == '>')
					*cp++ = *ip++;
				while  (isspace(*ip))
					ip++;
				wf = *ip;
				do  ip++;
				while  (isspace(*ip));
				while  (isdigit(*ip))
					*cp++ = *ip++;
				*cp++ = (char) wf;
			}
			XtFree(txt);
		}
		*cp = '\0';
		if  (ptr_format)
			free(ptr_format);
		ptr_format = stracpy(bigbuff);
		txt = get_ptrtitle();
		if  (ptitwid)  {
			XmString  str = XmStringCreateLocalized(txt);
			XtVaSetValues(ptitwid, XmNlabelString, str, NULL);
			XmStringFree(str);
		}
		free(txt);
		Ptr_seg.Last_ser = 0;
		pdisplay();
	}
	XtDestroyWidget(GetTopShell(w));
}

void  cb_setpdisplay(Widget parent)
{
	Widget	pd_shell, panew, pdispform, neww, editw, newsepw, editsepw, delw;
	Arg		args[6];
	int		n;

	CreateEditDlg(parent, "Vdisp", &pd_shell, &panew, &pdispform, 5);

	neww = XtVaCreateManagedWidget("Newfld",
				       xmPushButtonGadgetClass,	pdispform,
				       XmNshowAsDefault,	True,
				       XmNdefaultButtonShadowThickness,	1,
				       XmNtopOffset,		0,
				       XmNbottomOffset,		0,
				       XmNtopAttachment,	XmATTACH_FORM,
				       XmNleftAttachment,	XmATTACH_POSITION,
				       XmNleftPosition,		0,
				       NULL);

	editw = XtVaCreateManagedWidget("Editfld",
					xmPushButtonGadgetClass,	pdispform,
					XmNshowAsDefault,		False,
					XmNdefaultButtonShadowThickness,1,
					XmNtopOffset,			0,
					XmNbottomOffset,		0,
					XmNtopAttachment,		XmATTACH_FORM,
					XmNleftAttachment,		XmATTACH_POSITION,
					XmNleftPosition,		2,
					XmNtopAttachment,		XmATTACH_FORM,
					XmNleftAttachment,		XmATTACH_POSITION,
					XmNleftPosition,		2,
					NULL);

	newsepw = XtVaCreateManagedWidget("Newsep",
					  xmPushButtonGadgetClass,	pdispform,
					  XmNshowAsDefault,		False,
					  XmNdefaultButtonShadowThickness,	1,
					  XmNtopOffset,			0,
					  XmNbottomOffset,		0,
					  XmNtopAttachment,		XmATTACH_FORM,
					  XmNleftAttachment,		XmATTACH_POSITION,
					  XmNleftPosition,		4,
					  NULL);

	editsepw = XtVaCreateManagedWidget("Editsep",
					   xmPushButtonGadgetClass,		pdispform,
					   XmNshowAsDefault,			False,
					   XmNdefaultButtonShadowThickness,	1,
					   XmNtopOffset,			0,
					   XmNbottomOffset,			0,
					   XmNbottomOffset,			0,
					   XmNtopAttachment,			XmATTACH_FORM,
					   XmNleftAttachment,			XmATTACH_POSITION,
					   XmNleftPosition,			6,
					   NULL);

	delw = XtVaCreateManagedWidget("Delete",
				       xmPushButtonGadgetClass,	pdispform,
				       XmNshowAsDefault,		False,
				       XmNdefaultButtonShadowThickness,	1,
				       XmNtopOffset,			0,
				       XmNbottomOffset,			0,
				       XmNtopAttachment,		XmATTACH_FORM,
				       XmNleftAttachment,		XmATTACH_POSITION,
				       XmNleftPosition,			8,
				       NULL);

	XtAddCallback(neww, XmNactivateCallback, (XtCallbackProc) newrout, (XtPointer) 1);
	XtAddCallback(editw, XmNactivateCallback, (XtCallbackProc) newrout, (XtPointer) 0);
	XtAddCallback(newsepw, XmNactivateCallback, (XtCallbackProc) newseprout, (XtPointer) 1);
	XtAddCallback(editsepw, XmNactivateCallback, (XtCallbackProc) newseprout, (XtPointer) 0);
	XtAddCallback(delw, XmNactivateCallback, (XtCallbackProc) delrout, (XtPointer) 0);
	n = 0;
	XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT);	n++;
	XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, neww); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	listw = XmCreateScrolledList(pdispform, "Pdisplist", args, n);
	fillpdisplist();
	XtManageChild(listw);
	XtManageChild(pdispform);
	CreateActionEndDlg(pd_shell, panew, (XtCallbackProc) endpdisp, $H{xmspq new field dlg});
}

static	char	*f_name, *d_name, ishd;
extern	char	Confvarname[];
extern	char	*Helpfile_path;

static void  make_confline(FILE *fp, const char *vname)
{
	fprintf(fp, "%s=%s\n", vname, f_name);
}

static void  createnewhelp(FILE *ifl, FILE *ofl)
{
	int	ch, ch2, nn, hadj1 = 0, hadj2 = 0, hadp = 0, *wh = (int *) 0;
	char	*oline = (char *) 0;

	rewind(ifl);
	while  ((ch = getc(ifl)) != EOF)  {

		/* Ignore lines which don't start with a numeric string.  */

		if  (!isdigit(ch))  {
			while  (ch != '\n'  &&  ch != EOF)  {
				putc(ch, ofl);
				ch = getc(ifl);
			}
			if  (ch == EOF)
				break;
			putc(ch, ofl);
			continue;
		}

		/* Read in number, see if interesting */

		nn = 0;
		do  {
			nn = nn * 10 + ch - '0';
			ch = getc(ifl);
		}  while  (isdigit(ch));

		/* Check it interests us */

		if  (toupper(ch) != 'P'  ||
		     (nn != $P{xmspq job default fmt} &&
		      nn != $P{xmspq job default fmt}+1 &&
		      nn != $P{xmspq ptr default format}))  {
			fprintf(ofl, "%d%c", nn, ch);
		putrest:
			while  ((ch = getc(ifl)) != '\n'  &&  ch != EOF)
				putc(ch, ofl);
			if  (ch == EOF)
				break;
			putc(ch, ofl);
			continue;
		}

		/* Check terminated by colon */

		ch2 = getc(ifl);
		if  (ch2 != ':')  {
			fprintf(ofl, "%d%c%c", nn, ch, ch2);
			goto  putrest;
		}

		/* Discard rest of line */

		while  ((ch = getc(ifl)) != '\n'  &&  ch != EOF)
			;

		/* Splurge out replacement string */

		switch  (nn)  {
		case  $P{xmspq job default fmt}:
			oline = job_format;
			wh = &hadj1;
			break;
		case  $P{xmspq job default fmt}+1:
			oline = job_format;
			wh = &hadj2;
			break;
		case  $P{xmspq ptr default format}:
			oline = ptr_format;
			wh = &hadp;
			break;
		}
		if  (*wh)
			continue;
		fprintf(ofl, "%dP:%s\n", nn, oline);
		*wh = 1;
	}
	if  (hadj1 + hadj2 + hadp != 3)  {
		time_t	now = time((time_t *) 0);
		struct  tm  *tp = localtime(&now);
		fprintf(ofl, "\nNew formats %.2d/%.2d/%.2d %.2d:%.2d:%.2d\n\n",
			       tp->tm_year % 100, tp->tm_mon+1, tp->tm_mday,
			       tp->tm_hour, tp->tm_min, tp->tm_sec);
		if  (!hadj1)
			fprintf(ofl, "%dP:%s\n", $P{xmspq job default fmt}, job_format);
		if  (!hadj2)
			fprintf(ofl, "%dP:%s\n", $P{xmspq job default fmt}+1, job_format);
		if  (!hadp)
			fprintf(ofl, "%dP:%s\n", $P{xmspq ptr default format}, ptr_format);
	}
}

static void endmsaved(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
	if  (data)  {		/* OK selected */
		int	ret;
		FILE	*ofp;
		char	*resname;
		struct	stat	sbuf1, sbuf2;

		if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &f_name))
			return;

		disp_str = f_name;
		if  (stat(f_name, &sbuf1) >= 0)  { /* File must exist */
			if  ((sbuf1.st_mode & S_IFMT) != S_IFREG)  {
				doerror(w, $EH{spq help not flat file});
				XtFree(f_name);
				return;
			}

			/* Do not overwrite current help file */

			fstat(fileno(Cfile), &sbuf2);
			if  (sbuf1.st_dev == sbuf2.st_dev && sbuf1.st_ino == sbuf2.st_ino)  {
				doerror(w, $EH{spq help would overwrite});
				XtFree(f_name);
				return;
			}

			if  (!Confirm(w, $PH{Sure you want to overwrite}))  {
				XtFree(f_name);
				return;
			}
			SWAP_TO(Realuid);
			ofp = fopen(f_name, "w+");
			SWAP_TO(Daemuid);
			if  (!ofp)  {
				doerror(w, $EH{spq help file cannot open});
				XtFree(f_name);
				return;
			}
		}
		else  {
			SWAP_TO(Realuid);
			ofp = fopen(f_name, "w+");
			SWAP_TO(Daemuid);
			if  (!ofp)  {
				disp_str = f_name;
				doerror(w, $EH{spq help file cannot create});
				XtFree(f_name);
				return;
			}
		}
		createnewhelp(Cfile, ofp);
		if  ((ret = proc_save_opts(d_name, Confvarname, make_confline)))  {
			doerror(w, ret);
			XtFree(f_name);
			return;
		}
		if  (!(resname = malloc((unsigned) (strlen(d_name) + strlen(f_name) + 2))))
			nomem();
		sprintf(resname, "%s/%s", d_name, f_name);
		free(Helpfile_path);
		XtFree(f_name);
		Helpfile_path = resname;
		fclose(Cfile);
		Cfile = ofp;
		rewind(Cfile);
	}
	if  (ishd)
		free(d_name);
	XtDestroyWidget(w);
}

void  cb_saveformats(Widget parent, const int ish)
{
	Widget	msaved;
	if  ((ishd = (char) ish))
		d_name = envprocess("$HOME$LIBRARY");
	else
		d_name = Curr_pwd;
	chdir(d_name);
	msaved = XmCreateFileSelectionDialog(jwid, "msave", NULL, 0);
	XtAddCallback(msaved, XmNcancelCallback, (XtCallbackProc) endmsaved, (XtPointer) 0);
	XtAddCallback(msaved, XmNokCallback, (XtCallbackProc) endmsaved, (XtPointer) 1);
	XtAddCallback(msaved, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmspq save help file help});
	chdir(spdir);
	XtManageChild(msaved);
}
