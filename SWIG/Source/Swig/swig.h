/* ----------------------------------------------------------------------------- 
 * swig.h
 *
 *     Header file for the SWIG core.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *             Dustin Mitchell (djmitche@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 *
 * $Header$
 * ----------------------------------------------------------------------------- */

#ifndef _SWIGCORE_H
#define _SWIGCORE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "doh.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- File interface --- */

extern void  Swig_add_directory(const DOH *dirname);
extern DOH  *Swig_last_file();
extern DOH  *Swig_search_path();
extern FILE *Swig_open(const DOH *name);
extern DOH  *Swig_read_file(FILE *f);
extern DOH  *Swig_include(const DOH *name);
extern int   Swig_insert_file(const DOH *name, DOH *outfile);

#define  SWIG_FILE_DELIMETER   "/"

/* --- Super Strings --- */

extern DOH *NewSuperString(char *s, DOH *filename, int firstline);
extern int SuperString_check(DOH *s);

/* --- Command line parsing --- */

extern void  Swig_init_args(int argc, char **argv);
extern void  Swig_mark_arg(int n);
extern void  Swig_check_options();
extern void  Swig_arg_error();

/* --- Scanner Interface --- */

typedef struct SwigScanner SwigScanner;

extern SwigScanner *NewSwigScanner();
extern void     DelSwigScanner(SwigScanner *);
extern void     SwigScanner_clear(SwigScanner *);
extern void     SwigScanner_push(SwigScanner *, DOH *);
extern void     SwigScanner_pushtoken(SwigScanner *, int);
extern int      SwigScanner_token(SwigScanner *);
extern DOH     *SwigScanner_text(SwigScanner *);
extern void     SwigScanner_skip_line(SwigScanner *);
extern int      SwigScanner_skip_balanced(SwigScanner *, int startchar, int endchar);
extern void     SwigScanner_set_location(SwigScanner *, DOH *file, int line);
extern DOH     *SwigScanner_get_file(SwigScanner *);
extern int      SwigScanner_get_line(SwigScanner *);
extern void     SwigScanner_idstart(SwigScanner *, char *idchar);

#define   SWIG_MAXTOKENS          512
#define   SWIG_TOKEN_LPAREN        1  
#define   SWIG_TOKEN_RPAREN        2
#define   SWIG_TOKEN_SEMI          3
#define   SWIG_TOKEN_COMMA         4
#define   SWIG_TOKEN_STAR          5
#define   SWIG_TOKEN_LBRACE        6
#define   SWIG_TOKEN_RBRACE        7
#define   SWIG_TOKEN_EQUAL         8
#define   SWIG_TOKEN_EQUALTO       9
#define   SWIG_TOKEN_NOTEQUAL     10
#define   SWIG_TOKEN_PLUS         11
#define   SWIG_TOKEN_MINUS        12
#define   SWIG_TOKEN_AND          13
#define   SWIG_TOKEN_LAND         14
#define   SWIG_TOKEN_OR           15
#define   SWIG_TOKEN_LOR          16
#define   SWIG_TOKEN_XOR          17
#define   SWIG_TOKEN_LESSTHAN     18
#define   SWIG_TOKEN_GREATERTHAN  19
#define   SWIG_TOKEN_LTEQUAL      20
#define   SWIG_TOKEN_GTEQUAL      21
#define   SWIG_TOKEN_NOT          22
#define   SWIG_TOKEN_LNOT         23
#define   SWIG_TOKEN_LBRACKET     24
#define   SWIG_TOKEN_RBRACKET     25
#define   SWIG_TOKEN_SLASH        26
#define   SWIG_TOKEN_BACKSLASH    27
#define   SWIG_TOKEN_ENDLINE      28
#define   SWIG_TOKEN_STRING       29
#define   SWIG_TOKEN_POUND        30
#define   SWIG_TOKEN_PERCENT      31
#define   SWIG_TOKEN_COLON        32
#define   SWIG_TOKEN_DCOLON       33
#define   SWIG_TOKEN_LSHIFT       34
#define   SWIG_TOKEN_RSHIFT       35
#define   SWIG_TOKEN_ID           36
#define   SWIG_TOKEN_FLOAT        37
#define   SWIG_TOKEN_DOUBLE       38
#define   SWIG_TOKEN_INT          39
#define   SWIG_TOKEN_UINT         40
#define   SWIG_TOKEN_LONG         41
#define   SWIG_TOKEN_ULONG        42
#define   SWIG_TOKEN_CHAR         43
#define   SWIG_TOKEN_PERIOD       44
#define   SWIG_TOKEN_AT           45
#define   SWIG_TOKEN_DOLLAR       46
#define   SWIG_TOKEN_CODEBLOCK    47
#define   SWIG_TOKEN_ILLEGAL      98
#define   SWIG_TOKEN_LAST         99 

