/* xmsq_jlist.c -- xmspq display job list

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
#include "pages.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "xmsq_ext.h"
#include "displayopt.h"

#ifndef HAVE_XM_SPINB_H
extern void	widdn_cb(Widget, int, XmArrowButtonCallbackStruct *);
extern void	widup_cb(Widget, int, XmArrowButtonCallbackStruct *);
#endif
extern int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);

static	char	*localptr;

/* Open job file and get relevant messages.  */

void	openjfile(void)
{
	if  (!jobshminit(1))  {
		print_error($E{Cannot open jshm});
		exit(E_JOBQ);
	}
	localptr = gprompt($P{Spq local printer});
}

char	*job_format;
#define	DEFAULT_FORMAT	"%3n %<6N %6u %14h %16f%5Q%5S %3c %3p %14P"
static	char		*obuf, *bigbuff;
typedef	int	fmt_t;
#include "inline/jfmt_wattn.c"
#include "inline/jfmt_class.c"
#include "inline/jfmt_ppf.c"
#include "inline/jfmt_hold.c"
#include "inline/jfmt_sizek.c"
#include "inline/jfmt_krchd.c"
#include "inline/jfmt_jobno.c"
#include "inline/jfmt_oddev.c"
#include "inline/jfmt_ptr.c"
#include "inline/jfmt_pgrch.c"
#include "inline/jfmt_range.c"
#include "inline/jfmt_szpgs.c"
#include "inline/jfmt_nptim.c"
#include "inline/jfmt_user.c"
#include "inline/jfmt_puser.c"
#include "inline/jfmt_stime.c"
#include "inline/jfmt_mattn.c"
#include "inline/jfmt_cps.c"
#include "inline/jfmt_form.c"
#include "inline/jfmt_title.c"
#include "inline/jfmt_loco.c"
#include "inline/jfmt_mail.c"
#include "inline/jfmt_prio.c"
#include "inline/jfmt_retn.c"
#include "inline/jfmt_supph.c"
#include "inline/jfmt_ptime.c"
#include "inline/jfmt_write.c"
#include "inline/jfmt_origh.c"
#include "inline/jfmt_delim.c"
static	int	jcnt;
#include "inline/jfmt_seq.c"

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct	formatdef  {
	SHORT	statecode;	/* Code number for heading if applicable */
	SHORT	sugg_width;	/* Suggested width */
	char	*msg;		/* Heading */
	char	*explain;	/* More detailed explanation */
	int	(*fmt_fn)(const struct spq *, const int);
};

#define	NULLCP	(char *) 0

