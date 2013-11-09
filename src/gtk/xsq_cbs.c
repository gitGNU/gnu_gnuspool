/* xsq_cbs.c -- misc callback routines for xspq

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
#include <errno.h>
#include <gtk/gtk.h>
#include <pwd.h>
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "xsq_ext.h"
#include "gtk_lib.h"
#include "stringvec.h"
#include "displayopt.h"
#include "files.h"
#include "cfile.h"

char    *execprog;
extern  int     Dirty;

extern  struct  macromenitem    jobmacs[], ptrmacs[];

#define NOTE_PADDING    5

extern void  wotjprin(struct stringvec *);
extern char *gen_jfmts();
extern char *gen_pfmts();

/* Get the states of the class code button array to get the class code
   which has been set */

classcode_t  read_cc_butts(GtkWidget **buttlist)
{
        classcode_t  ncc = 0;
        int     cnt;

        for  (cnt = 0;  cnt < 32;  cnt++)
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttlist[cnt])))
                        ncc |= 1 << cnt;
        return  ncc;
}

static void  set_cc_butts(GtkWidget **buttlist, const classcode_t cc)
{
        int     cnt;

        for  (cnt = 0;  cnt < 32;  cnt++)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttlist[cnt]), cc & (1 << cnt)? TRUE: FALSE);
}

static void  cc_setall(GtkWidget **buttlist)
{
        classcode_t  ncc = read_cc_butts(buttlist);
        classcode_t  defcc = mypriv->spu_class;
        classcode_t  diffb = ncc ^ defcc;               /* Bits which are different */
        ULONG   hasover = mypriv->spu_flgs & PV_COVER;

        if  (diffb != 0)  {

                /* Diffb contains bits that are different from the default class.
                   If we & with the default class and the result is zero then the bits
                   set in diffb  are in excess of the default class. In which case set all bits
                   if we have override class privilege otherwise make default class.
                   In all other cases make it the default class. */

                set_cc_butts(buttlist, (diffb & defcc) == 0 && hasover? U_MAX_CLASS: defcc);
        }
        else  if  (hasover  &&  defcc != U_MAX_CLASS)
                set_cc_butts(buttlist, U_MAX_CLASS);
}

static void  cc_clearall(GtkWidget **buttlist)
{
        classcode_t  ncc = read_cc_butts(buttlist);
        classcode_t  defcc = mypriv->spu_class;
        classcode_t  diffb = ncc ^ defcc;               /* Bits which are different */

        /* Diffb contains bits that are different from the default class.
           If we & with the default class and the result is zero then the bits
           set in diffb are in excess of the default class. In which case
           make it the default case otherwise 0 */

        set_cc_butts(buttlist, diffb && (diffb & defcc) == 0? defcc: 0);
}

/* Set up class code section of dialog for jobs and printers.
   Pass the "vbox" of the dialog (might be a sub-one) to put it in.
   Pass the array of checkboxes which we create.
   Pass the existing class code */

void  cc_dlgsetup(GtkWidget *vbox, GtkWidget **ccbuts, const classcode_t existing)
{
        GtkWidget       *hbox, *button;
        int             rcnt, ccnt, bitnum;
        char            *pr, bname[2];

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        /* Create "Set all" button */

        pr = gprompt($P{xspq ccdlg setall});
        button = gtk_button_new_with_label(pr);
        free(pr);
        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(cc_setall), ccbuts);

        /* Create "Clear all" button */

        pr = gprompt($P{xspq ccdlg clearall});
        button = gtk_button_new_with_label(pr);
        free(pr);
        gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(cc_clearall), ccbuts);

        /* Set up the class codes in 8 rows of 4 */

        bitnum = 0;
        bname[1] = '\0';        /* Quickly make string by setting first element */

        for  (rcnt = 0;  rcnt < 4;  rcnt++)  {
                hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
                gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

                for  (ccnt = 0;  ccnt < 4;  ccnt++)  {
                        bname[0] = 'A' + bitnum;
                        button = ccbuts[bitnum] = gtk_check_button_new_with_label(bname);
                        if  (existing & (1 << bitnum))
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
                        button = ccbuts[bitnum] = gtk_check_button_new_with_label(bname);
                        if  (existing & (1 << bitnum))
                                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
                        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
                        bitnum++;
                }
        }
}

/* Create a combo_box_entry initialised to the result of the passed function
   and with the entry field filled with "existing" */

