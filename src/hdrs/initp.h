/* initp.h -- parameter packet for spdinit->spd

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

struct	initpkt	{
	ULONG	pi_flags;	/*  Flags  */
	ULONG	pi_flags2;	/*  More flags */
	ULONG	pi_charge;	/*  Charge per char  */
	ULONG	pi_windback;	/*  Number of pages to wind back*/
	ULONG	pi_obuf;	/*  Output buffer size  */
	USHORT	pi_oa;		/*  Open timeout  */
	USHORT	pi_ca;		/*  Close timeout */
	USHORT	pi_clsig;	/*  Close signal for net filters */
	USHORT	pi_postclsl;	/*  Post-close sleep for reopen case */
	USHORT	pi_offa;	/*  Offline timeout  */
	USHORT	pi_width;	/*  Paper width (chars)  */
	USHORT  pi_setup;	/*  Length of setup string  */
	USHORT  pi_halt;	/*  Length of halt string */
	USHORT  pi_docstart;	/*  Length of document start string */
	USHORT  pi_docend;	/*  Length of document end string */
	USHORT  pi_bdocstart;	/*  Length of banner document start string */
	USHORT  pi_bdocend;	/*  Length of banner document end string */
	USHORT  pi_sufstart;	/*  Length of suffix start string */
	USHORT  pi_sufend;	/*  Length of suffix end string */
	USHORT  pi_pagestart;	/*  Length of page start string */
	USHORT  pi_pageend;	/*  Length of page end string */
	USHORT	pi_abort;	/*  Length of abort string */
	USHORT	pi_restart;	/*  Length of restart string */
	USHORT  pi_align;	/*  Length of alignment file name  */
	USHORT  pi_filter;	/*  Filter string  */
	USHORT  pi_rfile;	/*  Record file  */
	USHORT  pi_offset;	/*  Offset in file  */
	USHORT	pi_logfile;	/*  Log file  */
	USHORT  pi_rcstring;	/*  Record count string  */
	USHORT  pi_portsu;	/*  Port setup string  */
	USHORT	pi_netfilt;	/*  Network filter */
	USHORT  pi_bannprog;	/*  Banner program */
	USHORT  pi_sttystring;	/*  Give up use stty string*/
	ULONG   pi_offlsig;	/*  1 << signum means offline*/
	ULONG   pi_errsig;	/*  1 << signum means error */
	ULONG   pi_offlexit[256/32]; /* Offline exit codes */
	ULONG   pi_errexit[256/32];  /* Error exit codes */
	unsigned	pi_rcount;	/*  Count of record strings */
#ifdef	HAVE_TERMIO_H
#ifdef	PRIME	/* Use old version which doesn't mangle ^s/^q */
	struct	Otermio	pi_tty;		/*  Terminal parameters  */
	struct	Otermio	pi_banntty;	/*  Terminal parameters  */
#else
	struct	termio	pi_tty;		/*  Terminal parameters  */
	struct	termio	pi_banntty;	/*  Terminal parameters  */
#endif
#else
	struct	sgttyb	pi_tty;		/*  Terminal parameters  */
	struct	sgttyb	pi_banntty;	/*  Terminal parameters  */
#endif
};

/* Following the above structure we have:
   setup string
   halt string
   sufstart string
   sufend string
   docstart string
   docend string
   ditto for banners
   abort string
   restart string
   alignment file
   filter string
   record file
   record count string
   log file
   port setup
   banner program
   network filter
   stty string

   Flags for pi_flags */

#define	PI_NOHDR	(1 << 0)
#define	PI_FORCEHDR	(1 << 1)
#define	PI_HDRPERCOPY	(1 << 2)
#define	PI_SINGLE	(1 << 3)
#define	PI_EX_SETUP	(1 << 4)
#define	PI_EX_HALT	(1 << 5)
#define	PI_EX_DOCST	(1 << 6)
#define	PI_EX_DOCEND	(1 << 7)
#define	PI_EX_BDOCST	(1 << 8)
#define	PI_EX_BDOCEND	(1 << 9)
#define	PI_EX_SUFST	(1 << 10)
#define	PI_EX_SUFEND	(1 << 11)
#define	PI_EX_PAGESTART	(1 << 12)
#define	PI_EX_PAGEEND	(1 << 13)
#define	PI_EX_ABORT	(1 << 14)
#define	PI_EX_RESTART	(1 << 15)
#define	PI_RETAIN	(1 << 16)
#define	PI_REOPEN	(1 << 17)
#define	PI_BANNBAUD	(1 << 18)
#define	PI_CANHANG	(1 << 19)	/* Nasty device can hang the system */
#define	PI_EX_ALIGN	(1 << 20)
#define	PI_LOGERROR	(1 << 21)	/* Send stderr to system log */
#define	PI_FBERROR	(1 << 22)	/* Feed back stderr in printer shm */
#define	PI_NORANGES	(1 << 23)	/* Do not try to emulate page ranges */
#define	PI_NOCOPIES	(1 << 24)	/* Force 1 copy only */
#define	PI_INCLP1	(1 << 25)	/* Include page 1 always */
/* The following must be greater than PI_EX_ALIGN to make the code for -exec work
   not setting ST_NOESC in readfile */
#define	PI_EXEXALIGN	(1 << 26)	/* Direct execute exec align */
#define	PI_EXBANNPROG	(1 << 27)	/* Direct execute banner prog */
#define	PI_EXPORTSU	(1 << 28)	/* Direct execute port setup */
#define	PI_EXFILTPROG	(1 << 29)	/* Direct execute filter prog */
#define	PI_EXNETFILT	(1 << 30)	/* Direct execute network filter */
#define	PI_EXSTTY	(1 << 31)	/* Direct execute stty prog */

#define	PI_ADDCR	(1 << 0) 	/* (pi_flags2) Add CR to each line for network printers */

/* Flags for closing filters with (arg to filtclose defined in
   sd_fctrl.c), used to be 2 separate args but I kept forgetting
   what they were.  */

#define	FC_NOERR	1
#define	FC_ABORT	2
#define	FC_KILL		4
