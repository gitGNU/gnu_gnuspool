/* sq_view.c -- view jobs within spq

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
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <curses.h>
#include <ctype.h>
#include "errnums.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "pages.h"
#include "keynames.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "incl_unix.h"
#include "incl_ugid.h"

void	dochelp(WINDOW *, const int);
void	doerror(WINDOW *, const int);
void	endhe(WINDOW *, WINDOW **);

int	rdpgfile(const struct spq *, struct pages *, char **, unsigned *, LONG **);
FILE	*net_feed(const int, const netid_t, const slotno_t, const jobno_t);

extern	char	helpclr;
extern	char	*Realuname;
extern	struct	spdet	*mypriv;

extern	WINDOW	*escr, *hlpscr, *Ew;

#define	FIRSTROW	2
#define	INITPAGES	20	/* Initial size to allocate vector for */
#define	INCPAGES	10	/* Incremental size page offsets vector */
#define	MINIT		12
#define	MINC		6

static	struct	match	{
	int	pagen, row, col;
}  *mlist;
static	int	mcount, mmax, lastmstrl;
static	char	*lastmstr;

static	char	manglable;

static	unsigned	nphys;

static	LONG	*pagestarts,
		*physpages,
		fileend;

static	int	npages,
		pagewidth,
		numpages,
		Colstep;

extern	struct	spr_req	jreq;
extern	struct	spq	JREQ;

static	char	notformfeed;

static	int	lpost,
		fpage;

static	char	*jnmsg,
		*rjnmsg,
		*thrmsg,
		*eofmsg,
		*eopmsg,
		*startmsg,
		*haltmsg,
		*endmsg,
		*fsmsg,
		*bsmsg,
		*selogf;

/* Read file to find where all the pages start.  */

int	scanfile(FILE * fp)
{
	int	curline, curpage, lcnt, ppage;
	int	ch;

	if  ((pagestarts = (LONG *) malloc(INITPAGES*sizeof(LONG))) == (LONG *) 0)
		return  -1;

	npages = INITPAGES;
	ppage = 1;
	pagewidth = 0;
	curline = 0;
	curpage = 0;
	pagestarts[0] = 0L;
	physpages[0] = 0L;
	lcnt = FIRSTROW;

	while  ((ch = getc(fp)) != EOF)  {
		switch  (ch)  {
		default:
			curline++;
			break;
		case  '\f':
			if  (ppage >= nphys)  {
				nphys += INCPAGES;
				physpages = (LONG *) realloc((char *) physpages,
							     (unsigned) (nphys + 2) * sizeof(LONG));
				if  (physpages == (LONG *) 0)
					return  -1;
			}
			physpages[ppage] = ftell(fp);
			ppage++;

		case  '\n':
			if  (curline > pagewidth)
				pagewidth = curline;
			curline = 0;
			if  (++lcnt < LINES  &&  ch != '\f')
				break;
			lcnt = FIRSTROW;
			if  (++curpage >= npages)  {
				npages += INCPAGES;
				pagestarts = (LONG *) realloc((char *)pagestarts, npages*sizeof(LONG));
				if  (pagestarts == (LONG *) 0)
					return  -1;
			}
			pagestarts[curpage] = ftell(fp);
			if  (curline > pagewidth)
				pagewidth = curline;
			curline = 0;
			break;
		case  '\t':
			curline = (curline + 8) & ~7;
			break;
		}
	}
	numpages = curpage;
	fileend = ftell(fp);
	if  (fileend > physpages[ppage])
		physpages[++ppage] = fileend;
	return  1;
}

