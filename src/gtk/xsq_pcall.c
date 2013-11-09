/* xsq_pcall.c -- xspq callback routines specific to printers

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
#include <gtk/gtk.h>
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
#include "xsq_ext.h"
#include "gtk_lib.h"
#include "stringvec.h"

extern  char    *execprog;

struct  macromenitem    ptrmacs[MAXMACS];

extern  GtkWidget *make_combo_box_entry(void (*)(struct stringvec *), const char *);
extern void     wotpform(struct stringvec *);
extern void     wotpprin(struct stringvec *);
extern void     wottty(struct stringvec *);
extern classcode_t      read_cc_butts(GtkWidget **);
extern void     cc_dlgsetup(GtkWidget *, GtkWidget **, const classcode_t);
extern int      validatedev(const char *);

static const Hashspptr *getselectedptr()
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pwid));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                guint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(sorted_plist_store), &iter, SEQ_COL, &seq, -1);
                return  Ptr_seg.pp_ptrs[seq];
        }
        return  NULL;
}

static const struct spptr *getselectedptr_chk(ULONG perm, int code)
{
        const  Hashspptr  *result = getselectedptr();
        const  struct  spptr    *rpt;

        if  (!result)  {
                doerror(Ptr_seg.nptrs != 0? $EH{No printer selected}: $EH{No printer to select});
                return  NULL;
        }

        rpt = &result->p;

        if  (perm  &&  !(mypriv->spu_flgs & perm))  {
                disp_str = rpt->spp_ptr;
                disp_str2 = rpt->spp_dev;
                doerror(code);
                return  NULL;
        }

        if  (rpt->spp_netid  &&  !(mypriv->spu_flgs & PV_REMOTEP))  {
                doerror($EH{No remote ptr priv});
                return  NULL;
        }

        PREQ = *rpt;
        OREQ = PREQS = result - Ptr_seg.plist;
        return  rpt;
}

/* Printer actions. */

void  cb_pact(GtkAction *action)
{
        const  struct   spptr   *pp;
        const  char     *act = gtk_action_get_name(action);
        int     msg;

        switch  (act[0])  {
        case  'G':
                if  (strcmp(act, "Go") == 0)  {
                        msg = SO_PGO;
                        break;
                }
        case  'H':
                if  (strcmp(act, "Heoj") == 0)  {
                        msg = SO_PHLT;
                        break;
                }
                if  (strcmp(act, "Halt") == 0)  {
                        msg = SO_PSTP;
                        break;
                }
        case  'O':
                if  (strcmp(act, "Ok") == 0)  {
                        msg = SO_OYES;
                        break;
                }
        case  'N':
                if  (strcmp(act, "NOk") == 0)  {
                        msg = SO_ONO;
                        break;
                }
        case  'I':
                if  (strcmp(act, "Interrupt") == 0)  {
                        msg = SO_INTER;
                        break;
                }
        case  'A':
                if  (strcmp(act, "Abortp") == 0)  {
                        msg = SO_PJAB;
                        break;
                }
        case  'R':
                if  (strcmp(act, "Restart") == 0)  {
                        msg = SO_RSP;
                        break;
                }
        case  'D':
                if  (strcmp(act, "Delete") == 0)  {
                        msg = SO_DELP;
                        break;
                }
        default:
                return;
        }

        switch  (msg)  {
        default:
                return;
        case  SO_PGO:
                pp = getselectedptr_chk(PV_HALTGO, $EH{No stop start priv});
                if  (!pp)
                        return;
                if  (pp->spp_state >= SPP_PROC  &&  !(pp->spp_sflags & SPP_HEOJ))  {
                        doerror($EH{Printer is running});
                        return;
                }
                break;
        case  SO_PHLT:
        case  SO_PSTP:
                pp = getselectedptr_chk(PV_HALTGO, $EH{No stop start priv});
                if  (!pp)
                        return;
                if  (pp->spp_state < SPP_PROC)  {
                        doerror($EH{Printer not running});
                        return;
                }
                break;
        case  SO_OYES:
        case  SO_ONO:
                pp = getselectedptr_chk(PV_PRINQ, $EH{No printer list access});
                if  (!pp)
                        return;
                if  (pp->spp_state != SPP_OPER)  {
                        if  (pp->spp_state != SPP_WAIT)  {
                                doerror($EH{Printer not aw oper});
                                return;
                        }
                        disp_str = pp->spp_ptr;
                        if  (!Confirm(msg == SO_OYES? $PH{xmspq bypass align}: $PH{xmspq reinstate align}))
                                return;
                }
                break;
        case  SO_INTER:
                pp = getselectedptr_chk(PV_HALTGO, $EH{No stop start priv});
                if  (!pp)
                        return;
                if  (pp->spp_state != SPP_RUN)  {
                        doerror($EH{Printer not running});
                        return;
                }
                break;
        case  SO_PJAB:
        case  SO_RSP:
                pp = getselectedptr_chk(PV_PRINQ, $EH{No printer list access});
                if  (!pp)
                        return;
                if  (pp->spp_state != SPP_RUN)  {
                        doerror($EH{Printer not running});
                        return;
                }
                break;
        case  SO_DELP:
                pp = getselectedptr_chk(PV_ADDDEL, $EH{No delete priv});
                if  (!pp)
                        return;
                if  (pp->spp_netid)  {
                        disp_str = look_host(pp->spp_netid);
                        doerror($EH{Printer is remote});
                        return;
                }
                disp_str = pp->spp_ptr;
                if  (!Confirm($PH{xmspq confirm delete printer}))
                        return;
                if  (pp->spp_state >= SPP_PROC)  {
                        doerror($EH{Printer is running});
                        return;
                }
                break;
        }
        womsg(msg);
}

