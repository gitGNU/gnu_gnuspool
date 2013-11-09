/* xsq_jcall.c -- callback routines specific to jobs

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
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <gtk/gtk.h>
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
#include "stringvec.h"
#include "xsq_ext.h"
#include "gtk_lib.h"

#define JOBOPT_FORM_PAGE        0
#define JOBOPT_PAGE_PAGE        1
#define JOBOPT_USER_PAGE        2
#define JOBOPT_RETAIN_PAGE      3
#define JOBOPT_CLASS_PAGE       4

#define LAB_PADDING     5
#define NOTE_PADDING    5

#define INIT_HT_OFFSET  3600

#define SECSPERDAY      (24L * 60L * 60L)
#define MAXLONG 0x7fffffffL     /*  Change this?  */

static  char    *Last_unqueue_dir;
extern  char    *execprog;
char    *udprog;

static  char    *daynames[7],
                *monnames[12];

struct  macromenitem    jobmacs[MAXMACS];

extern  GtkWidget *make_combo_box_entry(void (*)(struct stringvec *), const char *);
extern void  wotjform(struct stringvec *);
extern void  wotjprin(struct stringvec *);
extern classcode_t  read_cc_butts(GtkWidget **);
extern void  cc_dlgsetup(GtkWidget *, GtkWidget **, const classcode_t);

struct  dialog_data  {
        GtkWidget       *formsel, *titlew, *supphw, *ptrselw, *priw, *cpsw;
        GtkWidget       *startpw, *hatpw, *endpw;
        GtkWidget       *allpw, *oddpw, *evenpw, *swapoew;
        GtkWidget       *postprocw;
        GtkWidget       *postuw, *mailw, *writew, *mattnw, *wattnw;
        time_t          subtime_copy;
        GtkWidget       *retnw, *beenpw, *delpw, *whendelpw, *delnpw, *whendelnpw;
        GtkWidget       *hashtw, *holdth, *holdtm, *holdts, *holdtdate;
        time_t          holdtime_copy;
        GtkWidget       *classcodes[32], *locow;
};

static const Hashspq *getselectedjob()
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                guint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(jlist_store), &iter, SEQ_COL, &seq, -1);
                return  Job_seg.jj_ptrs[seq];
        }
        return  NULL;
}

/* Get the job the user is pointing at.  Winge if no such job, or user can't get at it.  */

const struct spq *getselectedjob_chk(const ULONG priv)
{
        const  Hashspq  *result = getselectedjob();
        const  struct  spq  *rj;

        if  (!result)  {
                doerror(Job_seg.njobs != 0? $EH{No job selected}: $EH{No jobs to select});
                return  NULL;
        }

        rj = &result->j;

        /* Set priv to 0 if no check required */

        if  (priv)  {
                if  (!(mypriv->spu_flgs & priv)  &&  strcmp(Realuname, rj->spq_uname) != 0)  {
                        disp_str = rj->spq_file;
                        disp_str2 = rj->spq_uname;
                        doerror(priv == PV_VOTHERJ? $EH{xmspq job not readable}: $EH{spq job not yours});
                        return  NULL;
                }
                if  (rj->spq_netid  &&  !(mypriv->spu_flgs & PV_REMOTEJ))  {
                        doerror($EH{spq no remote job priv});
                        return  NULL;
                }
        }
        JREQ = *rj;
        JREQS = result - Job_seg.jlist;
        return  rj;
}

/* Callback for one more copy.  */

void  cb_onemore()
{
        const  struct   spq     *cj = getselectedjob_chk(PV_OTHERJ);
        int     num, maxnum = 255;
        if  (!cj)
                return;
        num = cj->spq_cps + 1;
        if  (!(mypriv->spu_flgs & PV_ANYPRIO))
                maxnum = mypriv->spu_cps;
        if  (num > maxnum)  {
                disp_arg[1] = maxnum;
                doerror($EH{Too many copies});
                return;
        }
        JREQ.spq_cps = (unsigned char) num;
        my_wjmsg(SJ_CHNG);
}

/* Job actions, currently only SO_AB */

void  cb_jact()
{
        const  struct   spq     *jp = getselectedjob_chk(PV_OTHERJ);
        if  (!jp)
                return;
        OREQ = JREQS;
        if  (confabort > 1 || (confabort > 0  &&  !(jp->spq_dflags & SPQ_PRINTED)))  {
                if  (!Confirm(jp->spq_dflags & SPQ_PRINTED? $PH{Sure about deleting printed job}: $PH{Sure about deleting not printed}))
                        return;
        }
        womsg(SO_AB);
}

