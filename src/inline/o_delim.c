/* o_delim.c -- option to set delimiter

   Copyright 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

OPTION(o_delim)
{
	char	*rp;
	const	char	*ap = arg;
	char	*result;
	int	res, i;

	if  (!arg)
		return  OPTRESULT_MISSARG;

	/* Theory: it can't be bigger */
	result = (char *) malloc((unsigned) (strlen(arg) + 1));
	if  (result == (char *) 0)
		nomem();

	rp = result;

	for  (;;)  {
		switch  (*ap)  {
		case  '\0':
			*rp = '\0';
			pfe.deliml = rp - result;
			delimiter = result;
			return  OPTRESULT_ARG_OK;
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
				*rp++ = (char) res;
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
				*rp++ = (char) res;
				continue;
			}
		}
	}

 badesc:
	disp_str = arg;
	print_error($E{Invalid delim arg});
	exit(E_USAGE);
}
