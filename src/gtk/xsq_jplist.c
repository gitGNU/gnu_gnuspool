/* xsq_jplist.c -- job and printer list display

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
#include <gtk/gtk.h>
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
#include "xsq_ext.h"
#include "gtk_lib.h"
#include "displayopt.h"

/* Column numbers for job list */

#define	JNUM_COL	1
#define	JUSER_COL	2
#define	TITLE_COL	3
#define	JFORM_COL	4
#define	REACHED_COL	5
#define	PAGESIZE_COL	6
#define	BYTESIZE_COL	7
#define	COPIES_COL	8
#define	PRIORITY_COL	9
#define	JPRINTER_COL	10

/* The next are "non-standard" ones */

#define	WATTN_COL	11
#define	MATTN_COL	12
#define	WRITE_COL	13
#define	MAIL_COL	14
#define	PUSER_COL	15
#define	JCC_COL		16
#define	DELIM_COL	17
#define	DELIMNUM_COL	18
#define	PPFLAGS_COL	19
#define	STARTP_COL	20
#define	HATP_COL	21
#define	ENDP_COL	22
#define	HTIME_COL	23
#define	NOODDP_COL	24
#define	NOEVENP_COL	25
#define	SWAPOE_COL	26
#define	DELP_COL	27
#define	DELNP_COL	28
#define	SUBTIME_COL	29
#define	LOCALJ_COL	30
#define	OHOST_COL	31
#define	RETQ_COL	32
#define	SUPHDR_COL	33
#define	PRINTED_COL	34

/* Column numbers for printer list */

#define	PPNAME_COL	1
#define	PDEVICE_COL	2
#define	PFORM_COL	3
#define	PSTATEMSG_COL	4
#define	PLIM_COL	5
#define	PJOBNO_COL	6
#define	PTRUSER_COL	7
#define	PSHRIEK_COL	8

/* The next are "non-standard" ones */

#define	PAB_COL		9
#define	PCL_COL		10
#define	PCOMM_COL	11
#define	PHEOJ_COL	12
#define	PID_COL		13
#define	PLOCONLY_COL	14
#define	PMSG_COL	15
#define	PNA_COL		16
#define	POSTATE_COL	17
#define	PMINSIZE_COL	18
#define	PMAXSIZE_COL	19

/* Sort column ids */

#define	SORTBY_PSEQ	1
#define	SORTBY_PNAME	2
#define	SORTBY_PDEV	3
#define	SORTBY_PFORM	4
#define	SORTBY_PCOMM	5

struct	jplist_elems  jlist_els[] =  {
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq seq hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq jnum hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq juser hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq title hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq jform hdr}  },
	{  G_TYPE_UINT, JPREND_PROGRESS, -1, $P{xspq reached hdr}  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq pages hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq size hdr}  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq copies hdr}  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq pri hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq jptr hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq wattn hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq mattn hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq write hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq mail hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq post user hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq cc hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq delim hdr}  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq delimnum hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq ppflags hdr}  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq startp hdr}  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq hatp hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq endp hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq holdt hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq oddp hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq evenp hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq swapoe hdr}  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq delp hdr}  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq delnp hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq subtime hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq jlocal hdr}  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq origh hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq retq hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq supp hdr}  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq printed hdr}  }
};

#define	NUM_JOB_ELS	sizeof(jlist_els) / sizeof(struct jplist_elems)

struct	jplist_elems  plist_els[] =  {
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq ptr seq hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pname hdr}, SORTBY_PNAME  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq dev hdr}, SORTBY_PDEV  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pform hdr}, SORTBY_PFORM  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq statemsg hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq limit hdr}, SORTBY_PSEQ },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pjobno hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq puser hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pshriek hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq ab hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pclass hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pcomm hdr}, SORTBY_PCOMM  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq heoj hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pid hdr}, SORTBY_PSEQ  },
	{  G_TYPE_BOOLEAN, JPREND_TOGGLE, -1, $P{xspq plocal hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pmsg hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq pna hdr}, SORTBY_PSEQ  },
	{  G_TYPE_STRING, JPREND_TEXT, -1, $P{xspq ostate hdr}, SORTBY_PSEQ  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq pminsize hdr}, SORTBY_PSEQ  },
	{  G_TYPE_UINT, JPREND_TEXT, -1, $P{xspq pmaxsize hdr}, SORTBY_PSEQ  } };

#define	NUM_PTR_ELS	sizeof(plist_els) / sizeof(struct jplist_elems)

/* Default fields we set up initially */

USHORT	defflds[] =  { SEQ_COL, JNUM_COL, JUSER_COL, TITLE_COL, JFORM_COL, REACHED_COL, PAGESIZE_COL, COPIES_COL, PRIORITY_COL, JPRINTER_COL  };
USHORT	pdefflds[] =  { PPNAME_COL, PDEVICE_COL, PFORM_COL, PSTATEMSG_COL, PLIM_COL, PJOBNO_COL, PTRUSER_COL, PSHRIEK_COL  };

#define NDEFFLDS     sizeof(defflds)/sizeof(USHORT)
#define NPDEFFLDS     sizeof(pdefflds)/sizeof(USHORT)

USHORT	*def_jobflds, *def_ptrflds;
int	ndef_jobflds, ndef_ptrflds;

