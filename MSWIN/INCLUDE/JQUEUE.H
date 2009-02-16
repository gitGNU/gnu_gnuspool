/* jqueue.h -- For remote references to jobs

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

class	jident	{
 public:
	jobno_t		jobno;		// Job number
	netid_t	remote_id;	// Host it is on (0 means this one)
	slotno_t	remote_slot;	// Slot number in job queue
	jident(jobno_t j, netid_t h = 0, slotno_t s = -1) : jobno(j), remote_id(h), remote_slot(s) { };
	jident(const spq *q) { jobno = q->spq_job; remote_id = q->spq_netid; remote_slot = remote_id? q->spq_rslot: -1; };
	int operator==(const jident& b) { return  jobno == b.jobno  &&  remote_id == b.remote_id; };
};

#define	INITJOBS	10		// Initial space to reserve
#define	INCJOBS		5		// Space for more

class	pident;

class	jqueue	{
 private:
	unsigned	njobs;		// Number currently known
	unsigned	maxjobs;	// Number allocated
	spq		**jlist;	// Vector of pointers to structures
 public:
	jqueue();
	~jqueue();
	void	addjob(const jident &, const spq&);	// Note new job
	void	deljob(const jident &);			// Delete specified job 
	void	changedjob(const jident &, const spq&);	// Changed job
	void	chgjob(const spq &);			// Do changes to job
	void	opjob(const int, const jident &);	// Send message about job
	void	net_jclear(const netid_t);		// Zap details of jobs for dying machines
	void	locpassign(const jident &);		// Local printer assign
	void	jpassign(const jident &, const pident &);	//  Printer/job assign
	void	unassign(const jident &, const spq&);	// Unassigned job
	spq	*operator[] (const int);		// Read job
	unsigned  number();				// Number of jobs (we can see)
	int  jindex(const spq &);			// Where is it
	int  jindex(const jident &);			// Ditto based on job index
};

extern	jqueue	Jobs;