static	int	readpgfile(FILE *fp, const struct spq *jp)
{
	int	ch, curline, curpage, lcnt, ppage;
	LONG	char_count;
	char	*delim;
	struct	pages	pfe;

	nphys = 0;

	if  ((ch = rdpgfile(jp, &pfe, &delim, &nphys, &physpages)) <= 0)
		return  ch;

	/* Don't need this...  */

	free(delim);

	if  ((pagestarts = (LONG *) malloc(INITPAGES*sizeof(LONG))) == (LONG *) 0)
		return  -1;

	npages = INITPAGES;
	ppage = 1;
	pagewidth = 0;
	curline = 0;
	curpage = 0;
	pagestarts[0] = 0L;
	physpages[0] = 0L;
	lcnt = FIRSTROW;
	char_count = 0L;

	for  (;;)  {
		if  ((ch = getc(fp)) == EOF)  {
			fileend = char_count;
			numpages = curpage;
			if  (fileend <= physpages[ppage-1])
				numpages--;
			return  1;
		}
		if  (++char_count >= physpages[ppage])  {
			ppage++;
			lcnt = LINES;
		}
		else  if  (ch == '\t')  {
			curline = (curline + 8) & ~7;
			continue;
		}
		else  if  (ch != '\n')  {
			curline++;
			continue;
		}
		if  (curline > pagewidth)
			pagewidth = curline;
		curline = 0;
		if  (++lcnt < LINES)
			continue;
		lcnt = FIRSTROW;
		if  (++curpage >= npages)  {
			npages += INCPAGES;
			pagestarts = (LONG *) realloc((char *)pagestarts, npages*sizeof(LONG));
			if  (pagestarts == (LONG *) 0)
				return  -1;
		}
		pagestarts[curpage] = char_count;
	}
}

static	void	centralise(const int row, char *msg, const int param)
{
	move(row, (COLS - (int) strlen(msg)) / 2);
	standout();
	printw(msg, param);
	standend();
}

static	void	clrpost(void)
{
	move(0, COLS-lpost);
	clrtoeol();
}

static	void	post(char *msg)
{
	move(0, COLS - strlen(msg));
	standout();
	addstr(msg);
	standend();
}

static int	inmatches(const int page, const int row, const int col)
{
	int	first = 0, last = mcount, middle;
	struct	match	*mp;

	while  (first < last)  {
		middle = (first + last) / 2;
		mp = &mlist[middle];
		if  (page != mp->pagen)  {
			if  (page < mp->pagen)
				last = middle;
			else
				first = middle + 1;
			continue;
		}
		if  (row != mp->row)  {
			if  (row < mp->row)
				last = middle;
			else
				first = middle + 1;
			continue;
		}
		if  (col < mp->col)  {
			last = middle;
			continue;
		}
		if  (col >= mp->col + lastmstrl)  {
			first = middle + 1;
			continue;
		}
		return  1;
	}
	return  0;
}

/* Display next screenful.  */

