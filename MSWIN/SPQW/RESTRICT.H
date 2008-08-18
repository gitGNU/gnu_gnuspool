//	Restrict view of jobs and printers in particular windows
//	Write to allow room for expansion...

class	restrictdoc	{
public:
    enum	onlyunprinted { ALLITEMS, ONLYUNPRINTED, ONLYPRINTED};
private:
	CString	user;
	CString	printer;
	CString title;
	onlyunprinted  onlyp;
	int		jinclude;
	classcode_t	classcode;
public:
	restrictdoc(const restrictdoc * = NULL);
	void	getrestrict(int &, unsigned long &, CString &, CString &, CString &, int &);
	void	setuser(const CString &);
	void	setprinter(const CString &);
	void	settitle(const CString &);
	void	setonlyunprinted(const onlyunprinted op = ALLITEMS)
	    {	onlyp = op;	}
	void	setjincl(const int jincl = 1)
		{   jinclude = jincl;	}
	void	setclass(const classcode_t classc)
	    {	classcode = classc;	}
	int		visible(const spq FAR &);
	int		visible(const spptr FAR &);
	void	loadrestrict();
	void	saverestrict();
};		