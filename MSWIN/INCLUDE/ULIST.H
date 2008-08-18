//	Obtain a list of Unix Users

#define	UIDSIZE	11

class	UUserList	{
private:
	int			pos, nbytes;
	unsigned	preflen;
	char		prefix[UIDSIZE+1];
	char		buffer[CL_SV_BUFFSIZE];
	
public:
	UUserList(const char FAR * = (const char FAR *) 0);
	const  char  FAR  *nextuser(void);
};
	