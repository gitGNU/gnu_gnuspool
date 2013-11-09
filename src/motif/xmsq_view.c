/* xmsq_view.c -- xmspq view files etc

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#undef  CONST                   /* Because CONST is redefined by some peoples' Intrinsic.h */
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include <Xm/SeparatoGP.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include "config.h"             /* Because CONST is undefed by some people's Intrinsic.h */
#include "incl_sig.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xmsq_ext.h"
#include "xmmenu.h"

#include "xmspqdoc.bm"

extern int  rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
extern FILE *  net_feed(const int, const netid_t, const slotno_t, const jobno_t);

#define INCPAGES        10      /* Incremental size page offsets vector */

#define my_max(a,b) ((int)(a)>(int)(b)?(int)(a):(int)(b))
#define my_min(a,b) ((int)(a)<(int)(b)?(int)(a):(int)(b))

static  char    *startpmsg,
                *endpmsg,
                *hatpmsg,
                *buffer;

static  char    notformfeed, had_first, inprogress;
static  char    matchcase, sbackward, wraparound;

static  int     longest_msg;

static  unsigned        nphys,          /* Number of "physical" pages */
                        linecount,      /* Number of lines in document */
                        pagewidth;      /* In chars */

static  ULONG   linepixwidth;   /* In pixels */

static  unsigned  *pagecounts;  /* Line counts (0-based) at the start of each page  */

static  LONG    *physpages,     /* "Physical" pages in file */
                top_page,
                fileend;

static  Widget  spw,            /* Boxes for "started page" etc */
                epw,
                hpw,
                scrolled_w,
                drawing_a,      /* Drawing area */
                vsb,            /* Scroll bars */
                hsb;

static  GC      mygc;           /* Graphics context for writes */

static  Pixmap          docbitmap;

static  XFontStruct     *font;

static  Dimension       view_width,
                        view_height;

static  unsigned        cell_width,
                        cell_height,
                        pix_hoffset,
                        pix_voffset;

static  unsigned        last_top,
                        last_left;

static  FILE            *infile;

static  unsigned        last_matchline;
static  char            *matchtext;

static void  cb_vsrchfor(Widget);
static void  cb_vrsrch(Widget, int);
static void  cb_vspage(Widget, int);
static void  endview(Widget, int);
static void  clearlog(Widget);

static  casc_button
job_casc[] = {
        {       ITEM,   "Update",       endview,        1       },
        {       ITEM,   "Exit",         endview,        0       }},
search_casc[] = {
        {       ITEM,   "Search",       cb_vsrchfor,    0       },
        {       ITEM,   "Searchforw",   cb_vrsrch,      0       },
        {       ITEM,   "Searchback",   cb_vrsrch,      1       }},
page_casc[] = {
        {       ITEM,   "Setstart",     cb_vspage,      0       },
        {       ITEM,   "Setend",       cb_vspage,      1       },
        {       ITEM,   "Sethat",       cb_vspage,      2       },
        {       ITEM,   "Reset",        cb_vspage,      3       }},
helpv_casc[] = {
        {       ITEM,   "Help",         dohelp,         2       },
        {       ITEM,   "Helpon",       cb_chelp,       0       }},
helpse_casc[] = {
        {       ITEM,   "Help",         dohelp,         3       },
        {       ITEM,   "Helpon",       cb_chelp,       0       }},
file_casc[] = {
        {       ITEM,   "Clearlog",     clearlog,       0       },
        {       ITEM,   "Exit",         endview,        0       }};

static  pull_button
        job_button =
                {       "Job",          XtNumber(job_casc),     $H{View job menu},      job_casc        },
        srch_button =
                {       "Srch",         XtNumber(search_casc),  $H{View search menu},   search_casc     },
        page_button =
                {       "Page",         XtNumber(page_casc),    $H{View page menu},     page_casc       },
        helpv_button =
                {       "Help",         XtNumber(helpv_casc),   $H{View help menu},     helpv_casc,     1       },
        file_button =
                {       "File",         XtNumber(file_casc),    $H{Log file menu},      file_casc       },
        helpse_button =
                {       "Help",         XtNumber(helpse_casc),  $H{Log help menu},      helpse_casc,    1       };

static  pull_button     *viewmenlist[] = { &job_button, &srch_button, &page_button, &helpv_button, NULL },
                        *syserrmenlist[] = { &file_button, &helpse_button, NULL };

static void  setup_menus(Widget panew, pull_button **menlist)
{
        XtWidgetGeometry        size;
        Widget                  helpw, result;
        pull_button             **item;

        result = XmCreateMenuBar(panew, "viewmenu", NULL, 0);

        /* Get rid of resize button for menubar */

        size.request_mode = CWHeight;
        XtQueryGeometry(result, NULL, &size);
        XtVaSetValues(result, XmNpaneMaximum, size.height*2, XmNpaneMinimum, size.height*2, NULL);

        /* Use the BuildPulldown in xmspq.c */

        for  (item = menlist;  *item;  item++)
                if  ((helpw = BuildPulldown(result, *item)))
                        XtVaSetValues(result, XmNmenuHelpWidget, helpw, NULL);
        XtManageChild(result);
}

/* Read file to find where all the pages start.  */