GtkWidget *create_formpri_dlg(struct dialog_data *ddata, const struct spq *jp)
{
        GtkWidget  *frame, *vbox, *hbox, *lab;
        char    *pr;
        GtkAdjustment *adj;
        gdouble  minp = 1.0, maxp = 255.0, maxc = 255.0, curp, curc;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab jform});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* Form selection row */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        lab = gprompt_label($P{xspq job form});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        ddata->formsel = make_combo_box_entry(wotjform, jp->spq_form);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->formsel, FALSE, FALSE, DEF_DLG_HPAD);

        /* Title row */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        lab = gprompt_label($P{xspq job title});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        ddata->titlew = gtk_entry_new();
        if  (strlen(jp->spq_file) != 0)
                gtk_entry_set_text(GTK_ENTRY(ddata->titlew), jp->spq_file);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->titlew, FALSE, FALSE, DEF_DLG_HPAD);

        ddata->supphw = gprompt_checkbutton($P{xspq job supph});
        if  (jp->spq_jflags & SPQ_NOH)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->supphw), TRUE);

        /* Printer row */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        lab = gprompt_label($P{xspq job printer});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        ddata->ptrselw = make_combo_box_entry(wotjprin, jp->spq_ptr);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->ptrselw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Priority and copies */

        curp = jp->spq_pri;
        curc = jp->spq_cps;
        if  (!(mypriv->spu_flgs & PV_ANYPRIO))  {
                maxp = mypriv->spu_maxp;
                minp = mypriv->spu_minp;
                if  (curp < minp)
                        curp = minp;
                if  (curp > maxp)
                        curp = maxp;
                maxc = mypriv->spu_cps;
                if  (curc > maxc)
                        curc = maxc;
        }

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        lab = gprompt_label($P{xspq job copies});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new(curc, 0.0, maxc, 1.0, 10.0, 0.0);
        ddata->cpsw = gtk_spin_button_new(adj, 0.0, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->cpsw, FALSE, FALSE, DEF_DLG_HPAD);
        lab = gprompt_label($P{xspq job prio});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new(curp, minp, maxp, 1.0, 10.0, 0.0);
        ddata->priw = gtk_spin_button_new(adj, 0.0, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->priw, FALSE, FALSE, DEF_DLG_HPAD);

        return  frame;
}

int  extract_form_response(struct dialog_data *ddata)
{
        const  char  *newft = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->formsel))));
        const  char  *newtit = gtk_entry_get_text(GTK_ENTRY(ddata->titlew));
        gboolean  newsupph = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->supphw));
        const  char  *newptr = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->ptrselw))));
        int  newcps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->cpsw));
        int  newpri = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->priw));

        if  (strlen(newft) == 0)
                return  0;
        if  (!((mypriv->spu_flgs & PV_FORMS) || qmatch(mypriv->spu_formallow, newft)))  {
                disp_str = newft;
                disp_str2 = mypriv->spu_formallow;
                doerror($EH{form type not valid});
                return  0;
        }
        strncpy(JREQ.spq_form, newft, MAXFORM);
        strncpy(JREQ.spq_file, newtit, MAXTITLE);
        if  (newsupph)
                JREQ.spq_jflags |= SPQ_NOH;
        else
                JREQ.spq_jflags &= ~SPQ_NOH;
        strncpy(JREQ.spq_ptr, newptr, JPTRNAMESIZE);
        JREQ.spq_pri = newpri;
        JREQ.spq_cps = newcps;
        return  1;
}

GtkWidget *create_page_dlg(struct dialog_data *ddata, const struct spq *jp)
{
        GtkWidget  *frame, *vbox, *hbox, *lab;
        char    *pr;
        GtkAdjustment *adj;
        gdouble  npages = jp->spq_npages + 1, cpage;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab jpage});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* Start and "halted at" pages */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        lab = gprompt_label($P{xspq job page names});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        cpage = jp->spq_start + 1;
        if  (cpage > npages)
                cpage = npages;
        adj = (GtkAdjustment *) gtk_adjustment_new(cpage, 1.0, npages, 1.0, 10.0, 0.0);
        ddata->startpw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->startpw, FALSE, FALSE, DEF_DLG_HPAD);
        cpage = jp->spq_haltat + 1;
        if  (cpage > npages)
                cpage = npages;
        adj = (GtkAdjustment *) gtk_adjustment_new(cpage, 1.0, npages, 1.0, 10.0, 0.0);
        ddata->hatpw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->hatpw, FALSE, FALSE, DEF_DLG_HPAD);
        cpage = (gdouble) jp->spq_end + 1.0;
        if  (cpage > npages)
                cpage = npages;
        adj = (GtkAdjustment *) gtk_adjustment_new(cpage, 1.0, npages, 1.0, 10.0, 0.0);
        ddata->endpw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->endpw, FALSE, FALSE, DEF_DLG_HPAD);

        /* All/odd/even/swap buttons */

        ddata->allpw = gprompt_radiobutton($P{xspq job all pages});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->allpw, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->oddpw = gprompt_radiobutton_fromwidget(ddata->allpw, $P{xspq job odd pages});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->oddpw, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->evenpw = gprompt_radiobutton_fromwidget(ddata->allpw, $P{xspq job even pages});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->evenpw, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->swapoew = gprompt_checkbutton($P{xspq job swapoe});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->swapoew, FALSE, FALSE, DEF_DLG_VPAD);

        if  (jp->spq_jflags & (SPQ_ODDP|SPQ_EVENP))  {
                /* I did get these the right way round SPQ_ODDP kills odd pages and
                   the description says print even ones and vice versa */
                if  (jp->spq_jflags & SPQ_ODDP)
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->evenpw), TRUE);
                else
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->oddpw), TRUE);
                if  (jp->spq_jflags & SPQ_REVOE)
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->swapoew), TRUE);
        }
        else
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->allpw), TRUE);

        /* Postprocessing options */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq job postproc flags});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        ddata->postprocw = gtk_entry_new();
        if  (strlen(jp->spq_flags) != 0)
                gtk_entry_set_text(GTK_ENTRY(ddata->postprocw), jp->spq_flags);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->postprocw, FALSE, FALSE, DEF_DLG_HPAD);
        return  frame;
}

