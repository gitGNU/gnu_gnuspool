//  Home-grown DOS versions of the private profile stuff

int	DosGetPrivateProfileString(const char *,const char *,const char *,char *,UINT,const char *);
UINT DosGetPrivateProfileInt(const char *,const char *,const int,const char *);
BOOL DosWritePrivateProfileString(const char *,const char *,const char *,const char *);
BOOL DosWritePrivateProfileInt(const char *,const char *,unsigned long,const char *);
BOOL DosWritePrivateProfileHex(const char *,const char *,unsigned long,const char *);
