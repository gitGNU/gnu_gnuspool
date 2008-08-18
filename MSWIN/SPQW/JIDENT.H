class	jident	{
 private:
	jobno_t		jobno;		    // Job number
	netid_t	    remote_id;	    // Host it is on (0 means this one)
	slotno_t	remote_slot;	// Slot number in job queue
 public:
	jident(jobno_t j = 0, netid_t h = 0, slotno_t s = -1) : jobno(j), remote_id(h), remote_slot(s) { }
	jident(const spq FAR *q) { jobno = q->spq_job; remote_id = q->spq_netid; remote_slot = remote_id? q->spq_rslot: -1; };
	int operator==(const jident& b) { return  jobno == b.jobno  &&  remote_id == b.remote_id; };
	friend  class  joblist;
};
