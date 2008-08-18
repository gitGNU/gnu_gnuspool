/* gtk_lib.c -- library of routines common to GTK programs

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
#include <gtk/gtk.h>
#include "defaults.h"
#include "ecodes.h"
#include "errnums.h"
#include "incl_unix.h"
#include "gtk_lib.h"
#include "incl_ugid.h"

extern	GtkWidget	*toplevel;

static  char *makebigvec(char **mat)
{
	unsigned  totlen = 0, len;
	char	**ep, *newstr, *pos;

	for  (ep = mat;  *ep;  ep++)
		totlen += strlen(*ep) + 1;

	newstr = malloc((unsigned) totlen);
	if  (!newstr)
		nomem();
	pos = newstr;
	for  (ep = mat;  *ep;  ep++)  {
		len = strlen(*ep);
		strcpy(pos, *ep);
		free(*ep);
		pos += len;
		*pos++ = '\n';
	}
	pos[-1] = '\0';
	free((char *) mat);
	return  newstr;
}

void	doerror(int errnum)
{
	char	**evec = helpvec(errnum, 'E'), *newstr;
	GtkWidget  *msgw;
	if  (!evec[0])  {
		disp_arg[0] = errnum;
		free((char *) evec);
		evec = helpvec($E{Missing error code}, 'E');
	}
	newstr = makebigvec(evec);
	msgw = gtk_message_dialog_new(GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", newstr);
	free(newstr);
	evec = helpvec(errnum, 'H');
	newstr = makebigvec(evec);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgw), "%s", newstr);
	free(newstr);
	g_signal_connect_swapped(msgw, "response", G_CALLBACK(gtk_widget_destroy), msgw);
	gtk_widget_show(msgw);
	gtk_dialog_run(GTK_DIALOG(msgw));
}

int	Confirm(int code)
{
	int	ret;
	char	*msg = gprompt(code), *secmsg = makebigvec(helpvec(code, 'H'));
	GtkWidget  *dlg = gtk_message_dialog_new(GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", msg);
	free(msg);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), "%s", secmsg);
	free(secmsg);
	ret = gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_YES;
	gtk_widget_destroy(dlg);
	return  ret;
}

void	gtk_chk_uid(void)
{
	char	*hd = getenv("HOME");
	struct	stat	sbuf;

	if  (!hd  ||  stat(hd, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR  ||  sbuf.st_uid != Realuid)  {
		print_error($E{Check file setup});
		exit(E_SETUP);
	}
}

GtkWidget *gprompt_label(const int code)
{
	char	*pr = gprompt(code);
	GtkWidget  *lab = gtk_label_new(pr);
	free(pr);
	return  lab;
}

GtkWidget *gprompt_checkbutton(const int code)
{
	char	*pr = gprompt(code);
	GtkWidget  *butt = gtk_check_button_new_with_label(pr);
	free(pr);
	return  butt;
}

GtkWidget *gprompt_radiobutton(const int code)
{
	char	*pr = gprompt(code);
	GtkWidget  *butt = gtk_radio_button_new_with_label(NULL, pr);
	free(pr);
	return  butt;
}

GtkWidget  *gprompt_radiobutton_fromwidget(GtkWidget *w, const int code)
{
	char	*pr = gprompt(code);
	GtkWidget  *butt = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w), pr);
	free(pr);
	return  butt;
}
