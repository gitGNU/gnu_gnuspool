/* sctrl.h -- parameters for wnum/wgets

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

#ifdef	OS_PYRAMID
/* Bug in Pyramid CC, const args makes whole thing const */
typedef	char	**(*helpfn_t)(char *, int);
#else
typedef	char	**(*helpfn_t)(const char *, const int);
#endif

#define	HELPLESS	((helpfn_t) 0)

struct	sctrl	{
	int		helpcode;	/*  Help code (see help file)  */
	helpfn_t	helpfn;		/*  Routine which will return block
					    of specific possibilities  */
	USHORT	size;			/*  size of field  */
	SHORT	retv;			/*  Return value for MAG_R  */
	SHORT	col;			/*  Position on screen -1 not there */
	unsigned  char	magic_p;	/*  Magic printing chars  */
	LONG	min, vmax;		/*  Min/max values  */
	char	*msg;			/*  Special description (if any) */
};

struct	attrib_key	{
	int		helpcode;	/* Help code */
	helpfn_t	helpfn;		/* Help routine */
	SHORT	col;			/* Col -1 not there */
	USHORT	size;			/* Size of thing */
	unsigned  char	magic_p;	/* Magic printing chars */
	LONG	min, vmax;		/* Min/max values */
	int	(*actfn)();		/* Function to do action */
};

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct	sq_formatdef  {
	SHORT	statecode;	/* Code number for heading if applicable */
	SHORT	sugg_width;	/* Suggested width */
	SHORT	keycode;	/* Key code (if applicable) for attributes */
	char	*msg;		/* Heading */
	char	*explain;	/* More detailed explanation */
	int	(*fmt_fn)();
};

extern void	wh_fill(WINDOW *, const int, const struct sctrl *, const classcode_t);
extern void	wn_fill(WINDOW *, const int, const struct sctrl *, const LONG);
extern void	ws_fill(WINDOW *, const int, const struct sctrl *, const char *);
extern void	whdrstr(WINDOW *, const char *);
extern void	mvwhdrstr(WINDOW *, const int, const int, const char *);

extern classcode_t	whexnum(WINDOW *, const int, struct sctrl *, const classcode_t);
extern LONG	wnum(WINDOW *, const int, struct sctrl *, const LONG);
extern LONG	chk_wnum(WINDOW *, const int, struct sctrl *, const LONG, const int);
extern char	*wgets(WINDOW *, const int, struct sctrl *, const char *);
extern char	*chk_wgets(WINDOW *, const int, struct sctrl *, const char *, const int);