int  extract_page_response(struct dialog_data *ddata)
{
        int     spage = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->startpw));
        int     hpage = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->hatpw));
        int     epage = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->endpw));

        if  (spage > epage)  {
                doerror($EH{end page less than start page});
                return  0;
        }
        if  (hpage > epage)  {
                doerror($EH{end page less than haltat page});
                return  0;
        }
        if  (epage > JREQ.spq_npages)
                epage = MAXLONG;
        JREQ.spq_start = spage - 1;
        JREQ.spq_end = epage - 1;
        JREQ.spq_haltat = hpage - 1;
        JREQ.spq_jflags &= ~(SPQ_ODDP|SPQ_EVENP|SPQ_REVOE);
        if  (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->allpw)))  {
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->evenpw)))
                        JREQ.spq_jflags |= SPQ_EVENP;
                else  if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->oddpw)))
                        JREQ.spq_jflags |= SPQ_ODDP;
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->swapoew)))
                        JREQ.spq_jflags |= SPQ_REVOE;
        }

        strncpy(JREQ.spq_flags, gtk_entry_get_text(GTK_ENTRY(ddata->postprocw)), MAXFLAGS);
        return  1;
}

GtkWidget *create_usermail_dlg(struct dialog_data *ddata, const struct spq *jp)
{
        GtkWidget  *frame, *vbox, *hbox, *lab;
        int     cnt, uselect = 0;
        char    *pr;
        char    **ulist = gen_ulist((char *) 0, 0), **ulp;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab usermail});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* User to post to
           TODO - generate user list only if/when the page is selected */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        lab = gprompt_label($P{xspq post user});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        ddata->postuw = gtk_combo_box_new_text();
        cnt = 0;
        for  (ulp = ulist;  *ulp;  ulp++)  {
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->postuw), *ulp);
                if  (strcmp(*ulp, jp->spq_puname) == 0)
                        uselect = cnt;
                free(*ulp);
                cnt++;
        }
        free((char *) ulist);
        gtk_combo_box_set_active(GTK_COMBO_BOX(ddata->postuw), uselect);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->postuw, FALSE, FALSE, DEF_DLG_VPAD);

        ddata->mailw = gprompt_checkbutton($P{xspq job mail});
        if  (jp->spq_jflags & SPQ_MAIL)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->mailw), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->mailw, FALSE, FALSE, DEF_DLG_VPAD*2);

        ddata->writew = gprompt_checkbutton($P{xspq job write});
        if  (jp->spq_jflags & SPQ_WRT)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->writew), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->writew, FALSE, FALSE, DEF_DLG_VPAD*2);

        ddata->mattnw = gprompt_checkbutton($P{xspq job mattn});
        if  (jp->spq_jflags & SPQ_MATTN)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->mattnw), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->mattnw, FALSE, FALSE, DEF_DLG_VPAD*2);

        ddata->wattnw = gprompt_checkbutton($P{xspq job wattn});
        if  (jp->spq_jflags & SPQ_WATTN)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->wattnw), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->wattnw, FALSE, FALSE, DEF_DLG_VPAD*2);

        return  frame;
}

int  extract_user_response(struct dialog_data *ddata)
{
        gchar   *newu = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ddata->postuw));
        strncpy(JREQ.spq_puname, newu, UIDSIZE);
        g_free(newu);
        JREQ.spq_jflags &= ~(SPQ_MAIL|SPQ_WRT|SPQ_MATTN|SPQ_WATTN);
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->mailw)))
                JREQ.spq_jflags |= SPQ_MAIL;
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->writew)))
                JREQ.spq_jflags |= SPQ_WRT;
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->mattnw)))
                JREQ.spq_jflags |= SPQ_MATTN;
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->wattnw)))
                JREQ.spq_jflags |= SPQ_WATTN;
        return  1;
}

void  ptimeoutchanged(GtkWidget *spinner, struct dialog_data *ddata)
{
        GString *labp;
        gint    val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinner));
        time_t  when = ddata->subtime_copy + val * 3600;
        struct  tm  *tp = localtime(&when);
        GtkWidget  *wlab = spinner == ddata->delpw? ddata->whendelpw: ddata->whendelnpw;
        labp = g_string_new(NULL);
        g_string_printf(labp, "%s %d %s %d", daynames[tp->tm_wday], tp->tm_mday, monnames[tp->tm_mon], tp->tm_year+1900);
        gtk_label_set_text(GTK_LABEL(wlab), labp->str);
        g_string_free(labp, TRUE);
}