static int  scanfile(FILE *fp)
{
        int     curline, ppage, ch;

        /* nphys and physpages still got allocated in rdpgfile
           They may not be enough though. */

        if  (!(pagecounts = (unsigned *) malloc((nphys + 2) * sizeof(unsigned))))  {
                free((char *) physpages);
                return  -1;
        }

        ppage = 1;
        pagewidth = 0;
        linecount = 0;
        physpages[0] = 0L;
        pagecounts[0] = 0;
        curline = 0;

        while  ((ch = getc(fp)) != EOF)  {
                switch  (ch)  {
                default:
                        if  (ch & 0x80)  {
                                ch &= 0x7f;
                                curline += 2;
                        }
                        if  (ch < ' ')
                                curline++;
                        curline++;
                        break;
                case  '\f':
                        if  (ppage >= nphys)  {
                                nphys += INCPAGES;
                                physpages = (LONG *) realloc((char *) physpages,
                                                             (unsigned) (nphys + 2) * sizeof(LONG));
                                pagecounts = (unsigned *) realloc((char *) pagecounts,
                                                             (unsigned) (nphys + 2) * sizeof(unsigned));
                                if  (!(physpages && pagecounts))
                                        return  -1;
                        }
                        physpages[ppage] = ftell(fp) - 1;
                        pagecounts[ppage] = linecount + 1;      /* So end of page counts as line */
                        ppage++;
                case  '\n':
                        if  (curline > pagewidth)
                                pagewidth = curline;
                        curline = 0;
                        linecount++;
                        break;
                case  '\t':
                        curline = (curline + 8) & ~7;
                        break;
                }
        }
        fileend = ftell(fp);
        if  (fileend > physpages[ppage-1])  {
                physpages[ppage] = fileend;
                pagecounts[ppage] = linecount + 1;
        }
        nphys = ppage + 1;
        return  1;
}

static int  readpgfile(FILE *fp, const struct spq *jp)
{
        int     ch, curline, ppage;
        LONG    char_count;
        char    *delim;
        struct  pages   pfe;

        nphys = 0;

        if  ((ch = rdpgfile(jp, &pfe, &delim, &nphys, &physpages)) <= 0)
                return  ch;

        /* Don't need this...  */

        free(delim);

        if  (!(pagecounts = (unsigned *) malloc((nphys + 2) * sizeof(unsigned))))  {
                free((char *) physpages);
                return  -1;
        }

        ppage = 1;
        pagewidth = 0;
        linecount = 0;
        curline = 0;
        physpages[0] = 0L;
        pagecounts[0] = 0;
        char_count = 0L;

        for  (;;)  {
                if  ((ch = getc(fp)) == EOF)  {
                        fileend = char_count;
                        pagecounts[ppage] = linecount + 1;
                        nphys = ppage;
                        return  1;
                }
                if  (++char_count >= physpages[ppage])  {
                        pagecounts[ppage] = 1 + ++linecount;
                        if  (curline > pagewidth)
                                pagewidth = curline;
                        curline = 0;
                        ppage++;
                }
                if  (ch == '\t')  {
                        curline = (curline + 8) & ~7;
                        continue;
                }
                if  (ch == '\n')  {
                        linecount++;
                        if  (curline > pagewidth)
                                pagewidth = curline;
                        curline = 0;
                }
                if  (ch & 0x80)  {
                        ch &= 0x7f;
                        curline += 2;
                }
                if  (ch < ' ')
                        curline++;
                curline++;
        }
}

static int      my_getline(LONG upto)
{
        int     lng = 0, ch;

        if  (ftell(infile) == upto)
                return  0;

        while  ((ch = getc(infile)) != '\n')  {
                if  (ch == EOF)
                        return  -1;
                if  (ch == '\t')  {
                        do  buffer[lng++] = ' ';
                        while  ((lng & 7) != 0);
                }
                else  if  (ch != '\f' || notformfeed)  {
                        if  (ch & 0x80)  {
                                ch &= 0x7f;
                                buffer[lng++] = 'M';
                                buffer[lng++] = '-';
                        }
                        if  (ch < ' ')  {
                                buffer[lng++] = '^';
                                ch += ' ';
                        }
                        buffer[lng++] = (char) ch;
                }
                if  (ftell(infile) == upto)
                        break;
        }
        buffer[lng] = '\0';
        return  lng + 1;
}

static int  initpage(int nline, LONG *pagec)
{
        int     wpage, cline, ch;

        if  (nline >= linecount)
                return  -1;
        for  (wpage = nphys-1;  wpage > 0;  wpage--)
                if  (nline >= pagecounts[wpage])
                        break;
        *pagec = wpage;
        if  (nline == pagecounts[wpage+1] - 1)  {
                fseek(infile, (long) physpages[wpage+1], 0);
                return  0;
        }
        cline = pagecounts[wpage];
        fseek(infile, (long) physpages[wpage], 0);

        /* Find beginning of line we wanted */

        while  (cline < nline)  {
                if  ((ch = getc(infile)) == EOF) /* Safety */
                        return  -1;
                if  (ch == '\n')
                        cline++;
        }
        return  my_getline(physpages[wpage+1]);
}

