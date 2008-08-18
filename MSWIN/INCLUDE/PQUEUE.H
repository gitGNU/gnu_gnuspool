/*
 * Copyright (c) Xi Software Ltd. 1992.
 *
 * pqueue.h: created by John Collins on Thu Oct 22 1992.
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/PQUEUE.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: PQUEUE.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 * MS C++ Printers queue
 */

// For remote references to printers

class	jident;
class	pident	{
 public:
	netid_t	remote_id;	// Host it is on (0 means this one)
	slotno_t	remote_slot;	// Slot number in printer queue
	pident(netid_t h = 0, slotno_t s = -1) : remote_id(h), remote_slot(s) { };
	pident(const spptr *p) { remote_id = p->spp_netid; remote_slot = remote_id? p->spp_rslot: -1; };
};

#define	INITPTRS	5		// Initial space to reserve
#define	INCPTRS		2		// Space for more

class	pqueue	{
	friend	void	jqueue::jpassign(const jident &, const pident &);
 private:
	unsigned	nptrs;		// Number currently known
	unsigned	maxptrs;	// Number allocated
	spptr		**plist;	// Vector of pointers to structures
 public:
	pqueue();
	~pqueue();
	void	addptr(const pident &, const spptr&);	// Note new printer
	void	delptr(const pident &);			// Delete specified printer 
	void	changedptr(const pident &, const spptr&);// Changed printer
	void	chgptr(const spptr &);			// Do change printer
	void	opptr(const int, const pident &);	// Signal printer
	void	net_pclear(const netid_t);		// Clear away printers on death of host
	void	pqueue::unassign_ptr(const pident &, const spptr &);	// Finished printing
	spptr	*printing(const jident &);
	spptr	*operator[] (const int);		// Read printer
	unsigned  number();				// Number of printers
	int  pindex(const spptr &);			// Where is it
	int  pindex(const pident &);			// Ditto based on printer index
};

extern	pqueue	Printers;