static	void	displayscr(FILE *fp, const int lhcol, const int pnum)
{
	int	row, col, wcol = 0, physp, drow, hilite = 0;
	int	ch;
	LONG	char_count, epagecnt;

	row = FIRSTROW;
	col = 0;
	drow = 0;
	move(row, col);
	clrtobot();

	fseek(fp, (long) (char_count = pagestarts[pnum]), 0);

	/* Where are we in the physical page */

	for  (physp = 0;  physpages[physp] < pagestarts[pnum];  physp++)
		;
	if  (physpages[physp] > pagestarts[pnum])
		physp--;

	clrpost();
	if  (JREQ.spq_haltat != 0  &&  physp == JREQ.spq_haltat)
		post(haltmsg);
	else  if  (physp == JREQ.spq_start)
		post(startmsg);
	else  if  (physp == JREQ.spq_end)
		post(endmsg);

	fpage = physp;
	epagecnt = physpages[physp+1];

	/* If last page came exactly on the boundary...  */

	if  (notformfeed  &&  char_count == pagestarts[pnum+1])  {
		if  (hilite)  {
			hilite = 0;
#ifdef	HAVE_TERMINFO
			attroff(A_REVERSE);
#else
			standend();
#endif
		}
		centralise(row, char_count >= fileend? eofmsg: eopmsg, physp);
		return;
	}

	for  (;;)  {
		if  (notformfeed  &&  char_count >= epagecnt)  {
			if  (hilite)  {
				hilite = 0;
#ifdef	HAVE_TERMINFO
				attroff(A_REVERSE);
#else
				standend();
#endif
			}
			if  (col > 0)  {
				col = 0;
				drow = 0;
				if  (++row >= LINES)
					return;
			}
			if  (char_count >= fileend)
				centralise(row, eofmsg, physp);
			else
				centralise(row, eopmsg, ++physp);
			return;
		}

		ch = getc(fp);
		char_count++;

		switch  (ch)  {
		case  EOF:
			if  (hilite)  {
				hilite = 0;
#ifdef	HAVE_TERMINFO
				attroff(A_REVERSE);
#else
				standend();
#endif
			}
			centralise(row, eofmsg, 0);
			return;

		case  '\n':
			col = 0;
			drow = 0;
			row++;
			if  (hilite)  {
				hilite = 0;
#ifdef	HAVE_TERMINFO
				attroff(A_REVERSE);
#else
				standend();
#endif
			}
			break;

		case  '\f':
			if  (notformfeed)
				goto  control_char;
			if  (hilite)  {
				hilite = 0;
#ifdef	HAVE_TERMINFO
				attroff(A_REVERSE);
#else
				standend();
#endif
			}
			if  (col > 0)  {
				col = 0;
				drow = 0;
				if  (++row >= LINES)  {
					ungetc('\f', fp);
					break;
				}
			}
			centralise(row, eopmsg, ++physp);
			return;

		case  '\t':
			col = (col + 8) & ~7;
			break;

		default:
			wcol = col - lhcol;
			col++;
			if  (wcol < 0)
				break;
			if  (wcol >= COLS)  {
				if  (hilite)  {
					hilite = 0;
#ifdef	HAVE_TERMINFO
					attroff(A_REVERSE);
#else
					standend();
#endif
				}
				if  (!drow)  {
					mvaddch(row, COLS-1, '>');
					drow++;
				}
				break;
			}

#ifdef	CURSES_SCROLL_BUG
			/* Yuk!  Supress funnies with bottom rh corner.  */

			if  (wcol == COLS-1 && row == LINES-1)
				break;
#endif
			if  (mlist)  {
				if  (inmatches(pnum, row, col))  {
					if  (!hilite)  {
						hilite = 1;
#ifdef	HAVE_TERMINFO
						attron(A_REVERSE);
#else
						standout();
#endif
					}
				}
				else  if  (hilite)  {
					hilite = 0;
#ifdef	HAVE_TERMINFO
					attroff(A_REVERSE);
#else
					standend();
#endif
				}
			}

			if  (isprint(ch))
#ifdef	HAVE_TERMINFO
				mvaddch(row, wcol, (chtype) ch);
#else
				mvaddch(row, wcol, ch);
#endif
			else  if  (iscntrl(ch))  {
			control_char:
				ch = ch + '@';
				move(row, wcol);
#ifdef	HAVE_TERMINFO
				attron(A_UNDERLINE);
				addch((chtype) ch);
				attroff(A_UNDERLINE);
#else
				if  (!hilite)
					standout();
				addch(ch);
				if  (!hilite)
					standend();
#endif
			}
			else
				mvaddch(row, wcol, '?');
			break;
		}
		if  (row >= LINES)
			break;
	}
}

static	void	domarg(const int lhcol)
{
	int	col = 0, k;
	int	cy;

	move(FIRSTROW-1, 0);

	do  {
		if  (COLS - col < 5)  {
			while  (col < COLS)  {
				addch('.');
				col++;
			}
			return;
		}
		k = lhcol+col+1;
		do  {
			addch('.');
			col++;
			k++;
		}  while  ((k % 5) != 0);
		printw("%d", k);
		getyx(stdscr, cy, col);
	}  while  (cy < FIRSTROW);
}