static void  redraw(Boolean doclear)
{
        int     yloc, yindx, xindx, lng, endy, lstart;
        LONG    pageno, endofpage;

        /* AIX Motif takes a long time to allocate a window */

        if  (!XtWindow(drawing_a))
                return;

        lstart = yindx = pix_voffset / cell_height;
        xindx = pix_hoffset / cell_width;

        yloc = 0;
        endy = view_height;

        if  (!doclear  &&  last_left == pix_hoffset  &&  last_top == pix_voffset)
                return; /* Nothing to do */

        lng = initpage(lstart, &pageno);
        if  (pageno != top_page)  {
                char    nbuf[12];
                sprintf(nbuf, "%10ld", pageno + 1L);
                XmTextSetString(workw[WORKW_PTXTW], nbuf);
                XmTextSetString(spw, "");
                XmTextSetString(epw, "");
                XmTextSetString(hpw, "");
                if  (pageno == JREQ.spq_start)
                        XmTextSetString(spw, startpmsg);
                if  (pageno == JREQ.spq_end)
                        XmTextSetString(epw, endpmsg);
                if  (pageno == JREQ.spq_haltat  &&  pageno > 0)
                        XmTextSetString(hpw, hatpmsg);
                top_page = pageno;
        }

        endofpage = physpages[pageno+1];
        while  (lstart < yindx)  {
                if  (lng == 0)  {
                        pageno++;
                        endofpage = physpages[pageno+1];
                }
                lng = my_getline(endofpage);
                lstart++;
        }
        XClearArea(dpy, XtWindow(drawing_a), 0, yloc, view_width, endy, False);

        while  (lng >= 0  &&  yloc < endy)  {
                yloc += font->ascent;
                if  (--lng < 0)  {
                        XDrawLine(dpy, XtWindow(drawing_a), mygc, 0, yloc - cell_height/2, view_width, yloc - cell_height/2);
                        pageno++;
                        endofpage = physpages[pageno+1];
                }
                else  if  (lng > xindx)  {
                        lng -= xindx;
                        XDrawImageString(dpy, XtWindow(drawing_a), mygc, 0, yloc, buffer + xindx, lng);
                }
                lng = my_getline(endofpage);
                yloc += font->descent;
        }
        last_top = pix_voffset;
        last_left = pix_hoffset;
}

static void  se_redraw(Boolean doclear)
{
        int     yloc, yindx, xindx, lng, endy, lcount;

        /* AIX Motif takes a long time to allocate a window */

        if  (!XtWindow(drawing_a))
                return;

        yindx = pix_voffset / cell_height;
        xindx = pix_hoffset / cell_width;

        yloc = 0;
        endy = view_height;

        if  (doclear)
                goto  do_all;
        if  (last_left != pix_hoffset)
                goto  do_all;

        if  (last_top == pix_voffset)
                return; /* Nothing to do */

        if  (last_top < pix_voffset)  { /* Moved down file - move up unchanged bit */
                unsigned  delta = pix_voffset - last_top;
                unsigned  newyloc;
                if  (delta > view_height)
                        goto  do_all;
                newyloc = view_height - delta;

                XCopyArea(dpy, XtWindow(drawing_a), XtWindow(drawing_a), mygc, 0, delta, view_width, newyloc, 0, 0);

                /* Adjust position of last line in case window height
                   was such it got only half-drawn */

                yloc = newyloc - newyloc % cell_height;
                yindx += yloc / cell_height;
        }
        else  {
                unsigned  delta = last_top - pix_voffset;
                if  (delta > view_height)
                        goto  do_all;
                XCopyArea(dpy, XtWindow(drawing_a), XtWindow(drawing_a), mygc, 0, 0, view_width, view_height - delta, 0, delta);
                endy = delta;
        }

 do_all:

        lcount = 0;
        fseek(infile, 0L, 0);
        while  (lcount < yindx)  {
                int     ch;
                do  {
                        ch = getc(infile);
                        if  (ch == EOF)
                                goto  failed;
                }  while  (ch != '\n');
                lcount++;
        }
 failed:
        XClearArea(dpy, XtWindow(drawing_a), 0, yloc, view_width, endy, False);
        lng = my_getline(0x7fffffff);
        while  (lng >= 0  &&  yloc < endy)  {
                yloc += font->ascent;
                if  (--lng < 0)
                        XDrawLine(dpy, XtWindow(drawing_a), mygc, 0, yloc - cell_height/2, view_width, yloc - cell_height/2);
                else  if  (lng > xindx)  {
                        lng -= xindx;
                        XDrawImageString(dpy, XtWindow(drawing_a), mygc, 0, yloc, buffer + xindx, lng);
                }
                lng = my_getline(0x7fffffff);
                yloc += font->descent;
        }
        last_top = pix_voffset;
        last_left = pix_hoffset;
}

/* React to scrolling actions.  Reset position of Scrollbars; call
   redraw() to do actual scrolling.  cbs->value is Scrollbar's
   new position.  */

static void  scrolled(Widget scrollbar, int orientation, XmScrollBarCallbackStruct *cbs)
{
        if  (orientation == XmVERTICAL)
                pix_voffset = cbs->value * cell_height;
        else
                pix_hoffset = cbs->value * cell_width;
        redraw(False);
}

static void  se_scrolled(Widget scrollbar, int orientation, XmScrollBarCallbackStruct *cbs)
{
        if  (orientation == XmVERTICAL)
                pix_voffset = cbs->value * cell_height;
        else
                pix_hoffset = cbs->value * cell_width;
        se_redraw(False);
}