void  set_ptrdlglab(GtkWidget *dlg, const struct spptr *pp)
{
        char  *pr = gprompt($P{xspq pdlg lab});
        GString *labp = g_string_new(NULL);
        GtkWidget  *lab;

        g_string_printf(labp, "%s %s", pr, PTR_NAME(pp));
        free(pr);
        lab = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, DEF_DLG_VPAD);
}

void  cb_pform()
{
        char    *pr;
        GtkWidget  *dlg, *hbox, *lab, *formsel;
        const  struct  spptr  *pp = getselectedptr_chk(PV_PRINQ, $EH{No printer list access});

        if  (!pp)
                return;

        if  (pp->spp_state >= SPP_PROC)  {
                doerror($EH{Printer is running});
                return;
        }

        pr = gprompt($P{xspq form printer dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);
        set_ptrdlglab(dlg, pp);
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq pform lab});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        formsel = make_combo_box_entry(wotpform, pp->spp_form);
        gtk_box_pack_start(GTK_BOX(hbox), formsel, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *formname = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(formsel))));
                if  (strlen(formname) == 0)  {
                        doerror($EH{Null form name});
                        continue;
                }
                if  (strcmp(PREQ.spp_form, formname) != 0)  {
                        strncpy(PREQ.spp_form, formname, MAXFORM);
                        my_wpmsg(SP_CHGP);
                }
                break;
        }

        gtk_widget_destroy(dlg);
}

void  cb_pclass()
{
        char    *pr;
        const  struct   spptr   *pp = getselectedptr_chk(PV_ADDDEL, $EH{No perm change class});
        GtkWidget  *dlg, *hbox, *lab, *classcodes[32], *locw, *minspin, *maxspin;
        GtkAdjustment *adj;

        if  (!pp)
                return;

        if  (pp->spp_state >= SPP_PROC)  {
                doerror($EH{Printer is running});
                return;
        }

        pr = gprompt($P{xspq cc printer dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);
        set_ptrdlglab(dlg, pp);
        cc_dlgsetup(GTK_DIALOG(dlg)->vbox, classcodes, pp->spp_class);
        locw = gprompt_checkbutton($P{xspq newprin loconly});
        if  (pp->spp_netflags & SPP_LOCALONLY)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(locw), TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), locw, FALSE, FALSE, DEF_DLG_VPAD);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq cc dlg minsize});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) pp->spp_minsize, 0.0, 1E10, 1000.0, 1E6, 0.0);
        minspin = gtk_spin_button_new(adj, 0.2, 0);
        gtk_box_pack_start(GTK_BOX(hbox), minspin, FALSE, FALSE, DEF_DLG_HPAD);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq cc dlg maxsize});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) pp->spp_maxsize, 0.0, 1E10, 1000.0, 1E6, 0.0);
        maxspin = gtk_spin_button_new(adj, 0.2, 0);
        gtk_box_pack_start(GTK_BOX(hbox), maxspin, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                classcode_t  ncc = read_cc_butts(classcodes);
                gboolean  loco = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(locw));
                gdouble  mins = gtk_spin_button_get_value(GTK_SPIN_BUTTON(minspin));
                gdouble  maxs = gtk_spin_button_get_value(GTK_SPIN_BUTTON(maxspin));

                if  (!(mypriv->spu_flgs & PV_COVER))
                        ncc &= mypriv->spu_class;

                if  (ncc == 0)  {
                        doerror($EH{xmspq setting zero class ptr});
                        continue;
                }

                if  (mins > maxs)  {
                        doerror($EH{Minprint gt maxprint});
                        continue;
                }

                PREQ.spp_class = ncc;
                if  (loco)
                        PREQ.spp_netflags |= SPP_LOCALONLY;
                else
                        PREQ.spp_netflags &= ~SPP_LOCALONLY;

                PREQ.spp_minsize = (ULONG) mins;
                PREQ.spp_maxsize = (ULONG) mins;
                my_wpmsg(SP_CHGP);
                break;
        }

        gtk_widget_destroy(dlg);
}

