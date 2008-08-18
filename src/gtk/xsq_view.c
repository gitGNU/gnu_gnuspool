/* xsq_view.c -- view spool files in xspq

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
#include <gtk/gtk.h>
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
#include "xsq_ext.h"
#include "gtk_lib.h"

#define	DEFAULT_VIEW_WIDTH	500
#define	DEFAULT_VIEW_HEIGHT	600

#define	DEFAULT_SE_WIDTH	400
#define	DEFAULT_SE_HEIGHT	600

#define	INBUFSIZE	250
#define	PAGEBUF_INC	10

struct	view_data  {
	GtkTextBuffer  *fbuf;
	GtkWidget  	*dlg;			/* Dialog containing our view */
	GtkWidget	*startpl, *hatpl, *endpl; /* Labels to flag start/hat/endp */
	GtkWidget	*fromp;			/* Labels for from page */
	GtkWidget	*scroll, *view;		/* Scroll and TextView */
	unsigned	nphys;			/* Number of "physical" pages */
	LONG		*physpages;		/* Offsets in file of where pages start */
	LONG		*lineno_pagest;		/* Line numbers where pages start */
	ULONG		startp, hatp, endp;	/* Initial start/halted/end pages */
	int		isformfeed;		/* Greater than zero if non-formfeed delim */
	int		changes, changeable;	/* Record number of changes and if changeable */
	slotno_t	save_slot;
	const  struct	spq  *cj;
};

static	char	*startpmsg,
		*endpmsg,
		*hatpmsg;

static	char	*lastsearch;
static	int	lastwrap = 0;

extern int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
extern FILE	*net_feed(const int, const netid_t, const slotno_t, const jobno_t);

static void	cb_vexitsave(GtkAction *, struct view_data *);
static void	cb_vexitnosave(GtkAction *, struct view_data *);
static void	cb_vsearch(GtkAction *, struct view_data *);
static void	cb_vrsearch(GtkAction *, struct view_data *);
static void	cb_vspage(GtkAction *, struct view_data *);
static void	clearlog(GtkAction *, GtkWidget *);
static void	cb_quitsel(GtkAction *, GtkWidget *);

const struct spq *	getselectedjob_chk(const ULONG);

static GtkActionEntry ventries[] = {
	{ "VFileMenu", NULL, "_File"  },
	{ "VSearchMenu", NULL, "_Search"  },
	{ "SpageMenu", NULL, "_Pages"  },
	{ "ExitVSave", GTK_STOCK_QUIT, "_Exit and save", "<control>Q", "Exit view and save changes", G_CALLBACK(cb_vexitsave) },
	{ "ExitVNoS", GTK_STOCK_QUIT, "Exit _without saving", "<control>X", "Exit view without saving", G_CALLBACK(cb_vexitnosave) },
	{ "VSearch", NULL, "_Search for ...", NULL, "Search for string", G_CALLBACK(cb_vsearch)  },
	{ "VSearchf", NULL, "Search _forward", "F3", "Repeat last search going forward", G_CALLBACK(cb_vrsearch)  },
	{ "VSearchb", NULL, "Search _backward", "F4", "Repeat last search going backward", G_CALLBACK(cb_vrsearch)  },
	{ "Sstart", NULL, "Set _start page", NULL, "Set current to be start page", G_CALLBACK(cb_vspage)  },
	{ "Shat", NULL, "Set _halt page", NULL, "Set current to be halted-at page", G_CALLBACK(cb_vspage)  },
	{ "Send", NULL, "Set _end page", NULL, "Set current to be end page", G_CALLBACK(cb_vspage)  },
	{ "Crange", NULL, "_Clear range", NULL, "Clear page ranges", G_CALLBACK(cb_vspage)}  };