struct	fielddef  {
	GtkWidget	*view;
	struct	jplist_elems  *list_els;
	unsigned	num_els;
	int		is_sorted;
};

struct	fielddef	jobflds = { NULL, jlist_els, NUM_JOB_ELS } , ptrflds = { NULL, plist_els, NUM_PTR_ELS  };

/* Vector of states - assumed given prompt codes consecutively.  */

static	char	*statenames[SPP_NSTATES];

static	char	*halteoj,	/* Halt at end of job message */
		*namsg,		/* Non-aligned message */
		*intermsg,
		*localptr;

extern	int	Dirty;

/* Open job file and get relevant messages.  */

void	openjfile(void)
{
	if  (!jobshminit(1))  {
		print_error($E{Cannot open jshm});
		exit(E_JOBQ);
	}
}


/* Open print file.  */
void	openpfile(void)
{
	int	i;

	if  (!ptrshminit(1))  {
		print_error($E{Cannot open pshm});
		exit(E_PRINQ);
	}

	/* Read state names */

	for  (i = 0;  i < SPP_NSTATES;  i++)
		statenames[i] = gprompt($P{Printer status null}+i);
	halteoj = gprompt($P{Printer heoj});
	namsg = gprompt($P{Printer not aligned});
	intermsg = gprompt($P{Printer interrupted});
}

gint	sort_uint(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	guint	seq1, seq2;
	gint	colnum = GPOINTER_TO_INT(userdata);
        gtk_tree_model_get(model, a, colnum, &seq1, -1);
        gtk_tree_model_get(model, b, colnum, &seq2, -1);
	return  seq1 < seq2? -1:  seq1 == seq2? 0: 1;
}

gint	sort_string(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
        gchar	*name1, *name2;
	gint	ret = 0;
	gint	colnum = GPOINTER_TO_INT(userdata);

        gtk_tree_model_get(model, a, colnum, &name1, -1);
        gtk_tree_model_get(model, b, colnum, &name2, -1);

	if  (!name1  ||  !name2)  {
		if  (!name1  &&  !name2)
			return  0;
		if  (!name1)  {
			g_free(name2);
			return  -1;
		}
		else  {
			g_free(name1);
			return  1;
		}
	}

	ret = g_utf8_collate(name1, name2);
	g_free(name1);
	g_free(name2);
	return  ret;
}

void	set_tview_col(struct fielddef *, const int);

void	men_toggled(GtkMenuItem *item, struct fielddef *fd)
{
	unsigned  cnt;

	for  (cnt = 0;  cnt <  fd->num_els;  cnt++)
		if  (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(fd->list_els[cnt].menitem)))  {
			if  (fd->list_els[cnt].colnum < 0)  {
				set_tview_col(fd, cnt);
				gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(fd->view), TRUE);
				Dirty++;
			}
		}
		else  if  (fd->list_els[cnt].colnum >= 0)  {
			int	cn = fd->list_els[cnt].colnum;
			unsigned   cnt2;
			gtk_tree_view_remove_column(GTK_TREE_VIEW(fd->view), gtk_tree_view_get_column(GTK_TREE_VIEW(fd->view), cn));
			fd->list_els[cnt].colnum = -1;
			for  (cnt2 = 0;  cnt2 < fd->num_els;  cnt2++)
				if  (fd->list_els[cnt2].colnum > cn)
					fd->list_els[cnt2].colnum--;
			Dirty++;
		}
}

/* I'd like to make this a right-click but I don't know how - sob */

gboolean	hdr_clicked(GtkWidget *treeview, GdkEventButton *event, struct fielddef *fd)
{
	GtkWidget	*menu;
	unsigned  	cnt;

	if  (event->type != GDK_BUTTON_PRESS  ||  event->button != 3)
		return  FALSE;

	menu = gtk_menu_new();

	for  (cnt = 0;  cnt <  fd->num_els;  cnt++)  {
		GtkWidget  *item = gtk_check_menu_item_new_with_label(fd->list_els[cnt].descr);
		if  (fd->list_els[cnt].colnum >= 0)
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		g_signal_connect(item, "toggled", (GCallback) men_toggled, fd);
		fd->list_els[cnt].menitem = item;
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_object_unref(G_OBJECT(item));
	}

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
	return  TRUE;
}

void	set_tview_col(struct fielddef *fd, const int typenum)
{
	struct jplist_elems  *lp = &fd->list_els[typenum];
	GtkCellRenderer  *renderer;
	GtkTreeViewColumn  *col;
	GtkWidget	*lab;
	int  colnum = 0;

	switch  (lp->rendtype)  {
	case  JPREND_TEXT:
		renderer = gtk_cell_renderer_text_new();
		colnum = gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fd->view), -1, lp->msgtext, renderer, "text", typenum, NULL);
		break;

	case  JPREND_PROGRESS:
		renderer = gtk_cell_renderer_progress_new();
		colnum = gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fd->view), -1, lp->msgtext, renderer, "value", typenum, NULL);
		break;

	case  JPREND_TOGGLE:
		renderer = gtk_cell_renderer_toggle_new();
		colnum = gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fd->view), -1, lp->msgtext, renderer, "active", typenum, NULL);
		break;
	}

	col = gtk_tree_view_get_column(GTK_TREE_VIEW(fd->view), colnum-1);
	if  (fd->is_sorted)
		gtk_tree_view_column_set_sort_column_id(col, lp->sortid);
	lab = gtk_label_new(lp->msgtext);
	gtk_tree_view_column_set_widget(col, lab);
	gtk_widget_show(lab);
	g_signal_connect(gtk_widget_get_ancestor(lab, GTK_TYPE_BUTTON), "button-press-event", G_CALLBACK(hdr_clicked), fd);
	gtk_tree_view_column_set_alignment(col, 0.0);
	lp->colnum = colnum-1;
}

