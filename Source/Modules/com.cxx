/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * com.cxx
 *
 * COM module for SWIG.
 * ----------------------------------------------------------------------------- */

char cvsroot_com_cxx[] = "$Id$";

#include "swigmod.h"
#include "cparse.h"

typedef struct {
  unsigned long Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char Data4[8];
} GUID, UUID;

bool isNilGUID(GUID *arg) {
  if (arg->Data1 != 0 || arg->Data2 != 0 || arg->Data3 != 0)
    return false;

  for (int i = 0; i < 8; ++i)
    if (arg->Data4[i] != 0)
      return false;

  return true;
}

typedef struct {
  unsigned char octets[20];
} SHA1_hash;

/* Rotate left a 32-bit number */
unsigned int leftrot(unsigned int a, unsigned int pos) {
  return (a << pos) | ((a >> (32 - pos)) & ((1 << pos) - 1));
}

/* Implementation of SHA1. Needs unsigned int with at least 32 bits */
SHA1_hash SHA1(char *input_bytes, int input_len) {
  unsigned int h0 = 0x67452301;
  unsigned int h1 = 0xEFCDAB89;
  unsigned int h2 = 0x98BADCFE;
  unsigned int h3 = 0x10325476;
  unsigned int h4 = 0xC3D2E1F0;

  /*
   * Preprocessing: a '1'-bit is appended, followed by '0'-bits
   * until the number of bits % 512 == 448. Then the input's length
   * in bits is appended as a 64-bit number. The preprocessed input's
   * length is a multiple of 512 bits (64 bytes). Please note
   * that below the counting is in bytes instead of bits.
   */
  unsigned int padding = ((64 + 56) - (input_len + 1) % 64) % 64;
  unsigned int total_len = input_len + 1 + padding + 8;
  /* Preprocessed input */
  unsigned char *prep = new unsigned char[total_len];

  /* Copy the input to prep */
  for (unsigned int i = 0; i < input_len; ++i)
    prep[i] = input_bytes[i];

  /* Marker with single '1'-bit in most significant position */
  prep[input_len] = 0x80;

  /* Pad with 0's */
  for (unsigned int i = 0; i < padding; ++i)
    prep[input_len + 1 + i] = 0;

  /* Finally append 8 * input_len as a 64-bit number in big-endian format */
  unsigned int high_dword = (input_len >> 29) & 7;
  unsigned int low_dword = (input_len & ((1 << 29) - 1)) << 3;

  prep[total_len - 8] = high_dword >> 24;
  prep[total_len - 7] = (high_dword >> 16) & 0xff;
  prep[total_len - 6] = (high_dword >> 8) & 0xff;
  prep[total_len - 5] = high_dword & 0xff;
  prep[total_len - 4] = low_dword >> 24;
  prep[total_len - 3] = (low_dword >> 16) & 0xff;
  prep[total_len - 2] = (low_dword >> 8) & 0xff;
  prep[total_len - 1] = low_dword & 0xff;

  /* Divide preprocessed input into 512 bit chunks */
  for (unsigned int chunk = 0; chunk < total_len / 64; ++chunk) {
    unsigned int w[80];

    for (int i = 0; i < 16; ++i) {
      w[i] = ((unsigned int) prep[64 * chunk + 4 * i] << 24) |
          ((unsigned int) prep[64 * chunk + 4 * i + 1] << 16) |
          ((unsigned int) prep[64 * chunk + 4 * i + 2] << 8) |
          (unsigned int) prep[64 * chunk + 4 * i + 3];
    }

    for (int i = 16; i < 80; ++i) {
      w[i] = leftrot(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }

    unsigned int a = h0;
    unsigned int b = h1;
    unsigned int c = h2;
    unsigned int d = h3;
    unsigned int e = h4;

    for (int i = 0; i < 80; ++i) {
      unsigned int f, k;

      if (i < 20) {
        f = (b & c) | (~b & d);
        k = 0x5A827999;
      } else if (i < 40) {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      } else if (i < 60) {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDC;
      } else {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }

      int temp = leftrot(a, 5) + f + e + k + w[i];
      e = d;
      d = c;
      c = leftrot(b, 30);      
      b = a;
      a = temp;
    }

    h0 = h0 + a;
    h1 = h1 + b;
    h2 = h2 + c;
    h3 = h3 + d;
    h4 = h4 + e;
  }

  delete prep;

  SHA1_hash res = {
      (h0 >> 24) & 0xff, (h0 >> 16) & 0xff, (h0 >> 8) & 0xff, h0 & 0xff,
      (h1 >> 24) & 0xff, (h1 >> 16) & 0xff, (h1 >> 8) & 0xff, h1 & 0xff,
      (h2 >> 24) & 0xff, (h2 >> 16) & 0xff, (h2 >> 8) & 0xff, h2 & 0xff,
      (h3 >> 24) & 0xff, (h3 >> 16) & 0xff, (h3 >> 8) & 0xff, h3 & 0xff,
      (h4 >> 24) & 0xff, (h4 >> 16) & 0xff, (h4 >> 8) & 0xff, h4 & 0xff,
  };

  return res;
}

class COM:public Language {
  static const char *usage;
  const String *empty_string;

  Hash *swig_types_hash;
  List *filenames_list;

  File *f_runtime;
  File *f_header;
  File *f_module;
  File *f_deffile;
  File *f_rcfile;
  File *f_wrappers;
  File *f_proxy;
  File *f_proxy_forward_defs;
  File *f_vtables;
  File *f_vtable_defs;
  File *f_factory;

  bool proxy_flag;		// Flag for generating proxy classes
  bool dllexports_flag;
  bool deffile_flag;
  bool rcfile_flag;
  bool enum_constant_flag;	// Flag for when wrapping an enum or constant
  bool static_flag;		// Flag for when wrapping a static functions or member variables
  bool variable_wrapper_flag;	// Flag for when wrapping a nonstatic member variable
  bool wrapping_member_flag;	// Flag for when wrapping a member variable/enum/const
  bool global_variable_flag;	// Flag for when wrapping a global variable
  bool generate_property_declaration_flag;	// Flag for generating properties
  bool member_func_flag;
  bool constructor_flag;

  String *proxy_class_def;
  String *proxy_class_forward_def;
  String *proxy_class_code;
  String *proxy_class_name;
  String *proxy_class_constants_code;
  String *clsid_list;
  List *proxy_class_member_functions;
  String *variable_name;	//Name of a variable being wrapped
  GUID *proxy_iid;
  GUID *proxy_clsid;
  GUID guid_seed;
  GUID typelib_guid;
  GUID module_iid;
  GUID module_clsid;

  String *module_class_vtable_code;
  String *proxy_class_vtable_code;
  String *proxy_class_vtable_defs;

  String *module_class_name;	// module class name
  String *module_class_code;

public:

  /* -----------------------------------------------------------------------------
   * COM()
   * ----------------------------------------------------------------------------- */

  COM():empty_string(NewString("")),
      proxy_flag(true),
      deffile_flag(true),
      rcfile_flag(true),
      dllexports_flag(true),
      enum_constant_flag(false),
      proxy_class_vtable_code(NewString("")),
      proxy_class_vtable_defs(NewString("")),
      clsid_list(NewString("")) {
    /* Use NIL GUID by default */
    memset(&guid_seed, 0, sizeof(GUID));
    memset(&typelib_guid, 0, sizeof(GUID));
    memset(&module_iid, 0, sizeof(GUID));
    memset(&module_clsid, 0, sizeof(GUID));
  }

  /* -----------------------------------------------------------------------------
   * getProxyName()
   *
   * Test to see if a type corresponds to something wrapped with a proxy class
   * Return NULL if not otherwise the proxy class name
   * ----------------------------------------------------------------------------- */
  
   String *getProxyName(SwigType *t) {
    if (proxy_flag) {
      Node *n = classLookup(t);
      if (n) {
	return Getattr(n, "sym:name");
      }
    }
    return NULL;
  }

  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */

  virtual void main(int argc, char *argv[]) {

    SWIG_library_directory("com");

    // Look for certain command line options
    for (int i = 1; i < argc; i++) {
      if (argv[i]) {
	if (strcmp(argv[i], "-help") == 0) {
	  Printf(stdout, "%s\n", usage);
	} else if ((strcmp(argv[i], "-nodllexports") == 0)) {
	  Swig_mark_arg(i);
          dllexports_flag = false;
          deffile_flag = false;
          rcfile_flag = false;
	} else if (strcmp(argv[i], "-nodeffile") == 0) {
	  Swig_mark_arg(i);
          deffile_flag = false;
	} else if (strcmp(argv[i], "-norcfile") == 0) {
	  Swig_mark_arg(i);
          rcfile_flag = false;
	}
      }
    }
    // Add a symbol to the parser for conditional compilation
    Preprocessor_define("SWIGCOM 1", 0);

    // Add typemap definitions
    SWIG_typemap_lang("com");
    SWIG_config_file("com.swg");

    allow_overloading(0);
  }

  /* ---------------------------------------------------------------------
   * top()
   * --------------------------------------------------------------------- */

  virtual int top(Node *n) {

    // Get any options set in the module directive
    Node *optionsnode = Getattr(Getattr(n, "module"), "options");

    if (optionsnode) {
      if (Getattr(optionsnode, "guidseed")) {
	if (!parseGUID(Getattr(optionsnode, "guidseed"), &guid_seed)) {
          /* Bad GUID */
          /* FIXME: report an error */
        }
      }
      if (Getattr(optionsnode, "tlbid")) {
	if (!parseGUID(Getattr(optionsnode, "tlbid"), &typelib_guid)) {
          /* Bad GUID */
          /* FIXME: report an error */
        }
      }
      if (Getattr(optionsnode, "moduleiid")) {
	if (!parseGUID(Getattr(optionsnode, "moduleiid"), &module_iid)) {
          /* Bad GUID */
          /* FIXME: report an error */
        }
      }
      if (Getattr(optionsnode, "moduleclsid")) {
	if (!parseGUID(Getattr(optionsnode, "moduleclsid"), &module_clsid)) {
          /* Bad GUID */
          /* FIXME: report an error */
        }
      }
    }

    /* Initialize all of the output files */
    String *outfile = Getattr(n, "outfile");
    String *outfile_h = Getattr(n, "outfile_h");

    if (!outfile) {
      Printf(stderr, "Unable to determine outfile\n");
      SWIG_exit(EXIT_FAILURE);
    }

    f_runtime = NewFile(outfile, "w");
    if (!f_runtime) {
      FileErrorDisplay(outfile);
      SWIG_exit(EXIT_FAILURE);
    }

    f_header = NewString("");
    f_wrappers = NewString("");
    f_proxy = NewString("");
    f_proxy_forward_defs = NewString("");
    f_vtables = NewString("");
    f_vtable_defs = NewString("");
    f_factory = NewString("");

    /* Register file targets with the SWIG file handler */
    Swig_register_filebyname("header", f_header);
    Swig_register_filebyname("runtime", f_runtime);
    Swig_register_filebyname("factory", f_factory);

    swig_types_hash = NewHash();
    filenames_list = NewList();

    /* FIXME: do it as it is done in other targets */
    module_class_name = Copy(Getattr(n, "name"));

    module_class_code = NewString("");
    proxy_class_def = NewString("");
    proxy_class_forward_def = NewString("");
    proxy_class_code = NewString("");

    if (isNilGUID(&typelib_guid)) {
      String *tlbid_ident = NewStringf("%s.TLBID", module_class_name);
      generateGUID(&typelib_guid, tlbid_ident);
      Delete(tlbid_ident);
    }

    if (isNilGUID(&module_iid)) {
      String *module_iid_ident = NewStringf("%s.IID", module_class_name);
      generateGUID(&module_iid, module_iid_ident);
      Delete(module_iid_ident);
    }

    if (isNilGUID(&module_clsid)) {
      String *module_clsid_ident = NewStringf("%s.CLSID", module_class_name);
      generateGUID(&module_clsid, module_clsid_ident);
      Delete(module_clsid_ident);
    }

    module_class_vtable_code = NewString("");

    Printf(module_class_vtable_code, "DEFINE_GUID(IID_%s, ", module_class_name);
    formatGUID(module_class_vtable_code, &module_iid, true);
    Printf(module_class_vtable_code, ");\n\n");

    Printf(module_class_vtable_code, "DEFINE_GUID(CLSID_%s, ", module_class_name);
    formatGUID(module_class_vtable_code, &module_clsid, true);
    Printf(module_class_vtable_code, ");\n\n");

    Printf(module_class_vtable_code, "HRESULT SWIGSTDCALL _wrap%sQueryInterface(void *that, REFIID iid, "
        "void ** ppvObject) {\n", module_class_name);

    Printf(module_class_vtable_code, "  if (IsEqualIID(iid, IID_IUnknown) ||\n"
        "      IsEqualIID(iid, IID_IDispatch) ||\n"
        "      IsEqualIID(iid, IID_%s)", module_class_name);

    Printf(module_class_vtable_code, ") {\n"
        "    /* FIXME: This could be more elegant */\n"
        "    SWIGAddRef1(that);\n"
        "    *ppvObject = that;\n"
        "    return S_OK;\n"
        "  }\n\n");

    Printf(module_class_vtable_code, "  return E_NOINTERFACE;\n}\n\n");

    Printf(module_class_vtable_code, "extern SWIG_funcptr _wrap%s_vtable[];\n\n", module_class_name);

    Printf(module_class_vtable_code,
        "void * SWIGSTDCALL _wrap_new_%s(void *SWIG_ignored) {\n"
        "#ifdef __cplusplus\n"
        "  SWIGWrappedObject *res = new SWIGWrappedObject;\n"
        "#else\n"
        "  SWIGWrappedObject *res = (SWIGWrappedObject *) malloc(sizeof(SWIGWrappedObject));\n"
        "#endif\n"
        "  res->vtable = _wrap%s_vtable;\n"
        "  res->SWIGWrappedObject_vtable = NULL;\n"
        "  /* cPtr and cMemOwn make no sense for the module class */\n"
        "  res->cPtr = NULL;\n"
        "  res->cMemOwn = 0;\n"
        "  InterlockedIncrement(&globalRefCount);\n"
        "  res->refCount = 1;\n"
        "  res->deleteInstance = 0;\n"
        "  /* GetTypeInfoOfGuid */\n"
        "  ((HRESULT (SWIGSTDCALL *)(ITypeLib *, REFGUID, ITypeInfo **)) (((SWIGIUnknown *) SWIG_typelib)->vtable[6]))(SWIG_typelib, IID_%s, &res->typeInfo);\n"
        "  return (void *) res;\n"
        "};\n\n",
        module_class_name, module_class_name, module_class_name);

    Printf(module_class_vtable_code, "SWIG_funcptr _wrap%s_vtable[] = "
        "{\n  (SWIG_funcptr) _wrap%sQueryInterface,"
        "\n  (SWIG_funcptr) SWIGAddRef1,"
        "\n  (SWIG_funcptr) SWIGRelease1,"
        "\n  (SWIG_funcptr) SWIGGetTypeInfoCount,"
        "\n  (SWIG_funcptr) SWIGGetTypeInfo,"
        "\n  (SWIG_funcptr) SWIGGetIDsOfNames,"
        "\n  (SWIG_funcptr) SWIGInvoke",
        module_class_name, module_class_name);

    Printf(clsid_list, "static TCHAR * SWIG_tlb_guid_string = _T(\"{");
    formatGUID(clsid_list, &typelib_guid, false);
    Printf(clsid_list, "}\");\n\n");

    Printf(clsid_list, "static SWIGClassDescription_t SWIGClassDescription[] = {\n");
    Printf(clsid_list, "  { (SWIG_funcptr) _wrap_new_%s, CLSID_%s, _T(\"{", module_class_name, module_class_name);
    formatGUID(clsid_list, &module_clsid, false);
    Printf(clsid_list,  "}\") },\n");

    /* Emit code */
    Language::top(n);

    /* Insert guard element */
    Printf(clsid_list, "  { NULL, IID_IUnknown, NULL } /* guard element */\n");

    Printf(clsid_list, "};\n\n");

    Printv(module_class_vtable_code, "\n};\n\n", NIL);

    // Output a COM type wrapper class for each SWIG type
    for (Iterator swig_type = First(swig_types_hash); swig_type.key; swig_type = Next(swig_type)) {
      emitTypeWrapperClass(swig_type.key, swig_type.item);
    }

    /* Generate DEF file */
    if (deffile_flag) {
      String *filen = NewStringf("%s%s.def", SWIG_output_directory(), module_class_name);
      f_deffile = NewFile(filen, "w");
      if (!f_deffile) {
	FileErrorDisplay(filen);
	SWIG_exit(EXIT_FAILURE);
      }
      // Append(filenames_list, Copy(filen));
      Delete(filen);
      filen = NULL;

      Printf(f_deffile, "LIBRARY %s\n", module_class_name);

      Printf(f_deffile, "EXPORTS\n"
          "  DllGetClassObject PRIVATE\n"
          "  DllCanUnloadNow PRIVATE\n"
          "  DllRegisterServer PRIVATE\n"
          "  DllUnregisterServer PRIVATE\n"
          "  DllMain\n");
    }

    if (rcfile_flag) {
      String *filen = NewStringf("%s%s_rc.rc", SWIG_output_directory(), module_class_name);
      f_rcfile = NewFile(filen, "w");
      if (!f_rcfile) {
	FileErrorDisplay(filen);
	SWIG_exit(EXIT_FAILURE);
      }
      // Append(filenames_list, Copy(filen));
      Delete(filen);
      filen = NULL;

      Printf(f_rcfile, "1 typelib \"%s.tlb\"\n", module_class_name);
    }

    /* Generate the IDL file containing the module class and proxy classes */
    {
      String *filen = NewStringf("%s%s.idl", SWIG_output_directory(), module_class_name);
      f_module = NewFile(filen, "w");
      if (!f_module) {
	FileErrorDisplay(filen);
	SWIG_exit(EXIT_FAILURE);
      }
      // Append(filenames_list, Copy(filen));
      Delete(filen);
      filen = NULL;

      // Banner for the IDL file
      emitBanner(f_module);

      // Standard imports
      Printf(f_module, "[\n  uuid(");
      formatGUID(f_module, &typelib_guid, false);
      Printf(f_module, ")\n]\nlibrary %sTLB {\n\n", module_class_name);

      // Import IDispatch
      // FIXME: Printf(f_module, "  importlib(\"stdole32.tlb\");\n\n");
      Printf(f_module, "  importlib(\"stdole2.tlb\");\n\n");

      Printv(f_module, f_proxy_forward_defs, "\n", NIL);

      // Interface for module class
      Printf(f_module, "  [\n    object,\n    local,\n    uuid(");
      formatGUID(f_module, &module_iid, false);
      Printf(f_module, "),\n    dual\n  ]\n  interface %s : IDispatch {\n", module_class_name);
      // FIXME: a temporary workaround for a possible WIDL bug
      Printf(f_module, "#ifdef __WIDL__\n");
      Printf(f_module, "    import \"stdole2.idl\";\n");
      Printf(f_module, "#endif\n");

      // Add the wrapper methods
      Printv(f_module, module_class_code, NIL);

      Printf(f_module, "  };\n\n");

      Printv(f_module, "  [\n    uuid(", NIL);
      formatGUID(f_module, &module_clsid, false);
      Printv(f_module, ")\n  ]\n  coclass ", module_class_name, "Impl {\n"
          "    interface ", module_class_name, ";\n  };\n\n", NIL);

      // Add the proxy code
      Printv(f_module, f_proxy, NIL);

      Printf(f_module, "};\n");
    }

    /* Close all of the files */
    Dump(f_header, f_runtime);
    Delete(f_header);
    Printv(f_runtime, f_vtable_defs, "\n", NIL);
    Delete(f_vtable_defs);
    Dump(f_wrappers, f_runtime);
    Delete(f_wrappers);
    // Vtable for the module class
    Printv(f_runtime, module_class_vtable_code, NIL);
    Dump(f_vtables, f_runtime);
    Delete(f_vtables);
    Dump(clsid_list, f_runtime);
    Delete(clsid_list);
    Replaceall(f_factory, "$module", module_class_name);
    if (dllexports_flag)
      Dump(f_factory, f_runtime);
    Delete(f_factory);
    Close(f_runtime);
    Delete(f_runtime);
    Close(f_module);
    Delete(f_module);
    if (deffile_flag) {
      Close(f_deffile);
      Delete(f_deffile);
    }
    if (rcfile_flag) {
      Close(f_rcfile);
      Delete(f_rcfile);
    }
    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * emitBanner()
   * ----------------------------------------------------------------------------- */

  void emitBanner(File *f) {
    Printf(f, "/* ----------------------------------------------------------------------------\n");
    Printf(f, " * This file was automatically generated by SWIG (http://www.swig.org).\n");
    Printf(f, " * Version %s\n", Swig_package_version());
    Printf(f, " *\n");
    Printf(f, " * Do not make changes to this file unless you know what you are doing--modify\n");
    Printf(f, " * the SWIG interface file instead.\n");
    Printf(f, " * ----------------------------------------------------------------------------- */\n\n");
  }

  /* -----------------------------------------------------------------------------
   * functionWrapper()
   * ----------------------------------------------------------------------------- */

  virtual int functionWrapper(Node *n) {
    String *symname = Getattr(n, "sym:name");
    SwigType *t = Getattr(n, "type");
    ParmList *l = Getattr(n, "parms");
    String *tm;
    Parm *p;
    int i;
    String *c_return_type = NewString("");
    bool is_void_return;
    int num_arguments = 0;
    int num_required = 0;

    /* FIXME */
    String *overloaded_name = Copy(symname);

    // A new wrapper function object
    Wrapper *f = NewWrapper();

    // Make a wrapper name for this function
    String *wname = Swig_name_wrapper(overloaded_name);

    /* Attach the non-standard typemaps to the parameter list. */
    Swig_typemap_attach_parms("ctype", l, f);

    /* Get return types */
    if ((tm = Swig_typemap_lookup("ctype", n, "", 0))) {
      String *ctypeout = Getattr(n, "tmap:ctype:out");	// the type in the ctype typemap's out attribute overrides the type in the typemap
      if (ctypeout)
	tm = ctypeout;
      Printf(c_return_type, "%s", tm);
    } else {
      Swig_warning(WARN_COM_TYPEMAP_CTYPE_UNDEF, input_file, line_number, "No ctype typemap defined for %s\n", SwigType_str(t, 0));
    }

    is_void_return = (Cmp(c_return_type, "void") == 0);
    if (!is_void_return)
      Wrapper_add_localv(f, "jresult", c_return_type, "jresult", NIL);

    Printv(f->def, c_return_type, " SWIGSTDCALL ", wname, "(", NIL);

    // Emit all of the local variables for holding arguments.
    emit_parameter_variables(l, f);

    /* Attach the standard typemaps */
    emit_attach_parmmaps(l, f);

    // Parameter overloading
    Setattr(n, "wrap:parms", l);
    Setattr(n, "wrap:name", wname);

    /* Get number of required and total arguments */
    num_arguments = emit_num_arguments(l);
    num_required = emit_num_required(l);
    int gencomma = 0;

    /* There are no global or static member functions in COM - thus they need fake 'this' arguments */
    // FIXME: this should include static member functions too
    if ((!is_wrapping_class() || static_flag) && !enum_constant_flag) {
      Printv(f->def, "void *SWIG_ignored", NIL);
      gencomma = 1;
    }


    // Now walk the function parameter list and generate code to get arguments
    for (i = 0, p = l; i < num_arguments; i++) {

      while (checkAttribute(p, "tmap:in:numinputs", "0")) {
	p = Getattr(p, "tmap:in:next");
      }

      SwigType *pt = Getattr(p, "type");
      String *ln = Getattr(p, "lname");
      String *c_param_type = NewString("");
      String *arg = NewString("");

      Printf(arg, "j%s", ln);

      /* Get the ctype types of the parameter */
      if ((tm = Getattr(p, "tmap:ctype"))) {
	Printv(c_param_type, tm, NIL);
      } else {
	Swig_warning(WARN_COM_TYPEMAP_CTYPE_UNDEF, input_file, line_number, "No ctype typemap defined for %s\n", SwigType_str(pt, 0));
      }

      // Add parameter to C function
      Printv(f->def, gencomma ? ", " : "", c_param_type, " ", arg, NIL);

      gencomma = 1;

      // Get typemap for this argument
      if ((tm = Getattr(p, "tmap:in"))) {
	// FIXME: canThrow(n, "in", p);
	Replaceall(tm, "$source", arg);	/* deprecated */
	Replaceall(tm, "$target", ln);	/* deprecated */
	Replaceall(tm, "$arg", arg);	/* deprecated? */
	Replaceall(tm, "$input", arg);
	Setattr(p, "emit:input", arg);
	Printf(f->code, "%s\n", tm);
	p = Getattr(p, "tmap:in:next");
      } else {
	Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, "Unable to use type %s as a function argument.\n", SwigType_str(pt, 0));
	p = nextSibling(p);
      }
      Delete(c_param_type);
      Delete(arg);
    }

    String *null_attribute = 0;
    // Now write code to make the function call
    /* FIXME: if (!native_function_flag) */ {
      if (Cmp(nodeType(n), "constant") == 0) {
        // Wrapping a constant hack
        Swig_save("functionWrapper", n, "wrap:action", NIL);

        // below based on Swig_VargetToFunction()
        SwigType *ty = Swig_wrapped_var_type(Getattr(n, "type"), use_naturalvar_mode(n));
        Setattr(n, "wrap:action", NewStringf("result = (%s) %s;", SwigType_lstr(ty, 0), Getattr(n, "value")));
      }

      // FIXME: Swig_director_emit_dynamic_cast(n, f);
      String *actioncode = emit_action(n);

      if (Cmp(nodeType(n), "constant") == 0)
        Swig_restore(n);

      /* Return value if necessary  */
      if ((tm = Swig_typemap_lookup_out("out", n, "result", f, actioncode))) {
	// FIXME: canThrow(n, "out", n);
	Replaceall(tm, "$source", "result");	/* deprecated */
	Replaceall(tm, "$target", "jresult");	/* deprecated */
	Replaceall(tm, "$result", "jresult");

        if (GetFlag(n, "feature:new"))
          Replaceall(tm, "$owner", "1");
        else
          Replaceall(tm, "$owner", "0");

        /* FIXME: see if this is needed and works as it should */
        substituteClassname(t, tm);

	Printf(f->code, "%s", tm);
	null_attribute = Getattr(n, "tmap:out:null");
	if (Len(tm))
	  Printf(f->code, "\n");
      } else {
	Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(t, 0), Getattr(n, "name"));
      }
      emit_return_variable(n, t, f);
    }

    Printf(f->def, ") {");

    if (!is_void_return)
      Printv(f->code, "    return jresult;\n", NIL);
    Printf(f->code, "}\n");

    Wrapper_print(f, f_wrappers);

    if (!(proxy_flag && is_wrapping_class() && !constructor_flag) && !enum_constant_flag) {
      moduleClassFunctionHandler(n);
      Printf(module_class_vtable_code, ",\n  (SWIG_funcptr) %s", wname);
    }

    /* 
     * Generate the proxy class properties for public member variables.
     * Not for enums and constants.
     */
    if (proxy_flag && wrapping_member_flag && !enum_constant_flag) {
      // Capitalize the first letter in the variable in the getter/setter function name
      bool getter_flag = Cmp(symname, Swig_name_set(Swig_name_member(proxy_class_name, variable_name))) != 0;

      String *getter_setter_name = NewString("");
#if 0
      if (!getter_flag)
	Printf(getter_setter_name, "set");
      else
	Printf(getter_setter_name, "get");
      Putc(toupper((int) *Char(variable_name)), getter_setter_name);
      Printf(getter_setter_name, "%s", Char(variable_name) + 1);
#endif

      Printf(getter_setter_name, "%s", variable_name);

      Setattr(n, "proxyfuncname", getter_setter_name);
      Setattr(n, "imfuncname", symname);

      proxyClassFunctionHandler(n);
      Delete(getter_setter_name);
    }

    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------
   * globalvariableHandler()
   * ------------------------------------------------------------------------ */

  virtual int globalvariableHandler(Node *n) {

    generate_property_declaration_flag = true;
    variable_name = Getattr(n, "sym:name");
    global_variable_flag = true;
    int ret = Language::globalvariableHandler(n);
    global_variable_flag = false;
    generate_property_declaration_flag = false;

    return ret;
  }

  /* ----------------------------------------------------------------------
   * memberfunctionHandler()
   * ---------------------------------------------------------------------- */

  virtual int memberfunctionHandler(Node *n) {
    Language::memberfunctionHandler(n);

    if (proxy_flag) {
      // FIXME: String *overloaded_name = getOverloadedName(n);
      String *overloaded_name = Getattr(n, "sym:name");
      String *intermediary_function_name = Swig_name_member(proxy_class_name, overloaded_name);
      Setattr(n, "proxyfuncname", Getattr(n, "sym:name"));
      Setattr(n, "imfuncname", intermediary_function_name);
      proxyClassFunctionHandler(n);
      Delete(overloaded_name);
    }
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * membervariableHandler()
   * ---------------------------------------------------------------------- */

  virtual int membervariableHandler(Node *n) {

    generate_property_declaration_flag = true;
    variable_name = Getattr(n, "sym:name");
    wrapping_member_flag = true;
    variable_wrapper_flag = true;
    Language::membervariableHandler(n);
    wrapping_member_flag = false;
    variable_wrapper_flag = false;
    generate_property_declaration_flag = false;

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * staticmembervariableHandler()
   * ---------------------------------------------------------------------- */

  virtual int staticmembervariableHandler(Node *n) {

    bool static_const_member_flag = (Getattr(n, "value") == 0);

    generate_property_declaration_flag = true;
    variable_name = Getattr(n, "sym:name");
    wrapping_member_flag = true;
    static_flag = true;
    Language::staticmembervariableHandler(n);
    wrapping_member_flag = false;
    static_flag = false;
    generate_property_declaration_flag = false;

    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * constructorHandler()
   * ----------------------------------------------------------------------------- */

  virtual int constructorHandler(Node *n) {
    static_flag = true;
    constructor_flag = true;
    Language::constructorHandler(n);
    constructor_flag = false;
    static_flag = false;

    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * moduleClassFunctionHandler()
   * ----------------------------------------------------------------------------- */

  int moduleClassFunctionHandler(Node *n) {
    SwigType *t = Getattr(n, "type");
    ParmList *l = Getattr(n, "parms");
    String *tm;
    String *return_type = NewString("");
    String *function_code = NewString("");
    Parm *p;
    int i;
    String *func_name = NULL;
    bool setter_flag = false;

    if (l) {
      if (SwigType_type(Getattr(l, "type")) == T_VOID) {
	l = nextSibling(l);
      }
    }

    /* Attach the non-standard typemaps to the parameter list */
    Swig_typemap_attach_parms("comtype", l, NULL);
    Swig_typemap_attach_parms("comin", l, NULL);

    /* Get return types */
    if (!constructor_flag && (tm = Swig_typemap_lookup("comtype", n, "", 0))) {
      String *comtypeout = Getattr(n, "tmap:comtype:out");	// the type in the comtype typemap's out attribute overrides the type in the typemap
      if (comtypeout)
	tm = comtypeout;
      substituteClassname(t, tm);
      Printf(return_type, "%s", tm);
    } else if (constructor_flag) {
      Printf(return_type, "%s *", proxy_class_name);
    } else {
      Swig_warning(WARN_COM_TYPEMAP_COMTYPE_UNDEF, input_file, line_number, "No comtype typemap defined for %s\n", SwigType_str(t, 0));
    }

    if (proxy_flag && global_variable_flag) {
      func_name = NewString("");
      setter_flag = (Cmp(Getattr(n, "sym:name"), Swig_name_set(variable_name)) == 0);
      Printf(func_name, "%s", variable_name);

      if (setter_flag) {
        Printf(function_code, "    [ propput ]\n");
      } else {
        Printf(function_code, "    [ propget ]\n");
      }
    } else {
      /* FIXME: ... */
      func_name = Getattr(n, "sym:name");
    }

    Printf(function_code, "    %s %s(", return_type, func_name);

    /* Get number of required and total arguments */
    int num_arguments = emit_num_arguments(l);
    int num_required = emit_num_required(l);

    int gencomma = 0;

    /* Output each parameter */
    for (i = 0, p = l; i < num_arguments; i++) {

      /* Ignored parameters */
      while (checkAttribute(p, "tmap:in:numinputs", "0")) {
	p = Getattr(p, "tmap:in:next");
      }

      SwigType *pt = Getattr(p, "type");
      String *param_type = NewString("");

      /* Get the COM parameter type */
      if ((tm = Getattr(p, "tmap:comtype"))) {
	substituteClassname(pt, tm);
	Printf(param_type, "%s", tm);
      } else {
	Swig_warning(WARN_COM_TYPEMAP_COMTYPE_UNDEF, input_file, line_number, "No comtype typemap defined for %s\n", SwigType_str(pt, 0));
      }

      /* FIXME: get the real argument name, it is important in the IDL */
      String *arg = NewStringf("arg%d", i);

      /* Add parameter to module class function */
      if (gencomma >= 2)
	Printf(function_code, ", ");
      gencomma = 2;
      Printf(function_code, "%s %s", param_type, arg);

      p = Getattr(p, "tmap:in:next");
      Delete(arg);
      Delete(param_type);
    }

    Printf(function_code, ");\n");

    Printv(module_class_code, function_code, NIL);
    Delete(function_code);
    Delete(return_type);

    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * emitProxyClassDefAndCPPCasts()
   * ----------------------------------------------------------------------------- */

  void emitProxyClassDefAndCPPCasts(Node *n) {
    String *c_classname = SwigType_namestr(Getattr(n, "name"));
    String *c_baseclass = NULL;
    String *baseclass = NULL;
    String *c_baseclassname = NULL;
    String *typemap_lookup_type = Getattr(n, "classtypeobj");
    bool feature_director = Swig_directorclass(n) ? true : false;

    Node *attributes = NewHash();

    const String *pure_baseclass = NewString("");

    // C++ inheritance
    // FIXME: if (!purebase_replace) {
      List *baselist = Getattr(n, "bases");
      if (baselist) {
        Iterator base = First(baselist);
        while (base.item && GetFlag(base.item, "feature:ignore")) {
          base = Next(base);
        }
        if (base.item) {
          c_baseclassname = Getattr(base.item, "name");
          baseclass = Copy(getProxyName(c_baseclassname));
          if (baseclass)
            c_baseclass = SwigType_namestr(Getattr(base.item, "name"));
          base = Next(base);
          /* Warn about multiple inheritance for additional base class(es) */
          while (base.item) {
            if (GetFlag(base.item, "feature:ignore")) {
              base = Next(base);
              continue;
            }
            String *proxyclassname = SwigType_str(Getattr(n, "classtypeobj"), 0);
            String *baseclassname = SwigType_str(Getattr(base.item, "name"), 0);
            Swig_warning(WARN_COM_MULTIPLE_INHERITANCE, input_file, line_number,
                         "Warning for %s proxy: Base %s ignored. Multiple inheritance is not supported in C#.\n", proxyclassname, baseclassname);
            base = Next(base);
          }
        }
      }
    // FIXME: }

    const String *wanted_base = baseclass ? baseclass : pure_baseclass;
    bool derived = baseclass && getProxyName(c_baseclassname);

    if (!Getattr(n, "abstract")) {
      Printv(proxy_class_def, "  [\n    uuid(", NIL);
      formatGUID(proxy_class_def, proxy_clsid, false);
      Printv(proxy_class_def, ")\n  ]\n  coclass $comclassnameImpl {\n"
          "    interface $comclassname;\n  };\n\n", NIL);
    }

    Printv(proxy_class_forward_def, "  interface $comclassname;\n", NIL);
    Printv(proxy_class_def, "  [\n    object,\n    local,\n    uuid(", NIL);
    formatGUID(proxy_class_def, proxy_iid, false);
/*
    Printv(proxy_class_def, ")\n  ]\n  interface $comclassname",
        *Char(wanted_base) ? " : " : "",
        *Char(wanted_base) ? wanted_base : "", " {", NIL);
 */
    Printv(proxy_class_def, "),\n    dual\n  ]\n  interface $comclassname : ",
        *Char(wanted_base) ? wanted_base : "IDispatch", " {", NIL);

    Delete(attributes);

    // FIXME: temporary
    Printv(proxy_class_def, typemapLookup("combody", typemap_lookup_type, WARN_NONE),
	   NIL);

    // Emit extra user code
    Printv(proxy_class_def, typemapLookup("comcode", typemap_lookup_type, WARN_NONE),
	   NIL);

    // Substitute various strings into the above template
    Replaceall(proxy_class_code, "$comclassname", proxy_class_name);
    Replaceall(proxy_class_def, "$comclassname", proxy_class_name);
    Replaceall(proxy_class_forward_def, "$comclassname", proxy_class_name);

    Replaceall(proxy_class_def, "$module", module_class_name);
    Replaceall(proxy_class_code, "$module", module_class_name);

    Delete(baseclass);
  }

  /* ----------------------------------------------------------------------
   * generateGUID()
   * ---------------------------------------------------------------------- */

  void generateGUID(GUID *result, String *name) {
    int name_len = Len(name);
    char *name_chars = Char(name);

    char *prep_input = new char[16 + name_len];

    /* guid_seed serves as a "name space ID" as used in RFC 4122. */
    prep_input[0] = (guid_seed.Data1 >> 24) & 0xff;
    prep_input[1] = (guid_seed.Data1 >> 16) & 0xff;
    prep_input[2] = (guid_seed.Data1 >> 8) & 0xff;
    prep_input[3] = guid_seed.Data1 & 0xff;
    prep_input[4] = (guid_seed.Data2 >> 8) & 0xff;
    prep_input[5] = guid_seed.Data2 & 0xff;
    prep_input[6] = (guid_seed.Data3 >> 8) & 0xff;
    prep_input[7] = guid_seed.Data3 & 0xff;

    for (int i = 0; i < 8; ++i) {
      prep_input[8 + i] = guid_seed.Data4[i];
    }

    for (int i = 0; i < name_len; ++i) {
      prep_input[16 + i] = name_chars[i];
    }

    SHA1_hash hash = SHA1(prep_input, 16 + name_len);

    GUID res = {
      /* time_low */ ((unsigned int) hash.octets[0] << 24) |
          ((unsigned int) hash.octets[1] << 16) |
          ((unsigned int) hash.octets[2] << 8) |
          (unsigned int) hash.octets[3],
      /* time_mid */ ((unsigned int) hash.octets[4] << 8) |
          (unsigned int) hash.octets[5],
      /* time_hi_and_version */ ((unsigned int) hash.octets[6] << 8) |
          (unsigned int) hash.octets[7],
      hash.octets[8],
      hash.octets[9],
      hash.octets[10],
      hash.octets[11],
      hash.octets[12],
      hash.octets[13],
      hash.octets[14],
      hash.octets[15],
    };

    /* Set version to 5 */
    res.Data3 &= 0x0fff;
    res.Data3 |= 0x5000;

    /* Clear top 2 bits of Data4[0] */
    res.Data4[0] &= 0x3f;
    res.Data4[0] |= 0x80;

    *result = res;

    delete prep_input;
  }

  /* ----------------------------------------------------------------------
   * formatGUID()
   * ---------------------------------------------------------------------- */

  void formatGUID(File *output, GUID *input, bool cFormat) {
    if (cFormat) {
      Printf(output,
          "0x%08x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
          input->Data1, input->Data2, input->Data3, input->Data4[0], input->Data4[1],
          input->Data4[2], input->Data4[3], input->Data4[4], input->Data4[5], input->Data4[6], input->Data4[7]);
    } else {
      Printf(output,
          "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
          input->Data1, input->Data2, input->Data3, (((int) input->Data4[0]) << 8) | input->Data4[1],
          input->Data4[2], input->Data4[3], input->Data4[4], input->Data4[5], input->Data4[6], input->Data4[7]);
    }
  }

  /* ----------------------------------------------------------------------
   * parseHex()
   * ---------------------------------------------------------------------- */

  unsigned int parseHex(char *text, int start, int end) {
    unsigned int result = 0;

    for (int i = start; i < end; ++i) {
      int digit = 0;

      if (text[i] >= '0' && text[i] <= '9')
        digit = text[i] - '0';
      else if (text[i] >= 'a' && text[i] <= 'f')
        digit = text[i] - 'a' + 10;
      else
        digit = text[i] - 'A' + 10;

      result = 16 * result + digit;
    }

    return result;
  }

  /* ----------------------------------------------------------------------
   * parseGUID()
   * ---------------------------------------------------------------------- */

  bool parseGUID(String *input, GUID *output) {
    if (!input || Len(input) != 36)
      return false;

    char *input_chars = Char(input);

    /* Check GUID format */
    for (int i = 0; i < 36; ++i) {
      if (i == 8 || i == 13 || i == 18 || i == 23) {
        if (input_chars[i] != '-')
          return false;
      } else {
        if (input_chars[i] >= '0' && input_chars[i] <= '9')
          continue;
        if (input_chars[i] >= 'a' && input_chars[i] <= 'f')
          continue;
        if (input_chars[i] >= 'A' && input_chars[i] <= 'F')
          continue;

        return false;
      }
    }

    output->Data1 = parseHex(input_chars, 0, 8);
    output->Data2 = parseHex(input_chars, 9, 13);
    output->Data3 = parseHex(input_chars, 14, 18);
    output->Data4[0] = parseHex(input_chars, 19, 21);
    output->Data4[1] = parseHex(input_chars, 21, 23);

    for (int i = 0; i < 6; ++i) {
      output->Data4[2 + i] = parseHex(input_chars, 24 + 2 * i, 26 + 2 * i);
    }

    return true;
  }

  /* ----------------------------------------------------------------------
   * classHandler()
   * ---------------------------------------------------------------------- */

  virtual int classHandler(Node *n) {
    if (proxy_flag) {
      proxy_class_name = NewString(Getattr(n, "sym:name"));
      List *bases = NULL;

      if (!addSymbol(proxy_class_name, n))
	return SWIG_ERROR;

/* FIXME */
#if 0
      if (Cmp(proxy_class_name, imclass_name) == 0) {
	Printf(stderr, "Class name cannot be equal to intermediary class name: %s\n", proxy_class_name);
	SWIG_exit(EXIT_FAILURE);
      }
#endif

      if (Cmp(proxy_class_name, module_class_name) == 0) {
	Printf(stderr, "Class name cannot be equal to module class name: %s\n", proxy_class_name);
	SWIG_exit(EXIT_FAILURE);
      }

      Clear(proxy_class_def);
      Clear(proxy_class_code);
      Clear(proxy_class_forward_def);
      Clear(proxy_class_vtable_code);
      Clear(proxy_class_vtable_defs);

      proxy_iid = new GUID;
      if (Getattr(n, "feature:iid")) {
        parseGUID(Getattr(n, "feature:iid"), proxy_iid);
      } else {
        String *proxy_iid_ident = NewStringf("%s.IID", proxy_class_name);
        generateGUID(proxy_iid, proxy_iid_ident);
        Delete(proxy_iid_ident);
      }
      Setattr(n, "wrap:iid", proxy_iid);

      Printf(proxy_class_vtable_code, "DEFINE_GUID(IID_%s, ", proxy_class_name);
      formatGUID(proxy_class_vtable_code, proxy_iid, true);
      Printf(proxy_class_vtable_code, ");\n\n");

      if (!Getattr(n, "abstract")) {
        /* Generate class object */
        proxy_clsid = new GUID;
        if (Getattr(n, "feature:clsid")) {
          parseGUID(Getattr(n, "feature:clsid"), proxy_clsid);
        } else {
          String *proxy_clsid_ident = NewStringf("%s.CLSID", proxy_class_name);
          generateGUID(proxy_clsid, proxy_clsid_ident);
          Delete(proxy_clsid_ident);
        }

        Printf(proxy_class_vtable_code, "DEFINE_GUID(CLSID_%s, ", proxy_class_name);
        formatGUID(proxy_class_vtable_code, proxy_clsid, true);
        Printf(proxy_class_vtable_code, ");\n\n");

        Printf(clsid_list, "  { (SWIG_funcptr) _wrap_new_%s, CLSID_%s, _T(\"{", proxy_class_name, proxy_class_name);
        formatGUID(clsid_list, proxy_clsid, false);
        Printf(clsid_list,  "}\") },\n");
      }

      Printf(proxy_class_vtable_code, "HRESULT SWIGSTDCALL _wrap%sQueryInterface1(void *that, REFIID iid, "
          "void ** ppvObject) {\n", proxy_class_name);

      /* Look if the requested interface is ISWIGWrappedObject */
      Printf(proxy_class_vtable_code,
          "  if (IsEqualIID(iid, IID_ISWIGWrappedObject)) {\n"
          "    /* FIXME: This could be more elegant */\n"
          "    SWIGAddRef1(that);\n"
          /* Address of current object, incremented by the size of a pointer */
          "    *ppvObject = (void *) ((void **)that + 1);\n"
          "    return S_OK;\n"
          "  }\n\n");

      Printf(proxy_class_vtable_code, "  if (IsEqualIID(iid, IID_IUnknown) ||\n"
        "      IsEqualIID(iid, IID_IDispatch) ||\n"
        "      IsEqualIID(iid, IID_%s)", proxy_class_name);

      bases = Getattr(n, "bases");

      /* Iterate through the ancestors */
      while (bases) {
        Iterator base = First(bases);

        Printf(proxy_class_vtable_code, " ||\n      IsEqualIID(iid, IID_%s)", Getattr(base.item, "sym:name"));

        /* Get next base */
        bases = Getattr(base.item, "bases");
      }

      Printf(proxy_class_vtable_code, ") {\n"
          "    /* FIXME: This could be more elegant */\n"
          "    SWIGAddRef1(that);\n"
          "    *ppvObject = that;\n"
          "    return S_OK;\n"
          "  }\n\n");

      Printf(proxy_class_vtable_code, "  return E_NOINTERFACE;\n}\n\n");

      bases = NULL;

      Printf(proxy_class_vtable_code, "HRESULT SWIGSTDCALL _wrap%sQueryInterface2(void *that, REFIID iid, "
          "void ** ppvObject) {\n", proxy_class_name);

      Printf(proxy_class_vtable_code,
          "  return _wrap%sQueryInterface1((void *) ((void **) that - 1), iid, ppvObject);\n", proxy_class_name);

      Printf(proxy_class_vtable_code, "}\n\n");

      Printf(proxy_class_vtable_code, "SWIG_funcptr _wrap%sSWIGWrappedObject_vtable[] = "
          "{\n  (SWIG_funcptr) _wrap%sQueryInterface2,"
          "\n  (SWIG_funcptr) SWIGAddRef2,"
          "\n  (SWIG_funcptr) SWIGRelease2,"
          "\n  (SWIG_funcptr) SWIGGetCPtr"
          "\n};\n\n",
          proxy_class_name, proxy_class_name);

      Printf(proxy_class_vtable_code, "SWIG_funcptr _wrap%svtable[] = "
          "{\n  (SWIG_funcptr) _wrap%sQueryInterface1,"
          "\n  (SWIG_funcptr) SWIGAddRef1,"
          "\n  (SWIG_funcptr) SWIGRelease1,"
          "\n  (SWIG_funcptr) SWIGGetTypeInfoCount,"
          "\n  (SWIG_funcptr) SWIGGetTypeInfo,"
          "\n  (SWIG_funcptr) SWIGGetIDsOfNames,"
          "\n  (SWIG_funcptr) SWIGInvoke",
          proxy_class_name, proxy_class_name);

      bases = Getattr(n, "bases");

      if (!bases) {
        proxy_class_member_functions = NewList();
        Setattr(n, "wrap:member_functions", proxy_class_member_functions);
      } else {
        Iterator base = First(bases);

        List *base_member_functions = Getattr(base.item, "wrap:member_functions");
        Setattr(n, "wrap:member_functions", Copy(base_member_functions));

        for (Iterator func = First(base_member_functions); func.item; func = Next(func)) {
          Printf(proxy_class_vtable_code, ",\n  (SWIG_funcptr)%s", func.item);
        }
      }

      // FIXME: destructor_call = NewString("");
      proxy_class_constants_code = NewString("");
    }

    Language::classHandler(n);

    if (proxy_flag) {

      emitProxyClassDefAndCPPCasts(n);

      Replaceall(proxy_class_def, "$module", module_class_name);
      Replaceall(proxy_class_code, "$module", module_class_name);
      Replaceall(proxy_class_constants_code, "$module", module_class_name);
      // FIXME: Replaceall(proxy_class_def, "$imclassname", imclass_name);
      // FIXME: Replaceall(proxy_class_code, "$imclassname", imclass_name);
      // FIXME: Replaceall(proxy_class_constants_code, "$imclassname", imclass_name);
      // FIXME: Replaceall(proxy_class_def, "$dllimport", dllimport);
      // FIXME: Replaceall(proxy_class_code, "$dllimport", dllimport);
      // FIXME: Replaceall(proxy_class_constants_code, "$dllimport", dllimport);

      Printv(f_proxy_forward_defs, proxy_class_forward_def, NIL);
      Printv(f_proxy, proxy_class_def, proxy_class_code, NIL);

      // Write out all the constants
      if (Len(proxy_class_constants_code) != 0)
	Printv(f_proxy, proxy_class_constants_code, NIL);

      Printf(f_proxy, "  };\n\n");

      Printv(proxy_class_vtable_code, "\n};\n\n", NIL);

      Printf(proxy_class_vtable_code, "void * SWIGSTDCALL SWIG_wrap%s(void *arg, int cMemOwn) {\n"
          "#ifdef __cplusplus\n"
          "  SWIGWrappedObject *res = new SWIGWrappedObject;\n"
          "#else\n"
          "  SWIGWrappedObject *res = (SWIGWrappedObject *) malloc(sizeof(SWIGWrappedObject));\n"
          "#endif\n"
          "  res->vtable = _wrap%svtable;\n"
          "  res->SWIGWrappedObject_vtable = _wrap%sSWIGWrappedObject_vtable;\n"
          "  res->cPtr = arg;\n"
          "  res->cMemOwn = cMemOwn;\n"
          "  res->refCount = 0;\n"
          "  res->deleteInstance = _wrap_delete_%s;\n"
          "  /* GetTypeInfoOfGuid */\n"
          "  ((HRESULT (SWIGSTDCALL *)(ITypeLib *, REFGUID, ITypeInfo **)) (((SWIGIUnknown *) SWIG_typelib)->vtable[6]))(SWIG_typelib, IID_%s, &res->typeInfo);\n"
          "  return (void *) res;\n"
          "}\n\n",
          proxy_class_name, proxy_class_name, proxy_class_name, proxy_class_name, proxy_class_name);

      Printf(proxy_class_vtable_defs, "void * SWIGSTDCALL SWIG_wrap%s(void *arg, int cMemOwn);\n", proxy_class_name);

      Printv(f_vtables, proxy_class_vtable_code, NIL);
      Printv(f_vtable_defs, proxy_class_vtable_defs, NIL);

      Delete(proxy_class_name);
      proxy_class_name = NULL;
      Delete(proxy_class_constants_code);
      proxy_class_constants_code = NULL;
      delete proxy_iid;
    }

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * staticmemberfunctionHandler()
   * ---------------------------------------------------------------------- */

  virtual int staticmemberfunctionHandler(Node *n) {

    static_flag = true;
    member_func_flag = true;
    Language::staticmemberfunctionHandler(n);

    if (proxy_flag) {
      // FIXME: String *overloaded_name = getOverloadedName(n);
      String *overloaded_name = Getattr(n, "sym:name");
      String *intermediary_function_name = Swig_name_member(proxy_class_name, overloaded_name);
      Setattr(n, "proxyfuncname", Getattr(n, "sym:name"));
      Setattr(n, "imfuncname", intermediary_function_name);
      proxyClassFunctionHandler(n);
      Delete(overloaded_name);
    }
    static_flag = false;
    member_func_flag = false;

    return SWIG_OK;
  }

  /* -----------------------------------------------------------------------------
   * proxyClassFunctionHandler()
   *
   * Function called for creating a COM wrapper function around a c++ function in the 
   * proxy class. Used for both static and non-static C++ class functions.
   * C++ class static functions map to member functions of the objects and/or to
   * member functions of the module class (TODO).
   * ----------------------------------------------------------------------------- */

  void proxyClassFunctionHandler(Node *n) {
    SwigType *t = Getattr(n, "type");
    ParmList *l = Getattr(n, "parms");
    String *intermediary_function_name = Getattr(n, "imfuncname");
    String *proxy_function_name = Getattr(n, "proxyfuncname");
    String *tm;
    Parm *p;
    int i;
    String *return_type = NewString("");
    String *function_code = NewString("");
    bool setter_flag = false;
    String *pre_code = NewString("");
    String *post_code = NewString("");

    if (!proxy_flag)
      return;

    // Wrappers not wanted for some methods where the parameters cannot be overloaded in COM
    if (Getattr(n, "overload:ignore"))
      return;

    // Don't generate proxy method for additional explicitcall method used in directors
    if (GetFlag(n, "explicitcall"))
      return;

    if (!Getattr(n, "override")) {
      String *wname = Getattr(n, "wrap:name");
      Printf(proxy_class_vtable_code, ",\n  (SWIG_funcptr)%s", wname);
      Append(proxy_class_member_functions, wname);
    }

    if (l) {
      if (SwigType_type(Getattr(l, "type")) == T_VOID) {
	l = nextSibling(l);
      }
    }

    /* Attach the non-standard typemaps to the parameter list */
    Swig_typemap_attach_parms("in", l, NULL);
    Swig_typemap_attach_parms("comtype", l, NULL);
    // FIXME: Swig_typemap_attach_parms("javain", l, NULL);

    /* Get return types */
    if ((tm = Swig_typemap_lookup("comtype", n, "", 0))) {
      // Note that in the case of polymorphic (covariant) return types, the method's return type is changed to be the base of the C++ return type
      SwigType *covariant = Getattr(n, "covariant");
      substituteClassname(covariant ? covariant : t, tm);
      Printf(return_type, "%s", tm);
      if (covariant)
	Swig_warning(WARN_COM_COVARIANT_RET, input_file, line_number,
		     "Covariant return types not supported in COM. Proxy method will return %s.\n", SwigType_str(covariant, 0));
    } else {
      Swig_warning(WARN_COM_TYPEMAP_COMTYPE_UNDEF, input_file, line_number, "No comstype typemap defined for %s\n", SwigType_str(t, 0));
    }

    if (wrapping_member_flag && !enum_constant_flag) {
      // Properties
      setter_flag = (Cmp(Getattr(n, "sym:name"), Swig_name_set(Swig_name_member(proxy_class_name, variable_name))) == 0);
      
      if (setter_flag) {
        Printf(function_code, "    [ propput ]\n");
      } else {
        Printf(function_code, "    [ propget ]\n");
      }
    }


    /* Start generating the proxy function */
    Printf(function_code, "    %s %s(", return_type, proxy_function_name);

    emit_mark_varargs(l);

    // FIXME: int gencomma = !static_flag;
    int gencomma = 1;

    /* Output each parameter */
    for (i = 0, p = l; p; i++) {

      /* Ignored varargs */
      if (checkAttribute(p, "varargs:ignore", "1")) {
	p = nextSibling(p);
	continue;
      }

      /* Ignored parameters */
      if (checkAttribute(p, "tmap:in:numinputs", "0")) {
	p = Getattr(p, "tmap:in:next");
	continue;
      }

      /* Ignore the 'this' argument for variable wrappers */
      if (!(variable_wrapper_flag && i == 0) || static_flag) {
	SwigType *pt = Getattr(p, "type");
	String *param_type = NewString("");

	/* Get the COM parameter type */
	if ((tm = Getattr(p, "tmap:comtype"))) {
	  substituteClassname(pt, tm);
	  Printf(param_type, "%s", tm);
	} else {
	  Swig_warning(WARN_COM_TYPEMAP_COMTYPE_UNDEF, input_file, line_number, "No comtype typemap defined for %s\n", SwigType_str(pt, 0));
	}

	// FIXME: String *arg = makeParameterName(n, p, i, setter_flag);
        String *arg = NewStringf("arg%d", i);

	/* Add parameter to proxy function */
	if (gencomma >= 2)
	  Printf(function_code, ", ");
	gencomma = 2;
	Printf(function_code, "%s %s", param_type, arg);

	Delete(arg);
	Delete(param_type);
      }
      p = Getattr(p, "tmap:in:next");
    }

    Printf(function_code, ")");

    // FIXME: generateThrowsClause(n, function_code);
    Printv(function_code, ";\n", NIL);

    if (!Getattr(n, "override")) {
      Printv(proxy_class_code, function_code, NIL);
    }

    Delete(pre_code);
    Delete(post_code);
    Delete(function_code);
    Delete(return_type);
  }

  /* -----------------------------------------------------------------------------
   * emitTypeWrapperClass()
   * ----------------------------------------------------------------------------- */

  void emitTypeWrapperClass(String *classname, SwigType *type) {
    Clear(proxy_class_def);
    Clear(proxy_class_forward_def);

    proxy_iid = new GUID;

#if 0
// FIXME: Maybe we should allow specifying IIDs for opaque classes?
    if (Getattr(n, "feature:iid")) {
      parseGUID(Getattr(n, "feature:iid"), proxy_iid);
    } else {
      String *proxy_iid_ident = NewStringf("%s.IID", classname);
      generateGUID(proxy_iid, proxy_iid_ident);
      Delete(proxy_iid_ident);
    }
#endif
    {
      String *proxy_iid_ident = NewStringf("%s.IID", classname);
      generateGUID(proxy_iid, proxy_iid_ident);
      Delete(proxy_iid_ident);
    }

    Printv(proxy_class_forward_def, "  interface $comclassname;\n", NIL);

    Printv(proxy_class_def, "  [\n    object,\n    local,\n    uuid(", NIL);
    formatGUID(proxy_class_def, proxy_iid, false);
    Printv(proxy_class_def, ")\n  ]\n  interface $comclassname {\n",
           typemapLookup("combody", type, WARN_COM_TYPEMAP_COMBODY_UNDEF),
	   typemapLookup("comcode", type, WARN_NONE),
           "  };\n\n", NIL);

    Replaceall(proxy_class_forward_def, "$comclassname", classname);
    Replaceall(proxy_class_def, "$comclassname", classname);
    Replaceall(proxy_class_forward_def, "$module", module_class_name);
    Replaceall(proxy_class_def, "$module", module_class_name);

    Printf(f_vtable_defs, "void * (SWIGSTDCALL * SWIG_wrap%s)(void *, int) = SWIG_wrap_opaque;\n", classname);

    Printv(f_proxy_forward_defs, proxy_class_forward_def, NIL);
    Printv(f_proxy, proxy_class_def, NIL);

    delete proxy_iid;
  }

  /* -----------------------------------------------------------------------------
   * typemapLookup()
   * ----------------------------------------------------------------------------- */

  const String *typemapLookup(const String *op, String *type, int warning, Node *typemap_attributes = NULL) {
    String *tm = NULL;
    const String *code = NULL;

    if ((tm = Swig_typemap_search(op, type, NULL, NULL))) {
      code = Getattr(tm, "code");
      if (typemap_attributes)
	Swig_typemap_attach_kwargs(tm, op, typemap_attributes);
    }

    if (!code) {
      code = empty_string;
      if (warning != WARN_NONE)
	Swig_warning(warning, input_file, line_number, "No %s typemap defined for %s\n", op, type);
    }

    return code ? code : empty_string;
  }

  /* -----------------------------------------------------------------------------
   * substituteClassname()
   *
   * Substitute $comclassname with the proxy class name for classes/structs/unions that SWIG knows about.
   * Also substitutes enums with enum name.
   * Otherwise use the $descriptor name for the COM class name. Note that the $&comclassname substitution
   * is the same as a $&descriptor substitution, ie one pointer added to descriptor name.
   * Inputs:
   *   pt - parameter type
   *   tm - comtype typemap
   * Outputs:
   *   tm - comtype typemap with $comclassname substitution
   * Return:
   *   substitution_performed - flag indicating if a substitution was performed
   * ----------------------------------------------------------------------------- */

  bool substituteClassname(SwigType *pt, String *tm) {
    bool substitution_performed = false;
    SwigType *type = Copy(SwigType_typedef_resolve_all(pt));
    SwigType *strippedtype = SwigType_strip_qualifiers(type);

    if (Strstr(tm, "$comclassname")) {
      SwigType *classnametype = Copy(strippedtype);
      substituteClassnameSpecialVariable(classnametype, tm, "$comclassname");
      substitution_performed = true;
      Delete(classnametype);
    }
    if (Strstr(tm, "$*comclassname")) {
      SwigType *classnametype = Copy(strippedtype);
      Delete(SwigType_pop(classnametype));
      substituteClassnameSpecialVariable(classnametype, tm, "$*comclassname");
      substitution_performed = true;
      Delete(classnametype);
    }
    if (Strstr(tm, "$&comclassname")) {
      SwigType *classnametype = Copy(strippedtype);
      SwigType_add_pointer(classnametype);
      substituteClassnameSpecialVariable(classnametype, tm, "$&comclassname");
      substitution_performed = true;
      Delete(classnametype);
    }

    Delete(strippedtype);
    Delete(type);

    return substitution_performed;
  }

  /* -----------------------------------------------------------------------------
   * substituteClassnameSpecialVariable()
   * ----------------------------------------------------------------------------- */

  void substituteClassnameSpecialVariable(SwigType *classnametype, String *tm, const char *classnamespecialvariable) {
    if (SwigType_isenum(classnametype)) {
      // FIXME: String *enumname = getEnumName(classnametype);
      String *enumname = classnametype;
      if (enumname)
	Replaceall(tm, classnamespecialvariable, enumname);
      else
	Replaceall(tm, classnamespecialvariable, NewStringf("int"));
    } else {
      String *classname = getProxyName(classnametype);
      if (classname) {
	Replaceall(tm, classnamespecialvariable, classname);	// getProxyName() works for pointers to classes too
      } else {			// use $descriptor if SWIG does not know anything about this type. Note that any typedefs are resolved.
	String *descriptor = NewStringf("SWIGTYPE%s", SwigType_manglestr(classnametype));
	Replaceall(tm, classnamespecialvariable, descriptor);

	// Add to hash table so that the type wrapper classes can be created later
	Setattr(swig_types_hash, descriptor, classnametype);
	Delete(descriptor);
      }
    }
  }


};				/* class COM */

/* -----------------------------------------------------------------------------
 * swig_com()    - Instantiate module
 * ----------------------------------------------------------------------------- */

static Language *new_swig_com() {
  return new COM();
}
extern "C" Language *swig_com(void) {
  return new_swig_com();
}

/* -----------------------------------------------------------------------------
 * Static member variables
 * ----------------------------------------------------------------------------- */

const char *COM::usage = (char *) "\
COM Options (available with -com)\n\
     -norcfile       - Do not generate RC (resource definition) file\n\
     -nodeffile      - Do not generate DEF file\n\
     -nodllexports   - Do not generate DllGetClassObject and DllCanUnloadNow\n\
                       (implicates -nodeffile)\n\
\n";
