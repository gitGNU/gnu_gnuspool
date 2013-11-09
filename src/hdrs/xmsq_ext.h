/* xmsq_ext.h -- xmspq (Motif) declarations

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

extern  char    scrkeep,        /* Try to keep moved job  */
                confabort;      /* 0 no confirm 1 confirm unprinted (default) 2 always */

extern  int     arr_rtime, arr_rint;    /* Arrow repeat intervals */

extern  unsigned  Pollinit,     /* Initial polling */
                  Pollfreq;     /* Current polling frequency */

extern  char    *Realuname,     /* My user name */
                *ptdir,         /* Printers directory, typically /usr/spool/printers */
                *spdir,         /* Spool directory, typically /usr/spool/spd */
                *Curr_pwd;      /* Directory on entry */

extern  struct  spdet  *mypriv; /* My class code/privileges */

extern  struct  spr_req jreq,   /* Job requests */
                        preq,   /* Printer requests */
                        oreq;   /* Other requests */
extern  struct  spq     JREQ;
extern  struct  spptr   PREQ;

#define JREQS   jreq.spr_un.j.spr_jslot
#define OREQ    oreq.spr_un.o.spr_jpslot
#define PREQS   preq.spr_un.p.spr_pslot

/* X stuff */

extern  XtAppContext    app;
extern  Display         *dpy;

extern  Widget  toplevel,       /* Main window */
                jtitwid,        /* Job title */
                ptitwid,        /* Ptr title */
                jwid,           /* Job scroll list */
                pwid;           /* Printer scroll list */

extern  XtIntervalId    Ptimeout,
                        arrow_timer;

extern  classcode_t     copyclasscode;

#define WORKW_UTXTW     0       /* User text window */
#define WORKW_FTXTW     1       /* Form text window */
#define WORKW_PTXTW     2       /* Printer text window */
#define WORKW_HTXTW     3       /* Header text window */
#define WORKW_PRITXTW   4       /* Priority text window */
#define WORKW_CPSTXTW   5       /* Priority text window */
#define WORKW_SUPPHBW   6       /* Suppress header button window */
#define WORKW_SORTPW    7       /* Sort printers button */
#define WORKW_SPTXTW    0       /* Start page text window */
#define WORKW_EPTXTW    1       /* End page text window */
#define WORKW_HAPTXTW   2       /* Halted at page text window */
#define WORKW_ALLPBTW   3       /* All pages button window */
#define WORKW_ODDPBTW   4       /* Odd pages button window */
#define WORKW_EVENPBTW  5       /* Even pages button window */
#define WORKW_SWOEPBTW  6       /* Swap Odd/Even pages button window */
#define WORKW_FLAGSPTXTW 7      /* Post processor flags text window */
#define WORKW_MAILW     1       /* Mail */
#define WORKW_WRITEW    2       /* Write */
#define WORKW_MATTNW    3       /* Mail attn */
#define WORKW_WATTNW    4       /* Write attn */
#define WORKW_RETQBW    0       /* Retain on queue */
#define WORKW_PDELTXTW  1       /* Delete time printed */
#define WORKW_NPDELTXTW 2       /* Delete time notprinted */
#define WORKW_HOLDBW    3       /* Set hold time */
#define WORKW_HOURTW    4       /* Hour */
#define WORKW_MINTW     5       /* Minute */
#define WORKW_DOWTW     6       /* Day of week */
#define WORKW_DOMTW     7       /* Day of month */
#define WORKW_MONTW     8       /* Month */
#define WORKW_YEARTW    9       /* Year */
#define WORKW_PRINTED   10      /* Printed */
#define WORKW_LOCO      10      /* Local only */
#define WORKW_DEVTXTW   5       /* Device */
#define WORKW_DEVNETW   6       /* Device is network filter */
#define WORKW_PMINW     7       /* Minimum printer size */
#define WORKW_PMAXW     8       /* Minimum printer size */
#define WORKW_PDESCRW   9       /* Comment */

