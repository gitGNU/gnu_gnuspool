/* xspuser.c -- GTK program to update user permissions

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
static	char	rcsid2[] = "@(#) $Revision: 1.1 $";
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
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
#include "spuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "xspu_ext.h"
#ifdef	SHAREDLIBS
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"
#include "displayopt.h"
#endif

FILE	*Cfile;

uid_t	Daemuid,
	Realuid,
	Effuid;

int	hchanges,	/* Had changes to default */
	uchanges;	/* Had changes to user(s) */

unsigned		Nusers;
extern	struct	sphdr	Spuhdr;
struct	spdet		*ulist;
static	char		*defhdr,
			*s_class,
			*ns_class,
			*lt_class,
			*gt_class,
			*s_perm,
			*ns_perm,
			*lt_perm,
			*gt_perm;

static	char	*urestrict;	/* Restrict  */

GtkWidget	*toplevel,	/* Main window */
		*dwid,		/* Default list */
		*uwid;		/* User scroll list */

GtkListStore		*raw_ulist_store;
GtkTreeModelSort	*ulist_store;

static void	cb_about(void);

static GtkActionEntry entries[] = {
	{ "FileMenu", NULL, "_File" },
	{ "DefMenu", NULL, "_Defaults" },
	{ "UserMenu", NULL, "_Users"  },
	{ "ChargeMenu", NULL, "_Charges"  },
	{ "HelpMenu", NULL, "_Help" },
	{ "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit and save", G_CALLBACK(gtk_main_quit)},
	{ "Dpri", NULL, "Default _pri", "<shift>P", "Set default priorities", G_CALLBACK(cb_pri)},
	{ "Dform", NULL, "Default _form", "<shift>F", "Set default form type new users", G_CALLBACK(cb_form)},
	{ "Dptr", NULL, "Default ptr", NULL, "Set default printer", G_CALLBACK(cb_ptr)},
	{ "Dclass", NULL, "Default _class code", "<shift>C", "Set default class code", G_CALLBACK(cb_class)},
	{ "Dpriv", NULL, "Default privileges", "<shift>V", "Set default privileges", G_CALLBACK(cb_priv)},
	{ "Copyall", NULL, "Copy all", NULL, "Copy to all users", G_CALLBACK(cb_copyall)},

	{ "upri", NULL, "_Priorities", NULL, "Set priorities", G_CALLBACK(cb_pri)},
	{ "uform", NULL, "_Form", "F", "Set default form type for users", G_CALLBACK(cb_form)},
	{ "uptr", NULL, "Ptinter", "P", "Set default printer for users", G_CALLBACK(cb_ptr)},
	{ "uclass", NULL, "Class code", "C", "Set class codes for users", G_CALLBACK(cb_class)},
	{ "upriv", NULL, "Privileges", "v", "Set privileges for users", G_CALLBACK(cb_priv)},
	{ "copydef", NULL, "Copy defaults", NULL, "Copy default privileges to selected users", G_CALLBACK(cb_copydef)},

	{ "dispcharge", NULL, "Display charges", NULL, "Display charges for users", G_CALLBACK(cb_charges)},
	{ "zerou", NULL, "Zero charges", NULL, "Zero charges for selected users", G_CALLBACK(cb_zerou)},
	{ "zeroall", NULL, "Zero all charges", NULL, "Zero charges for all users", G_CALLBACK(cb_zeroall)},
	{ "impose", NULL, "Impose fee", NULL, "Impose fee on selected users", G_CALLBACK(cb_impose)},

	{ "About", NULL, "About xspuser", NULL, "About xspuser", G_CALLBACK(cb_about)}  };

#ifdef	SHAREDLIBS
struct	jshm_info	Job_seg;
struct	pshm_info	Ptr_seg;
struct	xfershm		*Xfer_shmp;
int	Ctrl_chan;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif
DEF_DISPOPTS;
#endif

/* For when we run out of memory.....  */

void	nomem(void)
{
	fprintf(stderr, "Ran out of memory\n");
	exit(E_NOMEM);
}

char	*authlist[] =  { "John M Collins", NULL  };