void	set_tview_attribs(struct fielddef *fd, USHORT *flds, unsigned nflds)
{
	unsigned  cnt;
	for  (cnt = 0; cnt < nflds;  cnt++)
		set_tview_col(fd, flds[cnt]);
}

/* Create job list window.
   The job list is returned in the global jwid.
   We also set up the global jlist_store for the Liststore. */

void	init_jlist_win(void)
{
	unsigned  cnt;
	GtkTreeSelection *sel;
	GType	coltypes[NUM_JOB_ELS];

	/* First get all the prompt messages */

	for  (cnt = 0;  cnt < NUM_JOB_ELS;  cnt++)  {
		jlist_els[cnt].msgtext = gprompt(jlist_els[cnt].msgcode);
		coltypes[cnt] = jlist_els[cnt].type;
	}

	for  (cnt = 0;  cnt < NUM_JOB_ELS;  cnt++)
		jlist_els[cnt].descr = gprompt(jlist_els[cnt].msgcode + $P{xspq seq full descr} - $P{xspq seq hdr});

	/*  Create job display treeview */

	jwid = gtk_tree_view_new();
	jobflds.view = jwid;
	jobflds.is_sorted = 0;
	jlist_store = gtk_list_store_newv(NUM_JOB_ELS, coltypes);
	gtk_tree_view_set_model(GTK_TREE_VIEW(jwid), GTK_TREE_MODEL(jlist_store));
	if  (def_jobflds)
		set_tview_attribs(&jobflds, def_jobflds, ndef_jobflds);
	else
		set_tview_attribs(&jobflds, defflds, NDEFFLDS);
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
	localptr = gprompt($P{Spq local printer});
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(jwid), TRUE);
}

void	set_time(char *buf, LONG tim)
{
	struct	tm	*tp;
	time_t	tt = tim;
	int	mon, day;

	if  (tim == 0)  {
		buf[0] = '-';;
		buf[1] = '\0';
		return;
	}

	tp = localtime(&tt);
	mon = tp->tm_mon+1;
	day = tp->tm_mday;
#ifdef	HAVE_TM_ZONE
	if  (tp->tm_gmtoff <= -4 * 60 * 60)
#else
	if  (timezone >= 4 * 60 * 60)
#endif
	{
		day = mon;
		mon = tp->tm_mday;
	}
	sprintf(buf, "%.2d/%.2d/%.4d %.2d:%.2d", day, mon, tp->tm_year + 1900, tp->tm_hour, tp->tm_min);
}