static GtkActionEntry slentries[] = {
	{ "SEFileMenu", NULL, "_File"  },
	{ "Clear", NULL, "_Clear log", NULL, "Clear log file and quit", G_CALLBACK(clearlog)},
	{ "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK(cb_quitsel)}  };

static int	get_top_scrolled_line(struct view_data * vd)
{
	GdkRectangle	vis_rect;
	GtkTextIter	vis_iter;

	gtk_text_view_get_visible_rect(GTK_TEXT_VIEW(vd->view), &vis_rect);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(vd->view), &vis_iter, vis_rect.x, vis_rect.y);
	return  gtk_text_iter_get_line(&vis_iter);
}

static LONG	get_top_scrolled_page(struct view_data * vd)
{
	int		linenum = get_top_scrolled_line(vd);
	unsigned	pagenum;

	for  (pagenum = 1;  pagenum < vd->cj->spq_npages;  pagenum++)
		if  (linenum < vd->lineno_pagest[pagenum])
			return  pagenum-1;
	return  vd->cj->spq_npages-1;
}

static void	vscroll_to(struct view_data *vd, GtkTextIter *posn)
{
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(vd->view), posn, 0.1, FALSE, 0.0, 0.0);
	gtk_text_buffer_place_cursor(vd->fbuf, posn);
}

static void	execute_vsearch(struct view_data *vd, const int isback)
{
	GtkTextIter  current_pos, match_start, buffbeg, buffend;
	GtkTextMark  *cpos;

	/* Kick off by getting current location */

	cpos = gtk_text_buffer_get_insert(vd->fbuf);
	gtk_text_buffer_get_iter_at_mark(vd->fbuf, &current_pos, cpos);

	if  (isback)  {
		if  (gtk_text_iter_backward_search(&current_pos, lastsearch, 0, &match_start, NULL, NULL))  {
			vscroll_to(vd, &match_start);
			return;
		}
		if  (lastwrap)  {
			gtk_text_buffer_get_end_iter(vd->fbuf, &buffend);
			if  (gtk_text_iter_backward_search(&buffend, lastsearch, 0, &match_start, NULL, NULL))  {
				vscroll_to(vd, &match_start);
				return;
			}
		}
		doerror($EH{spq view bs not found});
	}
	else  {
		int  chars = strlen(lastsearch);
		do  {
			if  (!gtk_text_iter_forward_char(&current_pos))
				break;
			chars--;
		}  while  (chars > 0);

		if  (gtk_text_iter_forward_search(&current_pos, lastsearch, 0, &match_start, NULL, NULL))  {
			vscroll_to(vd, &match_start);
			return;
		}
		if  (lastwrap)  {
			gtk_text_buffer_get_start_iter(vd->fbuf, &buffbeg);
			if  (gtk_text_iter_forward_search(&buffbeg, lastsearch, 0, &match_start, NULL, NULL))  {
				vscroll_to(vd, &match_start);
				return;
			}
		}
		doerror($EH{spq view fs not found});
	}
}

static void	cb_vsearch(GtkAction *action, struct view_data *vd)
{
	GtkWidget  *dlg, *hbox, *lab, *sstring, *wrapw, *backw;
	char	*pr;

	pr = gprompt($P{xspq vsearch dlg});
	dlg = gtk_dialog_new_with_buttons(pr,
					  GTK_WINDOW(toplevel),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_OK,
					  GTK_RESPONSE_OK,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_CANCEL,
					  NULL);
	free(pr);

	hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
	lab = gprompt_label($P{xspq vsearch str});
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

	sstring = gtk_entry_new();
	if  (lastsearch)
		gtk_entry_set_text(GTK_ENTRY(sstring), lastsearch);
	gtk_box_pack_start(GTK_BOX(hbox), sstring, FALSE, FALSE, DEF_DLG_HPAD);

	wrapw = gprompt_checkbutton($P{xspq vsearch wrap});
	if  (lastwrap)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrapw), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), wrapw, FALSE, FALSE, DEF_DLG_VPAD);

	backw = gprompt_checkbutton($P{xspq vsearch backw});
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), backw, FALSE, FALSE, DEF_DLG_VPAD);

	gtk_widget_show_all(dlg);

	while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
		const  char  *news = gtk_entry_get_text(GTK_ENTRY(sstring));
		int	isback;
		if  (strlen(news) == 0)  {
			doerror($EH{Null search string});
			continue;
		}
		if  (lastsearch)
			free(lastsearch);
		lastsearch = stracpy(news);
		lastwrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wrapw))? 1: 0;
		isback = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(backw))? 1: 0;
		execute_vsearch(vd, isback);
		break;
	}
	gtk_widget_destroy(dlg);
}

