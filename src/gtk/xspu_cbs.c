/* xspu_cbs.c -- various callback routines for xspuser

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
#include <errno.h>
#include <gtk/gtk.h>
#include "defaults.h"
#include "spuser.h"
#include "files.h"
#include "ecodes.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "stringvec.h"
#include "xspu_ext.h"
#include "gtk_lib.h"
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif

#define DLG_PRI_PAGE    0
#define DLG_FORM_PAGE   1
#define DLG_PTR_PAGE    2
#define DLG_CC_PAGE     3
#define DLG_PRIV_PAGE   4

#define DEF_DLG_VPAD    5
#define DEF_DLG_HPAD    5
#define LAB_PADDING     10
#define NOTE_PADDING    10

#define DEFAULT_CHG_WIDTH       200
#define DEFAULT_CHG_HEIGHT      300

#define MAX_UBUFF       80      /* FIXME */
#define MAX_UDISP       3

static  unsigned        *pendulist;
static  int             pendunum;

struct  dialog_data  {
        GtkWidget  *minpri, *defpri, *maxpri, *copies;
        GtkWidget  *cprio, *anyprio, *cdeflt; /* These are copies of priv bits on prio dialog */
        GtkWidget  *form, *ptr, *aform, *aptr;
        GtkWidget  *classcodes[32];
        GtkWidget  *override;   /* Copy of override class priv bit on cc dialog */
        GtkWidget  *privs[NUM_PRIVS];
};

#ifdef  NOTYET

static  GtkWidget  *msgwin;

void    dest_busy()
{
        gtk_widget_destroy(msgwin);
        msgwin = NULL;
}

void    displaybusy(const int on)
{
        if  (msgwin)  {
                if  (!on)
                        dest_busy();
                return;
        }
        if  (on)  {
                char    **evec = helpvec($E{Rebuilding spufile wait}, 'E');
                char    *msg = makebigvec(evec);
                msgwin = gtk_message_dialog_new(GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", msg);
                free(msg);
                g_signal_connect_swapped(msgwin, "response", G_CALLBACK(dest_busy), msgwin);
                gtk_widget_show_all(toplevel);
        }
}
#endif

extern  GtkWidget *findwidfromptab(struct dialog_data *, const ULONG);

/* Test it's not on before we force it on otherwise we could get an endless loop
   of signals */

void setifnotset(GtkWidget *wid)
{
        if  (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid), TRUE);
}

/* Ditto for unsetting */

void unsetifset(GtkWidget *wid)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid), FALSE);
}

void admin_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))  {
                int     cnt;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->cprio), TRUE);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->anyprio), TRUE);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->cdeflt), TRUE);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->override), TRUE);

                for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)
                        setifnotset(ddata->privs[cnt]);
        }
}

void cover_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))  {
                if  (wid == ddata->override)
                        setifnotset(findwidfromptab(ddata, PV_COVER));
                else
                        setifnotset(ddata->override);
        }
        else  {
                if  (wid == ddata->override)
                        unsetifset(findwidfromptab(ddata, PV_COVER));
                else
                        unsetifset(ddata->override);
        }
}

void cprio_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))  {
                if  (wid == ddata->cprio)
                        setifnotset(findwidfromptab(ddata, PV_CPRIO));
                else
                        setifnotset(ddata->cprio);
        }
        else  {
                if  (wid == ddata->cprio)
                        unsetifset(findwidfromptab(ddata, PV_CPRIO));
                else
                        unsetifset(ddata->cprio);
                unsetifset(ddata->anyprio);
                unsetifset(findwidfromptab(ddata, PV_ANYPRIO));
        }
}

void anyprio_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))  {
                if  (wid == ddata->anyprio)
                        setifnotset(findwidfromptab(ddata, PV_ANYPRIO));
                else
                        setifnotset(ddata->anyprio);
                setifnotset(ddata->cprio);
                setifnotset(findwidfromptab(ddata, PV_CPRIO));
        }
        else  {
                if  (wid == ddata->anyprio)
                        unsetifset(findwidfromptab(ddata, PV_ANYPRIO));
                else
                        unsetifset(ddata->anyprio);
        }
}

void vother_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                unsetifset(findwidfromptab(ddata, PV_OTHERJ));
}

void othj_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                setifnotset(findwidfromptab(ddata, PV_VOTHERJ));
}


