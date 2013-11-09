/* kw.h -- lexical tokens etc for spdinit parser

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

struct  rexpr   {
        struct  rexpr   *r_next;
        SHORT   r_type;
        SHORT   r_min, r_max;
        union   {
                struct  rexpr   *r_alt;
                unsigned  char  *r_range;
                SHORT   r_ch;
        }       r_un;
};

#define RE_CH           0
#define RE_RANGE        1
#define RE_NRANGE       2
#define RE_ALT          3

struct  string  {
        struct  string  *s_next;        /*  Next in chain  */
        USHORT  s_length;       /*  Length of string  */
        char    *s_str;                 /*  Actual string  */
};

struct  kw      {
        struct  kw      *k_next;        /*  Next in hash chain  */
        SHORT   k_type;                 /*  Type  */
        USHORT  k_nams;         /*  Length - 0 for std  */
        union  kwun     {
                struct  {
                        void    (*k_proc)();    /*  Action  */
                        unsigned        k_arg;  /*  Argument  */
                }  k_pa;
                struct  string  *k_str; /*  String  */
        }  k_un;
        char    *k_name;                /*  Chars of ident  */
};

#define TK_UNDEF        0
#define TK_STR          1
#define TK_PROC         2
#define TK_EXEC         3
#define TK_SETUP        4
#define TK_HALT         5
#define TK_DOCST        6
#define TK_DOCEND       7
#define TK_SUFST        8
#define TK_SUFEND       9
#define TK_PAGESTART    10
#define TK_PAGEEND      11
#define TK_ABORT        12
#define TK_RESTART      13
#define TK_DELIM        14
#define TK_BANNER       15
#define TK_EXIT         16
#define TK_SIGNAL       17
#define TK_OFFLINE      18
#define TK_ERROR        19
#define TK_BANNPROG     20
#define TK_ALIGN        21
#define TK_EXECALIGN    22
#define TK_PORTSU       23
#define TK_FILTER       24
#define TK_NETFILT      25
#define TK_STTY         26

/* Flag values for rdstr() */

#define ST_NOESC        1
#define ST_SPTERM       2
#define ST_SQTERM       4
#define ST_DQTERM       8
#define ST_GTTERM       16
