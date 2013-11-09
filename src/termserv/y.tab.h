/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C

      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.

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

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NUMBER = 258,
     BACKQUOTESTR = 259,
     SQUOTESTR = 260,
     DQUOTESTR = 261,
     BRACKETSTR = 262,
     BRACESTR = 263,
     SNMPVARVAL = 264,
     SNMPSTRVAL = 265,
     SNMPDEFINED = 266,
     SNMPUNDEFINED = 267,
     NAME = 268,
     NUMERIC_COMP = 269,
     STRING_COMP = 270,
     BITOP = 271,
     ASSIGN = 272,
     ONEASSIGN = 273,
     IF = 274,
     THEN = 275,
     ELSE = 276,
     ELIF = 277,
     FI = 278,
     MSG = 279,
     EXIT = 280,
     FLUSH = 281,
     LASTVAL = 282,
     ISNUM = 283,
     ISSTRING = 284,
     NOT = 285,
     ALLCLEAR = 286,
     OROP = 287,
     ANDOP = 288
   };
#endif
/* Tokens.  */
#define NUMBER 258
#define BACKQUOTESTR 259
#define SQUOTESTR 260
#define DQUOTESTR 261
#define BRACKETSTR 262
#define BRACESTR 263
#define SNMPVARVAL 264
#define SNMPSTRVAL 265
#define SNMPDEFINED 266
#define SNMPUNDEFINED 267
#define NAME 268
#define NUMERIC_COMP 269
#define STRING_COMP 270
#define BITOP 271
#define ASSIGN 272
#define ONEASSIGN 273
#define IF 274
#define THEN 275
#define ELSE 276
#define ELIF 277
#define FI 278
#define MSG 279
#define EXIT 280
#define FLUSH 281
#define LASTVAL 282
#define ISNUM 283
#define ISSTRING 284
#define NOT 285
#define ALLCLEAR 286
#define OROP 287
#define ANDOP 288




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 77 "xtlhpgram.y"

        int             intval;
        long            longval;
        char            *stringval;
        struct  value   *valval;
        struct  macro   *nameval;
        struct  boolexpr *exprval;
        struct  compare *cmpval;
        struct  command *cmdval;



/* Line 2068 of yacc.c  */
#line 129 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