static	int	chmatch(int sch, int fch)
{
	if  (fch < ' '  ||  fch > '~')
		return  0;
	if  (sch == fch)
		return  1;
	if  (sch >= 'a' && sch <= 'z')
		sch -= 'a' - 'A';
	if  (fch >= 'a' && fch <= 'z')
		fch -= 'a' - 'A';
	if  (sch == fch)
		return  1;
	return  0;
}

int	findmatches(FILE *fp, const char *str)
{
	int	col, row, page, physp;
	LONG	fplace, char_count;
	int	ch;
	const	char	*cp;
	struct	match	*mp;

	if  (mlist)  {
		free(mlist);
		mlist = (struct match *) 0;
		mmax = 0;
		mcount = 0;
	}
	rewind(fp);

	col = 0;
	row = FIRSTROW;
	page = 0;
	char_count = 0;
	physp = 1;

 next:
	for  (;;)  {
		ch = getc(fp);
		char_count++;
		if  (notformfeed && char_count >= physpages[physp])  {
			col = 0;
			row = FIRSTROW;
			physp++;
			page++;
			continue;
		}
		switch  (ch)  {
		case  EOF:
			return  mcount;

		case  '\f':
			if  (!notformfeed)  {
				col = 0;
				row = FIRSTROW;
				page++;
				physp++;
				continue;
			}
		default:
			col++;
			break;

		case  '\n':
			col = 0;
			if  (++row >= LINES)  {
				row = FIRSTROW;
				page++;
			}
			continue;

		case  '\t':
			col = (col + 8) & ~7;
			continue;
		}

		/* See if it matches first char of string.  */

		if  (!chmatch(*str, ch))
			continue;

		/* Keep our finger on the place */

		fplace = ftell(fp);
		cp = str;
		while  (*++cp)  {
			ch = getc(fp);
			if  (*cp != '.'  &&  !chmatch(*cp, ch))  {
				fseek(fp, (long) fplace, 0);
				goto  next;
			}
		}

		/* Found one....  */

		if  (mcount >= mmax)  {
			if  (mmax <= 0)  {
				mmax = MINIT;
				mlist = (struct match *) malloc((MINIT + 1) * sizeof(struct match));
			}
			else  {
				mmax += MINC;
				mlist = (struct match *) realloc((char *) mlist,
								 (unsigned)(mmax + 1) * sizeof(struct match));
			}
			if  (mlist == (struct match *) 0)
				return  -1;
		}
		mp = &mlist[mcount];
		mcount++;

		mp->pagen = page;
		mp->row = row;
		mp->col = col;
		fseek(fp, (long) fplace, 0);
	}
}

static	int	gstring(char *msg, FILE *fp)
{
	struct	sctrl	ss;
	char	*gstr;

	ss.helpcode = $H{spq view ask search};
	ss.helpfn = (char **(*)()) 0;
	ss.size = 20;
	ss.retv = 0;
	ss.col = strlen(msg);
	ss.magic_p = MAG_OK;
	ss.min = 0L;
	ss.vmax = 0L;
	ss.msg = (char *) 0;
	mvaddstr(LINES-1, 0, msg);
	clrtoeol();

	if  (lastmstr)  {
		ws_fill(stdscr, LINES-1, &ss, lastmstr);
		gstr = wgets(stdscr, LINES-1, &ss, lastmstr);
		if  (!gstr)
			return  -1;
		if  (gstr[0] == '\0')
			return  mcount;
	}
	else  {
		for  (;;)  {
			gstr = wgets(stdscr, LINES-1, &ss, "");
			if  (!gstr)
				return  -1;
			if  (gstr[0])
				break;
			doerror(stdscr, $E{spq view ask search});
		}
	}
	lastmstr = stracpy(gstr);
	lastmstrl = strlen(lastmstr);
	return  findmatches(fp, lastmstr);
}

/* Get a random error or say not there */