void prinq_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))  {
                unsetifset(findwidfromptab(ddata, PV_ADDDEL));
                unsetifset(findwidfromptab(ddata, PV_HALTGO));
        }
}

void haltp_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                setifnotset(findwidfromptab(ddata, PV_PRINQ));
        else
                unsetifset(findwidfromptab(ddata, PV_ADDDEL));
}

void addp_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))  {
                setifnotset(findwidfromptab(ddata, PV_HALTGO));
                setifnotset(findwidfromptab(ddata, PV_PRINQ));
        }
}

void cdeflt_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))  {
                if  (wid == ddata->cdeflt)
                        setifnotset(findwidfromptab(ddata, PV_CDEFLT));
                else
                        setifnotset(ddata->cdeflt);
        }
        else  {
                if  (wid == ddata->cdeflt)
                        unsetifset(findwidfromptab(ddata, PV_CDEFLT));
                else
                        unsetifset(ddata->cdeflt);
        }
}

void access_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                unsetifset(findwidfromptab(ddata, PV_FREEZEOK));
}

void freeze_changed(GtkWidget *wid, struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                setifnotset(findwidfromptab(ddata, PV_ACCESSOK));
}

struct  perm    {
        int     number;
        char    *string;
        ULONG   setbit;
        void    (*callback)(GtkWidget *, struct dialog_data *);
}  ptab[] = {
        { $P{Priv descr edit admin file},       (char *) 0,     PV_ADMIN,       admin_changed  },
        { $P{Priv descr override class},        (char *) 0,     PV_COVER,       cover_changed  },
        { $P{Priv descr masquerade user},       (char *) 0,     PV_MASQ,        NULL  },
        { $P{Priv descr stop spooler},          (char *) 0,     PV_SSTOP,       NULL  },
        { $P{Priv descr use other forms},       (char *) 0,     PV_FORMS,       NULL  },
        { $P{Priv descr use other ptrs},        (char *) 0,     PV_OTHERP,      NULL  },
        { $P{Priv descr change priority on Q},  (char *) 0,     PV_CPRIO,       cprio_changed  },
        { $P{Priv descr any priority Q},        (char *) 0,     PV_ANYPRIO,     anyprio_changed  },
        { $P{Priv descr view other jobs},       (char *) 0,     PV_VOTHERJ,     vother_changed  },
        { $P{Priv descr edit other users jobs}, (char *) 0,     PV_OTHERJ,      othj_changed  },
        { $P{Priv descr unqueue jobs},          (char *) 0,     PV_UNQUEUE,     NULL  },
        { $P{Priv descr remote jobs},           (char *) 0,     PV_REMOTEJ,     NULL  },
        { $P{Priv descr remote printers},       (char *) 0,     PV_REMOTEP,     NULL  },
        { $P{Priv descr select printer list},   (char *) 0,     PV_PRINQ,       prinq_changed  },
        { $P{Priv descr halt printers},         (char *) 0,     PV_HALTGO,      haltp_changed  },
        { $P{Priv descr add printers},          (char *) 0,     PV_ADDDEL,      addp_changed  },
        { $P{Priv descr own defaults},          (char *) 0,     PV_CDEFLT,      cdeflt_changed  },
        { $P{Priv descr access queue},          (char *) 0,     PV_ACCESSOK,    access_changed  },
        { $P{Priv descr freeze options},        (char *) 0,     PV_FREEZEOK,    freeze_changed  }
};

GtkWidget  *findwidfromptab(struct dialog_data *ddata, const ULONG fl)
{
        int     cnt;
        for  (cnt = 0;  cnt < sizeof(ptab)/sizeof(struct perm);  cnt++)
                if  (fl & ptab[cnt].setbit)
                        return  ddata->privs[cnt];
        _exit(E_SETUP);
}

void priv_setdef(struct dialog_data *ddata)
{
        int     cnt;

        for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)
                if  (Spuhdr.sph_flgs & ptab[cnt].setbit)
                        setifnotset(ddata->privs[cnt]);
                else
                        unsetifset(ddata->privs[cnt]);
}

void cc_setall(struct dialog_data *ddata)
{
        int     cnt;
        for  (cnt = 0;  cnt < 32;  cnt++)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->classcodes[cnt]), TRUE);
}