static  void    gen_expose_resize(void (*redrawfunc)(Boolean), XmDrawingAreaCallbackStruct *cbs, int isve)
{
        Dimension   new_width, new_height, oldw, oldh;
        int         do_clear = 0;

        if  (!had_first)  {
                int             slidesize;
                XGCValues       gcv;
                had_first++;
                XtVaGetValues(drawing_a,
                              XmNwidth, &view_width,
                              XmNheight, &view_height,
                              XtNforeground, &gcv.foreground,
                              XtNbackground, &gcv.background,
                              NULL);
                slidesize = view_height/cell_height;
                if  (linecount < slidesize)
                        slidesize = linecount;
                if  (slidesize <= 0)
                        slidesize = 1;
                vsb = XtVaCreateManagedWidget("vsb",
                                              xmScrollBarWidgetClass,   scrolled_w,
                                              XmNorientation,           XmVERTICAL,
                                              XmNmaximum,               linecount,
                                              XmNsliderSize,            slidesize,
                                              NULL);
                slidesize = view_width/cell_width;
                if  (pagewidth < slidesize)
                        slidesize = pagewidth;
                if  (slidesize <= 0)
                        slidesize = 1;
                hsb = XtVaCreateManagedWidget("hsb",
                                              xmScrollBarWidgetClass,   scrolled_w,
                                              XmNorientation,           XmHORIZONTAL,
                                              XmNmaximum,               pagewidth,
                                              XmNsliderSize,            slidesize,
                                              NULL);
                pix_hoffset = pix_voffset = last_top = last_left = 0;
                if  (isve)  {
                        XtAddCallback(vsb, XmNvalueChangedCallback, (XtCallbackProc) se_scrolled, (XtPointer) XmVERTICAL);
                        XtAddCallback(hsb, XmNvalueChangedCallback, (XtCallbackProc) se_scrolled, (XtPointer) XmHORIZONTAL);
                        XtAddCallback(vsb, XmNdragCallback, (XtCallbackProc) se_scrolled, (XtPointer) XmVERTICAL);
                        XtAddCallback(hsb, XmNdragCallback, (XtCallbackProc) se_scrolled, (XtPointer) XmHORIZONTAL);
                }
                else  {
                        XtAddCallback(vsb, XmNvalueChangedCallback, (XtCallbackProc) scrolled, (XtPointer) XmVERTICAL);
                        XtAddCallback(hsb, XmNvalueChangedCallback, (XtCallbackProc) scrolled, (XtPointer) XmHORIZONTAL);
                        XtAddCallback(vsb, XmNdragCallback, (XtCallbackProc) scrolled, (XtPointer) XmVERTICAL);
                        XtAddCallback(hsb, XmNdragCallback, (XtCallbackProc) scrolled, (XtPointer) XmHORIZONTAL);
                }
                gcv.font = font->fid;
                mygc = XCreateGC(dpy, XtWindow(toplevel), GCFont | GCForeground | GCBackground, &gcv);
                XmScrolledWindowSetAreas(scrolled_w, hsb, vsb, drawing_a);
                if  (isve)  {
                        int     value = linecount - my_min(view_height/cell_height, linecount);
                        if  (value > 0)  {
                                XtVaSetValues(vsb, XtNvalue, value, NULL);
                                pix_voffset = value * cell_height;
                        }
                }
        }

        if  (cbs->reason == XmCR_EXPOSE)  {
                (*redrawfunc)(True);
                return;
        }

        oldw = view_width;
        oldh = view_height;
        XtVaGetValues(drawing_a, XmNwidth, &view_width, XmNheight, &view_height, NULL);
        new_width = view_width / cell_width;
        new_height = view_height / cell_height;
        if  ((int) new_height >= linecount) {
                pix_voffset = 0;
                do_clear = 1;
                new_height = (Dimension) linecount;
        }
        else  {
                pix_voffset = my_min(pix_voffset, (linecount-new_height) * cell_height);
                if  (oldh > linecount * cell_height)
                        do_clear = 1;
        }
        XtVaSetValues(vsb,
                      XmNsliderSize,    my_max(new_height, 1),
                      XmNvalue,         pix_voffset / cell_height,
                      NULL);

        /* identical to vertical case above */

        if  ((int) new_width >= pagewidth)  {
                pix_hoffset = 0;
                do_clear = 1;
                new_width = (Dimension) pagewidth;
        }
        else  {
                pix_hoffset = my_min(pix_hoffset, (pagewidth-new_width)*cell_width);
                if  (oldw > linepixwidth)
                        do_clear = 1;
        }
        XtVaSetValues(hsb,
                      XmNsliderSize,    my_max(new_width, 1),
                      XmNvalue,         pix_hoffset / cell_width,
                      NULL);

        if  (do_clear)
                (*redrawfunc)(True);
}

static void expose_resize(Widget w, XtPointer unused, XmDrawingAreaCallbackStruct *cbs)
{
        gen_expose_resize(redraw, cbs, 0);
}

static void se_expose_resize(Widget w, XtPointer unused, XmDrawingAreaCallbackStruct *cbs)
{
        gen_expose_resize(se_redraw, cbs, 1);
}

static void  endview(Widget w, int data)
{
        /* This routine is shared by the system error view routine,
           but only job view will have the pagecounts vector set up.  */

        if  (data > 0  &&  pagecounts)  {
                if  (JREQ.spq_start > JREQ.spq_end)  {
                        doerror(w, $EH{spq view start after end});
                        return;
                }
                if  (JREQ.spq_haltat > JREQ.spq_end)  {
                        doerror(w, $EH{spq view hat after end});
                        return;
                }
                my_wjmsg(SJ_CHNG);
        }
        if  (physpages)  {
                free((char *) physpages);
                physpages = NULL;
        }
        if  (pagecounts)  {
                free((char *) pagecounts);
                pagecounts = NULL;
        }
        if  (buffer)  {
                free(buffer);
                buffer = NULL;
        }
        if  (infile)  {
                fclose(infile);
                infile = NULL;
        }
        if  (mygc)  {
                XFreeGC(dpy, mygc);
                mygc = NULL;
        }
        if  (matchtext)  {
                XtFree(matchtext);
                matchtext = NULL;
        }
        if  (data >= 0)
                XtDestroyWidget(GetTopShell(w));
        inprogress = 0;
}