void	set_jlist_store(const int jnum, GtkTreeIter *iter)
{
	const  struct  spq  *jp = &Job_seg.jj_ptrs[jnum]->j;
	const	char	*ob;
	char	outbuffer[200];

	/* Get job number */

	if  (jp->spq_netid)
		sprintf(outbuffer, "%s:%ld", look_host(jp->spq_netid), (long) jp->spq_job);
	else
		sprintf(outbuffer, "%ld", (long) jp->spq_job);

	gtk_list_store_set(jlist_store, iter,
			   SEQ_COL, 	(guint) jnum,
			   JNUM_COL, 	outbuffer,
			   JUSER_COL,	jp->spq_uname,
			   TITLE_COL,	jp->spq_file,
			   JFORM_COL,	jp->spq_form,
			   REACHED_COL,	(guint) (((double) jp->spq_posn * 100.0 / (double) jp->spq_size) + 0.5),
			   PAGESIZE_COL,jp->spq_npages,
			   BYTESIZE_COL,prin_size(jp->spq_size),
			   COPIES_COL,	(guint) jp->spq_cps,
			   PRIORITY_COL,(guint) jp->spq_pri,
			   -1);

	if  (jp->spq_dflags & SPQ_PQ)  {
		if  (jp->spq_pslot < 0  || jp->spq_pslot >= Ptr_seg.dptr->ps_maxptrs)
			ob = localptr;
		else  {
			const struct  spptr  *pp = &Ptr_seg.plist[jp->spq_pslot].p;
			if  (pp->spp_state == SPP_NULL)
				ob = localptr;
			else  if  (pp->spp_netid)  {
				sprintf(outbuffer, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
				ob = outbuffer;
			}
			else
				ob = pp->spp_ptr;
		}
	}
	else
		ob = jp->spq_ptr;

	gtk_list_store_set(jlist_store, iter,
			   JPRINTER_COL,ob,
			   WATTN_COL,	jp->spq_jflags & SPQ_WATTN? TRUE: FALSE,
			   MATTN_COL,	jp->spq_jflags & SPQ_MATTN? TRUE: FALSE,
			   WRITE_COL,	jp->spq_jflags & SPQ_WRT? TRUE: FALSE,
			   MAIL_COL,	jp->spq_jflags & SPQ_MAIL? TRUE: FALSE,
			   PUSER_COL,	jp->spq_puname,
			   JCC_COL,	hex_disp(jp->spq_class, 0),
			   DELIM_COL,	"\\f",
			   DELIMNUM_COL,1,
			   PPFLAGS_COL,	jp->spq_flags,
			   -1);

	if  (jp->spq_end < LOTSANDLOTS)  {
		sprintf(outbuffer, "%ld", (long) jp->spq_end + 1);
		ob = outbuffer;
	}
	else
		ob = "*";

	gtk_list_store_set(jlist_store, iter,
			   STARTP_COL,	jp->spq_start + 1,
			   HATP_COL,	jp->spq_haltat + 1,
			   ENDP_COL,	ob,
			   NOODDP_COL,	jp->spq_jflags & SPQ_ODDP? TRUE: FALSE,
			   NOEVENP_COL,	jp->spq_jflags & SPQ_EVENP? TRUE: FALSE,
			   SWAPOE_COL,	jp->spq_jflags & SPQ_REVOE? TRUE: FALSE,
			   DELP_COL,	jp->spq_ptimeout,
			   DELNP_COL,	jp->spq_nptimeout,
			   LOCALJ_COL,	jp->spq_jflags & SPQ_LOCALONLY? TRUE: FALSE,
			   RETQ_COL,	jp->spq_jflags & SPQ_RETN? TRUE: FALSE,
			   SUPHDR_COL,	jp->spq_jflags & SPQ_NOH? TRUE: FALSE,
			   PRINTED_COL,	jp->spq_dflags & SPQ_PRINTED? TRUE: FALSE,
			   -1);

	if  (jp->spq_jflags & HT_ROAMUSER)  {
		sprintf(outbuffer, "(%s)", jp->spq_uname);
		ob = outbuffer;
	}
	else
		ob = jp->spq_orighost? look_host(jp->spq_orighost): look_host(myhostid);

	gtk_list_store_set(jlist_store, iter, OHOST_COL, ob, -1);
	set_time(outbuffer, jp->spq_hold);
	gtk_list_store_set(jlist_store, iter, HTIME_COL, outbuffer, -1);
	set_time(outbuffer, jp->spq_time);
	gtk_list_store_set(jlist_store, iter, SUBTIME_COL, outbuffer, -1);
}

/* Create printer list window.
   The job list is returned in the global jwid.
   We also set up the global jlist_store for the Liststore. */

void	init_plist_win(void)
{
	unsigned  cnt;
	GtkTreeSelection *sel;
	GType	coltypes[NUM_PTR_ELS];

	/* First get all the prompt messages */

	for  (cnt = 0;  cnt < NUM_PTR_ELS;  cnt++)  {
		plist_els[cnt].msgtext = gprompt(plist_els[cnt].msgcode);
		coltypes[cnt] = plist_els[cnt].type;
	}

	for  (cnt = 0;  cnt < NUM_PTR_ELS;  cnt++)
		plist_els[cnt].descr = gprompt(plist_els[cnt].msgcode + $P{xspq pseq full descr} - $P{xspq ptr seq hdr});

	/*  Create printer display treeview */

	pwid = gtk_tree_view_new();
	ptrflds.view = pwid;
	ptrflds.is_sorted = 1;
	unsorted_plist_store = gtk_list_store_newv(NUM_PTR_ELS, coltypes);
	sorted_plist_store = (GtkTreeModelSort *) gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(unsorted_plist_store));

	/* Set printer sort funcs */

	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_plist_store), SORTBY_PSEQ, sort_uint, GINT_TO_POINTER(SEQ_COL), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_plist_store), SORTBY_PNAME, sort_string, GINT_TO_POINTER(PPNAME_COL), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_plist_store), SORTBY_PDEV, sort_string, GINT_TO_POINTER(PDEVICE_COL), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_plist_store), SORTBY_PFORM, sort_string, GINT_TO_POINTER(PFORM_COL), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_plist_store), SORTBY_PCOMM, sort_string, GINT_TO_POINTER(PCOMM_COL), NULL);

	/* Set initial sort - TODO read from config */

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sorted_plist_store), SORTBY_PSEQ, GTK_SORT_ASCENDING);

	/* Set up model */

	gtk_tree_view_set_model(GTK_TREE_VIEW(pwid), GTK_TREE_MODEL(sorted_plist_store));

	/* Set up initial fields either from our default or as read in from config file */

	if  (def_ptrflds)	/* Read in */
		set_tview_attribs(&ptrflds, def_ptrflds, ndef_ptrflds);
	else			/* Set from our default */
		set_tview_attribs(&ptrflds, pdefflds, NPDEFFLDS);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwid));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(pwid), TRUE);
}