struct	formatdef
	uppertab[] = { /* A-Z */
	{	$P{job fmt title}+'A'-1,6,		NULLCP, NULLCP,	fmt_wattn	},	/* A */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* B */
	{	$P{job fmt title}+'C'-1,32,		NULLCP, NULLCP,	fmt_class	},	/* C */
	{	$P{job fmt title}+'D'-1,4,		NULLCP, NULLCP,	fmt_delim	},	/* D */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* E */
	{	$P{job fmt title}+'F'-1,MAXFLAGS-20,	NULLCP, NULLCP, fmt_ppflags	},	/* F */
	{	$P{job fmt title}+'G'-1,10,		NULLCP, NULLCP, fmt_hat		},	/* G */
	{	$P{job fmt title}+'H'-1,14,		NULLCP, NULLCP, fmt_hold	},	/* H */
	{	0,			0,		NULLCP, NULLCP, 0		},	/* I */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* J */
	{	$P{job fmt title}+'K'-1,6,		NULLCP, NULLCP,	fmt_sizek	},	/* K */
	{	$P{job fmt title}+'L'-1,6,		NULLCP, NULLCP, fmt_kreached	},	/* L */
	{	0,			0,		NULLCP, NULLCP, 0		},	/* M */
	{	$P{job fmt title}+'N'-1,6,		NULLCP, NULLCP, fmt_jobno	},	/* N */
	{	$P{job fmt title}+'O'-1,6,		NULLCP, NULLCP,	fmt_oddeven	},	/* O */
	{	$P{job fmt title}+'P'-1,JPTRNAMESIZE-4,	NULLCP, NULLCP, fmt_printer	},	/* P */
	{	$P{job fmt title}+'Q'-1,6,		NULLCP, NULLCP,	fmt_pgreached	},	/* Q */
	{	$P{job fmt title}+'R'-1,10,		NULLCP, NULLCP, fmt_range	},	/* R */
	{	$P{job fmt title}+'S'-1,6,		NULLCP, NULLCP, fmt_szpages	},	/* S */
	{	$P{job fmt title}+'T'-1,6,		NULLCP, NULLCP, fmt_nptime	},	/* T */
	{	$P{job fmt title}+'U'-1,UIDSIZE-2,	NULLCP, NULLCP, fmt_puser	},	/* U */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* V */
	{	$P{job fmt title}+'W'-1,14,		NULLCP, NULLCP,	fmt_stime	},	/* W */
	{	0,			0,		NULLCP, NULLCP, 0		},	/* X */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* Y */
	{	0,			0,		NULLCP, NULLCP,	0		}	/* Z */
},
	lowertab[] = { /* a-z */
	{	$P{job fmt title}+'a'-1,6,		NULLCP, NULLCP,	fmt_mattn	},	/* a */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* b */
	{	$P{job fmt title}+'c'-1,3,		NULLCP, NULLCP,	fmt_cps		},	/* c */
	{	$P{job fmt title}+'d'-1,2,		NULLCP, NULLCP,	fmt_delimnum	},	/* d */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* e */
	{	$P{job fmt title}+'f'-1,MAXFORM-4,	NULLCP, NULLCP, fmt_form	},	/* f */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* g */
	{	$P{job fmt title}+'h'-1,MAXTITLE-6,	NULLCP, NULLCP, fmt_title	},	/* h */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* i */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* j */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* k */
	{	$P{job fmt title}+'l'-1,6,		NULLCP, NULLCP,	fmt_localonly	},	/* l */
	{	$P{job fmt title}+'m'-1,6,		NULLCP, NULLCP,	fmt_mail	},	/* m */
	{	$P{job fmt title}+'n'-1,3,		NULLCP, NULLCP,	fmt_seq		},	/* n */
	{	$P{job fmt title}+'o'-1,HOSTNSIZE-6,	NULLCP, NULLCP, fmt_orighost	},	/* o */
	{	$P{job fmt title}+'p'-1,3,		NULLCP, NULLCP, fmt_prio	},	/* p */
	{	$P{job fmt title}+'q'-1,6,		NULLCP, NULLCP,	fmt_retain	},	/* q */
	{	0,			0,		NULLCP, NULLCP, 0		},	/* r */
	{	$P{job fmt title}+'s'-1,6,		NULLCP, NULLCP, fmt_supph	},	/* s */
	{	$P{job fmt title}+'t'-1,6,		NULLCP, NULLCP, fmt_ptime	},	/* t */
	{	$P{job fmt title}+'u'-1,UIDSIZE-2,	NULLCP, NULLCP,	fmt_user	},	/* u */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* v */
	{	$P{job fmt title}+'w'-1,6,		NULLCP, NULLCP,	fmt_write	},	/* w */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* x */
	{	0,			0,		NULLCP, NULLCP,	0		},	/* y */
	{	0,			0,		NULLCP, NULLCP,	0		}	/* z */
};

