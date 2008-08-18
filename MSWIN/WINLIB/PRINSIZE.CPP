#include "stdafx.h"

const	struct  dtbl	{
	long	compar;
	int	shift;
	char	*fmt;
}  dtab[] = {
	{ 10000L, 0, "%ld" },
	{ 10L << 20, 10, "%ldK" },
	{ 1L << 30, 20, "%ldM" },
	{ 0L, 30, "%ldG" }
};

void	prin_size(char FAR * resbuf, const long l)
{
	for  (dtbl const *dt = dtab;  dt->compar &&  l >= dt->compar;  dt++)
		;
    
    long  cl = l;
	if  (dt->shift)
		cl = (cl + (1L << (dt->shift-1))) >> dt->shift;

	(void) wsprintf(resbuf, dt->fmt, cl);
}
