/* -----------------------------------------------------------------------------
 * symbol.c
 *
 *     This file implements the SWIG symbol table.  See details below.
 *
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swig.h"
#include "swigwarn.h"

/* -----------------------------------------------------------------------------
 * Synopsis
 *
 * This module provides symbol table management for all of SWIG.  In previous
 * releases, the management of symbols was rather haphazard.  This module tries
 * to correct that.
 *
 * All symbols are associated with simple identifiers.  For example, here are some
 * declarations that generate symbol table entries:
 *
 *  decl                                    symbol
 *  --------------                          ------------
 *  void foo(int);                          foo
 *  int  x;                                 x
 *  typedef int *blah;                      blah
 *
 * Associated with each symbol is a Hash table that can contain any set of
 * attributes that make sense for that object.  For example:
 *
 *  typedef int *blah;             ---->    "name" : 'blah'
 *                                          "type" : 'int'
 *                                          "decl" : 'p.'
 *                                       "storage" : 'typedef'          
 * 
 * In some cases, the symbol table needs to manage overloaded entries.  For instance,
 * overloaded functions.  In this case, a linked list is built.  The "sym:nextSibling"
 * attribute is reserved to hold a link to the next entry.  For example:
 *
 * int foo(int);            --> "name" : "foo"         "name" : "foo"
 * int foo(int,double);         "type" : "int"         "type" : "int" 
 *                              "decl" : "f(int)."     "decl" : "f(int,double)."
 *                               ...                    ...
 *                   "sym:nextSibling" :  --------> "sym:nextSibling": --------> ...
 *
 * When more than one symbol has the same name, the symbol declarator is 
 * used to detect duplicates.  For example, in the above case, foo(int) and
 * foo(int,double) are different because their "decl" attribute is different.
 * However, if a third declaration "foo(int)" was made, it would generate a 
 * conflict (due to having a declarator that matches a previous entry).
 *
 * Structures and classes:
 *
 * C/C++ symbol tables are normally managed in a few different spaces.  The
 * most visible namespace is reserved for functions, variables, typedef, enum values
 * and such.  In C, a separate tag-space is reserved for 'struct name', 'class name',
 * and 'union name' declarations.   In SWIG, a single namespace is used for everything
 * this means that certain incompatibilities will arise with some C programs. For instance:
 *
 *        struct Foo {
 *             ...
 *        }
 *
 *        int Foo();       // Error. Name clash.  Works in C though 
 * 
 * Due to the unified namespace for structures, special handling is performed for
 * the following:
 *
 *        typedef struct Foo {
 *
 *        } Foo;
 * 
 * In this case, the symbol table contains an entry for the structure itself.  The
 * typedef is left out of the symbol table.
 *
 * Target language vs C:
 *
 * The symbol tables are normally managed *in the namespace of the target language*.
 * This means that name-collisions can be resolved using %rename and related 
 * directives.   A quirk of this is that sometimes the symbol tables need to
 * be used for C type resolution as well.  To handle this, each symbol table
 * also has a C-symbol table lurking behind the scenes.  This is used to locate 
 * symbols in the C namespace.  However, this symbol table is not used for error 
 * reporting nor is it used for anything else during code generation.
 *
 * Symbol table structure:
 *
 * Symbol tables themselves are a special kind of node that is organized just like
 * a normal parse tree node.  Symbol tables are organized in a tree that can be
 * traversed using the SWIG-DOM API. The following attributes names are reserved.
 *
 *     name           -- Name of the scope defined by the symbol table (if any)
 *                       This name is the C-scope name and is not affected by
 *                       %renaming operations
 *     symtab         -- Hash table mapping identifiers to nodes.
 *     csymtab        -- Hash table mapping C identifiers to nodes.
 *
 * Reserved attributes on symbol objects:
 *
 * When a symbol is placed in the symbol table, the following attributes
 * are set:
 *       
 *     sym:name             -- Symbol name
 *     sym:nextSibling      -- Next symbol (if overloaded)
 *     sym:previousSibling  -- Previous symbol (if overloaded)
 *     sym:symtab           -- Symbol table object holding the symbol
 *     sym:overloaded       -- Set to the first symbol if overloaded
 *
 * These names are modeled after XML namespaces.  In particular, every attribute 
 * pertaining to symbol table management is prefaced by the "sym:" prefix.   
 * ----------------------------------------------------------------------------- */
     