char *	get_jobtitle(int nopage)
{
	int	nn, obuflen;
	struct	formatdef	*fp;
	char	*cp, *rp, *result, *mp;

	if  (!job_format  &&  !(job_format = helpprmpt($P{xmspq job default fmt}+nopage)))
		job_format = stracpy(DEFAULT_FORMAT);

	/* Initial pass to discover how much space to allocate */

	obuflen = 1;
	cp = job_format;
	while  (*cp)  {
		if  (*cp++ != '%')  {
			obuflen++;
			continue;
		}
		if  (*cp == '<')
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
	cp = job_format;
	while  (*cp)  {
		if  (*cp != '%')  {
			*rp++ = *cp++;
			continue;
		}
		cp++;

		/* Get width */

		if  (*cp == '<')
			cp++;
		nn = 0;
		do  nn = nn * 10 + *cp++ - '0';
		while  (isdigit(*cp));

		/* Get format char */

		if  (isupper(*cp))
			fp = &uppertab[*cp - 'A'];
		else  if  (islower(*cp))
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

/* Display contents of job file.  */

void	jdisplay(void)
{
	int	topj = 1, cjobpos = -1, newpos = -1, jlines;
	jobno_t		Cjobno = -1;
	netid_t		Chostno = 0;
	XmString	*elist;

	if  (Job_seg.njobs != 0)  {
		int	*plist, pcnt;

		/* First discover the currently-selected job and the
		   scroll position, if any.  */

		XtVaGetValues(jwid,
			      XmNtopItemPosition,	&topj,
			      XmNvisibleItemCount,	&jlines,
			      NULL);

		if  (XmListGetSelectedPos(jwid, &plist, &pcnt))  {
			cjobpos = plist[0] - 1;
			XtFree((char *) plist);
			if  ((unsigned) cjobpos < Job_seg.njobs  &&
			     Job_seg.jj_ptrs[cjobpos]->j.spq_job != 0)  {
				Cjobno = Job_seg.jj_ptrs[cjobpos]->j.spq_job;
				Chostno = Job_seg.jj_ptrs[cjobpos]->j.spq_netid;
			}
		}
		XtVaGetValues(jwid, XmNitems, &elist, NULL);
	}

	XmListDeleteAllItems(jwid);
	readjoblist(1);

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		XmString	str;
		const  struct  spq  *jp = &Job_seg.jj_ptrs[jcnt]->j;
		struct	formatdef  *fp;
		char		*cp = job_format, *rp = obuf, *lbp;
		int		currplace = -1, lastplace, nn, inlen;

		if  (jp->spq_job == Cjobno  &&  jp->spq_netid == Chostno)
			newpos = jcnt;

		while  (*cp)  {
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
			nn = 0;
			do  nn = nn * 10 + *cp++ - '0';
			while  (isdigit(*cp));

			/* Get format char */

			if  (isupper(*cp))
				fp = &uppertab[*cp - 'A'];
			else  if  (islower(*cp))
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
			inlen = (fp->fmt_fn)(jp, nn);
			lbp = bigbuff;
			if  (inlen > nn  &&  lastplace >= 0)  {
				rp = &obuf[lastplace];
				nn = currplace + nn - lastplace;
				inlen = (fp->fmt_fn)(jp, nn);
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

		str = XmStringCreateLocalized(obuf);
		XmListAddItem(jwid, str, 0);
		XmStringFree(str);
	}

	/* Adjust scrolling as requested, either by following job
	   (default), or by keeping the same amount scrolled as
	   we had before (scrkeep).  */

	if  (!scrkeep)  {
		if  (newpos >= 0)  {
			topj += newpos - cjobpos;
			if  (topj <= 0)
				topj = 1;
		}
	}
	if  (topj+jlines > Job_seg.njobs) /* Shrunk */
		XmListSetBottomPos(jwid, (int) Job_seg.njobs);
	else
		XmListSetPos(jwid, topj);

	if  (newpos >= 0)	/* Only gets set if we had one */
		XmListSelectPos(jwid, newpos+1, False);
}

extern	Widget	sep_valw;
static	Widget	listw, eatw;
static	int	whicline;
static	unsigned	char	wfld, isinsert;

static void	filljdisplist(void)
{
	char	*cp, *lbp;
	int		nn;
	XmString	str;
	struct	formatdef	*fp;

	cp = job_format;
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
			if  (*cp == '<')
				*lbp = *cp++;
			lbp++;
			*lbp++ = ' ';
			nn = 0;
			do  nn = nn * 10 + *cp++ - '0';
			while  (isdigit(*cp));
			if  (isupper(*cp))
				fp = &uppertab[*cp - 'A'];
			else  if  (islower(*cp))
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

static void	fld_turn(Widget w, int n, XmToggleButtonCallbackStruct * cbs)
{
	if  (cbs->set)  {
		struct	formatdef  *fp;
		wfld = (unsigned char) n;
		if  (isupper(n))
			fp = &uppertab[n - 'A'];
		else
			fp = &lowertab[n - 'a'];
#ifdef	HAVE_XM_SPINB_H
#ifdef	BROKEN_SPINBOX
		if  (fp->fmt_fn)
			XtVaSetValues(sep_valw, XmNposition, fp->sugg_width-1, NULL);
#else
		if  (fp->fmt_fn)
			XtVaSetValues(sep_valw, XmNposition, fp->sugg_width, NULL);
#endif
#else  /* ! HAVE_XM_SPINB_H */
		if  (fp->fmt_fn)  {
			char	nbuf[6];
			sprintf(nbuf, "%3d", fp->sugg_width);
			XmTextSetString(sep_valw, nbuf);
		}
#endif
	}
}

static void	endnewedit(Widget w, int data)
{
	if  (data)  {		/* OK pressed */
		char		*lbp;
		int		nn;
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
#else /* ! HAVE_XM_SPINB_H */
		XtVaGetValues(sep_valw, XmNvalue, &txt, NULL);
		nn = atoi(txt);
		XtFree(txt);
		if  (nn <= 0)
			return;
#endif /* ! HAVE_XM_SPINB_H */
		lbp = bigbuff;
		*lbp++ = XmToggleButtonGadgetGetState(eatw)? '<': ' ';
		sprintf(lbp, " %c %3d ", (char) wfld, nn);
		lbp += 7;
		strcpy(lbp, isupper(wfld)? uppertab[wfld - 'A'].explain: lowertab[wfld - 'a'].explain);
		str = XmStringCreateLocalized(bigbuff);
		if  (isinsert)
			XmListAddItem(listw, str, whicline <= 0? 0: whicline);
		else
			XmListReplaceItemsPos(listw, &str, 1, whicline);
		XmStringFree(str);
	}
	XtDestroyWidget(GetTopShell(w));
}

static void	newrout(Widget w, int isnew)
{
	Widget	ae_shell, panew, formw, prevleft, fldrc;
	int	*plist, cnt, wotc = 255, widsetting = 10, iseat = 0;
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
			if  (*cp++ == '<')  {
				iseat = 1;
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

	CreateEditDlg(w, "jfldedit", &ae_shell, &panew, &formw, 3);
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
#else /* ! HAVE_XM_SPINB_H */
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
				   1,					1,
				   1);

	sprintf(nbuf, "%3d", widsetting);
	XmTextSetString(sep_valw, nbuf);
#endif /* !HAVE_XM_SPINB_H */

	eatw = XtVaCreateManagedWidget("useleft",
				       xmToggleButtonGadgetClass,	formw,
				       XmNborderWidth,			0,
				       XmNtopAttachment,		XmATTACH_FORM,
				       XmNleftAttachment,		XmATTACH_WIDGET,
				       XmNleftWidget,			prevleft,
				       NULL);

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
					XmNnumColumns,			3,
					XmNisHomogeneous,		True,
					XmNentryClass,			xmToggleButtonGadgetClass,
					XmNradioBehavior,		True,
					NULL);

	if  (iseat)
		XmToggleButtonGadgetSetState(eatw, True, False);

	wfld = 255;
	for  (cnt = 0;  cnt < 26;  cnt++)  {
		Widget	wc;
		struct	formatdef  *fp = &uppertab[cnt];
		if  (fp->statecode == 0)
			continue;
		if  (!fp->explain)
			fp->explain = gprompt(fp->statecode + 500);
		sprintf(bigbuff, "%c: %s", cnt + 'A', fp->explain);
		wc = XtVaCreateManagedWidget(bigbuff,
					     xmToggleButtonGadgetClass,	fldrc,
					     XmNborderWidth,		0,
					     NULL);
		if  (cnt + 'A' == wotc)  {
			wfld = cnt + 'A';
			XmToggleButtonGadgetSetState(wc, True, False);
		}
		XtAddCallback(wc, XmNvalueChangedCallback, (XtCallbackProc) fld_turn, INT_TO_XTPOINTER(cnt + 'A'));
	}

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
	CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endnewedit, $H{new job field dlg});
}

static void	endnewsepedit(Widget w, int data)
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

static void	newseprout(Widget w, int isnew)
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
	CreateEditDlg(w, "jsepedit", &ae_shell, &panew, &formw, 3);
	prevleft = XtVaCreateManagedWidget("value",
					   xmLabelGadgetClass,	formw,
					   XmNtopAttachment,	XmATTACH_FORM,
					   XmNleftAttachment,	XmATTACH_FORM,
					   NULL);

	prevleft = sep_valw = XtVaCreateManagedWidget("val",
						      xmTextFieldWidgetClass,	formw,
						      XmNtopAttachment,		XmATTACH_FORM,
						      XmNleftAttachment,	XmATTACH_WIDGET,
						      XmNleftWidget,		prevleft,
						      XmNrightAttachment,	XmATTACH_FORM,
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
	CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endnewsepedit, $H{new job sep dlg});
}

static void	delrout(void)
{
	int	*plist, pcnt;

	if  (XmListGetSelectedPos(listw, &plist, &pcnt)  &&  pcnt > 0)  {
		int	which = plist[0];
		XtFree((char *) plist);
		XmListDeletePos(listw, which);
	}
}

static void	endjdisp(Widget w, int data)
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
				if  (*ip == '<')
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
		if  (job_format)
			free(job_format);
		job_format = stracpy(bigbuff);
		txt = get_jobtitle(0);
		if  (jtitwid)  {
			XmString  str = XmStringCreateLocalized(txt);
			XtVaSetValues(jtitwid, XmNlabelString, str, NULL);
			XmStringFree(str);
		}
		free(txt);
		Job_seg.Last_ser = 0;
		jdisplay();
	}
	XtDestroyWidget(GetTopShell(w));
}