static void	cb_vrsearch(GtkAction *action, struct view_data *vd)
{
	const	char	*act = gtk_action_get_name(action);
	int	isback = strcmp(act, "VSearchb") == 0;

	if  (!lastsearch)  {
		doerror($EH{No previous search});
		return;
	}

	execute_vsearch(vd, isback);
}

static void	cb_vspage(GtkAction *action, struct view_data *vd)
{
	const	char	*act = gtk_action_get_name(action);

	if  (!vd->changeable)
		return;

	if  (strcmp(act, "Sstart") == 0)  {
		vd->startp = get_top_scrolled_page(vd);
		gtk_label_set_text(GTK_LABEL(vd->startpl), startpmsg);
		vd->changes++;
	}
	else  if  (strcmp(act, "Shat") == 0)  {
		vd->hatp = get_top_scrolled_page(vd);
		gtk_label_set_text(GTK_LABEL(vd->hatpl), hatpmsg);
		vd->changes++;
	}
	else  if  (strcmp(act, "Send") == 0)  {
		vd->endp = get_top_scrolled_page(vd);
		gtk_label_set_text(GTK_LABEL(vd->endpl), endpmsg);
		vd->changes++;
	}
	else  if  (strcmp(act, "Crange") == 0)  {
		vd->startp = 0;
		vd->hatp = 0;
		vd->endp = 0x7ffffffeL;
		vd->changes++;
		gtk_label_set_text(GTK_LABEL(vd->startpl), get_top_scrolled_page(vd) == 0? startpmsg: "");
		gtk_label_set_text(GTK_LABEL(vd->hatpl), "");
		gtk_label_set_text(GTK_LABEL(vd->endpl), "");
	}
}

static void	buff_append(GtkTextBuffer *buf, char *tbuf, const int size)
{
	GtkTextIter  iter;
	gtk_text_buffer_get_end_iter(buf, &iter);
	gtk_text_buffer_insert(buf, &iter, tbuf, size);
}

static void	free_view_data(struct view_data *vd)
{
	if  (vd->isformfeed > 0)
		free((char *) vd->physpages);
	free((char *) vd->lineno_pagest);
	free((char *) vd);
}

