#include "stdafx.h"
#include <ctype.h>
#include <string.h> 
#include <memory.h>
#include <limits.h>
#include "pages.h"
#include "clientif.h"
#include "xtini.h" 
#include "sprserv.h"

#if	_MSC_VER == 700
#define UNIXTODOSTIME	((70UL * 365UL + 68/4 + 1) * 24UL * 3600UL)
#else
#define	UNIXTODOSTIME	0
#endif

extern	UINT	dosubmitj(spq &, const char *, spropts &);
extern	classcode_t	hextoi(const char *);
extern	char	*hex_disp(const classcode_t);
extern	char	*translate_delim(const char *, unsigned &);

static	void	parsesprcmd(const char *cmd, int promptfirst = 0)
{   
	//  Initialise to the defaults we picked up
	
	CSprservApp	&ma = *((CSprservApp *)AfxGetApp());
	spq  resjob = ma.m_options.qparams;
	spropts	Opts = ma.m_options.spr_options;
	
	//  Process any command line arguments.
	
 nexta:
    
    while  (isspace(*cmd))
    	cmd++;
    
    if  (*cmd != '-' && *cmd != '/')
    	goto  endopts;
    
    ++cmd;
	while  (*cmd)  {
		switch  (*cmd++)  {
		default:
			AfxMessageBox(IDP_SJINVALIDARG, MB_OK|MB_ICONSTOP);
			return;
		case  ' ':
		case  '\t':
			goto  nexta;     
		case  'Q':
			promptfirst = 1;
			continue;
		case  's':
			resjob.spq_jflags |= SPQ_NOH;
			continue;
		case  'r':
			resjob.spq_jflags &= ~SPQ_NOH;
			continue;
		case  'q':
		    resjob.spq_jflags |= SPQ_RETN;
            continue;
        case  'z':
        	resjob.spq_jflags &= ~SPQ_RETN;
			continue;
		case  'w':
			resjob.spq_jflags |= SPQ_WRT;
			continue;
		case  'm':
			resjob.spq_jflags |= SPQ_MAIL;
			continue;
		case  'a':
			resjob.spq_jflags |= SPQ_MATTN;
	        continue;
	    case  'A':
	        resjob.spq_jflags |= SPQ_WATTN;
	        continue;
	    case  'b':
	        resjob.spq_jflags &= ~(SPQ_MATTN|SPQ_WATTN);
	        continue;
	    case  'x':
	        resjob.spq_jflags &= ~(SPQ_MAIL|SPQ_WRT);
	        continue;
  		case  'l':
  			resjob.spq_jflags |= SPQ_LOCALONLY;
  			continue;
  		case  'L':
  			resjob.spq_jflags &= ~SPQ_LOCALONLY;
  			continue;
        case  'i':
        	Opts.interpolate = 1;
        	continue;
        case  'I':
        	Opts.interpolate = 0;
        	continue;
        case  'v':
        	Opts.verbose = 1;		//  NB No toggle like Unix version
            continue;
        case  'V':
        	Opts.verbose = 0;		//  NB No toggle like Unix version
            continue;
        case  'B':
            Opts.textmode = 0;
            continue;
        case  'Z':
            Opts.textmode = 1;
            continue;
  
  		case  'c':
  		case  'C':
  		case  'p':
  		case  'f':      
		case  'P':
		case  'u':
		case  'h':
		case  'R':
		case  'F':
		case  't':
		case  'T':
		case  'n':
		case  'N':		  
		case  'd':
		case  'D':
		case  'j':
		case  'O':
		case  'Y':
			break;
		}
		
		//  We have a command which takes an arg.
		//  The arg may follow immediately or after spaces
		
		int	 cmdlet = cmd[-1];
		char	copyarg[256];
		int  ccnt = 0;
		
		if  (!*cmd)  {
			AfxMessageBox(IDP_SJMISSARG, MB_OK|MB_ICONSTOP);
			return;
		}
		while  (isspace(*cmd))
			cmd++;
		while  (*cmd  &&  !isspace(*cmd))  {
			if  (*cmd == '\'' || *cmd == '\"')  {
				int	 quote = *cmd++;
				while  (*cmd != quote)  {
					if  (!*cmd)  {
						AfxMessageBox(IDP_SJINVALIDARG, MB_OK|MB_ICONSTOP);
            			return;
            		}
					copyarg[ccnt++] = *cmd++;
				}
				cmd++;
			}
			else
				copyarg[ccnt++] = *cmd++;
		}
		copyarg[ccnt] = '\0';
		const  char  *arg = copyarg;
		switch  (cmdlet)  {
		default:
			AfxMessageBox(IDP_SJINVALIDARG, MB_OK|MB_ICONSTOP);
   			return;
  		case  'c':
		{
			unsigned	num = atoi(arg);
			if  (num > ma.m_mypriv.spu_cps)  {
				AfxMessageBox(IDP_SJINVALIDCPS);
				return;
			}
			resjob.spq_cps = num;
			goto  nexta;
		}
 		case  'C':
		{
			classcode_t  num = ma.m_mypriv.resultclass(hextoi(arg));
			if  (num == 0)  {
				AfxMessageBox(IDP_SJINVALIDCC, MB_OK|MB_ICONSTOP);
				return;
			}
			resjob.spq_class = num;
			goto  nexta;
		}
  		case  'p':
  		{
			unsigned  num = atoi(arg);
			if  (num <  ma.m_mypriv.spu_minp || num > ma.m_mypriv.spu_maxp)  {
				AfxMessageBox(IDP_SJINVALIDPRIO, MB_OK|MB_ICONSTOP);
				return;
			}
			resjob.spq_pri = num;
			goto  nexta;
		}
  		case  'f':      
			strncpy(resjob.spq_form, arg, MAXFORM);
			resjob.spq_form[MAXFORM] = '\0';
			goto  nexta;
		case  'P':
			strncpy(resjob.spq_ptr, arg, JPTRNAMESIZE);
			resjob.spq_ptr[JPTRNAMESIZE] = '\0';
			goto  nexta;
		case  'u':
			if  (arg[0] == '-' &&  arg[1] == '\0')
				strcpy(resjob.spq_puname, resjob.spq_uname);
			else
				strncpy(resjob.spq_puname, arg, UIDSIZE);
			goto  nexta;
		case  'h':
			strncpy(resjob.spq_file, arg, MAXTITLE);
			resjob.spq_file[MAXTITLE] = '\0';
			goto  nexta;
		case  'R':
		{
			long	start = 1L, nd = LONG_MAX;
            
			if  (*arg == '-')
				nd = atol(++arg);
			else  {
				start = atol(arg);
				while  (*++arg && *arg != '-')
					;
				if  (*arg == '-')
					arg++;
				if  (*arg)
					nd = atol(arg);
				else
					nd = LONG_MAX;
			}
			if  (start > nd  ||  start <= 0L)  {
				AfxMessageBox(IDP_SJINVALIDRANGE, MB_OK|MB_ICONSTOP);
				return;
			}

			//	Humans number pages at 1...
			//	(silly people).

			resjob.spq_start = start - 1L;
			resjob.spq_end = nd - 1L;
			goto  nexta;
		}
		
		case  'Y':
		{
			resjob.spq_pglim = 0;
			resjob.spq_dflags &= ~(SPQ_PGLIMIT|SPQ_ERRLIMIT);
			if  (*arg == '-')
				goto  nexta;
			if  (toupper(*arg) == 'N')
				arg++;
			else  if  (toupper(*arg) == 'E')  {
				resjob.spq_dflags |= SPQ_ERRLIMIT;
				arg++;
			}
			unsigned  num = 0;
			while  (isdigit(*arg))
				num = num * 10 + *arg++ - '0';
			if  (num < 32767)
				resjob.spq_pglim = num;
			if  (*arg == 'P')
				resjob.spq_dflags |= SPQ_PGLIMIT;
			if  (resjob.spq_pglim == 0)
				resjob.spq_dflags &= ~(SPQ_PGLIMIT|SPQ_ERRLIMIT);
			goto  nexta;	
		}			
			
		case  'F':
			strncpy(resjob.spq_flags, arg, MAXFLAGS);
			resjob.spq_flags[MAXFLAGS] = '\0';
			goto  nexta;

		case  't':
		{
			unsigned	num = atoi(arg);
			if  (num >= 32767)  {
				AfxMessageBox(IDP_SJINVALIDTIMEOUT, MB_OK|MB_ICONSTOP);
				return;
			}
			resjob.spq_ptimeout = num;
			goto  nexta;
		}	
		case  'T':
		{
			unsigned	num = atoi(arg);
			if  (num >= 32767)  {
				AfxMessageBox(IDP_SJINVALIDTIMEOUT, MB_OK|MB_ICONSTOP);
				return;
			}
			resjob.spq_nptimeout = num;
			goto  nexta;
		}
		case  'n':
		{       
        	if  (*arg == '-')  {
				resjob.spq_hold = 0l;
				goto  nexta;
			}

			if  (!isdigit(*arg))  {
	badtim:
				AfxMessageBox(IDP_SJINVALIDHOLD, MB_OK|MB_ICONSTOP);
				return;
			}

			int  num = 0, hours, mins, secs;
			do  num = num * 10 + *arg++ - '0';
			while  (isdigit(*arg));

			if  (*arg != ':')  {
				if  (*arg != '\0')
					goto  badtim;
				hours = 0;
				mins = num;
				secs = 0;
			}
			else  {
				hours = num;
				mins = 0;
				arg++;
				while  (isdigit(*arg))
					mins = mins * 10 + *arg++ - '0';
				secs = 0;
				if  (*arg == ':')  {
					arg++;
					while  (isdigit(*arg))
						secs = secs * 10 + *arg++ - '0';
				}
				if  (*arg != '\0'  ||  mins >= 60  ||  secs >= 60)
					goto  badtim;
			}
			resjob.spq_hold = (hours * 60 + mins) * 60 + secs;
			if  (ma.m_options.spq_options.abshold)
				resjob.spq_hold += time((time_t *) 0) - UNIXTODOSTIME;
			goto  nexta;
		}
		case  'N':		  
		{
			static	char	month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

			if  (*arg == '-')  {
				resjob.spq_hold = 0l;
				goto  nexta;
			}

			time_t  now = time((time_t *) 0);
			tm  *tp = localtime(&now);
			int  year = tp->tm_year, month = tp->tm_mon + 1, day = tp->tm_mday,
					hour = tp->tm_hour, min = tp->tm_min, sec = tp->tm_sec;

			if  (!isdigit(*arg))  {
			badtime:
				AfxMessageBox(IDP_SJINVALIDHOLD, MB_OK|MB_ICONSTOP);
				return;
			}

			int  num = 0;
			do	num = num * 10 + *arg++ - '0';
			while  (isdigit(*arg));

			if  (*arg == '/')  {			// It's a date I think
				arg++;
				if  (!isdigit(*arg))
					goto  badtime;
				int  num2 = 0;
				do	num2 = num2 * 10 + *arg++ - '0';
				while  (isdigit(*arg));

				if  (*arg == '/')  {		// First digits were year
					if  (num > 1900)
						year = num - 1900;
					else  if  (num > 110)
						goto  badtime;
 					else  if  (num < 90)
						year = num + 100;
					else
						year = num;
					arg++;
					if  (!isdigit(*arg))
						goto  badtime;
					month = num2;
					day = 0;
					do	day = day * 10 + *arg++ - '0';
					while  (isdigit(*arg));
				}
				else  {
					//	Day/month or Month/day
					//	Decide by which side of the Atlantic

					if  (_timezone > 4 * 60 * 60)  {
						month = num;
						day = num2;
					}
					else  {
						month = num2;
						day = num;
					}

					if  (month < tp->tm_mon + 1  ||  (month == tp->tm_mon + 1  &&  day < tp->tm_mday))
						year++;
				}

				if  (*arg == '\0')
					goto  finish;
				if  (*arg != ',')
					goto  badtime;
				arg++;
				if  (!isdigit(*arg))
					goto  badtime;
				hour = 0;
				do	hour = hour * 10 + *arg++ - '0';
				while  (isdigit(*arg));
				if  (*arg != ':')
					goto  badtime;
				arg++;
				if  (!isdigit(*arg))
					goto  badtime;
				min = 0;
				do	min = min * 10 + *arg++ - '0';
				while  (isdigit(*arg));
				sec = 0;
				if  (*arg != ':')  {
					if  (*arg != '\0')
						goto  badtime;
				}
				else  {
					arg++;
					do	sec = sec * 10 + *arg++ - '0';
					while  (isdigit(*arg));
				}
			}
			else  {

				//	If tomorrow advance date

				hour = num;
				if  (*arg != ':')
					goto  badtime;
				arg++;
				if  (!isdigit(*arg))
					goto  badtime;
				min = 0;
				do	min = min * 10 + *arg++ - '0';
				while  (isdigit(*arg));

				sec = 0;
				if  (*arg != ':')  {
					if  (*arg != '\0')
						goto  badtime;
				}
				else  {
					arg++;
					do	sec = sec * 10 + *arg++ - '0';
					while  (isdigit(*arg));
				}

				if  (hour < tp->tm_hour  ||  (hour == tp->tm_hour && min <= tp->tm_min))  {
					day++;
					month_days[1] = year % 4 == 0? 29: 28;
					if  (day > month_days[month-1])  {
						day = 1;
						if  (++month > 12)  {
							month = 1;
							year++;
						}
					}
				}
			}

			if  (*arg != '\0')
				goto  badtime;

	 finish:
			if  (month > 12 || hour > 23 || min > 59 || sec > 59)
				goto  badtime;

			month_days[1] = year % 4 == 0? 29: 28;
			month--;
			year -= 70;
			if  (day > month_days[month])
				goto  badtime;

			time_t  result = year * 365;
			if  (year > 2)
				result += (year + 1) / 4;

			for  (int  i = 0;  i < month;  i++)
				result += month_days[i];
			result = (result + day - 1) * 24;

			//	Build it up once as at 12 noon and work out timezone
			//	shift from that

			time_t  testit = (result + 12) * 60 * 60 + UNIXTODOSTIME;
			tp = localtime(&testit);
			result = ((result + hour + 12 - tp->tm_hour) * 60 + min) * 60 + sec;

			if  (result + UNIXTODOSTIME <= now)
				result = 0L;
			else  if  (!ma.m_options.spq_options.abshold)
				result -= now - UNIXTODOSTIME;
			resjob.spq_hold = result;
			goto  nexta;
		}
		case  'd':
		{
			Opts.pfe.delimnum = atoi(arg);
			if  (Opts.pfe.delimnum <= 0)  {
				AfxMessageBox(IDP_SJINVALIDDELIM, MB_OK|MB_ICONSTOP);
				return;
			}
			goto  nexta;
		}
		case  'D':
		{
			unsigned  deliml;
			char	*newdelim = translate_delim(arg, deliml);
	
			if  (!newdelim)  {
				AfxMessageBox(IDP_SJINVALIDDELIM, MB_OK|MB_ICONSTOP);
				return;
			}

			delete [] Opts.delimiter;
			Opts.delimiter = newdelim;
			Opts.pfe.deliml = deliml;
			goto  nexta;
		}
		case  'j':
			goto  nexta;
		case  'O':
			switch  (*arg)  {
			default:
				AfxMessageBox(IDP_SJINVALIDOE, MB_OK|MB_ICONSTOP);
				return;
			case  '-':
				resjob.spq_jflags &= ~(SPQ_EVENP|SPQ_ODDP|SPQ_REVOE);
				goto  nexta;
			case  'o':case  'O':
				resjob.spq_jflags &= ~(SPQ_EVENP|SPQ_REVOE);
				resjob.spq_jflags |= SPQ_ODDP;
				break;
			case  'e':case  'E':
				resjob.spq_jflags &= ~(SPQ_ODDP|SPQ_REVOE);
				resjob.spq_jflags |= SPQ_EVENP;
				break;
			case  'a':case  'A':
				resjob.spq_jflags &= ~SPQ_ODDP;
				resjob.spq_jflags |= SPQ_EVENP|SPQ_REVOE;
				break;
			case  'b':case  'B':
				resjob.spq_jflags &= ~SPQ_EVENP;
				resjob.spq_jflags |= SPQ_ODDP|SPQ_REVOE;
				break;
			}
		}                 
	}

endopts:

	if  (!*cmd)  {
		AfxMessageBox(IDP_NOCMDFILES, MB_OK|MB_ICONSTOP);
		return;
	}
	do  {                           
		int		ccnt = 0;
		UINT	ret;
		char	filename[_MAX_PATH];		
		while  (*cmd  && *cmd != '\r' && *cmd != '\n' && *cmd != ('Z' & 0x1F))
			filename[ccnt++] = *cmd++;
		filename[ccnt] = '\0';
		if  (promptfirst)  {
			CString	pf;
			pf.LoadString(IDP_OKTOSUBMIT);
			pf += ' ';
			pf += filename;
			if  (AfxMessageBox(pf, MB_YESNO|MB_ICONQUESTION, IDP_OKTOSUBMIT) != IDYES)
				goto  missit;
		}
		ret = dosubmitj(resjob, filename, Opts);
		if  (ret)
			AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
	missit:
		while  (isspace(*cmd))
			cmd++;
	}  while  (*cmd && *cmd != '\r' && *cmd != '\n' && *cmd != ('Z' & 0x1F));
}		