GtkWidget *make_combo_box_entry(void (*fn)(struct stringvec *), const char *existing)
{
        struct  stringvec  possibles;
        GtkWidget *comb;
        int     cnt;

        stringvec_init(&possibles);
        (*fn)(&possibles);

        comb = gtk_combo_box_entry_new_text();
        if  (strlen(existing) != 0)
                gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(comb))), existing);
        for  (cnt = 0;  cnt < stringvec_count(possibles);  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(comb), stringvec_nth(possibles, cnt));
        stringvec_free(&possibles);
        return  comb;
}

struct  dialog_data  {
        GtkWidget       *noconfirm, *unprintedconfirm, *confirmall;
        GtkWidget       *usersel, *ptrsel, *titleres;
        GtkWidget       *classcodes[32];
        GtkWidget       *viewallh, *viewlocalh;
        GtkWidget       *allprint, *unprinted, *printed;
        GtkWidget       *justprin, *prinplusnull, *anyprin;
};

GtkWidget *create_viewopt_dlg(struct dialog_data *ddata)
{
        GtkWidget  *frame, *vbox;
        char    *pr;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab vof opt});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        ddata->noconfirm = gprompt_radiobutton($P{xspq opt no conf});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->noconfirm, FALSE, FALSE, DEF_DLG_VPAD);

        ddata->unprintedconfirm = gprompt_radiobutton_fromwidget(ddata->noconfirm, $P{xspq opt unp conf});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->unprintedconfirm, FALSE, FALSE, DEF_DLG_VPAD);

        ddata->confirmall = gprompt_radiobutton_fromwidget(ddata->noconfirm, $P{xspq opt all conf});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->confirmall, FALSE, FALSE, DEF_DLG_VPAD);
        if  (confabort == 2)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->confirmall), TRUE);
        else  if  (confabort == 1)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->unprintedconfirm), TRUE);
        else
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->noconfirm), TRUE);
        return  frame;
}

void  extract_viewopt_dlg(struct dialog_data *ddata)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->noconfirm)))
                confabort = 0;
        else  if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->unprintedconfirm)))
                confabort = 1;
        else
                confabort = 2;
}

GtkWidget *create_viewuser_dlg(struct dialog_data *ddata)
{
        GtkWidget  *frame, *vbox, *hbox, *lab;
        char    *pr, **ulist, **ulp;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab vof user});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq opt user lab});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        /* Make combo box with user list in */

        ulist = gen_ulist((char *) 0, 0);
        ddata->usersel = gtk_combo_box_entry_new_text();
        if  (Displayopts.opt_restru)
                gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->usersel))), Displayopts.opt_restru);
        for  (ulp = ulist;  *ulp;  ulp++)  {
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->usersel), *ulp);
                free(*ulp);
        }
        free((char *) ulist);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->usersel, FALSE, FALSE, DEF_DLG_HPAD);

        ddata->viewallh = gprompt_radiobutton($P{xspq opt all hosts});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->viewallh, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->viewlocalh = gprompt_radiobutton_fromwidget(ddata->viewallh, $P{xspq opt local only});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->viewlocalh, FALSE, FALSE, DEF_DLG_VPAD);

        if  (Displayopts.opt_localonly != NRESTR_NONE)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->viewlocalh), TRUE);
        else
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->viewallh), TRUE);

        return  frame;
}

void  extract_viewuser_dlg(struct dialog_data *ddata)
{
        const  char     *newu = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->usersel))));

        if  (Displayopts.opt_restru)  {
                free(Displayopts.opt_restru);
                Displayopts.opt_restru = 0;
        }
        if  (strlen(newu) != 0)
                Displayopts.opt_restru = stracpy(newu);

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->viewlocalh)))
                Displayopts.opt_localonly = NRESTR_LOCALONLY;
        else
                Displayopts.opt_localonly = NRESTR_NONE;
}