static void	fill_buff_spoolfile(FILE *inf, const struct spq *jp, struct view_data *vd)
{
	char	*delim;
	struct	pages	pfe;

	vd->nphys = 0;
	vd->physpages = 0;

	vd->isformfeed = rdpgfile(jp, &pfe, &delim, &vd->nphys, &vd->physpages);
	if  (vd->isformfeed > 0)  {
		LONG	fileoffset = 0;
		LONG	pagecount = 0;
		int	linecount = 0, colnum = 0, pendpage = 0;
		LONG	*pc = vd->physpages, *ps;
		int	ch, cp = 0;
		char	inbuf[INBUFSIZE+8];

		free(delim);	/* Don't need this any more */

		vd->lineno_pagest = (LONG *) malloc(sizeof(LONG) * (vd->nphys+2));
		if  (!vd->lineno_pagest)
			nomem();

		ps = vd->lineno_pagest;
		*ps++ = linecount;
		pc++;

		while  ((ch = getc(inf)) != EOF)  {
			fileoffset++;
			if  (fileoffset > *pc)  {
				pc++;
				pagecount++;
				pendpage = 1;
			}
			if  (ch & 0x80)  {
				inbuf[cp++] = 'M';
				inbuf[cp++] = '-';
				ch &= 0x7f;
				if  (ch == 0x7f)  {
					inbuf[cp++] = 'D';
					inbuf[cp++] = 'E';
					ch = 'L';
					colnum += 2;
				}
				else  if  (ch < ' ')  {
					inbuf[cp++] = '^';
					ch += ' ';
					colnum++;
				}
				inbuf[cp++] = ch;
				colnum += 3;
			}
			else  if  (ch < ' ')  {
				if  (ch == '\n')  {
					inbuf[cp++] = ch;
					linecount++;
					colnum = 0;
					if  (pendpage)  {
						*ps++ = linecount;
						pendpage = 0;
					}
				}
				else  if  (ch == '\r')
					continue;
				else  if  (ch == '\t')  {
					do  {
						inbuf[cp++] = ' ';
						colnum++;
					}  while  ((colnum & 7) != 0);
				}
				else  {
					inbuf[cp++] = '^';
					inbuf[cp++] = ch + ' ';
					colnum += 2;
				}
			}
			else  {
				if  (ch == 0x7f)  {
					inbuf[cp++] = 'D';
					inbuf[cp++] = 'E';
					ch = 'L';
					colnum += 2;
				}
				inbuf[cp++] = ch;
				colnum++;
			}
			if  (cp >= INBUFSIZE)  {
				buff_append(vd->fbuf, inbuf, cp);
				cp = 0;
			}
		}
		if  (cp > 0)  {
			if  (colnum != 0)  {
				inbuf[cp++] = '\n';
				linecount++;
			}
			buff_append(vd->fbuf, inbuf, cp);
			*ps = linecount;
		}
	}
	else  {

		LONG	pagecount = 0;
		int	linecount = 0, colnum = 0;
		unsigned  last_alloc = jp->spq_npages + 2;
		LONG	*ps;
		int	ch, cp = 0;
		char	inbuf[INBUFSIZE+8];

		vd->lineno_pagest = (LONG *) malloc(sizeof(LONG) * last_alloc);
		if  (!vd->lineno_pagest)
			nomem();

		ps = vd->lineno_pagest;
		*ps++ = linecount;

		while  ((ch = getc(inf)) != EOF)  {
			if  (ch & 0x80)  {
				inbuf[cp++] = 'M';
				inbuf[cp++] = '-';
				ch &= 0x7f;
				if  (ch == 0x7f)  {
					inbuf[cp++] = 'D';
					inbuf[cp++] = 'E';
					ch = 'L';
					colnum += 2;
				}
				else  if  (ch < ' ')  {
					inbuf[cp++] = '^';
					ch += ' ';
					colnum++;
				}
				inbuf[cp++] = ch;
				colnum += 3;
			}
			else  if  (ch < ' ')  {
				if  (ch == '\n')  {
					inbuf[cp++] = ch;
					linecount++;
					colnum = 0;
				}
				else  if  (ch == '\r')
					continue;
				else  if  (ch == '\f')  {
					if  (colnum != 0)  {
						inbuf[cp++] = '\n';
						colnum = 0;
						linecount++;
					}
					*ps++ = linecount;
					pagecount++;
					if  (pagecount >= last_alloc)  {
						unsigned  ptr = ps - vd->lineno_pagest;
						last_alloc += PAGEBUF_INC;
						vd->lineno_pagest = (LONG *) realloc((char *) vd->lineno_pagest, last_alloc * sizeof(LONG));
						if  (!vd->lineno_pagest)
							nomem();
						ps = &vd->lineno_pagest[ptr];
					}
				}
				else  if  (ch == '\t')  {
					do  {
						inbuf[cp++] = ' ';
						colnum++;
					}  while  ((colnum & 7) != 0);
				}
				else  {
					inbuf[cp++] = '^';
					inbuf[cp++] = ch + ' ';
					colnum += 2;
				}
			}
			else  {
				if  (ch == 0x7f)  {
					inbuf[cp++] = 'D';
					inbuf[cp++] = 'E';
					ch = 'L';
					colnum += 2;
				}
				inbuf[cp++] = ch;
				colnum++;
			}
			if  (cp >= INBUFSIZE)  {
				buff_append(vd->fbuf, inbuf, cp);
				cp = 0;
			}
		}
		if  (cp > 0)  {
			if  (colnum != 0)  {
				inbuf[cp++] = '\n';
				linecount++;
			}
			buff_append(vd->fbuf, inbuf, cp);
			*ps = linecount;
		}
		vd->nphys = pagecount;
	}
	vd->startp = jp->spq_start;
	vd->hatp = jp->spq_haltat;
	vd->endp = jp->spq_end;
}

