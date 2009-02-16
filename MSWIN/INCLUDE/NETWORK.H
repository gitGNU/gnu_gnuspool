/* network.h -- network handling routines

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


#ifdef	__cplusplus
class	local_params	{
 public:
	short	listsock;		// Socket to listen for connections on
	short	probesock;		// Datagram socket to probe machines with
		
// The following are all htons-ified (which means byte-swapped on DOS)
// Port numbers used for above

	unsigned  short	lportnum;
	unsigned  short	vportnum;
	unsigned  short	pportnum;
	unsigned  short uaportnum;	// User enquiry port number

	short	tcpproto;		// Protocol numbers (try to do things properly)
	short	udpproto;		// Or something equivalent for non TCP nets

	short	Netsync_req;		// Tells us if we need to sync something
	netid_t  myhostid;		// My internet address htonl-ified
	netid_t  servid;		// Server internet address htnol-ified
};

extern	local_params	Locparams;
#endif

struct	remote	{
	char	hostname[HOSTNSIZE];	// Actual host name 
	char	alias[HOSTNSIZE];	// Alias for within Xi-Text 
	short	sockfd;			// Socket fd to talk to it 
	netid_t	hostid;			// Host id in network byte order 
	unsigned char	is_sync;	// sync flags 
#define	NSYNC_NONE	0		// Not done yet 
#define	NSYNC_REQ	1		// Requested but not complete 
#define	NSYNC_OK	2		// Completed 
	unsigned  char	ht_flags;	// Host-type flags 
#define	HT_ISCLIENT	(1 << 0)	// Set to indicate "I" am client 
#define	HT_PROBEFIRST	(1 << 1)	// Probe connection first 
#define	HT_MANUAL	(1 << 2)	// Manual connection only
#define	HT_DOS		(1 << 3)	// DOS client (only used at other end)
#define	HT_SERVER	(1 << 4)	// Unix server
	unsigned  short	ht_timeout;	// Timeout value (seconds) 
	time_t	lastwrite;		// When last done
	unsigned  short	ht_seqto;	// Sequence TO
	unsigned  short ht_seqfrom;	// Sequence FROM
};

#ifdef	__cplusplus
struct	feeder	{
	char	fdtype;		// Type of file require 
#define	FEED_SP		0	// Feed spool file 
#define	FEED_NPSP	1	// Feed spool file, don't bother with pages 
#define	FEED_ER		2	// Feed error file 
#define	FEED_PF		3	// Feed page file 
	char	resvd[3];	// Pad out to 4 bytes 
	slotno_t  jobslot;	// Job slot net byte order 
	jobno_t	 jobno;		// Jobnumber net byte order
	feeder(const int t, const slotno_t s, const jobno_t j) : fdtype(char(t)), jobslot(s), jobno(j)
	{	resvd[0] = resvd[1] = resvd[2] = 0;	}
};

//  Function to invoke the above we don't use a feeder constructor as
//  things need byte-swapping
	
int	net_feed(const int, const netid_t, const slotno_t, const jobno_t);

//	General messages

struct	netmsg	{
	unsigned  short	code;	// Code number 
	short		seq;		// Sequence 
	netid_t	hostid;			// Host id net byte order 
	long		arg;		// Argument
	netmsg()  { }
	netmsg(const unsigned short c, const netid_t h, const unsigned short s=0, const long a=0) : code(c), hostid(h), arg(a), seq(s) {}
};

extern	void	netmonitor(void);
#endif