void cc_clearall(struct dialog_data *ddata)
{
        int     cnt;
        for  (cnt = 0;  cnt < 32;  cnt++)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->classcodes[cnt]), FALSE);
}

static int getselectedusers(const int moan)
{
        GtkTreeSelection *selection;

        /*  Free up last time's */

        if  (pendulist)  {
                free((char *) pendulist);
                pendulist = NULL;
        }

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(uwid));

        if  ((pendunum = gtk_tree_selection_count_selected_rows(selection)) > 0)  {
                GList *pu = gtk_tree_selection_get_selected_rows(selection, NULL);
                GList *nxt;
                unsigned  *n;
                n = pendulist = malloc((unsigned)(pendunum * sizeof(unsigned *)));
                if  (!pendulist)
                        nomem();
                for  (nxt = g_list_first(pu);  nxt;  nxt = g_list_next(nxt))  {
                        GtkTreeIter  iter;
                        unsigned  indx;
                        if  (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ulist_store), &iter, (GtkTreePath *)(nxt->data)))
                                continue;
                        gtk_tree_model_get(GTK_TREE_MODEL(ulist_store), &iter, INDEX_COL, &indx, -1);
                        *n++ = indx;
                }
                g_list_foreach(pu, (GFunc) gtk_tree_path_free, NULL);
                g_list_free(pu);
                return  1;
        }
        if  (moan)
                doerror($EH{No users selected});
        return  0;
}

/* Create priorities and copies dialog box (as part of notebook) */

GtkWidget *create_pri_dialog(struct dialog_data *ddata, const unsigned minp, const unsigned defp, const unsigned maxp, const unsigned cps, const ULONG priv)
{
        GtkWidget  *frame, *vbox, *hbox, *vbox2, *lab;
        char    *pr;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspuser framelab pricps});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*3);
        vbox2 = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);

        lab = gprompt_label($P{xspuser pcdlg minp});
        gtk_box_pack_start(GTK_BOX(vbox2), lab, TRUE, FALSE, 0);

        lab = gprompt_label($P{xspuser pcdlg defp});
        gtk_box_pack_start(GTK_BOX(vbox2), lab, TRUE, FALSE, 0);

        lab = gprompt_label($P{xspuser pcdlg maxp});
        gtk_box_pack_start(GTK_BOX(vbox2), lab, TRUE, FALSE, 0);

        lab = gprompt_label($P{xspuser pcdlg cps});
        gtk_box_pack_start(GTK_BOX(vbox2), lab, TRUE, FALSE, 0);

        vbox2 = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);
        ddata->minpri = gtk_hscale_new_with_range(1.0, 255.0, 1.0);
        gtk_scale_set_digits(GTK_SCALE(ddata->minpri), 0);
        gtk_range_set_value(GTK_RANGE(ddata->minpri), (gdouble) minp);
        gtk_box_pack_start(GTK_BOX(vbox2), ddata->minpri, TRUE, TRUE, 0);
        ddata->defpri = gtk_hscale_new_with_range(1.0, 255.0, 1.0);
        gtk_scale_set_digits(GTK_SCALE(ddata->defpri), 0);
        gtk_range_set_value(GTK_RANGE(ddata->defpri), (gdouble) defp);
        gtk_box_pack_start(GTK_BOX(vbox2), ddata->defpri, TRUE, TRUE, 0);
        ddata->maxpri = gtk_hscale_new_with_range(1.0, 255.0, 1.0);
        gtk_scale_set_digits(GTK_SCALE(ddata->maxpri), 0);
        gtk_range_set_value(GTK_RANGE(ddata->maxpri), (gdouble) maxp);
        gtk_box_pack_start(GTK_BOX(vbox2), ddata->maxpri, TRUE, TRUE, 0);
        ddata->copies = gtk_hscale_new_with_range(0.0, 255.0, 1.0);
        gtk_scale_set_digits(GTK_SCALE(ddata->copies), 0);
        gtk_range_set_value(GTK_RANGE(ddata->copies), (gdouble) cps);
        gtk_box_pack_start(GTK_BOX(vbox2), ddata->copies, TRUE, TRUE, 0);

        /* Add copies of privilege checkboxes related to priority */

        ddata->cprio = gprompt_checkbutton($P{Priv descr change priority on Q});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->cprio, FALSE, FALSE, 0);
        if  (priv & PV_CPRIO)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->cprio), TRUE);
        g_signal_connect(ddata->cprio, "toggled", G_CALLBACK(cprio_changed), ddata);

        ddata->anyprio = gprompt_checkbutton($P{Priv descr any priority Q});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->anyprio, FALSE, FALSE, 0);
        if  (priv & PV_ANYPRIO)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->anyprio), TRUE);
        g_signal_connect(ddata->anyprio, "toggled", G_CALLBACK(anyprio_changed), ddata);

        ddata->cdeflt = gprompt_checkbutton($P{Priv descr own defaults});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->cdeflt, FALSE, FALSE, 0);
        if  (priv & PV_CDEFLT)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->cdeflt), TRUE);
        g_signal_connect(ddata->cdeflt, "toggled", G_CALLBACK(cdeflt_changed), ddata);
        return  frame;
}