void	cb_setjdisplay(Widget parent)
{
	Widget	jd_shell, panew, jdispform, neww, editw, newsepw, editsepw, delw;
	Arg		args[6];
	int		n;

	CreateEditDlg(parent, "Jdisp", &jd_shell, &panew, &jdispform, 5);
	neww = XtVaCreateManagedWidget("Newfld",
				       xmPushButtonGadgetClass,	jdispform,
				       XmNshowAsDefault,	True,
				       XmNdefaultButtonShadowThickness,	1,
				       XmNtopOffset,		0,
				       XmNbottomOffset,		0,
				       XmNtopAttachment,	XmATTACH_FORM,
				       XmNleftAttachment,	XmATTACH_POSITION,
				       XmNleftPosition,		0,
				       NULL);

	editw = XtVaCreateManagedWidget("Editfld",
					xmPushButtonGadgetClass,	jdispform,
					XmNshowAsDefault,		False,
					XmNdefaultButtonShadowThickness,1,
					XmNtopOffset,			0,
					XmNbottomOffset,		0,
					XmNtopAttachment,		XmATTACH_FORM,
					XmNleftAttachment,		XmATTACH_POSITION,
					XmNleftPosition,		2,
					NULL);

	newsepw = XtVaCreateManagedWidget("Newsep",
					  xmPushButtonGadgetClass,	jdispform,
					  XmNshowAsDefault,		False,
					  XmNdefaultButtonShadowThickness,	1,
					  XmNtopOffset,			0,
					  XmNbottomOffset,		0,
					  XmNtopAttachment,		XmATTACH_FORM,
					  XmNleftAttachment,		XmATTACH_POSITION,
					  XmNleftPosition,		4,
					  NULL);

	editsepw = XtVaCreateManagedWidget("Editsep",
					   xmPushButtonGadgetClass,		jdispform,
					   XmNshowAsDefault,			False,
					   XmNdefaultButtonShadowThickness,	1,
					   XmNtopOffset,			0,
					   XmNbottomOffset,			0,
					   XmNtopAttachment,			XmATTACH_FORM,
					   XmNleftAttachment,			XmATTACH_POSITION,
					   XmNleftPosition,			6,
					   NULL);

	delw = XtVaCreateManagedWidget("Delete",
				       xmPushButtonGadgetClass,	jdispform,
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
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNtopWidget, neww); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	/*XtSetArg(args[n], XmNvisibleItemCount, 10); n++;*/
	listw = XmCreateScrolledList(jdispform, "Jdisplist", args, n);
	filljdisplist();
	XtManageChild(listw);
	XtManageChild(jdispform);
	CreateActionEndDlg(jd_shell, panew, (XtCallbackProc) endjdisp, $H{xmspq job display dlg});
}