/* --- Functions for manipulating the string-based type encoding --- */

extern void  StringType_add_pointer(DOH *t);
extern void  StringType_add_array(DOH *t, DOH *size);
extern void  StringType_add_reference(DOH *t);
extern void  StringType_add_qualifier(DOH *t, DOH *qual);
extern void  StringType_add_function(DOH *t, DOH *parms);
extern DOH  *StringType_split(DOH *t);
extern DOH  *StringType_pop(DOH *t);
extern void  StringType_push(DOH *t, DOH *s);
extern DOH  *StringType_split_parms(DOH *p);
extern DOH  *StringType_split_struct(DOH *s);
extern DOH  *StringType_split_enum(DOH *s);
extern DOH  *StringType_get_tag(DOH *s);
extern DOH  *StringType_cstr(DOH *s, DOH *id);
extern int   StringType_ispointer(DOH *t);
extern int   StringType_isreference(DOH *t);
extern int   StringType_isarray(DOH *t);
extern int   StringType_isfunction(DOH *t);
extern int   StringType_isstruct(DOH *t);
extern int   StringType_isqualifier(DOH *t);
extern DOH  *StringType_base(DOH *t);

/* --- NEW Type system --- */

   /* constructors */
DOH *SwigType_fromstring(DOH *string);
DOH *SwigType_new(int type, DOH *tag, DOH *contents, DOH *parent, int width);

DOH *SwigType_integer(int width);
DOH *SwigType_unsigned(int width);
DOH *SwigType_character(int width);
DOH *SwigType_float(int width);
DOH *SwigType_void();
DOH *SwigType_name(DOH *tag);
DOH *SwigType_enum(DOH *tag, DOH *contents);
DOH *SwigType_struct(DOH *tag, DOH *contents);
DOH *SwigType_union(DOH *tag, DOH *contents);

DOH *SwigType_array(DOH *size, DOH *parent);
DOH *SwigType_function(DOH *parameters, DOH *parent);
DOH *SwigType_pointer(DOH *parent);
DOH *SwigType_qualifier(DOH *tag, DOH *parent);

   /* accessors -- return information about the outermost
      constructor. */
int SwigType_get_type(DOH *t);
int SwigType_get_width(DOH *t);
DOH *SwigType_get_tag(DOH *t);
DOH *SwigType_get_contents(DOH *t);

   /* Type constants (returned from SwigType_gettype) */

#define SWIGTYPE_INTEGER 0
#define SWIGTYPE_UNSIGNED 1
#define SWIGTYPE_CHARACTER 2
#define SWIGTYPE_FLOAT 3
#define SWIGTYPE_VOID 4
#define SWIGTYPE_NAME 5
#define SWIGTYPE_ENUM 6
#define SWIGTYPE_STRUCT 7
#define SWIGTYPE_UNION 8
#define SWIGTYPE_CLASS 9
#define SWIGTYPE_ARRAY 10
#define SWIGTYPE_FUNCTION 11
#define SWIGTYPE_POINTER 12
#define SWIGTYPE_REFERENCE 13
#define SWIGTYPE_QUALIFIER 14

/* --- Parse tree support --- */

extern void Swig_dump_tags(DOH *obj, DOH *root);
extern void Swig_emit_rules(DOH *ruleset, DOH *context, DOH *node, 
			    int (*actionhandler)(DOH *ruleset, DOH *context, DOH *node, DOH *action));
extern DOH *Swig_match_rule(DOH *ruleset, DOH *nodelist);

/* --- Misc --- */
extern char *Swig_copy_string(const char *c);

#ifdef __cplusplus
}
#endif

#endif




