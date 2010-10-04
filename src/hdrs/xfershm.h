/* xfershm.h -- shared memory buffers for passing stuff around

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

/*----------------------------------------------------------------
 * Format of shared memory segment for passing job and printer structures
 * to scheduler.
 * Scheduler never sends them out it just tells other people where to find
 * them.
 */

#if	INITJALLOC < 1000
#define	TRANSBUF_NUM	((INITJALLOC*9 + 9) / 10)
#elif	NUMXBUFS * 25 > 1000
#define	TRANSBUF_NUM	1000
#else
#define	TRANSBUF_NUM	(NUMXBUFS*25)
#endif

struct	joborptr	{
	int_pid_t	jorp_sender;		/* For checking that we matched up */
	union	{
		struct	spq	q;
		struct	spptr	p;
	}  jorp_un;
};

struct	xfershm	{
	USHORT	xf_nonq;			/* Number queued */
	USHORT	xf_head, xf_tail;		/* Head/tail of list */
	struct  joborptr	xf_queue[TRANSBUF_NUM+1];
};

#ifndef	USING_FLOCK
extern void  set_xfer_server();
#endif
extern int  init_xfershm(const int);
extern int  wjmsg(struct spr_req *, struct spq *);
extern int  wpmsg(struct spr_req *, struct spptr *);
