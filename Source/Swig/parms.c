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

char cvsroot_parms_c[] = "$Header$";

#include "swig.h"

#define MAXPARMS 16

/* ------------------------------------------------------------------------
 * NewParm()
 *
 * Create a new parameter from datatype 'type' and name 'n'.
 * ------------------------------------------------------------------------ */

Parm *NewParm(SwigType *type, const String_or_char *n) {
  Parm *p = NewHash();
  
  if (type) {
    Setattr(p,"type", Copy(type));
  }
  Setattr(p,"name",n);
  return p;
}

/* ------------------------------------------------------------------------
 * CopyParm()
 * ------------------------------------------------------------------------ */

Parm *CopyParm(Parm *p) {
  SwigType *t;
  String   *name;
  String   *lname;
  String   *value;
  String   *ignore;
  String   *alttype;
  String   *byname;
  String   *compactdefargs;

  Parm *np = NewHash();
  t = Getattr(p,"type");
  name = Getattr(p,"name");
  lname = Getattr(p,"lname");
  value = Getattr(p,"value");
  ignore = Getattr(p,"ignore");
  alttype = Getattr(p,"alttype");
  byname = Getattr(p, "arg:byname");
  compactdefargs = Getattr(p, "compactdefargs");

  if (t) 
    Setattr(np,"type",Copy(t));
  if (name)
    Setattr(np,"name",Copy(name));
  if (lname)
    Setattr(np,"lname", Copy(lname));
  if (value)
    Setattr(np,"value", Copy(value));
  if (ignore)
    Setattr(np,"ignore", Copy(ignore));
  if (alttype) 
    Setattr(np,"alttype", Copy(alttype));
  if (byname)
    Setattr(np, "arg:byname", Copy(byname));
  if (compactdefargs)
    Setattr(np, "compactdefargs", Copy(compactdefargs));
      
  Setfile(np,Getfile(p));
  Setline(np,Getline(p));

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
      set_nextSibling(pp,np);
    } else {
      fp = np;
    }
    pp = np;
    p = nextSibling(p);
  }
  return fp;
}

/* ------------------------------------------------------------------
 * int ParmList_numarg()
 * ------------------------------------------------------------------ */

int ParmList_numarg(ParmList *p) {
  int  n = 0;
  while (p) {
    if (!Getattr(p,"ignore")) n++;
    p = nextSibling(p);
  }
  return n;
}

/* -----------------------------------------------------------------------------
 * int ParmList_numrequired().  Return number of required arguments
 * ----------------------------------------------------------------------------- */

int ParmList_numrequired(ParmList *p) {
  int i = 0;
  while (p) {
    SwigType *t = Getattr(p,"type");
    String   *value = Getattr(p,"value");
    if (value) return i;
    if (!(SwigType_type(t) == T_VOID)) i++;
    else break;
    p = nextSibling(p);
  }
  return i;
}

/* -----------------------------------------------------------------------------
 * int ParmList_len()
 * ----------------------------------------------------------------------------- */

int ParmList_len(ParmList *p) {
  int i = 0;
  while (p) {
    i++;
    p = nextSibling(p);
  }
  return i;
}

/* ---------------------------------------------------------------------
 * ParmList_str()
 *
 * Generates a string of parameters
 * ---------------------------------------------------------------------- */

String *ParmList_str(ParmList *p) {
  String *out = NewString("");
  while(p) {
    String *pstr = SwigType_str(Getattr(p,"type"), Getattr(p,"name"));
    Printf(out,"%s", pstr);
    p = nextSibling(p);
    if (p) {
      Printf(out,",");
    }
    Delete(pstr);
  }
  return out;
}

/* ---------------------------------------------------------------------
 * ParmList_str_defaultargs()
 *
 * Generates a string of parameters including default arguments
 * ---------------------------------------------------------------------- */

String *ParmList_str_defaultargs(ParmList *p) {
  String *out = NewString("");
  while(p) {
    String *value = Getattr(p,"value");
    String *pstr = SwigType_str(Getattr(p,"type"), Getattr(p,"name"));
    Printf(out,"%s", pstr);
    if (value) {
      Printf(out,"=%s", value);
    }
    p = nextSibling(p);
    if (p) {
      Printf(out,",");
    }
    Delete(pstr);    
  }
  return out;
}

/* ---------------------------------------------------------------------
 * ParmList_protostr()
 *
 * Generate a prototype string.
 * ---------------------------------------------------------------------- */

String *ParmList_protostr(ParmList *p) {
  String *out = NewString("");
  while(p) {
    if (Getattr(p,"hidden")) {
      p = nextSibling(p);
    } else {
      String *pstr = SwigType_str(Getattr(p,"type"), 0);
      Printf(out,"%s", pstr);
      p = nextSibling(p);
      if (p) {
	Printf(out,",");
      }
      Delete(pstr);
    }
  }
  return out;
}

/* ---------------------------------------------------------------------
 * ParmList_is_compactdefargs()
 *
 * Returns 1 if the parameter list passed in is marked for compact argument
 * handling (by the "compactdefargs" attribute). Otherwise returns 0.
 * ---------------------------------------------------------------------- */

int ParmList_is_compactdefargs(ParmList *p) {
  int compactdefargs = 0;
  
  if (p) {
    compactdefargs = Getattr(p,"compactdefargs") ? 1 : 0;

    /* The "compactdefargs" attribute should only be set on the first parameter in the list.
     * However, sometimes an extra parameter is inserted at the beginning of the parameter list,
     * so we check the 2nd parameter too. */
    if (!compactdefargs) {
      Parm *nextparm = nextSibling(p);
      compactdefargs = (nextparm && Getattr(nextparm,"compactdefargs")) ? 1 : 0;
    }
  }

  return compactdefargs;
}


/* ---------------------------------------------------------------------
 * ParmList_has_defaultargs()
 *
 * Returns 1 if the parameter list passed in is has one or more default
 * arguments. Otherwise returns 0.
 * ---------------------------------------------------------------------- */

int ParmList_has_defaultargs(ParmList *p) {
    int default_args = 0;
    while (p) {
      if (Getattr(p, "value")) {
        default_args = 1;
        break;
      }
      p = nextSibling(p);
    }
    return default_args;
}

/* ---------------------------------------------------------------------
 * ParmList_copy_all_except_last_parm()
 *
 * Create a new parameter list by copying all the parameters barring the
 * last parameter.
 * ---------------------------------------------------------------------- */

ParmList *ParmList_copy_all_except_last_parm(ParmList *p) {
  ParmList* newparms = 0;
  Parm *newparm = 0;
  Parm *pp = 0;
  Parm *fp = 0;
  while (nextSibling(p)) {
    newparm = CopyParm(p);
    if (pp) {
      set_nextSibling(pp,newparm);
    } else {
      fp = newparm;
    }
    pp = newparm;
    p = nextSibling(p);
  }
  newparms = fp;
  return newparms;
}

