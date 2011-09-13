%include <intrusive_ptr.i>

// Language specific macro implementing all the customisations for handling the smart pointer
%define SWIG_INTRUSIVE_PTR_TYPEMAPS(CONST, TYPE...)

// %naturalvar is as documented for member variables
%naturalvar TYPE;
%naturalvar SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >;

// destructor wrapper customisation
%feature("unref") TYPE "(void)arg1; delete smartarg1;"

// Typemap customisations...

%typemap(in, canthrow=1) CONST TYPE ($&1_type argp = 0, SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *smartarg = 0) %{
  // plain value
  argp = (*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input) ? (*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input)->get() : 0;
  if (!argp) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "Attempt to dereference null $1_type", 0);
    return $null;
  }
  $1 = *argp; 
%}
%typemap(out, fragment="SWIG_intrusive_deleter") CONST TYPE %{ 
  //plain value(out)
  $1_ltype* resultp = new $1_ltype(($1_ltype &)$1);
  intrusive_ptr_add_ref(resultp);
  *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(resultp, SWIG_intrusive_deleter< CONST TYPE >()); 
%}

%typemap(in, canthrow=1) CONST TYPE * (SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *smartarg = 0) %{
  // plain pointer
  smartarg = *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input;
  $1 = (TYPE *)(smartarg ? smartarg->get() : 0); 
%}
%typemap(out, fragment="SWIG_intrusive_deleter,SWIG_null_deleter") CONST TYPE * %{
  //plain pointer(out)
  #if ($owner)
  if ($1) {
    intrusive_ptr_add_ref($1);
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1, SWIG_intrusive_deleter< CONST TYPE >());  
  } else {
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = 0;
  }
  #else
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = $1 ? new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1 SWIG_NO_NULL_DELETER_0) : 0;
  #endif
%}

%typemap(in, canthrow=1) CONST TYPE & (SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *smartarg = 0) %{
  // plain reference
  $1 = ($1_ltype)((*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input) ? (*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input)->get() : 0);
  if(!$1) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "$1_type reference is null", 0);
    return $null;
  } 
%}
%typemap(out, fragment="SWIG_intrusive_deleter,SWIG_null_deleter") CONST TYPE & %{ 
  //plain reference(out)
  #if ($owner)
  if ($1) {
    intrusive_ptr_add_ref($1);
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1, SWIG_intrusive_deleter< CONST TYPE >());  
  } else {
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = 0;
  } 
  #else
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = $1 ? new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1 SWIG_NO_NULL_DELETER_0) : 0;
  #endif
%}

%typemap(in) TYPE *CONST& ($*1_ltype temp = 0, SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *smartarg = 0) %{ 
  // plain pointer by reference
  temp = ($*1_ltype)((*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input) ? (*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input)->get() : 0);
  $1 = &temp; 
%}
%typemap(out, fragment="SWIG_intrusive_deleter,SWIG_null_deleter") TYPE *CONST& %{ 
  // plain pointer by reference(out)
  #if ($owner)
  if (*$1) {
    intrusive_ptr_add_ref(*$1);
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(*$1, SWIG_intrusive_deleter< CONST TYPE >());  
  } else {
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = 0;
  } 
  #else
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(*$1 SWIG_NO_NULL_DELETER_0);
  #endif
%}

%typemap(in) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > ($&1_type argp, SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * smartarg) %{ 
  // intrusive_ptr by value
  smartarg = *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >**)&$input;
  if (smartarg) {
  	$1 = SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >(smartarg->get(), true); 
  }
%}
%typemap(out, fragment="SWIG_intrusive_deleter") SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > %{ 
  if ($1) {
  	intrusive_ptr_add_ref(result.get());
  	*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(result.get(), SWIG_intrusive_deleter< CONST TYPE >());
  } else {
   	*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = 0; 
  }
%}

%typemap(in) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > swigSharedPtrUpcast ($&1_type smartarg) %{
  // shared_ptr by value
  smartarg = *($&1_ltype*)&$input; 
  if (smartarg) $1 = *smartarg; 
%}
%typemap(out) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > ANY_TYPE_SWIGSharedPtrUpcast %{ 
  *($&1_ltype*)&$result = $1 ? new $1_ltype($1) : 0; 
%}

%typemap(in) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > & ($*1_ltype tempnull, $*1_ltype temp, SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * smartarg) %{ 
  // intrusive_ptr by reference
  if ( $input ) {
  	smartarg = *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >**)&$input; 
  	temp = SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >(smartarg->get(), true);
  	$1 = &temp;
  } else {
	$1 = &tempnull;
  }
%}
%typemap(memberin) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > & %{
  delete &($1);
  if ($self) {
    SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > * temp = new SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >(*$input);
    $1 = *temp;
  }
%}
%typemap(out, fragment="SWIG_intrusive_deleter") SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > & %{ 
  if (*$1) {
    intrusive_ptr_add_ref($1->get());
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1->get(), SWIG_intrusive_deleter< CONST TYPE >());
  } else {
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = 0;
  }
%} 

