/* -----------------------------------------------------------------------------
 * parms.cxx
 *
 *     Parameter list class.
 *
 * !!! This file is deprecated and is being replaced !!!
 *
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1998-2000.  The University of Chicago
 * Copyright (C) 1995-1998.  The University of Utah and The Regents of the
 *                           University of California.
 *
 * See the file LICENSE for information on usage and redistribution.
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swig.h"

#define MAXPARMS 16

/* ------------------------------------------------------------------------
 * NewParm()
 *
 * Create a new parameter from datatype 'type' and name 'n'.
 * ------------------------------------------------------------------------ */

Parm *NewParm(DataType *type, char *n) {
  Parm *p = NewHash();
  
  if (type) {
    Setattr(p,"type", NewVoid(CopyDataType(type), (void (*)(void *)) DelDataType));
  }
  Setattr(p,"name",n);
  return p;
}

/* ------------------------------------------------------------------------
 * CopyParm()
 * ------------------------------------------------------------------------ */

Parm *CopyParm(Parm *p) {
  DataType *t;
  char     *name;
  char     *lname;
  char     *value;
  int       ignore;

  Parm *np = NewHash();
  t = GetVoid(p,"type");
  name = GetChar(p,"name");
  lname = GetChar(p,"lname");
  value = GetChar(p,"value");
  ignore = GetInt(p,"ignore");

  Setattr(np,"type",NewVoid(CopyDataType(t), (void (*)(void *)) DelDataType));
  if (name)
    Setattr(np,"name",name);
  if (lname)
    Setattr(np,"lname", lname);
  if (value)
    Setattr(np,"value", value);
  if (ignore)
    SetInt(np,"ignore", ignore);
  return np;
}

/* ------------------------------------------------------------------
 * CopyParmList()
 * ------------------------------------------------------------------ */

ParmList *
CopyParmList(ParmList *p) {
  Parm *np;
  Parm *pp = 0;
  Parm *fp = 0;

  if (!p) return 0;

  while (p) {
    np = CopyParm(p);
    if (pp) {
      Setnext(pp,np);
    } else {
      fp = np;
    }
    pp = np;
    p = Getnext(p);
  }
  return fp;
}

/* ------------------------------------------------------------------
 * int ParmList_numarg()
 * ------------------------------------------------------------------ */

int ParmList_numarg(ParmList *p) {
  int  n = 0;
  while (p) {
    if (!Getignore(p)) n++;
    p = Getnext(p);
  }
  return n;
}

/* -----------------------------------------------------------------------------
 * int ParmList_len()
 * ----------------------------------------------------------------------------- */

int ParmList_len(ParmList *p) {
  int i = 0;
  while (p) {
    i++;
    p = Getnext(p);
  }
  return i;
}

/* ---------------------------------------------------------------------
 * ParmList_str()
 *
 * Generates a string of parameters
 * ---------------------------------------------------------------------- */

char *ParmList_str(ParmList *p) {
  static DOHString *out = 0;
  DataType *t;

  if (!out) out = NewString("");
  Clear(out);
  while(p) {
    t = Gettype(p);
    Printf(out,"%s", DataType_str(t,Getname(p)));
    p = Getnext(p);
    if (p)
      Printf(out,",");
  }
  return Char(out);
}

/* ---------------------------------------------------------------------
 * ParmList_str()
 *
 * Generate a prototype string.
 * ---------------------------------------------------------------------- */

char *ParmList_protostr(ParmList *p) {
  static DOHString *out = 0;
  DataType *t;

  if (!out) out = NewString("");
  Clear(out);
  while(p) {
    t = Gettype(p);
    Printf(out,"%s", DataType_str(t,0));
    p = Getnext(p);
    if (p)
      Printf(out,",");
  }
  return Char(out);
}



