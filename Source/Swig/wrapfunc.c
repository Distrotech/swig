/* ----------------------------------------------------------------------------- 
 * wrapfunc.c
 *
 *     This file defines a object for creating wrapper functions.  Primarily
 *     this is used for convenience since it allows pieces of a wrapper function
 *     to be created in a piecemeal manner.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1998-2000.  The University of Chicago
 * Copyright (C) 1995-1998.  The University of Utah and The Regents of the
 *                           University of California.
 *
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

char cvsroot_wrapfunc_c[] = "$Header$";

#include "swig.h"
#include <ctype.h>

/* -----------------------------------------------------------------------------
 * NewWrapper()
 *
 * Create a new wrapper function object.
 * ----------------------------------------------------------------------------- */

Wrapper *
NewWrapper() {
  Wrapper *w;
  w = (Wrapper *) malloc(sizeof(Wrapper));
  w->localh = NewHash();
  w->locals = NewString("");
  w->code = NewString("");
  w->def = NewString("");
  return w;
}

/* -----------------------------------------------------------------------------
 * DelWrapper()
 *
 * Delete a wrapper function object.
 * ----------------------------------------------------------------------------- */

void
DelWrapper(Wrapper *w) {
  Delete(w->localh);
  Delete(w->locals);
  Delete(w->code);
  Delete(w->def);
  free(w);
}

/* -----------------------------------------------------------------------------
 * Wrapper_pretty_print()
 *
 * Formats a wrapper function and fixes up the indentation.
 * ----------------------------------------------------------------------------- */

void 
Wrapper_pretty_print(String *str, File *f) {
  String *ts;
  int level = 0;
  int c, i;
  int empty = 1;

  ts = NewString("");
  Seek(str,0, SEEK_SET);
  Clear(ts);
  while ((c = Getc(str)) != EOF) {
    if (c == '\"') {
      Putc(c,ts);
      while ((c = Getc(str)) != EOF) {
	if (c == '\\') {
	  Putc(c,ts);
	  c = Getc(str);
	}
	Putc(c,ts);
	if (c == '\"') break;
      }
    } else if (c == '\'') {
      Putc(c,ts);
      while ((c = Getc(str)) != EOF) {
	if (c == '\\') {
	  Putc(c,ts);
	  c = Getc(str);
	}
	Putc(c,ts);
	if (c == '\'') break;
      }
    } else if (c == '{') {
      Putc(c,ts);
      Putc('\n',ts);
      for (i = 0; i < level; i++) 
	Putc(' ',f);
      Printf(f,"%s", ts);
      Clear(ts);
      level+=4;
      while ((c = Getc(str)) != EOF) {
	if (!isspace(c)) {
	  Ungetc(c,str);
	  break;
	}
      }
    } else if (c == '}') {
      if (!empty) {
	Putc('\n',ts);
	for (i = 0; i < level; i++)
	  Putc(' ',f);
	Printf(f,"%s",ts);
	Clear(ts);
      }
      level-=4;
      Putc(c,ts);
    } else if (c == '\n') {
      Putc(c,ts);
      for (i = 0; i < level; i++)
	Putc(' ',f);
      Printf(f,"%s",ts);
      Clear(ts);
      empty = 1;
    } else if (c == '/') {
      Putc(c,ts);
      c = Getc(str);
      if (c != EOF) {
	Putc(c,ts);
	if (c == '/') {         /* C++ comment */
	  while ((c = Getc(str)) != EOF) {
	    if (c == '\n') {
	      Ungetc(c,str);
	      break;
	    }
	    Putc(c,ts);
	  }
	} else if (c == '*') {  /* C comment */
          int endstar = 0;
	  while ((c = Getc(str)) != EOF) {
	    if (endstar && c == '/') {  /* end of C comment */
	      Putc(c,ts);
	      break;
	    }
            endstar = (c == '*');
	    Putc(c,ts);
            if (c == '\n') { /* multi-line C comment. Could be improved slightly. */
              for (i = 0; i < level; i++)
	        Putc(' ',ts);
            }
	  }
        }
      }
    } else {
      if (!empty || !isspace(c)) {
	Putc(c,ts);
	empty = 0;
      }
    }
  }
  if (!empty) Printf(f,"%s",ts);
  Delete(ts);
  Printf(f,"\n");
}