GtkWidget *create_viewptr_dlg(struct dialog_data *ddata)
{
        GtkWidget  *frame, *vbox, *hbox, *lab;
        char    *pr;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab vof ptr});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq opt ptr lab});

        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        ddata->ptrsel = make_combo_box_entry(wotjprin, Displayopts.opt_restrp? Displayopts.opt_restrp: "");
        gtk_box_pack_start(GTK_BOX(hbox), ddata->ptrsel, FALSE, FALSE, DEF_DLG_HPAD);

        ddata->justprin = gprompt_radiobutton($P{xspq opt pincl match});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->justprin, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->prinplusnull = gprompt_radiobutton_fromwidget(ddata->justprin, $P{xspq opt pincl matchnull});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->prinplusnull, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->anyprin = gprompt_radiobutton_fromwidget(ddata->justprin, $P{xspq opt pincl all});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->anyprin, FALSE, FALSE, DEF_DLG_VPAD);

        if  (Displayopts.opt_jinclude == JINCL_NONULL)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->justprin), TRUE);
        else  if  (Displayopts.opt_jinclude == JINCL_NULL)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->prinplusnull), TRUE);
        else
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->anyprin), TRUE);

        return  frame;
}

void  extract_viewptr_dlg(struct dialog_data *ddata)
{
        const  char     *newp = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ddata->ptrsel))));

        if  (Displayopts.opt_restrp)  {
                free(Displayopts.opt_restrp);
                Displayopts.opt_restrp = 0;
        }
        if  (strlen(newp) != 0)
                Displayopts.opt_restrp = stracpy(newp);

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->justprin)))
                Displayopts.opt_jinclude = JINCL_NONULL;
        else  if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->prinplusnull)))
                Displayopts.opt_jinclude = JINCL_NULL;
        else
                Displayopts.opt_jinclude = JINCL_ALL;
}

GtkWidget *create_viewtit_dlg(struct dialog_data *ddata)
{
        GtkWidget  *frame, *vbox, *hbox, *lab;
        char    *pr;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framelab vof tit});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq opt tit lab});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        ddata->titleres = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), ddata->titleres, FALSE, FALSE, 0);

        if  (Displayopts.opt_restrt)
                gtk_entry_set_text(GTK_ENTRY(ddata->titleres), Displayopts.opt_restrt);

        ddata->allprint = gprompt_radiobutton($P{xspq opt tit allpr});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->allprint, FALSE, FALSE, DEF_DLG_VPAD);

        ddata->unprinted = gprompt_radiobutton_fromwidget(ddata->allprint, $P{xspq opt tit unpr});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->unprinted, FALSE, FALSE, DEF_DLG_VPAD);

        ddata->printed = gprompt_radiobutton_fromwidget(ddata->allprint, $P{xspq opt tit pr});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->printed, FALSE, FALSE, DEF_DLG_VPAD);

        if  (Displayopts.opt_jprindisp == JRESTR_ALL)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->allprint), TRUE);
        else  if  (Displayopts.opt_jprindisp == JRESTR_UNPRINT)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->unprinted), TRUE);
        else
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->printed), TRUE);

        return  frame;
}

void  extract_viewtit_dlg(struct dialog_data *ddata)
{
        const  char     *newt = gtk_entry_get_text(GTK_ENTRY(ddata->titleres));

        if  (Displayopts.opt_restrt)  {
                free(Displayopts.opt_restrt);
                Displayopts.opt_restrt = 0;
        }
        if  (strlen(newt) != 0)
                Displayopts.opt_restrt = stracpy(newt);

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->allprint)))
                Displayopts.opt_jprindisp = JRESTR_ALL;
        else  if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->unprinted)))
                Displayopts.opt_jprindisp = JRESTR_UNPRINT;
        else
                Displayopts.opt_jprindisp = JRESTR_PRINT;
}

