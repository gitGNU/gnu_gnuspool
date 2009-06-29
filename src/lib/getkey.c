/* getkey.c -- hand-crafted "inch" routines

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

/* This was implemented because back in the 1980s we didn't have
   reliable curses libraries with function key hanndling so we had to
   write our own to get all the required functionality.
   Also it could get confused by incoming signals from the scheduler.
   We could probably phase this out now. */

#include "config.h"

#include <curses.h>
#ifdef	NO_TERMIO_IN_CURSES
#if  	defined(OS_BSDI) || defined(OS_FREEBSD)
#include <termios.h>
#define	TCGETA	TIOCGETA
#define	TCSETA	TIOCSETA
#define	termio	termios
#else
#include <termio.h>
#endif
#endif /* NO_TERMIO_IN_CURSES */
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#if defined(HAVE_SYS_SELECT_H) && defined(BROKEN_TERM_READ)
#include <sys/select.h>
#endif
#include "defaults.h"
#include "incl_sig.h"
#include <errno.h>
#include "ecodes.h"
#include "keymap.h"
#include "magic_ch.h"
#include "incl_unix.h"
#include "sctrl.h"

extern	unsigned  char	Key_delay;

extern	void	doerror();

#define	INPFD		0

#ifdef	OS_DYNIX

/* Dynix requires this, but doesn't provide it in the headers.  Good eh!!!  */

#define	FIONREAD	_IOR('f', 127, int)	/* get # bytes to read */

static	catchit()
{
}

static	void	isleep(n)
int	n;
{
	unsigned  oa;
	int	(*oldsig)() = signal(SIGALRM, catchit);

	n /= 10;
	if  (n <= 0)
		n = 1;
	oa = alarm(n);
	pause();
	signal(SIGALRM, oldsig);
	alarm(oa);
}

static	int	see_pend()
{
	char	inbuf[2];
	LONG	nextch;
	int	lng;

	ioctl(INPFD, FIONREAD, &nextch);
	if  (nextch <= 0)  {
		isleep(Key_delay);
		ioctl(INPFD, FIONREAD, &nextch);
	}
	if  (nextch <= 0)
		return  EOF;
	lng = read(INPFD, inbuf, 1);
	if  (lng > 0)
		return  inbuf[0] & (MAPSIZE - 1);
	return  EOF;
}

#else	/*  !OS_DYNIX  */

static int	see_pend(void)
{
	char	inbuf[2];
	struct	termio	tstr;
	int	savemin, savetime, lng;

	ioctl(INPFD, TCGETA, &tstr);
	savemin = tstr.c_cc[VMIN];
	savetime = tstr.c_cc[VTIME];
	tstr.c_cc[VMIN] = 0;
	tstr.c_cc[VTIME] = Key_delay;
	ioctl(INPFD, TCSETA, &tstr);
	lng = read(INPFD, inbuf, 1);
	tstr.c_cc[VMIN] = (char) savemin;
	tstr.c_cc[VTIME] = (char) savetime;
	ioctl(INPFD, TCSETA, &tstr);
	if  (lng > 0)
		return  inbuf[0] & (MAPSIZE - 1);
	return  EOF;
}
#endif	/* !OS_DYNIX */

static int	read_buf(char * buf)
{
	int	lng;
#ifdef	BROKEN_TERM_READ
	fd_set	ready;
	FD_ZERO(&ready);
	FD_SET(INPFD, &ready);
	if  (select(INPFD+1, &ready, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0) < 0)
		return  0;
#endif
	if  ((lng = read(INPFD, buf, MAXTBUF)) < 0)  {
		if  (errno == EINTR)
			return  0;
		doerror(Ew, $E{getkey term input error});
		exit(E_INPUTIO);
	}
	return  lng;
}

static int	getmore(char * buf, int lng, struct keymap_sparse * ksp)
{
	int	ch;

	for  (;;)  {
		if  (lng <= 0)  {
			lng = read_buf(buf);
			if  (lng == 0)
				return EOF;
		}

		ch = buf[0] & (MAPSIZE - 1);
		for  (;  ksp;  ksp = ksp->ks_next)
			if  (ch == ksp->ks_char)
				goto  found;
		doerror(Ew, $E{Undef key seq});
		return  EOF;

	found:

		if  (!(ksp->ks_type & KV_SMAP))
			return  ksp->ks_value;

		if  (!(ksp->ks_type & KV_CHAR) || lng > 1)  {
			buf++;
			lng--;
			ksp = ksp->ks_link;
			continue;
		}

		if  ((ch = see_pend()) < 0)
			return  ksp->ks_value;
		*buf = (char) ch;
		ksp = ksp->ks_link;
	}
}

/* Get a key value using the current key maps.  The argument
   "magic_printing" determines whether we permit printing
   characters to be "magic", i.e. possibly interpreted as a
   "special" character.  */

static int	sgetkey(const unsigned magic_printing)
{
	int	lng, ch;
	struct	keymap_vec	*kv;
	char	inbuf[MAXTBUF*2];

	if  ((lng = read_buf(inbuf)) == 0)
		return  EOF;

	/* If character is printing and not "magic", return it.  */

	ch = inbuf[0] & (MAPSIZE-1);

	if  (isprint(ch))  {
		if  ((magic_printing & (MAG_A|MAG_P)) == 0)
			return  ch;
		if  ((magic_printing & MAG_A) == 0)  {
			if  (isalnum(ch))
				return  ch;
			if  (ch == '.' || ch == '-' || ch == '_')
				return  ch;
		}
	}

	kv = &curr_map[ch];
	if  (kv->kv_type == 0)
		return  ch;

	/* Ok - it's either a terminating char, the start of a sequence, or (yuk) both.  */

	if  (!(kv->kv_type & KV_SMAP))
		return  kv->kv_value;

	if  (!(kv->kv_type & KV_CHAR) || lng > 1)
		return  getmore(inbuf+1, lng-1, kv->kv_link);

	if  ((ch = see_pend()) < 0)
		return  kv->kv_value;
	inbuf[0] = (char) ch;
	return  getmore(inbuf, 1, kv->kv_link);
}

int	getkey(const unsigned magic_printing)
{
#ifdef	SIGTSTP
	int	ch;

	for  (;;)  {
		if  ((ch = sgetkey(magic_printing)) != $K{key stop})
			return  ch;
		kill(0, SIGTSTP);
	}
#else
	return  sgetkey(magic_printing);
#endif
}
