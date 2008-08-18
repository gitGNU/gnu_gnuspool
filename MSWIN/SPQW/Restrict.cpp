#include "stdafx.h"
#include "jobdoc.h"
#include "spqw.h"
#include <string.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern	BOOL	qmatch(CString &, const char FAR *);
extern	BOOL	issubset(CString &patterna, CString &patternb);

restrictdoc::restrictdoc(const restrictdoc *previous)
{
   	if  (previous)
   		*this = *previous;
   	else  {
   		user = "";
   		printer = "";
   		title = "";
   		onlyp = ALLITEMS;
   		jinclude = 1;
   		classcode = U_MAX_CLASS;
   	}
}

void	restrictdoc::getrestrict(int &isonlyp, unsigned long &classc, CString &up, CString &pp, CString &tt, int &jincl)
{
	isonlyp = onlyp == ONLYPRINTED? 2: onlyp == ONLYUNPRINTED? 1: 0;
	classc = classcode;
	up = user;
	pp = printer;
	tt = title;
	jincl = jinclude;
}
			 
void	restrictdoc::setuser(const CString &u)
{
	user = u;
}

void	restrictdoc::setprinter(const CString &p)
{
	printer = p;
}

void	restrictdoc::settitle(const CString &t)
{
	title = t;
}

int		restrictdoc::visible(const spq FAR &q)
{
	return  (user.IsEmpty() || qmatch(user, q.spq_uname))  &&
			(title.IsEmpty() || qmatch(title, q.spq_file)) &&
			(printer.IsEmpty() ||
				(q.spq_ptr[0] == '\0' &&  jinclude >= 1) ||
				(jinclude > 1  ||  issubset(CString(q.spq_ptr), CString(printer))))  &&
			(onlyp == ALLITEMS || (onlyp == ONLYUNPRINTED && !(q.spq_dflags & SPQ_PRINTED))
							   || (onlyp == ONLYPRINTED  &&  q.spq_dflags & SPQ_PRINTED))  &&
			(classcode & q.spq_class) != 0;
}

int		restrictdoc::visible(const spptr FAR &p)
{
	return	(printer.IsEmpty()  ||  qmatch(printer, p.spp_ptr)) &&
			(classcode & p.spp_class) != 0;		
}

void	restrictdoc::loadrestrict()
{
	spqopts	&opts = ((CSpqwApp*)AfxGetApp())->m_options.spq_options;
	printer = opts.Restrp;
	user = opts.Restru;
	title = opts.Restrt;
	onlyp = opts.Restrunp > 1? ONLYPRINTED: opts.Restrunp? ONLYUNPRINTED: ALLITEMS;
	jinclude = 1;
	classcode = opts.classcode;
}	

void	restrictdoc::saverestrict()
{
	spqopts	&opts = ((CSpqwApp*)AfxGetApp())->m_options.spq_options;
	opts.Restrp = printer;
	opts.Restru = user;
	opts.Restrt = title;
	opts.Restrunp = onlyp == ONLYPRINTED? 2: onlyp == ONLYUNPRINTED? 1: 0;
	opts.classcode = classcode;
}