GtkWidget *create_form_dialog(struct dialog_data *ddata, const char *eform, const char *eforma)
{
        struct  stringvec  possforms, possallow;
        unsigned  cnt;
        GtkWidget       *frame, *vbox, *hbox, *lab;
        char    *pr;

        /* Build up vector of possible form types and possible possible ones */

        stringvec_init(&possforms);
        stringvec_init(&possallow);
        stringvec_insert_unique(&possforms, Spuhdr.sph_form);
        stringvec_insert_unique(&possallow, Spuhdr.sph_form);
        stringvec_insert_unique(&possallow, Spuhdr.sph_formallow);
        for  (cnt = 0;  cnt < Npwusers;  cnt++)  {
                stringvec_insert_unique(&possforms, ulist[cnt].spu_form);
                stringvec_insert_unique(&possallow, ulist[cnt].spu_form);
                stringvec_insert_unique(&possallow, ulist[cnt].spu_formallow);
        }

        frame = gtk_frame_new(NULL);

        pr = gprompt($P{xspuser framelab forms});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        lab = gprompt_label($P{xspuser fmdlg deffm});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        ddata->form = gtk_combo_box_entry_new_text();
        for  (cnt = 0;  cnt < stringvec_count(possforms);  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->form), stringvec_nth(possforms, cnt));
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->form))), eform);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->form, FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        lab = gprompt_label($P{xspuser fmdlg allfm});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        ddata->aform = gtk_combo_box_entry_new_text();
        for  (cnt = 0;  cnt < stringvec_count(possallow);  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->aform), stringvec_nth(possallow, cnt));
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->aform))), eforma);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->aform, FALSE, FALSE, 0);

        stringvec_free(&possforms);
        stringvec_free(&possallow);

        return  frame;
}

GtkWidget *create_ptr_dialog(struct dialog_data *ddata, const char *eptr, const char *eptra)
{
        struct  stringvec  possptrs, possallow;
        unsigned  cnt;
        GtkWidget       *frame, *vbox, *hbox, *lab;
        char    *pr;

        /* Build up vector of possible ptr types and possible possible ones
           Possible possible ones include ones we're thinking of for printer types*/

        stringvec_init(&possptrs);
        stringvec_init(&possallow);
        stringvec_insert_unique(&possptrs, Spuhdr.sph_ptr);
        stringvec_insert_unique(&possallow, Spuhdr.sph_ptr);
        stringvec_insert_unique(&possallow, Spuhdr.sph_ptrallow);
        for  (cnt = 0;  cnt < Npwusers;  cnt++)  {
                stringvec_insert_unique(&possptrs, ulist[cnt].spu_ptr);
                stringvec_insert_unique(&possallow, ulist[cnt].spu_ptr);
                stringvec_insert_unique(&possallow, ulist[cnt].spu_ptrallow);
        }

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspuser framelab printers});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        lab = gprompt_label($P{xspuser ptrdlg defptr});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        ddata->ptr = gtk_combo_box_entry_new_text();
        for  (cnt = 0;  cnt < stringvec_count(possptrs);  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->ptr), stringvec_nth(possptrs, cnt));
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->ptr))), eptr);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->ptr, FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        lab = gprompt_label($P{xspuser ptrdlg allptr});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        ddata->aptr = gtk_combo_box_entry_new_text();
        for  (cnt = 0;  cnt < stringvec_count(possallow);  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->aptr), stringvec_nth(possallow, cnt));
        gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->aptr))), eptra);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->aptr, FALSE, FALSE, 0);
        stringvec_free(&possptrs);
        stringvec_free(&possallow);
        return  frame;
}

