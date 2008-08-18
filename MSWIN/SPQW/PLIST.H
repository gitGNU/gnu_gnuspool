#define	INITPTRS	5		// Initial space to reserve
#define	INCPTRS		2		// Space for more

class	plist	{
 private:
	unsigned	nptrs;		// Number currently known
	unsigned	maxptrs;	// Number allocated
	spptr		**list;		// Vector of pointers to structures
	void	checkgrow();
 public:
	plist()		{  nptrs = maxptrs = 0;  list = NULL;	}
	~plist()	{	if (list)	delete [] list;	}
	unsigned	number()  const  FAR	{	return  nptrs;	}
	int		pindex(const pident &)  const  FAR;
	int		pindex(const spptr &)  const  FAR;
	spptr	*printing(const jident &)  const  FAR;
	spptr	*operator [] (const unsigned)  const  FAR;
	spptr	*operator [] (const pident &)  const  FAR;
	void	append(spptr *);
	void	remove(const unsigned);
	void	clear()	{	nptrs = 0;	}
	// Reset assignment slots.
	void	resetass(const int from, const int to)
	{
		for  (unsigned cnt = 0; cnt < nptrs; cnt++)
			if  (list[cnt]->spp_jslot == from)
				list[cnt]->spp_jslot = to;
	}			
// Network operations
	void	net_pclear(const netid_t);
	void	addptr(const pident &, const spptr &);
	void	changedptr(const pident &, const spptr &);
	void	unassign_ptr(const pident &, const spptr &);
	void	delptr(const pident &);
	void	chgptr(const spptr &);
	void	opptr(const int, const spptr *);
	void	assign(const int, const int, const jobno_t);
};
