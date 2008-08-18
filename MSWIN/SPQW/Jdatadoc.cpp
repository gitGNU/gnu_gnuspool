// jdatadoc.cpp : implementation file
//

#include "stdafx.h"
#include "formatcode.h"
#include "spqw.h"                   
#include "jdatadoc.h"
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJdatadoc

IMPLEMENT_DYNCREATE(CJdatadoc, CDocument)

SOCKET	net_feed(const int type, const spq  &job)
{
	SOCKET	sock;
	sockaddr_in  sin;

	if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)  {
		AfxMessageBox(IDP_CCFEEDSOCKET, MB_OK|MB_ICONEXCLAMATION);
		return  INVALID_SOCKET;                                   
	}	

	//	Set up bits and pieces.

	sin.sin_family = AF_INET;
	sin.sin_port = Locparams.vportnum;
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = job.spq_netid;

	if  (connect(sock, (sockaddr FAR *) &sin, sizeof(sin)) != 0)  {
		closesocket(sock);
		AfxMessageBox(IDP_CCOFEEDSOCKET, MB_OK|MB_ICONEXCLAMATION);
		return  INVALID_SOCKET;
	}

	//	Send out initial packet saying what we want.
	//	In fact the job slot number doesn't matter except
	//	for paged jobs.

	feeder	fd(type, htonl(job.spq_rslot), htonl(job.spq_job));
	if  (send(sock, (char FAR *) &fd, sizeof(fd), 0) != sizeof(fd))	{
		closesocket(sock);
		AfxMessageBox(IDP_CSFEEDSOCKET, MB_OK|MB_ICONEXCLAMATION);
		return  INVALID_SOCKET;
	}
	return  sock;
}
                              
//  Read and join up boundaries
                              
int	net_if_read(const SOCKET  sk, char FAR *buffer, const int length)
{
	int  nbytes, toread = length;
                           
	while  ((nbytes = recv(sk, buffer, toread, 0)) > 0)  {
		if  (nbytes == toread)
			return	length;
		buffer += nbytes;
		toread -= nbytes;
	}
	return	nbytes;
}	

static	UINT	charsize(char ch, UINT  chcount)
{
	if  (ch == '\t')
		return  4 - (chcount & 3);
	if  (ch == '\r')
		return  0;
	if  (ch & 0x80)  {
		ch &= ~0x80;
		return  isprint(ch)? 3: 4;
	}
	else
		return  isprint(ch)? 1: 2;
}
                                      
CJdatadoc::CJdatadoc()
{
	pagebreaks = NULL;
	charwidth = charheight = 0;
	changes = 0;
	m_invalid = m_donttrustpages = FALSE;
}

CJdatadoc::~CJdatadoc()
{
	if  (pagebreaks)
		delete [] pagebreaks;
	if  (!tname.IsEmpty())  {
		jobfile.Close();
		remove(tname);
	}	
	if  (changes  &&  !(m_invalid || m_donttrustpages))
		Jobs().pageupdate(jcopy);		
}
   
void	CJdatadoc::DeleteContents()
{
	if  (pagebreaks)  {
		delete [] pagebreaks;
		pagebreaks = NULL;
	}
}

