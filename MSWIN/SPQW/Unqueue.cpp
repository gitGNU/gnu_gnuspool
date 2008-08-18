// unqueue.cpp : implementation file
//

#include "stdafx.h"
#include <ctype.h>
#include <time.h>
#include <iostream.h>
#include <fstream.h>
#include <direct.h>
#include "jobdoc.h"
#include "mainfrm.h"
#include "spqw.h"
#include "files.h"

#define	LOTSANDLOTS	99999999L

// Defined in JDatadoc.cpp

extern	SOCKET	net_feed(const int, const spq &);
extern	int		net_if_read(const SOCKET, char FAR *, const int);

static	CString	resultcmd;
ifstream	Cfile;

extern	char	FAR	basedir[];

static	void	soakrest(const SOCKET sk)
{
	char	buffer[256];
	while  (recv(sk, buffer, sizeof(buffer), 0) > 0)
		;
}

static	void	getdelimiter(const spq &jb, CString &del, unsigned &num)
{
	if  (!(jb.spq_dflags & SPQ_PAGEFILE))  {
	giveup:
		del = "\\f";
		num = 1;
		return;
	}
	SOCKET	sk;
	if  ((sk = net_feed(FEED_PF, jb)) == INVALID_SOCKET)
		goto  giveup;

	pages  pfep;	  		                        
	if  (net_if_read(sk, (char FAR *) &pfep, sizeof(pfep)) < 0)  {
	giveup2:
		closesocket(sk);
		goto  giveup;   
	}

	unsigned  delimsize = unsigned(ntohl(pfep.deliml));
	if  (delimsize == 0)
		goto  giveup2;
	num = unsigned(ntohl(pfep.delimnum));
	char  *delimin = new char [delimsize];
	if  (!delimin)
		goto  giveup2;
	
	if  (net_if_read(sk, (char FAR *) delimin, delimsize) < 0)  {
		delete [] delimin;
		goto  giveup2;
	}
	soakrest(sk);
	closesocket(sk);	//  Byebye nice knowing you
	
	//  Untranslate delimiter
	
	del = '\"';

	const char	hexchs[] = "0123456789abcdef";
	
	for  (unsigned char  *cp = (unsigned char *) delimin;  *cp;  cp++)  {
		switch  (*cp)  {
		default:
			if  (*cp >= ' ' && *cp <= '~')  {
				del += char(*cp);                  
				continue;
			}
			else  if  (*cp < ' ')  {
				del += '^';
				del += char(*cp);
				continue;
			}	
			del += "\\x";
			del += hexchs[(*cp >> 4) & 0x0f];
			del += hexchs[*cp & 0x0f];
			break;
		case  '\033':
			del += "\\e";
			break;
		case  'h' & 0x1f:
			del += "\\b";
			break;
		case  '\r':
			del += "\\r";
			break;
		case  '\n':
			del += "\\n";
			break;
		case  '\f':
			del += "\\f";
			break;
		case  '\t':
			del += "\\t";
			break;
		case  '\v':
			del += "\\t";
			break;
		}
	}
	del += '\"';
	delete [] delimin;
}

inline	void	appendlong(const long item)
{
	char	thing[24];
	wsprintf(thing, "%ld", item);
	resultcmd += thing;
}