void  turn_ht(struct dialog_data *ddata, gboolean onoff)
{
        gtk_widget_set_sensitive(ddata->holdth, onoff);
        gtk_widget_set_sensitive(ddata->holdtm, onoff);
        gtk_widget_set_sensitive(ddata->holdts, onoff);
        gtk_widget_set_sensitive(ddata->holdtdate, onoff);
}

void  set_ht_val(struct dialog_data *ddata, time_t when)
{
        struct  tm  *tp;
        ddata->holdtime_copy = when;
        tp = localtime(&when);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->holdth), (gdouble) tp->tm_hour);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->holdtm), (gdouble) tp->tm_min);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->holdts), (gdouble) tp->tm_sec);
        gtk_calendar_select_month(GTK_CALENDAR(ddata->holdtdate), tp->tm_mon, tp->tm_year + 1900);
        gtk_calendar_select_day(GTK_CALENDAR(ddata->holdtdate), tp->tm_mday);
}

void  set_ht(struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->hashtw)))  {
                turn_ht(ddata, TRUE);
                set_ht_val(ddata, time(0) + INIT_HT_OFFSET);
        }
        else  {
                turn_ht(ddata, FALSE);
                ddata->holdtime_copy = 0;
        }
}

GtkWidget *create_retain_dlg(struct dialog_data *ddata, const struct spq *jp)
{
        GtkWidget  *frame, *vbox, *hbox, *lab;
        char    *pr;
        GString *labp;
        time_t  when;
        struct  tm      *tp;
        gdouble val;
        GtkAdjustment *adj;

        if  (!daynames[0])  {
                int  cnt;
                for  (cnt = 0;  cnt < 7;  cnt++)
                        daynames[cnt] = gprompt(cnt+$P{Full Sunday});
                for  (cnt = 0;  cnt < 12;  cnt++)
                        monnames[cnt] = gprompt(cnt+$P{Full January});
        }

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab retain});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        pr = gprompt($P{xspq job sumitted});
        ddata->subtime_copy = jp->spq_time;
        tp = localtime(&ddata->subtime_copy);
        labp = g_string_new(NULL);
        g_string_printf(labp, "%s %s %d %s %d", pr, daynames[tp->tm_wday], tp->tm_mday, monnames[tp->tm_mon], tp->tm_year+1900);
        lab = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), lab, FALSE, FALSE, DEF_DLG_VPAD);

        ddata->retnw = gprompt_checkbutton($P{xspq job retain});
        if  (jp->spq_jflags & SPQ_RETN)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->retnw), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->retnw, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->beenpw = gprompt_checkbutton($P{xspq job printed});
        if  (jp->spq_dflags & SPQ_PRINTED)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->beenpw), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->beenpw, FALSE, FALSE, DEF_DLG_VPAD);

        /* Delete printed time */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq delp time});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        val = jp->spq_ptimeout;
        if  (val < 1.0)
                val = 1.0;
        adj = (GtkAdjustment *) gtk_adjustment_new(val, 1.0, 32767.0, 1.0, 10.0, 0.0);
        ddata->delpw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->delpw, FALSE, FALSE, DEF_DLG_HPAD);
        when = ddata->subtime_copy + jp->spq_ptimeout * 3600;
        tp = localtime(&when);
        labp = g_string_new(NULL);
        g_string_printf(labp, "%s %d %s %d", daynames[tp->tm_wday], tp->tm_mday, monnames[tp->tm_mon], tp->tm_year+1900);
        ddata->whendelpw = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->whendelpw, FALSE, FALSE, DEF_DLG_HPAD);
        g_signal_connect(G_OBJECT(ddata->delpw), "value_changed", G_CALLBACK(ptimeoutchanged), (gpointer) ddata);

        /* Delete not printed time */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq delnp time});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        val = jp->spq_nptimeout;
        if  (val < 1.0)
                val = 1.0;
        adj = (GtkAdjustment *) gtk_adjustment_new(val, 1.0, 32767.0, 1.0, 10.0, 0.0);
        ddata->delnpw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->delnpw, FALSE, FALSE, DEF_DLG_HPAD);
        when = ddata->subtime_copy + jp->spq_nptimeout * 3600;
        tp = localtime(&when);
        labp = g_string_new(NULL);
        g_string_printf(labp, "%s %d %s %d", daynames[tp->tm_wday], tp->tm_mday, monnames[tp->tm_mon], tp->tm_year+1900);
        ddata->whendelnpw = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->whendelnpw, FALSE, FALSE, DEF_DLG_HPAD);
        g_signal_connect(G_OBJECT(ddata->delnpw), "value_changed", G_CALLBACK(ptimeoutchanged), (gpointer) ddata);

        /* Now for hold time */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->hashtw = gprompt_checkbutton($P{xspq job set hold});
        gtk_box_pack_start(GTK_BOX(hbox), ddata->hashtw, FALSE, FALSE, 0);
        adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 23.0, 1.0, 1.0, 0.0);
        ddata->holdth = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->holdth, FALSE, FALSE, 0);
        adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 59.0, 1.0, 1.0, 0.0);
        ddata->holdtm = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->holdtm, FALSE, FALSE, 0);
        adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 59.0, 1.0, 1.0, 0.0);
        ddata->holdts = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->holdts, FALSE, FALSE, 0);
        ddata->holdtdate = gtk_calendar_new();
        gtk_calendar_set_display_options(GTK_CALENDAR(ddata->holdtdate), GTK_CALENDAR_SHOW_HEADING|GTK_CALENDAR_SHOW_DAY_NAMES);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->holdtdate, FALSE, FALSE, 0);
        when = time(0);
        if  (jp->spq_hold < when)  {
                turn_ht(ddata, FALSE);
                ddata->holdtime_copy = 0;
        }
        else  {
                set_ht_val(ddata, jp->spq_hold);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->hashtw), TRUE);
        }
        g_signal_connect_swapped(G_OBJECT(ddata->hashtw), "toggled", G_CALLBACK(set_ht), (gpointer) ddata);
        return  frame;
}