%typemap(in) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > * ($*1_ltype tempnull, $*1_ltype temp, SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * smartarg) %{ 
  // intrusive_ptr by pointer
  if ( $input ) {
  	smartarg = *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >**)&$input; 
  	temp = SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >(smartarg->get(), true);
  	$1 = &temp; 
  } else {
	$1 = &tempnull;
  }
%}
%typemap(memberin) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > * %{
  delete $1;
  if ($self) $1 = new SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >(*$input);
%}
%typemap(out, fragment="SWIG_intrusive_deleter") SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > * %{ 
  if ($1 && *$1) {
    intrusive_ptr_add_ref($1->get());
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1->get(), SWIG_intrusive_deleter< CONST TYPE >());
  } else {
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = 0;
  }
  if ($owner) delete $1; 
%}

%typemap(in) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *& (SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > temp, $*1_ltype tempp = 0, SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * smartarg) %{ 
  // intrusive_ptr by pointer reference
  smartarg = *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >**)&$input;
  if ($input) {
    temp = SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >(smartarg->get(), true);
  }
  tempp = &temp;
  $1 = &tempp;
%}
%typemap(memberin) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *& %{
  if ($self) $1 = *$input;
%}
%typemap(out, fragment="SWIG_intrusive_deleter") SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *& %{ 
  if (*$1 && **$1) {
    intrusive_ptr_add_ref((*$1)->get());
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >((*$1)->get(), SWIG_intrusive_deleter< CONST TYPE >());
  } else {
    *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = 0;
  }
%} 

// various missing typemaps - If ever used (unlikely) ensure compilation error rather than runtime bug
%typemap(in) CONST TYPE[], CONST TYPE[ANY], CONST TYPE (CLASS::*) %{
#error "typemaps for $1_type not available"
%}
%typemap(out) CONST TYPE[], CONST TYPE[ANY], CONST TYPE (CLASS::*) %{
#error "typemaps for $1_type not available"
%}


%typemap (ctype)    SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > &,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *& "void *"
%typemap (imtype, out="IntPtr")  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >, 
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > &,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *& "HandleRef"
%typemap (cstype) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >,
                  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > &,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *,
                  SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *& "$typemap(cstype, TYPE)"
%typemap(csin) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >,
                 SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >,
                 SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > &,
                 SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *,
                 SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *& "$typemap(cstype, TYPE).getCPtr($csinput)"

%typemap(csout, excode=SWIGEXCODE) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > {
    IntPtr cPtr = $imcall;
    $typemap(cstype, TYPE) ret = (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);$excode
    return ret; 
  }
%typemap(csout, excode=SWIGEXCODE) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > {
    IntPtr cPtr = $imcall;
    $typemap(cstype, TYPE) ret = (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > & {
    IntPtr cPtr = $imcall;
    $typemap(cstype, TYPE) ret = (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);$excode
    return ret; 
  }
%typemap(csout, excode=SWIGEXCODE) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > * {
    IntPtr cPtr = $imcall;
    $typemap(cstype, TYPE) ret = (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);$excode
    return ret; 
  }
%typemap(csout, excode=SWIGEXCODE) SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE > *& {
    IntPtr cPtr = $imcall;
    $typemap(cstype, TYPE) ret = (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);$excode
    return ret; 
  }


%typemap(csout, excode=SWIGEXCODE) CONST TYPE {
    $typemap(cstype, TYPE) ret = new $typemap(cstype, TYPE)($imcall, true);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) CONST TYPE & {
    $typemap(cstype, TYPE) ret = new $typemap(cstype, TYPE)($imcall, true);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) CONST TYPE * {
    IntPtr cPtr = $imcall;
    $typemap(cstype, TYPE) ret = (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);$excode
    return ret;
  }
%typemap(csout, excode=SWIGEXCODE) TYPE *CONST& {
    IntPtr cPtr = $imcall;
    $typemap(cstype, TYPE) ret = (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);$excode
    return ret;
  }

// Base proxy classes
%typemap(csbody) TYPE %{
  private HandleRef swigCPtr;
  private bool swigCMemOwnBase;

  public $csclassname(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwnBase = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }
%}

// Derived proxy classes
%typemap(csbody_derived) TYPE %{
  private HandleRef swigCPtr;
  private bool swigCMemOwnDerived;

  public $csclassname(IntPtr cPtr, bool cMemoryOwn)
  : base($imclassname.$csclazznameSWIGSmartPtrUpcast(cPtr), true) {
    
    swigCMemOwnDerived = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }
%}

%typemap(csdestruct, methodname="Dispose", methodmodifiers="public") TYPE {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwnBase) {
          swigCMemOwnBase = false;
          $imcall;
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

%typemap(csdestruct_derived, methodname="Dispose", methodmodifiers="public") TYPE {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwnDerived) {
          swigCMemOwnDerived = false;
          $imcall;
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
      base.Dispose();
    }
  }

// CONST version needed ???? also for C#
%typemap(imtype, nopgcpp="1") SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< TYPE > swigSharedPtrUpcast "HandleRef"
%typemap(imtype, nopgcpp="1") SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > swigSharedPtrUpcast "HandleRef"


%template() SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >;
%template() SWIG_INTRUSIVE_PTR_QNAMESPACE::intrusive_ptr< CONST TYPE >;
%enddef


/////////////////////////////////////////////////////////////////////


%include <shared_ptr.i>

%define SWIG_INTRUSIVE_PTR_TYPEMAPS_NO_WRAP(CONST, TYPE...)

%naturalvar TYPE;
%naturalvar SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >;

// destructor mods
%feature("unref") TYPE "(void)arg1; delete smartarg1;"


// plain value
%typemap(in, canthrow=1) CONST TYPE ($&1_type argp = 0) %{
  argp = (*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input) ? (*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input)->get() : 0;
  if (!argp) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "Attempt to dereference null $1_type", 0);
    return $null;
  }
  $1 = *argp; %}