GtkWidget *create_viewcc_dlg(struct dialog_data *ddata)
{
        GtkWidget  *frame, *vbox;
        char    *pr;

        frame = gtk_frame_new(NULL);
        pr = gprompt($P{xspq framlab vof cc});
        gtk_frame_set_label(GTK_FRAME(frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        cc_dlgsetup(vbox, ddata->classcodes, Displayopts.opt_classcode);
        return  frame;
}

void  extract_viewcc_dlg(struct dialog_data *ddata)
{
        classcode_t  ncc = read_cc_butts(ddata->classcodes);

        if  (!(mypriv->spu_flgs & PV_COVER))
                ncc &= mypriv->spu_class;
        if  (ncc == 0)
                ncc = mypriv->spu_class;
        Displayopts.opt_classcode = ncc;
}

void  cb_viewopt()
{
        GtkWidget  *dlg, *lab, *notebook, *userdlg, *ptrdlg, *titdlg, *ccdlg, *optdlg;
        char    *pr;
        struct  dialog_data     ddata;

        pr = gprompt($P{xspq viewopt dialogtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);
        notebook = gtk_notebook_new();
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
        optdlg = create_viewopt_dlg(&ddata);
        userdlg = create_viewuser_dlg(&ddata);
        ptrdlg = create_viewptr_dlg(&ddata);
        titdlg = create_viewtit_dlg(&ddata);
        ccdlg = create_viewcc_dlg(&ddata);

        /* Set up tab names for notebook */

        lab = gprompt_label($P{xspq frametab vo opt});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), optdlg, lab);

        lab = gprompt_label($P{xspq frametab vo user});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), userdlg, lab);

        lab = gprompt_label($P{xspq frametab vo ptr});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ptrdlg, lab);

        lab = gprompt_label($P{xspq frametab vo title});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), titdlg, lab);

        lab = gprompt_label($P{xspq frametab vo cc});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), ccdlg, lab);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), notebook, TRUE, TRUE, NOTE_PADDING);
        gtk_widget_show_all(dlg);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                extract_viewopt_dlg(&ddata);
                extract_viewuser_dlg(&ddata);
                extract_viewptr_dlg(&ddata);
                extract_viewtit_dlg(&ddata);
                extract_viewcc_dlg(&ddata);
                Job_seg.Last_ser = Ptr_seg.Last_ser = 0;
                Dirty = 1;
        }
        gtk_widget_destroy(dlg);
}

void  cb_saveopts()
{
        PIDTYPE pid;
        static  char    *gtkprog;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  (!gtkprog)
                gtkprog = envprocess(GTKSAVE);

        if  ((pid = fork()) == 0)  {
                char    *jf = gen_jfmts(), *pf = gen_pfmts();
                char    digbuf[4], *argbuf[16 + 8 * MAXMACS];
                char    **ap = argbuf;
                int     cnt;
                *ap++ = gtkprog; /* Arg 0 is the program we're running */
                *ap++ = "XSPQDISPOPT";
                digbuf[0] = Displayopts.opt_jinclude + '0';
                digbuf[1] = Displayopts.opt_jprindisp + '0';
                digbuf[2] = Displayopts.opt_localonly + '0';
                digbuf[3] = '\0';
                *ap++ = digbuf;
                *ap++ = "XSPQDISPCC";
                *ap++ = hex_disp(Displayopts.opt_classcode, 0);
                *ap++ = "XSPQDISPUSER";
                *ap++ = Displayopts.opt_restru? Displayopts.opt_restru: "-";
                *ap++ = "XSPQDISPPTR";
                *ap++ = Displayopts.opt_restrp? Displayopts.opt_restrp: "-";
                *ap++ = "XSPQDISPTIT";
                *ap++ = Displayopts.opt_restrt? Displayopts.opt_restrt: "-";
                *ap++ = "XSPQJOBFLD";
                *ap++ = jf;
                *ap++ = "XSPQPTRFLD";
                *ap++ = pf;
                for  (cnt = 0;  cnt < MAXMACS;  cnt++)  {
                        struct macromenitem  *mi = &jobmacs[cnt];
                        char  nbuf[14];
                        sprintf(nbuf, "XSPQJOBMAC%d", cnt+1);
                        *ap++ = stracpy(nbuf);
                        *ap++ = mi->cmd? mi->cmd: "-";
                        sprintf(nbuf, "XSPQJOBMACD%d", cnt+1);
                        *ap++ = stracpy(nbuf);
                        *ap++ = mi->descr? mi->descr: "-";
                }
                for  (cnt = 0;  cnt < MAXMACS;  cnt++)  {
                        struct macromenitem  *mi = &ptrmacs[cnt];
                        char  nbuf[14];
                        sprintf(nbuf, "XSPQPTRMAC%d", cnt+1);
                        *ap++ = stracpy(nbuf);
                        *ap++ = mi->cmd? mi->cmd: "-";
                        sprintf(nbuf, "XSPQPTRMACD%d", cnt+1);
                        *ap++ = stracpy(nbuf);
                        *ap++ = mi->descr? mi->descr: "-";
                }
                *ap = 0;
                execv(execprog, argbuf);
                exit(E_SETUP);
        }

        if  (pid < 0)  {
                doerror($EH{xspq saveopts no fork});
                return;
        }
#ifdef  HAVE_WAITPID
        while  (waitpid(pid, &status, 0) < 0)
                ;
#else
        while  (wait(&status) != pid)
                ;
#endif
        if  (status != 0)  {
                disp_arg[0] = status >> 8;
                disp_arg[1] = status & 127;
                doerror($EH{xspq saveopts failed});
                return;
        }
        Dirty = 0;
}

