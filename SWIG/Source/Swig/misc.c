/* ----------------------------------------------------------------------------- 
 * misc.c
 *
 *     Miscellaneous functions that don't really fit anywhere else.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swig.h"
#include "swigver.h"
#include <ctype.h>

/* -----------------------------------------------------------------------------
 * Swig_copy_string()
 *
 * Duplicate a NULL-terminate string given as a char *.
 * ----------------------------------------------------------------------------- */

char *
Swig_copy_string(const char *s) {
  char *c = 0;
  if (s) {
    c = (char *) malloc(strlen(s)+1);
    strcpy(c,s);
  }
  return c;
}

/* -----------------------------------------------------------------------------
 * Swig_banner()
 *
 * Emits the SWIG identifying banner.
 * ----------------------------------------------------------------------------- */

void
Swig_banner(File *f) {
  Printf(f,
"/* ----------------------------------------------------------------------------\n\
 * This file was automatically generated by SWIG (http://www.swig.org).\n\
 * Version %s\n\
 * \n\
 * This file is not intended to be easily readable and contains a number of \n\
 * coding conventions designed to improve portability and efficiency. Do not make\n\
 * changes to this file unless you know what you are doing--modify the SWIG \n\
 * interface file instead. \n\
 * ----------------------------------------------------------------------------- */\n\n", SWIG_VERSION);

}

/* -----------------------------------------------------------------------------
 * Swig_section()
 * 
 * Print a comment denoting a section of wrapper code 
 * ----------------------------------------------------------------------------- */

void Swig_section(File *f, const String_or_char *name) {
  Printf(f,"/* -----------------------------------------------------------------------------\n");
  Printf(f," * %s\n", name);
  Printf(f," * ----------------------------------------------------------------------------- */\n");
}

/* -----------------------------------------------------------------------------
 * Swig_temp_result()
 *
 * This function is used to return a "temporary" result--a result that is only
 * guaranteed to exist for a short period of time.   Typically this is used by
 * functions that return strings and other intermediate results that are
 * used in print statements.
 *
 * Note: this is really a bit of a kludge to make it easier to work with
 * temporary variables (so that the caller doesn't have to worry about
 * memory management).   In theory, it is possible to break this if an
 * operation produces so many temporaries that it overflows the internal
 * array before they are used.   However, in practice, this would only
 * occur for very deep levels of recursion or functions taking lots of
 * parameters---neither of which occur very often in SWIG (if at all).
 * Also, a user can prevent destruction of a temporary object by increasing
 * it's reference count using DohIncref().
 *
 * It is an error to place two identical results onto this list.  It is also
 * an error for a caller to free anything returned by this function.
 *
 * Note: SWIG1.1 did something similar to this in a less-organized manner.
 * ----------------------------------------------------------------------------- */

#define MAX_RESULT  1024

static DOH       *results[MAX_RESULT];
static int        results_index = 0;
static int        results_init = 0;

DOH  *Swig_temp_result(DOH *x) {
  int i;
  if (!results_init) {
    for (i = 0; i < MAX_RESULT; i++) results[i] = 0;
    results_init = 1;
  }
  /*  Printf(stdout,"results_index = %d, %x, '%s'\n", results_index, x, x); */
  if (results[results_index]) Delete(results[results_index]);
  results[results_index] = x;
  results_index = (results_index + 1) % MAX_RESULT;
  return x;
}
  

/* -----------------------------------------------------------------------------
 * Swig_string_escape()
 *
 * Takes a string object and produces a string with escape codes added to it.
 * ----------------------------------------------------------------------------- */

String *Swig_string_escape(String *s) {
  String *ns;
  int c;
  ns = NewString("");
  
  while ((c = Getc(s)) != EOF) {
    if (c == '\n') {
      Printf(ns,"\\n");
    } else if (c == '\r') {
      Printf(ns,"\\r");
    } else if (c == '\t') {
      Printf(ns,"\\t");
    } else if (c == '\\') {
      Printf(ns,"\\\\");
    } else if (c == '\'') {
      Printf(ns,"\\'");
    } else if (c == '\"') {
      Printf(ns,"\\\"");
    } else if (c == ' ') {
      Putc(c,ns);
    } else if (!isgraph(c)) {
      Printf(ns,"\\0%o", c);
    } else {
      Putc(c,ns);
    }
  }
  return ns;
}


/* -----------------------------------------------------------------------------
 * Swig_string_upper()
 *
 * Takes a string object and convets it to all caps.
 * ----------------------------------------------------------------------------- */

String *Swig_string_upper(String *s) {
  String *ns;
  int c;
  ns = NewString("");
  
  while ((c = Getc(s)) != EOF) {
    Putc(toupper(c),ns);
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_lower()
 *
 * Takes a string object and convets it to all lower.
 * ----------------------------------------------------------------------------- */

String *Swig_string_lower(String *s) {
  String *ns;
  int c;
  ns = NewString("");
  
  while ((c = Getc(s)) != EOF) {
    Putc(tolower(c),ns);
  }
  return ns;
}


/* -----------------------------------------------------------------------------
 * Swig_string_title()
 *
 * Takes a string object and convets it to all lower.
 * ----------------------------------------------------------------------------- */

String *Swig_string_title(String *s) {
  String *ns;
  int first = 1;
  int c;
  ns = NewString("");
  
  while ((c = Getc(s)) != EOF) {
    Putc(first ? toupper(c) : tolower(c),ns);
    first = 0;
  }
  return ns;
}

/* -----------------------------------------------------------------------------
 * Swig_string_typecode()
 *
 * Takes a string with possible type-escapes in it and replaces them with
 * real C datatypes.
 * ----------------------------------------------------------------------------- */

String *Swig_string_typecode(String *s) {
  String *ns;
  int c;
  String *tc;
  ns = NewString("");
  while ((c = Getc(s)) != EOF) {
    if (c == '`') {
      tc = NewString("");
      while ((c = Getc(s)) != EOF) {
	if (c == '`') break;
	Putc(c,tc);
      }
      Printf(ns,"%s",SwigType_str(tc,0));
    } else {
      Putc(c,ns);
      if (c == '\'') {
	while ((c = Getc(s)) != EOF) {
	  Putc(c,ns);
	  if (c == '\'') break;
	  if (c == '\\') {
	    c = Getc(s);
	    Putc(c,ns);
	  }
	}
      } else if (c == '\"') {
	while ((c = Getc(s)) != EOF) {
	  Putc(c,ns);
	  if (c == '\"') break;
	  if (c == '\\') {
	    c = Getc(s);
	    Putc(c,ns);
	  }
	}
      }
    }
  }
  return ns;
}
      
/* -----------------------------------------------------------------------------
 * Swig_string_mangle()
 * 
 * Take a string and mangle it by stripping all non-valid C identifier characters
 * ----------------------------------------------------------------------------- */

String *Swig_string_mangle(String *s) {
  String *t = Copy(s);
  char *c = Char(t);
  while (*c) {
    if (!isalnum(*c)) *c = '_';
    c++;
  }
  return t;
}

/* -----------------------------------------------------------------------------
 * Swig_init()
 *
 * Initialize the SWIG core
 * ----------------------------------------------------------------------------- */

void
Swig_init() {
  /* Set some useful string encoding methods */
  DohEncoding("escape", Swig_string_escape);
  DohEncoding("upper", Swig_string_upper);
  DohEncoding("lower", Swig_string_lower);
  DohEncoding("title", Swig_string_title);
  DohEncoding("typecode",Swig_string_typecode);

  /* Initialize typemaps */
  Swig_typemap_init();

  /* Initialize symbol table */
  Swig_symbol_init();
}
