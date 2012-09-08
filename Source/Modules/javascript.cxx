#include "swigmod.h"

/**
 * Enables extra debugging information in typemaps.
 */
bool js_template_enable_debug = false;

// keywords used for state variables
#define NAME "name"
#define NAME_MANGLED "name_mangled"
#define TYPE "type"
#define TYPE_MANGLED "type_mangled"
#define WRAPPER_NAME "wrapper"
#define IS_IMMUTABLE "is_immutable"
#define IS_STATIC "is_static"
#define IS_ABSTRACT "is_abstract"
#define GETTER "getter"
#define SETTER "setter"
#define PARENT "parent"
#define CTOR "ctor"
#define CTOR_WRAPPERS "ctor_wrappers"
#define CTOR_DISPATCHERS "ctor_dispatchers"
#define ARGCOUNT "wrap:argc"

// variables used in code templates
// ATTENTION: be aware of prefix collisions when defining those variables
#define T_NAME            "$jsname"
#define T_NAME_MANGLED    "$jsmangledname"
#define T_TYPE            "$jstype"
#define T_TYPE_MANGLED    "$jsmangledtype"
#define T_WRAPPER         "$jswrapper"
#define T_CTOR            "$jsctor"
#define T_GETTER          "$jsgetter"
#define T_SETTER          "$jssetter"
#define T_DISPATCH_CASES  "$jsdispatchcases"
#define T_BASECLASS       "$jsbaseclass"
#define T_NAMESPACE       "$jsnspace"
#define T_PARENT          "$jsparent"
#define T_OVERLOAD        "$jsoverloadext"
#define T_ARGCOUNT        "$jsargcount"
#define T_LOCALS          "$jslocals"
#define T_CODE            "$jscode"

// v8 specific variables used in templates
#define V8_NAME_SPACES                              "$jsv8nspaces"
#define V8_CLASS_TEMPLATES                          "$jsv8classtemplates"
#define V8_WRAPPERS                                 "$jsv8wrappers"
#define V8_INHERITANCE                              "$jsv8inheritance"
#define V8_CLASS_INSTANCES                          "$jsv8classinstances"
#define V8_STATIC_WRAPPERS                          "$jsv8staticwrappers"
#define V8_REGISTER_CLASSES                         "$jsv8registerclasses"
#define V8_REGISTER_NS                              "$jsv8registernspaces"

/**
 * A convenience class to manage state variables for emitters.
 * The implementation delegates to swig Hash DOHs and provides
 * named sub-hashes for class, variable, and function states.
 */
class JSEmitterState {
  
public:

  JSEmitterState();

  ~JSEmitterState();
    
  DOH *global();
  
  DOH *global(const char* key, DOH *initial = 0);
  
  DOH *clazz(bool reset = false);
  
  DOH *clazz(const char* key, DOH *initial = 0);

  DOH *function(bool reset = false);

  DOH *function(const char* key, DOH *initial = 0);

  DOH *variable(bool reset = false);

  DOH *variable(const char* key, DOH *initial = 0);
  
  static int IsSet(DOH *val);
  
private:

  DOH *getState(const char* key, bool reset = false);

  Hash *_global;
};

/**
 * A convenience class that wraps a code snippet used as template 
 * for code generation.
 */
class Template {

public:
  Template(const String *code);

  Template(const String *code, const String *templateName);

  ~Template();

  String *str();

  Template& replace(const String *pattern, const String *repl);

  Template& pretty_print(DOH *doh);
  
  void operator=(const Template& t);

private:

  String *code;
  String *templateName;

};

/**
 * JSEmitter represents an abstraction of javascript code generators
 * for different javascript engines.
 **/
class JSEmitter {

protected:

  typedef JSEmitterState State;

  enum MarshallingMode {
    Setter,
    Getter,
    Ctor,
    Function
  };

public:

  enum JSEmitterType {
    JavascriptCore,
    V8,
    QtScript
  };

  JSEmitter();

  virtual ~JSEmitter();

  /**
   * Opens output files and temporary output DOHs.
   */
  virtual int initialize(Node *n);

  /**
   * Writes all collected code into the output file(s).
   */
  virtual int dump(Node *n) = 0;

  /**
   * Cleans up all open output DOHs.
   */
  virtual int close() = 0;

  /**
   * Switches the context for code generation.
   * 
   * Classes, global variables and global functions may need to
   * be registered in certain static tables.
   * This method should be used to switch output DOHs correspondingly.
   */
  virtual int switchNamespace(Node *);

  /**
   * Invoked at the beginning of the classHandler.
   */
  virtual int enterClass(Node *);

  /**
   * Invoked at the end of the classHandler.
   */
  virtual int exitClass(Node *) {
    return SWIG_OK;
  };

  /**
   * Invoked at the beginning of the variableHandler.
   */
  virtual int enterVariable(Node *);

  /**
   * Invoked at the end of the variableHandler.
   */
  virtual int exitVariable(Node *) {
    return SWIG_OK;
  };

  /**
   * Invoked at the beginning of the functionHandler.
   */
  virtual int enterFunction(Node *);

  /**
   * Invoked at the end of the functionHandler.
   */
  virtual int exitFunction(Node *) {
    return SWIG_OK;
  };

  /**
   * Invoked by functionWrapper callback after call to Language::functionWrapper.
   */
  virtual int emitWrapperFunction(Node *n);

  /**
   * Invoked from constantWrapper after call to Language::constantWrapper.
   **/
  virtual int emitConstant(Node *n);

  /**
   * Registers a given code snippet for a given key name.
   * 
   * This method is called by the fragmentDirective handler
   * of the JAVASCRIPT language module.
   **/
  int registerTemplate(const String *name, const String *code);

  /**
   * Retrieve the code template registered for a given name.
   */
  Template getTemplate(const String *name);

  State &getState();

protected:

  /**
   * Generates code for a constructor function.
   */
  virtual int emitCtor(Node *n);

  /**
   * Generates code for a destructor function.
   */
  virtual int emitDtor(Node *n);

  /**
   * Generates code for a function.
   */
  virtual int emitFunction(Node *n, bool is_member, bool is_static);

  virtual int emitFunctionDispatcher(Node *n, bool /*is_member */ );

  /**
   * Generates code for a getter function.
   */
  virtual int emitGetter(Node *n, bool is_member, bool is_static);

  /**
   * Generates code for a setter function.
   */
  virtual int emitSetter(Node *n, bool is_member, bool is_static);

  virtual void marshalInputArgs(Node *n, ParmList *parms, Wrapper *wrapper, MarshallingMode mode, bool is_member, bool is_static) = 0;
  
  virtual void emitInputTypemap(Node *n, Parm *p, Wrapper *wrapper, String *arg);

  virtual void marshalOutput(Node *n, String *actioncode, Wrapper *wrapper);

  /**
   * Helper function to retrieve the first parent class node.
   */
  Node *getBaseClass(Node *n);
  
  Parm *skipIgnoredArgs(Parm *p);

  virtual int createNamespace(String *scope);

  virtual Hash *createNamespaceEntry(const char *name, const char *parent);

  virtual int emitNamespaces() = 0;

protected:

  Hash *templates;
  
  State state;
  
  // contains context specific data (DOHs)
  // to allow generation of namespace related code
  // which are switched on namespace change
  Hash *namespaces;
  Hash *current_namespace;
  
  File *f_wrappers;

};

/**********************************************************************
 * JAVASCRIPT: swig module implementation
 **********************************************************************/

/* factory methods for concrete JSEmitters: */

JSEmitter *swig_javascript_create_JSCEmitter();
JSEmitter *swig_javascript_create_V8Emitter();

class JAVASCRIPT:public Language {

public:

  JAVASCRIPT(): emitter(NULL) {}
  
  ~JAVASCRIPT() {
    delete emitter;
  }

