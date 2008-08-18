/* xspu_ext.h -- declarations for xspuser

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

extern	int	hchanges,	/* Had changes to default */
		uchanges;	/* Had changes to user(s) */

extern	unsigned	Nusers;
extern	struct	sphdr	Spuhdr;
extern	struct	spdet	*ulist;

/* X stuff */

extern	GtkWidget	*toplevel,	/* Main window */
			*dwid,		/* Default list */
			*uwid;		/* User scroll list */

extern	GtkListStore		*raw_ulist_store;
extern	GtkTreeModelSort	*ulist_store;

extern void	activate_action(GtkAction *);

extern void	cb_pri(GtkAction *);
extern void	cb_form(GtkAction *);
extern void	cb_ptr(GtkAction *);
extern void	cb_aform(GtkAction *);
extern void	cb_aptr(GtkAction *);
extern void	cb_class(GtkAction *);
extern void	cb_priv(GtkAction *);
extern void	cb_copyall(GtkAction *);
extern void	cb_copydef(GtkAction *);
extern void	cb_charges(GtkAction *);
extern void	cb_zerou(GtkAction *);
extern void	cb_zeroall(GtkAction *);
extern void	cb_impose(GtkAction *);

extern void	defdisplay(void);
extern void	doerror(const int);
#ifdef	NOTYET
extern void	displaybusy(const int);
#endif
extern void	update_all_users(void);
extern void	update_selected_users(void);
extern void	redispallu(void);


extern int	Confirm(const int);

/* Column numbers for user list */

#define	INDEX_COL	0
#define	UID_COL		1
#define	USNAM_COL	2
#define	DEFPRI_COL	3
#define	MINPRI_COL	4
#define	MAXPRI_COL	5
#define	COPIES_COL	6
#define	DEFFORM_COL	7
#define	DEFPTR_COL	8
#define	CLASS_COL	9
#define	PRIV_COL	10
