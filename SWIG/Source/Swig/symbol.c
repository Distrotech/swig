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

char cvsroot_symbol_c[] = "$Header$";

#include "swig.h"
#include "swigwarn.h"
#include <ctype.h>

/*#define SWIG_DEBUG*/
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
 *
 * An example dump of the parse tree showing symbol table entries for the 
 * following code should clarify this:
 *
 *   namespace OuterNamespace {
 *       namespace InnerNamespace {
 *           class Class {
 *           };
 *           struct Struct {
 *               int Var;
 *           };
 *       }
 *    }
 *
 *   +++ namespace ----------------------------------------
 *   | sym:name     - "OuterNamespace"
 *   | symtab       - 0xa064bf0
 *   | sym:symtab   - 0xa041690
 *   | sym:overname - "__SWIG_0"
 *  
 *         +++ namespace ----------------------------------------
 *         | sym:name     - "InnerNamespace"
 *         | symtab       - 0xa064cc0
 *         | sym:symtab   - 0xa064bf0
 *         | sym:overname - "__SWIG_0"
 *  
 *               +++ class ----------------------------------------
 *               | sym:name     - "Class"
 *               | symtab       - 0xa064d80
 *               | sym:symtab   - 0xa064cc0
 *               | sym:overname - "__SWIG_0"
 *               | 
 *               +++ class ----------------------------------------
 *               | sym:name     - "Struct"
 *               | symtab       - 0xa064f00
 *               | sym:symtab   - 0xa064cc0
 *               | sym:overname - "__SWIG_0"
 *  
 *                     +++ cdecl ----------------------------------------
 *                     | sym:name     - "Var"
 *                     | sym:symtab   - 0xa064f00
 *                     | sym:overname - "__SWIG_0"
 *                     | 
 *  
 *
 * Each class and namespace has its own scope and thus a new symbol table (sym)
 * is created. The sym attribute is only set for the first entry in the symbol
 * table. The sym:symtab entry points to the symbol table in which the symbol
 * exists, so for example, Struct is in the scope OuterNamespace::InnerNamespace
 * so sym:symtab points to this symbol table (0xa064cc0).
 *
 * ----------------------------------------------------------------------------- */
     
static Hash *current = 0;         /* The current symbol table hash */
static Hash *ccurrent = 0;        /* The current c symbol table hash */
static Hash *current_symtab = 0;  /* Current symbol table node */
static Hash *symtabs = 0;         /* Hash of all symbol tables by fully-qualified name */
static Hash *global_scope = 0;    /* Global scope */

#if 0
void
Swig_symbol_dump_symtable() {
  Printf(stdout, "DUMPING SYMTABLE start =======================================\n");
  {
    Hash* cst = Getattr(current_symtab, "csymtab");
    Swig_print_tree(cst);
    /*
    Swig_print_tree(Getattr(cst, "NumSpace"));
    */
  }
  Printf(stdout, "DUMPING SYMTABLE end   =======================================\n");
}
#endif

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
  if (!Getattr(symtabs,qname)) {
    Setattr(symtabs,qname,s);
  }
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_inherit()
 *
 * Inherit symbols from another scope.
 * ----------------------------------------------------------------------------- */