static void	cb_vexitsave(GtkAction *action, struct view_data *vd)
{
	GtkWidget *dlg;

	if  (vd->changes > 0)  {

		/* Moan if page ranges make no sense */

		if  (vd->startp > vd->endp)  {
			doerror($EH{spq view start after end});
			return;
		}
		if  (vd->hatp > vd->endp)  {
			doerror($EH{spq view hat after end});
			return;
		}

		/* We need to reset this to the current state of the job in case
		   someone else got in there whilst we were viewing or we were viewing
		   more than one */

		JREQ = *vd->cj;
		JREQ.spq_start = vd->startp;
		JREQ.spq_haltat = vd->hatp;
		JREQ.spq_end = vd->endp;
		JREQS = vd->save_slot;
		my_wjmsg(SJ_CHNG);
	}
	dlg = vd->dlg;
	free_view_data(vd);
	gtk_dialog_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
}

static void	cb_vexitnosave(GtkAction *action, struct view_data *vd)
{
	GtkWidget *dlg = vd->dlg;
	free_view_data(vd);
	gtk_dialog_response(GTK_DIALOG(dlg), GTK_RESPONSE_CANCEL);
}

static void	set_pagenum(struct view_data *vd)
{
	ULONG	pnum = get_top_scrolled_page(vd);
	char	nbuf[10];
	sprintf(nbuf, "%ld", (long) pnum+1);
	gtk_label_set_text(GTK_LABEL(vd->fromp), nbuf);
	gtk_label_set_text(GTK_LABEL(vd->startpl), vd->startp == pnum? startpmsg: "");
	gtk_label_set_text(GTK_LABEL(vd->hatpl), vd->hatp == pnum && pnum != 0? hatpmsg: "");
	gtk_label_set_text(GTK_LABEL(vd->endpl), vd->endp == pnum? endpmsg: "");
}

