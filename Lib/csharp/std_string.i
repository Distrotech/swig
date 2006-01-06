//
// SWIG typemaps for std::string
// William Fulton
//
// C# implementation
//
/* ------------------------------------------------------------------------
  Typemaps for std::string and const std::string&
  These are mapped to a C# String and are passed around by value.

  To use non-const std::string references use the following %apply.  Note 
  that they are passed by value.
  %apply const std::string & {std::string &};
  ------------------------------------------------------------------------ */

%{
#include <string>
%}

namespace std {

class string;

// string
%typemap(ctype) string "char *"
%typemap(imtype) string "string"
%typemap(cstype) string "string"

%typemap(in, canthrow=1) string 
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   $1 = std::string($input); %}
%typemap(out) string %{ $result = SWIG_csharp_string_callback($1.c_str()); %}

%typemap(csin) string "$csinput"
%typemap(csout, excode=SWIGEXCODE) string {
    string ret = $imcall;$excode
    return ret;
  }

%typemap(csvarin, excode=SWIGEXCODE2) string %{
    set {
      $imcall;$excode
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) string %{
    get {
      string ret = $imcall;$excode
      return ret;
    } %}

%typemap(typecheck) string = char *;

%typemap(throws, canthrow=1) string
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.c_str());
   return $null; %}

// const string &
%typemap(ctype) const string & "char *"
%typemap(imtype) const string & "string"
%typemap(cstype) const string & "string"

%typemap(in, canthrow=1) const string &
%{ if (!$input) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "null string", 0);
    return $null;
   }
   std::string $1_str($input);
   $1 = &$1_str; %}
%typemap(out) const string & %{ $result = SWIG_csharp_string_callback($1->c_str()); %}

%typemap(csin) const string & "$csinput"
%typemap(csout, excode=SWIGEXCODE) const string & {
    string ret = $imcall;$excode
    return ret;
  }

%typemap(csvarin, excode=SWIGEXCODE2) const string & %{
    set {
      $imcall;$excode
    } %}
%typemap(csvarout, excode=SWIGEXCODE2) const string & %{
    get {
      string ret = $imcall;$excode
      return ret;
    } %}

%typemap(typecheck) const string & = char *;

%typemap(throws, canthrow=1) const string &
%{ SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, $1.c_str());
   return $null; %}

}

