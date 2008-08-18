/* sd_bann.c -- ASCII banner pages for spd

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

#define	VOIDUSED	3
#include "config.h"
#include <stdio.h>
#include "incl_sig.h"
#include <setjmp.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef	HAVE_TERMIO_H
#include <termio.h>
#else
#include <sgtty.h>
#endif
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "errnums.h"
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "pages.h"
#include "initp.h"
#define	UCONST
#include "q_shm.h"
#include "incl_unix.h"
#include "incl_ugid.h"

#define	BANNWIDTH	16

extern	struct	initpkt	in_params;
extern	char	*bdsstr, *bdestr, *abortstr;
extern	PIDTYPE	childproc;

extern	struct	spptr		*Pptr;	/*  Current ptr descr  */

void	path_execute(char *, char *, const int);
int	execorsend(char *, char *, unsigned, const ULONG, const int);
void	exec_prep(const int, const int);
int	exec_wait(void);
void	filtopen(void);
void	fpush(const int);
void	nfreport(const int);
void	report(const int);
void	pagethrow(int);
void	pchar(const int);
void	pflush(void);
int	opendev(void);
int	filtclose(const int);
RETSIGTYPE	stopit(int);
void	holdorignore(const int);
void	seterrorstate(const char *);
extern	void	set_signal(const int, RETSIGTYPE (*)(int));

extern	LONG	Pages_done;

static	int	chrbase,	/* Offset of lowest char we understand */
		chrmax;		/* Offset of highest char */
static	unsigned  char	**chrtab;

/* Function to stuff chars into.

   This is either into the pipe for a filter, or it is pchar if we are
   talking directly to the output.

   Bug in (some) Pyramid ccs - const parameter makes pfunc itself const so you can't
   assign it */

#ifdef	OS_PYRAMID
static void	(*pfunc)(int);
#else
static void	(*pfunc)(const int);
#endif /* !PYRAMID */

extern	jmp_buf	stopj;

extern	char	*filternam,
		*destr,
		*rdelim,
		*bannprog,
		*shellname;

extern	struct	pages	pfe;
extern	int	pfile;
static	int	lcnt;

static	int	hexchar(const int ch)
{
	switch  (ch)  {
	default:
		return  0;
	case '0':case '1':case '2':case '3':case '4':
	case '5':case '6':case '7':case '8':case '9':
		return  ch - '0';
	case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
		return  ch - 'a' + 10;
	case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
		return  ch - 'A' + 10;
	}
}

/* Initialise vector 'chrtab' containing offsets of 16-byte vectors
   for each big letter.  The first letter we understand is
   "chrbase". (usually space) */

void	init_bigletter(void)
{
	int	pch;
	char	*cp;
	unsigned  char  *ip;
	char	*str;
	unsigned  char	*ltab;

	/* Cope with (unlikely) case of chars < space by searching backwards */

	for  (pch = ' ' - 1;  pch >= 0;  pch--)  {
		if  (!(str = helpprmpt(pch + $P{Bitmaps})))
			break;
		free(str);
	}

	/* Pch either -ve or 1 less than last */

	chrbase = pch + 1;
	chrmax = '~' + 1;
	if  (!(chrtab = (unsigned char **) malloc((unsigned)((chrmax - chrbase) * sizeof(unsigned char *)))))
		nomem();

	for  (pch = chrbase;  pch < 256;  pch++)  {
		if  (!(str = helpprmpt(pch + $P{Bitmaps})))
			break;

		/* Grow table if it goes beyond 126 */

		if  (pch >= chrmax)  {
			chrmax = 256;
			if  (!(chrtab = (unsigned char **) realloc((char *) chrtab, (unsigned)((chrmax - chrbase) * sizeof(unsigned char *)))))
				nomem();
		}

		/* Get table for letter and interpret */

		if  (!(ltab = (unsigned char *) malloc(BANNWIDTH)))
			nomem();

		for  (cp = str, ip = ltab;  *cp  &&  ip < &ltab[BANNWIDTH];  cp += 2)
			*ip++ = (hexchar(cp[0]) << 4) + hexchar(cp[1]);

		while  (ip < &ltab[BANNWIDTH])
			*ip++ = 0;

		free(str);
		chrtab[pch - chrbase] = ltab;
	}

	/* Clear excess */

	if  (chrmax > pch)  {
		chrmax = pch;
		if  (!(chrtab = (unsigned char **) realloc((char *) chrtab, (unsigned)((chrmax - chrbase) * sizeof(unsigned char *)))))
			nomem();
	}
}