/* -----------------------------------------------------------------------------
 * Wrapper_print()
 *
 * Print out a wrapper function.  Does pretty printing as well.
 * ----------------------------------------------------------------------------- */

void 
Wrapper_print(Wrapper *w, File *f) {
  String *str;

  str = NewString("");
  Printf(str,"%s\n", w->def);
  Printf(str,"%s\n", w->locals);
  Printf(str,"%s\n", w->code);
  Wrapper_pretty_print(str,f);
}

/* -----------------------------------------------------------------------------
 * Wrapper_add_local()
 *
 * Adds a new local variable declaration to a function. Returns -1 if already
 * present (which may or may not be okay to the caller).
 * ----------------------------------------------------------------------------- */

int
Wrapper_add_local(Wrapper *w, const String_or_char *name, const String_or_char *decl) {
  /* See if the local has already been declared */
  if (Getattr(w->localh,name)) {
    return -1;
  }
  Setattr(w->localh,name,decl);
  Printf(w->locals,"%s;\n", decl);
  return 0;
}

/* -----------------------------------------------------------------------------
 * Wrapper_add_localv()
 *
 * Same as add_local(), but allows a NULL terminated list of strings to be
 * used as a replacement for decl.   This saves the caller the trouble of having
 * to manually construct the 'decl' string before calling.
 * ----------------------------------------------------------------------------- */

int
Wrapper_add_localv(Wrapper *w, const String_or_char *name, ...) {
  va_list ap;
  int     ret;
  String *decl;
  DOH       *obj;
  decl = NewString("");
  va_start(ap,name);

  obj = va_arg(ap,void *);
  while (obj) {
    Printv(decl,obj,NIL);
    Putc(' ', decl);
    obj = va_arg(ap, void *);
  }
  va_end(ap);

  ret = Wrapper_add_local(w,name,decl);
  Delete(decl);
  return ret;
}

/* -----------------------------------------------------------------------------
 * Wrapper_check_local()
 *
 * Check to see if a local name has already been declared
 * ----------------------------------------------------------------------------- */

int
Wrapper_check_local(Wrapper *w, const String_or_char *name) {
  if (Getattr(w->localh,name)) {
    return 1;
  }
  return 0;
}

/* ----------------------------------------------------------------------------- 
 * Wrapper_new_local()
 *
 * Adds a new local variable with a guarantee that a unique local name will be
 * used.  Returns the name that was actually selected.
 * ----------------------------------------------------------------------------- */

char *
Wrapper_new_local(Wrapper *w, const String_or_char *name, const String_or_char *decl) {
  int i;
  String *nname = NewString(name);
  String *ndecl = NewString(decl);
  char      *ret;

  i = 0;

  while (Wrapper_check_local(w,nname)) {
    Clear(nname);
    Printf(nname,"%s%d",name,i);
    i++;
  }
  Replace(ndecl, name, nname, DOH_REPLACE_ID);
  Setattr(w->localh,nname,ndecl);
  Printf(w->locals,"%s;\n", ndecl);
  ret = Char(nname);
  Delete(nname);
  Delete(ndecl);
  return ret;      /* Note: nname should still exists in the w->localh hash */
}


/* -----------------------------------------------------------------------------
 * Wrapper_add_localv()
 *
 * Same as add_local(), but allows a NULL terminated list of strings to be
 * used as a replacement for decl.   This saves the caller the trouble of having
 * to manually construct the 'decl' string before calling.
 * ----------------------------------------------------------------------------- */

char *
Wrapper_new_localv(Wrapper *w, const String_or_char *name, ...) {
  va_list ap;
  char *ret;
  String *decl;
  DOH       *obj;
  decl = NewString("");
  va_start(ap,name);

  obj = va_arg(ap,void *);
  while (obj) {
    Printv(decl,obj,NIL);
    Putc(' ',decl);
    obj = va_arg(ap, void *);
  }
  va_end(ap);

  ret = Wrapper_new_local(w,name,decl);
  Delete(decl);
  return ret;
}