GtkWidget *create_cc_dialog(struct dialog_data *ddata, const classcode_t cc, const ULONG priv)
{
        GtkWidget       *frame, *vbox, *hbox, *button;
        int             rcnt, ccnt, bitnum;
        char            *pr, bname[2];

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspuser framelab ccodes});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD*2);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        pr = gprompt($P{xspuser ccdlg setall});
        button = gtk_button_new_with_label(pr);
        free(pr);
        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(cc_setall), ddata);

        pr = gprompt($P{xspuser ccdlg clearall});
        button = gtk_button_new_with_label(pr);
        free(pr);
        gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(cc_clearall), ddata);

        bitnum = 0;
        bname[1] = '\0';

        for  (rcnt = 0;  rcnt < 4;  rcnt++)  {
                hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
                gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

                for  (ccnt = 0;  ccnt < 4;  ccnt++)  {
                        bname[0] = 'A' + bitnum;
                        button = ddata->classcodes[bitnum] = gtk_check_button_new_with_label(bname);
                        if  (cc & (1 << bitnum))
                                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
                        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
                        bitnum++;
                }
        }

        for  (rcnt = 0;  rcnt < 4;  rcnt++)  {
                hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
                gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

                for  (ccnt = 0;  ccnt < 4;  ccnt++)  {
                        bname[0] = 'a' + bitnum - 16;
                        button = ddata->classcodes[bitnum] = gtk_check_button_new_with_label(bname);
                        if  (cc & (1 << bitnum))
                                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
                        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
                        bitnum++;
                }
        }

        ddata->override = gprompt_checkbutton($P{Priv descr override class});
        if  (priv & PV_COVER)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->override), TRUE);
        g_signal_connect(G_OBJECT(ddata->override), "toggled", G_CALLBACK(cover_changed), ddata);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->override, FALSE, FALSE, 0);

        return  frame;
}

GtkWidget *create_priv_dialog(struct dialog_data *ddata, const ULONG priv, const int isuser)
{
        static  int     expanded = 0;
        int     cnt;
        GtkWidget       *frame, *vbox, *button;
        char    *pr;

        /* Get messages if we haven't got them already */

        if  (!expanded)  {
                for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)
                        ptab[cnt].string = gprompt(ptab[cnt].number);
                expanded = 1;
        }

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspuser framelab privs});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)  {
                button = gtk_check_button_new_with_label(ptab[cnt].string);
                if  (priv & ptab[cnt].setbit)
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
                if  (ptab[cnt].callback)
                        g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(ptab[cnt].callback), ddata);
                gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
                ddata->privs[cnt] = button;
        }

        if  (isuser)  {
                pr = gprompt($P{xspuser priv setdef});
                button = gtk_button_new_with_label(pr);
                free(pr);
                g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(priv_setdef), ddata);
                gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
        }

        return  frame;
}

GtkWidget *user_lab(const int isusers, const char *msg)
{
        GtkWidget  *lab;
        if  (isusers)  {
                int     cnt, mxu = pendunum;
                char    buf[MAX_UBUFF];
                if  (mxu > MAX_UDISP)
                        mxu = MAX_UDISP;
                strcpy(buf, msg);
                for  (cnt = 0;  cnt < mxu;  cnt++)  {
                        if  (cnt != 0)
                                strcat(buf, ",");
                        strcat(buf, prin_uname(ulist[pendulist[cnt]].spu_user));
                }
                if  (mxu != pendunum)
                        strcat(buf, "...");
                lab = gtk_label_new(buf);
        }
        else
                lab = gprompt_label($P{xspuser deflab});

        return  lab;
}