void	set_plist_store(const int pnum, GtkTreeIter *iter)
{
	const  struct  spptr  *pp = &Ptr_seg.pp_ptrs[pnum]->p;
	char	ptrnamebuf[PTRNAMESIZE+30], devnamebuf[LINESIZE+3], statenbuf[PFEEDBACK+20], limb[3];
	char	jobnumbuf[40], pidbuf[10];
	const	char	*unam = "";
	int	staten = pp->spp_state >= SPP_NSTATES? SPP_NULL: pp->spp_state;

	/* Printer name */

	if  (pp->spp_netid)
		sprintf(ptrnamebuf, "%s:%s", look_host(pp->spp_netid), pp->spp_ptr);
	else
		strcpy(ptrnamebuf, pp->spp_ptr);

	/* Device name */

	if  (pp->spp_netflags & SPP_LOCALNET)
		sprintf(devnamebuf, "[%s]", pp->spp_dev);
	else
		strcpy(devnamebuf, pp->spp_dev);

	/* State plus message */

	if  (staten < SPP_HALT  &&  pp->spp_feedback[0])
		sprintf(statenbuf, "%s:%s", statenames[staten], pp->spp_feedback);
	else
		strcpy(statenbuf, statenames[staten]);

	/* Limit */

	if  (pp->spp_minsize != 0 || pp->spp_maxsize != 0)  {
		char	*lp = limb;
		if  (pp->spp_minsize != 0)
			*lp++ = '<';
		if  (pp->spp_maxsize != 0)
			*lp++ = '>';
		*lp = '\0';
	}
	else
		limb[0] = '\0';

	/* Job number */

	jobnumbuf[0] = '\0';

	if  (staten >= SPP_PREST  &&  pp->spp_jslot >= 0  &&  pp->spp_jslot < Job_seg.dptr->js_maxjobs)  {
		const struct  spq  *jp = &Job_seg.jlist[pp->spp_jslot].j;

		if  (jp->spq_netid)
			sprintf(jobnumbuf, "%s:%ld", look_host(jp->spq_netid), (long) pp->spp_job);
		else
			sprintf(jobnumbuf, "%ld", (long) pp->spp_job);

		unam = jp->spq_uname;
	}

	pidbuf[0] = '\0';
	if  (!pp->spp_netid  &&  pp->spp_state >= SPP_PROC)
		sprintf(pidbuf, "%ld", (long) pp->spp_pid);

	gtk_list_store_set(unsorted_plist_store, iter,
			   SEQ_COL,		(guint) pnum,
			   PPNAME_COL,		ptrnamebuf,
			   PDEVICE_COL,		devnamebuf,
			   PFORM_COL,		pp->spp_form,
			   PSTATEMSG_COL,	statenbuf,
			   PLIM_COL,		limb,
			   PJOBNO_COL,		jobnumbuf,
			   PUSER_COL,		unam,
			   PSHRIEK_COL,		pp->spp_dflags == 0? "": pp->spp_dflags & SPP_HADAB? intermsg: namsg,
			   PAB_COL,		pp->spp_dflags & SPP_HADAB? intermsg: "",
			   PCL_COL,		hex_disp(pp->spp_class, 0),
			   PCOMM_COL,		pp->spp_comment,
			   PHEOJ_COL,		pp->spp_sflags & SPP_HEOJ? halteoj: "",
			   PID_COL,		pidbuf,
			   PLOCONLY_COL,	pp->spp_netflags & SPP_LOCALONLY? TRUE: FALSE,
			   PMSG_COL,		pp->spp_feedback,
			   PNA_COL,		pp->spp_dflags & SPP_HADAB? namsg: "",
			   POSTATE_COL,		statenames[staten],
			   PMINSIZE_COL,	pp->spp_minsize,
			   PMAXSIZE_COL,	pp->spp_maxsize,
			   -1);
}

/* Initial display of jobs */

void	init_jdisplay(void)
{
	unsigned  jcnt;

	readjoblist(1);

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		GtkTreeIter   iter;
		gtk_list_store_append(jlist_store, &iter);
		set_jlist_store(jcnt, &iter);
	}
}

void	job_redisplay(void)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
	GtkTreeIter  iter;
	gdouble		cvalue;
	unsigned	jcnt;
	jobno_t		Cjobno = -1;
	netid_t		Chostno = 0;

	/*	Get currently selected job if any  */

	if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
		guint  seq;
		const  struct  spq  *jp;
		gtk_tree_model_get(GTK_TREE_MODEL(jlist_store), &iter, SEQ_COL, &seq, -1);
		jp = &Job_seg.jj_ptrs[seq]->j;
		Cjobno = jp->spq_job;
		Chostno = jp->spq_netid;
	}

	cvalue = gtk_adjustment_get_value(gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(jwid)));

	gtk_list_store_clear(jlist_store);
	readjoblist(1);

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		gtk_list_store_append(jlist_store, &iter);
		set_jlist_store(jcnt, &iter);
	}

	/* Re-find and select selected job if we had one */

	if  (Cjobno > 0)  {
		for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
			const  struct  spq  *jp = &Job_seg.jj_ptrs[jcnt]->j;
			if  (jp->spq_job == Cjobno  &&  jp->spq_netid == Chostno)  {
				if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(jlist_store), &iter, NULL, jcnt))
					gtk_tree_selection_select_iter(sel, &iter);
				break;
			}
		}
	}
}

/* Initial display of printers */

void	init_pdisplay(void)
{
	unsigned  pcnt;

	readptrlist(1);

	for  (pcnt = 0;  pcnt < Ptr_seg.nptrs;  pcnt++)  {
		GtkTreeIter   iter;
		gtk_list_store_append(unsorted_plist_store, &iter);
		set_plist_store(pcnt, &iter);
	}
}

