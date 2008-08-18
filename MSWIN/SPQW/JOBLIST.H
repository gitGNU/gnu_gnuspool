#define	INITJOBS		10		// Initial space to reserve
#define	INCJOBS			5		// Space for more

class	joblist	{
private:
	unsigned  njobs;
	unsigned  maxjobs;
	spq		  **list;
	void	  checkgrow();
public:
	joblist()	{ njobs = maxjobs = 0; list = NULL;	}
	~joblist()	{ if  (list)  delete [] list; }
	unsigned	number()	const  {  return  njobs;	}
	int		jindex(const jident &)  const;
	int		jindex(const spq &)  const;
	spq		*operator [] (const unsigned)  const;
	spq		*operator [] (const jident &)  const;
	void	append(spq  *);
	void	insert(spq  *, const unsigned);
	void	remove(const unsigned);
	int		move(const unsigned, const unsigned);
	void	clear() { njobs = 0; }
// Network ops
	void	net_jclear(const netid_t);
	void	addjob(const jident &, const spq &);
	void	changedjob(const jident &, const spq &);
	void	unassign(const jident &, const spq &);
	void	deljob(const jident &);
	void	locpassign(const jident &);
	void	chgjob(const spq &);
	void	opjob(const int, const jident &);
	void	assign(const int, const int);
	void	pageupdate(const spq &);
};
	