int  extract_retain_response(struct dialog_data *ddata)
{
        gint    pto = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->delpw));
        gint    npto = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->delnpw));

        if  (pto > npto  &&  !Confirm($PH{xmspq not ptd lt ptd}))
                return  0;

        JREQ.spq_ptimeout = pto;
        JREQ.spq_nptimeout = npto;

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->retnw)))
                JREQ.spq_jflags |= SPQ_RETN;
        else
                JREQ.spq_jflags &= ~SPQ_RETN;

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->beenpw)))
                JREQ.spq_dflags |= SPQ_PRINTED;
        else
                JREQ.spq_dflags &= ~SPQ_PRINTED;

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->hashtw)))  {
                int     hr = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->holdth));
                int     min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->holdtm));
                int     sec = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->holdts));
                guint   day, month, year;
                time_t  when;
                struct  tm      *tp, mt;
                gtk_calendar_get_date(GTK_CALENDAR(ddata->holdtdate), &year, &month, &day);

                /* These gyrations are in case it crosses DST */

                mt.tm_hour = hr;
                mt.tm_min = min;
                mt.tm_sec = sec;
                mt.tm_mday = day;
                mt.tm_mon = month;
                mt.tm_year = year - 1900;
                mt.tm_isdst = 0;
                when = mktime(&mt);
                tp = localtime(&when);
                mt = *tp;
                mt.tm_hour = hr;
                mt.tm_min = min;
                when = mktime(&mt);
                JREQ.spq_hold = when < time(0)? 0: when;
        }
        else
                JREQ.spq_hold = 0;

        return  1;
}

GtkWidget *create_cc_dlg(struct dialog_data *ddata, const struct spq *jp)
{
        GtkWidget  *frame, *vbox;
        char    *pr;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab ccodes});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        cc_dlgsetup(vbox, ddata->classcodes, jp->spq_class);

        ddata->locow = gprompt_checkbutton($P{xspq job loconly});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->locow, FALSE, FALSE, DEF_DLG_VPAD*2);
        if  (jp->spq_jflags & SPQ_LOCALONLY)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->locow), TRUE);

        return  frame;
}

int  extract_cc_response(struct dialog_data *ddata)
{
        classcode_t  ncc = read_cc_butts(ddata->classcodes);

        if  (!(mypriv->spu_flgs & PV_COVER))
                ncc &= mypriv->spu_class;

        if  (ncc == 0)  {
                doerror($EH{xmspq setting zero class});
                return  0;
        }
        JREQ.spq_class = ncc;
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->locow)))
                JREQ.spq_jflags |= SPQ_LOCALONLY;
        else
                JREQ.spq_jflags &= ~SPQ_LOCALONLY;
        return  1;
}

GtkWidget  *create_dialog(struct dialog_data *ddata, const struct spq *jp, const int wpage)
{
        GtkWidget  *dlg, *lab, *notebook, *formdlg, *pagedlg, *userdlg, *retndlg, *ccdlg;
        char    *pr;
        GString *labp;

        pr = gprompt($P{xspq jobopt dialogtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        /* Say what job we are editing */

        pr = gprompt($P{xspq editing job});
        labp = g_string_new(NULL);
        g_string_printf(labp, "%s %s", pr, JOB_NUMBER(jp));
        free(pr);
        lab = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, LAB_PADDING);

        /* Set up notebook */

        notebook = gtk_notebook_new();
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

        formdlg = create_formpri_dlg(ddata, jp);
        pagedlg = create_page_dlg(ddata, jp);
        userdlg = create_usermail_dlg(ddata, jp);
        retndlg = create_retain_dlg(ddata, jp);
        ccdlg = create_cc_dlg(ddata, jp);

        /* Set up tab names for notebook */

        lab = gprompt_label($P{xspq frametab form});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), formdlg, lab);

        lab = gprompt_label($P{xspq frametab page});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pagedlg, lab);

        lab = gprompt_label($P{xspq frametab user});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), userdlg, lab);

        lab = gprompt_label($P{xspq frametab retain});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), retndlg, lab);

        lab = gprompt_label($P{xspq frametab cc});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ccdlg, lab);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), notebook, TRUE, TRUE, NOTE_PADDING);
        gtk_widget_show_all(dlg);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), wpage);
        return  dlg;
}

