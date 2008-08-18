#include "stdafx.h"
#include <ctype.h>
#include <string.h> 
#include <memory.h>
#include <limits.h>
#include <iostream.h>
#include <fstream.h>
#include "pages.h"
#include "monfile.h"               
#include "clientif.h"
#include "xtini.h" 
#include "sprserv.h"

extern	sockaddr_in	serv_addr;

UINT	xmit(char *msg, const int lng)
{
	char	reply[1];

	if  (sendto(Locparams.uasocket, msg, lng, 0, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		return	IDP_JSENDFAIL;
	if  (recvfrom(Locparams.uasocket, reply, sizeof(reply), 0, (sockaddr *) 0, (int *) 0) < 0)
		return	IDP_JRECVFAIL;
	if  (reply[0] != XTNQ_OK)  {
		if  (reply[0] == XTNR_WARN_LIMIT)  {
			AfxMessageBox(IDP_WARNLIMIT, MB_OK|MB_ICONEXCLAMATION);
			return  0;
		}
		return  IDP_REJECTED + reply[0] - 1;
	}
	return  0;
}

void	qinit(const char *username, const char *filename, spq &srcjob, spq &destjob, pages &srcp, pages &destp, const char *delimiter)
{
	srcjob.spq_dflags &= ~SPQ_PAGEFILE;
	if  (srcp.deliml != 1 || delimiter[0] != '\f')
			srcjob.spq_dflags |= SPQ_PAGEFILE;
	destjob.spq_job = htonl(long(srcjob.spq_job));
	destjob.spq_netid = 0L;
	destjob.spq_rslot = 0L;
	destjob.spq_pslot = htonl(-1L);
	destjob.spq_time = htonl(long(srcjob.spq_time));
	long	dtime = 0L;
	if  (srcjob.spq_hold)  {
		time_t  th = srcjob.spq_hold;
		time_t  now = time(NULL);
		if  (((CSprservApp *)AfxGetApp())->m_options.spq_options.abshold)
			dtime = th > now ? htonl(th): 0;
		else
			dtime = th + now;
	}
	destjob.spq_hold = htonl(dtime);
	destjob.spq_size = htonl(long(srcjob.spq_size));
	destjob.spq_posn = htonl(long(srcjob.spq_posn));
	destjob.spq_pagec = htonl(long(srcjob.spq_pagec));
	destjob.spq_start = htonl(long(srcjob.spq_start));
	destjob.spq_end = htonl(long(srcjob.spq_end));
	destjob.spq_npages = htonl(long(srcjob.spq_npages));
	destjob.spq_haltat = htonl(long(srcjob.spq_haltat));

	destjob.spq_jflags = htons(srcjob.spq_jflags);
	destjob.spq_nptimeout = htons(srcjob.spq_nptimeout);
	destjob.spq_ptimeout = htons(srcjob.spq_ptimeout);
	destjob.spq_uid = htonl(srcjob.spq_uid);
	destjob.spq_class = htonl(srcjob.spq_class);
	destjob.spq_extrn = 0;
	destjob.spq_pglim = htons(srcjob.spq_pglim);
	destjob.spq_proptime = 0L;
	destjob.spq_starttime = 0L;
	destjob.spq_wpri = htons(short(srcjob.spq_wpri));

	destjob.spq_cps = srcjob.spq_cps;
	destjob.spq_pri = srcjob.spq_pri;
	destjob.spq_sflags = srcjob.spq_sflags;
	destjob.spq_dflags = srcjob.spq_dflags;

	strncpy(destjob.spq_uname, username, UIDSIZE+1);
	strncpy(destjob.spq_puname, srcjob.spq_puname, UIDSIZE+1);
	strncpy(destjob.spq_file, srcjob.spq_file, MAXTITLE+1);
	strncpy(destjob.spq_form, srcjob.spq_form, MAXFORM+1);
	strncpy(destjob.spq_ptr, srcjob.spq_ptr, JPTRNAMESIZE+1);
	strncpy(destjob.spq_flags, srcjob.spq_flags, MAXFLAGS+1);

	destp.delimnum	= htonl(srcp.delimnum);
	destp.deliml = htonl(srcp.deliml);
	destp.lastpage = 0L;                        

	//  If no title make one out of file name
	
	if  (destjob.spq_file[0] == '\0')  {
		char	*cp = strrchr(filename, '\\');
		if  (cp)
			strncpy(destjob.spq_file, cp + 1, MAXTITLE);
		else
			strncpy(destjob.spq_file, filename, MAXTITLE);
		destjob.spq_file[MAXTITLE] = '\0';
	}
}	

UINT	spew_header(spq &swapedj, pages &swapedp, const char *delimiter)
{
	long	pmsgl[CL_SV_BUFFSIZE]; // Force to long
	char	*pmsg = (char *) pmsgl;
	pmsg[0] = CL_SV_STARTJOB;
	pmsg += sizeof(long);
	unsigned  datalength = sizeof(long);
	memcpy((void *) pmsg, (void *) &swapedj, sizeof(spq));
	pmsg += sizeof(spq);
	datalength += sizeof(spq);

	time(&Locparams.tlastop);

	if  (swapedj.spq_dflags & SPQ_PAGEFILE)  {
		memcpy((void *) pmsg, (void *) &swapedp, sizeof(swapedp));
		pmsg += sizeof(pages);
		datalength += sizeof(pages);

		unsigned  delimtogo = unsigned(delimiter);
		while  (delimtogo != 0)  {
			unsigned  delimbytes = CL_SV_BUFFSIZE - datalength;
			int	ret;
			if  (delimbytes > delimtogo)
				delimbytes = delimtogo;
			datalength += delimbytes;
			memcpy((void *) pmsg, (void *) delimiter, delimbytes);
			if  (ret = xmit((char *) pmsgl, datalength))
				return	ret;
			delimiter += delimbytes;
			delimtogo -= delimbytes;
			datalength = 1;
			pmsg = (char *) pmsgl;
			pmsg[0] = CL_SV_CONTDELIM;		//  Mayhap more
			pmsg++;
		}
		return  0;
	}
	else
		return  xmit((char *) pmsgl, datalength);
}

UINT	spew_data(CFile &fl)
{
	long	pmsgl[CL_SV_BUFFSIZE]; // Force to long
	char	*pmsg = (char *) pmsgl;
	pmsg[0] = CL_SV_JOBDATA;
	UINT  nbytes;
	int  ret;
	while  ((nbytes = fl.Read((char *)&pmsg[1], CL_SV_BUFFSIZE-1)) != 0)
		if  (ret = xmit(pmsg, nbytes+1))
			return  ret;
	return  0;	
}	

UINT	spew_endjob()
{
	char	pmsg[1];
	pmsg[0] = CL_SV_ENDJOB;
	return  xmit(pmsg, 1);
}

UINT	dosubmitj(spq &job, const char *file, spropts &opts)
{
	CFile	ifl;
	
	if  (!ifl.Open(file, CFile::modeRead))
		return  IDP_CANTOPENJOBFILE;

	spq		packedj;
	pages	packedp;
	qinit(((CSprservApp *)AfxGetApp())->m_username, file, job, packedj, opts.pfe, packedp, opts.delimiter);

	UINT	ret;
	if  (ret = spew_header(packedj, packedp, opts.delimiter))
		return  ret;
	if  (ret = spew_data(ifl))
		return  ret;
	spew_endjob();
	return  0;
}	