%include <shared_ptr.i>

%define SWIG_SHARED_PTR_TYPEMAPS(CONST, TYPE...)

%naturalvar TYPE;
%naturalvar SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >;

// destructor mods
%feature("unref") TYPE
//"if (debug_shared) { cout << \"deleting use_count: \" << (*smartarg1).use_count() << \" [\" << (boost::get_deleter<SWIG_null_deleter>(*smartarg1) ? std::string(\"CANNOT BE DETERMINED SAFELY\") : ((*smartarg1).get() ? (*smartarg1)->getValue() : std::string(\"NULL PTR\"))) << \"]\" << endl << flush; }\n"
                               "(void)arg1; delete smartarg1;"


// plain value
%typemap(in, canthrow=1) CONST TYPE ($&1_type argp = 0) %{
  argp = ((SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input) ? ((SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input)->get() : 0;
  if (!argp) {
    SWIG_DSetPendingException(SWIG_DIllegalArgumentException, "Attempt to dereference null $1_type");
    return $null;
  }
  $1 = *argp; %}
%typemap(out) CONST TYPE
%{ $result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(new $1_ltype(($1_ltype &)$1)); %}

// plain pointer
%typemap(in, canthrow=1) CONST TYPE * (SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *smartarg = 0) %{
  smartarg = (SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input;
  $1 = (TYPE *)(smartarg ? smartarg->get() : 0); %}
%typemap(out, fragment="SWIG_null_deleter") CONST TYPE * %{
  $result = $1 ? new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1 SWIG_NO_NULL_DELETER_$owner) : 0;
%}

// plain reference
%typemap(in, canthrow=1) CONST TYPE & %{
  $1 = ($1_ltype)(((SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input) ? ((SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input)->get() : 0);
  if (!$1) {
    SWIG_DSetPendingException(SWIG_DIllegalArgumentException, "$1_type reference is null");
    return $null;
  } %}
%typemap(out, fragment="SWIG_null_deleter") CONST TYPE &
%{ $result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1 SWIG_NO_NULL_DELETER_$owner); %}

// plain pointer by reference
%typemap(in) TYPE *CONST& ($*1_ltype temp = 0)
%{ temp = (TYPE *)(((SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input) ? ((SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input)->get() : 0);
   $1 = &temp; %}
%typemap(out, fragment="SWIG_null_deleter") TYPE *CONST&
%{ $result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(*$1 SWIG_NO_NULL_DELETER_$owner); %}

// shared_ptr by value
%typemap(in) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >
%{ if ($input) $1 = *($&1_ltype)$input; %}
%typemap(out) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >
%{ $result = $1 ? new $1_ltype($1) : 0; %}

// shared_ptr by reference
%typemap(in, canthrow=1) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > & ($*1_ltype tempnull)
%{ $1 = $input ? ($1_ltype)$input : &tempnull; %}
%typemap(out) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > &
%{ $result = *$1 ? new $*1_ltype(*$1) : 0; %}

// shared_ptr by pointer
%typemap(in) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * ($*1_ltype tempnull)
%{ $1 = $input ? ($1_ltype)$input : &tempnull; %}
%typemap(out, fragment="SWIG_null_deleter") SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *
%{ $result = ($1 && *$1) ? new $*1_ltype(*($1_ltype)$1) : 0;
   if ($owner) delete $1; %}

// shared_ptr by pointer reference
%typemap(in) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *& (SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > tempnull, $*1_ltype temp = 0)
%{ temp = $input ? *($1_ltype)&$input : &tempnull;
   $1 = &temp; %}
%typemap(out) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *&
%{ *($1_ltype)&$result = (*$1 && **$1) ? new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(**$1) : 0; %}

// various missing typemaps - If ever used (unlikely) ensure compilation error rather than runtime bug
%typemap(in) CONST TYPE[], CONST TYPE[ANY], CONST TYPE (CLASS::*) %{
#error "typemaps for $1_type not available"
%}
%typemap(out) CONST TYPE[], CONST TYPE[ANY], CONST TYPE (CLASS::*) %{
#error "typemaps for $1_type not available"
%}


%typemap (ctype)  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > &,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *& "void *"
%typemap (imtype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > &,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *& "void*"
%typemap (dtype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > &,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *& "$typemap(dtype, TYPE)"

%typemap(din) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >,
               SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > &,
               SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *,
               SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *& "$typemap(dtype, TYPE).swigGetCPtr($dinput)"

%typemap(dout, excode=SWIGEXCODE) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > {
  void* cPtr = $imcall;
  auto ret = (cPtr is null) ? null : new $typemap(dtype, TYPE)(cPtr, true);$excode
  return ret;
}
%typemap(dout, excode=SWIGEXCODE) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > & {
  void* cPtr = $imcall;
  auto ret = (cPtr is null) ? null : new $typemap(dtype, TYPE)(cPtr, true);$excode
  return ret;
}
%typemap(dout, excode=SWIGEXCODE) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * {
  void* cPtr = $imcall;
  auto ret = (cPtr is null) ? null : new $typemap(dtype, TYPE)(cPtr, true);$excode
  return ret;
}
%typemap(dout, excode=SWIGEXCODE) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *& {
  void* cPtr = $imcall;
  auto ret = (cPtr is null) ? null : new $typemap(dtype, TYPE)(cPtr, true);$excode
  return ret;
}


%typemap(dout, excode=SWIGEXCODE) CONST TYPE {
  auto ret = new $typemap(dtype, TYPE)($imcall, true);$excode
  return ret;
}
%typemap(dout, excode=SWIGEXCODE) CONST TYPE & {
  auto ret = new $typemap(dtype, TYPE)($imcall, true);$excode
  return ret;
}
%typemap(dout, excode=SWIGEXCODE) CONST TYPE * {
  void* cPtr = $imcall;
  auto ret = (cPtr is null) ? null : new $typemap(dtype, TYPE)(cPtr, true);$excode
  return ret;
}
%typemap(dout, excode=SWIGEXCODE) TYPE *CONST& {
  void* cPtr = $imcall;
  auto ret = (cPtr is null) ? null : new $typemap(dtype, TYPE)(cPtr, true);$excode
  return ret;
}

// For shared pointers, both the derived and the base class have to »own« their
// pointer; otherwise the reference count is not decreased properly on destruction.
%typemap(dbody) SWIGTYPE %{
private void* swigCPtr;
private bool swigCMemOwn;

public this(void* cObject, bool ownCObject) {
  swigCPtr = cObject;
  swigCMemOwn = ownCObject;
}

public static void* swigGetCPtr($dclassname obj) {
  return (obj is null) ? null : obj.swigCPtr;
}
%}

%typemap(dbody_derived) SWIGTYPE %{
private void* swigCPtr;
private bool swigCMemOwn;

public this(void* cObject, bool ownCObject) {
  super($imdmodule.$dclassnameSmartPtrUpcast(cObject), ownCObject);
  swigCPtr = cObject;
  swigCMemOwn = ownCObject;
}

public static void* swigGetCPtr($dclassname obj) {
  return (obj is null) ? null : obj.swigCPtr;
}
%}

%typemap(ddispose, methodname="dispose", methodmodifiers="public") TYPE {
  synchronized(this) {
    if (swigCPtr !is null) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        $imcall;
      }
      swigCPtr = null;
    }
  }
}

%typemap(ddispose_derived, methodname="dispose", methodmodifiers="public") TYPE {
  synchronized(this) {
    if (swigCPtr !is null) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        $imcall;
      }
      swigCPtr = null;
      super.dispose();
    }
  }
}

%template() SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >;
%enddef