void	parsecmdfile(const char *filename, const int promptfirst = 0)
{
	TRY	{
		CString	fname = filename;
		CFile	cmdfile(fname, CFile::modeRead);
		char	*buff = new char [cmdfile.GetLength() + 1];
		if  (!buff)  {
			AfxMessageBox(IDP_FILETOOBIG, MB_OK|MB_ICONSTOP);
			return;
		}
		UINT	lng = cmdfile.Read(buff, UINT(cmdfile.GetLength()));
		cmdfile.Close();
		while  (lng > 0  &&  buff[lng-1] == '\n' || buff[lng-1] == '\r' || buff[lng-1] == ' ')
			lng--;
		buff[lng] = '\0';
		if  (lng < 4  || _strnicmp(buff, "spr ", 4) != 0)  {
			delete [] buff;
			AfxMessageBox(IDP_NOTSAVEDJOB, MB_OK|MB_ICONSTOP);
			return;
		}
		char  *arglist = buff + 4;
		while  (isspace(*arglist))
			arglist++;
		parsesprcmd(arglist, promptfirst);
		delete [] buff;
		return;
	}
	CATCH(CException, e)
	{
		AfxMessageBox(IDP_CANNOTOPENFILE, MB_OK|MB_ICONSTOP);
		return;
	}
	END_CATCH
}

void	parsecmdline(const char *cmdline)
{   
	//  Take -Q or /Q before file name to mean prompt first
	
	int  promptfirst = 0;
	if  ((*cmdline == '-' || *cmdline == '/') && cmdline[1] == 'Q')  {
		promptfirst = 1;
		cmdline += 2;
		while  (isspace(*cmdline))
			cmdline++;
	}
	
	//  Have a look at first argument.
	//  If it's of form xyz.bat or xyz.xtc, then go for parsecmdfile.
	//  Otherwise go for parsesprcmd.

	//	const char *sp = strchr(cmdline, ' ');
	const char *cp = strchr(cmdline, '.');
	
	//  If no dot, or dot comes after the first space, skip it.
	
	//	if  (!cp || (sp && cp > sp))  {
	//	parsesprcmd(cmdline);
	//	return;
	//}
	if  (stricmp(cp+1, "xtc") == 0  || stricmp(cp+1, "bat") == 0)  {
		parsecmdfile(cmdline, promptfirst);
		return;
	}
	parsesprcmd(cmdline);
}