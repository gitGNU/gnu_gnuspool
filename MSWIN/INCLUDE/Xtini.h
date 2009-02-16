/* Xtini.h -- options spec in .ini file

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
