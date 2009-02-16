/* jscreen.h -- part of curses implementation on windows

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

class  screen	{
	friend	class  screen_state;
 protected:
	WINDOW	*win;			//  Currently implemented by curses bugalike
	unsigned  dirty : 1;	//  Needs refreshing
 public:
	screen()  { dirty = 1; }	//  Don't make window as we don't know how big
	~screen()  { if  (win  &&  win != stdscr)  delwin(win); }
	void	mangled()  {  dirty = 1;  }
	virtual  void  display() = 0;
	virtual  void  reposition() = 0;
	virtual  void  scr_refresh() = 0;
	virtual  void  update() = 0;
};

class	action_screen	:  public  screen  {
	friend  class  screen_state;
 protected:
	int	hline;			//  Line at top of screen (may be -ve)
	int	eline;			//  Line we are editing
	short	ebase;			//  Offset for error messages about cursor movement
	unsigned  rowsize : 3;		//  Height of lines in rows
	unsigned  moved : 1;		//  Cursor has been moved
	unsigned  more_above : 1;	//  "More above" line present
	unsigned  more_below : 1;	//  "More below" line present
	gprompt	 moreapr, morebpr;	//  Prompts for more above/below
	void	adjusteline(const unsigned);	//  Check "eline" is on screen
	void	scrollup(const int, const unsigned);
	void	scrolldown(const int, const unsigned);
 public:
	//  Hacked version for doing options with

	action_screen() : moreapr(0), morebpr(0) { win = stdscr; moved = 1;
						   hline = eline = ebase = more_above = more_below = 0;
						   rowsize = 1;
						   dirty = 1;
					         }
	action_screen(const int rows,
		      const short eb,
		      const short mapr,
		      const short mbpr,
		      const int startrow = 0,
		      int rowsz = 1) : ebase(eb), rowsize(rowsz), moreapr(mapr), morebpr(mbpr)
	{
		if  (!(win = newwin(rows, 0, startrow, 0)))
			nomem();
		moved = 1;
		hline = 0;
		eline = 0;
	}
	void  reposition() {
		if  (moved)  {
			win->wmove(int(more_above) + (eline - hline) * rowsize, 0);
			moved = 0;
		}
	}
	void  update() {
		if  (dirty)  {
			display();
			dirty = 0;
			moved = 1;
		}
	}
	void  display() {}			//  To allow for view screen
	void  scr_refresh();			//  Redraw screen
	void  top(const unsigned);		//  Basic cursor movement functions
	void  bottom(const unsigned);
	void  cursup(const unsigned);
	void  cursdown(const unsigned);
	void  fullscrollup(const unsigned);
	void  fullscrolldown(const unsigned);
	void  halfscrollup(const unsigned);
	void  halfscrolldown(const unsigned);
	int   currow()	{  return  eline;  }
	long	wnum(sctrl &const, const long, int = -1);
	char	*wgets(sctrl &const, char * const, int = -1);
	classcode_t  whexnum(sctrl &const, const classcode_t, int = -1);
	void	wn_fill(const sctrl &const, const long, const int = -1);
	void	ws_fill(const sctrl &const, const char *const, const int = -1);
	void	wh_fill(const sctrl &const, const classcode_t, const int = -1);
};

class  jscr  :  public  action_screen	{
	friend  class  screen_state;
 public:
	jscr(const int rows,
		const short eb,
		const short mapr,
		const short mbpr,
		const int startrow = 0,
		int rowsz = 1) : action_screen(rows, eb, mapr, mbpr, startrow, rowsz) {}
	void  size_fill(spq *); 		//  Refill size field
	void	display();
	void	cjfind();
	int	checkokj();
	friend	void	notejob(void);
};

class  pscr :  public  action_screen  {
	friend  class  screen_state;
 public:
	pscr(const int rows,
		const short eb,
		const short mapr,
		const short mbpr,
		const int startrow = 0,
		int rowsz = 1) : action_screen(rows, eb, mapr, mbpr, startrow, rowsz) {}
	int	state_fill(int);
	void	display();
	int	chngcls(spptr &);
	int	chnglmt(const int, unsigned long &, char *);
};

class  viewscr : public  action_screen	{
	friend  class  screen_state;
 public:
	void	display();
};

class  header_screen  :  public screen  {
	friend  class  screen_state;
 private:
	const  char	keyletter;		//  Letter in file
	char		**rowvec;		//  Vector of rows (may be null)
 public:
	header_screen(const char);
	~header_screen();
	int	lines();			// Give number of lines by looking at rowvec
	void	init(const int);		// Initialise after deciding position
	void	display();			// Fill screen
	void	reposition()	{}		// Null reposition function (not actually used)
	void	scr_refresh();			// Refresh
	void	update()
	{
		if  (dirty)  {
			display();
			dirty = 0;
		}
	}
};

#define	MAXLAYOUT	5			// Max number to lay out

class  screen_state  {
 private:
	short	keystate;			// Current state for key maps
	short	pushed_state;			// Previous key state
	screen	*current;			// Current screen selected
	screen	*pushed_screen;			// Previous screen
	screen	*layout[MAXLAYOUT];		// Stack of screens supported
	WINDOW	*helpwin, *ewin;		// Help/error windows
 public:
	screen_state();
	short	currstate() const  {  return  keystate;	}
	void	addlayout(screen * const);
	void	push_state();
	void	pop_state();
	void	select_state(screen * const, const short = -1);
	void	select_state(const short);
	void	reset_state();
	void	dochelp(const short = -1);	// Generate help for current state
	void	dohelp(const sctrl &const, const char *const);	// Specific help
	void	doerror(const short);
	void	endhe();
	void	mangled();
	int	getconfirm(const short, const short);
	int	gobblechar();
	int	getkhelp(const unsigned);
	void	screen_check();
};

extern	header_screen	*hjscr, *hpscr, *tpscr;
extern	jscr		*Job_screen;
extern	pscr		*Ptr_screen;
extern	screen_state	Screen_state;