  virtual int functionHandler(Node *n);
  virtual int globalfunctionHandler(Node *n);
  virtual int variableHandler(Node *n);
  virtual int globalvariableHandler(Node *n);
  virtual int staticmemberfunctionHandler(Node *n);
  virtual int classHandler(Node *n);
  virtual int functionWrapper(Node *n);
  virtual int constantWrapper(Node *n);
  virtual void main(int argc, char *argv[]);
  virtual int top(Node *n);
  
  /**
   *  Registers all %fragments assigned to section "templates".
   **/
  virtual int fragmentDirective(Node *n);

private:

  JSEmitter *emitter;
};

/* ---------------------------------------------------------------------
 * functionWrapper()
 *
 * Low level code generator for functions 
 * --------------------------------------------------------------------- */

int JAVASCRIPT::functionWrapper(Node *n) {

  // note: the default implementation only prints a message
  // Language::functionWrapper(n);
  emitter->emitWrapperFunction(n);
  
  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * functionHandler()
 *
 * Function handler for generating wrappers for functions 
 * --------------------------------------------------------------------- */
int JAVASCRIPT::functionHandler(Node *n) {

  if (GetFlag(n, "isextension") == 1) {
    SetFlag(n, "ismember");
  }

  emitter->enterFunction(n);
  Language::functionHandler(n);
  emitter->exitFunction(n);

  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * globalfunctionHandler()
 *
 * Function handler for generating wrappers for functions 
 * --------------------------------------------------------------------- */

int JAVASCRIPT::globalfunctionHandler(Node *n) {
  emitter->switchNamespace(n);
  Language::globalfunctionHandler(n);
  
  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * staticmemberfunctionHandler()
 *
 * Function handler for generating wrappers for static member functions 
 * --------------------------------------------------------------------- */

int JAVASCRIPT::staticmemberfunctionHandler(Node *n) {
  /*
   *  Note: storage=static is removed by Language::staticmemberfunctionHandler.
   *    So, don't rely on that after here. Instead use the state variable which is
   *    set by JSEmitter::enterFunction().
   */
  Language::staticmemberfunctionHandler(n);  
  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * variableHandler()
 *
 * Function handler for generating wrappers for variables 
 * --------------------------------------------------------------------- */

int JAVASCRIPT::variableHandler(Node *n) {

  emitter->enterVariable(n);  
  Language::variableHandler(n);
  emitter->exitVariable(n);

  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * globalvariableHandler()
 *
 * Function handler for generating wrappers for global variables 
 * --------------------------------------------------------------------- */

int JAVASCRIPT::globalvariableHandler(Node *n) {

  emitter->switchNamespace(n);
  Language::globalvariableHandler(n);

  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * constantHandler()
 *
 * Function handler for generating wrappers for constants 
 * --------------------------------------------------------------------- */

int JAVASCRIPT::constantWrapper(Node *n) {

  // Note: callbacks trigger this wrapper handler
  // TODO: handle callback declarations
  if (Equal(Getattr(n, "kind"), "function")) {
    return SWIG_OK;
  }
  emitter->emitConstant(n);

  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * classHandler()
 *
 * Function handler for generating wrappers for class 
 * --------------------------------------------------------------------- */

int JAVASCRIPT::classHandler(Node *n) {
  emitter->switchNamespace(n);

  emitter->enterClass(n);
  Language::classHandler(n);
  emitter->exitClass(n);

  return SWIG_OK;
}

int JAVASCRIPT::fragmentDirective(Node *n) {

  // catch all fragment directives that have "templates" as location
  // and register them at the emitter.
  String *section = Getattr(n, "section");

  if (Equal(section, "templates")) {
    emitter->registerTemplate(Getattr(n, "value"), Getattr(n, "code"));
  } else {
    Swig_fragment_register(n);
  }

  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * top()
 *
 * Function handler for processing top node of the parse tree 
 * Wrapper code generation essentially starts from here 
 * --------------------------------------------------------------------- */

int JAVASCRIPT::top(Node *n) {

  emitter->initialize(n);

  Language::top(n);

  emitter->dump(n);
  emitter->close();

  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * main()
 *
 * Entry point for the JAVASCRIPT module
 * --------------------------------------------------------------------- */

void JAVASCRIPT::main(int argc, char *argv[]) {

  // Set javascript subdirectory in SWIG library
  SWIG_library_directory("javascript");

  int mode = -1;

  bool debug_templates = false;
  for (int i = 1; i < argc; i++) {
    if (argv[i]) {
      if (strcmp(argv[i], "-v8") == 0) {
        Swig_mark_arg(i);
        mode = JSEmitter::V8;
        SWIG_library_directory("javascript/v8");
      } else if (strcmp(argv[i], "-jsc") == 0) {
        Swig_mark_arg(i);
        mode = JSEmitter::JavascriptCore;
        SWIG_library_directory("javascript/jsc");
      } else if (strcmp(argv[i], "-qt") == 0) {
        Swig_mark_arg(i);
        mode = JSEmitter::QtScript;
        SWIG_library_directory("javascript/qt");
      } else if (strcmp(argv[i], "-debug-codetemplates") == 0) {
        Swig_mark_arg(i);
        debug_templates = true;
      }
    }
  }

  switch (mode) {
  case JSEmitter::V8:
    {
      emitter = swig_javascript_create_V8Emitter();
      break;
    }
  case JSEmitter::JavascriptCore:
    {
      emitter = swig_javascript_create_JSCEmitter();
      break;
    }
  case JSEmitter::QtScript:
    {
      Printf(stderr, "QtScript support is not yet implemented.");
      SWIG_exit(-1);
      break;
    }
  default:
    {
      Printf(stderr, "Unknown emitter type.");
      SWIG_exit(-1);
      break;
    }
  }

  if (debug_templates) {
    js_template_enable_debug = true;
  }

  // Add a symbol to the parser for conditional compilation
  Preprocessor_define("SWIGJAVASCRIPT 1", 0);

  // Add typemap definitions    
  SWIG_typemap_lang("javascript");

  // Set configuration file 
  SWIG_config_file("javascript.swg");

  allow_overloading();
}

/* -----------------------------------------------------------------------------
 * swig_javascript()    - Instantiate module
 * ----------------------------------------------------------------------------- */

static Language *new_swig_javascript() {
  return new JAVASCRIPT();
}

extern "C" Language *swig_javascript(void) {
  return new_swig_javascript();
}

/**********************************************************************
 * Emitter implementations
 **********************************************************************/

/* -----------------------------------------------------------------------------
 * JSEmitter()
 * ----------------------------------------------------------------------------- */

JSEmitter::JSEmitter()
: templates(NewHash()),
  namespaces(NULL), 
  current_namespace(NULL),
  f_wrappers(NULL)
{
}

/* -----------------------------------------------------------------------------
 * ~JSEmitter()
 * ----------------------------------------------------------------------------- */

JSEmitter::~JSEmitter() {
  Delete(templates);
}

/* -----------------------------------------------------------------------------
 * JSEmitter::RegisterTemplate() :  Registers a code template
 * 
 *  Note: this is used only by JAVASCRIPT::fragmentDirective().
 * ----------------------------------------------------------------------------- */

int JSEmitter::registerTemplate(const String *name, const String *code) {
  return Setattr(templates, name, code);
}

/* -----------------------------------------------------------------------------
 * JSEmitter::getTemplate() :  Provides a registered code template
 * ----------------------------------------------------------------------------- */

Template JSEmitter::getTemplate(const String *name) {
  String *templ = Getattr(templates, name);

  if (!templ) {
    Printf(stderr, "Could not find template %s\n.", name);
    SWIG_exit(EXIT_FAILURE);
  }

  Template t(templ, name);

  return t;
}

JSEmitterState &JSEmitter::getState() {
  return state;
}

int JSEmitter::initialize(Node * n) {

  if(namespaces != NULL) {
    Delete(namespaces);
  }
  namespaces = NewHash();
  Hash *global_namespace = createNamespaceEntry(Char(Getattr(n, "name")), "global");
  Setattr(namespaces, "::", global_namespace);
  current_namespace = global_namespace;
  
  f_wrappers = NewString("");
  
  return SWIG_OK;
}

/* ---------------------------------------------------------------------
 * skipIgnoredArgs()
 * --------------------------------------------------------------------- */

Parm *JSEmitter::skipIgnoredArgs(Parm *p) {
  while (checkAttribute(p, "tmap:in:numinputs", "0")) {
    p = Getattr(p, "tmap:in:next");
  }
  return p;
}

/* -----------------------------------------------------------------------------
 * JSEmitter::getBaseClass() :  the node of the base class or NULL
 * 
 * Note: the first base class is provided. Multiple inheritance is not
 *       supported.
 * ----------------------------------------------------------------------------- */

Node *JSEmitter::getBaseClass(Node *n) {
  // retrieve the first base class that is not %ignored
  List *baselist = Getattr(n, "bases");
  if (baselist) {
    Iterator base = First(baselist);
    while (base.item && GetFlag(base.item, "feature:ignore")) {
      base = Next(base);
    }
    return base.item;
  }
  return NULL;
}

 /* -----------------------------------------------------------------------------
  * JSEmitter::emitWrapperFunction() :  dispatches emitter functions.
  * 
  * This allows to have small sized, dedicated emitting functions.
  * All state dependent branching is done here.
  * ----------------------------------------------------------------------------- */

int JSEmitter::emitWrapperFunction(Node *n) {
  int ret = SWIG_OK;

  String *kind = Getattr(n, "kind");

  if (kind) {
    bool is_member = GetFlag(n, "ismember");
    if (Cmp(kind, "function") == 0) {
      bool is_static = GetFlag(state.function(), IS_STATIC);
      ret = emitFunction(n, is_member, is_static);
    } else if (Cmp(kind, "variable") == 0) {
      bool is_static = GetFlag(state.variable(), IS_STATIC);
      bool is_setter = GetFlag(n, "wrap:issetter");
      if (is_setter) {
        ret = emitSetter(n, is_member, is_static);
      } else {
        ret = emitGetter(n, is_member, is_static);
      }
    } else {
      Printf(stderr, "Warning: unsupported wrapper function type\n");
      Swig_print_node(n);
      ret = SWIG_ERROR;
    }
  } else {
    String *view = Getattr(n, "view");

    if (Cmp(view, "constructorHandler") == 0) {
      ret = emitCtor(n);
    } else if (Cmp(view, "destructorHandler") == 0) {
      ret = emitDtor(n);
    } else {
      Printf(stderr, "Warning: unsupported wrapper function type");
      Swig_print_node(n);
      ret = SWIG_ERROR;
    }
  }

  return ret;
}

int JSEmitter::enterClass(Node *n) {

  state.clazz(true);
  state.clazz(NAME, Getattr(n, "sym:name"));
  state.clazz(NAME_MANGLED, SwigType_manglestr(Getattr(n, "name")));
  state.clazz(TYPE, NewString(Getattr(n, "classtype")));

  String *type = SwigType_manglestr(Getattr(n, "classtypeobj"));
  String *classtype_mangled = NewString("");
  Printf(classtype_mangled, "p%s", type);
  Delete(type);
  state.clazz(TYPE_MANGLED, classtype_mangled);

  String *ctor_wrapper = NewString("_wrap_new_veto_");
  Append(ctor_wrapper, state.clazz(NAME));
  state.clazz(CTOR, ctor_wrapper);
  state.clazz(CTOR_DISPATCHERS, NewString(""));

  // HACK: assume that a class is abstract
  // this is resolved by emitCtor (which is only called for non abstract classes)
  SetFlag(state.clazz(), IS_ABSTRACT);
  
  return SWIG_OK;
}

int JSEmitter::enterFunction(Node *n) {

  state.function(true);
  state.function(NAME, Getattr(n, "sym:name"));
  if(Equal(Getattr(n, "storage"), "static")) {
    SetFlag(state.function(), IS_STATIC);
  }

  return SWIG_OK;
}

int JSEmitter::enterVariable(Node *n) {

  state.variable(true);
  state.variable(NAME, Swig_scopename_last(Getattr(n, "name")));
  
  if(Equal(Getattr(n, "storage"), "static")) {
    SetFlag(state.variable(), IS_STATIC);
  }

  if (!Language::instance()->is_assignable(n)
      // FIXME: test "arrays_global" does not compile with that as it is not allowed to assign to char[]
      // probably some error in char[] typemap 
      || Equal(Getattr(n, "type"), "a().char")) {
    SetFlag(state.variable(), IS_IMMUTABLE);
  }

  return SWIG_OK;
}

int JSEmitter::switchNamespace(Node *n) {
  
  if (!GetFlag(n, "feature:nspace")) {
    current_namespace = Getattr(namespaces, "::");
  } else {
    String *scope = Swig_scopename_prefix(Getattr(n, "name"));
    if (scope) {
      // if the scope is not yet registered
      // create (parent) namespaces recursively
      if (!Getattr(namespaces, scope)) {
        createNamespace(scope);
      }
      current_namespace = Getattr(namespaces, scope);
    } else {
      current_namespace = Getattr(namespaces, "::");
    }
  }

  return SWIG_OK;
}

int JSEmitter::createNamespace(String *scope) {

  String *parent_scope = Swig_scopename_prefix(scope);
  Hash *parent_namespace;
  if (parent_scope == 0) {
    parent_namespace = Getattr(namespaces, "::");
  } else if (!Getattr(namespaces, parent_scope)) {
    createNamespace(parent_scope);
    parent_namespace = Getattr(namespaces, parent_scope);
  } else {
    parent_namespace = Getattr(namespaces, parent_scope);
  }
  assert(parent_namespace != 0);

  Hash *new_namespace = createNamespaceEntry(Char(scope), Char(Getattr(parent_namespace, "name")));
  Setattr(namespaces, scope, new_namespace);

  Delete(parent_scope);
  return SWIG_OK;
}

Hash *JSEmitter::createNamespaceEntry(const char *_name, const char *parent) {
  Hash *entry = NewHash();
  String *name = NewString(_name);
  Setattr(entry, NAME, Swig_scopename_last(name));
  Setattr(entry, NAME_MANGLED, Swig_name_mangle(name));
  Setattr(entry, PARENT, NewString(parent));
  
  Delete(name);
  return entry;
}

/**********************************************************************
 * JavascriptCore: JSEmitter implementation for JavascriptCore engine
 **********************************************************************/
 
class JSCEmitter:public JSEmitter {

public:

  JSCEmitter();

  virtual ~ JSCEmitter();

  virtual int initialize(Node *n);

  virtual int dump(Node *n);

  virtual int close();


protected:

  virtual int enterVariable(Node *n);

  virtual int exitVariable(Node *n);

  virtual int enterFunction(Node *n);

  virtual int exitFunction(Node *n);

  virtual int enterClass(Node *n);

  virtual int exitClass(Node *n);

  virtual void marshalInputArgs(Node *n, ParmList *parms, Wrapper *wrapper, MarshallingMode mode, bool is_member, bool is_static);

  virtual Hash *createNamespaceEntry(const char *name, const char *parent);

  virtual int emitNamespaces();

private:

  String *NULL_STR;
  String *VETO_SET;
  const char *GLOBAL_STR;


  // output file and major code parts
  File *f_wrap_cpp;
  File *f_runtime;
  File *f_header;
  File *f_init;

};

// keys for global state variables
#define CREATE_NAMESPACES "create_namespaces"
#define REGISTER_NAMESPACES "register_namespaces"
#define INITIALIZER "initializer"

// keys for class scoped state variables
#define MEMBER_VARIABLES "member_variables"
#define MEMBER_FUNCTIONS "member_functions"
#define STATIC_FUNCTIONS "static_functions"
#define STATIC_VARIABLES "static_variables"

// keys for function scoped state variables
#define FUNCTION_DISPATCHERS "function_dispatchers"

JSCEmitter::JSCEmitter()
: JSEmitter(), 
  NULL_STR(NewString("NULL")),
  VETO_SET(NewString("JS_veto_set_variable")),
  GLOBAL_STR(NULL),
  f_wrap_cpp(NULL),
  f_runtime(NULL), 
  f_header(NULL), 
  f_init(NULL)
{
}

JSCEmitter::~JSCEmitter() {
  Delete(NULL_STR);
  Delete(VETO_SET);
}


/* ---------------------------------------------------------------------
 * marshalInputArgs()
 *
 * Process all of the arguments passed into the argv array
 * and convert them into C/C++ function arguments using the
 * supplied typemaps.
 * --------------------------------------------------------------------- */

void JSCEmitter::marshalInputArgs(Node *n, ParmList *parms, Wrapper *wrapper, MarshallingMode mode, bool is_member, bool is_static) {
  Parm *p;

  // determine an offset index, as members have an extra 'this' argument
  // except: static members and ctors.
  int startIdx = 0;
  if (is_member && !is_static && mode!=Ctor) {
    startIdx = 1;
  }

  // store number of arguments for argument checks
  int num_args = emit_num_arguments(parms) - startIdx;
  String *argcount = NewString("");
  Printf(argcount, "%d", num_args);
  Setattr(n, ARGCOUNT, argcount);

  // process arguments
  int i = 0;
  for (p = parms; p; p = nextSibling(p), i++) {
    String *arg = NewString("");
    switch (mode) {
    case Getter:
    case Function:
      if (is_member && !is_static && i == 0) {
        Printv(arg, "thisObject", 0);
      } else {
        Printf(arg, "argv[%d]", i - startIdx);
      }
      break;
    case Setter:
      if (is_member && !is_static && i == 0) {
        Printv(arg, "thisObject", 0);
      } else {
        Printv(arg, "value", 0);
      }
      break;
    case Ctor:
      Printf(arg, "argv[%d]", i);
      break;
    default:
      throw "Illegal state.";
    }
    emitInputTypemap(n, p, wrapper, arg);
    Delete(arg);
  }
}

int JSCEmitter::initialize(Node *n) {
  
  JSEmitter::initialize(n);

  /* Get the output file name */
  String *outfile = Getattr(n, "outfile");

  /* Initialize I/O */
  f_wrap_cpp = NewFile(outfile, "w", SWIG_output_files());
  if (!f_wrap_cpp) {
    FileErrorDisplay(outfile);
    SWIG_exit(EXIT_FAILURE);
  }

  /* Initialization of members */
  f_runtime = NewString("");
  f_init = NewString("");
  f_header = NewString("");

  state.global(CREATE_NAMESPACES, NewString(""));
  state.global(REGISTER_NAMESPACES, NewString(""));
  state.global(INITIALIZER, NewString(""));

  /* Register file targets with the SWIG file handler */
  Swig_register_filebyname("begin", f_wrap_cpp);
  Swig_register_filebyname("header", f_header);
  Swig_register_filebyname("wrapper", f_wrappers);
  Swig_register_filebyname("runtime", f_runtime);
  Swig_register_filebyname("init", f_init);

  return SWIG_OK;
}

int JSCEmitter::dump(Node *n) {
  /* Get the module name */
  String *module = Getattr(n, "name");

  // write the swig banner
  Swig_banner(f_wrap_cpp);

  SwigType_emit_type_table(f_runtime, f_wrappers);

  Printv(f_wrap_cpp, f_runtime, "\n", 0);
  Printv(f_wrap_cpp, f_header, "\n", 0);
  Printv(f_wrap_cpp, f_wrappers, "\n", 0);

  emitNamespaces();

  // compose the initializer function using a template
  Template initializer(getTemplate("JS_initializer"));
  initializer.replace(T_NAME, module)
      .replace("$jsinitializercode", state.global(INITIALIZER))
      .replace("$jscreatenamespaces", state.global(CREATE_NAMESPACES))
      .replace("$jsregisternamespaces", state.global(REGISTER_NAMESPACES))
      .pretty_print(f_init);

  Printv(f_wrap_cpp, f_init, 0);

  return SWIG_OK;
}

int JSCEmitter::close() {
  /* strings */
  Delete(f_runtime);
  Delete(f_header);
  Delete(f_wrappers);
  Delete(f_init);

  Delete(namespaces);

  /* files */
  Close(f_wrap_cpp);
  Delete(f_wrap_cpp);

  return SWIG_OK;
}

int JSCEmitter::enterFunction(Node *n) {

  JSEmitter::enterFunction(n);

  /* Initialize DOH for collecting function dispatchers */
  bool is_overloaded = GetFlag(n, "sym:overloaded");
  if (is_overloaded && state.global(FUNCTION_DISPATCHERS) == 0) {
    state.global(FUNCTION_DISPATCHERS, NewString(""));
  }

  return SWIG_OK;
}

int JSCEmitter::exitFunction(Node *n) {
  Template t_function = getTemplate("JS_functiondecl");

  bool is_member = GetFlag(n, "ismember");
  bool is_overloaded = GetFlag(n, "sym:overloaded");

  // handle overloaded functions
  if (is_overloaded) {
    if (!Getattr(n, "sym:nextSibling")) {
      state.function(WRAPPER_NAME, Swig_name_wrapper(Getattr(n, "name")));
      // create dispatcher
      emitFunctionDispatcher(n, is_member);
    } else {
      //don't register wrappers of overloaded functions in function tables
      return SWIG_OK;
    }
  }

  t_function.replace(T_NAME, state.function(NAME))
      .replace(T_WRAPPER, state.function(WRAPPER_NAME));

  if (is_member) {
    if (GetFlag(state.function(), IS_STATIC)) {
      Append(state.clazz(STATIC_FUNCTIONS), t_function.str());
    } else {
      Append(state.clazz(MEMBER_FUNCTIONS), t_function.str());
    }
  } else {
    Append(Getattr(current_namespace, "functions"), t_function.str());
  }

  return SWIG_OK;
}

int JSCEmitter::enterVariable(Node *n) {
  JSEmitter::enterVariable(n);
  state.variable(GETTER, NULL_STR);
  state.variable(SETTER, VETO_SET);
  return SWIG_OK;
}


int JSCEmitter::exitVariable(Node *n) {
  Template t_variable(getTemplate("JS_variabledecl"));
  t_variable.replace(T_NAME, state.variable(NAME))
      .replace(T_GETTER, state.variable(GETTER))
      .replace(T_SETTER, state.variable(SETTER));

  if (GetFlag(n, "ismember")) {
    if (GetFlag(state.function(), IS_STATIC) 
        || Equal(Getattr(n, "nodeType"), "enumitem")) {
      Append(state.clazz(STATIC_VARIABLES), t_variable.str());
    } else {
      Append(state.clazz(MEMBER_VARIABLES), t_variable.str());
    }
  } else {
    Append(Getattr(current_namespace, "values"), t_variable.str());
  }

  return SWIG_OK;
}

int JSCEmitter::enterClass(Node *n) {
  JSEmitter::enterClass(n);
  state.clazz(MEMBER_VARIABLES, NewString(""));
  state.clazz(MEMBER_FUNCTIONS, NewString(""));
  state.clazz(STATIC_VARIABLES, NewString(""));
  state.clazz(STATIC_FUNCTIONS, NewString(""));

  Template t_class_defn = getTemplate("JS_class_definition");
  t_class_defn.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .pretty_print(f_wrappers);

  return SWIG_OK;
}

int JSCEmitter::exitClass(Node *n) {
  Template t_class_tables(getTemplate("JS_class_tables"));
  t_class_tables.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .replace("$jsclassvariables", state.clazz(MEMBER_VARIABLES))
      .replace("$jsclassfunctions", state.clazz(MEMBER_FUNCTIONS))
      .replace("$jsstaticclassfunctions", state.clazz(STATIC_FUNCTIONS))
      .replace("$jsstaticclassvariables", state.clazz(STATIC_VARIABLES))
      .pretty_print(f_wrappers);

  /* adds the ctor wrappers at this position */
  // Note: this is necessary to avoid extra forward declarations.
  //Append(f_wrappers, state.clazz(CTOR_WRAPPERS));

  // for abstract classes add a vetoing ctor
  if(GetFlag(state.clazz(), IS_ABSTRACT)) {
    Template t_veto_ctor(getTemplate("JS_veto_ctor"));
    t_veto_ctor.replace(T_CTOR, state.clazz(CTOR))
      .replace(T_NAME, state.clazz(NAME))
      .pretty_print(f_wrappers);
  }

  /* adds a class template statement to initializer function */
  Template t_classtemplate(getTemplate("JS_create_class_template"));

  /* prepare registration of base class */
  String *base_name_mangled = NewString("_SwigObject");
  Node *base_class = getBaseClass(n);
  if (base_class != NULL) {
    Delete(base_name_mangled);
    base_name_mangled = SwigType_manglestr(Getattr(base_class, "name"));
  }
  t_classtemplate.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .replace(T_TYPE_MANGLED, state.clazz(TYPE_MANGLED))
      .replace(T_BASECLASS, base_name_mangled)
      .replace(T_CTOR, state.clazz(CTOR))
      .pretty_print(state.global(INITIALIZER));
  Delete(base_name_mangled);

  /* Note: this makes sure that there is a swig_type added for this class */
  SwigType_remember_clientdata(state.clazz(TYPE_MANGLED), NewString("0"));

  /* adds a class registration statement to initializer function */
  Template t_registerclass(getTemplate("JS_register_class"));
  t_registerclass.replace(T_NAME, state.clazz(NAME))
      .replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .replace(T_NAMESPACE, Getattr(current_namespace, NAME_MANGLED))
      .pretty_print(state.global(INITIALIZER));

  return SWIG_OK;
}

int JSEmitter::emitCtor(Node *n) {
    
  Wrapper *wrapper = NewWrapper();

  bool is_overloaded = GetFlag(n, "sym:overloaded");
  
  Template t_ctor(getTemplate("JS_ctordefn"));

  //String *mangled_name = SwigType_manglestr(Getattr(n, "name"));
  String *wrap_name = Swig_name_wrapper(Getattr(n, "sym:name"));
  if(is_overloaded) {
    Append(wrap_name, Getattr(n, "sym:overname"));
  }
  Setattr(n, "wrap:name", wrap_name);
  // note: removing the is_abstract flag, as this emitter
  //       is supposed to be called for non-abstract classes only.
  Setattr(state.clazz(), IS_ABSTRACT, 0);

  ParmList *params = Getattr(n, "parms");
  emit_parameter_variables(params, wrapper);
  emit_attach_parmmaps(params, wrapper);

  Printf(wrapper->locals, "%sresult;", SwigType_str(Getattr(n, "type"), 0));

  String *action = emit_action(n);
  marshalInputArgs(n, params, wrapper, Ctor, true, false);

  Printv(wrapper->code, action, "\n", 0);
  t_ctor.replace(T_WRAPPER, wrap_name)
      .replace(T_TYPE_MANGLED, state.clazz(TYPE_MANGLED))
      .replace(T_LOCALS, wrapper->locals)
      .replace(T_CODE, wrapper->code)
      .replace(T_ARGCOUNT, Getattr(n, ARGCOUNT))
      .pretty_print(f_wrappers);

  Template t_ctor_case(getTemplate("JS_ctor_dispatch_case"));
  t_ctor_case.replace(T_WRAPPER, wrap_name)
      .replace(T_ARGCOUNT, Getattr(n, ARGCOUNT));
  Append(state.clazz(CTOR_DISPATCHERS), t_ctor_case.str());
  
  DelWrapper(wrapper);

  // create a dispatching ctor
  if(is_overloaded) {
    if (!Getattr(n, "sym:nextSibling")) {
      String *wrap_name = Swig_name_wrapper(Getattr(n, "sym:name"));
      Template t_mainctor(getTemplate("JS_mainctordefn"));
      t_mainctor.replace(T_WRAPPER, wrap_name)
          .replace(T_DISPATCH_CASES, state.clazz(CTOR_DISPATCHERS))
          .pretty_print(f_wrappers);
      state.clazz(CTOR, wrap_name);
    }
  } else {
    state.clazz(CTOR, wrap_name);
  }

  return SWIG_OK;
}

int JSEmitter::emitDtor(Node *) {

  Template t_dtor = getTemplate("JS_destructordefn");
  t_dtor.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .replace(T_TYPE, state.clazz(TYPE))
      .pretty_print(f_wrappers);

  return SWIG_OK;
}

int JSEmitter::emitGetter(Node *n, bool is_member, bool is_static) {
  Wrapper *wrapper = NewWrapper();
  Template t_getter(getTemplate("JS_getproperty"));
  
  // prepare wrapper name
  String *wrap_name = Swig_name_wrapper(Getattr(n, "sym:name"));
  Setattr(n, "wrap:name", wrap_name);
  state.variable(GETTER, wrap_name);

  // prepare local variables
  ParmList *params = Getattr(n, "parms");
  emit_parameter_variables(params, wrapper);
  emit_attach_parmmaps(params, wrapper);

  // prepare code part
  String *action = emit_action(n);
  marshalInputArgs(n, params, wrapper, Getter, is_member, is_static);
  marshalOutput(n, action, wrapper);

  t_getter.replace(T_GETTER, wrap_name)
      .replace(T_LOCALS, wrapper->locals)
      .replace(T_CODE, wrapper->code)
      .pretty_print(f_wrappers);

  DelWrapper(wrapper);

  return SWIG_OK;
}

int JSEmitter::emitSetter(Node *n, bool is_member, bool is_static) {

  // skip variables that are immutable
  if (State::IsSet(state.variable(IS_IMMUTABLE))) {
    return SWIG_OK;
  }
  
  Wrapper *wrapper = NewWrapper();

  Template t_setter(getTemplate("JS_setproperty"));

  // prepare wrapper name
  String *wrap_name = Swig_name_wrapper(Getattr(n, "sym:name"));
  Setattr(n, "wrap:name", wrap_name);
  state.variable(SETTER, wrap_name);

  // prepare local variables
  ParmList *params = Getattr(n, "parms");
  emit_parameter_variables(params, wrapper);
  emit_attach_parmmaps(params, wrapper);

  // prepare code part
  String *action = emit_action(n);
  marshalInputArgs(n, params, wrapper, Setter, is_member, is_static);
  Append(wrapper->code, action);

  t_setter.replace(T_SETTER, wrap_name)
      .replace(T_LOCALS, wrapper->locals)
      .replace(T_CODE, wrapper->code)
      .pretty_print(f_wrappers);

  DelWrapper(wrapper);

  return SWIG_OK;
}

/* -----------------------------------------------------------------------------
 * JSCEmitter::emitConstant() :  triggers code generation for constants
 * ----------------------------------------------------------------------------- */

int JSEmitter::emitConstant(Node *n) {

  Wrapper *wrapper = NewWrapper();

  Template t_getter(getTemplate("JS_getproperty"));

  // call the variable methods as a constants are
  // registred in same way
  enterVariable(n);

  // prepare function wrapper name
  String *wrap_name = Swig_name_wrapper(Getattr(n, "name"));
  state.variable(GETTER, wrap_name);
  Setattr(n, "wrap:name", wrap_name);

  // prepare code part
  String *action = NewString("");
  String *value = Getattr(n, "rawval");
  if (value == NULL) {
    value = Getattr(n, "rawvalue");
  }
  if (value == NULL) {
    value = Getattr(n, "value");
  }
  Printf(action, "result = %s;\n", value);
  Setattr(n, "wrap:action", action);
  marshalOutput(n, action, wrapper);

  t_getter.replace(T_GETTER, wrap_name)
      .replace(T_LOCALS, wrapper->locals)
      .replace(T_CODE, wrapper->code)
      .pretty_print(f_wrappers);

  exitVariable(n);

  DelWrapper(wrapper);

  return SWIG_OK;
}

int JSEmitter::emitFunction(Node *n, bool is_member, bool is_static) {
  
  Wrapper *wrapper = NewWrapper();  
  Template t_function(getTemplate("JS_functionwrapper"));

  bool is_overloaded = GetFlag(n, "sym:overloaded");

  // prepare the function wrapper name
  String *wrap_name = Swig_name_wrapper(Getattr(n, "sym:name"));
  if (is_overloaded) {
    t_function = getTemplate("JS_functionwrapper_overload");
    Append(wrap_name, Getattr(n, "sym:overname"));
  }
  Setattr(n, "wrap:name", wrap_name);
  state.function(WRAPPER_NAME, wrap_name);

  // prepare local variables
  ParmList *params = Getattr(n, "parms");
  emit_parameter_variables(params, wrapper);
  emit_attach_parmmaps(params, wrapper);

  // prepare code part
  String *action = emit_action(n);
  marshalInputArgs(n, params, wrapper, Function, is_member, is_static);
  marshalOutput(n, action, wrapper);

  t_function.replace(T_WRAPPER, wrap_name)
      .replace(T_LOCALS, wrapper->locals)
      .replace(T_CODE, wrapper->code)
      .replace(T_ARGCOUNT, Getattr(n, ARGCOUNT))
      .pretty_print(f_wrappers);

  // handle function overloading
  if (is_overloaded) {
    Template t_dispatch_case = getTemplate("JS_function_dispatch_case");
    t_dispatch_case.replace(T_WRAPPER, wrap_name)
        .replace(T_ARGCOUNT, Getattr(n, ARGCOUNT));
    Append(state.global(FUNCTION_DISPATCHERS), t_dispatch_case.str());
  }
  
  DelWrapper(wrapper);

  return SWIG_OK;
}

int JSEmitter::emitFunctionDispatcher(Node *n, bool /*is_member */ ) {

  Template t_function(getTemplate("JS_functionwrapper"));

  Wrapper *wrapper = NewWrapper();
  String *wrap_name = Swig_name_wrapper(Getattr(n, "name"));
  Setattr(n, "wrap:name", wrap_name);

  Wrapper_add_local(wrapper, "res", "int res");

  Append(wrapper->code, state.global(FUNCTION_DISPATCHERS));
  Append(wrapper->code, getTemplate("JS_function_dispatch_case_default").str());

  t_function.replace(T_LOCALS, wrapper->locals)
      .replace(T_CODE, wrapper->code);
      
  // call this here, to replace all variables
  t_function.replace(T_WRAPPER, wrap_name)
      .replace(T_NAME, state.function(NAME))
      .pretty_print(f_wrappers);

  // Delete the state variable
  state.global(FUNCTION_DISPATCHERS, 0);
  DelWrapper(wrapper);

  return SWIG_OK;
}

void JSEmitter::emitInputTypemap(Node *n, Parm *p, Wrapper *wrapper, String *arg) {
  // Get input typemap for current param
  String *tm = Getattr(p, "tmap:in");
  SwigType *pt = Getattr(p, "type");
  
  if (tm != NULL) {
    Replaceall(tm, "$input", arg);
    Setattr(p, "emit:input", arg);

    // do replacements for built-in variables
    if (Getattr(p, "wrap:disown") || (Getattr(p, "tmap:in:disown"))) {
      Replaceall(tm, "$disown", "SWIG_POINTER_DISOWN");
    } else {
      Replaceall(tm, "$disown", "0");
    }
    Replaceall(tm, "$symname", Getattr(n, "sym:name"));
    Printf(wrapper->code, "%s\n", tm);
  } else {
    Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, "Unable to use type %s as a function argument.\n", SwigType_str(pt, 0));
  }
}

void JSEmitter::marshalOutput(Node *n, String *actioncode, Wrapper *wrapper) {
  SwigType *type = Getattr(n, "type");
  Setattr(n, "type", type);
  String *tm;

  // HACK: output types are not registered as swig_types automatically
  if (SwigType_ispointer(type)) {
    SwigType_remember_clientdata(type, NewString("0"));
  }

  if ((tm = Swig_typemap_lookup_out("out", n, "result", wrapper, actioncode))) {
    Replaceall(tm, "$result", "jsresult");
    Replaceall(tm, "$objecttype", Swig_scopename_last(SwigType_str(SwigType_strip_qualifiers(type), 0)));

    if (GetFlag(n, "feature:new")) {
      Replaceall(tm, "$owner", "SWIG_POINTER_OWN");
    } else {
      Replaceall(tm, "$owner", "0");
    }
    Append(wrapper->code, tm);

    if (Len(tm) > 0) {
      Printf(wrapper->code, "\n");
    }
  } else {
    Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(type, 0), Getattr(n, "name"));
  }
  emit_return_variable(n, type, wrapper);
}

Hash *JSCEmitter::createNamespaceEntry(const char *name, const char *parent) {
  Hash *entry = JSEmitter::createNamespaceEntry(name, parent);
  Setattr(entry, "functions", NewString(""));
  Setattr(entry, "values", NewString(""));
  return entry;
}

int JSCEmitter::emitNamespaces() {
  Iterator it;
  for (it = First(namespaces); it.item; it = Next(it)) {
    Hash *entry = it.item;
    String *name = Getattr(entry, NAME);
    String *name_mangled = Getattr(entry, NAME_MANGLED);
    String *parent = Getattr(entry, PARENT);
    String *parent_mangled = Swig_name_mangle(parent);
    String *functions = Getattr(entry, "functions");
    String *variables = Getattr(entry, "values");

    Template namespace_definition(getTemplate("JS_globaldefn"));
    namespace_definition.replace("$jsglobalvariables", variables)
        .replace("$jsglobalfunctions", functions)
        .replace(T_NAMESPACE, name_mangled)
        .pretty_print(f_wrap_cpp);

    Template t_createNamespace(getTemplate("JS_create_namespace"));
    t_createNamespace.replace(T_NAME_MANGLED, name_mangled);
    Append(state.global(CREATE_NAMESPACES), t_createNamespace.str());

    Template t_registerNamespace(getTemplate("JS_register_namespace"));
    t_registerNamespace.replace(T_NAME_MANGLED, name_mangled)
        .replace(T_NAME, name)
        .replace(T_PARENT, parent_mangled);
    Append(state.global(REGISTER_NAMESPACES), t_registerNamespace.str());
  }

  return SWIG_OK;
}

JSEmitter *swig_javascript_create_JSCEmitter() {
  return new JSCEmitter();
}

class V8Emitter: public JSEmitter {
    
public:

  V8Emitter();

  virtual ~V8Emitter();
  
  virtual int initialize(Node *n);

  virtual int dump(Node *n);
  
  virtual int close();
  
  virtual int enterClass(Node *n);
  
  virtual int exitClass(Node *n);

  virtual int enterVariable(Node *n);

  virtual int exitVariable(Node *n);

  virtual int enterFunction(Node *n);

  virtual int exitFunction(Node *n);

protected:

  virtual void marshalInputArgs(Node *n, ParmList *parms, Wrapper *wrapper, MarshallingMode mode, bool is_member, bool is_static);

  virtual int emitNamespaces();

private:

  File *f_runtime;
  File *f_header;
  File *f_class_templates;
  File *f_init;
  
  File *f_init_namespaces;
  File *f_init_class_templates;
  File *f_init_wrappers;
  File *f_init_inheritance;
  File *f_init_class_instances;
  File *f_init_static_wrappers;
  File *f_init_register_classes;
  File *f_init_register_namespaces;
  
  // the output cpp file
  File *f_wrap_cpp;
      
  String* GLOBAL;
  String* NULL_STR;
};

V8Emitter::V8Emitter() 
: JSEmitter(), 
  GLOBAL(NewString("global")),
  NULL_STR(NewString("0"))
{
}

V8Emitter::~V8Emitter()
{
  Delete(GLOBAL);
  Delete(NULL_STR);
}

int V8Emitter::initialize(Node *n)
{
  JSEmitter::initialize(n);
  
  // Get the output file name
  String *outfile = Getattr(n,"outfile");
  f_wrap_cpp = NewFile(outfile, "w", SWIG_output_files());
  if (!f_wrap_cpp) {
     FileErrorDisplay(outfile);
     SWIG_exit(EXIT_FAILURE);
  }

  f_runtime = NewString("");
  f_header = NewString("");
  f_class_templates = NewString("");
  f_init = NewString("");
      
  f_init_namespaces = NewString("");
  f_init_class_templates = NewString("");
  f_init_wrappers = NewString("");
  f_init_inheritance = NewString("");
  f_init_class_instances = NewString("");
  f_init_static_wrappers = NewString("");
  f_init_register_classes = NewString("");
  f_init_register_namespaces = NewString("");

  // note: this is necessary for built-in generation of swig runtime code
  Swig_register_filebyname("runtime", f_runtime);
  Swig_register_filebyname("header", f_header);
  Swig_register_filebyname("init", f_init);

  return SWIG_OK;
}

int V8Emitter::dump(Node *n)
{
   // Get the module name
  String* module = Getattr(n,"name");

 // write the swig banner
  Swig_banner(f_wrap_cpp);

  SwigType_emit_type_table(f_runtime, f_wrappers);

  Printv(f_wrap_cpp, f_runtime, "\n", 0);
  Printv(f_wrap_cpp, f_header, "\n", 0);
  Printv(f_wrap_cpp, f_class_templates, "\n", 0);
  Printv(f_wrap_cpp, f_wrappers, "\n", 0);

  emitNamespaces();

  // compose the initializer function using a template
  // filled with sub-parts
  Template initializer(getTemplate("JS_initializer"));
  initializer.replace(T_NAME, module)
      .replace(V8_NAME_SPACES,        f_init_namespaces)
      .replace(V8_CLASS_TEMPLATES,    f_init_class_templates)
      .replace(V8_WRAPPERS,           f_init_wrappers)
      .replace(V8_INHERITANCE,        f_init_inheritance)
      .replace(V8_CLASS_INSTANCES,    f_init_class_instances)
      .replace(V8_STATIC_WRAPPERS,    f_init_static_wrappers)
      .replace(V8_REGISTER_CLASSES,   f_init_register_classes)
      .replace(V8_REGISTER_NS,        f_init_register_namespaces)
      .pretty_print(f_wrap_cpp);

  return SWIG_OK;
}

int V8Emitter::close()
{
  // strings
  Delete(f_runtime);
  Delete(f_header);
  Delete(f_class_templates);
  Delete(f_init_namespaces);
  Delete(f_init_class_templates);
  Delete(f_init_wrappers);
  Delete(f_init_inheritance);
  Delete(f_init_class_instances);
  Delete(f_init_static_wrappers);
  Delete(f_init_register_classes);
  Delete(f_init_register_namespaces);    
  
  // files
  Close(f_wrap_cpp);
  Delete(f_wrap_cpp);
  
  return SWIG_OK;
}

int V8Emitter::enterClass(Node *n)
{
  JSEmitter::enterClass(n);
    
  // emit declaration of a v8 class template
  Template t_decl_class(getTemplate("jsv8_declare_class_template"));
  t_decl_class.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .pretty_print(f_class_templates);

  // emit definition of v8 class template
  Template t_def_class(getTemplate("jsv8_define_class_template"));
  t_def_class.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .replace(T_NAME, state.clazz(NAME))
      .pretty_print(f_init_class_templates);

  Template t_class_instance(getTemplate("jsv8_create_class_instance"));
  t_class_instance.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .pretty_print(f_init_class_instances);

  return SWIG_OK;
}

int V8Emitter::exitClass(Node *n)
{
  //  emit inheritance setup
  Node* baseClass = getBaseClass(n);
  if(baseClass) {
    Template t_inherit(getTemplate("jsv8_inherit"));
    String *base_name_mangled = SwigType_manglestr(Getattr(baseClass, "name"));
    t_inherit.replace(T_NAME_MANGLED,  state.clazz(NAME_MANGLED))
        .replace(T_BASECLASS, base_name_mangled)
        .pretty_print(f_init_inheritance);
    Delete(base_name_mangled);
  }
      
  //  emit registeration of class template
  Template t_register(getTemplate("jsv8_register_class"));
  t_register.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
      .replace(T_NAME,   state.clazz(NAME))
      .replace(T_PARENT, Getattr(current_namespace, "name_mangled"))
      .pretty_print(f_init_register_classes);
  
  return SWIG_OK;
}

int V8Emitter::enterVariable(Node* n)
{
  JSEmitter::enterVariable(n);
  
  state.variable(GETTER, NULL_STR);
  state.variable(SETTER, NULL_STR);

  return SWIG_OK;
}

int V8Emitter::exitVariable(Node* n)
{
  if(GetFlag(n, "ismember")) {
    if(GetFlag(state.variable(), IS_STATIC)) {
      Template t_register(getTemplate("jsv8_register_static_variable"));
      String *class_instance = NewString("");
      Printf(class_instance, "class_%s", state.clazz(NAME_MANGLED));
      t_register.replace(T_PARENT, class_instance)
          .replace(T_NAME, state.variable(NAME))
          .replace(T_GETTER, state.variable(GETTER))
          .replace(T_SETTER, state.variable(SETTER))
          .pretty_print(f_init_static_wrappers);
      Delete(class_instance);
    } else {
      Template t_register(getTemplate("jsv8_register_member_variable"));
      t_register.replace(T_NAME_MANGLED, state.clazz(NAME_MANGLED))
          .replace(T_NAME, state.clazz(NAME))
          .replace(T_GETTER, state.variable(GETTER))
          .replace(T_SETTER, state.variable(SETTER))
          .pretty_print(f_init_wrappers);
    }
  } else {
    // Note: a global variable is treated like a static variable
    //       with the parent being a nspace object (instead of class object) 
    Template t_register(getTemplate("jsv8_register_static_variable"));
    t_register.replace(T_PARENT, Getattr(current_namespace, "name"))
        .replace(T_NAME, state.variable(NAME))
        .replace(T_GETTER, state.variable(GETTER))
        .replace(T_SETTER, state.variable(SETTER))
        .pretty_print(f_init_wrappers);
  }
    
  return SWIG_OK;
}

int V8Emitter::enterFunction(Node* n)
{
  JSEmitter::enterFunction(n);
      
  return SWIG_OK;
}

int V8Emitter::exitFunction(Node* n)
{    
  // register the function at the specific context
  if(GetFlag(n, "ismember")) {
    if(GetFlag(state.function(), IS_STATIC)) {
      Template t_register(getTemplate("jsv8_register_static_function"));
      String *class_instance = NewString("");
      Printf(class_instance, "class_%s", state.clazz(NAME_MANGLED));
      t_register.replace(T_PARENT, class_instance)
          .replace(T_NAME, state.function(NAME))
          .replace(T_WRAPPER, Getattr(n, "wrap:name"));
      Printv(f_init_static_wrappers, t_register.str(), 0);
      Delete(class_instance);
    } else {
      Template t_register(getTemplate("jsv8_register_member_function"));
      t_register.replace(T_PARENT, state.clazz(NAME_MANGLED))
          .replace(T_NAME, state.function(NAME))
          .replace(T_WRAPPER, Getattr(n, "wrap:name"));
      Printv(f_init_wrappers, t_register.str(), "\n", 0);
    }
  } else {
    // Note: a global function is treated like a static function
    //       with the parent being a nspace object instead of class object 
    Template t_register(getTemplate("jsv8_register_static_function"));
    t_register.replace(T_PARENT, Getattr(current_namespace, "name"))
        .replace(T_NAME, state.function(NAME))
        .replace(T_WRAPPER, Getattr(n, "wrap:name"));
    Printv(f_init_wrappers, t_register.str(), 0);
  }

  return SWIG_OK;
}

void V8Emitter::marshalInputArgs(Node *n, ParmList *parms, Wrapper *wrapper, MarshallingMode mode, bool is_member, bool is_static) {
  Parm *p;

  int startIdx = 0;
  if (is_member && !is_static) {
    startIdx = 1;
  }

  // store number of arguments for argument checks
  int num_args = emit_num_arguments(parms) - startIdx;
  String *argcount = NewString("");
  Printf(argcount, "%d", num_args);
  Setattr(n, ARGCOUNT, argcount);

  int i = 0;
  for (p = parms; p; p = nextSibling(p), i++) {
    String *arg = NewString("");

    switch (mode) {
    case Getter:
      if (is_member && !is_static && i == 0) {
        Printv(arg, "info.Holder()", 0);
      } else {
        Printf(arg, "args[%d]", i - startIdx);
      }
      break;
    case Function:
      if (is_member && !is_static && i == 0) {
        Printv(arg, "args.Holder()", 0);
      } else {
        Printf(arg, "args[%d]", i - startIdx);
      }
      break;
    case Setter:
      if (is_member && !is_static && i == 0) {
        Printv(arg, "value", 0);
      } else {
        Printv(arg, "value", 0);
      }
      break;
    case Ctor:
      Printf(arg, "args[%d]", i);
      break;
    default:
      throw "Illegal state.";
    }
    emitInputTypemap(n, p, wrapper, arg);
    Delete(arg);
  }
}

int V8Emitter::emitNamespaces() {
// TODO: handle namespaces:
/*
    // create namespace object and register it to the parent scope
    Template t_create_ns(getTemplate(V8_CREATE_NAMESPACE));
    t_create_ns.Replace(V8_MANGLED_NAME, scope_mangled);
    
    Template t_register_ns(getTemplate(V8_REGISTER_NAMESPACE));
    t_register_ns.Replace(V8_MANGLED_NAME, scope_mangled)
        .Replace(V8_CONTEXT, parent_scope_mangled)
        .Replace(V8_UNQUALIFIED_NAME, scope_unqualified);

    Printv(f_init_namespaces, t_create_ns.str(), 0);
    // prepend in order to achieve reversed order of registration statements
    Insert(f_init_register_namespaces, 0, t_register_ns.str());
*/
  
  return SWIG_OK;
}


JSEmitter *swig_javascript_create_V8Emitter() {
  return new V8Emitter();
}


/**********************************************************************
 * Helper implementations
 **********************************************************************/

JSEmitterState::JSEmitterState() 
  : _global(NewHash()) 
{
  // initialize sub-hashes
  Setattr(_global, "class", NewHash());
  Setattr(_global, "function", NewHash());
  Setattr(_global, "variable", NewHash());
}

JSEmitterState::~JSEmitterState() 
{ 
  Delete(_global);
}
  
DOH *JSEmitterState::getState(const char* key, bool _new) 
{
  if (_new) {
    Hash *hash = NewHash();
    Setattr(_global, key, hash);
  }
  return Getattr(_global, key);
}
  
DOH *JSEmitterState::global() {
  return _global;
}

DOH *JSEmitterState::global(const char* key, DOH *initial)
{
  if(initial != 0) {
    Setattr(_global, key, initial);
  }
  return Getattr(_global, key);
}

DOH *JSEmitterState::clazz(bool _new)
{
  return getState("class", _new);
}
  
DOH *JSEmitterState::clazz(const char* key, DOH *initial)
{
  DOH *c = clazz();
  if(initial != 0) {
    Setattr(c, key, initial);
  }
  return Getattr(c, key);
}

DOH *JSEmitterState::function(bool _new)
{
  return getState("function", _new);
}

DOH *JSEmitterState::function(const char* key, DOH *initial)
{
  DOH *f = function();
  if(initial != 0) {
    Setattr(f, key, initial);
  }
  return Getattr(f, key);
}

DOH *JSEmitterState::variable(bool _new)
{
  return getState("variable", _new);
}

DOH *JSEmitterState::variable(const char* key, DOH *initial) 
{
  DOH *v = variable();
  if(initial != 0) {
    Setattr(v, key, initial);
  }
  return Getattr(v, key);
}
  
/*static*/
int JSEmitterState::IsSet(DOH *val) 
{
  if (!val) {
    return 0;
  } else {
    const char *cval = Char(val);
    if (!cval)
      return 0;
    return (strcmp(cval, "0") != 0) ? 1 : 0;
  }
}

/* -----------------------------------------------------------------------------
 * Template::Template() :  creates a Template class for given template code
 * ----------------------------------------------------------------------------- */

Template::Template(const String *code_) {

  if (!code_) {
    Printf(stdout, "Template code was null. Illegal input for template.");
    SWIG_exit(EXIT_FAILURE);
  }
  code = NewString(code_);
  templateName = NewString("");
}

Template::Template(const String *code_, const String *templateName_) {

  if (!code_) {
    Printf(stdout, "Template code was null. Illegal input for template.");
    SWIG_exit(EXIT_FAILURE);
  }

  code = NewString(code_);
  templateName = NewString(templateName_);
}


/* -----------------------------------------------------------------------------
 * Template::~Template() :  cleans up of Template.
 * ----------------------------------------------------------------------------- */

Template::~Template() {
  Delete(code);
  Delete(templateName);
}

/* -----------------------------------------------------------------------------
 * String* Template::str() :  retrieves the current content of the template.
 * ----------------------------------------------------------------------------- */

String *Template::str() {
  if (js_template_enable_debug) {
    String *pre_code = NewString("");
    String *post_code = NewString("");
    String *debug_code = NewString("");
    Printf(pre_code, "//begin fragment(\"%s\")\n", templateName);
    Printf(post_code, "//end fragment(\"%s\")\n", templateName);
    Printf(debug_code, "%s\n%s\n%s", pre_code, code, post_code);

    Delete(code);
    Delete(pre_code);
    Delete(post_code);

    code = debug_code;
  }
  return code;
}

/* -----------------------------------------------------------------------------
 * Template&  Template::replace(const String* pattern, const String* repl) :
 * 
 *  replaces all occurences of a given pattern with a given replacement.
 * 
 *  - pattern:  the pattern to be replaced
 *  - repl:     the replacement string
 *  - returns a reference to the Template to allow chaining of methods.
 * ----------------------------------------------------------------------------- */

Template& Template::replace(const String *pattern, const String *repl) {
  Replaceall(code, pattern, repl);
  return *this;
}

Template& Template::pretty_print(DOH *doh) {
  Wrapper_pretty_print(str(), doh);
  return *this;
}

void Template::operator=(const Template& t) {
  Delete(code);
  Delete(templateName);
  code = NewString(t.code);
  templateName = NewString(t.templateName);
}