extern  USHORT  *def_jobflds, *def_ptrflds;
extern  int     ndef_jobflds, ndef_ptrflds;

int  parse_fldarg(char *arg, USHORT **list)
{
        char  *cp, *np;
        int     result = 1;
        USHORT  *lp;

        /* Count bits */

        for  (cp = arg;  (np = strchr(cp, ','));  cp = np+1)
                result++;

        *list = (USHORT *) malloc((unsigned) (result * sizeof(USHORT)));
        if  (!*list)
                nomem();

        lp = *list;

        for  (cp = arg;  (np = strchr(cp, ','));  cp = np+1)
                *lp++ = atoi(cp);
        *lp = atoi(cp);

        free(arg);
        return  result;
}

void  loadmac(struct macromenitem *mlist, const int cnt, const char *jorp)
{
        char    nbuf[16], *cmd, *descr;
        sprintf(nbuf, "XSPQ%sMAC%d", jorp, cnt);
        cmd = optkeyword(nbuf);
        sprintf(nbuf, "XSPQ%sMACD%d", jorp, cnt);
        descr = optkeyword(nbuf);
        if  (cmd  &&  descr)  {
                mlist[cnt-1].cmd = cmd;
                mlist[cnt-1].descr = descr;
        }
        else  {
                if  (cmd)
                        free(cmd);
                if  (descr)
                        free(descr);
        }
}

/* Put this here to co-ordinate with above hopefully */

void  load_optfile()
{
        char    *arg;
        int     cnt;

        /* Display options */

        if  ((arg = optkeyword("XSPQDISPOPT")))  {
                if  (arg[0])  {
                        Displayopts.opt_jinclude = arg[0] - '0';
                        if  (arg[1])  {
                                Displayopts.opt_jprindisp = arg[1] - '0';
                                if  (arg[2])
                                        Displayopts.opt_localonly = arg[2] - '0';
                        }
                }
                free(arg);
        }

        if  ((arg = optkeyword("XSPQDISPCC")))  {
                Displayopts.opt_classcode = hextoi(arg);
                free(arg);
        }

        if  ((arg = optkeyword("XSPQDISPUSER")))  {
                if  (strcmp(arg, "-") != 0)
                        Displayopts.opt_restru = arg;
                else
                        free(arg);
        }

        if  ((arg = optkeyword("XSPQDISPPTR")))  {
                if  (strcmp(arg, "-") != 0)
                        Displayopts.opt_restrp = arg;
                else
                        free(arg);
        }

        if  ((arg = optkeyword("XSPQDISPTIT")))  {
                if  (strcmp(arg, "-") != 0)
                        Displayopts.opt_restrt = arg;
                else
                        free(arg);
        }

        if  ((arg = optkeyword("XSPQJOBFLD")))
                ndef_jobflds = parse_fldarg(arg, &def_jobflds);

        if  ((arg = optkeyword("XSPQPTRFLD")))
                ndef_ptrflds = parse_fldarg(arg, &def_ptrflds);

        for  (cnt = 1;  cnt <= MAXMACS;  cnt++)  {
                loadmac(jobmacs, cnt, "JOB");
                loadmac(ptrmacs, cnt, "PTR");
        }

        close_optfile();
}

char    menutmpl[] =
"<ui>"
"<menubar name='MenuBar'>"
"<menu action='%cmacMenu'>"
"<placeholder name='%cmac%d'>"
"<menuitem action='%s'/>"
"</placeholder>"
"</menu>"
"</menubar>"
"</ui>";

extern void  jmacruncb(GtkAction *, struct macromenitem *);
extern void  pmacruncb(GtkAction *, struct macromenitem *);
extern  GtkUIManager    *ui;

