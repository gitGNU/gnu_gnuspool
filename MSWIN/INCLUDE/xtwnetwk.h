// Copyright (c) Xi Software Ltd. 1994.
//
// winnetwk.h: created by John Collins on Wed Jan  5 1994.
//----------------------------------------------------------------------
// $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/xtwnetwk.h,v 1.1 2008/08/18 16:25:54 jmc Exp $
// $Log: xtwnetwk.h,v $
// Revision 1.1  2008/08/18 16:25:54  jmc
// Initial revision
//
//----------------------------------------------------------------------
// Network.h revised for the windows market....

class	local_params	{
 public:
	SOCKET	listsock;		// Socket to listen for connections on
	SOCKET	probesock;		// Datagram socket to probe machines with
	SOCKET	uasocket;		// User enquiry socket
		
// The following are all htons-ified (which means byte-swapped on DOS)
// Port numbers used for above - SPRSETW Re-uses lportnum for count of hosts

	unsigned  short	lportnum;
	unsigned  short	vportnum;
	unsigned  short	pportnum;
	unsigned  short uaportnum;	// User enquiry port number

	short	Netsync_req;		// Tells us if we need to sync something
	unsigned  short	servtimeout;	//  Timeout for server
	time_t	 tlastop;		// Time of last op on uasocket
	netid_t  myhostid;		// My internet address htonl-ified
	netid_t  servid;		// Server internet address htnol-ified
	local_params() { listsock = probesock = uasocket = INVALID_SOCKET; }
};

extern	local_params	Locparams;

#ifdef SPRSETW
inline	unsigned  num_hosts()	{	return  Locparams.lportnum;	}
#endif

class	remote	{
private:
	remote	*hh_next;		// Hash table of host names
	remote	*ha_next;		// Hash table of alias names
	remote	*hn_next;		// Hash table of netids
#ifdef SPRSETW
	char		*h_name;	// Actual host name 
	char		*h_alias;	// Alias for within Xi-Text
#else
	const char	*h_name;	// Actual host name 
	const char	*h_alias;	// Alias for within Xi-Text
#endif
public:
	const netid_t  hostid;	// hton-ified host id
	SOCKET		sockfd;		// Socket fd to talk to it 
	unsigned char	is_sync;// sync flags 
#define	NSYNC_NONE	0		// Not done yet 
#define	NSYNC_REQ	1		// Requested but not complete 
#define	NSYNC_OK	2		// Completed 
	unsigned  char	ht_flags;	// Host-type flags 
#define	HT_ISCLIENT	(1 << 0)	// Set to indicate "I" am client 
#define	HT_PROBEFIRST	(1 << 1)	// Probe connection first 
#define	HT_MANUAL	(1 << 2)		// Manual connection only
#define	HT_DOS		(1 << 3)		// DOS client (only used at other end)
#define	HT_SERVER	(1 << 4)		// Unix server
	unsigned  short	ht_timeout;		// Timeout value (seconds) 
	time_t	lastwrite;				// When last done
	unsigned  short	ht_seqto;		// Sequence TO
	unsigned  short ht_seqfrom;		// Sequence FROM
#ifdef	SPQW
	unsigned  short	bytesleft;		// Bytes left to read in current message
	unsigned  short byteoffset;		// Where we have got to
	char			buffer[1024];	//  Believed to be the longest thing
#endif

// Access functions

	remote(const  netid_t,
	       const char FAR *,
	       const char FAR * = NULL,
	       const unsigned char = 0,
	       const unsigned short = NETTICKLE);

	const  char	*namefor()	const	{  return  h_alias? h_alias: h_name;	}
	
#ifdef SPRSETW
	~remote()	{	if  (h_name) delete h_name; if  (h_alias) delete h_alias;  }
	
	const  char	*hostname()	const	{  return  h_name;	}
	const  char	*aliasname()	const	{  return  h_alias? h_alias: "";	}
	void	delhost();
	friend void	savehostfile();    
	friend BOOL	clashcheck(const char FAR *);
	friend BOOL	clashcheck(const netid_t);                                                     
	friend remote	*get_nth(const unsigned);                                                     
#endif
	void	addhost();
	friend remote		*find_host(const netid_t);    
	friend remote		*find_host(const SOCKET);    
	friend const char 	*look_host(const netid_t);
	friend netid_t		look_hname(const char *);
#ifdef	SPQW
	void	probe_attach();
	void	conn_attach();
	void	rattach();
	friend	void	attach_hosts();
#endif
};

#define HASHMOD 17     // Not very big prime number
#define INC_REMOTES 4

//  Lists of remotes we know and are probing for.  Do this
//  as a nice class.

class	remote_queue	{
 private:
	unsigned  max, lookingat;
	remote	  **list;
 public:
	remote_queue()
	{
		max = 0;
		lookingat = 0;
		list = (remote **) 0;
	}
	unsigned	alloc(remote *);
	remote		*find(const netid_t);
#ifdef	SPQW
	remote		*find(const SOCKET);
#endif
	void		setfirst()	{	lookingat = 0;	}
	void		setlast()	{	lookingat = max;	}
	remote		*next()
	{   
		remote  *result = (remote *) 0;
		while  (lookingat < max  &&  !result)
			result = list[lookingat++];
		return  result;			
	}
	remote		*prev()
	{
		remote  *result = (remote *) 0;
		while  (lookingat != 0  &&  !result)
			result = list[--lookingat];
		return  result;
	}
	void	free(const remote * const);
	unsigned	index(const remote * const);
	remote	*operator [] (const unsigned);
};

extern	remote_queue	current_q, pending_q;

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
	short			seq;	// Sequence 
	netid_t			hostid;	// Host id net byte order 
	long			arg;	// Argument
	netmsg()  { }
	netmsg(const unsigned short c, const netid_t h, const unsigned short s=0, const long a=0) : code(c), hostid(h), arg(a), seq(s) {}
};

extern	UINT    initsockets();
extern	UINT	initenqsocket(const netid_t);
extern	BOOL	refreshconn();


