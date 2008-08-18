/* lpctypes.h -- defs for parsing xtlpc control file

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

struct	varname		{
	struct	varname	*vn_next;		/* In hash chain */
	char		*vn_name;		/* Name of variable */
	char		*vn_value;		/* Value */
};

struct	condition	{
	struct	varname	*cond_var;		/* Variable involved */
	char		*cond_string;		/* Condition string */
	unsigned  char	cond_type;		/* Condition type */
#define	COND_DEF	0			/* Defined as non-null */
#define	COND_EQ		1			/* Equals */
#define	COND_NEQ	2			/* Not equals */
#define	COND_MATCH	3			/* Pattern match */
#define	COND_NONMATCH	4			/* Pattern non-match */
#define	COND_MULT	5			/* Multiply */
};

struct	ctrltype	{
	struct ctrltype	*ctrl_next;		/* Next in chain */
	struct condition *ctrl_cond;		/* Condition if any */
	char		*ctrl_string;		/* Value to assign or execute */
	unsigned  char	ctrl_action;		/* Action */
#define	CT_ASSIGN	0			/* Assignment */
#define	CT_PREASSIGN	1			/* Assign prepend */
#define	CT_POSTASSIGN	2			/* Assign append */
#define	CT_ADDCFILE	3			/* Add line to C-File */
#define	CT_SENDLINE	4			/* Send line to server plus \n */
#define	CT_RECVLINE	5			/* Receive line from server NO \n */
#define	CT_SENDFILE	6			/* Send file to server */
#define	CT_SENDLINEONCE	7			/* As sendline but once only */
#define	CT_RECVLINEONCE	8			/* As recvline but once only */
#define	CT_NOP		9			/* What ONCE ones get transmogrified into */
};

extern	struct	ctrltype	*card_list, *proto_list;

extern struct varname *	lookuphash(const char *);
extern char  *expandvars(char *);