static void	cb_about(void)
{
	GtkWidget  *dlg = gtk_about_dialog_new();
	char	*cp = strchr(rcsid2, ':');
	char	vbuf[20];

	if  (!cp)
		strcpy(vbuf, "Initial version");
	else  {
		char  *ep;
		cp++;
		ep = strchr(cp, '$');
		int  n = ep - cp;
		strncpy(vbuf, cp, n);
		vbuf[n] = '\0';
	}
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), vbuf);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dlg), "Xi Software Ltd 2008");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), "http://www.xisl.com");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg), (const char **) authlist);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

/*  Make top level window and set title and icon */

static void	winit(void)
{
	GError *err;
	char	*fn;

	toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(toplevel), 400, 400);
	fn = gprompt($P{xspuser app title});
	gtk_window_set_title(GTK_WINDOW(toplevel), fn);
	free(fn);
	gtk_container_set_border_width(GTK_CONTAINER(toplevel), 5);
	fn = envprocess(XSPUSER_ICON);
	gtk_window_set_default_icon_from_file(fn, &err);
	free(fn);
	gtk_window_set_resizable(GTK_WINDOW(toplevel), TRUE);
	g_signal_connect(G_OBJECT(toplevel), "delete_event", G_CALLBACK(gtk_false), NULL);
	g_signal_connect(G_OBJECT(toplevel), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

gint sort_userid(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	guint	id1, id2;
        gtk_tree_model_get(model, a, UID_COL, &id1, -1);
        gtk_tree_model_get(model, b, UID_COL, &id2, -1);
	return  id1 < id2? -1:  id1 == id2? 0: 1;
}

gint sort_username(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
        gchar	*name1, *name2;
	gint	ret = 0;

        gtk_tree_model_get(model, a, USNAM_COL, &name1, -1);
        gtk_tree_model_get(model, b, USNAM_COL, &name2, -1);

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

void	set_sort_col(int colnum)
{
	GtkTreeSortable *sortable = GTK_TREE_SORTABLE(ulist_store);
	GtkSortType      order;
	gint             sortid;

	if  (gtk_tree_sortable_get_sort_column_id(sortable, &sortid, &order) == TRUE  &&  sortid == colnum)  {
		GtkSortType  neworder;
		neworder = (order == GTK_SORT_ASCENDING)? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
		gtk_tree_sortable_set_sort_column_id(sortable, sortid, neworder);
	}
	else
		gtk_tree_sortable_set_sort_column_id(sortable, colnum, GTK_SORT_ASCENDING);
}

GtkWidget  *wstart(void)
{
	char	*mf;
	GError *err;
	GtkActionGroup *actions;
	GtkUIManager *ui;
	GtkTreeSelection    *sel;
	GtkWidget  *vbox;
	int	cnt;

	actions = gtk_action_group_new("Actions");
	gtk_action_group_add_actions(actions, entries, G_N_ELEMENTS(entries), NULL);
	ui = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui, actions, 0);
	gtk_window_add_accel_group(GTK_WINDOW(toplevel), gtk_ui_manager_get_accel_group(ui));
	mf = envprocess(XSPUSER_MENU);
	if  (!gtk_ui_manager_add_ui_from_file(ui, mf, &err))  {
		g_message("Menu build failed");
		exit(99);
	}
	free(mf);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(toplevel), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui, "/MenuBar"), FALSE, FALSE, 0);

	/*  Create user display treeview */

	uwid = gtk_tree_view_new();
	for  (cnt = 0;  cnt <= PRIV_COL-USNAM_COL;  cnt++)  {
		GtkCellRenderer     *renderer = gtk_cell_renderer_text_new();
		char	*msg = gprompt($P{xspuser user hdr}+cnt);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(uwid), -1, msg, renderer, "text", cnt+USNAM_COL, NULL);
		free(msg);
	}

	raw_ulist_store = gtk_list_store_new(11,
					 G_TYPE_UINT, /* Index number we don't display */
					 G_TYPE_UINT, /* User id which we don't display */
					 G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT,
					 G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING,
					 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	ulist_store = (GtkTreeModelSort *) gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(raw_ulist_store));

	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ulist_store), USNAM_COL, sort_username, NULL, NULL);
	for  (cnt = DEFPRI_COL;  cnt <= PRIV_COL;  cnt++)
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ulist_store), cnt, sort_userid, NULL, NULL);

	/* set initial sort order */

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ulist_store), DEFPRI_COL, GTK_SORT_ASCENDING);
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(uwid), TRUE);

	for  (cnt = 0;  cnt < 9;  cnt++)  {
		GtkTreeViewColumn   *col = gtk_tree_view_get_column(GTK_TREE_VIEW(uwid), cnt);
		g_signal_connect_swapped(G_OBJECT(col), "clicked", G_CALLBACK(set_sort_col), GINT_TO_POINTER(cnt+USNAM_COL));
	}

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(uwid));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

	/* Bit for display of default */

	dwid = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(dwid), FALSE);

	/* Get all the strings now before we start to display them */

	s_class = gprompt($P{Class std});
	ns_class = gprompt($P{Non std class});
	lt_class = gprompt($P{Class less than});
	gt_class = gprompt($P{Class greater than});
	s_perm = gprompt($P{Perm std});
	ns_perm = gprompt($P{Non std perm});
	lt_perm = gprompt($P{Perm less than});
	gt_perm = gprompt($P{Perm greater than});
	defhdr = gprompt($P{Spuser default string});
	return  vbox;
}