void  cb_padd()
{
        char    *pr;
        GtkWidget  *dlg, *hbox, *lab, *ptrsel, *devsel, *nw, *formsel, *comm, *classcodes[32], *locw;

        if  (!(mypriv->spu_flgs & PV_ADDDEL))  {
                doerror($EH{No add priv});
                return;
        }

        pr = gprompt($P{xspq add printer dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);

        /* Row for printer name */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq newprin name lab});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        ptrsel = make_combo_box_entry(wotpprin, "");
        gtk_box_pack_start(GTK_BOX(hbox), ptrsel, FALSE, FALSE, DEF_DLG_HPAD);

        /* Row for device name */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq newprin dev lab});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        devsel = make_combo_box_entry(wottty, "");
        gtk_box_pack_start(GTK_BOX(hbox), devsel, FALSE, FALSE, DEF_DLG_HPAD);

        /* Row for network device */

        nw = gprompt_checkbutton($P{xspq ptr localhost});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), nw, FALSE, FALSE, DEF_DLG_VPAD);

        /* Row for form type */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq newprin form lab});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        formsel = make_combo_box_entry(wotpform, "");
        gtk_box_pack_start(GTK_BOX(hbox), formsel, FALSE, FALSE, DEF_DLG_HPAD);

        /* Row for printer description */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq ptr comment});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        comm = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), comm, FALSE, FALSE, DEF_DLG_HPAD);

        /* Class code stuff */

        cc_dlgsetup(GTK_DIALOG(dlg)->vbox, classcodes, mypriv->spu_class);

        /* Local only stuff */

        locw = gprompt_checkbutton($P{xspq newprin loconly});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), locw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Show the stuff */

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *ptrname = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ptrsel))));
                const  char  *devname = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(devsel))));
                const  char  *formname = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(formsel))));
                const  char  *comment = gtk_entry_get_text(GTK_ENTRY(comm));
                classcode_t  ncc = read_cc_butts(classcodes);
                gboolean  isnetw = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nw));
                gboolean  loco = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(locw));

                if  (!(mypriv->spu_flgs & PV_COVER))
                        ncc &= mypriv->spu_class;

                /* Check for valid entries.
                   We insist on non-null printer, device and form names and a non-zero class code */

                if  (strlen(ptrname) == 0)  {
                        doerror($EH{Null ptr name});
                        gtk_window_set_focus(GTK_WINDOW(dlg), GTK_WIDGET(ptrsel));
                        continue;
                }
                if  (strlen(devname) == 0)  {
                        doerror($EH{Null device name});
                        gtk_window_set_focus(GTK_WINDOW(dlg), GTK_WIDGET(devsel));
                        continue;
                }
                if  (!isnetw)  {
                        int  valcode = validatedev(devname);
                        if  (valcode != 0  &&  !Confirm(valcode))
                                continue;
                }
                if  (strlen(formname) == 0)  {
                        doerror($EH{Null form name});
                        gtk_window_set_focus(GTK_WINDOW(dlg), GTK_WIDGET(formsel));
                        continue;
                }
                if  (ncc == 0)  {
                        doerror($EH{xmspq setting zero class ptr});
                        continue;
                }

                BLOCK_ZERO(&PREQ, sizeof(PREQ));
                strncpy(PREQ.spp_ptr, ptrname, PTRNAMESIZE);
                strncpy(PREQ.spp_dev, devname, LINESIZE);
                strncpy(PREQ.spp_form, formname, MAXFORM);
                strncpy(PREQ.spp_comment, comment, COMMENTSIZE);
                if  (isnetw)
                        PREQ.spp_netflags |= SPP_LOCALNET;
                if  (loco)
                        PREQ.spp_netflags |= SPP_LOCALONLY;
                PREQ.spp_class = ncc;
                my_wpmsg(SP_ADDP);
                break;
        }

        gtk_widget_destroy(dlg);
}