void  setup_macro(const char jorp, struct macromenitem *mlist, const int macnum)
{
        GtkAction  *act;
        GtkActionGroup  *grp;
        char    anbuf[10], gnbuf[10];
        GString *uif;

        sprintf(anbuf, "%cm%d", jorp, macnum);
        act = gtk_action_new(anbuf, mlist[macnum-1].descr, NULL, NULL);
        g_signal_connect(act, "activate", jorp == 'j'? G_CALLBACK(jmacruncb): G_CALLBACK(pmacruncb), &mlist[macnum-1]);
        sprintf(gnbuf, "%cmg%d", jorp, macnum);
        grp = gtk_action_group_new(gnbuf);
        gtk_action_group_add_action(grp, act);
        g_object_unref(G_OBJECT(act));
        gtk_ui_manager_insert_action_group(ui, grp, 0);
        g_object_unref(G_OBJECT(grp));
        uif = g_string_new(NULL);
        g_string_printf(uif, menutmpl, jorp, jorp, macnum, anbuf);
        mlist[macnum-1].mergeid = gtk_ui_manager_add_ui_from_string(ui, uif->str, -1, NULL);
        g_string_free(uif, TRUE);
}

void  delete_macro(const char jorp, struct macromenitem *mlist, const int macnum)
{
        GList  *groups, *lp;
        char    gnbuf[10];

        sprintf(gnbuf, "%cmg%d", jorp, macnum);
        gtk_ui_manager_remove_ui(ui, mlist[macnum-1].mergeid);
        groups = gtk_ui_manager_get_action_groups(ui);
        for  (lp = groups;  lp;  lp = lp->next)  {
                GtkActionGroup  *grp = (GtkActionGroup *) lp->data;
                const  char  *nameg = gtk_action_group_get_name(grp);
                if  (strcmp(nameg, gnbuf) == 0)  {
                        gtk_ui_manager_remove_action_group(ui, grp);
                        break;
                }
        }
}

void  loadmacs(const char jorp, struct macromenitem *mlist)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAXMACS;  cnt++)
                if  (mlist[cnt].cmd)
                        setup_macro(jorp, mlist, cnt+1);
}

char *get_macro_description()
{
        GtkWidget *dlg, *lab, *ent;
        char    *pr, *result = (char *) 0;

        pr = gprompt($P{xspq macname dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);
        lab = gprompt_label($P{xspq macname lab});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, DEF_DLG_VPAD);
        ent = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ent, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *res = gtk_entry_get_text(GTK_ENTRY(ent));
                if  (strlen(res) == 0)  {
                        doerror($EH{xspq macname null});
                        continue;
                }
                result = stracpy(res);
                break;
        }
        gtk_widget_destroy(dlg);
        return  result;
}

int  add_macro_to_list(const char *cmdtext, const char jorp, struct macromenitem *mlist)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAXMACS;  cnt++)  {
                if  (!mlist[cnt].cmd)  {
                        char    *descr;
                        if  (!Confirm($PH{xspq addmac to list}))
                                return  0;
                        if  (!(descr = get_macro_description()))
                                return  0;
                        mlist[cnt].cmd = stracpy(cmdtext);
                        mlist[cnt].descr = descr;
                        setup_macro(jorp, mlist, cnt+1);
                        Dirty++;
                        return  1;
                }
        }
        return  0;
}

struct  macupddata  {
        GtkWidget  *view;
        GtkListStore    *store;
        char    jorp;
        struct macromenitem *mlist;
};

GtkWidget *make_push_button(const int code)
{
        GtkWidget  *button, *hbox, *lab;
        button = gtk_button_new();
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_container_add(GTK_CONTAINER(button), hbox);
        lab = gprompt_label(code);
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_BUTTON_PAD);
        return  button;
}

void  editmac(struct macupddata *mdata, const int cnt)
{
        int     macnum = cnt + 1;
        GtkWidget  *dlg, *hbox, *lab, *cmdw, *descrw;
        char    *pr;

        pr = gprompt($P{xspq mac dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq macdlg cmd});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        cmdw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), cmdw, FALSE, FALSE, DEF_DLG_HPAD);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq macdlg descr});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        descrw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), descrw, FALSE, FALSE, DEF_DLG_HPAD);

        if  (mdata->mlist[cnt].cmd)  {
                gtk_entry_set_text(GTK_ENTRY(cmdw), mdata->mlist[cnt].cmd);
                gtk_entry_set_text(GTK_ENTRY(descrw), mdata->mlist[cnt].descr);
        }

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *newc = gtk_entry_get_text(GTK_ENTRY(cmdw));
                const  char  *newd = gtk_entry_get_text(GTK_ENTRY(descrw));
                GtkTreeIter  iter;

                if  (strlen(newc) == 0)  {
                        doerror($EH{xspq maccmd null});
                        continue;
                }
                if  (strlen(newd) == 0)  {
                        doerror($EH{xspq macname null});
                        continue;
                }

                if  (mdata->mlist[cnt].cmd)  {
                        delete_macro(mdata->jorp, mdata->mlist, macnum);
                        free(mdata->mlist[cnt].cmd);
                        free(mdata->mlist[cnt].descr);
                        gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(mdata->store), &iter, NULL, cnt);
                }
                else
                        gtk_list_store_append(mdata->store, &iter);

                mdata->mlist[cnt].cmd = stracpy(newc);
                mdata->mlist[cnt].descr = stracpy(newd);
                gtk_list_store_set(mdata->store, &iter, 0, cnt, 1, mdata->mlist[cnt].cmd, 2, mdata->mlist[cnt].descr, -1);
                setup_macro(mdata->jorp, mdata->mlist, macnum);
                Dirty++;
                break;
        }

        gtk_widget_destroy(dlg);
}