static void	wcomplete(GtkWidget * vbox)
{
	GtkWidget  *paned, *scroll;

	gtk_tree_view_set_model(GTK_TREE_VIEW(uwid), GTK_TREE_MODEL(ulist_store));
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), uwid);
	paned = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
	gtk_paned_pack1(GTK_PANED(paned), scroll, TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(paned), dwid, FALSE, TRUE);
	gtk_widget_show_all(toplevel);
}

void	defdisplay(void)
{
	char	buf[300];
	snprintf(buf, sizeof(buf), "%s Def pri %d min %d max %d\nMax copies %d\nDef form: %s\nDef ptr: %s",
		 defhdr, Spuhdr.sph_defp, Spuhdr.sph_minp, Spuhdr.sph_maxp, Spuhdr.sph_cps, Spuhdr.sph_form, Spuhdr.sph_ptr);
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(dwid)), buf, -1);
}

static  char *get_cmsg(struct spdet *uitem)
{
	char	*msg;
	classcode_t	exclc;

	msg = s_class;
	exclc = uitem->spu_class ^ Spuhdr.sph_class;
	if  (exclc != 0)  {
		msg = ns_class;
		if  ((exclc & Spuhdr.sph_class) == 0)
			msg = gt_class;
		else  if  ((exclc & ~Spuhdr.sph_class) == 0)
			msg = lt_class;
	}
	return  msg;
}

static char  *get_pmsg(struct spdet *uitem)
{
	char	*pmsg;
	ULONG		exclp;

	pmsg = s_perm;
	exclp = uitem->spu_flgs ^ Spuhdr.sph_flgs;
	if  (exclp != 0)  {
		pmsg = ns_perm;
		if  ((exclp & Spuhdr.sph_flgs) == 0)
			pmsg = gt_perm;
		else  if  ((exclp & ~Spuhdr.sph_flgs) == 0)
			pmsg = lt_perm;
	}
	return  pmsg;
}

void	upd_udisp(struct spdet *uitem, GtkTreeIter *iter)
{
	gtk_list_store_set(raw_ulist_store, iter,
			   DEFPRI_COL,	(guint) uitem->spu_defp,
			   MINPRI_COL,  (guint) uitem->spu_minp,
			   MAXPRI_COL,  (guint) uitem->spu_maxp,
			   COPIES_COL,  (guint) uitem->spu_cps,
			   DEFFORM_COL, uitem->spu_form,
			   DEFPTR_COL,	uitem->spu_ptr,
			   CLASS_COL,	get_cmsg(uitem),
			   PRIV_COL,	get_pmsg(uitem),
			   -1);
}

void	update_all_users(void)
{
	GtkTreeIter  iter;
	gboolean  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(raw_ulist_store), &iter);

	while  (valid)  {
		unsigned  indx;
		gtk_tree_model_get(GTK_TREE_MODEL(raw_ulist_store), &iter, INDEX_COL, &indx, -1);
		upd_udisp(&ulist[indx], &iter);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(raw_ulist_store), &iter);
	}
}

void	update_selected_users(void)
{
	GtkTreeSelection *selection;
	GList  *pu, *nxt;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(uwid));
	pu = gtk_tree_selection_get_selected_rows(selection, NULL);
	for  (nxt = g_list_first(pu);  nxt;  nxt = g_list_next(nxt))  {
		GtkTreeIter  iter, citer;
		unsigned  indx;
		if  (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ulist_store), &iter, (GtkTreePath *)(nxt->data)))
			continue;
		gtk_tree_model_get(GTK_TREE_MODEL(ulist_store), &iter, INDEX_COL, &indx, -1);
		gtk_tree_model_sort_convert_iter_to_child_iter(ulist_store, &citer, &iter);
		upd_udisp(&ulist[indx], &citer);
	}
	g_list_foreach(pu, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(pu);
}

