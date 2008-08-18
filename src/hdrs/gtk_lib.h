/* gtk_lib.h -- declarations of GTK "library" functions

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

extern void	doerror(int);
extern int	Confirm(int);
extern void	gtk_chk_uid(void);

/* Functions to make "gprompt"-stuff easier */

extern GtkWidget *gprompt_label(const int);
extern GtkWidget *gprompt_checkbutton(const int);
extern GtkWidget *gprompt_radiobutton(const int);
extern GtkWidget *gprompt_radiobutton_fromwidget(GtkWidget *, const int);
