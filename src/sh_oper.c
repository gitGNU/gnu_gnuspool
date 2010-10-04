/* sh_oper.c -- spshed interact with UI programs

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
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include "errnums.h"
#include "defaults.h"
#include "network.h"
#include "spq.h"
#include "files.h"
#include "incl_unix.h"

void  report(const int);
extern  void  rewrjq();
extern  void  rewrpq();

extern	int	qchanges;

#define	OPINIT	5
#define	OPINC	3

unsigned  numopers, maxopers;
struct	opstr	{
	int_pid_t	pid;
	int_ugid_t	uid;
}  *oplist;

#ifdef	NETWORK_VERSION
extern	PIDTYPE	Xtns_pid;
#endif

/* Allocate a structure for an operator.  */

void  makeop(const int_pid_t pid, const int_ugid_t uid)
{
	if  (numopers >= maxopers)  {
		if  (maxopers)  {
			maxopers += OPINC;
			oplist = (struct opstr *) realloc((char *) oplist, maxopers*sizeof(struct opstr));
		}
		else  {
			maxopers = OPINIT;
			oplist = (struct opstr *) malloc(OPINIT * sizeof(struct opstr));
		}
		if  (oplist == (struct opstr *) 0)
			nomem();
	}

	oplist[numopers].pid = pid;
	oplist[numopers].uid = uid;
	numopers++;
}

/* Delete a structure for an operator.  */

void  killop(const int_pid_t pid)
{
	int	i;

	for  (i = 0;  i < numopers;  i++)
		if  (pid == oplist[i].pid)  {
			--numopers;
			if  (i != numopers)
				oplist[i] = oplist[numopers];
			return;
		}
}

/* Find an operator by process id.  */

int_pid_t  findop(const int_pid_t pid)
{
	int	i;

	for  (i = 0;  i < numopers;  i++)
		if  (oplist[i].pid == pid)
			return  pid;
	return	0;
}

/* An operator has just signed on - note the fact.  */

void  addoper(struct sp_omsg *rq)
{
	if  (findop(rq->spr_pid) == 0)
		makeop(rq->spr_pid, (int_ugid_t) rq->spr_arg1);

	kill((PIDTYPE) rq->spr_pid, QRFRESH);
}

/* Tell operators. */

void  tellopers()
{
	int	i = 0;

	qchanges = 0;
#ifdef	NETWORK_VERSION
	if  (Xtns_pid > 0)
		kill(-Xtns_pid, QRFRESH);
#endif
redo:
	for  (;  i < numopers;  i++)  {

		if  (kill((PIDTYPE) oplist[i].pid, QRFRESH) < 0)  {

			if  (errno == EPERM)
				report($E{Recompile port});

			/* He must have gone away (or setuid bug).  */

			killop(oplist[i].pid);
			goto  redo;
		}
	}
}

/* Delete an operator. */

void  deloper(struct sp_omsg *rq)
{
	if  (findop(rq->spr_pid) == 0)
		return;
	killop(rq->spr_pid);
}

/* Kill off operators.  */

void  killops()
{
	int	i;

	for  (i = 0;  i < numopers;  i++)
		kill((PIDTYPE) oplist[i].pid, SIGTERM);
}

/* Is given uid on list of operators */

int  islogged(const int_ugid_t uid)
{
	int	i;

	for  (i = 0;  i < numopers;  i++)
		if  (oplist[i].uid == uid)
			return  1;
	return  0;
}