static	void	dumphdrs(const spq &jb)
{
	resultcmd += "-B";	//  BINARY
	resultcmd += jb.spq_jflags & SPQ_NOH? 's': 'r';
	if  ((jb.spq_jflags & (SPQ_WRT|SPQ_MAIL)) != (SPQ_WRT|SPQ_MAIL))
		resultcmd += 'x';
	if  (jb.spq_jflags & SPQ_WRT)
		resultcmd += 'w';
	if  (jb.spq_jflags & SPQ_MAIL)
		resultcmd += 'm';
	
	if  ((jb.spq_jflags & (SPQ_WATTN|SPQ_MATTN)) != (SPQ_WATTN|SPQ_MATTN))
		resultcmd += 'b';
	if  (jb.spq_jflags & SPQ_WATTN)
		resultcmd += 'A';
	if  (jb.spq_jflags & SPQ_MATTN)
		resultcmd += 'a';

	resultcmd += jb.spq_jflags & SPQ_RETN? 'q': 'z';
	resultcmd += jb.spq_jflags & SPQ_LOCALONLY? 'l': 'L';

	resultcmd += " -c";
	appendlong(long(jb.spq_cps));
	if  (jb.spq_file[0])
		resultcmd += " -h\'" + CString(jb.spq_file) + '\'';
	
	resultcmd += " -f\'" + CString(jb.spq_form) + '\'';	
	
	if  (jb.spq_ptr[0])
		resultcmd += " -P" + CString(jb.spq_ptr);

	if  (jb.spq_puname[0] && strcmp(jb.spq_uname, jb.spq_puname) != 0)
		resultcmd += " -u" + CString(jb.spq_puname);
	
	resultcmd += " -p";
	appendlong(long(jb.spq_pri));
	resultcmd += " -C";
	if  (jb.spq_class == 0)
		resultcmd += '.';
	else  {
		for  (int i = 0;  i < 16;  i++)
			if  ((jb.spq_class & (1L << i)))
				resultcmd += char('A' + i);
		for  (i = 0;  i < 16;  i++)
			if  (jb.spq_class & (1L << (i+16)))
				resultcmd += char('a' + i);
	}
	if  (jb.spq_flags[0])
		resultcmd += " -F'\'" + CString(jb.spq_flags) + '\'';

	if  (jb.spq_start != 0L  &&  jb.spq_end <= LOTSANDLOTS)  {
		resultcmd += " -R";
		if  (jb.spq_start != 0L)
			appendlong(jb.spq_start+1L);
		resultcmd += '-';
		if  (jb.spq_end <= LOTSANDLOTS)
			appendlong(jb.spq_end+1L);
	}
	if  (jb.spq_jflags & (SPQ_ODDP|SPQ_EVENP))  {
		resultcmd += " -O";
		if  (jb.spq_jflags & SPQ_REVOE)
			resultcmd += jb.spq_jflags & SPQ_ODDP? 'A': 'B';
		else
			resultcmd += jb.spq_jflags & SPQ_ODDP? 'O': 'E';
    }
    resultcmd += " -t";
    appendlong(jb.spq_ptimeout);
    resultcmd += " -T";
    appendlong(jb.spq_nptimeout);
	long	toffs = jb.spq_hold - time(NULL);
    if  (toffs > 0)  {
		if  (((CSpqwApp *)(AfxGetApp()))->m_options.spq_options.abshold)  {
    		resultcmd += " -N";
    		tm	*tp = localtime(&jb.spq_hold);
    		char	obuf[36];
    		wsprintf(obuf, "%.2d/%.2d/%.2d,%.2d:%.2d:%.2d",
    				tp->tm_year%100, tp->tm_mon+1, tp->tm_mday,
    				tp->tm_hour, tp->tm_min, tp->tm_sec);
    		resultcmd += obuf;
		}
		else  {
			resultcmd += " -n";
			appendlong(toffs / 3600L);
			resultcmd += ':';
			toffs %= 3600L;
			appendlong(toffs / 60L);
			resultcmd += ':';
			toffs %= 60L;
			appendlong(toffs);
		}
    }
    CString	delim;
    unsigned  delimnum;
	getdelimiter(jb, delim, delimnum); 
    resultcmd += " -d";
    appendlong(delimnum);
    resultcmd += " -D";
    resultcmd += delim;
}