void Swig_symbol_inherit(Symtab *s) {
  int i;
  List *inherit = Getattr(current_symtab,"inherit");
  if (!inherit) {
    inherit = NewList();
    Setattr(current_symtab,"inherit", inherit);
  }
  assert(s != current_symtab);
  for (i = 0; i < Len(inherit); i++) {
    Node *n = Getitem(inherit,i);
    if (n == s) return;              /* Already inherited */
  }
  Append(inherit,s);
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_cadd()
 *
 * Adds a node to the C symbol table only.
 * ----------------------------------------------------------------------------- */

void
Swig_symbol_cadd(String_or_char *name, Node *n) {
  Node *append = 0;

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

  if (!name) return;
  if (SwigType_istemplate(name)) {
    String *dname = Swig_symbol_template_deftype(name,0);
    if (Strcmp(dname,name)) {    
      Swig_symbol_cadd(dname, n);
    }
    Delete(dname);
  }

#ifdef SWIG_DEBUG
  Printf(stderr,"symbol_cadd %s %x\n", name, n);
#endif
  cn = Getattr(ccurrent,name);

  if (cn && (Getattr(cn,"sym:typename"))) {
    /* The node in the C symbol table is a typename.  Do nothing */
    /* We might append the symbol at the end */
    append = n;
  } else if (cn && (Getattr(cn,"sym:weak"))) {
    /* The node in the symbol table is weak. Replace it */
    Setattr(ccurrent,name, n);
  } else if (cn && (Getattr(n,"sym:weak"))) {
    /* The node being added is weak.  Don't worry about it */
  } else if (cn && (Getattr(n,"sym:typename"))) {
    /* The node being added is a typename.  We definitely add it */
    Setattr(ccurrent,name,n);
    append = cn;
  } else if (cn && (Strcmp(nodeType(cn),"templateparm") == 0)) {
    Swig_error(Getfile(n),Getline(n),
	       "Declaration of '%s' shadows template parameter,\n",
	       name);
    Swig_error(Getfile(cn),Getline(cn),
	       "previous template parameter declaration '%s'.\n",
	       name);
    return;
  } else if (cn) {
    append = n;
  } else if (!cn) {
    /* No conflict. Add the symbol */
    Setattr(ccurrent,name,n);
  }

  /* Multiple entries in the C symbol table.   We append to to the symbol table */
  if (append) {
    Node *fn, *pn = 0;
    cn = Getattr(ccurrent,name);
    fn = cn;
    while (fn) {
      pn = fn;
      if (fn == append) {
	/* already added. Bail */
	return;
      }
      fn = Getattr(fn,"csym:nextSibling");
    }
    if (pn) {
      Setattr(pn,"csym:nextSibling",append);
    }
  }

  /* Special typedef handling.  When a typedef node is added to the symbol table, we
     might have to add a type alias.   This would occur if the typedef mapped to another
     scope in the system.  For example:

           class Foo {
           };

           typedef Foo OtherFoo;

     In this case, OtherFoo becomes an alias for Foo. */

  {
    Node *td = n;
    while (td && (Strcmp(nodeType(td),"cdecl") == 0) && (checkAttribute(td,"storage","typedef"))) {
      SwigType *type;
      Node *td1;
      type = Copy(Getattr(td,"type"));
      SwigType_push(type,Getattr(td,"decl"));
      td1 = Swig_symbol_clookup(type,0);
      Delete(type);
      if (td1 == td) break;
      td = td1;
      if (td) {
	Symtab *st = Getattr(td,"symtab");
	if (st) {
	  Swig_symbol_alias(Getattr(n,"name"),st);
	  break;
	}
      }
    }
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
  int      u1 = 0, u2 = 0;
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
  if (name && Len(name)) {
    Swig_symbol_cadd(name,n);
  }

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
      if (Strcmp(nt1,"using") == 0) u1 = 1;
      if (Strcmp(nt2,"using") == 0) u2 = 1;

      if ((Strcmp(nt1,nt2) != 0) && !(u1 || u2)) return c;
    }
    if (!(u1 || u2)) {
      if ((!SwigType_isfunction(decl)) || (!SwigType_isfunction(ndecl))) {
	/* Symbol table conflict */
	return c;
      }
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
    {
      int n_template = (Strcmp(nodeType(n),"template") == 0) && (Strcmp(Getattr(n,"templatetype"),"cdecl") == 0);
      int n_plain_cdecl = (Strcmp(nodeType(n),"cdecl") == 0);
      cn = c;
      pn = 0;
      while (cn) {
        decl = Getattr(cn,"decl");
        if (!(u1 || u2)) {
          if (Cmp(ndecl,decl) == 0) {
            /* Declarator conflict */
            /* Now check we don't have a non-templated function overloaded by a templated function with same params,
             * eg void foo(); template<typename> void foo(); */
            int cn_template = (Strcmp(nodeType(cn),"template") == 0) && (Strcmp(Getattr(cn,"templatetype"),"cdecl") == 0);
            int cn_plain_cdecl = (Strcmp(nodeType(cn),"cdecl") == 0);
            if (!((n_template && cn_plain_cdecl) || (cn_template && n_plain_cdecl))) {
              /* found a conflict */
              return cn;
            }
          }
        }
        cl = cn;
        cn = Getattr(cn,"sym:nextSibling");
        pn++;
      }
    }
    /* Well, we made it this far.  Guess we can drop the symbol in place */
    Setattr(n,"sym:symtab",current_symtab);
    Setattr(n,"sym:name",symname);
    /* Printf(stdout,"%s %x\n", Getattr(n,"sym:overname"), current_symtab); */
    assert(!Getattr(n,"sym:overname"));
    Setattr(n,"sym:overname", NewStringf("__SWIG_%d", pn));
    /*Printf(stdout,"%s %s %s\n", symname, Getattr(n,"decl"), Getattr(n,"sym:overname")); */
    Setattr(cl,"sym:nextSibling",n);
    Setattr(n,"sym:previousSibling",cl);
    Setattr(cl,"sym:overloaded",c);
    Setattr(n,"sym:overloaded",c);
    return n;
  }

  /* No conflict.  Just add it */
  Setattr(n,"sym:symtab",current_symtab);
  Setattr(n,"sym:name",symname);
  /* Printf(stdout,"%s\n", Getattr(n,"sym:overname")); */
  Setattr(n,"sym:overname", NewStringf("__SWIG_%d", pn));
  /* Printf(stdout,"%s %s %s\n", symname, Getattr(n,"decl"), Getattr(n,"sym:overname")); */
  Setattr(current,symname,n);
  return n;
}

/* -----------------------------------------------------------------------------
 * symbol_lookup()
 *
 * Internal function to handle fully qualified symbol table lookups.  This
 * works from the symbol table supplied in symtab and unwinds its way out
 * towards the global scope. 
 *
 * This function operates in the C namespace, not the target namespace.
 *
 * The check function is an optional callback that can be used to verify a particular
 * symbol match.   This is only used in some of the more exotic parts of SWIG. For instance,
 * verifying that a class hierarchy implements all pure virtual methods.
 * ----------------------------------------------------------------------------- */

static Node *
symbol_lookup(String_or_char *name, Symtab *symtab, int (*check)(Node *n)) {
  Node *n;
  List *inherit;
  Hash *sym = Getattr(symtab,"csymtab");
  if (Getmark(symtab)) return 0;
  Setmark(symtab,1);


  n = Getattr(sym,name);

#ifdef SWIG_DEBUG
  Printf(stderr,"symbol_look %s %x %x %s\n", name, n, symtab, Getattr(symtab,"name"));
#endif
  
  if (n) {
    /* if a check-function is defined.  Call it to determine a match */
    if (check) {
      int c = check(n);
      if (c == 1) {
	Setmark(symtab,0);
	return n;
      }
      if (c < 0) {
	/* Terminate the search right away */
	Setmark(symtab,0);
	return 0;
      }
    } else {
      Setmark(symtab,0);
      return n;
    }
  }

  if (!n && SwigType_istemplate(name)) {
    String *dname = 0;
    Setmark(symtab,0);
    dname = Swig_symbol_template_deftype(name,symtab);
    if (Strcmp(name,dname)) {
      n = symbol_lookup(dname, symtab, check);
    }
    Delete(dname);
    if (n) return n;
  }

  inherit = Getattr(symtab,"inherit");
  if (inherit) {
    int  i,len;
    len = Len(inherit);
    for (i = 0; i < len; i++) {
      n = symbol_lookup(name, Getitem(inherit,i),check);
      if (n) {
	Setmark(symtab,0);
	return n;
      }
    }
  }

  Setmark(symtab,0);
  return 0;
}

/* -----------------------------------------------------------------------------
 * symbol_lookup_qualified()
 * ----------------------------------------------------------------------------- */

static Node *
symbol_lookup_qualified(String_or_char *name, Symtab *symtab, String *prefix, int local, int (*checkfunc)(Node *n)) {
  /* This is a little funky, we search by fully qualified names */

  if (!symtab) return 0;
  if (!prefix) {
    Node *n;
    String *bname;
    String *prefix;
    bname = Swig_scopename_last(name);
    prefix = Swig_scopename_prefix(name);
    n = symbol_lookup_qualified(bname,symtab,prefix,local,checkfunc);
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
    /* Found a scope match */
    if (st) {
      if (!name) return st;
      n = symbol_lookup(name, st,checkfunc);
    }

    Delete(qname);
    if (!n) {
      if (!local) {
	Node *pn = parentNode(symtab);
	if (pn) n = symbol_lookup_qualified(name,pn, prefix, local,checkfunc);
      } else {
	n = 0;
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
  Hash *hsym = 0;
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
    if (Strncmp(name,"::",2) == 0) {
      String *nname = NewString(Char(name)+2);
      if (Swig_scopename_check(nname)) {
	s = symbol_lookup_qualified(nname,global_scope,0,0,0);
      }
    } else {
      String *prefix = Swig_scopename_prefix(name);
      if (prefix) {
	s = symbol_lookup_qualified(name,hsym,0,0,0);
	Delete(prefix);
	if (!s) {
	  return 0;
	}
      }
    }
  }
  if (!s) {
    while (hsym) {
      s = symbol_lookup(name,hsym,0);
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

/* -----------------------------------------------------------------------------
 * Swig_symbol_clookup_check()
 *
 * This function is identical to Swig_symbol_clookup() except that it
 * accepts a callback function that is invoked to determine a symbol match.
 * The purpose of this function is to support complicated algorithms that need
 * to examine multiple definitions of the same symbol that might appear in an
 * inheritance hierarchy. 
 * ----------------------------------------------------------------------------- */

Node *
Swig_symbol_clookup_check(String_or_char *name, Symtab *n, int (*checkfunc)(Node *n)) {
  Hash *hsym = 0;
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
    if (Strncmp(name,"::",2) == 0) {
      String *nname = NewString(Char(name)+2);
      if (Swig_scopename_check(nname)) {
	s = symbol_lookup_qualified(nname,global_scope,0,0,checkfunc);
      }
      Delete(nname);
    } else {
      String *prefix = Swig_scopename_prefix(name);
      if (prefix) {
	s = symbol_lookup_qualified(name,hsym,0,0,checkfunc);
	Delete(prefix);
	if (!s) {
	  return 0;
	}
      }
    }
  }
  if (!s) {
    while (hsym) {
      s = symbol_lookup(name,hsym,checkfunc);
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
    if (!ss && !checkfunc) {
      Swig_warning(WARN_PARSE_USING_UNDEF, Getfile(s), Getline(s), "Nothing known about '%s'.\n", Getattr(s,"uname"));
    }
    s = ss;
  }
  return s;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_clookup_local()
 * ----------------------------------------------------------------------------- */

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
      String *nname = NewString(Char(name)+2);
      if (Swig_scopename_check(nname)) {
	s = symbol_lookup_qualified(nname,global_scope,0,0,0);
      }
      Delete(nname);
    } else {
      s = symbol_lookup_qualified(name,hsym,0,0,0);
    }
  }
  if (!s) {
    s = symbol_lookup(name,hsym,0);
  }
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
 * Swig_symbol_clookup_local_check()
 * ----------------------------------------------------------------------------- */

Node *
Swig_symbol_clookup_local_check(String_or_char *name, Symtab *n, int (*checkfunc)(Node *)) {
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
      String *nname = NewString(Char(name)+2);
      if (Swig_scopename_check(nname)) {
	s = symbol_lookup_qualified(nname,global_scope,0,0,checkfunc);
      }
      Delete(nname);
    } else {
      s = symbol_lookup_qualified(name,hsym,0,0,checkfunc);
    }
  }
  if (!s) {
    s = symbol_lookup(name,hsym,checkfunc);
  }
  if (!s) return 0;
  /* Check if s is a 'using' node */
  while (s && Strcmp(nodeType(s),"using") == 0) {
    Node *ss = Swig_symbol_clookup_local_check(Getattr(s,"uname"), Getattr(s,"sym:symtab"),checkfunc);
    if (!ss && !checkfunc) {
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
  if (Strncmp(name,"::",2) == 0) return symbol_lookup_qualified(0, global_scope, name, 0,0);
  return symbol_lookup_qualified(0,symtab,name,0,0);
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_remove()
 *
 * Remove a symbol. If the symbol is an overloaded function and the symbol removed
 * is not the last in the list of overloaded functions, then the overloaded
 * names (sym:overname attribute) are changed to start from zero, eg __SWIG_0.
 * ----------------------------------------------------------------------------- */

void
Swig_symbol_remove(Node *n) {
  Symtab  *symtab; 
  String  *symname;
  Node    *symprev;
  Node    *symnext;
  Node    *fixovername = 0;
  symtab  = Getattr(n,"sym:symtab");        /* Get symbol table object */
  symtab  = Getattr(symtab,"symtab");       /* Get actual hash table of symbols */
  symname = Getattr(n,"sym:name");
  symprev = Getattr(n,"sym:previousSibling");
  symnext = Getattr(n,"sym:nextSibling");

  /* If previous symbol, just fix the links */
  if (symprev) {
    if (symnext) {
      Setattr(symprev,"sym:nextSibling",symnext);
      fixovername = symprev; /* fix as symbol to remove is somewhere in the middle of the linked list */
    } else {
      Delattr(symprev,"sym:nextSibling");
    }
  } else {
    /* If no previous symbol, see if there is a next symbol */
    if (symnext) {
      Setattr(symtab,symname,symnext);
      fixovername = symnext; /* fix as symbol to remove is at head of linked list */
    } else {
      Delattr(symtab,symname);
    }
  }
  if (symnext) {
    if (symprev) {
      Setattr(symnext,"sym:previousSibling",symprev);
    } else {
      Delattr(symnext,"sym:previousSibling");
    }
  }
  Delattr(n,"sym:symtab");
  Delattr(n,"sym:previousSibling");
  Delattr(n,"sym:nextSibling");
  Delattr(n,"csym:nextSibling");
  Delattr(n,"sym:overname");
  Delattr(n,"csym:previousSibling");
  Delattr(n,"sym:overloaded");
  n = 0;

  if (fixovername) {
    Node *nn = fixovername;
    Node *head = fixovername;
    int pn = 0;

    /* find head of linked list */
    while (nn) {
      head = nn;
      nn = Getattr(nn, "sym:previousSibling");
    }

    /* adjust all the sym:overname strings to start from 0 and increment by one */
    nn = head;
    while (nn) {
      assert(Getattr(nn,"sym:overname"));
      Delattr(nn,"sym:overname");
      Setattr(nn,"sym:overname", NewStringf("__SWIG_%d", pn));
      pn++;
      nn = Getattr(nn,"sym:nextSibling");
    }
  }
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
#ifdef SWIG_DEBUG
  Printf(stderr,"symbol_qscope %s %x %s\n", Getattr(n,"name"), symtab,Getattr(symtab,"name"));
#endif
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

static int no_constructor(Node *n) {
  SwigType *type = nodeType(n);
#ifdef SWIG_DEBUG
  Printf(stderr,"node type %s\n", Getattr(n,"name"), type);
#endif
  return type ? (Strcmp(type,"constructor") != 0): 1;
}

static SwigType *
Swig_symbol_template_qualify(const SwigType *e, Symtab *st) {
  String *tprefix, *tsuffix;
  SwigType *qprefix;
  List   *targs;
  Iterator ti;
  tprefix = SwigType_templateprefix(e);
  tsuffix = SwigType_templatesuffix(e);
  qprefix = Swig_symbol_type_qualify(tprefix,st);
  targs = SwigType_parmlist(e);
  Printf(qprefix,"<(");
  for (ti = First(targs); ti.item;) {
    String *qparm = Swig_symbol_type_qualify(ti.item,st);
    String *vparm = Swig_symbol_template_param_eval(qparm, st);
    Append(qprefix,vparm);
    ti = Next(ti);
    if (ti.item) {
      Putc(',',qprefix);
    }
    Delete(qparm);
    Delete(vparm);
  }
  Append(qprefix,")>");
  Append(qprefix,tsuffix);
  Delete(tprefix);
  Delete(tsuffix);
#ifdef SWIG_DEBUG
  Printf(stderr,"symbol_temp_qual %s %s\n", e, qprefix);
#endif
  return qprefix;
}

SwigType *
Swig_symbol_type_qualify(const SwigType *t, Symtab *st) {
  List   *elements;
  String *result;
  int     i,len;

  result = NewString("");
  elements = SwigType_split(t);

  len = Len(elements);
  for (i = 0; i < len; i++) {
    String *e = Getitem(elements,i);
    if (SwigType_issimple(e)) {
      Node *n = Swig_symbol_clookup_check(e,st,no_constructor);
      if (n) {
	String *name = Getattr(n,"name");
	Clear(e);
	Append(e,name);
#ifdef SWIG_DEBUG
	Printf(stderr,"symbol_qual_ei %d %s %s %x\n", i, name, e, st);
#endif
	if (!Swig_scopename_check(name)) {
	  String *qname = Swig_symbol_qualified(n);
	  if (Len(qname)) {
	    Insert(e,0,"::");
	    Insert(e,0,qname);
	  }
#ifdef SWIG_DEBUG
	  Printf(stderr,"symbol_qual_sc %d %s %s %x\n", i, qname, e, st);
#endif
	  Delete(qname);
	}
      } else if (SwigType_istemplate(e)) {
	SwigType *ty = Swig_symbol_template_qualify(e,st);
	Clear(e);
	Append(e,ty);
      }
      if (Strncmp(e,"::",2) == 0) {
	Delitem(e,0);
	Delitem(e,0);
      }
      Append(result,e);
    } else if (SwigType_isfunction(e)) {
      Iterator pi;
      List *parms = SwigType_parmlist(e);
      String *s = NewString("f(");
      pi = First(parms);
      while (pi.item) {
	String *pf = Swig_symbol_type_qualify(pi.item,st);
	Append(s,pf);
	pi = Next(pi);
	if (pi.item) {
	  Append(s,",");
	}
	Delete(pf);
      }
      Append(s,").");
      Append(result,s);
      Delete(s);
    } else {
      Append(result,e);
    }
  }
  Delete(elements);
#ifdef SWIG_DEBUG
  Printf(stderr,"symbol_qualify %s %s %x %s\n", t, result, st, st ?Getattr(st,"name"): 0);
#endif
  return result;
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_typedef_reduce()
 *
 * Chase a typedef through symbol tables looking for a match.
 * ----------------------------------------------------------------------------- */

static 
SwigType *Swig_symbol_template_reduce(SwigType *qt, Symtab *ntab)
{
  Iterator pi;
  Parm *p;
  List *parms = SwigType_parmlist(qt);
  String *tprefix = SwigType_templateprefix(qt);
  String *tsuffix = SwigType_templatesuffix(qt);
  String *qprefix = SwigType_typedef_qualified(tprefix);
  Printv(qprefix,"<(",NIL);
  pi = First(parms);
  while ((p = pi.item)) {
    String *np;
    String *tp = Swig_symbol_typedef_reduce(p, ntab);
    String *qp = Swig_symbol_type_qualify(tp, ntab);
    Node *n = Swig_symbol_clookup(qp,ntab);
    if (n) {
      String *qual = Swig_symbol_qualified(n);
      np = Copy(Getattr(n,"name"));
      tp = np;
      if (qual) {
	Insert(np,0,"::");
	Insert(np,0,qual);
	Delete(qual);
      }
    } else {
      np = qp;
    }
    Append(qprefix,np);
    pi= Next(pi);
    if (pi.item) {
      Append(qprefix,",");
    }
    Delete(qp);
    Delete(tp);
  }
  Append(qprefix,")>");
  Insert(tsuffix, 0, qprefix);
  return tsuffix;
}


SwigType *Swig_symbol_typedef_reduce(SwigType *ty, Symtab *tab) {
  SwigType *prefix, *base;
  Node *n;


  base = SwigType_base(ty);
  prefix = SwigType_prefix(ty);

  n = Swig_symbol_clookup(base,tab);
  if (!n) {
    Delete(base);
    if (SwigType_istemplate(ty)) {
      SwigType *qt = Swig_symbol_template_reduce(ty,tab);
      Append(prefix,qt);
#ifdef SWIG_DEBUG
      Printf(stderr,"symbol_reduce %s %s\n", ty, prefix);
#endif
      return prefix;
    } else {
      Delete(prefix);
#ifdef SWIG_DEBUG
      Printf(stderr,"symbol_reduce %s %s\n", ty, ty);
#endif
      return Copy(ty);
    }
  }
  if (Strcmp(nodeType(n),"using") == 0) {
    String *uname = Getattr(n,"uname");
    if (uname) {
      n = Swig_symbol_clookup(base,Getattr(n,"sym:symtab"));
      if (!n) {
	Delete(base);
	Delete(prefix);
#ifdef SWIG_DEBUG
	Printf(stderr,"symbol_reduce %s %s\n", ty, ty);
#endif
	return Copy(ty);
      }
    } 
  }
  if (Strcmp(nodeType(n),"cdecl") == 0) {
    String *storage = Getattr(n,"storage");
    if (storage && (Strcmp(storage,"typedef") == 0)) {
      SwigType *decl;
      SwigType *rt;
      SwigType *qt;
      Symtab *ntab;
      SwigType *nt = Copy(Getattr(n,"type"));
      
      /* Fix for case 'typedef struct Hello hello;' */
      {	
	const char* dclass[3] = {"struct ", "union ", "class "};
	int i;
	for (i=0; i<3; i++) {
	  char * c = Char(nt);
	  if (strstr(c, dclass[i]) == c) {
	    Replace(nt,dclass[i],"", DOH_REPLACE_FIRST);
	  }
	}
      }
      decl = Getattr(n,"decl");
      if (decl) {
	SwigType_push(nt,decl);
      }
      SwigType_push(nt,prefix);
      Delete(base);
      Delete(prefix);
      ntab = Getattr(n,"sym:symtab");
      rt = Swig_symbol_typedef_reduce(nt, ntab);
      qt = Swig_symbol_type_qualify(rt, ntab);
      if (SwigType_istemplate(qt)) {
	SwigType *qtr = Swig_symbol_template_reduce(qt,ntab);
	Delete(qt);
	qt = qtr;
      }      
      Delete(nt);
      Delete(rt);
#ifdef SWIG_DEBUG
      Printf(stderr,"symbol_reduce %s %s\n", qt, ty);
#endif
      return qt;
    }
  }
  Delete(base);
  Delete(prefix);
#ifdef SWIG_DEBUG
      Printf(stderr,"symbol_reduce %s %s\n", ty, ty);
#endif
  return Copy(ty);
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_string_qualify()
 *
 * This function takes a string and looks for identifiers.  Identifiers are
 * then qualified according to scope rules.  This function is used in a number
 * of settings including expression evaluation, scoping of conversion operators,
 * and so forth.
 * ----------------------------------------------------------------------------- */

String *
Swig_symbol_string_qualify(String *s, Symtab *st) {
  char *c;
  String *id, *r;
  int     have_id = 0;

  id = NewString("");
  r = NewString("");
  c = Char(s);
  while (*c) {
    if (isalpha((int)*c) || (*c == '_') || (*c == ':')) {
      Putc(*c,id);
      have_id = 1;
    } else {
      if (have_id) {
	String *qid = Swig_symbol_type_qualify(id,st);
	Append(r,qid);
	Clear(id);
	Delete(qid);
	have_id = 0;
      }
      Putc(*c,r);
    }
    c++;
  }
  if (have_id) {
    String *qid = Swig_symbol_type_qualify(id,st);
    Append(r,qid);
    Delete(qid);
  }
  Delete(id);
  return r;
}


/* -----------------------------------------------------------------------------
 * Swig_symbol_template_defargs()
 *
 * Apply default arg from generic template default args 
 * ----------------------------------------------------------------------------- */


void
Swig_symbol_template_defargs(Parm *parms, Parm *targs, Symtab *tscope, Symtab *tsdecl) {
  if (Len(parms) < Len(targs)) {
    Parm *lp = parms;
    Parm *p = lp;
    Parm *tp = targs;
    while(p && tp) {
      p = nextSibling(p);
      tp = nextSibling(tp);
      if (p) lp = p;
    }
    while (tp) {
      String *value = Getattr(tp,"value");
      if (value) {
	Parm *cp;
	Parm *ta = targs;
	Parm *p = parms;
	SwigType *nt = Swig_symbol_string_qualify(value,tsdecl);
	SwigType *ntq = 0;
#ifdef SWIG_DEBUG
	Printf(stderr,"value %s %s %s\n",value, nt,tsdecl?Getattr(tsdecl,"name") : tsdecl);
#endif
	while(p && ta) {
	  String *name = Getattr(ta,"name");
	  String *value = Getattr(p,"value") ? Getattr(p,"value") : Getattr(p,"type");
	  String *ttq = Swig_symbol_type_qualify(value,tscope);
	  /* value = SwigType_typedef_resolve_all(value);*/
	  Replaceid(nt, name, ttq);
	  p = nextSibling(p);
	  ta = nextSibling(ta);
	  Delete(ttq);
	}
	ntq = Swig_symbol_type_qualify(nt,tsdecl);
	if (SwigType_istemplate(ntq)) {
	  String *ty = Swig_symbol_template_deftype(ntq, tscope);
	  Delete(ntq);
	  ntq = ty;
	}
	/* Printf(stderr,"value %s %s %s\n",value,ntr,ntq);*/
	cp = NewParm(ntq,0);
	set_nextSibling(lp,cp);
	lp = cp;
	tp = nextSibling(tp);
	Delete(nt);
	Delete(ntq);
      } else {
	tp = 0;
      }
    }
  }
}

/* -----------------------------------------------------------------------------
 * Swig_symbol_template_deftype()
 *
 * Apply default args to generic template type
 * ----------------------------------------------------------------------------- */
SwigType*
Swig_symbol_template_deftype(const SwigType *type, Symtab *tscope) {
  String *result   = NewString("");
  List   *elements = SwigType_split(type);
  int     len = Len(elements);
  int     i;
  for (i = 0; i < len; i++) {
    String *e = Getitem(elements,i);
    if (SwigType_isfunction(e)) {
      Iterator pi;
      List *parms = SwigType_parmlist(e);
      String *s = NewString("f(");
      pi = First(parms);
      while (pi.item) {
	String *pf = SwigType_istemplate(e) ? 
	  Swig_symbol_template_deftype(pi.item,tscope)
	  : Swig_symbol_type_qualify(pi.item,tscope);
	Append(s,pf);
	pi = Next(pi);
	if (pi.item) {
	  Append(s,",");
	}
	Delete(pf);
      }
      Append(s,").");
      Append(result,s);
      Delete(s);
    } else if (SwigType_istemplate(e)) {
      String *prefix   = SwigType_prefix(e);
      String *base     = SwigType_base(e);
      String *tprefix  = SwigType_templateprefix(base);
      String *targs    = SwigType_templateargs(base);
      String *tsuffix  = SwigType_templatesuffix(base);
      ParmList *tparms = SwigType_function_parms(targs);
      Node *tempn = Swig_symbol_clookup_local(tprefix,tscope);
      /* Printf(stderr,"deftype type %s \n", e);*/
      if (tempn) {
	ParmList *tnargs = Getattr(tempn,"templateparms");
	Parm *p;
	Symtab *tsdecl = Getattr(tempn,"sym:symtab");
	
	/* Printf(stderr,"deftype type %s %s %s %s\n", tprefix, targs, tsuffix);*/
	Append(tprefix,"<(");
	Swig_symbol_template_defargs(tparms, tnargs,tscope,tsdecl);
	p = tparms;
	while (p) {
	  SwigType *ptype = Getattr(p,"type");
	  SwigType *ttr = ptype ? ptype : Getattr(p,"value");
	  SwigType *ttf = Swig_symbol_type_qualify(ttr,tscope);
	  SwigType *ttq = Swig_symbol_template_param_eval(ttf,tscope);
	  SwigType *ttd = 0;
	  if (SwigType_istemplate(ttq)) {
	    ttd = Swig_symbol_template_deftype(ttq, tscope);
	    ttq = ttd;
	  }	
	  Append(tprefix,ttq);
	  p = nextSibling(p);
	  if (p) Putc(',',tprefix);
	  Delete(ttr);
	  Delete(ttf);
	  Delete(ttd);
	}
	Append(tprefix,")>");
	Append(tprefix,tsuffix);
	Append(prefix,tprefix);
	/* Printf(stderr,"deftype %s %s \n", type, tprefix); */
	Append(result,prefix);
      } else {
	Append(result,e);
      }
      Delete(prefix);
      Delete(base);
      Delete(tprefix);
      Delete(tsuffix);
      Delete(targs);
      Delete(tparms);
    } else {
      Append(result,e);
    }
  }
  Delete(elements);
  return result;
}

SwigType *Swig_symbol_template_param_eval(const SwigType *p, Symtab *symtab)
{
  String *value = Copy(p);
  Node *lastnode = 0;
  while (1) {
    Node *n = Swig_symbol_clookup(value,symtab);
    if (n == lastnode) break;
    lastnode = n;
    if (n) {
      if (Strcmp(nodeType(n),"enumitem") == 0) {
	/* An enum item.   Generate a fully qualified name */
	String *qn = Swig_symbol_qualified(n);
	if (Len(qn)) {
	  Append(qn,"::");
	  Append(qn,Getattr(n,"name"));
	  Delete(value);
	  value = qn;
	  continue;
	} else {
	  Delete(qn);
	  break;
	}
      } else if ((Strcmp(nodeType(n),"cdecl") == 0) && (Getattr(n,"value"))) {
	Delete(value);
	value = Copy(Getattr(n,"value"));
	continue;
      }
    }
    break;
  }
  return value;
}