void	cb_view(void)
{
	const  struct  spq  *cj = getselectedjob_chk(PV_VOTHERJ);
	FILE	*infile;
	char	*pr;
	GtkWidget  *hbox, *lab;
	GtkActionGroup *actions;
	GtkUIManager	*vui;
	GString	*labp;
	GError		*err;
	PangoFontDescription *font_desc;
	GtkTextIter	biter;
	struct	view_data	*vd;

	/* Give up now if no job */

	if  (!cj)
		return;

	/* Remember slot number in case we have another view going */

	vd = (struct view_data *) malloc(sizeof(struct view_data));
	if  (!vd)
		nomem();

	vd->save_slot = JREQS;
	vd->cj = cj;

	/* If it's a remote file .... */

	if  (cj->spq_netid)  {
		if  (!(infile = net_feed(FEED_NPSP, cj->spq_netid, cj->spq_rslot, cj->spq_job)))  {
			disp_arg[0] = cj->spq_job;
			disp_str = cj->spq_file;
			disp_str2 = look_host(cj->spq_netid);
			doerror($EH{Cannot obtain network job});
			return;
		}
	}
	else  if  (!(infile = fopen(mkspid(SPNAM, cj->spq_job), "r")))	{
		disp_arg[0] = cj->spq_job;
		doerror($EH{Cannot open local job});
		return;
	}

	vd->changes = 0;
	vd->changeable = (mypriv->spu_flgs & PV_OTHERJ)  ||  strcmp(Realuname, JREQ.spq_uname) == 0;

	vd->fbuf = gtk_text_buffer_new(NULL);
	fill_buff_spoolfile(infile, cj, vd);
	fclose(infile);

	/* Move cursor to start for benefit of searches */

	gtk_text_buffer_get_start_iter(vd->fbuf, &biter);
	gtk_text_buffer_place_cursor(vd->fbuf, &biter);

	if  (gtk_text_buffer_get_char_count(vd->fbuf) <= 0)  {
		doerror($EH{Null job in view});
		free_view_data(vd);
		g_object_unref(G_OBJECT(vd->fbuf));
		return;
	}

	if  (!startpmsg)  {
		startpmsg = gprompt($P{xmspq start page});
		endpmsg = gprompt($P{xmspq end page});
		hatpmsg = gprompt($P{xmspq hat page});
	}

	pr = gprompt($P{xspq view dlg});
	vd->dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
	free(pr);
	gtk_window_set_default_size(GTK_WINDOW(vd->dlg), DEFAULT_VIEW_WIDTH, DEFAULT_VIEW_HEIGHT);

	actions = gtk_action_group_new("VJActions");
	gtk_action_group_add_actions(actions, ventries, G_N_ELEMENTS(ventries), vd);
	vui = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(vui, actions, 0);
	g_object_unref(G_OBJECT(actions));
	gtk_window_add_accel_group(GTK_WINDOW(vd->dlg), gtk_ui_manager_get_accel_group(vui));
	pr = envprocess(XSPQVIEW_MENU);
	gtk_ui_manager_add_ui_from_file(vui, pr, &err);
	free(pr);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), gtk_ui_manager_get_widget(vui, "/ViewMenu"), FALSE, FALSE, 0);
	g_object_unref(G_OBJECT(vui));

	pr = gprompt($P{xspq view job});
	labp = g_string_new(NULL);
	if  (cj->spq_netid)
		g_string_printf(labp, "%s %s:%ld ", pr, look_host(cj->spq_netid), (long) cj->spq_job);
	else
		g_string_printf(labp, "%s %ld ", pr, (long) cj->spq_job);
	free(pr);

	if  (strlen(cj->spq_file) == 0)  {
		pr = gprompt($P{xspq view no title});
		g_string_append(labp, pr);
		free(pr);
	}
	else
		g_string_append(labp, cj->spq_file);

	lab = gtk_label_new(labp->str);
	g_string_free(labp, TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), lab, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), hbox, FALSE, FALSE, 0);

	lab = gprompt_label($P{xspq view user});
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);
	lab = gtk_label_new(cj->spq_uname);
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);
	lab = gprompt_label($P{xspq view page});
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);
	vd->fromp = gtk_label_new("1");
	gtk_box_pack_start(GTK_BOX(hbox), vd->fromp, FALSE, FALSE, 0);
	pr = gprompt($P{xspq view of});
	labp = g_string_new(NULL);
	g_string_printf(labp, " %s %ld", pr, (long) cj->spq_npages);
	free(pr);
	lab = gtk_label_new(labp->str);
	g_string_free(labp, TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), hbox, FALSE, FALSE, 0);

	vd->startpl = gtk_label_new(vd->startp == 0? startpmsg: "");
	gtk_box_pack_start(GTK_BOX(hbox), vd->startpl, FALSE, FALSE, 0);
	vd-> hatpl = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), vd->hatpl, FALSE, FALSE, 0);
	vd->endpl= gtk_label_new(vd->endp == 0? endpmsg: "");
	gtk_box_pack_start(GTK_BOX(hbox), vd->endpl, FALSE, FALSE, 0);

	vd->view = gtk_text_view_new_with_buffer(vd->fbuf);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(vd->view), FALSE);
	g_object_unref(G_OBJECT(vd->fbuf));
	if  ((font_desc = pango_font_description_from_string("fixed")))  {
		gtk_widget_modify_font(vd->view, font_desc);
		pango_font_description_free(font_desc);
	}
	vd->scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(vd->scroll), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vd->scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vd->scroll), vd->view);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), vd->scroll, TRUE, TRUE, 0);
	g_signal_connect_swapped(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(vd->scroll)), "value-changed", G_CALLBACK(set_pagenum), vd);
	gtk_widget_show_all(vd->dlg);
	g_signal_connect_swapped(vd->dlg, "response", G_CALLBACK(gtk_widget_destroy), vd->dlg);
}

/*
 ******************************************************************************
 *
 *			System error log file viewing
 *
 ******************************************************************************
 */

