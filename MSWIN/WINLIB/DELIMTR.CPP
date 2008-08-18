#include "stdafx.h"
#include <string.h>
#include <ctype.h>

//  Translate delimiter expressed in \fs etc to a string

char	*translate_delim(const char *inbuf, unsigned &deliml)
{                                          
	//  Allocate a result buffer on the theory that it can't get any
	//  bigger
	
	char	*result = new char[strlen(inbuf) + 1];
	if  (!result)
		return  result;
		
	char	*rp = result;
	const  char  *ap = inbuf;
	int	res, i;

	for  (;;)  {
		switch  (*ap)  {
		case  '\0':
			*rp = '\0';
			deliml = rp - result;
			return  result;
			
		default:
			*rp++ = *ap++;
			continue;
			
		case  '^':
			switch  (*++ap)  {
			default:
				*rp++ = *ap++ & 0x1f;
				continue;
				
			case  '^':
				*rp++ = *ap++;
				continue;
				
			case  '\0':
				goto  badesc;
			}
			
		case  '\\':
			switch  (*++ap)  {
			case  '\0':
				goto  badesc;

			default:
				*rp++ = *ap++;
				break;

			case  'e':case  'E':
				*rp++ = '\033';	ap++;	break;
			case  'b':case  'B':
				*rp++ = '\010';	ap++;	break;
			case  'r':case  'R':
				*rp++ = '\r';	ap++;	break;
			case  'n':case  'N':
				*rp++ = '\n';	ap++;	break;
			case  'f':case  'F':
				*rp++ = '\f';	ap++;	break;
			case  's':case  'S':
				*rp++ = ' ';	ap++;	break;
			case  't':case  'T':
				*rp++ = '\t';	ap++;	break;
			case  'v':case  'V':
				*rp++ = '\v';	ap++;	break;

			case  '0':
				res = 0;
				ap++;
				if  (!isdigit(*ap))
					goto  badesc;
				
				for  (i = 0;  i < 3;  i++)  {
					if  (*ap < '0' || *ap > '7')
						break;
					res = (res << 3) + *ap++ - '0';
				}
				*rp++ = res;
				break;

			case  'x':
			case  'X':
				res = 0;
				ap++;
				if  (!isxdigit(*ap))
					goto  badesc;
				for  (i = 0;  i < 2;  i++)  {
					if  (isdigit(*ap))
						res = (res<<4) + *ap++ - '0';
					else if (*ap >= 'a' && *ap <= 'f')
						res = (res<<4) + *ap++ - 'a' + 10;
					else if (*ap >= 'A' && *ap <= 'F')
						res = (res<<4) + *ap++ - 'A' + 10;
					else
						break;
				}
				*rp++ = res;
				continue;
			}
		}
	}           

badesc:
	delete [] result;
	return  (char *) 0;
}

char	*untranslate_delim(const char *inbuf, const unsigned leng)
{                                                   
	unsigned  outleng = 1;
	for  (unsigned  dcnt = 0;  dcnt < leng;  dcnt++)  {
		int	 nch = inbuf[dcnt] & 255;
		switch  (nch)  {
		default:
			outleng++;
			if  (nch < ' ')
				outleng++;
			else  if  (nch > '~')
				outleng += 3;
			break;
		case  '\033':
		case  'h' & 0x1f:
		case  '\r':
		case  '\n':
		case  '\f':
		case  '\t':
		case  '\v':
			outleng += 2;
			break;       
		}
	}
	
	char	*result = new char [outleng];
	if  (!result)
		return  result;
		
	char	*rp = result;
	for  (dcnt = 0;  dcnt < leng;  dcnt++)  {
		int	 nch = inbuf[dcnt] & 255;
		switch  (nch)  {
		default:
			if  (nch >= ' ' && nch <= '~')
				*rp++ = char(nch);
			else  if  (nch < ' ')  {
				*rp++ = '^';
				*rp++ = char(nch + '@');
			}	
			else  {
				const char	hexchs[] = "0123456789abcdef";
				*rp++ = '\\';
				*rp++ = 'x';
				*rp++ = hexchs[(nch >> 4) & 0x0f];
				*rp++ = hexchs[nch & 0x0f];
			}
			break;
		case  '\033':
			*rp++ = '\\';
			*rp++ = 'e';
			break;
		case  'h' & 0x1f:
			*rp++ = '\\';
			*rp++ = 'b';
			break;
		case  '\r':
			*rp++ = '\\';
			*rp++ = 'r';
			break;
		case  '\n':
			*rp++ = '\\';
			*rp++ = 'n';
			break;
		case  '\f':
			*rp++ = '\\';
			*rp++ = 'f';
			break;
		case  '\t':
			*rp++ = '\\';
			*rp++ = 't';
			break;
		case  '\v':
			*rp++ = '\\';
			*rp++ = 'v';
			break;
		}
	}
	*rp = '\0';
	return  result;
}
