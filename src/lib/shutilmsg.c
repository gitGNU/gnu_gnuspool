/* shutilmsg.c -- message/signal handling for shell routines

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
#include "incl_sig.h"
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "spq.h"
#include "spuser.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "xfershm.h"
#include "errnums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "shutilmsg.h"

#define	HTIME	5		/* Forge prompt if one doesn't come */

int	hadrfresh;
#ifdef	UNSAFE_SIGNALS
char	jset;
static	jmp_buf	Mj;
#endif

int	Ctrl_chan;

/* This notes signals from (presumably) the scheduler.  */

RETSIGTYPE  markit(int sig)
{
#ifdef	UNSAFE_SIGNALS
	signal(sig, markit);
	hadrfresh++;
	if  (jset)
		longjmp(Mj, 1);
#else
	hadrfresh++;
#endif
}

void  waitsig()
{
#ifdef	UNSAFE_SIGNALS
	if  (!setjmp(Mj))
		jset = 1;
	alarm(HTIME);
	while  (!hadrfresh)
		pause();
	hadrfresh = 0;
	alarm(0);
	jset = 0;
#else
	/* We have some sort of "safe" signal.  */

#ifdef	HAVE_SIGACTION
	sigset_t	nset, uset;
	sigfillset(&uset);
	sigdelset(&uset, QRFRESH);
#endif
	alarm(HTIME);
	while  (!hadrfresh)
#ifdef	HAVE_SIGACTION
		sigsuspend(&uset);
#elif	defined(STRUCT_SIG)
		sigpause(0);
#elif	defined(HAVE_SIGSET)
		sigpause(QRFRESH);
#else
		pause();
#endif
	alarm(0);
	hadrfresh = 0;
#ifdef	HAVE_SIGACTION
	sigemptyset(&nset);
	sigaddset(&nset, QRFRESH);
	sigprocmask(SIG_UNBLOCK, &nset, (sigset_t *) 0);
#elif	defined(STRUCT_SIG)
	sigsetmask(0);
#else
	sigrelse(QRFRESH);
#endif
#endif
}

int  msg_log(const int msg, const int wt)
{
	struct	spr_req	oreq;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	zm;
#endif
#ifdef	HAVE_SIGACTION
	sigset_t	nset;
#endif
	int	blkcount = MSGQ_BLOCKS;

	if  (wt)  {
#ifdef	STRUCT_SIG
		zm.sighandler_el = markit;
		sigmask_clear(zm);
		zm.sigflags_el = SIGVEC_INTFLAG;
		sigact_routine(QRFRESH, &zm, (struct sigstruct_name *) 0);
		sigact_routine(SIGALRM, &zm, (struct sigstruct_name *) 0);
#else
		signal(QRFRESH, markit);
		signal(SIGALRM, markit);
#endif
#ifndef	UNSAFE_SIGNALS
#ifdef	HAVE_SIGACTION
		sigemptyset(&nset);
		sigaddset(&nset, QRFRESH);
		sigprocmask(SIG_BLOCK, &nset, (sigset_t *) 0);
#elif	defined(STRUCT_SIG)
		sigsetmask(sigmask(QRFRESH));
#else
		sighold(QRFRESH);
#endif
#endif /* UNSAFE_SIGNALS */
	}
	else  {
#ifdef	STRUCT_SIG
		zm.sighandler_el = SIG_IGN;
		sigmask_clear(zm);
		zm.sigflags_el = SIGVEC_INTFLAG;
		sigact_routine(QRFRESH, &zm, (struct sigstruct_name *) 0);
		sigact_routine(SIGALRM, &zm, (struct sigstruct_name *) 0);
#else
		signal(QRFRESH, SIG_IGN);
		signal(SIGALRM, SIG_IGN);
#endif
	}

	oreq.spr_mtype = MT_SCHED;
	oreq.spr_un.o.spr_pid = getpid();
	oreq.spr_un.o.spr_act = (USHORT) msg;
	oreq.spr_un.o.spr_arg1 = Realuid;
	oreq.spr_un.o.spr_arg2 = 0;
	oreq.spr_un.o.spr_seq = 0;
	oreq.spr_un.o.spr_netid = 0;

	while  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), IPC_NOWAIT) < 0)  {
		if  (errno != EAGAIN)
			return  $E{IPC msg q error};
		blkcount--;
		if  (blkcount <= 0)
			return  $E{IPC msg q full};
		sleep(MSGQ_BLOCKWAIT);
	}
	if  (wt)
		waitsig();
	return  0;
}
