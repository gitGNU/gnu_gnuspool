/* xtlhpdefs.h -- declarations for xtlhp

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

struct  macro  {
        struct  macro   *next;
        char            *name;
        char            *expansion;
};

struct  value  {
        int             type;
#define UNDEF_VALUE             0
#define STRING_VALUE            1
#define FSTRING_VALUE           2
#define NUM_VALUE               3
#define CMD_STRING_VALUE        4
#define CMD_NUM_VALUE           5
#define NAME_VALUE              6
#define BRACE_VALUE             7
#define SNMPVAR_VALUE           8
#define LASTVAL_VALUE           9
        union  {
                long            longval;
                char            *stringval;
                struct  macro   *namev;
        }  val_un;
};

struct  compare  {
        int             type;
#define COMP_EQ         0
#define COMP_NE         1
#define COMP_LT         2
#define COMP_LE         3
#define COMP_GT         4
#define COMP_GE         5
#define STR_COMP        6
#define BIT_SET         7
#define BIT_CLEAR       8
        struct  value   *left, *right;
};

struct  boolexpr  {
        int             type;
#define VARDEFINED      0
#define VARUNDEFINED    1
#define ISSTRINGVAL     2
#define ISNUMVAL        3
#define COMPARE         4
#define ANDEXPR         5
#define OREXPR          6
#define NOTEXPR         7
#define BITOPER         8
#define ALL_CLEAR       9
        union  {
                struct  value   *val;
                struct  compare  *comp;
                struct  boolexpr *expr;
                char    *snmpstring;
        }  left_un;
        struct  boolexpr *rightexpr;
};

struct  command  {
        struct  command *next;
        int             type;
#define CMD_NOP         0
#define CMD_ONEASS      1
#define CMD_ASS         2
#define CMD_IF          3
#define CMD_EXIT        4
#define CMD_MSG         5
#define CMD_FLUSH       6
        union  {
                struct  {
                        struct  macro   *ass_name;
                        struct  value   *ass_value;
                }  ass;
                struct  {
                        struct  boolexpr        *comp_expr;
                        struct  command         *thenpart;
                        struct  command         *elsepart;
                }  ifthen;
                struct  value   *exitcode;
                struct  value   *msgval;
        }  cmd_un;
};

#define MAXNAMESIZE     49
#define MAXEXPSIZE      99
#define BLDBUFF         1024

#define EXIT_NULL       1
#define EXIT_USAGE      4
#define EXIT_DEVERROR   5
#define EXIT_OFFLINE    6
#define EXIT_SYSERROR   7
#define EXIT_SNMPERROR  8

#define DEFAULT_BLKSIZE         10240
#define DEFAULT_CONFIGNAME      "xtsnmpdef"
#define DEFAULT_CTRLNAME        "xtlhp-ctrl"
#define HOSTNAME_NAME           "HOSTNAME"
#define PORTNAME_NAME           "PORTNAME"
