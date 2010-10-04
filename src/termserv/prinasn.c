/* prinasn.c -- print out asn structure

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

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "incl_unix.h"
#include "asn.h"

void  prinbuf(asn_octet *buf, const unsigned len)
{
	unsigned  cnt4, cnt;
	for  (cnt4 = 0;  cnt4 < len;  cnt4 += 16)  {
		for  (cnt = 0;  cnt < 16;  cnt++)  {
			if  (cnt4 + cnt >= len)
				fprintf(stderr, "  ");
			else
				fprintf(stderr, "%.2x", buf[cnt4+cnt]);
			if  ((cnt & 3) == 3)
				putc(' ', stderr);
		}
		for  (cnt = 0;  cnt < 16;  cnt++)  {
			if  ((cnt & 3) == 0)
				putc(' ', stderr);
			if  (cnt4 + cnt < len)  {
				if  (isprint(buf[cnt4+cnt]))
					putc(buf[cnt4+cnt], stderr);
				else
					putc('.', stderr);
			}
		}
		putc('\n', stderr);
	}
}

void  prin_asn(struct asn_node *nd, const int level)
{
	while  (nd)  {
		int	cnt;
		for  (cnt = 0;  cnt < level;  cnt++)
			putc('\t', stderr);
		fprintf(stderr, "Type=%.2x ", nd->asn_type);
		switch  (nd->asn_int_type)  {
		case  ASN_INT_NULL:
			fprintf(stderr, "Null\n");
			break;
		case  ASN_INT_BOOL:
			fprintf(stderr, "Bool: %s\n", nd->asn_un.asn_bool? "true": "false");
			break;
		case  ASN_INT_SIGNED:
			fprintf(stderr, "Signed: %ld\n", nd->asn_un.asn_signed);
			break;
		case  ASN_INT_UNSIGNED:
			fprintf(stderr, "Unsigned: %lu\n", nd->asn_un.asn_unsigned);
			break;
		case  ASN_INT_STRING:
			fprintf(stderr, "String: %s\n", nd->asn_un.asn_string);
			break;
		case  ASN_INT_OBJID:
			fprintf(stderr, "Object id: %s\n", nd->asn_un.asn_string);
			break;
		case  ASN_INT_SEQUENCE:
			fprintf(stderr, "Sequence\n");
			prin_asn(nd->asn_firstchild, level+1);
			break;
		}
		nd = nd->asn_next;
	}
}
