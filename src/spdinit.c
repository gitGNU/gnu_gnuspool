/* spdinit.c -- parse printer setup files

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
#ifdef	HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <sys/types.h>
#ifdef	HAVE_TERMIO_H
#include <termio.h>
#else
#include <sgtty.h>
#endif
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <errno.h>
#include "incl_sig.h"
#include "errnums.h"
#include "kw.h"
#include "defaults.h"
#include "initp.h"
#include "files.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "cfile.h"
#include "network.h"
#include "spq.h"
#include "xfershm.h"
#include "q_shm.h"

#ifdef	HAVE_TERMIO_H
#ifndef	CINTR
#define	CINTR	0177
#endif
#ifndef	CQUIT
#define	CQUIT	034
#endif
#ifndef	CERASE
#define	CERASE	'#'
#endif
#ifndef	CKILL
#define	CKILL	'@'
#endif
#define CONTCHARS CINTR, CQUIT, CERASE, CKILL, 0, 0
#endif

void  checkfor(const int, char *);
void  error(const int);
extern  void  readdescr();
void  sethash(char *, void (*)(const int, const unsigned), const unsigned);
void  setsymb(char *, const int);

extern  int  rdnum();

struct string	*rdstr(const int);

char	*ptdir = "",
	*Suffix,
	*mbeg,
	*mend,
	*formname,
	*curr_file = "",
	*default_form;

FILE	*infile;

struct	initpkt	out_params = {
	0,				/*  Flags  */
	0,				/*  Flags 2  */
	DEF_CHARGE,			/*  Charge rate  */
	0,				/*  Windback pages */
	DEF_OBUF,			/*  Output buffer size  */
	DEF_OPEN,			/*  Default open timeout */
	DEF_CLOSE,			/*  Default close timeout */
	SIGINT,				/*  Default interrupt signal*/
	0,				/*  Post-close wait */
	DEF_OFFLINE,			/*  Default offline timeout */
	DEF_WIDTH,			/*  Width  */
	0, 0,				/*  Setup/halt string length  */
	0, 0,				/*  Docstart/Docend string length  */
	0, 0,				/*  Bann docstart/end string length  */
	0, 0,				/*  Sufstart/Sufend string length  */
	0, 0,				/*  Pagestart/Pageend string length */
	0, 0,				/*  Abort/restart string lengths */
	0,				/*  Align file length  */
	0,				/*  Filter length  */
	0,				/*  Record length  */
	0,				/*  Record offset  */
	0,				/*  Log file  */
	0,				/*  Record count string  */
	0,				/*  Port setup string */
	0,				/*  Network filter */
	0,				/*  Banner program */
	0,				/*  Stty string */
	(ULONG) ((1 << SIGHUP) | (1 << SIGPIPE)),	/*  Offline signals */
	(ULONG) (~((1 << SIGHUP)|(1 << SIGPIPE))),	/*  Error signals */
	{0L,0L,0L,0L, 0L,0L,0L,0L},	/*  Offline exits */
       {(ULONG)0xfffffffe,(ULONG)0xffffffff,(ULONG)0xffffffff,(ULONG)0xffffffff,
	(ULONG)0xffffffff,(ULONG)0xffffffff,(ULONG)0xffffffff,(ULONG)0xffffffff}, /* Error exits */
	0,				/*  Count of record strings  */
#ifdef	HAVE_TERMIO_H
	{	IGNBRK|ISTRIP,
		0,
#ifdef OS_DYNIX
		B9600|CS7|CREAD|HUPCL|PARENB,
		ICANON,
		0,
#else
		B9600|CS8|CREAD|HUPCL,
		0,
		0,
#endif
		{ CONTCHARS }
	}
#else
	{	B9600, B9600,
		CERASE, CKILL,
		0
	}
#endif
	,
#ifdef	HAVE_TERMIO_H
	{	IGNBRK|ISTRIP,
		0,
#ifdef OS_DYNIX
		B9600|CS7|CREAD|HUPCL|PARENB,
		ICANON,
		0,
#else
		B9600|CS8|CREAD|HUPCL,
		0,
		0,
#endif
		{ CONTCHARS }
	}