void	ptr_redisplay(void)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwid));
	GtkTreeIter  iter;
	unsigned	pcnt;
	gdouble		cvalue;
	netid_t		Chostno = -1;
	slotno_t	Cslotno = -1;

	/*	Get currently selected job if any  */

	if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
		guint  seq;
		const  struct  spptr  *pp;
		gtk_tree_model_get(GTK_TREE_MODEL(sorted_plist_store), &iter, SEQ_COL, &seq, -1);
		pp = &Ptr_seg.pp_ptrs[seq]->p;
		Cslotno = pp->spp_rslot;
		Chostno = pp->spp_netid;
	}

	cvalue = gtk_adjustment_get_value(gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(pwid)));

	gtk_list_store_clear(unsorted_plist_store);
	readptrlist(1);

	for  (pcnt = 0;  pcnt < Ptr_seg.nptrs;  pcnt++)  {
		gtk_list_store_append(unsorted_plist_store, &iter);
		set_plist_store(pcnt, &iter);
	}

	/* Re-find and select selected printer if we had one */

	if  (Cslotno >= 0)  {
		for  (pcnt = 0;  pcnt < Ptr_seg.nptrs;  pcnt++)  {
			const  struct  spptr  *pp = &Ptr_seg.pp_ptrs[pcnt]->p;
			if  (pp->spp_netid == Chostno  &&  pp->spp_rslot == Cslotno)  {
				if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(unsorted_plist_store), &iter, NULL, pcnt))  {
					GtkTreeIter  siter;
					gtk_tree_model_sort_convert_child_iter_to_iter(GTK_TREE_MODEL_SORT(sorted_plist_store), &siter, &iter);
					gtk_tree_selection_select_iter(sel, &siter);
				}
				break;
			}
		}
	}
}

/* Structure to help us sort the columns into order from the fields */

struct	sortcols  {
	unsigned  colnum;	/* Column number that the view recognises */
	unsigned  fldnum;	/* Field number */
};

int	compare_cols(struct sortcols *a, struct sortcols * b)
{
	return  a->colnum < b->colnum? -1: a->colnum > b->colnum? 1: 0;
}

char  *gen_colorder(struct jplist_elems *lst, const int num)
{
	struct	sortcols	scl[40]; 		/* This is the most we currently need */
	struct	sortcols	*sp = scl, *fp;
	struct	jplist_elems	*lp;
	char	rbuf[40*3], dbuf[4];

	for  (lp = lst;  lp < &lst[num];  lp++)
		if  (lp->colnum >= 0)  {
			sp->colnum = lp->colnum;
			sp->fldnum = lp - lst;
			sp++;
		}

	qsort(QSORTP1 scl, sp - scl, sizeof(struct sortcols), QSORTP4 compare_cols);

	rbuf[0] = '\0';

	for  (fp = scl;  fp < sp;  fp++)  {
		sprintf(dbuf, ",%u", fp->fldnum);
		strcat(rbuf, dbuf);
	}
	return  stracpy(&rbuf[1]);
}

char *	gen_jfmts(void)
{
	return  gen_colorder(jlist_els, NUM_JOB_ELS);
}

char *	gen_pfmts(void)
{
	return  gen_colorder(plist_els, NUM_PTR_ELS);
}

static	char	*laststring;
static	char	last_wrap,
		last_isptr,
		last_match,
		srch_title = 1,
		srch_form = 1,
		srch_ptr = 1,
		srch_dev = 1,
		srch_user = 1;

struct	sparts  {
	GtkWidget  *stit, *sform, *sptr, *sdev, *suser;
};

/* Match routine taking just '.' as wild card
   FIXME give regex option sometime */

static  int  smatch_in(const char *field)
{
	const	char	*tp, *mp;

	while  (*field)  {
		tp = field;
		mp = laststring;
		while  (*mp)  {
			char	mch = *mp, fch = *tp;
			if  (!last_match)  {
				mch = toupper(mch);
				fch = toupper(fch);
			}
			if  (*mp != '.'  &&  mch != fch)
				goto  ng;
			mp++;
			tp++;
		}
		return  1;
	ng:
		field++;
	}
	return  0;
}

static int	foundin_job(const int jnum)
{
	const  struct  spq  *jp = &Job_seg.jj_ptrs[jnum]->j;

	if  (srch_title  &&  smatch_in(jp->spq_file))
		return  1;
	if  (srch_form  &&  smatch_in(jp->spq_form))
		return  1;
	if  (srch_ptr  &&  smatch_in(jp->spq_ptr))
		return  1;
	if  (srch_user  &&  smatch_in(jp->spq_uname))
		return  1;
	return  0;
}

static int	foundin_ptr(GtkTreeIter *iter)
{
	const  struct  spptr  *pp;
	guint  seq;

	gtk_tree_model_get(GTK_TREE_MODEL(sorted_plist_store), iter, SEQ_COL, &seq, -1);
	pp = &Ptr_seg.pp_ptrs[seq]->p;

	if  (srch_form  &&  smatch_in(pp->spp_form))
		return  1;
	if  (srch_ptr  &&  smatch_in(pp->spp_ptr))
		return  1;
	if  (srch_dev  &&  smatch_in(pp->spp_dev))
		return  1;
	return  0;
}

static void	jsearch_result(const int jnum)
{
	GtkTreeIter  iter;
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
	GtkTreePath  *path;

	gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(jlist_store), &iter, NULL, jnum);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(jlist_store), &iter);
	gtk_tree_selection_select_path(sel, path);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(jwid), path, NULL, FALSE, 0.0, 0.0);
	gtk_tree_path_free(path);
}