static	BOOL	dumpjob(CString &filename, CString &jobname, const spq &jb)
{
	resultcmd = "spr ";
	dumphdrs(jb);
	resultcmd += ' ';
	resultcmd += jobname;
	resultcmd += "\r\n";
	resultcmd += char('Z' & 0x1f);
	TRY {
		CFile bfile(filename, CFile::modeCreate | CFile::modeWrite);
		bfile.Write((const char *) resultcmd, resultcmd.GetLength());
	}
	CATCH(CException, e)
	{
		AfxMessageBox(IDP_CWBATFILE, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}
	END_CATCH
	resultcmd.Empty();
	return  TRUE;
}

void	spq::unqueue(const BOOL and_delete)
{
	CFileDialog	jfn(FALSE,
					"xtj",
					NULL,
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,
					"Xitext job files (*.xtj) | *.xtj | All Files (*.*) | *.* ||");
	
	spqopts	 &qopts = ((CSpqwApp *) AfxGetApp())->m_options.spq_options;

	CFileDialog	qfn(FALSE,
					qopts.batext? "bat": "xtc",
					NULL,
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,
					qopts.batext? "DOS-Style Batch files (*.bat) | *.bat | All Files (*.*) | *.* ||":
					"Xitext command files (*.xtc) | *.xtc | All Files (*.*) | *.* ||");
	
	jfn.SetHelpID(IDD_JOBFILE);
	
	if  (jfn.DoModal() != IDOK)
		return;
		
	CString	outfile = jfn.GetPathName();

	//  Obtain file over network
		
	SOCKET	sk = net_feed(FEED_NPSP, *this);
	if  (sk == INVALID_SOCKET)
		return;
		
	CFile	jobfile;

	if  (!jobfile.Open((const char *) outfile, CFile::modeCreate | CFile::modeWrite))  {
		closesocket(sk);
		AfxMessageBox(IDP_CCJOBQFILE, MB_OK|MB_ICONEXCLAMATION);
		return;      
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
		AfxMessageBox(IDP_CWJOBQFILE, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	END_CATCH
	// Finished with network
	closesocket(sk);
	jobfile.Close();
	
	qfn.SetHelpID(IDD_BATFILE);
	if  (qfn.DoModal() != IDOK)  {
	zapit:
		TRY {
			CFile::Remove((const char *) outfile);
		}
		CATCH(CException, e)
		{
			AfxMessageBox(IDP_CDJOBQFILE, MB_OK|MB_ICONSTOP);
		}
		END_CATCH
		return;                        
	}

	CString	batfile = qfn.GetPathName();
    
	if  (!dumpjob(batfile, outfile, *this))
		goto  zapit;
	if  (and_delete)
		Jobs().opjob(SO_AB, jident(this));
}

const  char poptname[] = "Program options";
const  char	qdefname[] = "Queue defaults";

inline	void	WritePrivateProfileInt(const char *section, const char *field, const long value, const char *file)
{
	char	obuf[24];
	
	wsprintf(obuf, "%ld", value);
	::WritePrivateProfileString(section, field, obuf, file);
}

inline	void	WritePrivateProfileHex(const char *section, const char *field, const unsigned long value, const char *file)
{
	char	obuf[12];
	
	wsprintf(obuf, "0x%lx", value);
	::WritePrivateProfileString(section, field, obuf, file);
}

inline	void	WritePrivateProfileBool(const char *section, const char *field, const int value, const char *file)
{
	WritePrivateProfileInt(section, field, value? 1: 0, file);
}

void	spq::optcopy()
{
	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, INIFILE);

	WritePrivateProfileBool(qdefname, "Nohdr", spq_jflags & SPQ_NOH, pfilepath);
	WritePrivateProfileBool(qdefname, "Retain", spq_jflags & SPQ_RETN, pfilepath);
	WritePrivateProfileBool(qdefname, "Localonly", spq_jflags & SPQ_LOCALONLY, pfilepath);
	WritePrivateProfileBool(qdefname, "Write", spq_jflags & SPQ_WRT, pfilepath);
	WritePrivateProfileBool(qdefname, "Mail", spq_jflags & SPQ_MAIL, pfilepath);
	WritePrivateProfileBool(qdefname, "Wattn", spq_jflags & SPQ_WATTN, pfilepath);
	WritePrivateProfileBool(qdefname, "Mattn", spq_jflags & SPQ_MATTN, pfilepath);
	WritePrivateProfileBool(qdefname, "Skipodd", spq_jflags & SPQ_ODDP, pfilepath);
	WritePrivateProfileBool(qdefname, "Skipeven", spq_jflags & SPQ_EVENP, pfilepath);
	WritePrivateProfileBool(qdefname, "Swapoe", spq_jflags & SPQ_REVOE, pfilepath);
	WritePrivateProfileInt(qdefname, "Copies", spq_cps, pfilepath);
	WritePrivateProfileInt(qdefname, "Priority", spq_pri, pfilepath);
	WritePrivateProfileHex(qdefname, "Qclass", spq_class, pfilepath);
	::WritePrivateProfileString(qdefname, "Form", spq_form, pfilepath);
	::WritePrivateProfileString(qdefname, "Printer", spq_ptr, pfilepath);
	::WritePrivateProfileString(qdefname, "Puser", spq_puname, pfilepath);
	::WritePrivateProfileString(qdefname, "Title", (const char *)('\"' + CString(spq_file) + '\"'), pfilepath);
	WritePrivateProfileInt(qdefname, "Spage", spq_start+1L, pfilepath);
	WritePrivateProfileInt(qdefname,
							  "Epage",
							  spq_end > LOTSANDLOTS? 0:
							  spq_end+1L, pfilepath);
	CString	delim;
	unsigned  delimnum;
	getdelimiter(*this, delim, delimnum);
	WritePrivateProfileInt(qdefname, "Delimnum", delimnum, pfilepath);
	::WritePrivateProfileString(qdefname, "Delimiter", (const char *) delim, pfilepath);

	::WritePrivateProfileString(qdefname, "PPflags", (const char *)('\"' + CString(spq_flags) + '\"'), pfilepath);
	WritePrivateProfileInt(qdefname, "Pdeltime", spq_ptimeout, pfilepath);
	WritePrivateProfileInt(qdefname, "NPdeltime", spq_nptimeout, pfilepath);
	WritePrivateProfileInt(qdefname, "NPdeltime", spq_nptimeout, pfilepath);
	long toffs = (((CSpqwApp *)(AfxGetApp()))->m_options.spq_options.abshold)?
		spq_hold - time(NULL): spq_hold - spq_time;
	WritePrivateProfileInt(qdefname, "Holdtime", toffs > 0? toffs: 0, pfilepath);
}
	
	