int  extract_job_response(struct dialog_data *ddata)
{
        if  (extract_form_response(ddata)  &&
             extract_page_response(ddata)  &&
             extract_user_response(ddata)  &&
             extract_retain_response(ddata)  &&
             extract_cc_response(ddata))  {
                my_wjmsg(SJ_CHNG);
                return  1;
        }
        return  0;
}

void  cb_options(const int wpage)
{
        struct  dialog_data     ddata;
        GtkWidget  *dlg;
        const  struct  spq  *jp = getselectedjob_chk(PV_OTHERJ);

        if  (!jp)
                return;

        dlg = create_dialog(&ddata, jp, wpage);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK  &&  !extract_job_response(&ddata))
                ;
        gtk_widget_destroy(dlg);
}

void  cb_jform()
{
        cb_options(JOBOPT_FORM_PAGE);
}

void  cb_jpages()
{
        cb_options(JOBOPT_PAGE_PAGE);
}

void  cb_juser()
{
        cb_options(JOBOPT_USER_PAGE);
}

void  cb_jretain()
{
        cb_options(JOBOPT_RETAIN_PAGE);
}

void  cb_jclass()
{
        cb_options(JOBOPT_CLASS_PAGE);
}

struct  unqueue_data  {
        GtkWidget       *dirsw;
        GtkWidget       *cfw;
        GtkWidget       *jfw;
};

void  cmd_sel(struct unqueue_data *unqd)
{
        GtkWidget *fsw;
        char    *pr = gprompt($P{xspq unqueue cmd dlg title});
        GString *labp = g_string_new(NULL);
        const  char  *dirn, *filen;

        fsw = gtk_file_selection_new(pr);
        free(pr);

        dirn = gtk_label_get_text(GTK_LABEL(unqd->dirsw));
        filen = gtk_label_get_text(GTK_LABEL(unqd->cfw));
        if  (strlen(dirn) <= 1)
                g_string_printf(labp, "%s%s", dirn, filen);
        else
                g_string_printf(labp, "%s/%s", dirn, filen);
        gtk_file_selection_set_filename(GTK_FILE_SELECTION(fsw), labp->str);
        g_string_free(labp, TRUE);
        gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fsw));

        while  (gtk_dialog_run(GTK_DIALOG(fsw)) == GTK_RESPONSE_OK)  {
                const char *newfile = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fsw));
                const char *sp = strrchr(newfile, '/');
                if  (!sp)  {
                        doerror($EH{xmspq no directory});
                        continue;
                }
                if  (strlen(sp) <= 1)  {
                        doerror($EH{xmspq no cmd file});
                        continue;
                }
                if  (newfile == sp)
                        gtk_label_set_text(GTK_LABEL(unqd->dirsw), "/");
                else  {
                        char    *newd = malloc(sp - newfile + 1);
                        if  (!newd)
                                nomem();
                        strncpy(newd, newfile, sp-newfile);
                        newd[sp-newfile] = '\0';
                        gtk_label_set_text(GTK_LABEL(unqd->dirsw), newd);
                        free(newd);
                }
                gtk_label_set_text(GTK_LABEL(unqd->cfw), sp+1);
                break;
        }
        gtk_widget_destroy(fsw);
}

void  job_sel(struct unqueue_data *unqd)
{
        GtkWidget *fsw;
        char    *pr = gprompt($P{xspq unqueue job dlg title});
        const  char  *dirn, *filen;

        fsw = gtk_file_selection_new(pr);
        free(pr);

        dirn = gtk_label_get_text(GTK_LABEL(unqd->dirsw));
        filen = gtk_label_get_text(GTK_LABEL(unqd->jfw));

        if  (filen[0] == '/')
                gtk_file_selection_set_filename(GTK_FILE_SELECTION(fsw), filen);
        else  {
                GString *labp = g_string_new(NULL);
                if  (strlen(dirn) <= 1)
                        g_string_printf(labp, "%s%s", dirn, filen);
                else
                        g_string_printf(labp, "%s/%s", dirn, filen);
                gtk_file_selection_set_filename(GTK_FILE_SELECTION(fsw), labp->str);
                g_string_free(labp, TRUE);
        }

        gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fsw));

        while  (gtk_dialog_run(GTK_DIALOG(fsw)) == GTK_RESPONSE_OK)  {
                const char *newfile = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fsw));
                const char *sp = strrchr(newfile, '/');
                if  (!sp)  {
                        doerror($EH{xmspq no directory});
                        continue;
                }
                if  (strlen(sp) <= 1)  {
                        doerror($EH{xmspq no job file});
                        continue;
                }
                if  (strncmp(dirn, newfile, sp-newfile) == 0)
                        gtk_label_set_text(GTK_LABEL(unqd->jfw), sp+1);
                else
                        gtk_label_set_text(GTK_LABEL(unqd->jfw), newfile);
                break;
        }
        gtk_widget_destroy(fsw);
}