static	char **gerr(const int num)
{
	char	**result = helpvec(num, 'E');

	if  (*result == (char *) 0)  {
		free((char *) result);
		disp_arg[0] = num;
		result = helpvec($E{Missing error code}, 'E');
	}
	return  result;
}

static	void	initmsgs(void)
{
	int	i;
	jnmsg = gprompt($P{Job view number form});
	thrmsg = gprompt($P{Job view percent thru});
	eofmsg = gprompt($P{Job view eof});
	eopmsg = gprompt($P{Job view eop});
	startmsg = gprompt($P{Job view start mkr});
	endmsg = gprompt($P{Job view end mkr});
	haltmsg = gprompt($P{Job view haltat mkr});
	fsmsg = gprompt($P{Job view fsearch});
	bsmsg = gprompt($P{Job view rsearch});
	rjnmsg = gprompt($P{Remote job view number});
	selogf = gprompt($P{spq view syslog});
	lpost = strlen(startmsg);
	if  ((i = strlen(endmsg)) > lpost)
		lpost = i;
	if  ((i = strlen(haltmsg)) > lpost)
		lpost = i;
	Colstep = (COLS / (3 * 8)) * 8;
}

static	void	freeall(void)
{
	if  (pagestarts)  {
		free((char *) pagestarts);
		pagestarts = (LONG *) 0;
	}
	if  (physpages)  {
		free((char *) physpages);
		physpages = (LONG *) 0;
	}
	if  (mlist)  {
		free((char *) mlist);
		mlist = (struct match *) 0;
	}
	if  (lastmstr)  {
		free(lastmstr);
		lastmstr = (char *) 0;
	}
	mcount = 0;
	mmax = 0;
}

/* Process file view from error file display or interrogate */

