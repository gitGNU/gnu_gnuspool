#include "stdafx.h"
#include "GetRegData.h"

#ifdef	REGSTRING

CString	GetRegString(CString name)
{
	CString	result = "";
	int  wh = name.ReverseFind('\\');
	if  (wh < 0)
		return  result;
	HKEY	hk;
	if  (RegOpenKeyEx(HKEY_LOCAL_MACHINE, name.Left(wh), 0, KEY_READ, &hk) != ERROR_SUCCESS)
		return  result;
	DWORD  buffsize = 100;
	char	buff[100];
	if  (RegQueryValueEx(hk, name.Right(name.GetLength() - wh - 1), 0, NULL, (LPBYTE) buff, &buffsize) == ERROR_SUCCESS)
		result = buff;
	RegCloseKey(hk);
	return  result;
}

#else

void	GetUserAndComputerNames(CString &usern, CString &computn)
{
	char	buffr[100];
	DWORD	buffrs = sizeof(buffr);
	if  (GetUserName(buffr, &buffrs))
		usern = buffr;
	buffrs = sizeof(buffr);
	if  (GetComputerName(buffr, &buffrs))
		computn = buffr;

}
#endif