static Hash *current = 0;         /* The current symbol table hash */
static Hash *ccurrent = 0;        /* The current c symbol table hash */
static Hash *current_symtab = 0;  /* Current symbol table node */
static Hash *symtabs = 0;         /* Hash of all symbol tables by fully-qualified name */
static Hash *global_scope = 0;    /* Global scope */

/* -----------------------------------------------------------------------------
 * Swig_symbol_new()
 *
 * Create a new symbol table object
 * ----------------------------------------------------------------------------- */

void
Swig_symbol_init() {
  current = NewHash();
  current_symtab = NewHash();
  ccurrent = NewHash();
  set_nodeType(current_symtab,"symboltable");
  Setattr(current_symtab,"symtab",current);
  Setattr(current_symtab,"csymtab", ccurrent);

  /* Set the global scope */
  symtabs = NewHash();
  Setattr(symtabs,"",current_symtab);
  global_scope = current_symtab;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_setscopename()
 *
 * Set the C scopename of the current symbol table.
 * ----------------------------------------------------------------------------- */

void
Swig_symbol_setscopename(const String_or_char *name) {
  String *qname;
  assert(!Getattr(current_symtab,"name"));
  Setattr(current_symtab,"name",name);

  /* Set nested scope in parent */

  qname = Swig_symbol_qualifiedscopename(current_symtab);

  /* Save a reference to this scope */
  Setattr(symtabs,qname,current_symtab);
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_getscopename()
 *
 * Get the C scopename of the current symbol table
 * ----------------------------------------------------------------------------- */

String *
Swig_symbol_getscopename() {
  return Getattr(current_symtab,"name");
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_getscope()
 *
 * Given a fully qualified C scopename, this function returns a symbol table
 * ----------------------------------------------------------------------------- */

Symtab *
Swig_symbol_getscope(const String_or_char *name) {
  if (!symtabs) return 0;
  if (Strcmp(name,"::") == 0) name = "";
  return Getattr(symtabs,name);
}

/* ----------------------------------------------------------------------------- 
 * Swig_symbol_qualifiedscopename()
 *
 * Get the fully qualified C scopename of a symbol table.  Note, this only pertains
 * to the C/C++ scope name.  It is not affected by renaming.
 * ----------------------------------------------------------------------------- */

String *
Swig_symbol_qualifiedscopename(Symtab *symtab) {
  String *result = 0;
  Hash *parent;
  String *name;
  if (!symtab) symtab = current_symtab;
  parent = parentNode(symtab);
  if (parent) {
    result = Swig_symbol_qualifiedscopename(parent);
  }
  name = Getattr(symtab,"name");
  if (name) {
    if (!result) {
      result = NewString("");
    }
    if (Len(result)) {
      Printf(result,"::%s",name);
    } else {
      Printf(result,"%s",name);
    }
  }
  return result;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_newscope()
 *
 * Create a new scope.  Returns the newly created scope.
 * ----------------------------------------------------------------------------- */

Symtab *
Swig_symbol_newscope() 
{
  Hash *n;
  Hash *hsyms, *h;

  hsyms = NewHash();
  h = NewHash();

  set_nodeType(h,"symboltable");  
  Setattr(h,"symtab",hsyms);
  set_parentNode(h,current_symtab);
  
  n = lastChild(current_symtab);
  if (!n) {
    set_firstChild(current_symtab,h);
  } else {
    set_nextSibling(n,h);
  }
  set_lastChild(current_symtab,h);
  current = hsyms;
  ccurrent = NewHash();
  Setattr(h,"csymtab",ccurrent);
  current_symtab = h;
  return current_symtab;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_setscope()
 *
 * Set the current scope.  Returns the previous current scope.
 * ----------------------------------------------------------------------------- */

Symtab *
Swig_symbol_setscope(Symtab *sym) {
  Symtab *ret = current_symtab;
  current_symtab = sym;
  current = Getattr(sym,"symtab");
  assert(current);
  ccurrent = Getattr(sym,"csymtab");
  assert(ccurrent);
  return ret;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_popscope()
 *
 * Pop out of the current scope.  Returns the popped scope and sets the
 * scope to the parent scope.
 * ----------------------------------------------------------------------------- */

Symtab *
Swig_symbol_popscope() {
  Hash *h = current_symtab;
  current_symtab = parentNode(current_symtab);
  assert(current_symtab);
  current = Getattr(current_symtab,"symtab");
  assert(current);
  ccurrent = Getattr(current_symtab,"csymtab");
  assert(ccurrent);
  return h;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_current()
 *
 * Return the current symbol table.
 * ----------------------------------------------------------------------------- */

Symtab *
Swig_symbol_current() {
  return current_symtab;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_alias()
 *
 * Makes an alias for a symbol in the global symbol table.
 * ----------------------------------------------------------------------------- */

void
Swig_symbol_alias(String_or_char *aliasname, Symtab *s) {
  String *qname;
  qname = Swig_symbol_qualifiedscopename(current_symtab);
  if (qname) {
    Printf(qname,"::%s", aliasname);
  } else {
    qname = NewString(aliasname);
  }
  Setattr(symtabs,qname,s);
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_inherit()
 *
 * Inherit symbols from another scope.
 * ----------------------------------------------------------------------------- */

void Swig_symbol_inherit(Symtab *s) {
  List *inherit = Getattr(current_symtab,"inherit");
  if (!inherit) {
    inherit = NewList();
    Setattr(current_symtab,"inherit", inherit);
  }
  assert(s != current_symtab);
  Append(inherit,s);
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_cadd()
 *
 * Adds a node to the C symbol table only.
 * ----------------------------------------------------------------------------- */

void
Swig_symbol_cadd(String_or_char *name, Node *n) {

  Node *cn;
  /* There are a few options for weak symbols.  A "weak" symbol 
     is any symbol that can be replaced by another symbol in the C symbol
     table.  An example would be a forward class declaration.  A forward
     class sits in the symbol table until a real class declaration comes along.

     Certain symbols are marked as "sym:typename".  These are important 
     symbols related to the C++ type-system and take precedence in the C
     symbol table.  An example might be code like this:

            template<class T> T foo(T x);
            int foo(int);

     In this case, the template is marked with "sym:typename" so that it
     stays in the C symbol table (so that it can be expanded using %template).
   */

  cn = Getattr(ccurrent,name);
  if (cn && (Getattr(cn,"sym:typename"))) {
    /* The node in the C symbol table is a typename.  Do nothing */
  } else if (cn && (Getattr(cn,"sym:weak"))) {
    /* The node in the symbol table is weak. Replace it */
    Setattr(ccurrent,name, n);
  } else if (cn && (Getattr(n,"sym:weak"))) {
    /* The node being added is weak.  Don't worry about it */
  } else if (cn && (Getattr(n,"sym:typename"))) {
    /* The node being added is a typename.  We definitely add it */
    Setattr(ccurrent,name,n);
  } else if (!cn) {
    /* No conflict. Add the symbol */
    Setattr(ccurrent,name,n);
  }
}

/* ----------------------------------------------------------------------------- 
 * Swig_symbol_add()
 *
 * Adds a node to the symbol table.  Returns the node itself if successfully
 * added.  Otherwise, it returns the symbol table entry of the conflicting node.
 *
 * Also places the symbol in a behind-the-scenes C symbol table.  This is needed
 * for namespace support, type resolution, and other issues.
 * ----------------------------------------------------------------------------- */

Node *
Swig_symbol_add(String_or_char *symname, Node *n) {
  Hash *c, *cn, *cl = 0;
  SwigType *decl, *ndecl;
  String   *cstorage, *nstorage;
  int      nt = 0, ct = 0;
  int      pn = 0;
  String   *name;

  /* See if the node has a name.  If so, we place in the C symbol table for this
     scope. We don't worry about overloading here---the primary purpose of this
     is to record information for type/name resolution for later. Conflicts
     in C namespaces are errors, but these will be caught by the C++ compiler
     when compiling the wrapper code */

  
  /* There are a few options for weak symbols.  A "weak" symbol 
     is any symbol that can be replaced by another symbol in the C symbol
     table.  An example would be a forward class declaration.  A forward
     class sits in the symbol table until a real class declaration comes along.

     Certain symbols are marked as "sym:typename".  These are important 
     symbols related to the C++ type-system and take precedence in the C
     symbol table.  An example might be code like this:

            template<class T> T foo(T x);
            int foo(int);

     In this case, the template is marked with "sym:typename" so that it
     stays in the C symbol table (so that it can be expanded using %template).
   */

  name = Getattr(n,"name");
  if (name) {
    Swig_symbol_cadd(name,n);
  }
#ifdef OLD
  {
    String *name = Getattr(n,"name");
    if (name) {
      cn = Getattr(ccurrent,name);
      if (cn && (Getattr(cn,"sym:typename"))) {
	  /* The node in the C symbol table is a typename.  Do nothing */
      } else if (cn && (Getattr(cn,"sym:weak"))) {
	  /* The node in the symbol table is weak. Replace it */
	  Setattr(ccurrent,name, n);
      } else if (cn && (Getattr(n,"sym:weak"))) {
	  /* The node being added is weak.  Don't worry about it */
      } else if (cn && (Getattr(n,"sym:typename"))) {
	  /* The node being added is a typename.  We definitely add it */
	  Setattr(ccurrent,name,n);
      } else if (!cn) {
	  /* No conflict. Add the symbol */
	  Setattr(ccurrent,name,n);
      }
    }
  }
#endif
  /* No symbol name defined.  We return. */
  if (!symname) {
    Setattr(n,"sym:symtab",current_symtab);
    return n;
  }

  /* If node is ignored. We don't proceed any further */
  if (Getattr(n,"feature:ignore")) return n;

  /* See if the symbol already exists in the table */
  c = Getattr(current,symname);

  /* Check for a weak symbol.  A weak symbol is allowed to be in the
     symbol table, but is silently overwritten by other symbols.  An example
     would be a forward class declaration.  For instance:

           class Foo;

     In this case, "Foo" sits in the symbol table.  However, the
     definition of Foo would replace the entry if it appeared later. */
     
  if (c && Getattr(c,"sym:weak")) {
    c = 0;
  }
  if (c) {
    /* There is a symbol table conflict.  There are a few cases to consider here:
        (1) A conflict between a class/enum and a typedef declaration is okay.
            In this case, the symbol table entry is set to the class/enum declaration
            itself, not the typedef.   

        (2) A conflict between namespaces is okay--namespaces are open

        (3) Otherwise, overloading is only allowed for functions
    */

    /* Check for namespaces */
    if ((Strcmp(nodeType(n),nodeType(c)) == 0) && ((Strcmp(nodeType(n),"namespace") == 0))) {
      Node *cl, *pcl = 0;
      cl = c;
      while (cl) {
	pcl = cl;
	cl = Getattr(cl,"sym:nextSibling");
      }
      Setattr(pcl,"sym:nextSibling",n);
      Setattr(n,"sym:symtab", current_symtab);
      Setattr(n,"sym:name", symname);
      Setattr(n,"sym:previousSibling", pcl);
      return n;
    }
    if (Getattr(n,"allows_typedef")) nt = 1;
    if (Getattr(c,"allows_typedef")) ct = 1;
    if (nt || ct) {
      Node *td, *other;
      String *s;
      /* At least one of the nodes allows typedef overloading.  Make sure that
         both don't--this would be a conflict */

      if (nt && ct) return c;

      /* Figure out which node allows the typedef */
      if (nt) {
	td = n;
	other = c;
      } else {
	td = c;
	other = n;
      }
      /* Make sure the other node is a typedef */
      s = Getattr(other,"storage");
      if (!s || (Strcmp(s,"typedef"))) return c;  /* No.  This is a conflict */
      
      /* Hmmm.  This appears to be okay.  Make sure the symbol table refers to the allow_type node */
      
      if (td != c) {
	Setattr(current,symname, td);
	Setattr(td,"sym:symtab", current_symtab);
	Setattr(td,"sym:name", symname);
      }
      return n;
    }
     
    decl = Getattr(c,"decl");
    ndecl = Getattr(n,"decl");

    {
      String *nt1, *nt2;
      nt1 = nodeType(n);
      if (Strcmp(nt1,"template") == 0) nt1 = Getattr(n,"templatetype");
      nt2 = nodeType(c);
      if (Strcmp(nt2,"template") == 0) nt2 = Getattr(c,"templatetype");
      if (Strcmp(nt1,nt2) != 0) return c;
    }
    if ((!SwigType_isfunction(decl)) || (!SwigType_isfunction(ndecl))) {
      /* Symbol table conflict */
      return c;
    }
    
    /* Hmmm. Declarator seems to indicate that this is a function */
    /* Look at storage class to see if compatible */
    cstorage = Getattr(c,"storage");
    nstorage = Getattr(n,"storage");

    /* If either one is declared as typedef, forget it. We're hosed */
    if (Cmp(cstorage,"typedef") == 0) {
      return c;
    }
    if (Cmp(nstorage,"typedef") == 0) {
      return c;
    }
    /* Okay. Walk down the list of symbols and see if we get a declarator match */
    cn = c;
    pn = 0;
    while (cn) {
      decl = Getattr(cn,"decl");
      if (Cmp(ndecl,decl) == 0) {
	/* Declarator conflict */
	return cn;
      }
      cl = cn;
      cn = Getattr(cn,"sym:nextSibling");
      pn++;
    }

    /* Well, we made it this far.  Guess we can drop the symbol in place */
    Setattr(n,"sym:symtab",current_symtab);
    Setattr(n,"sym:name",symname);
    Setattr(n,"sym:overname", NewStringf("__SWIG_%d", pn));
    Setattr(cl,"sym:nextSibling",n);
    Setattr(n,"sym:previousSibling",cl);
    Setattr(cl,"sym:overloaded",c);
    Setattr(n,"sym:overloaded",c);
    return n;
  }

  /* No conflict.  Just add it */
  Setattr(n,"sym:symtab",current_symtab);
  Setattr(n,"sym:name",symname);
  Setattr(n,"sym:overname", NewStringf("__SWIG_%d", pn));
  Setattr(current,symname,n);
  return n;
}

/* -----------------------------------------------------------------------------
 * symbol_lookup_qualified()
 *
 * Internal function to handle fully qualified symbol table lookups.  This
 * works from the symbol table supplied in symtab and unwinds its way out
 * towards the global scope. 
 *
 * This function operates in the C namespace, not the target namespace.
 * ----------------------------------------------------------------------------- */

static Node *
symbol_lookup(String_or_char *name, Symtab *symtab) {
  Node *n;
  List *inherit;
  Hash *sym = Getattr(symtab,"csymtab");
  
  if (Getmark(symtab)) return 0;
  Setmark(symtab,1);

  n = Getattr(sym,name);
  if (n) {
    Setmark(symtab,0);
    return n;
  }
  inherit = Getattr(symtab,"inherit");
  if (inherit) {
    int  i,len;
    len = Len(inherit);
    for (i = 0; i < len; i++) {
      n = symbol_lookup(name, Getitem(inherit,i));
      if (n) {
	Setmark(symtab,0);
	return n;
      }
    }
  }
  Setmark(symtab,0);
  return 0;
}

static Node *
symbol_lookup_qualified(String_or_char *name, Symtab *symtab, String *prefix, int local) {

  /* This is a little funky, we search by fully qualified names */

  if (!symtab) return 0;
  if (!prefix) {
    Node *n;
    String *bname;
    String *prefix;
    bname = Swig_scopename_last(name);
    prefix = Swig_scopename_prefix(name);
    n = symbol_lookup_qualified(bname,symtab,prefix,local);
    Delete(bname);
    Delete(prefix);
    return n;
  } else {
    String *qname;
    Symtab *st;
    Node *n = 0;
    /* Make qualified name of current scope */
    qname = Swig_symbol_qualifiedscopename(symtab);
    if (qname && Len(qname)) {
      if (Len(prefix)) {
	Append(qname,"::");
	Append(qname,prefix);
      }
    } else {
      qname = NewString(prefix);
    }
    st = Getattr(symtabs,qname);
    Delete(qname);

    /* Found a scope match */
    if (st) {
      if (!name) return st;
      n = symbol_lookup(name, st);
    }

    if (!n) {
      if (!local) {
	return symbol_lookup_qualified(name,parentNode(symtab), prefix, local);
      } else {
	return 0;
      }
    }
    return n;
  }
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_clookup()
 *
 * Look up a symbol in the symbol table.   This uses the C name, not scripting
 * names.   Note: If we come across a using a directive, we follow it to
 * to get the real node.
 * ----------------------------------------------------------------------------- */

Node *
Swig_symbol_clookup(String_or_char *name, Symtab *n) {
  Hash *hsym;
  Node *s = 0;
  
  if (!n) {
    hsym = current_symtab;
  } else {
    if (Strcmp(nodeType(n),"symboltable")) {
      n = Getattr(n,"sym:symtab");
    }
    assert(n);
    if (n) {
      hsym = n;
    }
  }
  
  if (Swig_scopename_check(name)) {
    if (Strncmp(name,"::",2) == 0) s = symbol_lookup_qualified(Char(name)+2,global_scope,0,0);
    else {
      String *prefix = Swig_scopename_prefix(name);
      if (prefix) {
	s = symbol_lookup_qualified(name,hsym,0,0);
	Delete(prefix);
	if (!s) {
	  return 0;
	}
      }
    }
  }
  if (!s) {
    while (hsym) {
      s = symbol_lookup(name,hsym);
      if (s) break;
      hsym = parentNode(hsym);
      if (!hsym) break;
    }
  }
  if (!s) {
    return 0;
  }
  /* Check if s is a 'using' node */
  while (s && Strcmp(nodeType(s),"using") == 0) {
    Node *ss;
    ss = Swig_symbol_clookup(Getattr(s,"uname"), Getattr(s,"sym:symtab"));
    if (!ss) {
      Swig_warning(WARN_PARSE_USING_UNDEF, Getfile(s), Getline(s), "Nothing known about '%s'.\n", Getattr(s,"uname"));
    }
    s = ss;
  }
  return s;
}

Node *
Swig_symbol_clookup_local(String_or_char *name, Symtab *n) {
  Hash *h, *hsym;
  Node *s = 0;

  if (!n) {
    hsym = current_symtab;
    h = ccurrent;
  } else {
    if (Strcmp(nodeType(n),"symboltable")) {
      n = Getattr(n,"sym:symtab");
    }
    assert(n);
    hsym = n;
    h = Getattr(n,"csymtab");
  }

  if (Swig_scopename_check(name)) {
    if (Strncmp(name,"::",2) == 0) {
      s = symbol_lookup_qualified(Char(name)+2,global_scope,0,0);
    } else {
      s = symbol_lookup_qualified(name,hsym,0,0);
    }
  }
  if (!s)
    s = symbol_lookup(name,hsym);
  if (!s) return 0;
  /* Check if s is a 'using' node */
  while (s && Strcmp(nodeType(s),"using") == 0) {
    Node *ss = Swig_symbol_clookup_local(Getattr(s,"uname"), Getattr(s,"sym:symtab"));
    if (!ss) {
      Swig_warning(WARN_PARSE_USING_UNDEF, Getfile(s), Getline(s), "Nothing known about '%s'.\n", Getattr(s,"uname"));
    }
    s = ss;
  }
  return s;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_cscope()
 *
 * Look up a scope name.
 * ----------------------------------------------------------------------------- */

Symtab *
Swig_symbol_cscope(String_or_char *name, Symtab *symtab) {
  if (Strncmp(name,"::",2) == 0) return symbol_lookup_qualified(0, global_scope, name, 0);
  return symbol_lookup_qualified(0,symtab,name,0);
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_remove()
 *
 * Remove a symbol 
 * ----------------------------------------------------------------------------- */

void
Swig_symbol_remove(Node *n) {
  Symtab  *symtab; 
  String  *symname;
  Node    *symprev;
  Node    *symnext;
  symtab  = Getattr(n,"sym:symtab");        /* Get symbol table object */
  symtab  = Getattr(symtab,"symtab");       /* Get actual hash table of symbols */
  symname = Getattr(n,"sym:name");
  symprev = Getattr(n,"sym:previousSibling");
  symnext = Getattr(n,"sym:nextSibling");

  /* If previous symbol, just fix the links */
  if (symprev) {
    if (symnext) {
      Setattr(symprev,"sym:nextSibling",symnext);
    } else {
      Delattr(symprev,"sym:nextSibling");
    }
  } else {
    /* If no previous symbol, see if there is a next symbol */
    if (symnext) {
      Setattr(symtab,symname,symnext);
    } else {
      Delattr(symtab,symname);
    }
  }
  Delattr(n,"sym:symtab");
  Delattr(n,"sym:previousSibling");
  Delattr(n,"sym:nextSibling");
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_qualified()
 *
 * Return the qualified name of a symbol
 * ----------------------------------------------------------------------------- */

String *
Swig_symbol_qualified(Node *n) {
  Hash *symtab;
  if (Strcmp(nodeType(n),"symboltable") == 0) {
    symtab = n;
  } else {
    symtab = Getattr(n,"sym:symtab");
  }
  if (!symtab) return NewString("");
  return Swig_symbol_qualifiedscopename(symtab);
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_isoverloaded()
 * 
 * Check if a symbol is overloaded.  Returns the first symbol if so.
 * ----------------------------------------------------------------------------- */

Node *
Swig_symbol_isoverloaded(Node *n) {
  return Getattr(n,"sym:overloaded");
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_type_qualify()
 *
 * Create a fully qualified type name
 * ----------------------------------------------------------------------------- */

SwigType *
Swig_symbol_type_qualify(SwigType *t, Symtab *st) {
  List   *elements;
  String *result;
  int     i,len;

  result = NewString("");
  elements = SwigType_split(t);
  len = Len(elements);
  for (i = 0; i < len; i++) {
    String *e = Getitem(elements,i);
    if (SwigType_issimple(e)) {
      Node *n = Swig_symbol_clookup(e,st);
      if (n) {
	String *name = Getattr(n,"name");
	Clear(e);
	Append(e,name);
	{
	  String *qname = Swig_symbol_qualified(n);
	  if (Len(qname)) {
	    Insert(e,0,"::");
	    Insert(e,0,qname);
	  }
	  Delete(qname);
	}
      }
      if (Strncmp(e,"::",2) == 0) {
	Delitem(e,0);
	Delitem(e,0);
      }
      Append(result,e);
    } else if (SwigType_isfunction(e)) {
      List *parms = SwigType_parmlist(e);
      String *s = NewString("f(");
      String *p;
      p = Firstitem(parms);
      while (p) {
	Append(s,Swig_symbol_type_qualify(p,st));
	p = Nextitem(parms);
	if (p) {
	  Append(s,",");
	}
      }
      Append(s,").");
      Append(result,s);
      Delete(s);
    } else {
      Append(result,e);
    }
  }
  Delete(elements);
  return result;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_typedef_reduce()
 *
 * Chase a typedef through symbol tables looking for a match.
 * ----------------------------------------------------------------------------- */

SwigType *Swig_symbol_typedef_reduce(SwigType *ty, Symtab *tab) {
  SwigType *prefix, *base;
  Node *n;

  base = SwigType_base(ty);
  prefix = SwigType_prefix(ty);

  n = Swig_symbol_clookup(base,tab);
  if (!n) {
    Delete(base);
    Delete(prefix);
    return Copy(ty);
  }
  if (Strcmp(nodeType(n),"using") == 0) {
    String *uname = Getattr(n,"uname");
    if (uname) {
      n = Swig_symbol_clookup(base,Getattr(n,"sym:symtab"));
      if (!n) {
	Delete(base);
	Delete(prefix);
	return Copy(ty);
      }
    } 
  }
  if (Strcmp(nodeType(n),"cdecl") == 0) {
    String *storage = Getattr(n,"storage");
    if (Strcmp(storage,"typedef") == 0) {
      SwigType *decl;
      SwigType *rt;
      SwigType *nt = Copy(Getattr(n,"type"));
      decl = Getattr(n,"decl");
      if (decl) {
	SwigType_push(nt,decl);
      }
      SwigType_push(nt,prefix);
      Delete(base);
      Delete(prefix);
      rt = Swig_symbol_typedef_reduce(nt, Getattr(n,"sym:symtab"));
      Delete(nt);
      return rt;
    }
  }
  Delete(base);
  Delete(prefix);
  return Copy(ty);
}