GtkWidget *create_dialog(struct dialog_data *ddata, const int wpage, const int isusers)
{
        GtkWidget  *dlg, *lab, *notebook, *pridlg, *formdlg, *ptrdlg, *ccdlg, *privdlg;
        char    *pr;

        pr = gprompt(isusers? $P{xspuser udlgtit}: $P{xspuser ddlgtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), user_lab(isusers, "Editing user params: "), FALSE, FALSE, LAB_PADDING);
        notebook = gtk_notebook_new();
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

        if  (isusers)  {
                struct  spdet  *up = &ulist[pendulist[0]];
                pridlg = create_pri_dialog(ddata, up->spu_minp, up->spu_defp, up->spu_maxp, up->spu_cps, up->spu_flgs);
                formdlg = create_form_dialog(ddata, up->spu_form, up->spu_formallow);
                ptrdlg = create_ptr_dialog(ddata, up->spu_ptr, up->spu_ptrallow);
                ccdlg = create_cc_dialog(ddata, up->spu_class, up->spu_class);
                privdlg = create_priv_dialog(ddata, up->spu_flgs, 1);
        }
        else  {
                pridlg = create_pri_dialog(ddata, Spuhdr.sph_minp, Spuhdr.sph_defp, Spuhdr.sph_maxp, Spuhdr.sph_cps, Spuhdr.sph_flgs);
                formdlg = create_form_dialog(ddata, Spuhdr.sph_form, Spuhdr.sph_formallow);
                ptrdlg = create_ptr_dialog(ddata, Spuhdr.sph_ptr, Spuhdr.sph_ptrallow);
                ccdlg = create_cc_dialog(ddata, Spuhdr.sph_class, Spuhdr.sph_class);
                privdlg = create_priv_dialog(ddata, Spuhdr.sph_flgs, 0);
        }

        /* Set up tab names for notebook */

        lab = gprompt_label($P{xspuser frametab pricps});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pridlg, lab);

        lab = gprompt_label($P{xspuser frametab forms});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), formdlg, lab);

        lab = gprompt_label($P{xspuser frametab printers});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ptrdlg, lab);

        lab = gprompt_label($P{xspuser frametab ccodes});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ccdlg, lab);

        lab = gprompt_label($P{xspuser frametab privs});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), privdlg, lab);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), notebook, TRUE, TRUE, NOTE_PADDING);
        gtk_widget_show_all(dlg);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), wpage);
        return  dlg;
}

classcode_t get_class_from_widgets(struct dialog_data *ddata)
{
        classcode_t  ncc = 0;
        int     cnt;

        for  (cnt = 0;  cnt < 32;  cnt++)
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->classcodes[cnt])))
                        ncc |= 1 << cnt;
        return  ncc;
}

ULONG get_priv_from_widgets(struct dialog_data *ddata)
{
        ULONG   npriv = 0;
        int     cnt;

        for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->privs[cnt])))
                        npriv |= ptab[cnt].setbit;
        return  npriv;
}

void extract_ddata_defaults(struct dialog_data *ddata)
{
        int     redisp = 0;
        int     minp, defp, maxp, cps;
        const   char  *nform, *nptr, *nforma, *nptra;
        classcode_t     ncc;
        ULONG           npriv;

        minp = gtk_range_get_value(GTK_RANGE(ddata->minpri));
        defp = gtk_range_get_value(GTK_RANGE(ddata->defpri));
        maxp = gtk_range_get_value(GTK_RANGE(ddata->maxpri));
        cps = gtk_range_get_value(GTK_RANGE(ddata->copies));

        /* Do it this way because
           1. Not all GTKs have gtk_combo_box_get_active_text
           2. The result needs freeing anyhow and we only require a const value */

        nform = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->form))));
        nptr = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->ptr))));
        nforma = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->aform))));
        nptra = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->aptr))));

        ncc = get_class_from_widgets(ddata);
        npriv = get_priv_from_widgets(ddata);

        if  (ncc != Spuhdr.sph_class  ||  npriv != Spuhdr.sph_flgs)  {
                redisp = 1;
                hchanges++;
                Spuhdr.sph_class = ncc;
                Spuhdr.sph_flgs = npriv;
        }

        if  (minp != Spuhdr.sph_minp || defp != Spuhdr.sph_minp || maxp != Spuhdr.sph_maxp)  {
                hchanges++;
                Spuhdr.sph_minp = minp;
                Spuhdr.sph_defp = defp;
                Spuhdr.sph_maxp = maxp;
        }
        if  (cps != Spuhdr.sph_cps)  {
                hchanges++;
                Spuhdr.sph_cps = cps;
        }
        if  (strcmp(nform, Spuhdr.sph_form) != 0)  {
                hchanges++;
                strncpy(Spuhdr.sph_form, nform, MAXFORM);
        }
        if  (strcmp(nforma, Spuhdr.sph_formallow) != 0)  {
                hchanges++;
                strncpy(Spuhdr.sph_formallow, nforma, ALLOWFORMSIZE);
        }
        if  (strcmp(nptr, Spuhdr.sph_ptr) != 0)  {
                hchanges++;
                strncpy(Spuhdr.sph_ptr, nptr, PTRNAMESIZE);
        }
        if  (strcmp(nptra, Spuhdr.sph_ptrallow) != 0)  {
                hchanges++;
                strncpy(Spuhdr.sph_ptrallow, nptra, JPTRNAMESIZE);
        }

        if  (redisp)
                redispallu();
        defdisplay();
}

