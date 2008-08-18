/*
 *	Interface to xtlogin server.
 */
 
#ifdef	__cplusplus
extern	"C"		{
#endif

extern	const  char  FAR  *	FAR	/*_loadds*/	getxtlogin(HANDLE, const char FAR *, const char FAR *, const UINT);
extern	BOOL	FAR	/*_loadds*/		runxtlog(const char FAR *, const char FAR *);

#ifdef	__cplusplus
}
#endif
