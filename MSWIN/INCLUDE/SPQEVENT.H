/*
 *----------------------------------------------------------------------
 * $Header: /sources/gnuspool/gnuspool/MSWIN/INCLUDE/SPQEVENT.H,v 1.1 2008/08/18 16:25:54 jmc Exp $
 * $Log: SPQEVENT.H,v $
 * Revision 1.1  2008/08/18 16:25:54  jmc
 * Initial revision
 *
 *----------------------------------------------------------------------
 * DOS spq Classes MS C++
 * John Collins 10/10/92
 */

class	spq_event   {
public:
	virtual  void   process() = 0;
};

#define JOBOP_CLASS(name)   class  name: public spq_event {\
public:\
	void    process();\
}

JOBOP_CLASS(job_unknownc);
JOBOP_CLASS(job_refresh);
JOBOP_CLASS(job_help);
JOBOP_CLASS(job_next);
JOBOP_CLASS(job_prev);
JOBOP_CLASS(job_nscr);
JOBOP_CLASS(job_pscr);
JOBOP_CLASS(job_nhscr);
JOBOP_CLASS(job_phscr);
JOBOP_CLASS(job_top);
JOBOP_CLASS(job_bottom);
JOBOP_CLASS(job_toptr);
JOBOP_CLASS(job_quit);
JOBOP_CLASS(job_viewerror);
JOBOP_CLASS(job_abort);
JOBOP_CLASS(job_copies);
JOBOP_CLASS(job_priority);
JOBOP_CLASS(job_printer);
JOBOP_CLASS(job_form);
JOBOP_CLASS(job_title);
JOBOP_CLASS(job_options);
JOBOP_CLASS(job_view);
JOBOP_CLASS(job_unqueue);
JOBOP_CLASS(job_saveopts);
#undef	JOBOP_CLASS

#define PTROP_CLASS(name)	 class	name: public spq_event {\
public:\
	void    process();\
}

PTROP_CLASS(ptr_unknownc);
PTROP_CLASS(ptr_refresh);
PTROP_CLASS(ptr_help);
PTROP_CLASS(ptr_next);
PTROP_CLASS(ptr_prev);
PTROP_CLASS(ptr_nscr);
PTROP_CLASS(ptr_pscr);
PTROP_CLASS(ptr_nhscr);
PTROP_CLASS(ptr_phscr);
PTROP_CLASS(ptr_top);
PTROP_CLASS(ptr_bottom);
PTROP_CLASS(ptr_tojob);
PTROP_CLASS(ptr_quit);
PTROP_CLASS(ptr_viewerror);
PTROP_CLASS(ptr_heoj);
PTROP_CLASS(ptr_halt);
PTROP_CLASS(ptr_go);
PTROP_CLASS(ptr_interrupt);
PTROP_CLASS(ptr_restart);
PTROP_CLASS(ptr_ok);
PTROP_CLASS(ptr_nok);
PTROP_CLASS(ptr_delete);
PTROP_CLASS(ptr_abort);
PTROP_CLASS(ptr_form);
PTROP_CLASS(ptr_name);
PTROP_CLASS(ptr_add);
PTROP_CLASS(ptr_classcode);
PTROP_CLASS(ptr_llimit);
PTROP_CLASS(ptr_ulimit);
PTROP_CLASS(ptr_saveopts);
#undef	PTROP_CLASS

extern	spq_event	*next_event();
