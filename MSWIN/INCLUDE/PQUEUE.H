/* pqueue.h -- MS C++ Printers queue

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
