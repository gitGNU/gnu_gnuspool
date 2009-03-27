/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEG = 258,
     DEFINE = 259,
     MODULES = 260,
     INCR = 261,
     DECR = 262,
     ROUND = 263,
     COPY = 264,
     REQUIRE = 265,
     FILENAME = 266,
     STRING = 267,
     COMMENT = 268,
     VALNAME = 269,
     NUMBER = 270,
     HELPTYPE = 271,
     CHNUMBER = 272
   };
#endif
/* Tokens.  */
#define NEG 258
#define DEFINE 259
#define MODULES 260
#define INCR 261
#define DECR 262
#define ROUND 263
#define COPY 264
#define REQUIRE 265
#define FILENAME 266
#define STRING 267
#define COMMENT 268
#define VALNAME 269
#define NUMBER 270
#define HELPTYPE 271
#define CHNUMBER 272




/* Copy the first part of user declarations.  */
#line 17 "msgparse.y"

#include "config.h"
#include <stdio.h>
#include "incl_unix.h"
#include "hdefs.h"

extern	void	nomem();
extern	int	yylex();
extern	void	valname_usage(struct valexpr *, const unsigned);
extern	void	apphelps(char *, struct valexpr *, char *, struct valexpr *, struct filelist *, struct textlist *);
extern	void	merge_helps(struct valexpr *, struct valexpr *);
 
int	errors = 0,
	pass1 = 1;

extern	int	line_count;

struct	valname	*last_assign;
struct	valexpr	*last_expr;
long		last_value;

extern	struct	helpfile	helpfiles[];

static	char	htlist[MAXHELPTYPES];