/* After changing default class codes or permissions we must update
   the corresponding user fields */

void	redispallu(void)
{
	GtkTreeIter   iter;
	gboolean      isnext;

	isnext = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(raw_ulist_store), &iter);

	while  (isnext)  {
		guint  indx;
		gtk_tree_model_get(GTK_TREE_MODEL(raw_ulist_store), &iter, INDEX_COL, &indx, -1);
		gtk_list_store_set(raw_ulist_store, &iter,
				   CLASS_COL,	get_cmsg(&ulist[indx]),
				   PRIV_COL,	get_pmsg(&ulist[indx]),
				   -1);
		isnext = gtk_tree_model_iter_next(GTK_TREE_MODEL(raw_ulist_store), &iter);
	}
}

static void	udisplay(void)
{
	unsigned  ucnt;
	GtkTreeIter   iter;

	for  (ucnt = 0;  ucnt < Nusers;  ucnt++)  {

		/* Add reow to store.
		   Put in index and uid which udisp doesn't do */

		gtk_list_store_append(raw_ulist_store, &iter);
		gtk_list_store_set(raw_ulist_store, &iter,
				   INDEX_COL, (guint) ucnt,
				   UID_COL, (guint) ulist[ucnt].spu_user,
				   USNAM_COL, prin_uname((uid_t) ulist[ucnt].spu_user),
				   -1);
		upd_udisp(&ulist[ucnt], &iter);
	}
}

/* Ye olde main routine.  */

MAINFN_TYPE	main(int argc, char **argv)
{
	struct	spdet	*mypriv;
	GtkWidget  *vbox;

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	Realuid = getuid();
	Effuid = geteuid();
	if  ((LONG) (Daemuid = lookup_uname(SPUNAME)) == UNKNOWN_UID)
		Daemuid = ROOTID;
	Cfile = open_cfile("XSPUSERCONF", "xmspuser.help");

#ifdef	HAVE_SETREUID
	setreuid(Daemuid, Daemuid);
#else
	setuid(Daemuid);
#endif
	gtk_init(&argc, &argv);

	if  (argc > 1  &&  strcmp(argv[1], "*") != 0)
		urestrict = stracpy(argv[1]);
	winit();

	if  (!(mypriv = getspuentry(Realuid)))  {
		doerror($EH{Not registered yet});
		exit(E_UNOTSETUP);
	}
	if  (!(mypriv->spu_flgs & PV_ADMIN))  {
		doerror($EH{No admin file permission});
		exit(E_NOPRIV);
	}
	if  (spu_needs_rebuild && Confirm($PH{Xmspuser confirm rebuild}))  {
		char  *name = envprocess(DUMPPWFILE);
		int	wuz = access(name, 0);
		if  (wuz >= 0)  {
			un_rpwfile();
			unlink(name);
		}
		free(name);
		rebuild_spufile();
		if  (wuz >= 0)
			dump_pwfile();
		produser();
	}
	ulist = getspulist(&Nusers);

	vbox = wstart();

	/* Chop down list to ones we want */

	if  (Nusers != 0  &&  urestrict)  {
		unsigned   cnt, nucnt = 0;
		struct  spdet  *cp, *np;
		struct  spdet  *newulist = (struct spdet *) malloc(Nusers * sizeof(struct spdet));
		if  (!newulist)
			nomem();
		cp = ulist;
		np = newulist;
		for  (cnt = 0;  cnt < Nusers;  cp++, cnt++)  {
			char	*un = prin_uname((uid_t) cp->spu_user);
			if  (!qmatch(urestrict, un))
				continue;
			*np++ = *cp;
			nucnt++;
		}
		free((char *) ulist);
		ulist = newulist;
		Nusers = nucnt;
	}

	defdisplay();
	udisplay();
	wcomplete(vbox);
	gtk_main();
	if  (uchanges)
		putspulist(ulist, Nusers, hchanges);
	else  if  (hchanges)
		putspulist((struct spdet *) 0, 0, hchanges);
	return  0;
}