int		CJdatadoc::loaddoc()
{   
	SOCKET	sk = net_feed(FEED_NPSP, jcopy);
	if  (sk == INVALID_SOCKET)
		return  0;

	char	title[80];
	char	*fil = jcopy.spq_file;
	if  (fil[0])  {
		wsprintf(title, "Job %s:%ld:", (char FAR *) look_host(jcopy.spq_netid), jcopy.spq_job);
		strcat(title, fil);
	}	
	else
		wsprintf(title, "Job %s:%ld (untitled)", (char FAR *) look_host(jcopy.spq_netid), jcopy.spq_job);
	SetTitle(title);

	char	*tn = _tempnam("C:\\XITEXT", "JDAT");
	if  (!tn)  {
		closesocket(sk);
		AfxMessageBox(IDP_CCTEMPFILE, MB_OK|MB_ICONEXCLAMATION);
		return  0;      
	}	
	tname = tn;
	if  (!jobfile.Open(tname, CFile::modeCreate | CFile::modeReadWrite))  {
		closesocket(sk);
		AfxMessageBox(IDP_CCTEMPFILE, MB_OK|MB_ICONEXCLAMATION);
		return  0;      
	}	

	//  Slurp up file from network and write to file
		
	char	buffer[256];
	int		nbytes;
	
	TRY {
		while  ((nbytes = recv(sk, buffer, sizeof(buffer), 0)) > 0)
			jobfile.Write(buffer, UINT(nbytes));
	}
	CATCH(CException, e)
	{
		closesocket(sk);
		AfxMessageBox(IDP_CWTEMPFILE, MB_OK|MB_ICONEXCLAMATION);
		return  0;
	}
	END_CATCH
	
	// Finished with network (unless we want a page file)
	
	closesocket(sk);
	
	// Allocate ourselves a number of pages + 2 for pagebreaks
	
	pagebreaks = new pagemarker [jcopy.spq_npages + 2];
	if  (!pagebreaks)  {
		AfxMessageBox(IDP_NOMEMFORPAGES, MB_OK|MB_ICONEXCLAMATION);
		return  0;
	}

	if  (jcopy.spq_dflags & SPQ_PAGEFILE)  {
	
	  	//  Page file to read
	  	
	  	if  ((sk = net_feed(FEED_PF, jcopy)) == INVALID_SOCKET)  {
	  		m_donttrustpages = TRUE;
	  		return  scanbyformfeeds(buffer, sizeof(buffer));
	  	}          
	  	
	  	//  Read pages descriptor. This is only to discover how
	  	//  big the delimiter is (to avoid reading that)
	  	
		pages  pfep;	  		                        
		if  (net_if_read(sk, (char FAR *) &pfep, sizeof(pfep)) < 0)  {
		giveup:
			m_donttrustpages = TRUE;
			closesocket(sk);
			return  scanbyformfeeds(buffer, sizeof(buffer));
		}	
        
        //  Skip over delimiter.
        
		long  toread = ntohl(pfep.deliml);
		do  {
			if  ((nbytes = recv(sk, buffer, int(toread > sizeof(buffer)? sizeof(buffer): toread), 0)) <= 0)
				goto  giveup;
		}  while  ((toread -= nbytes) > 0);
        
        //  Initialise first entry in vector to start of document.
        
		pagebreaks[0].pageoffset = 0L;
		pagebreaks[0].linecount = 0;

		//	Read in vector of page offsets starting at 1

		for  (unsigned long  cnt = 1;  cnt <= jcopy.spq_npages;  cnt++)  {
			if  (net_if_read(sk, (char FAR *) &pagebreaks[cnt].pageoffset, sizeof(long)) < 0)
				goto  giveup;                                                                
			pagebreaks[cnt].pageoffset = ntohl(pagebreaks[cnt].pageoffset);
		}	
		closesocket(sk);	//  Byebye nice knowing you
		
		//  Now slurp our way through the file assigning line counts
		//  and working out the width of the longest line
		
		jobfile.SeekToBegin();
		long		pagenum = 1, charcount = 0;
		charwidth = charheight = 0;		//  The members
		unsigned	nlcount = 0, cwidth = 0;

		while  ((nbytes = jobfile.Read(buffer, sizeof(buffer))) > 0)
			for  (int  nc = 0;  nc < nbytes;  nc++)  {
				if  (charcount >= pagebreaks[pagenum].pageoffset)
					pagebreaks[pagenum++].linecount = nlcount;
				charcount++;
			   	if  (buffer[nc] == '\n')  {
			   		charheight++;
			   		nlcount++;
			   		if  (charwidth < cwidth)
		   				charwidth = cwidth;
		   			cwidth = 0;
			    }
			    else
			    	cwidth += charsize(buffer[nc], cwidth);
		    }
		pagebreaks[pagenum].pageoffset = charcount;
		pagebreaks[pagenum].linecount = nlcount;
		return  1;
	}
	else
		return  scanbyformfeeds(buffer, sizeof(buffer));
}

//  If no page file, scan by form feeds.
//  We may not be able to believe the page counts set in which case
//  we set m_donttrustpages so that setting page markers is suppressed

int	CJdatadoc::scanbyformfeeds(char *buffer, UINT bufsiz)
{
	int		nbytes;
	unsigned		nlcount = 0;
	unsigned  long  pagenum = 1;
	charwidth = charheight = 0;		//  The members
	unsigned	cwidth = 0;

    //  Scan it once to get number of pages
    
	jobfile.SeekToBegin();
	while  ((nbytes = jobfile.Read(buffer, bufsiz)) > 0)
		for  (int  nc = 0;  nc < nbytes;  nc++)  {
			int	 ch = buffer[nc];
			if  (ch == '\f')  {
				nlcount = 0;
				pagenum++;
				cwidth += 2;
			}	
		   	else  if  (ch == '\n')  {
		   		nlcount++;
		   		charheight++;
		   		if  (charwidth < cwidth)
					charwidth = cwidth;
				cwidth = 0;
		    }
		    else
		    	cwidth += charsize(ch, cwidth);
		}

	if  (pagenum != jcopy.spq_npages)  {
		m_donttrustpages = TRUE;
		delete [] pagebreaks;
		jcopy.spq_npages = pagenum;
		pagebreaks = new pagemarker [pagenum + 2];
		if  (!pagebreaks)  {
			AfxMessageBox(IDP_NOMEMFORPAGES, MB_OK|MB_ICONEXCLAMATION);
			return  0;
		}
	}

	if  (nlcount != 0)
		pagenum++;
	
	jobfile.SeekToBegin();              
	pagenum = 1;         
	nlcount = 0;
	long  charcount = 0L;                     
	pagebreaks[0].pageoffset = 0L;
	pagebreaks[0].linecount = 0L;
	while  ((nbytes = jobfile.Read(buffer, sizeof(buffer))) > 0)
		for  (int  nc = 0;  nc < nbytes;  nc++)  {
			if  (buffer[nc] == '\f')  {
				pagebreaks[pagenum].pageoffset = charcount;
				pagebreaks[pagenum].linecount = nlcount;
				pagenum++;
			}
			else  if  (buffer[nc] == '\n')
				nlcount++;
			charcount++;
		}			
	pagebreaks[pagenum].pageoffset = charcount;
	pagebreaks[pagenum].linecount = nlcount;
	return  1;	
}