void  newmac_clicked(struct macupddata *mdata)
{
        int     cnt;
        for  (cnt = 0;  cnt < MAXMACS;  cnt++)
                if  (!mdata->mlist[cnt].cmd)  {
                        editmac(mdata, cnt);
                        break;
                }
}

void  delmac_clicked(struct macupddata *mdata)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(mdata->view));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                gint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(mdata->store), &iter, 0, &seq, -1);
                delete_macro(mdata->jorp, mdata->mlist, seq+1);
                gtk_list_store_remove(mdata->store, &iter);
                free(mdata->mlist[seq].cmd);
                free(mdata->mlist[seq].descr);
                mdata->mlist[seq].cmd = 0;
                mdata->mlist[seq].descr = 0;
                Dirty++;
        }
}

void  updmac_clicked(struct macupddata *mdata)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(mdata->view));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                gint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(mdata->store), &iter, 0, &seq, -1);
                editmac(mdata, seq);
        }
}

static void mlist_dblclk(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, struct macupddata *mdata)
{
        GtkTreeIter     iter;
        if  (gtk_tree_model_get_iter(GTK_TREE_MODEL(mdata->store), &iter, path))  {
                gint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(mdata->store), &iter, 0, &seq, -1);
                editmac(mdata, seq);
        }
}

void  macro_edit(const char jorp, struct macromenitem *mlist)
{
        GtkWidget  *dlg, *mwid, *scroll, *hbox, *butt;
        GtkCellRenderer     *rend;
        GtkListStore    *mlist_store;
        GtkTreeSelection *sel;
        struct  macupddata      mdata;
        int     cnt;
        char    *pr;

        mlist_store = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);

        for  (cnt = 0;  cnt < MAXMACS;  cnt++)
                if  (mlist[cnt].cmd)  {
                        GtkTreeIter   iter;
                        gtk_list_store_append(mlist_store, &iter);
                        gtk_list_store_set(mlist_store, &iter, 0, cnt, 1, mlist[cnt].cmd, 2, mlist[cnt].descr, -1);
                }

        pr = gprompt($P{xspq macedit dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
        free(pr);
        mwid = gtk_tree_view_new();
        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(mwid), -1, "Command", rend, "text", 1, NULL);
        gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(mwid), 0), TRUE);
        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(mwid), -1, "Description", rend, "text", 2, NULL);
        gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(mwid), 1), TRUE);

        gtk_tree_view_set_model(GTK_TREE_VIEW(mwid), GTK_TREE_MODEL(mlist_store));
        g_object_unref(mlist_store);            /* So that it gets deallocated */

        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), mwid);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, TRUE, TRUE, DEF_DLG_VPAD, GTK_PACK_START);

        mdata.view = mwid;
        mdata.store = mlist_store;
        mdata.mlist = mlist;
        mdata.jorp = jorp;

        hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD, GTK_PACK_START);
        butt = make_push_button($P{xspq new macro});
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(newmac_clicked), &mdata);
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);
        butt = make_push_button($P{xspq del macro});
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(delmac_clicked), &mdata);
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);
        butt = make_push_button($P{xspq edit macro});
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(updmac_clicked), &mdata);
        g_signal_connect(G_OBJECT(mwid), "row-activated", (GCallback) mlist_dblclk, (gpointer) &mdata);
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);

        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(mwid));
        gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

        gtk_widget_show_all(dlg);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
}

void  cb_jmacedit()
{
        macro_edit('j', jobmacs);
}

void  cb_pmacedit()
{
        macro_edit('p', ptrmacs);
}