/* Unqueue (or copy job).
   We need to invoke spexec to restore the user id and in
   turn invoke spjobdump to actually do the dirty */

void  cb_unqueue()
{
        GtkWidget  *dlg, *hbox, *lab, *copyonly, *button;
        char    *pr;
        GString *labp;
        const  struct   spq     *jp = getselectedjob_chk(PV_OTHERJ);
        struct  unqueue_data    unqd;

        if  (!jp)
                return;

        if  (!(mypriv->spu_flgs & PV_UNQUEUE))  {
                doerror($EH{spq cannot unqueue});
                return;
        }

        if  (!Last_unqueue_dir)
                Last_unqueue_dir = stracpy(Curr_pwd);

        pr = gprompt($P{xspq unqueue dialogtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        /* Say what job we are unqueuing */

        pr = gprompt($P{xspq unqueuing job});
        labp = g_string_new(NULL);
        g_string_printf(labp, "%s %s", pr, JOB_NUMBER(jp));
        free(pr);
        lab = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, LAB_PADDING);

        /* Checkbox about copy only no delete on separate line */

        copyonly = gprompt_checkbutton($P{xspq copy no delete});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), copyonly, FALSE, FALSE, DEF_DLG_VPAD);

        /* Display line with directory */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq unqueue directory});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        unqd.dirsw = gtk_label_new(Last_unqueue_dir);
        gtk_box_pack_start(GTK_BOX(hbox), unqd.dirsw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Now for shell script file name first label */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);

        lab = gprompt_label($P{xspq shell script});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        /* Name for the shell script file */

        labp = g_string_new(NULL);
        pr = gprompt($P{sqdel default cmd prefix});
        g_string_printf(labp, "%s%ld", pr, (long) jp->spq_job);
        free(pr);

        /* Use helpprmpt because that returns null if no suffix */

        pr = helpprmpt($P{sqdel default cmd suffix});
        if  (pr)  {
                g_string_append(labp, pr);
                free(pr);
        }
        unqd.cfw = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        button = gtk_button_new();
        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_container_add(GTK_CONTAINER(button), hbox);
        gtk_box_pack_start(GTK_BOX(hbox), unqd.cfw, FALSE, FALSE, DEF_BUTTON_PAD);
        g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(cmd_sel), (gpointer) &unqd);

        /* Now for job file name first label */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);

        lab = gprompt_label($P{xspq job file name});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        /* Now for job file */

        labp = g_string_new(NULL);
        pr = gprompt($P{sqdel default job prefix});
        g_string_printf(labp, "%s%ld", pr, (long) jp->spq_job);
        free(pr);
        pr = helpprmpt($P{sqdel default job suffix});
        if  (pr)  {
                g_string_append(labp, pr);
                free(pr);
        }
        unqd.jfw = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);

        /* Button to change */

        button = gtk_button_new();
        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, DEF_DLG_HPAD);
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_container_add(GTK_CONTAINER(button), hbox);
        gtk_box_pack_start(GTK_BOX(hbox), unqd.jfw, FALSE, FALSE, DEF_BUTTON_PAD);
        g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(job_sel), (gpointer) &unqd);

        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *newdir = gtk_label_get_text(GTK_LABEL(unqd.dirsw));
                const  char  *newcf = gtk_label_get_text(GTK_LABEL(unqd.cfw));
                const  char  *newjf = gtk_label_get_text(GTK_LABEL(unqd.jfw));
                PIDTYPE pid;
                const  char  **ap, *argbuf[8];
                struct  stat    sbuf;

                if  (!execprog)
                        execprog = envprocess(EXECPROG);

                if  (!udprog)
                        udprog = envprocess(DUMPJOB);

                if  (newdir[0] == '\0')  {
                        doerror($EH{xmspq no directory});
                        continue;
                }
                if  (newdir[0] != '/')  {
                        disp_str = newdir;
                        doerror($EH{Not absolute path});
                        continue;
                }
                if  (stat(newdir, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                        disp_str = newdir;
                        doerror($EH{Not a directory});
                        continue;

                }
                if  (newcf[0] == '\0')  {
                        doerror($EH{xmspq no cmd file});
                        return;
                }
                if  (newjf[0] == '\0')  {
                        doerror($EH{xmspq no job file});
                        return;
                }
                free(Last_unqueue_dir);
                Last_unqueue_dir = stracpy(newdir);

                if  ((pid = fork()))  {
                        int     status;

                        if  (pid < 0)  {
                                doerror($EH{Unqueue no fork});
                                continue;
                        }

#ifdef  HAVE_WAITPID
                        while  (waitpid(pid, &status, 0) < 0)
                                ;
#else
                        while  (wait(&status) != pid)
                                ;
#endif
                        if  (status == 0)       /* All ok */
                                break;

                        if  (status & 0xff)  {
                                disp_arg[9] = status & 0xff;
                                doerror($EH{Unqueue program fault});
                                continue;
                        }

                        status = (status >> 8) & 0xff;
                        disp_arg[0] = JREQ.spq_job;
                        disp_str = JREQ.spq_file;

                        switch  (status)  {
                        default:
                                disp_arg[1] = status;
                                doerror($EH{Unqueue misc error});
                                continue;
                        case  E_SETUP:
                                disp_str = udprog;
                                doerror($EH{Cannot find unqueue});
                                continue;
                        case  E_JDFNFND:
                                doerror($EH{Unqueue spool not found});
                                continue;
                        case  E_JDNOCHDIR:
                                doerror($EH{Unqueue dir not found});
                                continue;;
                        case  E_JDFNOCR:
                                doerror($EH{Unqueue no create});
                                continue;
                        case  E_JDJNFND:
                                doerror($EH{Unqueue unknown job});
                                continue;
                        }
                }

                /* Child process */

                Ignored_error = chdir(Curr_pwd);        /* So it picks up the right config file */
                ap = argbuf;
                *ap++ = udprog;
                if  (copyonly)
                        *ap++ = "-n";
                *ap++ = JOB_NUMBER(&JREQ);
                *ap++ = Last_unqueue_dir;
                *ap++ = (char *) newcf;
                *ap++ = (char *) newjf;
                *ap++ = (char *) 0;
                execv(execprog, (char **) argbuf);
                exit(E_SETUP);
        }
        gtk_widget_destroy(dlg);
}