static void	execute_jsearch(const int isback)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
	GtkTreeIter  iter;
	int  jnum;

	/* Save messing about if no jobs there */

	if  (Job_seg.njobs == 0)  {
		doerror($EH{No jobs to select});
		return;
	}

	/* If we have a selected job start from just before or just after that one */

	if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
		guint  seq;

		/* Get which one from the sequence number */

		gtk_tree_model_get(GTK_TREE_MODEL(jlist_store), &iter, SEQ_COL, &seq, -1);
		jnum = seq;	/* Easier working from signed */

		if  (isback)  {
			for  (jnum--;  jnum >= 0;  jnum--)  {
				if  (foundin_job(jnum))  {
					jsearch_result(jnum);
					return;
				}
			}

			if  (last_wrap)  {
				for  (jnum = Job_seg.njobs-1;  jnum >= 0;  jnum--)  {
					if  (foundin_job(jnum))  {
						jsearch_result(jnum);
						return;
					}
				}
			}
		}
		else  {
			for  (jnum++;  (unsigned) jnum < Job_seg.njobs;  jnum++)  {
				if  (foundin_job(jnum))  {
					jsearch_result(jnum);
					return;
				}
			}
			if  (last_wrap)  {
				for  (jnum = 0;  (unsigned) jnum < Job_seg.njobs;  jnum++)  {
					if  (foundin_job(jnum))  {
						jsearch_result(jnum);
						return;
					}
				}
			}
		}
	}
	else  if  (isback)  {
		for  (jnum = Job_seg.njobs-1;  jnum >= 0;  jnum--)  {
			if  (foundin_job(jnum))  {
				jsearch_result(jnum);
				return;
			}
		}
	}
	else  {
		for  (jnum = 0;  (unsigned) jnum < Job_seg.njobs;  jnum++)  {
			if  (foundin_job(jnum))  {
				jsearch_result(jnum);
				return;
			}
		}
	}

	doerror(isback? $EH{Rsearch job not found}: $EH{Fsearch job not found});
}

static void	psearch_result(GtkTreePath *path)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwid));

	gtk_tree_selection_select_path(sel, path);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(jwid), path, NULL, FALSE, 0.0, 0.0);
	gtk_tree_path_free(path);
}

static void	execute_psearch(const int isback)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwid));
	GtkTreeIter  iter;
	GtkTreePath  *path;

	/* Save messing about if no printers there */

	if  (Ptr_seg.nptrs == 0)  {
		doerror($EH{No printer to select});
		return;
	}

	/* We have to use "iter" rather than the printer number in the array as the printer list might be sorted.
	   Some of these gyrations are because we don't have a "prev" operation on iterators
	   and you cant have "one past the end" iterator */

	if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {

		/* We had a selected printer so work on from there */

		path = gtk_tree_model_get_path(GTK_TREE_MODEL(sorted_plist_store), &iter);
		if  (isback)  {
			while  (gtk_tree_path_prev(path))  {
				gtk_tree_model_get_iter(GTK_TREE_MODEL(sorted_plist_store), &iter, path);
				if  (foundin_ptr(&iter))  {
					psearch_result(path); /* Calls gtk_tree_path_free(path); */
					return;
				}
			}
			if  (last_wrap)  {
				/* There doesn't seem to be a "get last" function and it would be
				   nice to have "one after the last" to make the loops look the same */
				gtk_tree_path_free(path);
				gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(sorted_plist_store), &iter, NULL, Ptr_seg.nptrs-1);
				path = gtk_tree_model_get_path(GTK_TREE_MODEL(sorted_plist_store), &iter);
				do  {
					gtk_tree_model_get_iter(GTK_TREE_MODEL(sorted_plist_store), &iter, path);
					if  (foundin_ptr(&iter))  {
						psearch_result(path); /* Calls gtk_tree_path_free(path); */
						return;
					}
				}  while  (gtk_tree_path_prev(path));
			}
		}
		else  {
			for  (;;)  {
				gtk_tree_path_next(path); /* Doesn't say whether valid */
				if  (!gtk_tree_model_get_iter(GTK_TREE_MODEL(sorted_plist_store), &iter, path))
					break;
				if  (foundin_ptr(&iter))  {
					psearch_result(path); /* Calls gtk_tree_path_free(path); */
					return;
				}
			}
			if  (last_wrap)  {
				gtk_tree_path_free(path);
				path = gtk_tree_path_new_first();
				while  (gtk_tree_model_get_iter(GTK_TREE_MODEL(sorted_plist_store), &iter, path))  {
					if  (foundin_ptr(&iter))  {
						psearch_result(path); /* Calls gtk_tree_path_free(path); */
						return;
					}
					gtk_tree_path_next(path);
				}
			}
		}
	}
	else  if  (isback)  {
		/* There doesn't seem to be a "get last" function */
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(sorted_plist_store), &iter, NULL, Ptr_seg.nptrs-1);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(sorted_plist_store), &iter);
		do  {
			gtk_tree_model_get_iter(GTK_TREE_MODEL(sorted_plist_store), &iter, path);
			if  (foundin_ptr(&iter))  {
				psearch_result(path); /* Calls gtk_tree_path_free(path); */
				return;
			}
		}  while  (gtk_tree_path_prev(path));
	}
	else  {
		path = gtk_tree_path_new_first();
		while  (gtk_tree_model_get_iter(GTK_TREE_MODEL(sorted_plist_store), &iter, path))  {
			if  (foundin_ptr(&iter))  {
				psearch_result(path); /* Calls gtk_tree_path_free(path); */
				return;
			}
			gtk_tree_path_next(path);
		}
	}
	doerror(isback? $EH{Rsearch ptr not found}: $EH{Fsearch ptr not found});
}

