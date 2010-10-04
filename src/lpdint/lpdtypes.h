/* lpdtypes.h -- defs for xtlpd

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

struct	ctrltype	{
	struct ctrltype	*ctrl_next;		/* Next in chain */
	char		ctrl_letter;		/* Letter in control file */
	unsigned  char	ctrl_action;		/* What to do with it */
#define	CT_NONE		0			/* Nothing */
#define	CT_INDENT	1			/* Indent command */
#define	CT_UNLINK	2			/* Unlink command */
#define	CT_ASSIGN	3			/* Assign */
#define	CT_PREASSIGN	4			/* Assign prepend */
#define	CT_POSTASSIGN	5			/* Assign append */
#define	CT_PIPECOMMAND	6			/* Run command */
	unsigned  char	ctrl_repeat;
	unsigned  char	ctrl_fieldtype;
#define	CT_NUMBER	1			/* Field type is numeric */
#define	CT_STRING	2			/* Field type is string */
#define	CT_FILENAME	3			/* Field type is file name */
	char		*ctrl_string;		/* Value to assign or execute */
	struct	varname	*ctrl_var;
};

extern	struct	ctrltype	*ctrl_list[],
				*begin_ctrl,
				*end_ctrl,
				*repeat_ctrl,
				*norepeat_ctrl;

extern struct	varname	*lookuphash(const char *);
extern char *expandvars(char *);
extern void  tf_unlink(char *, const int);

/* Predefined variable names to be set up in control file.  */

#define	SPOOLDIR	"XTLPDSPOOL"
#define	LOGFILE		"LOGFILE"
#define	DEBUG_VAR	"DEBUG"
#define	PORTNAME	"PORTNAME"
#define	SHORT_LIST	"SHORTLIST"
#define	LONG_LIST	"LONGLIST"
#define	REMOVE		"REMOVE"

/* Variable names set up by each print.  */

#define	HOSTNAME	"HOSTNAME"
#define	PRINTER_VAR	"PRINTER"
#define	PERSON_VAR	"PERSON"
#define	CMDLINE_VAR	"CMDLINE"