void	yyerror(char *msg)
{
	fprintf(stderr, "Parse error: %s on line %d\n", msg, line_count);
	errors++;
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 50 "msgparse.y"
{
	char			ch;
	int			num;
	char			*str;
	struct	valname		*vname;
	struct	valexpr		*vexpr;
	struct	module_list	*mlist;
	struct	program		*pgm;
	struct	program_list	*plist;
	struct	textlist	*tlist;
	struct	filelist	*flist;
}
/* Line 193 of yacc.c.  */
#line 176 "y.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 189 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   125

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  30
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  32
/* YYNRULES -- Number of rules.  */
#define YYNRULES  65
/* YYNRULES -- Number of states.  */
#define YYNSTATES  107

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   272

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     7,     2,     2,
      26,    27,     5,     3,    28,     4,     2,     6,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    24,    23,
       2,    25,     2,     2,    29,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     6,     8,    11,    13,    15,    17,    20,
      27,    29,    32,    35,    41,    43,    46,    48,    55,    57,
      59,    62,    64,    65,    69,    71,    74,    76,    78,    80,
      84,    92,    93,    95,    96,    98,   102,   104,   108,   111,
     112,   114,   115,   118,   120,   124,   126,   127,   129,   131,
     134,   136,   138,   140,   142,   144,   151,   155,   159,   162,
     166,   170,   174,   175,   177,   181
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      31,     0,    -1,    32,    45,    -1,    33,    -1,    32,    33,
      -1,    34,    -1,    37,    -1,    40,    -1,     1,    23,    -1,
       9,    16,    44,    24,    35,    23,    -1,    36,    -1,    35,
      36,    -1,    44,    16,    -1,    16,    44,    24,    38,    23,
      -1,    39,    -1,    38,    39,    -1,    16,    -1,    10,    41,
      44,    25,    42,    23,    -1,    16,    -1,    43,    -1,    42,
      43,    -1,    16,    -1,    -1,    26,    16,    27,    -1,    46,
      -1,    45,    46,    -1,    47,    -1,    48,    -1,     1,    -1,
      19,    25,    59,    -1,    49,    55,    50,    52,    53,    54,
      60,    -1,    -1,    18,    -1,    -1,    51,    -1,    26,    59,
      27,    -1,    21,    -1,    52,    28,    21,    -1,    59,    58,
      -1,    -1,    14,    -1,    -1,    15,    56,    -1,    57,    -1,
      56,    28,    57,    -1,    16,    -1,    -1,    11,    -1,    12,
      -1,    25,    59,    -1,    19,    -1,    20,    -1,    29,    -1,
      22,    -1,    51,    -1,    13,    26,    59,    28,    59,    27,
      -1,    59,     3,    59,    -1,    59,     4,    59,    -1,     4,
      59,    -1,    59,     5,    59,    -1,    59,     6,    59,    -1,
      59,     7,    59,    -1,    -1,    61,    -1,    60,    28,    61,
      -1,    17,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    85,    85,    87,    88,    90,    90,    90,    90,    92,
     101,   102,   113,   122,   131,   132,   143,   151,   159,   170,
     171,   180,   187,   191,   200,   201,   203,   205,   207,   216,
     237,   289,   291,   295,   299,   302,   308,   315,   325,   346,
     350,   357,   361,   367,   369,   376,   396,   400,   405,   410,
     416,   421,   426,   431,   437,   442,   450,   458,   466,   477,
     485,   493,   503,   505,   510,   524
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "'+'", "'-'", "'*'", "'/'", "'%'", "NEG",
  "DEFINE", "MODULES", "INCR", "DECR", "ROUND", "COPY", "REQUIRE",
  "FILENAME", "STRING", "COMMENT", "VALNAME", "NUMBER", "HELPTYPE",
  "CHNUMBER", "';'", "':'", "'='", "'('", "')'", "','", "'@'", "$accept",
  "helpfiledefs", "definitions", "definition", "macrodef", "modlist",
  "module", "helpusedin", "proglist", "program", "progmodules",
  "defined_program", "modormacrolist", "modormacro", "optsubdir",
  "helptexts", "helptext", "namedef", "helpdefn", "optcomment",
  "optprimary", "primary", "helptypes", "nameorexpr", "optcopy",
  "optrequire", "reqlist", "req", "optassign", "valexpr", "textlist",
  "textstring", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,    43,    45,    42,    47,    37,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,    59,    58,    61,    40,    41,    44,    64
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    30,    31,    32,    32,    33,    33,    33,    33,    34,
      35,    35,    36,    37,    38,    38,    39,    40,    41,    42,
      42,    43,    44,    44,    45,    45,    46,    46,    46,    47,
      48,    49,    49,    50,    50,    51,    52,    52,    53,    54,
      54,    55,    55,    56,    56,    57,    58,    58,    58,    58,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    60,    60,    60,    61
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     2,     1,     1,     1,     2,     6,
       1,     2,     2,     5,     1,     2,     1,     6,     1,     1,
       2,     1,     0,     3,     1,     2,     1,     1,     1,     3,
       7,     0,     1,     0,     1,     3,     1,     3,     2,     0,
       1,     0,     2,     1,     3,     1,     0,     1,     1,     2,
       1,     1,     1,     1,     1,     6,     3,     3,     2,     3,
       3,     3,     0,     1,     3,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,    22,     0,     0,     3,     5,     6,
       7,     8,    22,    18,    22,     0,     0,     1,    28,    32,
       0,     4,     0,    24,    26,    27,    41,     0,     0,     0,
       0,     0,    28,    25,     0,    33,    22,     0,    23,    16,
       0,    14,     0,     0,    50,    51,    53,     0,    52,    54,
      29,    45,    42,    43,     0,    34,    22,    10,     0,    21,
       0,    19,    13,    15,    58,     0,     0,     0,     0,     0,
       0,     0,     0,    36,     0,     9,    11,    12,    17,    20,
       0,    35,    56,    57,    59,    60,    61,    44,     0,    39,
      46,     0,    37,    40,    62,    47,    48,     0,    38,     0,
      65,    30,    63,    49,    55,     0,    64
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     5,     6,     7,     8,    56,    57,     9,    40,    41,
      10,    14,    60,    61,    58,    22,    23,    24,    25,    26,
      54,    49,    74,    89,    94,    35,    52,    53,    98,    50,
     101,   102
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -43
static const yytype_int8 yypact[] =
{
       2,   -21,     1,     5,    -7,    37,    38,   -43,   -43,   -43,
     -43,   -43,    -7,   -43,    -7,    22,    16,   -43,   -21,   -43,
      18,   -43,    15,   -43,   -43,   -43,    35,    28,    33,    34,
      49,    58,   -43,   -43,    52,    44,    -7,    56,   -43,   -43,
      -3,   -43,    58,    48,   -43,   -43,   -43,    58,   -43,   -43,
     118,   -43,    51,   -43,    42,   -43,   -22,   -43,    65,   -43,
      19,   -43,   -43,   -43,   -43,    58,    85,    58,    58,    58,
      58,    58,    52,   -43,    47,   -43,   -43,   -43,   -43,   -43,
       3,   -43,    39,    39,   -43,   -43,   -43,   -43,    61,    69,
      95,    58,   -43,   -43,    68,   -43,   -43,    58,   -43,    90,
     -43,    75,   -43,   118,   -43,    68,   -43
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -43,   -43,   -43,    80,   -43,   -43,    53,   -43,   -43,    64,
     -43,   -43,   -43,    45,    10,   -43,    86,   -43,   -43,   -43,
     -43,    76,   -43,   -43,   -43,   -43,   -43,    41,   -43,   -42,
     -43,     9
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -32
static const yytype_int8 yytable[] =
{
      64,    75,    11,     1,    15,    66,    67,    68,    69,    70,
      71,     2,     3,    39,    16,    -2,    32,    12,     4,    15,
      62,    13,    27,    80,    28,    82,    83,    84,    85,    86,
     -31,    91,    90,    19,    20,    59,   -31,    17,    29,    18,
      30,   -31,    78,    31,    69,    70,    71,     2,     3,    99,
      34,    42,    36,   -31,     4,   103,    19,    20,    37,   -31,
      43,    38,    42,    73,   -31,    39,    44,    45,    51,    46,
      47,    43,    59,    47,    65,    88,    48,    44,    45,    72,
      46,    77,    92,    93,    47,   100,    21,    48,    67,    68,
      69,    70,    71,    67,    68,    69,    70,    71,    67,    68,
      69,    70,    71,   105,    63,    79,    95,    96,    33,    76,
       0,    55,    81,    87,   106,     0,     0,   104,     0,     0,
      97,    67,    68,    69,    70,    71
};

static const yytype_int8 yycheck[] =
{
      42,    23,    23,     1,    26,    47,     3,     4,     5,     6,
       7,     9,    10,    16,     4,     0,     1,    16,    16,    26,
      23,    16,    12,    65,    14,    67,    68,    69,    70,    71,
      15,    28,    74,    18,    19,    16,    21,     0,    16,     1,
      24,    26,    23,    25,     5,     6,     7,     9,    10,    91,
      15,     4,    24,    15,    16,    97,    18,    19,    25,    21,
      13,    27,     4,    21,    26,    16,    19,    20,    16,    22,
      26,    13,    16,    26,    26,    28,    29,    19,    20,    28,
      22,    16,    21,    14,    26,    17,     6,    29,     3,     4,
       5,     6,     7,     3,     4,     5,     6,     7,     3,     4,
       5,     6,     7,    28,    40,    60,    11,    12,    22,    56,
      -1,    35,    27,    72,   105,    -1,    -1,    27,    -1,    -1,
      25,     3,     4,     5,     6,     7
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     9,    10,    16,    31,    32,    33,    34,    37,
      40,    23,    16,    16,    41,    26,    44,     0,     1,    18,
      19,    33,    45,    46,    47,    48,    49,    44,    44,    16,
      24,    25,     1,    46,    15,    55,    24,    25,    27,    16,
      38,    39,     4,    13,    19,    20,    22,    26,    29,    51,
      59,    16,    56,    57,    50,    51,    35,    36,    44,    16,
      42,    43,    23,    39,    59,    26,    59,     3,     4,     5,
       6,     7,    28,    21,    52,    23,    36,    16,    23,    43,
      59,    27,    59,    59,    59,    59,    59,    57,    28,    53,
      59,    28,    21,    14,    54,    11,    12,    25,    58,    59,
      17,    60,    61,    59,    27,    28,    61
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 9:
#line 93 "msgparse.y"
    {
			if  (pass1)
				macro_define((yyvsp[(2) - (6)].str), (yyvsp[(3) - (6)].str), (yyvsp[(5) - (6)].mlist));
			else
				free((yyvsp[(2) - (6)].str));
		}
    break;

  case 11:
#line 103 "msgparse.y"
    {
			if  (pass1)  {
				struct  module_list  *ml;
				for  (ml = (yyvsp[(1) - (2)].mlist);  ml->ml_next;  ml = ml->ml_next)
					;
				ml->ml_next = (yyvsp[(2) - (2)].mlist);
			}
		}
    break;

  case 12:
#line 114 "msgparse.y"
    {
			if  (pass1)
				(yyval.mlist) = alloc_module((yyvsp[(2) - (2)].str), (yyvsp[(1) - (2)].str));
			else
				free((yyvsp[(2) - (2)].str));
		}
    break;

  case 13:
#line 123 "msgparse.y"
    {
			if  (pass1)
				define_helpsfor((yyvsp[(1) - (5)].str), (yyvsp[(2) - (5)].str), (yyvsp[(4) - (5)].plist));
			else
				free((yyvsp[(1) - (5)].str));
		}
    break;

  case 15:
#line 133 "msgparse.y"
    {
			if  (pass1)  {
				struct  program_list  *pl;
				for  (pl = (yyvsp[(1) - (2)].plist);  pl->pl_next;  pl = pl->pl_next)
					;
				pl->pl_next = (yyvsp[(2) - (2)].plist);
			}
		}
    break;

  case 16:
#line 144 "msgparse.y"
    {
			if  (pass1)
				(yyval.plist) = alloc_proglist((yyvsp[(1) - (1)].str));
			free((yyvsp[(1) - (1)].str));
		}
    break;

  case 17:
#line 152 "msgparse.y"
    {
			if  (pass1)
				assign_progmods((yyvsp[(2) - (6)].pgm), (yyvsp[(3) - (6)].str), (yyvsp[(5) - (6)].mlist));
			free_modlist((yyvsp[(5) - (6)].mlist));
		}
    break;

  case 18:
#line 160 "msgparse.y"
    {
				struct	program	*res = find_program((yyvsp[(1) - (1)].str));
				if  (!res)
					yyerror("Undefined program name");
				(yyval.pgm) = res;
				free((yyvsp[(1) - (1)].str));
			}
    break;

  case 20:
#line 172 "msgparse.y"
    {
			struct  module_list  *ml;
			for  (ml = (yyvsp[(1) - (2)].mlist);  ml->ml_next;  ml = ml->ml_next)
				;
			ml->ml_next = (yyvsp[(2) - (2)].mlist);
		}
    break;

  case 21:
#line 181 "msgparse.y"
    {
			(yyval.mlist) = lookupallocmods((yyvsp[(1) - (1)].str));
		}
    break;

  case 22:
#line 187 "msgparse.y"
    {
			(yyval.str) = (char *) 0;
		}
    break;

  case 23:
#line 192 "msgparse.y"
    {
			if  (pass1)
				(yyval.str) = (yyvsp[(2) - (3)].str);
			else
				free((yyvsp[(2) - (3)].str));
		}
    break;

  case 28:
#line 208 "msgparse.y"
    {
			int  lc = line_count;
			do  yylex();
			while  (line_count == lc);
			yyclearin;
		}
    break;

  case 29:
#line 217 "msgparse.y"
    {
			if  (pass1)  {
				if  ((yyvsp[(1) - (3)].vname)->vn_flags & VN_HASVALUE)
					yyerror("Symbol value redefined");
				else  {
					(yyvsp[(1) - (3)].vname)->vn_value = last_value = evaluate((yyvsp[(3) - (3)].vexpr));
					(yyvsp[(1) - (3)].vname)->vn_flags |= VN_HASVALUE;
				}
			}
			else
				last_value = evaluate((yyvsp[(3) - (3)].vexpr));
			last_assign = (yyvsp[(1) - (3)].vname);
			if  (last_expr)  {
				throwaway_expr(last_expr);
				last_expr = (struct valexpr *) 0;
			}
			throwaway_expr((yyvsp[(3) - (3)].vexpr));
		}
    break;

  case 30:
#line 238 "msgparse.y"
    {
			int	n, htcnt = 0, tlcnt = 0;
			struct  textlist  *tl;
			for  (n = 0;  n < MAXHELPTYPES && (yyvsp[(4) - (7)].str)[n];  n++)
				if  (strchr("EHPAKQRX", htlist[n]))
					htcnt++;
			for  (tl = (yyvsp[(7) - (7)].tlist);  tl;  tl = tl->tl_next)
				tlcnt++;
			if  (tlcnt != htcnt)
				yyerror("Help types and text lists do not match");

			if  (pass1) {
				unsigned  uflags = 0;
				for  (n = 0;  n < MAXHELPTYPES && (yyvsp[(4) - (7)].str)[n];  n++)
					switch  ((yyvsp[(4) - (7)].str)[n])  {
					case  'E':	uflags |= VN_DEFDE;	break;
					case  'H':	uflags |= VN_DEFDH;	break;
					case  'P':	uflags |= VN_DEFDP;	break;
					case  'Q':	uflags |= VN_DEFDQ;	break;
					case  'R':	uflags |= VN_DEFDQ|VN_DEFDR;	break;
					case  'A':	uflags |= VN_DEFDA;	break;
					case  'K':	uflags |= VN_DEFDK;	break;
					case  'N':	uflags |= VN_DEFDN;	break;
					case  'S':	uflags |= VN_DEFDS;	break;
					case  'X':	uflags |= VN_DEFDX;	break;
					}
				valname_usage((yyvsp[(5) - (7)].vexpr), uflags);
			}
			else  {
				if  ((yyvsp[(6) - (7)].num) && last_expr)
					merge_helps(last_expr, (yyvsp[(5) - (7)].vexpr));
				apphelps((yyvsp[(1) - (7)].str), (yyvsp[(3) - (7)].vexpr), (yyvsp[(4) - (7)].str), (yyvsp[(5) - (7)].vexpr), (yyvsp[(2) - (7)].flist), (yyvsp[(7) - (7)].tlist));
			}
			if  ((yyvsp[(1) - (7)].str))
				free((yyvsp[(1) - (7)].str));
			if  ((yyvsp[(2) - (7)].flist))  {
				struct	filelist  *c = (yyvsp[(2) - (7)].flist), *n;
				do  {
					n = c->next;
					free(c->name);
					free((char *) c);
				}  while  ((c = n));
			}
			throwaway_strs((yyvsp[(7) - (7)].tlist));
			if  (last_expr)
				throwaway_expr(last_expr);
			last_expr = (yyvsp[(5) - (7)].vexpr);
		}
    break;

  case 31:
#line 289 "msgparse.y"
    {	(yyval.str) = (char *) 0;	}
    break;

  case 33:
#line 295 "msgparse.y"
    {
			(yyval.vexpr) = (struct valexpr *) 0;
		}
    break;

  case 35:
#line 303 "msgparse.y"
    {
			(yyval.vexpr) = (yyvsp[(2) - (3)].vexpr);
		}
    break;

  case 36:
#line 309 "msgparse.y"
    {
			memset(htlist, '\0', sizeof(htlist));
			htlist[0] = (yyvsp[(1) - (1)].ch);
			(yyval.str) = htlist;
		}
    break;

  case 37:
#line 316 "msgparse.y"
    {
			char  *cp = htlist;
			while  (*cp)
				cp++;
			*cp = (yyvsp[(3) - (3)].ch);
			(yyval.str) = htlist;
		}
    break;

  case 38:
#line 326 "msgparse.y"
    {
			if  ((yyvsp[(2) - (2)].vexpr))  {
				if  ((yyvsp[(1) - (2)].vexpr)->val_op != VAL_NAME)
					yyerror("Cannot assign to expression");
				else  {
					struct	valname  *vn = (yyvsp[(1) - (2)].vexpr)->val_un.val_name;
					if  (pass1  &&  vn->vn_flags & VN_HASVALUE)
						yyerror("Symbol value redefined in message def");
					vn->vn_flags |= VN_HASVALUE;
					last_assign = vn;
					vn->vn_value = last_value = evaluate((yyvsp[(2) - (2)].vexpr));
				}
			}
			else
				last_value = evaluate((yyvsp[(1) - (2)].vexpr));
			(yyval.vexpr) = (yyvsp[(1) - (2)].vexpr);
		}
    break;

  case 39:
#line 346 "msgparse.y"
    {
			(yyval.num) = 0;
		}
    break;

  case 40:
#line 351 "msgparse.y"
    {
			(yyval.num) = 1;
		}
    break;

  case 41:
#line 357 "msgparse.y"
    {
			(yyval.flist) = (struct filelist *) 0;
		}
    break;

  case 42:
#line 362 "msgparse.y"
    {
			(yyval.flist) = (yyvsp[(2) - (2)].flist);
		}
    break;

  case 44:
#line 370 "msgparse.y"
    {
			(yyvsp[(3) - (3)].flist)->next = (yyvsp[(1) - (3)].flist);
			(yyval.flist) = (yyvsp[(3) - (3)].flist);
		}
    break;

  case 45:
#line 377 "msgparse.y"
    {
			if  (!((yyval.flist) = (struct filelist *) malloc(sizeof(struct filelist))))
				nomem();
			(yyval.flist)->next = (struct filelist *) 0;
			(yyval.flist)->name = (yyvsp[(1) - (1)].str);
			if  (pass1)  {
				int	cnt;
				for  (cnt = 0;  cnt < MAXHELPFILES && helpfiles[cnt].hf_name;  cnt++)
					if  (strcmp((yyvsp[(1) - (1)].str), helpfiles[cnt].hf_name) == 0)
						goto  ok;
				fprintf(stderr, "Unknown help file %s on line %d\n", (yyvsp[(1) - (1)].str), line_count);
				errors++;
			}
		ok:
			;
		}
    break;

  case 46:
#line 396 "msgparse.y"
    {
			(yyval.vexpr) = (struct valexpr *) 0;
		}
    break;

  case 47:
#line 401 "msgparse.y"
    {
			(yyval.vexpr) = make_value(last_value + 1);
		}
    break;

  case 48:
#line 406 "msgparse.y"
    {
			(yyval.vexpr) = make_value(last_value - 1);
		}
    break;

  case 49:
#line 411 "msgparse.y"
    {
			(yyval.vexpr) = (yyvsp[(2) - (2)].vexpr);
		}
    break;

  case 50:
#line 417 "msgparse.y"
    {
			(yyval.vexpr) = make_name((yyvsp[(1) - (1)].vname));
		}
    break;

  case 51:
#line 422 "msgparse.y"
    {
			(yyval.vexpr) = make_value((yyvsp[(1) - (1)].num));
		}
    break;

  case 52:
#line 427 "msgparse.y"
    {
		        (yyval.vexpr) = make_value(last_value);
		}
    break;

  case 53:
#line 432 "msgparse.y"
    {
			(yyval.vexpr) = make_value((yyvsp[(1) - (1)].ch));
			(yyval.vexpr)->val_op = VAL_CHVALUE;
		}
    break;

  case 54:
#line 438 "msgparse.y"
    {
			(yyval.vexpr) = (yyvsp[(1) - (1)].vexpr);
		}
    break;

  case 55:
#line 443 "msgparse.y"
    {
			(yyval.vexpr) = alloc_expr();
			(yyval.vexpr)->val_op = VAL_ROUND;
			(yyval.vexpr)->val_left = (yyvsp[(3) - (6)].vexpr);
			(yyval.vexpr)->val_un.val_right = (yyvsp[(5) - (6)].vexpr);
		}
    break;

  case 56:
#line 451 "msgparse.y"
    {
			(yyval.vexpr) = alloc_expr();
			(yyval.vexpr)->val_op = (yyvsp[(2) - (3)].ch);
			(yyval.vexpr)->val_left = (yyvsp[(1) - (3)].vexpr);
			(yyval.vexpr)->val_un.val_right = (yyvsp[(3) - (3)].vexpr);
		}
    break;

  case 57:
#line 459 "msgparse.y"
    {
			(yyval.vexpr) = alloc_expr();
			(yyval.vexpr)->val_op = (yyvsp[(2) - (3)].ch);
			(yyval.vexpr)->val_left = (yyvsp[(1) - (3)].vexpr);
			(yyval.vexpr)->val_un.val_right = (yyvsp[(3) - (3)].vexpr);
		}
    break;

  case 58:
#line 467 "msgparse.y"
    {
			(yyval.vexpr) = alloc_expr();
			(yyval.vexpr)->val_op = (yyvsp[(1) - (2)].ch);
			(yyval.vexpr)->val_un.val_right = (yyvsp[(2) - (2)].vexpr);
			(yyval.vexpr)->val_left = alloc_expr();
			(yyval.vexpr)->val_left->val_op = VAL_VALUE;
			(yyval.vexpr)->val_left->val_left = (struct valexpr *) 0;
			(yyval.vexpr)->val_left->val_un.val_value = 0;
		}
    break;

  case 59:
#line 478 "msgparse.y"
    {
			(yyval.vexpr) = alloc_expr();
			(yyval.vexpr)->val_op = (yyvsp[(2) - (3)].ch);
			(yyval.vexpr)->val_left = (yyvsp[(1) - (3)].vexpr);
			(yyval.vexpr)->val_un.val_right = (yyvsp[(3) - (3)].vexpr);
		}
    break;

  case 60:
#line 486 "msgparse.y"
    {
			(yyval.vexpr) = alloc_expr();
			(yyval.vexpr)->val_op = (yyvsp[(2) - (3)].ch);
			(yyval.vexpr)->val_left = (yyvsp[(1) - (3)].vexpr);
			(yyval.vexpr)->val_un.val_right = (yyvsp[(3) - (3)].vexpr);
		}
    break;

  case 61:
#line 494 "msgparse.y"
    {
			(yyval.vexpr) = alloc_expr();
			(yyval.vexpr)->val_op = (yyvsp[(2) - (3)].ch);
			(yyval.vexpr)->val_left = (yyvsp[(1) - (3)].vexpr);
			(yyval.vexpr)->val_un.val_right = (yyvsp[(3) - (3)].vexpr);
		}
    break;

  case 62:
#line 503 "msgparse.y"
    {	(yyval.tlist) = (struct textlist *) 0;	}
    break;

  case 63:
#line 506 "msgparse.y"
    {
			(yyval.tlist) = (yyvsp[(1) - (1)].tlist);
		}
    break;

  case 64:
#line 511 "msgparse.y"
    {
			if  ((yyvsp[(1) - (3)].tlist))  {
				struct  textlist  *tl;
				for  (tl = (yyvsp[(1) - (3)].tlist);  tl->tl_next;  tl = tl->tl_next)
					;
				tl->tl_next = (yyvsp[(3) - (3)].tlist);
				(yyval.tlist) = (yyvsp[(1) - (3)].tlist);
			}
			else
				(yyval.tlist) = (yyvsp[(3) - (3)].tlist);
		}
    break;

  case 65:
#line 525 "msgparse.y"
    {
			(yyval.tlist) = alloc_textlist((yyvsp[(1) - (1)].str));
		}
    break;


/* Line 1267 of yacc.c.  */
#line 1971 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