static void	execute_search(const int isback)
{
	if  (last_isptr)
		execute_psearch(isback);
	else
		execute_jsearch(isback);
}

static void	jorp_flip(GtkWidget *jorp, struct sparts *sp)
{
	if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jorp)))  {
		gtk_widget_set_sensitive(sp->sdev, TRUE);
		gtk_widget_set_sensitive(sp->stit, FALSE);
		gtk_widget_set_sensitive(sp->suser, FALSE);
	}
	else  {
		gtk_widget_set_sensitive(sp->sdev, FALSE);
		gtk_widget_set_sensitive(sp->stit, TRUE);
		gtk_widget_set_sensitive(sp->suser, TRUE);
	}
}

void	cb_search(void)
{
	GtkWidget  *dlg, *hbox, *lab, *sstringw, *jorp, *backw, *matchw, *wrapw;
	char	*pr;
	struct	sparts  sp;

	pr = gprompt($P{xspq search jplist});
	dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	free(pr);

	/* String to search for */

	hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
	pr = gprompt($P{xspq search str});
	lab = gtk_label_new(pr);
	free(pr);
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
	sstringw = gtk_entry_new();
	if  (laststring)
		gtk_entry_set_text(GTK_ENTRY(sstringw), laststring);
	gtk_box_pack_start(GTK_BOX(hbox), sstringw, FALSE, FALSE, DEF_DLG_HPAD);

	jorp = gprompt_checkbutton($P{xspq search jorp});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), jorp, FALSE, FALSE, DEF_DLG_VPAD);

	sp.stit = gprompt_checkbutton($P{xspq search title});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.stit, FALSE, FALSE, DEF_DLG_VPAD);

	sp.sform = gprompt_checkbutton($P{xspq search form});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.sform, FALSE, FALSE, DEF_DLG_VPAD);

	sp.sptr = gprompt_checkbutton($P{xspq search ptr});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.sptr, FALSE, FALSE, DEF_DLG_VPAD);

	sp.sdev = gprompt_checkbutton($P{xspq search dev});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.sdev, FALSE, FALSE, DEF_DLG_VPAD);

	sp.suser = gprompt_checkbutton($P{xspq search user});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.suser, FALSE, FALSE, DEF_DLG_VPAD);

	backw = gprompt_checkbutton($P{xspq search backw});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), backw, FALSE, FALSE, DEF_DLG_VPAD);

	wrapw = gprompt_checkbutton($P{xspq search wrap});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), wrapw, FALSE, FALSE, DEF_DLG_VPAD);

	matchw = gprompt_checkbutton($P{xspq search match});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), matchw, FALSE, FALSE, DEF_DLG_VPAD);

	if  (last_isptr)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(jorp), TRUE);
	if  (last_wrap)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrapw), TRUE);
	if  (last_match)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(matchw), TRUE);
	if  (srch_title)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.stit), TRUE);
	if  (srch_ptr)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.sptr), TRUE);
	if  (srch_form)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.sform), TRUE);
	if  (srch_dev)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.sdev), TRUE);
	if  (srch_user)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.suser), TRUE);

	/* Turn off inappropriate ones according to whether job or printer selected */

	if  (last_isptr)  {
		gtk_widget_set_sensitive(sp.stit, FALSE);
		gtk_widget_set_sensitive(sp.suser, FALSE);
	}
	else
		gtk_widget_set_sensitive(sp.sdev, FALSE);

	g_signal_connect(G_OBJECT(jorp), "toggled", G_CALLBACK(jorp_flip), (gpointer) &sp);

	gtk_widget_show_all(dlg);

	while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
		const  char  *news = gtk_entry_get_text(GTK_ENTRY(sstringw));
		gboolean  isprin = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jorp));
		gboolean  isback = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(backw));

		if  (strlen(news) == 0)  {
			doerror($EH{Null search string});
			continue;
		}
		if  (laststring)
			free(laststring);
		laststring = stracpy(news);
		srch_title = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.stit))? 1: 0;
		srch_ptr = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.sptr))? 1: 0;
		srch_form = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.sform))? 1: 0;
		srch_dev = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.sdev))? 1: 0;
		srch_user = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.suser))? 1: 0;
		last_isptr = isprin? 1: 0;
		last_wrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wrapw))? 1: 0;
		last_match = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(matchw))? 1: 0;
		if  (isprin)  {
			if  (!(srch_form || srch_ptr || srch_dev))  {
				doerror($EH{No ptr crits given});
				continue;
			}
		}
		else  if  (!(srch_title || srch_ptr || srch_form || srch_user))  {
			doerror($EH{No job crits given});
			continue;
		}
		execute_search(isback);
		break;
	}
	gtk_widget_destroy(dlg);
}

void	cb_rsearch(GtkAction *action)
{
	const	char	*act = gtk_action_get_name(action);

	if  (!laststring)  {
		doerror($EH{No previous search});
		return;
	}

	execute_search(act[0] == 'R');
}