static void  cb_vspage(Widget w, int data)
{
        ULONG   *whichp = (ULONG *) 0;
        char    *whichm = (char *) 0;
        Widget  whichw = (Widget) 0;

        /* Ignore efforts if not writable.  */

        if  (!(mypriv->spu_flgs & PV_OTHERJ)  &&  strcmp(Realuname, JREQ.spq_uname) != 0)
                return;

        switch  (data)  {
        case  0:
                whichp = &JREQ.spq_start;
                whichw = spw;
                whichm = startpmsg;
                break;
        case  1:
                whichp = &JREQ.spq_end;
                whichw = epw;
                whichm = endpmsg;
                break;
        case  2:
                whichp = &JREQ.spq_haltat;
                whichw = hpw;
                whichm = hatpmsg;
                break;
        case  3:                /* Reset to full document */
                JREQ.spq_start = JREQ.spq_haltat = 0L;
                JREQ.spq_end = 0x7ffffffeL;
                XmTextSetString(spw, top_page == 0? startpmsg: "");
                XmTextSetString(epw, "");
                XmTextSetString(hpw, "");
                return;
        }
        if  (top_page != *whichp)  {
                *whichp = top_page;
                XmTextSetString(whichw, whichm);
        }
}

static int  foundmatch()
{
        int     bufl = strlen(buffer);
        int     matchl = strlen(matchtext);
        int     endl = bufl - matchl;
        int     bpos, mpos;

        for  (bpos = 0;  bpos <= endl;  bpos++)  {
                int     boffset = bpos;
                for  (mpos = 0;  mpos < matchl;  mpos++)  {
                        int     mch = matchtext[mpos];
                        int     bch = buffer[mpos+boffset];
                        if  (!matchcase)  {
                                mch = toupper(mch);
                                bch = toupper(bch);
                        }
                        if  (!(mch == bch  ||  mch == '.'))
                                goto  nomatch;

                        /* Make one space in pattern match any number
                           of spaces in buffer */

                        if  (bch == ' ')
                                while  (buffer[mpos+boffset+1] == ' ')
                                        boffset++;
                }

                /* Success...  */

                return  bpos;
        nomatch:
                ;
        }

        return  -1;
}

static void  execute_search()
{
        int     lng, mpos, redraw_needed = 0;
        unsigned        cline = last_matchline, view_cells;
        LONG    pageno;

        lng = initpage(cline, &pageno);
        if  (sbackward)  {
                while  (lng >= 0  &&  cline != 0)  {
                        lng = initpage(--cline, &pageno);
                        if  (lng > 0  &&  (mpos = foundmatch()) >= 0)
                                goto  foundit;
                }
                if  (wraparound)  {
                        cline = linecount - 1;
                        while  (cline > last_matchline)  {
                                lng = initpage(cline, &pageno);
                                if  (lng > 0  &&  (mpos = foundmatch()) >= 0)
                                        goto  foundit;
                                cline--;
                        }
                }
                doerror(drawing_a, $EH{spq view bs not found});
        }
        else  {
                LONG    endofpage = physpages[pageno+1];
                while  (lng >= 0  &&  ++cline < linecount)  {
                        if  (lng == 0)  {
                                pageno++;
                                endofpage = physpages[pageno+1];
                        }
                        if  ((lng = my_getline(endofpage)) > 0  &&  (mpos = foundmatch()) >= 0)
                                goto  foundit;
                }
                if  (wraparound)  {
                        cline = 0;
                        lng = initpage(cline, &pageno);
                        endofpage = physpages[pageno+1];
                        while  (lng >= 0  &&  cline < last_matchline)  {
                                if  (lng > 0)  {
                                        if  ((mpos = foundmatch()) >= 0)
                                                goto  foundit;
                                }
                                else  {
                                        pageno++;
                                        endofpage = physpages[pageno+1];
                                }
                                lng = my_getline(endofpage);
                                cline++;
                        }
                }
                doerror(drawing_a, $EH{spq view fs not found});
        }
        return;                 /* Drawn a blank */

 foundit:

        /* Found it.
           Now scroll the picture so that matched line is at the top */

        last_matchline = cline;
        view_cells = view_height / cell_height;

        if  (view_cells < linecount)  {
                if  (cline > linecount - view_cells)
                        cline = linecount - view_cells;
                pix_voffset = cline * cell_height;
                XtVaSetValues(vsb, XmNvalue, cline, NULL);
                redraw_needed++; /* Why cant the set values routine invoke the callback???? */
        }

        view_cells = view_width / cell_width;
        if  (view_cells < pagewidth)  {
                if  (mpos > pagewidth - view_cells)
                        mpos = pagewidth - view_cells;
                pix_hoffset = mpos * cell_width;
                XtVaSetValues(hsb, XmNvalue, mpos, NULL);
                redraw_needed++;
        }
        if  (redraw_needed)
                redraw(False);
}

static void  cb_vrsrch(Widget w, int data)
{
        if  (!matchtext  ||  matchtext[0] == '\0')  {
                doerror(w, $EH{No previous search});
                return;
        }
        sbackward = (char) data;
        execute_search();
}

static void  endsdlg(Widget w, int data)
{
        if  (data)  {
                if  (matchtext) /* Last time round */
                        XtFree(matchtext);
                XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &matchtext, NULL);
                if  (matchtext[0] == '\0')  {
                        doerror(w, $EH{Null search string});
                        return;
                }
                sbackward = XmToggleButtonGadgetGetState(workw[WORKW_FORWW])? 0: 1;
                matchcase = XmToggleButtonGadgetGetState(workw[WORKW_MATCHW])? 1: 0;
                wraparound = XmToggleButtonGadgetGetState(workw[WORKW_WRAPW])? 1: 0;
                XtDestroyWidget(GetTopShell(w));
                last_matchline = pix_voffset / cell_height;
                execute_search();
        }
        else
                XtDestroyWidget(GetTopShell(w));
}

static void  cb_vsrchfor(Widget parent)
{
        Widget  s_shell, panew, formw;

        InitsearchDlg(parent, &s_shell, &panew, &formw, matchtext);
        InitsearchOpts(formw, workw[WORKW_STXTW], sbackward, matchcase, wraparound);
        CreateActionEndDlg(s_shell, panew, (XtCallbackProc) endsdlg, $H{xmspq view search dlg});
}