static int  jmacroexec(const char *str, const struct spq *jp)
{
        PIDTYPE pid;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  ((pid = fork()) == 0)  {
                const   char    *argbuf[3];
                argbuf[0] = (char *) str;
                if  (jp)  {
                        argbuf[1] = JOB_NUMBER(jp);
                        argbuf[2] = (char *) 0;
                }
                else
                        argbuf[1] = (char *) 0;
                Ignored_error = chdir(Curr_pwd);
                execv(execprog, (char **) argbuf);
                exit(E_SPEXEC1);
        }
        if  (pid < 0)  {
                doerror($EH{Macro fork failed});
                return  0;
        }
#ifdef  HAVE_WAITPID
        while  (waitpid(pid, &status, 0) < 0)
                ;
#else
        while  (wait(&status) != pid)
                ;
#endif
        if  (status != 0)  {
                if  (status & 255)  {
                        disp_arg[0] = status & 255;
                        doerror($EH{Macro command gave signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror($EH{Macro command error});
                }
                return  0;
        }

        return  1;
}


static  struct  stringvec  previous_commands;

/* Version of job macro for where we prompt */

void  cb_jmac()
{
        const  Hashspq  *hjp = getselectedjob();
        const   struct  spq  *jp = 0;
        GtkWidget  *dlg, *lab, *hbox, *cmdentry;
        int        oldmac = is_init(previous_commands);
        char       *pr;

        pr = gprompt($P{xspg jmac dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);

        if  (hjp)  {
                jp = &hjp->j;
                GString *labp = g_string_new(NULL);
                pr = gprompt($P{xspq jmac named});
                g_string_printf(labp, "%s %s", pr, JOB_NUMBER(jp));
                free(pr);
                lab = gtk_label_new(labp->str);
                g_string_free(labp, TRUE);
        }
        else
                lab = gprompt_label($P{xspq jmac noname});

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, 0);
        lab = gprompt_label($P{xspq jmac cmd});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        if  (oldmac)  {
                unsigned  cnt;
                cmdentry = gtk_combo_box_entry_new_text();
                for  (cnt = 0;  cnt < stringvec_count(previous_commands);  cnt++)
                        gtk_combo_box_append_text(GTK_COMBO_BOX(cmdentry), stringvec_nth(previous_commands, cnt));
        }
        else
                cmdentry = gtk_entry_new();

        gtk_box_pack_start(GTK_BOX(hbox), cmdentry, FALSE, FALSE, 0);
        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *cmdtext;
                if  (oldmac)
                        cmdtext = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cmdentry))));
                else
                        cmdtext = gtk_entry_get_text(GTK_ENTRY(cmdentry));
                if  (strlen(cmdtext) == 0)  {
                        doerror($EH{xmsq empty macro command});
                        continue;
                }

                if  (jmacroexec(cmdtext, jp))  {
                        if  (add_macro_to_list(cmdtext, 'j', jobmacs))
                                break;
                        if  (!oldmac)
                                stringvec_init(&previous_commands);
                        stringvec_insert_unique(&previous_commands, cmdtext);
                        break;
                }
        }
        gtk_widget_destroy(dlg);
}

void  jmacruncb(GtkAction *act, struct macromenitem *mitem)
{
        const  Hashspq  *hjp = getselectedjob();
        const   struct  spq  *jp = 0;
        if  (hjp)
                jp = &hjp->j;
        jmacroexec(mitem->cmd, jp);
}