%typemap(out) CONST TYPE 
%{ *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(new $1_ltype(($1_ltype &)$1)); %}

// plain pointer
%typemap(in) CONST TYPE * (SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *smartarg = 0) %{
  smartarg = *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input;
  $1 = (TYPE *)(smartarg ? smartarg->get() : 0); %}
%typemap(out, fragment="SWIG_null_deleter") CONST TYPE * %{
  *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = $1 ? new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1 SWIG_NO_NULL_DELETER_$owner) : 0;
%}

// plain reference
%typemap(in, canthrow=1) CONST TYPE & %{
  $1 = ($1_ltype)((*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input) ? (*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input)->get() : 0);
  if (!$1) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "$1_type reference is null", 0);
    return $null;
  } %}
%typemap(out, fragment="SWIG_null_deleter") CONST TYPE &
%{ *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1 SWIG_NO_NULL_DELETER_$owner); %}

// plain pointer by reference
%typemap(in) TYPE *CONST& ($*1_ltype temp = 0)
%{ temp = ($*1_ltype)((*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input) ? (*(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$input)->get() : 0);
   $1 = &temp; %}
%typemap(out, fragment="SWIG_null_deleter") TYPE *CONST&
%{ *(SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > **)&$result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(*$1 SWIG_NO_NULL_DELETER_$owner); %}

%typemap(in) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > swigSharedPtrUpcast ($&1_type smartarg) %{
  // shared_ptr by value
  smartarg = *($&1_ltype*)&$input; 
  if (smartarg) $1 = *smartarg; 
%}
%typemap(out) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > ANY_TYPE_SWIGSharedPtrUpcast %{ 
  *($&1_ltype*)&$result = $1 ? new $1_ltype($1) : 0; 
%}

// various missing typemaps - If ever used (unlikely) ensure compilation error rather than runtime bug
%typemap(in) CONST TYPE[], CONST TYPE[ANY], CONST TYPE (CLASS::*) %{
#error "typemaps for $1_type not available"
%}
%typemap(out) CONST TYPE[], CONST TYPE[ANY], CONST TYPE (CLASS::*) %{
#error "typemaps for $1_type not available"
%}


%typemap (ctype)    SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > "void *"
%typemap (imtype)  SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > "void *"
%typemap (cstype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > "$typemap(cstype, TYPE)"
%typemap (csin) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > "$typemap(cstype, TYPE).getCPtr($csinput)"
%typemap(csout, excode=SWIGEXCODE) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > {
    IntPtr cPtr = $imcall;
    return (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);
  }

%typemap(csout, excode=SWIGEXCODE) CONST TYPE {
    return new $typemap(cstype, TYPE)($imcall, true);
  }
%typemap(csout, excode=SWIGEXCODE) CONST TYPE & {
    return new $typemap(cstype, TYPE)($imcall, true);
  }
%typemap(csout, excode=SWIGEXCODE) CONST TYPE * {
    IntPtr cPtr = $imcall;
    return (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);
  }
%typemap(csout, excode=SWIGEXCODE) TYPE *CONST& {
    IntPtr cPtr = $imcall;
    return (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);
  }

// Base proxy classes
%typemap(csbody) TYPE %{
  private HandleRef swigCPtr;
  private bool swigCMemOwnBase;

  public $csclassname(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwnBase = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }
%}

// Derived proxy classes
%typemap(csbody_derived) TYPE %{
  private HandleRef swigCPtr;
  private bool swigCMemOwnDerived;

  public $csclassname(IntPtr cPtr, bool cMemoryOwn) 
    : base($imclassname.$csclazznameSWIGSmartPtrUpcast(cPtr), true) {

    swigCMemOwnDerived = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr($csclassname obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }
%}

%typemap(csdestruct, methodname="Dispose", methodmodifiers="public") TYPE {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwnBase) {
          swigCMemOwnBase = false;
          $imcall;
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

%typemap(csdestruct_derived, methodname="Dispose", methodmodifiers="public") TYPE {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwnDerived) {
          swigCMemOwnDerived = false;
          $imcall;
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
      base.Dispose();
    }
  }


// CONST version needed ???? also for C#
%typemap(imtype, nopgcpp="1") SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< TYPE > swigSharedPtrUpcast "HandleRef"
%typemap(imtype, nopgcpp="1") SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > swigSharedPtrUpcast "HandleRef"


%template() SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >;
%enddef

