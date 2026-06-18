/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    RW_CREATE = 258,               /* RW_CREATE  */
    RW_BUILD = 259,                /* RW_BUILD  */
    RW_REBUILD = 260,              /* RW_REBUILD  */
    RW_DROP = 261,                 /* RW_DROP  */
    RW_DESTROY = 262,              /* RW_DESTROY  */
    RW_PRINT = 263,                /* RW_PRINT  */
    RW_LOAD = 264,                 /* RW_LOAD  */
    RW_HELP = 265,                 /* RW_HELP  */
    RW_QUIT = 266,                 /* RW_QUIT  */
    RW_SELECT = 267,               /* RW_SELECT  */
    RW_INTO = 268,                 /* RW_INTO  */
    RW_WHERE = 269,                /* RW_WHERE  */
    RW_INSERT = 270,               /* RW_INSERT  */
    RW_DELETE = 271,               /* RW_DELETE  */
    RW_PRIMARY = 272,              /* RW_PRIMARY  */
    RW_NUMBUCKETS = 273,           /* RW_NUMBUCKETS  */
    RW_ALL = 274,                  /* RW_ALL  */
    RW_FROM = 275,                 /* RW_FROM  */
    RW_AS = 276,                   /* RW_AS  */
    RW_TABLE = 277,                /* RW_TABLE  */
    RW_AND = 278,                  /* RW_AND  */
    RW_OR = 279,                   /* RW_OR  */
    RW_NOT = 280,                  /* RW_NOT  */
    RW_VALUES = 281,               /* RW_VALUES  */
    INT_TYPE = 282,                /* INT_TYPE  */
    REAL_TYPE = 283,               /* REAL_TYPE  */
    CHAR_TYPE = 284,               /* CHAR_TYPE  */
    T_EQ = 285,                    /* T_EQ  */
    T_LT = 286,                    /* T_LT  */
    T_LE = 287,                    /* T_LE  */
    T_GT = 288,                    /* T_GT  */
    T_GE = 289,                    /* T_GE  */
    T_NE = 290,                    /* T_NE  */
    T_EOF = 291,                   /* T_EOF  */
    T_STAR = 292,                  /* T_STAR  */
    NOTOKEN = 293,                 /* NOTOKEN  */
    T_INT = 294,                   /* T_INT  */
    T_REAL = 295,                  /* T_REAL  */
    T_STRING = 296,                /* T_STRING  */
    T_QSTRING = 297,               /* T_QSTRING  */
    T_SHELL_CMD = 298              /* T_SHELL_CMD  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 23 "parse.y"

  int ival;
  float rval;
  char *sval;
  NODE *n;

#line 114 "y.tab.H"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
