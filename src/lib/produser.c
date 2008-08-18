/* produser.c -- after changing user file, tell scheduler

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "ipcstuff.h"
#include "incl_unix.h"

/* After rebuilding spufile, see if scheduler is running and if so kick scheduler.  */

void	produser(void)
{
	int	Ctrl_chan;

	if  ((Ctrl_chan = msgget(MSGID, 0)) >= 0)  {
		struct	spr_req	oreq;
		BLOCK_ZERO(&oreq, sizeof(oreq));
		oreq.spr_mtype = MT_SCHED;
		oreq.spr_un.o.spr_act = SOU_PWCHANGED;
		msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(struct sp_omsg), 0);
	}
}