#else
	{	B9600, B9600,
		CERASE, CKILL,
		0
	}
#endif
};

struct	string	*su_str = (struct string *) 0,
		*hlt_str = (struct string *) 0,
		*ds_str = (struct string *) 0,
		*de_str = (struct string *) 0,
		*bds_str = (struct string *) 0,
		*bde_str = (struct string *) 0,
		*ss_str = (struct string *) 0,
		*se_str = (struct string *) 0,
		*ps_str = (struct string *) 0,
		*pe_str = (struct string *) 0,
		*rc_str = (struct string *) 0,
		*ab_str = (struct string *) 0,
		*res_str = (struct string *) 0,
		*out_bannprog = (struct string *) 0,
		*out_align = (struct string *) 0,
		*out_portsu = (struct string *) 0,
		*out_filter = (struct string *) 0,
		*out_netfilt = (struct string *) 0,
		*out_sttystring = (struct string *) 0;

struct	string	**out_str_last = &su_str;

int	execf	=  PI_EX_SETUP, Nott, Hadbann, Isbann;

struct	string	*out_record,
		*out_logfile;

static	char	*pname = "";
static	int	setuperrs = 0;

extern	uid_t	Realuid, Effuid, Daemuid;

/* Open report file if possible write message to it.  */

void  report(const int msgno)
{
	int	fid;
	time_t	tim;
	struct  tm	*tp;
	FILE	*rpfile;
	int	saverrno = errno;
	int	mon, mday;
	char	*dir;

	setuperrs++;

	dir = envprocess(SPDIR);
	Ignored_error = chdir(dir);
	free(dir);

	if  ((fid = open(REPFILE, O_WRONLY|O_APPEND|O_CREAT, 0666)) < 0)
		return;
	if  ((rpfile = fdopen(fid, "a")) == (FILE *) 0)  {
		close(fid);
		return;
	}

	time(&tim);
	tp = localtime(&tim);
	mon = tp->tm_mon + 1;
	mday = tp->tm_mday;

	/* Keep those dyslexic pirates across the pond  happy by swapping round
	   days and months if > 4 hours West */

#ifdef	HAVE_TM_ZONE
	if  (tp->tm_gmtoff <= -4 * 60 * 60)  {
#else
	if  (timezone >= 4 * 60 * 60)  {
#endif
		mday = mon;
		mon = tp->tm_mday;
	}

	fprintf(rpfile, "%s: %s/%s/%s: %.2d:%.2d:%.2d %.2d/%.2d\n==============\n",
		       progname,
		       ptdir, pname, curr_file,
		       tp->tm_hour, tp->tm_min, tp->tm_sec, mday, mon);
	errno = saverrno;
	fprint_error(rpfile, msgno);
	fflush(rpfile);
}

void  nomem()
{
	report($E{NO MEMORY});
	exit(E_NOMEM);
}

/* Procedures to do specific actions.  */

void  do_wid(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (obey)
		out_params.pi_width = (USHORT) n;
}

void  do_hdropt(const int obey, const unsigned arg)
{
	if  (obey)  {
		out_params.pi_flags &= ~(PI_NOHDR|PI_FORCEHDR);
		if  (!Nott)	/* Turn off everything if guy is used to -nohdr */
			out_params.pi_flags |= arg;
	}
}

void  do_flag(const int obey, const unsigned arg)
{
	if  (obey)  {
		if  (Nott)
			out_params.pi_flags &= ~arg;
		else
			out_params.pi_flags |= arg;
	}
}

void  do_flag2(const int obey, const unsigned arg)
{
	if  (obey)  {
		if  (Nott)
			out_params.pi_flags2 &= ~arg;
		else
			out_params.pi_flags2 |= arg;
	}
}

void  do_charge(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (obey && n > 0)
		out_params.pi_charge = n;
}

void  do_windback(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (obey && n > 0)
		out_params.pi_windback = n;
}

void  do_obuf(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (obey && n > 0)
		out_params.pi_obuf = n;
}

void  do_offline(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (obey)
		out_params.pi_offa = (USHORT) n;
}

void  do_open(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (obey)
		out_params.pi_oa = (USHORT) n;
}

void  do_close(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (obey)
		out_params.pi_ca = (USHORT) n;
}

void  do_postcl(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (obey)
		out_params.pi_postclsl = (USHORT) n;
}

void  do_clsig(const int obey, const unsigned dummy)
{
	int	n = rdnum();

	if  (n <= 0  || n > 31)  {
		disp_arg[1] = n;
		disp_arg[2] = 1;
		disp_arg[3] = 31;
		error($E{spdinit number range});
	}
	if  (obey)
		out_params.pi_clsig = (USHORT) n;
}

void  do_record(const int obey, const unsigned dummy)
{
	struct	string	*r;
	int	n = rdnum();

	if  (obey)
		out_params.pi_offset = (USHORT) n;

	checkfor(':', (char *) 0);
	r = rdstr(ST_SPTERM);
	if  (obey)  {
		out_record = r;
		r->s_cnt++;
	}
	else  {
		if  (r->s_length)
			free(r->s_str);
		free((char *) r);
	}
}

void  do_logfile(const int obey, const unsigned dummy)
{
	struct	string	*r;

	checkfor('=', (char *) 0);
	r = rdstr(ST_NOESC);
	if  (obey)  {
		out_logfile = r;
		r->s_cnt++;
	}
	else  {
		if  (r->s_length)
			free(r->s_str);
		free((char *) r);
	}
}

#ifdef	HAVE_TERMIO_H
void  do_baud(const int obey, const unsigned dummy)
#else
void  do_baud(const int obey, const unsigned arg)
#endif
{
	int	i = rdnum();

	switch	(i)	{
	default:
		disp_arg[9] = i;
		error($E{spdinit bad baud});

	case  50:	i = B50;  break;
	case  75:	i = B75;  break;
	case  110:	i = B110; break;
	case  134:	i = B134; break;
	case  150:	i = B150; break;
	case  200:	i = B200; break;
	case  300:	i = B300; break;
	case  600:	i = B600; break;
	case  1200:	i = B1200;break;
	case  1800:	i = B1800;break;
	case  2400:	i = B2400;break;
	case  4800:	i = B4800;break;
	case  9600:	i = B9600;break;
#ifdef	B19200
	case  19200:	i = B19200;break;
#else
	case  19200:	i = EXTA;break;		/*  Kludge  */
#endif
#ifdef	B38400
	case  38400:	i = B38400;break;
#else
	case  38400:	i = EXTB;break;		/*  Kludge  */
#endif
	}

	if  (!obey)				/*  Check correct anyway! */
		return;
#ifdef	HAVE_TERMIO_H
	if  (Isbann)  {
		out_params.pi_flags |= PI_BANNBAUD;
		out_params.pi_banntty.c_cflag &= ~CBAUD;
		out_params.pi_banntty.c_cflag |= i;
	}
	else  {
		out_params.pi_tty.c_cflag &= ~CBAUD;
		out_params.pi_tty.c_cflag |= i;
	}
#else
	/* Split speed printers????  Heaven help us, let alone this thing.  */

	if  (Isbann)  {
		out_params.pi_flags |= PI_BANNBAUD;
		if  (arg != 2)
			out_params.pi_banntty.sg_ispeed = i;
		if  (arg != 1)
			out_params.pi_banntty.sg_ospeed = i;
	}
	else  {
		if  (arg != 2)
			out_params.pi_tty.sg_ispeed = i;
		if  (arg != 1)
			out_params.pi_tty.sg_ospeed = i;
	}
#endif
}

#ifdef	HAVE_TERMIO_H
void  do_csize(const int obey, const unsigned arg)
{
	if  (obey)  {
#ifdef OS_DYNIX
		switch (arg) {
			case CS7:
				if  (Isbann)  {
					out_params.pi_flags |= PI_BANNBAUD;
					out_params.pi_banntty.c_lflag |= ICANON;
				}
				else
					out_params.pi_tty.c_lflag |= ICANON;
				break;
			case CS8:
				if  (Isbann)  {
					out_params.pi_flags |= PI_BANNBAUD;
					out_params.pi_banntty.c_lflag &= ~ICANON;
				}
				else
					out_params.pi_tty.c_lflag &= ~ICANON;
				break;
		}
#else
		if  (Isbann)  {
			out_params.pi_flags |= PI_BANNBAUD;
			out_params.pi_banntty.c_cflag &= ~CSIZE;
			out_params.pi_banntty.c_cflag |= arg;
		}
		else  {
			out_params.pi_tty.c_cflag &= ~CSIZE;
			out_params.pi_tty.c_cflag |= arg;
		}
#endif
	}
}

void  do_parity(const int obey, const unsigned arg)
{
	if  (obey)  {
		if  (Isbann)  {
			out_params.pi_flags |= PI_BANNBAUD;
			out_params.pi_banntty.c_cflag &= ~(PARENB|PARODD);
			if  (!Nott)
				out_params.pi_banntty.c_cflag |= arg;
		}
		else  {
			out_params.pi_tty.c_cflag &= ~(PARENB|PARODD);
			if  (!Nott)
				out_params.pi_tty.c_cflag |= arg;
		}
	}
}

void  do_cflags(const int obey, const unsigned arg)
{
	if  (obey)  {
		if  (Isbann)  {
			out_params.pi_flags |= PI_BANNBAUD;
			if  (Nott)
				out_params.pi_banntty.c_cflag &= ~arg;
			else
				out_params.pi_banntty.c_cflag |= arg;
		}
		else  {
			if  (Nott)
				out_params.pi_tty.c_cflag &= ~arg;
			else
				out_params.pi_tty.c_cflag |= arg;
		}
	}
}

void  do_oflag(const int obey, const unsigned arg)
{
	if  (obey)  {
		if  (Isbann)  {
			out_params.pi_flags |= PI_BANNBAUD;
			if  (Nott)
				out_params.pi_banntty.c_oflag &= ~arg;
			else
				out_params.pi_banntty.c_oflag |= arg;
		}
		else  {
			if  (Nott)
				out_params.pi_tty.c_oflag &= ~arg;
			else
				out_params.pi_tty.c_oflag |= arg;
		}
	}
}

void  do_iflag(const int obey, const unsigned arg)
{
	if  (obey)  {
		if  (Isbann)  {
			out_params.pi_flags |= PI_BANNBAUD;
			if  (Nott)
				out_params.pi_banntty.c_iflag &= ~arg;
			else
				out_params.pi_banntty.c_iflag |= arg;
		}
		else  {
			if  (Nott)
				out_params.pi_tty.c_iflag &= ~arg;
			else
				out_params.pi_tty.c_iflag |= arg;
		}
	}
}

#ifdef OS_DYNIX
void	do_lflag(obey, arg)
const	int	obey;
const	unsigned	arg;
{
	if  (obey)  {
		if  (Isbann)  {
			out_params.pi_flags |= PI_BANNBAUD;
			if  (Nott)
				out_params.pi_banntty.c_lflag &= ~arg;
			else
				out_params.pi_banntty.c_lflag |= arg;
		}
		else  {
			if  (Nott)
				out_params.pi_tty.c_lflag &= ~arg;
			else
				out_params.pi_tty.c_lflag |= arg;
		}
	}
}

#endif	/* ifdef DYNIX */

#else
void  do_flags(const int obey, const unsigned arg)
{
	if  (obey)  {
		if  (Isbann)  {
			out_params.pi_flags |= PI_BANNBAUD;
			if  (Nott)
				out_params.pi_banntty.sg_flags &= ~arg;
			else
				out_params.pi_banntty.sg_flags |= arg;
		}
		else  {
			if  (Nott)
				out_params.pi_tty.sg_flags &= ~arg;
			else
				out_params.pi_tty.sg_flags |= arg;
		}
	}
}

#endif

void  do_mat(const int obey, const unsigned dummy)
{
	if  (obey && mbeg)  {
		char  *stuff = (char *) malloc((unsigned)(mend-mbeg+1));
		struct  string  *rr = (struct string *) malloc(sizeof(struct string));
		if  (stuff == (char *) 0 || rr == (struct string *) 0)
			nomem();
		rr->s_length = mend - mbeg;
		strncpy(stuff, mbeg, (int) rr->s_length);
		stuff[rr->s_length] = '\0';
		rr->s_str = stuff;
		rr->s_cnt = 1;
		rr->s_next = (struct string *) 0;
		*out_str_last = rr;
		out_str_last = &rr->s_next;
	}
}

void  initsymbs()
{
	sethash("_", do_mat, 0);

	sethash("width", do_wid, DEF_WIDTH);
	sethash("nohdr", do_hdropt, PI_NOHDR);
	sethash("forcehdr", do_hdropt, PI_FORCEHDR);
	sethash("stdhdr", do_hdropt, 0);
	sethash("hdrpercopy", do_flag, PI_HDRPERCOPY);
	sethash("retain", do_flag, PI_RETAIN);
	sethash("reopen", do_flag, PI_REOPEN);
	sethash("single", do_flag, PI_SINGLE);
	sethash("canhang", do_flag, PI_CANHANG);
	sethash("logerror", do_flag, PI_LOGERROR);
	sethash("fberror", do_flag, PI_FBERROR);
	sethash("noranges", do_flag, PI_NORANGES);
	sethash("onecopy", do_flag, PI_NOCOPIES);
	sethash("inclpage1", do_flag, PI_INCLP1);
	sethash("addcr", do_flag2, PI_ADDCR);
	sethash("record", do_record, 0);
	sethash("logfile", do_logfile, 0);

	sethash("charge", do_charge, 0);
	sethash("windback", do_windback, 0);
	sethash("open", do_open, 0);
	sethash("close", do_close, 0);
	sethash("closekill", do_clsig, 0);
	sethash("postclose", do_postcl, 0);
	sethash("offline", do_offline, 0);
	sethash("outbuffer", do_obuf, 0);

	sethash("baud", do_baud, 0);
#ifdef	HAVE_TERMIO_H
	sethash("clocal", do_cflags, CLOCAL);

#ifndef OS_DYNIX
	sethash("parenb", do_parity, PARENB);
#endif
	sethash("parodd", do_parity, PARENB|PARODD);

#ifndef OS_DYNIX
	sethash("cs5", do_csize, CS5);
	sethash("cs6", do_csize, CS6);
#endif
	sethash("cs7", do_csize, CS7);
	sethash("cs8", do_csize, CS8);
	sethash("twostop", do_cflags, CSTOPB);

#ifdef OS_DYNIX
	sethash("olcuc", do_lflag, XCASE);
	sethash("onlcr", do_oflag, ONLCR);
#else
	sethash("olcuc", do_oflag, OPOST|OLCUC);
	sethash("onlcr", do_oflag, OPOST|ONLCR);
	sethash("ocrnl", do_oflag, OPOST|OCRNL);
	sethash("onocr", do_oflag, OPOST|ONOCR);
	sethash("onlret", do_oflag, OPOST|ONLRET);
#endif
	sethash("extabs", do_oflag, OPOST|TAB3);
	sethash("ixon", do_iflag, IXON);
	sethash("ixany", do_iflag, IXANY);

#else
	sethash("ibaud", do_baud, 1);
	sethash("obaud", do_baud, 2);
	sethash("even", do_flags, EVENP);
	sethash("odd", do_flags, ODDP);
	sethash("raw", do_flags, RAW);
	sethash("crmod", do_flags, CRMOD);
	sethash("lcase", do_flags, LCASE);
	sethash("cbreak", do_flags, CBREAK);
	sethash("tandem", do_flags, TANDEM);
	sethash("extabs", do_flags, XTABS);
	sethash("tabs", do_flags, XTABS);
#endif

	setsymb("setup", TK_SETUP);
	setsymb("halt", TK_HALT);
	setsymb("docstart", TK_DOCST);
	setsymb("docend", TK_DOCEND);
	setsymb("sufstart", TK_SUFST);
	setsymb("sufend", TK_SUFEND);
	setsymb("pagestart", TK_PAGESTART);
	setsymb("pageend", TK_PAGEEND);
	setsymb("abort", TK_ABORT);
	setsymb("restart", TK_RESTART);
	setsymb("exec", TK_EXEC);
	setsymb("delimiter", TK_DELIM);
	setsymb("banner", TK_BANNER);
	setsymb("exit", TK_EXIT);
	setsymb("signal", TK_SIGNAL);
	setsymb("setoffline", TK_OFFLINE);
	setsymb("seterror", TK_ERROR);
	/* These are now tokens */
	setsymb("bannprog", TK_BANNPROG);
	setsymb("align", TK_ALIGN);
	setsymb("execalign", TK_EXECALIGN);
	setsymb("portsetup", TK_PORTSU);
	setsymb("filter", TK_FILTER);
	setsymb("network", TK_NETFILT);
	setsymb("stty", TK_STTY);
}

/* Calculate the length of a string - the result includes a
   terminating null character if it exists.  */

unsigned  calclength(struct string *str)
{
	unsigned  result  =  1;

	if  (!str)
		return  0;

	do  result += str->s_length;
	while  ((str = str->s_next));

	return  result;
}

void  checkfile(struct string *str, USHORT *lp, const unsigned mode, const int msgno, const int noappend)
{
	unsigned  lng  =  calclength(str);
	unsigned  elng = strlen(ptdir) + 1 + strlen(pname) + 1;

	if  (lng == 0)
		return;

	/* Reject 0-length strings */

	if  (lng == 1)  {
		disp_str = str->s_str;
		report(msgno);
		return;
	}

	*lp = lng;

	/* Find first bit containing char (must be 1 'cous lng > 1) */

	while  (str->s_length == 0)
		str = str->s_next;

	if  (mode != 0)  {
		char	*mbuf, *mp;

		if  (!(mbuf = malloc(lng + elng)))
			nomem();
		mp = mbuf;

		if  (!noappend  &&  str->s_str[0] != '/')  {
			*lp += elng;
			sprintf(mp, "%s/%s/", ptdir, pname);
			mp += elng;
		}
		do  {
			strcpy(mp, str->s_str);
			mp += str->s_length;
			str = str->s_next;
		}  while  (str);

		if  (access(mbuf, (int) mode) < 0)  {
			disp_str = str->s_str;
			report(msgno);
			return;
		}

		free(mbuf);
	}
	else  if  (!noappend  &&  str->s_str[0] != '/')
		*lp += elng;
}

void  dumpstr(struct string *str)
{
	if  (!str)
		return;

	do  fwrite(str->s_str, 1, (int) str->s_length, stdout);
	while  ((str = str->s_next));

	/* And a final null.  */

	putchar(0);
}

void  dumpfile(struct string *str, const int noappend)
{
	struct  string  *sp;
	for  (sp = str;  sp;  sp = sp->s_next)
		if  (sp->s_length > 0)
			break;
	if  (!sp)
		return;

	if  (!noappend  &&  sp->s_str[0] != '/')
		printf("%s/%s/", ptdir, pname);
	dumpstr(sp);
}

void  makeoutput()
{
	out_params.pi_setup = calclength(su_str);
	out_params.pi_halt = calclength(hlt_str);
	out_params.pi_docstart = calclength(ds_str);
	out_params.pi_docend = calclength(de_str);
	if  (!(out_params.pi_flags & PI_BANNBAUD))
		out_params.pi_banntty = out_params.pi_tty;
	if  (Hadbann)  {
		out_params.pi_bdocstart = calclength(bds_str);
		out_params.pi_bdocend = calclength(bde_str);
	}
	else  {
		/* If no banner case copy ordinary case including exec flags */
		if  (out_params.pi_flags & PI_EX_DOCST)
			out_params.pi_flags |= PI_EX_BDOCST;
		if  (out_params.pi_flags & PI_EX_DOCEND)
			out_params.pi_flags |= PI_EX_BDOCEND;
		out_params.pi_bdocstart = calclength(ds_str);
		out_params.pi_bdocend = calclength(de_str);
	}
	out_params.pi_sufstart = calclength(ss_str);
	out_params.pi_sufend = calclength(se_str);
	out_params.pi_pagestart = calclength(ps_str);
	out_params.pi_pageend = calclength(pe_str);
	out_params.pi_abort = calclength(ab_str);
	out_params.pi_restart = calclength(res_str);
	out_params.pi_rcstring = calclength(rc_str);

	/* Repeat for alignment file, record file and log file */

	if  (out_params.pi_flags & PI_EX_ALIGN)
		checkfile(out_align, &out_params.pi_align, 0, $E{spdinit align file access error}, 1);
	else
		checkfile(out_align, &out_params.pi_align, 04, $E{spdinit align file access error}, 0);
	checkfile(out_record, &out_params.pi_rfile, 06, $E{spdinit record file error}, 0);
	checkfile(out_logfile, &out_params.pi_logfile, 0, $E{spdinit log file error}, 0);

	/* Repeat for filter, network filter and port setup.  */

	out_params.pi_filter = calclength(out_filter);
	out_params.pi_netfilt = calclength(out_netfilt);
	out_params.pi_bannprog = calclength(out_bannprog);
	out_params.pi_portsu = calclength(out_portsu);
	out_params.pi_sttystring = calclength(out_sttystring);

	/* Stick out the header packet.  */

	fwrite((char *)&out_params, sizeof(out_params), 1, stdout);

	/* Stick out setup strings etc.  */

	dumpstr(su_str);
	dumpstr(hlt_str);
	dumpstr(ds_str);
	dumpstr(de_str);
	if  (Hadbann)  {
		dumpstr(bds_str);
		dumpstr(bde_str);
	}
	else  {
		dumpstr(ds_str);
		dumpstr(de_str);
	}
	dumpstr(ss_str);
	dumpstr(se_str);
	dumpstr(ps_str);
	dumpstr(pe_str);
	dumpstr(ab_str);
	dumpstr(res_str);
	dumpfile(out_align, out_params.pi_flags & PI_EX_ALIGN);
	dumpstr(out_filter);
	dumpfile(out_record, 0);
	dumpstr(rc_str);
	dumpfile(out_logfile, 0);
	dumpstr(out_portsu);
	dumpstr(out_bannprog);
	dumpstr(out_netfilt);
	dumpstr(out_sttystring);
}

/* Main routine.  Arguments are as follows:

	spdinit	printer formtype	*/

MAINFN_TYPE  main(int argc, char **argv)
{
	versionprint(argv, "$Revision: 1.2 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();
	Realuid = getuid();
	Effuid = geteuid();

	if  ((Cfile = open_icfile()) == (FILE *) 0)
		exit(E_NOCONFIG);

	if  (argc != 3)  {
		report($E{spdinit usage});
		exit(E_USAGE);
	}

	if  ((Suffix = strpbrk(argv[2], DEF_SUFCHARS)))
		*Suffix++ = '\0';
	else
		Suffix = "";

	default_form = envprocess(DEFAULT_FORM);
	ptdir = envprocess(PTDIR);
	pname = argv[1];

	if  (chdir(ptdir) >= 0  &&  chdir(pname) >= 0)  {
		if  ((infile = fopen(curr_file = PDEVFILE, "r")))  {
			formname = argv[2]; /* For it to open the other one later */
			initsymbs();
			readdescr();
			fclose(infile);
		}
		else  if  ((infile = fopen(curr_file = argv[2], "r"))  ||
			   (default_form[0]  &&  (infile = fopen(curr_file = default_form, "r"))))  {
			initsymbs();
			readdescr();
			fclose(infile);
		}
		else  {
			disp_str = pname;
			disp_str2 = default_form;
			report($E{spdinit no setup file});
		}
	}
	else  {
		disp_str = pname;
		disp_str2 = ptdir;
		report($E{spdinit no setup dir});
	}

	makeoutput();
	return  setuperrs > 0? E_BADSETUP :0;
}