static void	fill_buff_syserr(GtkTextBuffer *fbuf, FILE *inf)
{
	int	ch, cp = 0, colnum = 0;
	char	inbuf[INBUFSIZE+8];

	cp = 0;

	while  ((ch = getc(inf)) != EOF)  {
		switch	(ch)  {
		default:
			if  (ch & 0x80)	 {
				inbuf[cp++] = 'M';
				inbuf[cp++] = '-';
				ch &= 0x7f;
				colnum += 2;
			}
			if  (ch < ' ')  {
				inbuf[cp++] = '^';
				ch += ' ';
				colnum++;
			}
			colnum++;
			inbuf[cp++] = ch;
			break;
		case  '\n':
			inbuf[cp++] = ch;
			colnum = 0;
		case  '\r':
			break;
		case  '\t':
			do  {
				inbuf[cp++] = ' ';
				colnum++;
			}  while  ((colnum & 7) != 0);
			break;
		}

		if  (cp >= INBUFSIZE)  {
			buff_append(fbuf, inbuf, cp);
			cp = 0;
		}
	}

	if  (cp > 0)
		buff_append(fbuf, inbuf, cp);
}

static void	cb_quitsel(GtkAction *action, GtkWidget *dlg)
{
	gtk_dialog_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
}

static void	clearlog(GtkAction *action, GtkWidget *dlg)
{
	if  (Confirm($PH{Clear syserr log}))
		close(open(REPFILE, O_TRUNC|O_WRONLY));
	gtk_dialog_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
}

void	cb_syserr(void)
{
	char	*pr;
	FILE	*infile;
	GtkTextBuffer  *fbuf;
	GtkTextMark	*mark;
	GtkActionGroup *actions;
	GtkUIManager	*seui;
	GtkWidget	*dlg, *view, *scroll;
	GError		*err;
	GtkTextIter	iter;
	PangoFontDescription *font_desc;

	if  (!(infile = fopen(REPFILE, "r")))  {
		doerror($EH{No log file yet});
		return;
	}

	fbuf = gtk_text_buffer_new(NULL);
	fill_buff_syserr(fbuf, infile);
	fclose(infile);

	/* If we didn't actually read anything, ditch it */

	if  (gtk_text_buffer_get_char_count(fbuf) <= 0)  {
		doerror($EH{No log file yet});
		g_object_unref(G_OBJECT(fbuf));
		return;
	}

	pr = gprompt($P{xspq syserr dlg});
	dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	free(pr);
	gtk_window_set_default_size(GTK_WINDOW(dlg), DEFAULT_SE_WIDTH, DEFAULT_SE_HEIGHT);

	actions = gtk_action_group_new("SEActions");
	gtk_action_group_add_actions(actions, slentries, G_N_ELEMENTS(slentries), dlg);
	seui = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(seui, actions, 0);
	g_object_unref(G_OBJECT(actions));
	gtk_window_add_accel_group(GTK_WINDOW(dlg), gtk_ui_manager_get_accel_group(seui));
	pr = envprocess(XSPQSEL_MENU);
	gtk_ui_manager_add_ui_from_file(seui, pr, &err);
	free(pr);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), gtk_ui_manager_get_widget(seui, "/SELMenu"), FALSE, FALSE, 0);
	g_object_unref(G_OBJECT(seui));
	view = gtk_text_view_new_with_buffer(fbuf);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	g_object_unref(G_OBJECT(fbuf));
	if  ((font_desc = pango_font_description_from_string("fixed")))  {
		gtk_widget_modify_font(view, font_desc);
		pango_font_description_free(font_desc);
	}
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), view);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, TRUE, TRUE, 0);

	gtk_text_buffer_get_end_iter(fbuf, &iter);
	mark = gtk_text_buffer_create_mark(fbuf, NULL, &iter, TRUE);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(view), mark, 0.0, FALSE, 0.0, 1.0);
	gtk_widget_show_all(dlg);
	g_signal_connect_swapped(dlg, "response", G_CALLBACK(gtk_widget_destroy), dlg);
}
