//----------------------------------------------------------------------
// $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/Xtini.h,v 1.1 2008/08/18 16:25:54 jmc Exp $
// $Log: Xtini.h,v $
// Revision 1.1  2008/08/18 16:25:54  jmc
// Initial revision
//
//----------------------------------------------------------------------
// Options description for DOS/Windows Xi-Text.
// Note that we now store everything in the text file C:\XITEXT\XITEXT.INI
// rather than using a binary file.

struct	spropts	{
	unsigned  char	verbose,
					interpolate,
					textmode,
					freeze_wanted;	
	struct	pages	pfe;
	char			*delimiter;

	//	Define defaults with this constructor

	spropts()
	{
		verbose = 0;
		interpolate = 0;
		textmode = 0;
		freeze_wanted = 0;
		pfe.delimnum = 1;
		pfe.deliml = 1;
		pfe.lastpage = 0;
		delimiter = new char [2];
		delimiter[0] = '\f';
		delimiter[1] = '\0';
	}
	spropts(const spropts &r)
	{
		verbose = r.verbose;
		interpolate = r.interpolate;
		textmode = r.textmode;
		freeze_wanted = r.freeze_wanted;
		pfe = r.pfe;
		delimiter = new char [pfe.deliml + 1];
		memcpy((void *) delimiter, (void *) r.delimiter, (size_t) pfe.deliml);
	}
	~spropts()
	{
		if  (delimiter)  {
			delete [] delimiter;
			delimiter = NULL;
		}
	}
	spropts &operator=(const spropts &r)
	{
		verbose = r.verbose;
		interpolate = r.interpolate;
		textmode = r.textmode;
		freeze_wanted = r.freeze_wanted;
		pfe = r.pfe;
		delete [] delimiter;
		delimiter = new char [pfe.deliml + 1];
		memcpy((void *) delimiter, (void *) r.delimiter, (size_t) pfe.deliml);
		return  *this;
	}
};

struct	spqopts	{
	unsigned  short  Pollinit;	//  Initial polling
	unsigned  short  Pollfreq;	//  Current polling frequency
	unsigned  long   classcode;	//  Current class code obtained from user profile
	unsigned  char	confabort,	//  Confirm deletion
					Restrunp,	//  Restrict to unprinted
					probewarn,	//  Warn about failed probes
					batext,     //  Unqueue with .bat extension
					abshold;	//  Absolute value for hold time
	CString	Restrp,             //  Name of printer we restrict attention to
			Restru,             //  Name of user we restrict attention to
			Restrt;             //  Title we restrict attention to

	//	Define defaults with this constructor

	spqopts()
	{
		Restrunp = batext = abshold = 0;
		Restrp = Restru = Restrt = "";
		confabort = 1;
		Pollinit = Pollfreq = DEFAULT_REFRESH;		//  In defaults.h
		probewarn = 1;
	}
};

struct	xtini	{
	struct	spq 		qparams;
	struct	spropts		spr_options;	//  Options for SPR
	struct	spqopts		spq_options;	//  Options for SPQ
	xtini();
};

extern	struct	xtini	*Options;