static  int  do_view(FILE *fp, const int state, const int help_no, const int err_no, int pnum)
{
	int	i, kch, lhcol = 0, changes = 0;

	domarg(lhcol);
	displayscr(fp, lhcol, pnum);
#ifdef	CURSES_OVERLAP_BUG
	touchwin(stdscr);
#endif
	select_state(state);

	for  (;;)  {
		move(LINES-1, COLS-1);
		refresh();

	nextin:
		do  kch = getkey(MAG_A|MAG_P);
		while  (kch == EOF  &&  (hlpscr || escr));

		if  (hlpscr)  {
			endhe(stdscr, &hlpscr);
			if  (helpclr)
				goto  nextin;
		}
		if  (escr)
			endhe(stdscr, &escr);

		switch  (kch)  {
		default:
		badcmd:
			doerror(stdscr, err_no);

		case  EOF:
			continue;

		case  $K{key help}:
			disp_str = JREQ.spq_file;
			disp_arg[0] = JREQ.spq_job;
			dochelp(stdscr, help_no);
			continue;

		case  $K{key halt}:
			fclose(fp);
			freeall();
			return  changes;

		case  $K{key guess}:
		case  $K{key eol}:
		case  $K{key cursor down}:
			if  (pnum >= numpages)  {
				doerror(stdscr, $E{job view at eof});
				continue;
			}
			pnum++;
			displayscr(fp, lhcol, pnum);
			continue;

		case  $K{key cursor up}:
			if  (pnum <= 0)  {
				doerror(stdscr, $E{job view at bof});
				continue;
			}
			pnum--;
			displayscr(fp, lhcol, pnum);
			continue;

		case  $K{spq key scroll file left}:
			if  (lhcol <= 0)
				doerror(stdscr, $E{job view at lhs});
			else  {
				lhcol -= Colstep;
				domarg(lhcol);
				displayscr(fp, lhcol, pnum);
			}
			continue;

		case  $K{spq key scroll file right}:
			if  (lhcol + Colstep >= pagewidth)
				doerror(stdscr, $E{job view at rhs});
			else  {
				lhcol += Colstep;
				domarg(lhcol);
				displayscr(fp, lhcol, pnum);
			}
			continue;

		case  $K{key top}:
			if  (pnum <= 0)
				continue;
			pnum = 0;
			displayscr(fp, lhcol, 0);
			continue;

		case  $K{key bottom}:
			if  (pnum >= numpages)
				continue;
			pnum = numpages;
			displayscr(fp, lhcol, pnum);
			continue;

		case  $K{spq key left marg}:
			if  (lhcol > 0)  {
				lhcol = 0;
				domarg(0);
				displayscr(fp, 0, pnum);
			}
			continue;

		case  $K{spq key right marg}:
			if  (lhcol + Colstep < pagewidth)  {
				lhcol = (pagewidth / Colstep) * Colstep;
				if  ((pagewidth % Colstep) == 0)  {
					lhcol -= Colstep;
					if  (lhcol < 0)
						lhcol = 0;
				}
				domarg(lhcol);
				displayscr(fp, lhcol, pnum);
			}
			continue;

		case  $K{spq key start page}:
			if  (!manglable)  {
			nomangle:
				doerror(stdscr, $E{spq job not yours});
				continue;
			}
			if  (state != $S{spq view job})
				goto  badcmd;
			JREQ.spq_start = fpage;
			clrpost();
			post(startmsg);
			changes++;
			if  (JREQ.spq_start > JREQ.spq_end)
				doerror(stdscr, $E{spq view start after end});
			continue;

		case  $K{spq key end page}:
			if  (state != $S{spq view job})
				goto  badcmd;
			if  (!manglable)
				goto  nomangle;
			JREQ.spq_end = fpage;
			clrpost();
			post(endmsg);
			changes++;
			continue;

		case  $K{spq key halted at page}:
			if  (state != $S{spq view job})
				goto  badcmd;
			if  (!manglable)
				goto  nomangle;
			if  (JREQ.spq_haltat == 0)  {
				doerror(stdscr, $E{spq view bad hat});
				continue;
			}
			changes++;
			JREQ.spq_haltat = fpage;
			clrpost();
			post(haltmsg);
			if  (JREQ.spq_haltat > JREQ.spq_end)
				doerror(stdscr, $E{spq view hat after end});
			continue;

		case  $K{key forward search}:
			if  (gstring(fsmsg, fp) < 0)
				goto  forgetit;
			if  (mcount <= 0)  {
				doerror(stdscr, $E{spq view ss not found});
				goto  forgetit;
			}
			for  (i = 0;  i < mcount;  i++)
				if  (mlist[i].pagen > pnum)
					goto  ff;
			doerror(stdscr, $E{spq view fs not found});
			goto  forgetit;
		ff:
			pnum = mlist[i].pagen;
			while  (mlist[i].col < lhcol)
				lhcol -= Colstep;
			while  (mlist[i].col > lhcol + COLS)
				lhcol += Colstep;
			domarg(lhcol);
		forgetit:
			displayscr(fp, lhcol, pnum);
			continue;

		case  $K{key backward search}:
			if  (gstring(bsmsg, fp) < 0)
				goto  forgetit;
			if  (mcount <= 0)  {
				doerror(stdscr, $E{spq view ss not found});
				goto  forgetit;
			}
			for  (i = mcount-1;  i >= 0;  i--)
				if  (mlist[i].pagen < pnum)
					goto  ff;
			doerror(stdscr, $E{spq view bs not found});
			goto  forgetit;

		case  $K{key refresh}:
			wrefresh(curscr);
			continue;
		}
	}
}