/* Print character of banner.  */

void	banch(int x, const int row)
{
	unsigned  ent, bit;
	int  i;

	if  (x < chrbase  ||  x >= chrmax)
		x = chrbase;
	ent = chrtab[x - chrbase][row];
	for  (i = 7, bit = 0x80;  i >= 0;  i--, bit >>= 1)
		(*pfunc)(ent & bit? 'X': ' ');
}

/* Print word of banner.  */

void	banwd(const char *word)
{
	int  row, cnt;
	const	char  *cp;
	int	Pwidth = in_params.pi_width > 10? in_params.pi_width: 80;

	for  (row = 0;  row < 16;  row++)  {
		for  (cp = word, cnt = 8;  *cp && cnt < Pwidth;  cp++, cnt += 8)
			banch(*cp, row);
		lcnt++;
		(*pfunc)('\n');
	}
}

/* Generate newlines.  */

void	newline(int n)
{
	lcnt += n;
	while  (--n >= 0)
		(*pfunc)('\n');
}

/* Print time.  */

void	tprin(time_t tim)
{
	struct  tm  *tp = localtime(&tim);
	char	*cp;
	int	mon, mday;
	static	char	*weekdays;
	static	char	fmt[] = " on %.3s %.2d/%.2d/%.4d at %.2d:%.2d:%.2d\n";
	char	bff[sizeof(fmt) + 10];

	if  (!weekdays  &&  (weekdays = helpprmpt($P{Weekdays 3 chars})) == (char *) 0)
		weekdays = "SunMonTueWedThuFriSat";

	mon = tp->tm_mon+1;
	mday = tp->tm_mday;

	/* Keep those dyslexic pirates at SCH happy by swapping round
	   days and months if > 4 hours West */

#ifdef	HAVE_TM_ZONE
	if  (tp->tm_gmtoff <= -4 * 60 * 60)  {
#else
	if  (timezone >= 4 * 60 * 60)  {
#endif
		mday = mon;
		mon = tp->tm_mday;
	}

	sprintf(bff, fmt, &weekdays[tp->tm_wday*3],
		mday, mon, tp->tm_year + 1900, tp->tm_hour, tp->tm_min, tp->tm_sec);
	for  (cp = bff;  *cp;  cp++)
		(*pfunc)(*cp);
	lcnt++;
}

void	pfstr(const char *s)
{
	while  (*s)
		(*pfunc)(*s++);
}

/* Push message at output function.  */

void	pfpe(const int n)
{
	char	**emess = helpvec(n, 'E');
	char	**ep;

	for  (ep = emess;  *ep;  ep++)  {
		pfstr(*ep);
		(*pfunc)('\n');
		lcnt++;
		free(*ep);
	}
	free((char *) emess);
}

/* Ditto, first ensuring output function is set */

void	outerr(const int n)
{
	pfunc = pchar;
	pfpe(n);
}

/* Print the banner.  */

int	bannprin(struct spq *jp, void (*pfl)(const int))
{
	static	char	*submsg, *commsg;
	time_t	now;

	lcnt = 0;
	pfunc = pfl;

	if  (setjmp(stopj))  {
		if  (setjmp(stopj))  {
			/* Another one....
			   The guy must be desperate.  */
			filtclose(FC_NOERR|FC_ABORT|FC_KILL);
			return  0;
		}
		if  (filternam)
			filtclose(FC_NOERR|FC_ABORT);
		else  if  (abortstr)  {
			if  (!execorsend("ABORT", abortstr, in_params.pi_abort, in_params.pi_flags & PI_EX_ABORT, 0))
				goto  abt;
		}
		else  {
			newline(2);
			pfpe($E{aborted message});
			if  (!destr)
				pagethrow(lcnt);
		}
		execorsend("BANNDE", bdestr, in_params.pi_docend, in_params.pi_flags & PI_EX_BDOCEND, 0);
	abt:
		pflush();
		return  0;
	}

	set_signal(DAEMSTOP, stopit);

	newline(5);
	banwd(jp->spq_puname);

	if  (jp->spq_file[0] != '\0')  {
		newline(2);
		banwd(jp->spq_file);
	}
	newline(4);
	if  (strcmp(jp->spq_uname, jp->spq_puname) != 0)  {
		disp_str = jp->spq_uname;
		pfpe($E{Job submitted by});
	}

	disp_arg[0] = jp->spq_job;
	disp_arg[1] = jp->spq_size;
	disp_arg[2] = jp->spq_pri;
	if  (jp->spq_netid)  {
		disp_str = look_host(jp->spq_netid);
		pfpe($E{Remote job header message});
	}
	else
		pfpe($E{Job header message});
	if  (!submsg)  {
		submsg = gprompt($P{Job submitted message});
		commsg = gprompt($P{Job commenced message});
	}

	pfstr(submsg);
	tprin((time_t) jp->spq_time);
	pfstr(commsg);
	tprin(time(&now));
	holdorignore(DAEMSTOP);
	return  lcnt % pfe.delimnum;
}

/* Run banner program */

void	runbann(struct spq *jp)
{
	char	*cmdbuff;

	if  ((childproc = fork()) != 0)  {
		int	status;

		if  (childproc < 0)  {
			nfreport($E{Internal cannot fork});
			return;
		}
		if  ((status = exec_wait()) == 0)
			return;
		disp_str = bannprog;
		if  (status & 255)  {
			disp_arg[0] = status & 127;
			nfreport(status & 128? $E{Banner program core dumped}: $E{Banner program crashed});
		}
		else  {
			disp_arg[0] = status >> 8;
			nfreport($E{Banner program error halted});
		}
		return;
	}

	/* Child process - setpgrp etc */

	exec_prep(pfile, pfile);

	/* Don't forget lots of goodies are also passed in the environment.  */

	if  (!(cmdbuff = (char *) malloc((unsigned) (strlen(bannprog) + 3*13 + 2*UIDSIZE + 4 + 3))))
	       nomem();

	sprintf(cmdbuff,
		       "%s %ld %s %s %ld %ld %d \'\' 0",
		       bannprog,
		       (long) jp->spq_job,
		       jp->spq_uname,
		       jp->spq_puname,
		       (long) jp->spq_size,
		       (long) jp->spq_time,
		       jp->spq_pri);

	path_execute("EXBANN", cmdbuff, (in_params.pi_flags & PI_EXBANNPROG)? 1: 0);
	disp_str = bannprog;
	report($E{Cannot run banner program});
}

/* Worry about banners */

void	dobanner(struct spq *jp)
{
	if  (!(in_params.pi_flags & PI_FORCEHDR)  &&
	     (in_params.pi_flags & PI_NOHDR || jp->spq_jflags & SPQ_NOH))
		return;

	if  (pfile < 0  &&  !opendev())
		seterrorstate((const char *) 0);
	pflush();

	if  (in_params.pi_flags & PI_BANNBAUD)  {
#ifdef	HAVE_TERMIO_H
#ifdef	PRIME
		ioctl(pfile, oTCSETAW, &in_params.pi_banntty);
#else
		ioctl(pfile, TCSETAW, &in_params.pi_banntty);
#endif
#else
		ioctl(pfile, TIOCSETP, &in_params.pi_banntty);
#endif
	}

	if  (!execorsend("BANNDS", bdsstr, in_params.pi_bdocstart, in_params.pi_flags & PI_EX_BDOCST, 0))
		goto  bfin;

	/* Note that we don't bother with filters if we're already
	   messing about with banner programs.  */

	if  (bannprog)  {
		pflush();
		runbann(jp);
	}
	else  if  (filternam)  {
		filtopen();
		bannprin(jp, fpush);
		filtclose(FC_NOERR);
	}
	else  {
		int	lincnt = bannprin(jp, pchar);
		if  (!bdestr)
			pagethrow(lincnt);
	}

	execorsend("BANNDE", bdestr, in_params.pi_bdocend, in_params.pi_flags & PI_EX_BDOCEND, 0);

 bfin:
	pflush();
	Pages_done++;		/* Don't try to be more sophisticated */

	if  (in_params.pi_flags & PI_BANNBAUD)  {
#ifdef	HAVE_TERMIO_H
#ifdef	PRIME
		ioctl(pfile, oTCSETAW, &in_params.pi_tty);
#else
		ioctl(pfile, TCSETAW, &in_params.pi_tty);
#endif
#else
		ioctl(pfile, TIOCSETP, &in_params.pi_tty);
#endif
	}
}
