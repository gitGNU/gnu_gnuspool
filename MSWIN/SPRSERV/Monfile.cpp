#include "stdafx.h"
#include "pages.h"
#include "monfile.h"               
#include "clientif.h"
#include "resource.h"
#include <string.h> 
#include <memory.h>
#include <limits.h>

extern	void	qinit(const char *, const char *, spq &, spq &, pages &, pages &, const char *);
extern	UINT	spew_header(spq &, pages &, const char *);
extern	UINT	spew_data(CFile &);
extern	UINT	spew_endjob();

IMPLEMENT_SERIAL(CMonFile, CObject, 1)

CMonFile::CMonFile(const char *file, const spq &qp, const pages &pp, const char *d, const unsigned mt) :mon_time(mt)
{
	strcpy(mon_filename, file);
	qparams = qp;
	delimdescr = pp;
	qparams.spq_dflags &= ~SPQ_PAGEFILE;
	if  (d && pp.deliml > 0)  {
		delimiter = new char [pp.deliml];
		memcpy(delimiter, d, unsigned(pp.deliml));
		if  (pp.deliml != 1 || d[0] != '\f')
			qparams.spq_dflags |= SPQ_PAGEFILE;
	}
	else
		delimiter = (char *) 0;        
	mon_notyet = TRUE;
}

CMonFile::CMonFile(const char *file, const CMonFile *orig, const unsigned mt) : mon_time(mt)
{
	strcpy(mon_filename, file);
	qparams = orig->qparams;
	delimdescr = orig->delimdescr;
	if  (orig->delimiter && delimdescr.deliml > 0)  {
		delimiter = new char [delimdescr.deliml];
		memcpy(delimiter, orig->delimiter, unsigned(delimdescr.deliml));
	}
	else
		delimiter = (char *) 0;
	mon_notyet = TRUE;
}
			
CMonFile::~CMonFile()
{
	if  (delimiter)
		delete [] delimiter;
}		

	 
unsigned	CMonFile::nexttime(CFile &fl) const
{                         
	if  (mon_notyet)
		return  UINT_MAX;
	if  (fl.Open(mon_filename, CFile::modeRead | CFile::shareDenyNone))  {
		if  (fl.GetLength() == 0)  {
			fl.Close();
			return  mon_time;
		}
		fl.Close();
		if  (fl.Open(mon_filename, CFile::modeRead | CFile::shareExclusive))
			return  0;
	}
	return  mon_time; 
}
                      

UINT	CMonFile::queuejob(const char *username, CFile &fl)
{
	UINT	ret;
	spq	packedj;
	pages	packedp;
	const char	*delimp = delimiter;
	qinit(username, mon_filename, qparams, packedj, delimdescr, packedp, delimp);
	if  (ret = spew_header(packedj, packedp, delimp))  {
		fl.Close();
		return  ret;
	}
	ret = spew_data(fl);
	fl.Close();
	if  (ret)
		return  ret;
	remove(mon_filename);
	return  spew_endjob();
}	

void	CMonFile::Serialize(CArchive &ar)
{
	CObject::Serialize(ar);
	if  (ar.IsStoring())  {
		ar << (WORD) mon_notyet << (WORD) mon_time << (CString) mon_filename;
		unsigned  char  *cp = (unsigned char *) &qparams;
		for  (unsigned  cnt = 0;  cnt < sizeof(spq);  cnt++)
			ar << (BYTE) *cp++;
		ar << (LONG) delimdescr.deliml << (LONG) delimdescr.delimnum;
		if  (delimdescr.deliml > 0)  {
			unsigned char *fcp = (unsigned char *) delimiter;
			for  (cnt = 0;  cnt < unsigned(delimdescr.deliml);  cnt++)
				ar << (BYTE) *fcp++;
		}
	}		
	else  {
		WORD  inw0;
		WORD  inw1;
		CString	ins1;
		ar >> inw0 >> inw1 >> ins1;
		mon_notyet = inw0;
		mon_time = inw1;
		strcpy(mon_filename, (const char *) ins1);
		unsigned  char  *cp = (unsigned char *) &qparams;
		for  (unsigned  cnt = 0;  cnt < sizeof(spq);  cnt++)  {
			BYTE	inb;
			ar >> inb;
			*cp++ = inb;
		}	
		LONG	l1, l2;
		ar >> l1 >> l2;
		delimdescr.deliml = l1;
		delimdescr.delimnum = l2;
		delimdescr.lastpage = 0L;
		if  (delimdescr.deliml > 0)  {
			delimiter = new char [delimdescr.deliml];
			unsigned char *fcp = (unsigned char *) delimiter;
			for  (cnt = 0;  cnt < unsigned(delimdescr.deliml);  cnt++)  {
				BYTE	inb;
				ar >> inb;
				*fcp++ = inb;
			}	
		}
	}
}		

IMPLEMENT_SERIAL(CMFList, CObject, 1)

CMFList::CMFList()
{
	list = NULL;
	num = max = lookingat = 0;
}

CMFList::~CMFList()
{
	if  (list)
		delete [] list;
}

void	CMFList::addfile(CMonFile *f)
{
	if  (num >= max)  {
		if  (max != 0)  {
			CMonFile	**oldlist = list;
			unsigned	oldmax = max;
			max += INC_LIST;
			list = new CMonFile * [max];
			memcpy((void *) list, (void *) oldlist, oldmax * sizeof(CMonFile *));
			delete [] oldlist;
		}
		else  {
			list = new CMonFile * [INIT_LIST];
			max = INIT_LIST;
		}
	}
	list[num++] = f;
}

void	CMFList::delfile(CMonFile *w)
{
	for  (unsigned  count = 0;  count < num;  count++)
		if  (list[count] == w)  {
			delfile(count);
			return;
		}
}

void	CMFList::delfile(const unsigned  w)
{
	if  (w >= num)
		return;
	--num;
	delete list[w];
	for  (unsigned  nxt = w;  nxt < num;  nxt++)
		list[nxt] = list[nxt+1];
}

void	CMFList::repfile(const unsigned w, CMonFile *replacement)
{
	if  (w >= num)
		return;
	delete list[w];
	list[w] = replacement;
}	
	
CMonFile	*CMFList::next()
{
	if  (lookingat < num)
		return  list[lookingat++];
	return  NULL;
}
	
void	CMFList::Serialize(CArchive &ar)
{
	CObject::Serialize(ar);
	if  (ar.IsStoring())  {
		ar << (WORD) num << (WORD) max;
		for  (unsigned  cnt = 0;  cnt < num;  cnt++)
			list[cnt]->Serialize(ar);
	}
	else  {
		WORD	inw1, inw2;
		ar >> inw1 >> inw2;
		num = inw1;
		max = inw2;
		if  (max != 0)
			list = new CMonFile * [max];
		for  (unsigned  cnt = 0;  cnt < num;  cnt++)  {
			list[cnt] = new CMonFile;
			list[cnt]->Serialize(ar);
		}
	}
}				