#define WORKW_STXTW     4       /* Search text window */
#define WORKW_FORWW     5       /* Forward selection */
#define WORKW_MATCHW    6       /* Match exactly */
#define WORKW_WRAPW     7       /* Wrap around */
#define WORKW_JBUTW     8       /* Search for jobs */
#define WORKW_PBUTW     9       /* Search for ptrs */
#define WORKW_DBUTW     10      /* Search for device */
#define WORKW_JDIRW     7       /* Job unqueue directory */
#define WORKW_JCFW      8       /* Job command file */
#define WORKW_JJFW      9       /* Job job file */
#define WORKW_JCONLY    10      /* Job copy only */

extern  Widget  workw[];

extern void  CreateActionEndDlg(Widget, Widget, XtCallbackProc, int);
extern void  CreateEditDlg(Widget, char *, Widget *, Widget *, Widget *, const int);
extern void  InitsearchDlg(Widget, Widget *, Widget *, Widget *, char *);
extern void  InitsearchOpts(Widget, Widget, int, int, int);
extern void  cb_chelp(Widget, int, XmAnyCallbackStruct *);
extern void  cb_jact(Widget, int);
extern void  cb_jclass(Widget);
extern void  cb_jform(Widget);
extern void  cb_jpages(Widget);
extern void  cb_jretain(Widget);
extern void  cb_juser(Widget);
extern void  cb_macroj(Widget, int);
extern void  cb_macrop(Widget, int);
extern void  cb_onemore();
extern void  cb_pact(Widget, int);
extern void  cb_padd(Widget);
extern void  cb_pclass(Widget);
extern void  cb_pdev(Widget);
extern void  cb_pform();
extern void  cb_rsrch(Widget, int);
extern void  cb_saveformats(Widget, const int);
extern void  cb_srchfor(Widget);
extern void  cb_syserr(Widget);
extern void  cb_unqueue(Widget);
extern void  cb_view(Widget);
extern void  cb_viewopt(Widget, int);
extern void  cb_setjdisplay(Widget);
extern void  cb_setpdisplay(Widget);
extern void  doerror(Widget, int);
extern void  dohelp(Widget, int);
extern void  getdevsel(Widget);
extern void  getformsel(Widget, int);
extern void  getnewptrsel(Widget);
extern void  getptrsel(Widget, int);
extern void  jdisplay();
extern void  nomem();
extern void  openjfile();
extern void  openpfile();
extern void  pdisplay();
extern void  pollit(int, XtIntervalId);
extern void  pformhelp(Widget, int);
extern void  womsg(const int);
extern void  my_wjmsg(const int);
extern void  my_wpmsg(const int);

extern int  Confirm(Widget, int);
extern int  validatedev(char *);

extern char **makefvec(FILE *);
extern char **wotjform();
extern char **wotjprin();
extern char **wotpform();
extern char **wotpprin();
extern char **wottty();
extern char *get_jobtitle(int);
extern char *get_ptrtitle();

extern const struct spq *getselectedjob(const ULONG);

extern FILE *hexists(const char *dir, const char *d2);

extern Widget  CreateArrowPair(char*,Widget,Widget,Widget,XtCallbackProc,XtCallbackProc,int,int,int);
extern Widget  CreateCCDialog(Widget, Widget, classcode_t, Widget *);
#if defined(HAVE_XM_COMBOBOX_H)  &&  !defined(BROKEN_COMBOBOX)
extern Widget  CreateFselDialog(Widget, Widget, char *, char **);
extern Widget  CreatePselDialog(Widget, Widget, char *, char **, int, int);
#else
extern Widget  CreateFselDialog(Widget, Widget, char *, XtCallbackProc, int);
extern Widget  CreatePselDialog(Widget, Widget, char *, XtCallbackProc, int);
#endif
extern Widget  CreateJtitle(Widget);
extern Widget  CreateUselDialog(Widget, Widget, char *, int);
extern Widget  FindWidget(Widget);
extern Widget  GetTopShell(Widget);
