/* xmspu_ext.h -- xmspuser (Motif) declarations

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

extern  int     arr_rtime, arr_rint;    /* Arrow repeat intervals */

extern  int     hchanges,       /* Had changes to default */
                uchanges;       /* Had changes to user(s) */

extern  char            alphsort;
extern  struct  sphdr   Spuhdr;
extern  struct  spdet   *ulist;

/* X stuff */

extern  XtAppContext    app;
extern  Display         *dpy;

extern  Widget  toplevel,       /* Main window */
                dwid,           /* Default list */
                uwid;           /* User scroll list */

extern  Widget  workw[];

#define WORKW_SORTA     0
#define WORKW_MINPW     1
#define WORKW_DEFPW     2
#define WORKW_MAXPW     3
#define WORKW_CPSW      4
#define WORKW_ADMIN     5
#define WORKW_SSTOP     6
#define WORKW_FORMS     7
#define WORKW_OTHERP    8
#define WORKW_CPRIO     9
#define WORKW_OTHERJ    10
#define WORKW_PRINQ     11
#define WORKW_HALTGO    12
#define WORKW_ANYPRIO   13
#define WORKW_CDEFLT    14
#define WORKW_ADDDEL    15
#define WORKW_COVER     16
#define WORKW_UNQUEUE   17
#define WORKW_VOTHERJ   18
#define WORKW_REMOTEJ   19
#define WORKW_REMOTEP   20
#define WORKW_ACCESSOK  21
#define WORKW_FREEZEOK  22
#define WORKW_IMPW      1
#define WORKW_STXTW     0
#define WORKW_FORWW     1
#define WORKW_MATCHW    2
#define WORKW_WRAPW     3

#define FORMETC_DFORM   0
#define FORMETC_DFORMA  1
#define FORMETC_UFORM   2
#define FORMETC_UFORMA  3
#define FORMETC_DPTR    4
#define FORMETC_DPTRA   5
#define FORMETC_UPTR    6
#define FORMETC_UPTRA   7

extern void  cb_disporder(Widget);
extern void  cb_pris(Widget, int);
extern void  cb_formetc(Widget, int);
extern void  cb_class(Widget, int);
extern void  cb_priv(Widget, int);
extern void  cb_copydef(Widget, int);
extern void  cb_cdisplay(Widget, int);
extern void  cb_zeroc(Widget, int);
extern void  cb_impose(Widget, int);
extern void  cb_macrou(Widget, int);
extern void  cb_srchfor(Widget);
extern void  cb_rsrch(Widget, int);
extern void  cb_chelp(Widget, int, XmAnyCallbackStruct *);
extern void  defdisplay();
extern void  dohelp(Widget, int);
extern void  doerror(Widget, int);
extern void  displaybusy(const int);
extern void  udisplay(int, int *);

extern int  Confirm(Widget, int);

extern int  sort_id(struct spdet *, struct spdet *);
extern int  sort_u(struct spdet *, struct spdet *);