static void  EndSetupViewDlg(Widget shelldlg, Widget panew, XtCallbackProc endrout, int helpcode)
{
        Widget  actform, okw, cancw, helpw;
        Dimension       h;

        actform = XtVaCreateWidget("actform",
                                   xmFormWidgetClass,           panew,
                                   XmNfractionBase,             7,
                                   NULL);

        okw = XtVaCreateManagedWidget("Ok",
                                      xmPushButtonGadgetClass,  actform,
                                      XmNtopAttachment,         XmATTACH_FORM,
                                      XmNbottomAttachment,      XmATTACH_FORM,
                                      XmNleftAttachment,        XmATTACH_POSITION,
                                      XmNleftPosition,          1,
                                      XmNrightAttachment,       XmATTACH_POSITION,
                                      XmNrightPosition,         2,
                                      XmNshowAsDefault,         True,
                                      XmNdefaultButtonShadowThickness,  1,
                                      XmNtopOffset,             0,
                                      XmNbottomOffset,          0,
                                      NULL);

        cancw = XtVaCreateManagedWidget("Cancel",
                                        xmPushButtonGadgetClass,        actform,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNbottomAttachment,            XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                3,
                                        XmNrightAttachment,             XmATTACH_POSITION,
                                        XmNrightPosition,               4,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,        1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        helpw = XtVaCreateManagedWidget("Help",
                                        xmPushButtonGadgetClass,        actform,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNbottomAttachment,            XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                5,
                                        XmNrightAttachment,             XmATTACH_POSITION,
                                        XmNrightPosition,               6,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,        1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        XtAddCallback(okw, XmNactivateCallback, endrout, (XtPointer) 1);
        XtAddCallback(cancw, XmNactivateCallback, endrout, (XtPointer) 0);
        XtAddCallback(helpw, XmNactivateCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(helpcode));
        XtManageChild(actform);
        XtVaGetValues(cancw, XmNheight, &h, NULL);
        XtVaSetValues(actform, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
        XtManageChild(panew);
        XtPopup(shelldlg, XtGrabNone);
}

void  cb_view(Widget parent)
{
        Widget  v_shell;
        Widget  panew, formw, previous, prevabove;
        const  struct   spq     *cj;
        int     ret;
        char    nbuf[12];

        if  (inprogress)  {
                doerror(jwid, $EH{View op in progress});
                return;
        }
        inprogress++;
        had_first = 0;
        cj = getselectedjob(PV_VOTHERJ);
        if  (!cj)  {
                inprogress = 0;
                return;
        }

        /* If it's a remote file slurp it up into a temporary file */

        if  (cj->spq_netid)  {
                FILE    *ifl;
                int     kch;

                if  (!(ifl = net_feed(FEED_NPSP, cj->spq_netid, cj->spq_rslot, cj->spq_job)))  {
                        disp_arg[0] = cj->spq_job;
                        disp_str = cj->spq_file;
                        disp_str2 = look_host(cj->spq_netid);
                        doerror(jwid, $EH{Cannot obtain network job});
                        inprogress = 0;
                        return;
                }
                else  {
                        infile = tmpfile();
                        while  ((kch = getc(ifl)) != EOF)
                                putc(kch, infile);
                        fclose(ifl);
                        rewind(infile);
                }
        }
        else  if  (!(infile = fopen(mkspid(SPNAM, cj->spq_job), "r")))  {
                disp_arg[0] = cj->spq_job;
                doerror(jwid, $EH{Cannot open local job});
                inprogress = 0;
                return;
        }

        /* Otherwise it worked....
           See what kind of pagination we're using. */

        if  ((ret = readpgfile(infile, cj)))
                notformfeed = 1;
        else  {
                ret = scanfile(infile);
                notformfeed = 0;
        }
        if  (ret < 0)  {
        nom:
                disp_arg[0] = cj->spq_job;
                disp_str = cj->spq_file;
                disp_arg[1] = cj->spq_size;
                doerror(jwid, $EH{spq page break memory error});
                inprogress = 0;
                return;
        }
        if  (linecount == 0 || pagewidth == 0)  {
                doerror(jwid, $EH{Null job in view});
                free((char *) physpages);
                inprogress = 0;
                return;
        }
        if  (!(buffer = malloc(pagewidth+1)))  {
                free((char *) physpages);
                goto  nom;
        }

        if  (!startpmsg)  {
                int     lng;
                startpmsg = gprompt($P{xmspq start page});
                endpmsg = gprompt($P{xmspq end page});
                hatpmsg = gprompt($P{xmspq hat page});
                longest_msg = strlen(startpmsg);
                if  ((lng = strlen(endpmsg)) > longest_msg)
                        longest_msg = lng;
                if  ((lng = strlen(hatpmsg)) > longest_msg)
                        longest_msg = lng;
                font = XLoadQueryFont(dpy, "fixed");
                cell_width = font->max_bounds.width;
                cell_height = font->ascent + font->descent;
        }

        linepixwidth = cell_width * pagewidth;

        v_shell = XtVaAppCreateShell("xmspqview",       NULL,
                                     topLevelShellWidgetClass,
                                     dpy,
                                     NULL);
        if  (!docbitmap)
                docbitmap = XCreatePixmapFromBitmapData(dpy,
                                                        RootWindowOfScreen(XtScreen(toplevel)),
                                                        xmspqdoc_bits, xmspqdoc_width, xmspqdoc_height, 1, 0, 1);
        XtVaSetValues(v_shell, XmNiconPixmap, docbitmap, NULL);

        panew = XtVaCreateWidget("pane",
                                 xmPanedWindowWidgetClass,      v_shell,
                                 XmNsashWidth,                  1,
                                 XmNsashHeight,                 1,
                                 NULL);

        setup_menus(panew, viewmenlist);

        formw = XtVaCreateWidget("form",
                                 xmFormWidgetClass,             panew,
                                 XmNfractionBase,               3,
                                 NULL);

        /* Build Jobno, title, form, user left to right */

        previous = CreateJtitle(formw);
        previous = XtVaCreateManagedWidget("Header",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_FORM,
                                           XmNleftAttachment,   XmATTACH_WIDGET,
                                           XmNleftWidget,       previous,
                                           NULL);

        previous =
                workw[WORKW_HTXTW] =
                        XtVaCreateManagedWidget("hdr",
                                                xmTextFieldWidgetClass, formw,
                                                XmNcolumns,             MAXTITLE,
                                                XmNmaxWidth,            MAXTITLE,
                                                XmNcursorPositionVisible,       False,
                                                XmNeditable,            False,
                                                XmNtopAttachment,       XmATTACH_FORM,
                                                XmNleftAttachment,      XmATTACH_WIDGET,
                                                XmNleftWidget,          previous,
                                                NULL);

        previous = XtVaCreateManagedWidget("Form",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_FORM,
                                           XmNleftAttachment,   XmATTACH_WIDGET,
                                           XmNleftWidget,       previous,
                                           NULL);

        prevabove =
                workw[WORKW_FTXTW] =
                        XtVaCreateManagedWidget("form",
                                                xmTextFieldWidgetClass, formw,
                                                XmNcolumns,             MAXFORM,
                                                XmNmaxWidth,            MAXFORM,
                                                XmNcursorPositionVisible,       False,
                                                XmNeditable,            False,
                                                XmNtopAttachment,       XmATTACH_FORM,
                                                XmNleftAttachment,      XmATTACH_WIDGET,
                                                XmNleftWidget,          previous,
                                                NULL);

        previous = XtVaCreateManagedWidget("User",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_WIDGET,
                                           XmNtopWidget,        prevabove,
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           NULL);

        previous =
                workw[WORKW_UTXTW] =
                        XtVaCreateManagedWidget("user",
                                                xmTextFieldWidgetClass, formw,
                                                XmNcolumns,             UIDSIZE,
                                                XmNmaxWidth,            UIDSIZE,
                                                XmNcursorPositionVisible,       False,
                                                XmNeditable,            False,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_WIDGET,
                                                XmNleftWidget,          previous,
                                                NULL);

        XmTextSetString(workw[WORKW_HTXTW], (char *) cj->spq_file);
        XmTextSetString(workw[WORKW_FTXTW], (char *) cj->spq_form);
        XmTextSetString(workw[WORKW_UTXTW], (char *) cj->spq_uname);

        previous = XtVaCreateManagedWidget("npages",
                                           xmTextFieldWidgetClass,      formw,
                                           XmNcolumns,                  10,
                                           XmNmaxWidth,                 10,
                                           XmNcursorPositionVisible,    False,
                                           XmNeditable,                 False,
                                           XmNtopAttachment,            XmATTACH_WIDGET,
                                           XmNtopWidget,                prevabove,
                                           XmNrightAttachment,          XmATTACH_FORM,
                                           NULL);

        sprintf(nbuf, "%10ld", (long) cj->spq_npages);
        XmTextSetString(previous, nbuf);
        previous = XtVaCreateManagedWidget("of",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_WIDGET,
                                           XmNtopWidget,        prevabove,
                                           XmNrightAttachment,  XmATTACH_WIDGET,
                                           XmNrightWidget,      previous,
                                           NULL);

        previous = workw[WORKW_PTXTW] = XtVaCreateManagedWidget("cpage",
                                                                xmTextFieldWidgetClass,         formw,
                                                                XmNcolumns,                     10,
                                                                XmNmaxWidth,                    10,
                                                                XmNcursorPositionVisible,       False,
                                                                XmNeditable,                    False,
                                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                                XmNtopWidget,                   prevabove,
                                                                XmNrightAttachment,             XmATTACH_WIDGET,
                                                                XmNrightWidget,                 previous,
                                                                NULL);

        XtVaCreateManagedWidget("Page",
                                xmLabelGadgetClass,     formw,
                                XmNtopAttachment,       XmATTACH_WIDGET,
                                XmNtopWidget,   prevabove,
                                XmNrightAttachment,     XmATTACH_WIDGET,
                                XmNrightWidget, previous,
                                NULL);

        sprintf(nbuf, "%10ld", 1L);
        XmTextSetString(workw[WORKW_PTXTW], nbuf);

        /* Spaces for start/end/haltat messages */

        spw = XtVaCreateManagedWidget("spage",
                                      xmTextFieldWidgetClass,   formw,
                                      XmNcolumns,               longest_msg,
                                      XmNmaxWidth,              longest_msg,
                                      XmNcursorPositionVisible, False,
                                      XmNeditable,                      False,
                                      XmNtopAttachment, XmATTACH_WIDGET,
                                      XmNtopWidget,             workw[WORKW_PTXTW],
                                      XmNleftAttachment,        XmATTACH_FORM,
                                      NULL);

        epw = XtVaCreateManagedWidget("epage",
                                      xmTextFieldWidgetClass,   formw,
                                      XmNcolumns,               longest_msg,
                                      XmNmaxWidth,              longest_msg,
                                      XmNcursorPositionVisible, False,
                                      XmNeditable,                      False,
                                      XmNtopAttachment, XmATTACH_WIDGET,
                                      XmNtopWidget,             workw[WORKW_PTXTW],
                                      XmNrightAttachment,       XmATTACH_FORM,
                                      NULL);

        hpw = XtVaCreateManagedWidget("hpage",
                                      xmTextFieldWidgetClass,   formw,
                                      XmNcolumns,               longest_msg,
                                      XmNmaxWidth,              longest_msg,
                                      XmNcursorPositionVisible, False,
                                      XmNeditable,                      False,
                                      XmNtopAttachment,         XmATTACH_WIDGET,
                                      XmNtopWidget,             workw[WORKW_PTXTW],
                                      XmNleftAttachment,        XmATTACH_WIDGET,
                                      XmNleftWidget,            spw,
                                      NULL);

        scrolled_w = XtVaCreateManagedWidget("vscroll",
                                             xmScrolledWindowWidgetClass,       formw,
                                             XmNscrollingPolicy,                XmAPPLICATION_DEFINED,
                                             XmNvisualPolicy,                   XmVARIABLE,
                                             XmNtopAttachment,                  XmATTACH_WIDGET,
                                             XmNtopWidget,                      hpw,
                                             XmNbottomAttachment,               XmATTACH_FORM,
                                             XmNleftAttachment,                 XmATTACH_FORM,
                                             XmNrightAttachment,                XmATTACH_FORM,
                                             NULL);

        drawing_a = XtVaCreateManagedWidget("vdraw",
                                            xmDrawingAreaWidgetClass,           scrolled_w,
                                            NULL);

        XtAddCallback(drawing_a, XmNexposeCallback, (XtCallbackProc) expose_resize, (XtPointer) 0);
        XtAddCallback(drawing_a, XmNresizeCallback, (XtCallbackProc) expose_resize, (XtPointer) 0);
        top_page = -1L;
        XtManageChild(formw);
        EndSetupViewDlg(v_shell, panew, (XtCallbackProc) endview, $H{xmspq view job help});
        XtAddCallback(v_shell, XmNpopdownCallback, (XtCallbackProc) endview, (XtPointer) 0);
        XtAddCallback(v_shell, XmNdestroyCallback, (XtCallbackProc) endview, (XtPointer) -1);
}

static void  clearlog(Widget w)
{
        if  (!Confirm(w, $PH{Clear syserr log}))
                return;
        unlink(REPFILE);
        endview(w, 1);
}

void  cb_syserr(Widget parent)
{
        Widget  v_shell, panew;
        int     curline, ch;

        if  (!(infile = fopen(REPFILE, "r")))  {
                doerror(jwid, $EH{No log file yet});
                return;
        }
        if  (inprogress)  {
                doerror(jwid, $EH{View op in progress});
                return;
        }
        inprogress++;
        had_first = 0;
        pagewidth = 0;
        linecount = 0;
        curline = 0;

        /* Just get widths */

        while  ((ch = getc(infile)) != EOF)  {
                switch  (ch)  {
                default:
                        if  (ch & 0x80)  {
                                ch &= 0x7f;
                                curline += 2;
                        }
                        if  (ch < ' ')
                                curline++;
                        curline++;
                        break;
                case  '\n':
                        if  (curline > pagewidth)
                                pagewidth = curline;
                        curline = 0;
                        linecount++;
                        break;
                case  '\t':
                        curline = (curline + 8) & ~7;
                        break;
                }
        }
        fileend = ftell(infile);

        if  (linecount == 0 || pagewidth == 0)  {
                doerror(jwid, $EH{No log file yet});
                inprogress = 0;
                return;
        }

        if  (!(buffer = malloc(pagewidth+1)))  {
                doerror(jwid, $EH{No mem for syslog});
                inprogress = 0;
                return;
        }

        if  (!font)  {
                font = XLoadQueryFont(dpy, "fixed");
                cell_width = font->max_bounds.width;
                cell_height = font->ascent + font->descent;
        }

        linepixwidth = cell_width * pagewidth;

        v_shell = XtVaAppCreateShell("xmspqverr",       NULL,
                                     topLevelShellWidgetClass,
                                     dpy,
                                     NULL);
        if  (!docbitmap)
                docbitmap = XCreatePixmapFromBitmapData(dpy,
                                                        RootWindowOfScreen(XtScreen(toplevel)),
                                                        xmspqdoc_bits, xmspqdoc_width, xmspqdoc_height, 1, 0, 1);
        XtVaSetValues(v_shell, XmNiconPixmap, docbitmap, NULL);

        panew = XtVaCreateWidget("pane",
                                 xmPanedWindowWidgetClass,      v_shell,
                                 XmNsashWidth,                  1,
                                 XmNsashHeight,                 1,
                                 NULL);

        setup_menus(panew, syserrmenlist);

        scrolled_w = XtVaCreateManagedWidget("vscroll",
                                             xmScrolledWindowWidgetClass,       panew,
                                             XmNscrollingPolicy,                XmAPPLICATION_DEFINED,
                                             XmNvisualPolicy,                   XmVARIABLE,
                                             NULL);

        drawing_a = XtVaCreateManagedWidget("vdraw",
                                            xmDrawingAreaWidgetClass,           scrolled_w,
                                            NULL);

        XtAddCallback(drawing_a, XmNexposeCallback, (XtCallbackProc) se_expose_resize, (XtPointer) 0);
        XtAddCallback(drawing_a, XmNresizeCallback, (XtCallbackProc) se_expose_resize, (XtPointer) 0);
        EndSetupViewDlg(v_shell, panew, (XtCallbackProc) endview, $H{xmspq log view menu});
        XtAddCallback(v_shell, XmNpopdownCallback, (XtCallbackProc) endview, (XtPointer) 0);
        XtAddCallback(v_shell, XmNdestroyCallback, (XtCallbackProc) endview, (XtPointer) -1);
}