void  cb_pdev()
{
        GtkWidget  *dlg, *hbox, *lab, *devselc, *netf, *comm;
        char       *pr;
        const  struct   spptr   *pp = getselectedptr_chk(PV_ADDDEL, $EH{No perm change dev});

        if  (!pp)
                return;

        if  (pp->spp_state >= SPP_PROC)  {
                doerror($EH{Printer is running});
                return;
        }

        if  (pp->spp_netid)  {
                doerror($EH{Cannot change dev remote});
                return;
        }

        pr = gprompt($P{xspq pdev dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);
        set_ptrdlglab(dlg, pp);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq devip lab});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        devselc = make_combo_box_entry(wottty, pp->spp_dev);
        gtk_box_pack_start(GTK_BOX(hbox), devselc, FALSE, FALSE, DEF_DLG_HPAD);
        netf = gprompt_checkbutton($P{xspq ptr localhost});
        if  (pp->spp_netflags & SPP_LOCALNET)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(netf), TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), netf, FALSE, FALSE, DEF_DLG_VPAD);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xspq ptr comment});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        comm = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), comm, FALSE, FALSE, DEF_DLG_HPAD);
        if  (strlen(pp->spp_comment) != 0)
                gtk_entry_set_text(GTK_ENTRY(comm), pp->spp_comment);
        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *ndev = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(devselc))));
                gboolean  nw = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(netf));
                const  char  *descr = gtk_entry_get_text(GTK_ENTRY(comm));
                int     validdev = 0;

                if  (strlen(ndev) == 0)  {
                        doerror($EH{Null device name});
                        gtk_window_set_focus(GTK_WINDOW(dlg), GTK_WIDGET(devselc));
                        continue;
                }
                if  (!nw  &&  (validdev = validatedev(ndev))  &&  !Confirm(validdev))  {
                        gtk_window_set_focus(GTK_WINDOW(dlg), GTK_WIDGET(devselc));
                        continue;
                }

                strncpy(PREQ.spp_dev, ndev, LINESIZE);
                strncpy(PREQ.spp_comment, descr, COMMENTSIZE);
                if  (nw)
                        PREQ.spp_netflags |= SPP_LOCALNET;
                else
                        PREQ.spp_netflags &= ~SPP_LOCALNET;

                my_wpmsg(SP_CHGP);
                break;
        }
        gtk_widget_destroy(dlg);
}

static int  pmacroexec(const char *str, const struct spptr *pp)
{
        PIDTYPE pid;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  ((pid = fork()) == 0)  {
                const   char    *argbuf[3];
                argbuf[0] = (char *) str;
                if  (pp)  {
                        argbuf[1] = PTR_NAME(pp);
                        argbuf[2] = (char *) 0;
                }
                else
                        argbuf[1] = (char *) 0;
                Ignored_error = chdir(Curr_pwd);
                execv(execprog, (char **) argbuf);
                exit(255);
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

/* Version of printer macro for where we prompt */

void  cb_pmac()
{
        const  Hashspptr *hpp = getselectedptr();
        const  struct   spptr   *pp = 0;
        GtkWidget  *dlg, *lab, *hbox, *cmdentry;
        int        oldmac = is_init(previous_commands);
        char       *pr;

        pr = gprompt($P{xspg pmac dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        free(pr);

        if  (hpp)  {
                GString *labp = g_string_new(NULL);
                pp = &hpp->p;
                pr = gprompt($P{xspq pmac named});
                g_string_printf(labp, "%s %s", pr, pp->spp_ptr);
                free(pr);
                lab = gtk_label_new(labp->str);
                g_string_free(labp, TRUE);
        }
        else
                lab = gprompt_label($P{xspq pmac noname});

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_HPAD);
        lab = gprompt_label($P{xspq pmac cmd});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        if  (oldmac)  {
                unsigned  cnt;
                cmdentry = gtk_combo_box_entry_new_text();
                for  (cnt = 0;  cnt < stringvec_count(previous_commands);  cnt++)
                        gtk_combo_box_append_text(GTK_COMBO_BOX(cmdentry), stringvec_nth(previous_commands, cnt));
        }
        else
                cmdentry = gtk_entry_new();

        gtk_box_pack_start(GTK_BOX(hbox), cmdentry, FALSE, FALSE, DEF_DLG_HPAD);
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

                if  (pmacroexec(cmdtext, pp))  {
                        if  (add_macro_to_list(cmdtext, 'p', ptrmacs))
                                break;
                        if  (!oldmac)
                                stringvec_init(&previous_commands);
                        stringvec_insert_unique(&previous_commands, cmdtext);
                        break;
                }
        }
        gtk_widget_destroy(dlg);
}

void  pmacruncb(GtkAction *act, struct macromenitem *mitem)
{
        const  Hashspptr  *hpp = getselectedptr();
        const   struct  spptr  *pp = 0;
        if  (hpp)
                pp = &hpp->p;
        pmacroexec(mitem->cmd, pp);
}