void	CJdatadoc::setpage(unsigned long spq::*w, const long page)
{
	if  ((unsigned long) page != jcopy.*w)  {
		jcopy.*w = page;
		UpdateAllViews(NULL, JD_LHINT_PAGES);
		changes++;
	}	
}

//  Locate the given row

long	CJdatadoc::Locrow(const int row)
{
	for  (unsigned  cnt = 0;  cnt < jcopy.spq_npages;  cnt++)
		if  (pagebreaks[cnt+1].linecount > unsigned(row))  {
			jobfile.Seek(pagebreaks[cnt].pageoffset, CFile::begin);
			char  buffer[100];
			UINT  nbytes;
			int	  frow = pagebreaks[cnt].linecount;
			long  splace = pagebreaks[cnt].pageoffset;
			while  (frow < row)  {
				nbytes = jobfile.Read(buffer, sizeof(buffer));
				for  (UINT nc = 0;  nc < nbytes;  nc++)  {
					if  (buffer[nc] == '\n')  {
						frow++;
						if  (frow >= row)
							return splace + nc + 1;
					}	
				}
				splace += nbytes;
			}
			return  splace;
		}	
	
	return  -1L;
}

UINT	CJdatadoc::RowLength(const long splace)
{
	jobfile.Seek(splace, CFile::begin);
	UINT	chcount = 0;
	for  (;;)  {
		char	buffer[100];
		int  nbytes = jobfile.Read(buffer, sizeof(buffer));
		for  (int  nc = 0;  nc < nbytes;  nc++)
			if  (buffer[nc] == '\n')
				return  chcount + 1;
			else
				chcount += charsize(buffer[nc], chcount);
	}
}	

char	*CJdatadoc::FindRow(const int row)
{
	long	splace = Locrow(row);
	if  (splace < 0)
		return  NULL;	
	UINT	chcount = RowLength(splace);
	char  *result = new char [chcount];
	if  (!result)
		return  NULL;
	jobfile.Seek(splace, CFile::begin);
	UINT  pos = 0;
	for  (;;)  {         
		char  buffer[100];
		int  nbytes = jobfile.Read(buffer, sizeof(buffer));
		if  (nbytes <= 0)  {
			result[pos] = '\0';
			return  result;
		}
		for  (int  nc = 0;  nc < nbytes;  nc++)
			if  (buffer[nc] == '\n')  {
				result[pos] = '\0';
				return  result;
			}
			else  if  (buffer[nc] != '\r')
				result[pos++] = buffer[nc];					               
	}	
}

char	*CJdatadoc::GetRow(const int row, char *result)
{
	long	splace = Locrow(row);
	if  (splace < 0)
		return  NULL;	
	jobfile.Seek(splace, CFile::begin);
	UINT  pos = 0;
	for  (;;)  {
		char  buffer[100];
		int  nbytes = jobfile.Read(buffer, sizeof(buffer));
		for  (int  nc = 0;  nc < nbytes;  nc++)  {
			int  ch = buffer[nc];			
			if  (ch == '\n')  {
				result[pos] = '\0';
				return  result;
			}
			else  if  (ch == '\t')
				do  result[pos] = ' ';
				while  (++pos & 3);
			else  if  (ch == '\r')
				continue;
			else  {
				if  (ch & 0x80)  {
					ch &= 0x7f;
					result[pos++] = 'M';
					result[pos++] = '-';
				}
				if  (!isprint(ch))  {
					result[pos++] = '^';
					if  (ch < ' ')
						result[pos++] = char(ch + '@');
					else
						result[pos++] = '?';
				}
				else
					result[pos++] = ch;
			}
		}	
	}			
}					

unsigned	CJdatadoc::whatpage(const int row)
{
	for  (unsigned cnt = 0;  cnt < jcopy.spq_npages;  cnt++)
		if  (pagebreaks[cnt+1].linecount > unsigned(row))
			return  cnt;
	return  unsigned(jcopy.spq_npages);
}

BEGIN_MESSAGE_MAP(CJdatadoc, CDocument)
	//{{AFX_MSG_MAP(CJdatadoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

