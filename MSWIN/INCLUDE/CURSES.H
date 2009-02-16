/* curses.h -- client program interface

   Copyright 2009 Free Software Foundation, Inc.

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

#define	CHT_FLAGS_NONE		0
#define	CHT_FLAGS_TOUCHED	1
#define	CHT_FLAGS_SO		2
#define CHT_FLAGS_ERR		4

class	chtype	{
  private:
	unsigned  char	flags;		//  Flags as above
	unsigned  char	chx;		//  Value
  public:
	chtype(const char x = ' ', const unsigned char f = 0) : chx(x), flags(f) { }
	int  touched() { return  flags & CHT_FLAGS_TOUCHED; }
	void	untouched() { flags &= ~CHT_FLAGS_TOUCHED; }
	int  so() { return flags & CHT_FLAGS_SO; }
	int  err() { return flags & CHT_FLAGS_ERR; }
	int  val() { return  chx;  }
	unsigned char enh() const { return flags & ~CHT_FLAGS_TOUCHED; }
	int operator == (chtype b) { return  chx == b.chx && (flags & ~CHT_FLAGS_TOUCHED) == (b.flags & ~CHT_FLAGS_TOUCHED); }
	int operator != (chtype b) { return  chx != b.chx || (flags & ~CHT_FLAGS_TOUCHED) != (b.flags & ~CHT_FLAGS_TOUCHED); }
	void	set(const chtype b)
	{	chx = b.chx;	flags = b.flags | CHT_FLAGS_TOUCHED;	}
};

extern	short	LINES, COLS;

class	WINDOW	{
 private:
	short	currow, curcol;
	short	rows, cols;
	short	startrow, startcol;
	unsigned  char	currflags;
	unsigned  long	changes;
	chtype	**chars;
 public:
	short  begx() const { return  startcol; }
	short  begy() const { return  startrow; }
	short  getx() const { return  curcol; }
	short  gety() const { return  currow; }
	short  nrows() const { return  rows; }
	short  ncols() const { return  cols; }
	void   wmove(const short r, const short c)
	{	if  (r < 0)  currow = 0;
		else  if  (r >= rows) currow = rows - 1;
		else  currow = r;
		if  (c < 0)  curcol = 0;
		else  if  (c >= cols) curcol = cols - 1;
		else  curcol = c;
	}
	void  checkoverlaps();
	void  touchwin() {  changes++; }
	void  waddch(const chtype);
	void  mvwaddch(const short r, const short c, const chtype x)
	{	wmove(r, c);	waddch(x);	}
	void  waddch(const char x) { waddch(chtype(x, currflags)); }
	void  mvwaddch(const short r, const short c, const char x)
	{	wmove(r, c);	waddch(x);	}
	void  waddstr(const char *);
	void  mvwaddstr(const short r, const short c, const char *x)
	{	wmove(r, c);	waddstr(x);	}
	void  whdrstr(const char *);
	void  mvwhdrstr(const short r, const short c, const char *x)
	{	wmove(r, c);	whdrstr(x);	}
	void  wprintw(const char *, ...);
	void  mvwprintw(const short, const short, const char *, ...);
	void  wclear();
	void  werase();
	void  wrefresh();
	void  wnoutrefresh();
	void  flash();
	void  wstandout() { currflags |= CHT_FLAGS_SO; }
	void  wstandend() { currflags &= ~CHT_FLAGS_SO; }
	void  errcolour() { currflags |= CHT_FLAGS_ERR; }
	void  normcolour() { currflags &= ~CHT_FLAGS_ERR; }
	void  wclrtoeol();
	void  wclrtobot();
	void  box(chtype = chtype(0), chtype = chtype(0));
	void  dbox(chtype = chtype(0), chtype = chtype(0));
	WINDOW(short = LINES, short = COLS, short = 0, short = 0);
	~WINDOW();
	friend	void	doupdate(void);
};

extern	WINDOW	*curscr, *stdscr;

//	Hideous defines but they do the job....

#define	printw		stdscr->wprintw
#define	mvprintw	stdscr->mvwprintw

inline	void	clear(void) {	stdscr->wclear(); }
inline	void	move(const short r, const short c)	{  stdscr->wmove(r, c);	}
inline	void	addch(const chtype ch)	{  stdscr->waddch(ch);	}
inline	void	mvaddch(const short r, const short c, const chtype ch)	{  stdscr->mvwaddch(r, c, ch);	}
inline	void	addstr(const char *str)	{  stdscr->waddstr(str);	}
inline	void	mvaddstr(const short r, const short c, const char *str)	{  stdscr->mvwaddstr(r, c, str);  }
inline	void	hdrstr(const char *str)		{  stdscr->whdrstr(str); }
inline	void	mvhdrstr(const short r, const short c,  const char *str)	{  stdscr->mvwhdrstr(r, c, str);  }
inline	void	erase(void)	{  stdscr->werase();	}
inline	void	refresh(void)	{  stdscr->wrefresh();	}
inline	void	nooutrefresh(void)	{  stdscr->wnoutrefresh();	}
inline	void	standout(void)	{  stdscr->wstandout();	}
inline	void	standend(void)	{  stdscr->wstandend();	}
inline	void	clrtoeol(void)	{  stdscr->wclrtoeol();	}
inline	void	clrtobot(void)	{  stdscr->wclrtobot();	}

// Can replace these by new/delete and invoke constructor/destructor

#define	newwin	new WINDOW
#define	delwin	delete

extern	void	initscr(void);
extern	void	endwin(void);
extern	void	doupdate(void);