int	viewfile(void)
{
	FILE	*fp = (FILE *) 0;
	char	**ev = (char **) 0;
	LONG	place = 0;
	int	ret;

	manglable = mypriv->spu_flgs & PV_OTHERJ || strcmp(Realuname, JREQ.spq_uname) == 0;

	Ew = stdscr;

	if  (JREQ.spq_dflags & SPQ_PQ)
		place = (JREQ.spq_posn * 100) / JREQ.spq_size;

	if  (!jnmsg)
		initmsgs();

	/* If it's a remote file slurp it up into a temporary file */

	if  (JREQ.spq_netid)  {
		FILE	*ifl;
		int	kch;

		if  (!(ifl = net_feed(FEED_NPSP, JREQ.spq_netid, JREQ.spq_rslot, JREQ.spq_job)))  {
			disp_arg[0] = JREQ.spq_job;
			disp_str = JREQ.spq_file;
			disp_str2 = look_host(JREQ.spq_netid);
			ev = gerr($E{spq view remote job access});
		}
		else  {
			fp = tmpfile();
			while  ((kch = getc(ifl)) != EOF)
				putc(kch, fp);
			fclose(ifl);
			rewind(fp);
		}
	}
	else  if  (!(fp = fopen(mkspid(SPNAM, JREQ.spq_job), "r")))
		ev = gerr($E{spq view job error});

 doev:
	if  (ev)  {
		int	rows, cols, row, col, i;

		count_hv(ev, &rows, &cols);
		if  (rows <= 0)
			return  0;
		clear();
		standout();
		col = (COLS - cols) / 2;
		for  (i = 0, row = (LINES - rows)/2; i < rows;  i++, row++)
			mvprintw(row, col, "%s", ev[i]);
		freehelp(ev);
		standend();
		refresh();
		reset_state();
	nextin2:
		do  i = getkey(MAG_A|MAG_P);
		while  (i == EOF && (hlpscr || escr));
		if  (hlpscr)  {
			endhe(stdscr, &hlpscr);
			if  (helpclr)
				goto  nextin2;
		}
		if  (escr)
			endhe(stdscr, &escr);
		return  0;
	}

	/* Otherwise it worked....
	   See what kind of pagination we're using.  */

	if  ((ret = readpgfile(fp, &JREQ)))
		notformfeed = 1;
	else  {
		ret = scanfile(fp);
		notformfeed = 0;
	}
	if  (ret < 0)  {
		disp_arg[0] = JREQ.spq_job;
		disp_str = JREQ.spq_file;
		disp_arg[1] = JREQ.spq_size;
		ev = gerr($E{spq page break memory error});
		if  (!ev)			/*  Can't allocate .... give up */
			nomem();
		freeall();
		goto  doev;
	}
	clear();
	standout();
	if  (JREQ.spq_netid)
		printw(rjnmsg, look_host(JREQ.spq_netid), JREQ.spq_job, JREQ.spq_file, JREQ.spq_form);
	else
		printw(jnmsg, JREQ.spq_job, JREQ.spq_file, JREQ.spq_form);
	standend();

	if  (place > 0)
		printw(thrmsg, place);

#ifdef	CURSES_MEGA_BUG
	ret =  do_view(fp, $H{spq view job}, $H{spq view job}, $E{spq view job unknownc}, 0);
	clear();
	refresh();
	return  ret;
#else
	return  do_view(fp, $H{spq view job}, $H{spq view job}, $E{spq view job unknownc}, 0);
#endif
}

/* View system error file, or return 0 if there isn't one (yet) */

int	view_errors(const int injobs)
{
	FILE	*fp;

	if  (!(fp = fopen(REPFILE, "r")))
		return  0;

	if  (!jnmsg)
		initmsgs();

	nphys = INITPAGES;
	if  (!(physpages = (LONG *) malloc(INITPAGES * sizeof(LONG))))
		return  -1;

	if  (scanfile(fp) < 0)  {
		free((char *) physpages);
		return  -1;
	}
	notformfeed = 0;

	clear();
	standout();
	addstr(selogf);
	standend();

	do_view(fp,
		       $S{spq view syslog}, $H{spq view syslog},
		       injobs? $E{spq j log file unkc}:
		       $E{spq p log file unkc},
		       numpages);
#ifdef	CURSES_MEGA_BUG
	clear();
	refresh();
#endif
	return  1;
}
