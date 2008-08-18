/* xsq_ext.h -- declarations for xspq

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

extern	char	scrkeep,	/* Try to keep moved job  */
		confabort;	/* 0 no confirm 1 confirm unprinted (default) 2 always */

extern	char	*Realuname,	/* My user name */
		*ptdir,		/* Printers directory, typically /usr/spool/printers */
		*spdir,		/* Spool directory, typically /usr/spool/spd */
		*Curr_pwd;	/* Directory on entry */

extern	struct	spdet  *mypriv;	/* My class code/privileges */

extern	struct	spr_req	jreq,	/* Job requests */
			preq,	/* Printer requests */
			oreq;	/* Other requests */
extern	struct	spq	JREQ;
extern	struct	spptr	PREQ;

#define	JREQS	jreq.spr_un.j.spr_jslot
#define	OREQ	oreq.spr_un.o.spr_jpslot
#define	PREQS	preq.spr_un.p.spr_pslot

#ifndef	USING_FLOCK
extern	int	Sem_chan;
#endif

/* X stuff */

extern	GtkWidget	*toplevel,	/* Main window */
			*jwid,		/* Job scroll list */
			*pwid;		/* Printer scroll list */

extern	GtkListStore	*jlist_store,
			*unsorted_plist_store;

extern	GtkTreeModelSort	*sorted_plist_store;

enum	jprend_t  { JPREND_TEXT, JPREND_PROGRESS, JPREND_TOGGLE  };

struct	jplist_elems  {
	GType	type;			/* Type of element for settling up list store */
	enum	jprend_t     rendtype;	/* Renderer type for treeview */
	int	colnum;			/* Column number in treeview */
	int	msgcode;		/* Message prompt number */
	int	sortid;			/* Sort id where applicable */
	char	*msgtext;		/* Message text */
	char	*descr;			/* Full description for menu */
	GtkWidget  *menitem;		/* Menu item */
};

#define	DEF_DLG_HPAD	5
#define	DEF_DLG_VPAD	5
#define	DEF_BUTTON_PAD	3

/* This is the column in the list stores we use to remember the sequence number */

#define	SEQ_COL		0

extern void	womsg(const int);
extern void	my_wjmsg(const int);
extern void	my_wpmsg(const int);

#define	MAXMACS		10

struct	macromenitem  {
	char	*cmd;
	char	*descr;
	unsigned  mergeid;
};

extern int	add_macro_to_list(const char *, const char, struct macromenitem *);
