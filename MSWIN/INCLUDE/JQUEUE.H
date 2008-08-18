/*
 * Copyright (c) Xi Software Ltd. 1992.
 *
 * jqueue.h: created by John Collins on Wed Oct 21 1992.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/JQUEUE.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: JQUEUE.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 * MS C++ Version
 */

// For remote references to jobs

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
