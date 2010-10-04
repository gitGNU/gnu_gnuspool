/* snmp.h -- SNMP op declarations

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

struct	snmp_result  {
	unsigned  res_type;
	unsigned  res_length;
	char	  *res_id_string;
#define	RES_TYPE_SIGNED		0
#define	RES_TYPE_UNSIGNED	1
#define	RES_TYPE_STRING		2
	union  {
		char		*res_string;
		long		res_signed;
		unsigned  long	res_unsigned;
	}  res_un;
};

#define	RES_OK		0
#define	RES_OFFLINE	1
#define	RES_UNDEF	2
#define	RES_ERROR	3
#define	RES_NULL	4

extern asn_octet *gen_snmp_get(const char *, const char *, unsigned *, const int);
extern int  snmp_parse_result(asn_octet *, const unsigned, struct snmp_result *);
