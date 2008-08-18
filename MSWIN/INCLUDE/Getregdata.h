#ifdef	REGSTRING
//  Extract a string from the registry.
extern	CString	GetRegString(CString);
#else
extern	void	GetUserAndComputerNames(CString &, CString &);
#endif
