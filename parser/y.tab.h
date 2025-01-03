/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     RW_CREATE = 258,
     RW_BUILD = 259,
     RW_REBUILD = 260,
     RW_DROP = 261,
     RW_DESTROY = 262,
     RW_PRINT = 263,
     RW_LOAD = 264,
     RW_HELP = 265,
     RW_QUIT = 266,
     RW_SELECT = 267,
     RW_INTO = 268,
     RW_WHERE = 269,
     RW_INSERT = 270,
     RW_DELETE = 271,
     RW_PRIMARY = 272,
     RW_NUMBUCKETS = 273,
     RW_ALL = 274,
     RW_FROM = 275,
     RW_AS = 276,
     RW_TABLE = 277,
     RW_AND = 278,
     RW_OR = 279,
     RW_NOT = 280,
     RW_VALUES = 281,
     INT_TYPE = 282,
     REAL_TYPE = 283,
     CHAR_TYPE = 284,
     T_EQ = 285,
     T_LT = 286,
     T_LE = 287,
     T_GT = 288,
     T_GE = 289,
     T_NE = 290,
     T_EOF = 291,
     T_STAR = 292,
     NOTOKEN = 293,
     T_INT = 294,
     T_REAL = 295,
     T_STRING = 296,
     T_QSTRING = 297,
     T_SHELL_CMD = 298
   };
#endif
/* Tokens.  */
#define RW_CREATE 258
#define RW_BUILD 259
#define RW_REBUILD 260
#define RW_DROP 261
#define RW_DESTROY 262
#define RW_PRINT 263
#define RW_LOAD 264
#define RW_HELP 265
#define RW_QUIT 266
#define RW_SELECT 267
#define RW_INTO 268
#define RW_WHERE 269
#define RW_INSERT 270
#define RW_DELETE 271
#define RW_PRIMARY 272
#define RW_NUMBUCKETS 273
#define RW_ALL 274
#define RW_FROM 275
#define RW_AS 276
#define RW_TABLE 277
#define RW_AND 278
#define RW_OR 279
#define RW_NOT 280
#define RW_VALUES 281
#define INT_TYPE 282
#define REAL_TYPE 283
#define CHAR_TYPE 284
#define T_EQ 285
#define T_LT 286
#define T_LE 287
#define T_GT 288
#define T_GE 289
#define T_NE 290
#define T_EOF 291
#define T_STAR 292
#define NOTOKEN 293
#define T_INT 294
#define T_REAL 295
#define T_STRING 296
#define T_QSTRING 297
#define T_SHELL_CMD 298




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 23 "parse.y"
{
  int ival;
  float rval;
  char *sval;
  NODE *n;
}
/* Line 1529 of yacc.c.  */
#line 142 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