void extract_ddata_users(struct dialog_data *ddata)
{
        int     cnt, minp, defp, maxp, cps;
        const   char  *nform, *nptr, *nforma, *nptra;
        classcode_t     ncc;
        ULONG           npriv;

        minp = gtk_range_get_value(GTK_RANGE(ddata->minpri));
        defp = gtk_range_get_value(GTK_RANGE(ddata->defpri));
        maxp = gtk_range_get_value(GTK_RANGE(ddata->maxpri));
        cps = gtk_range_get_value(GTK_RANGE(ddata->copies));

        /* Do it this way because
           1. Not all GTKs have gtk_combo_box_get_active_text
           2. The result needs freeing anyhow and we only require a const value */

        nform = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->form))));
        nptr = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->ptr))));
        nforma = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->aform))));
        nptra = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->aptr))));

        ncc = get_class_from_widgets(ddata);
        npriv = get_priv_from_widgets(ddata);

        uchanges++;

        for  (cnt = 0;  cnt < pendunum;  cnt++)  {
                struct  spdet  *up = &ulist[pendulist[cnt]];
                up->spu_class = ncc;
                up->spu_flgs = npriv;
                up->spu_minp = minp;
                up->spu_defp = defp;
                up->spu_maxp = maxp;
                up->spu_cps = cps;
                strncpy(up->spu_form, nform, MAXFORM);
                strncpy(up->spu_formallow, nforma, ALLOWFORMSIZE);
                strncpy(up->spu_ptr, nptr, PTRNAMESIZE);
                strncpy(up->spu_ptrallow, nptra, JPTRNAMESIZE);
        }

        update_selected_users();
}

void    cb_options(const int isusers, const int wpage)
{
        struct  dialog_data     ddata;
        GtkWidget  *dlg;

        if  (isusers  &&  !getselectedusers(1))
                return;

        dlg = create_dialog(&ddata, wpage, isusers);
        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                if  (isusers)
                        extract_ddata_users(&ddata);
                else
                        extract_ddata_defaults(&ddata);
        }
        gtk_widget_destroy(dlg);
}

void cb_pri(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_PRI_PAGE);
}

void cb_form(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_FORM_PAGE);
}

void cb_ptr(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_PTR_PAGE);
}

void cb_class(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_CC_PAGE);
}

void cb_priv(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_PRIV_PAGE);
}

static void copyu(struct spdet *n)
{
        n->spu_defp = Spuhdr.sph_defp;
        n->spu_minp = Spuhdr.sph_minp;
        n->spu_maxp = Spuhdr.sph_maxp;
        n->spu_cps = Spuhdr.sph_cps;
        n->spu_class = Spuhdr.sph_class;
        strncpy(n->spu_form, Spuhdr.sph_form, MAXFORM);
        strncpy(n->spu_formallow, Spuhdr.sph_formallow, ALLOWFORMSIZE);
        strncpy(n->spu_ptr, Spuhdr.sph_ptr, PTRNAMESIZE);
        strncpy(n->spu_ptrallow, Spuhdr.sph_ptrallow, JPTRNAMESIZE);
}

void cb_copyall(GtkAction *action)
{
        unsigned  cnt;
        for  (cnt = 0;  cnt < Npwusers;  cnt++)
                copyu(&ulist[cnt]);
        update_all_users();
        uchanges++;
}

void cb_copydef(GtkAction *action)
{
        unsigned  cnt;
        if  (!getselectedusers(1))
                return;
        for  (cnt = 0;  cnt < pendunum;  cnt++)
                copyu(&ulist[pendulist[cnt]]);
        update_selected_users();
        uchanges++;
}
