%module special_variable_macros

// test $typemap() special variable function
// these tests are not typical of how $typemap() should be used, but it checks that it is mostly working

%inline %{
struct Name {
  Name(const char *n="none") : name(n) {}
  const char *getName() const { return name; };
  Name *getNamePtr() { return this; };
private:
  const char *name;
};
struct NameWrap {
  NameWrap(const char *n="casternone") : name(n) {}
  Name *getNamePtr() { return &name; };
private:
  Name name;
};
%}

// check $1 and $input get expanded properly when used from $typemap()
%typemap(in) Name *GENERIC ($*1_type temp)
%{
  /*%typemap(in) Name *GENERIC start */
  temp = Name("$specialname");
  (void)$input;
  $1 = ($1_ltype) &temp;
  /*%typemap(in) Name *GENERIC end */
%}

// This would never be done in real code, it is just a test of what madness can be done.
// Note that the special variable substitutions $*1_type, $descriptor etc are for NameWrap 
// even when used within the Name typemap via $typemap. I can't think of any useful use cases
// for this behaviour in the C/C++ typemaps, but it is possible.
%typemap(in) NameWrap *NAMEWRAP ($*1_type temp)
%{
  /*%typemap(in) NameWrap *NAMEWRAP start */
  temp = $*1_ltype("$descriptor");
  (void)$input;
  $1 = temp.getNamePtr();
  /*%typemap(in) NameWrap *NAMEWRAP end */
%}


//////////////////////////////////////////////////////////////////////////////////////

// This should use Name *GENERIC typemap which ignores passed in Name * and instead uses a newly a newly constructed Name
// held in a typemap variable with name="$specialname"
%typemap(in) Name *jack {
// %typemap(in) Name *jack start
$typemap(in, Name *GENERIC)
// %typemap(in) Name *jack end
}

// as above, but also perform variable substitution
%typemap(in) Name *jill {
// %typemap(in) Name *jill start
$typemap(in, Name *GENERIC, specialname=jilly)
// %typemap(in) Name *jill end
}

%typemap(in) Name *mary {
// %typemap(in) Name *mary start
$typemap(in, NameWrap *NAMEWRAP)
// %typemap(in) Name *mary end
}

%inline %{
const char * testFred(Name *fred) {
  return fred->getName();
}
const char * testJack(Name *jack) {
  return jack->getName();
}
const char * testJill(Name *jill) {
  return jill->getName();
}
const char * testMary(Name *mary) {
  return mary->getName();
}
%}

//////////////////////////////////////////////////////////////////////////////////////
// Multi-arg typemap lookup
// One would never do something like this in reality, it just checks $typemap with multi-arg typemaps
%typemap(in) (Name *multiname, int num)($*1_type temp_name, $2_ltype temp_count)
%{
  /*%typemap(in) (Name *multiname, int num) start */
  temp_name = $*1_ltype("multiname num");
  temp_count = strlen(temp_name.getNamePtr()->getName());
  (void)$input;
  $1 = temp_name.getNamePtr();
  $2 = temp_count + 100;
  /*%typemap(in) (Name *multiname, int num) end */
%}

%typemap(in) (Name *jim, int count) {
// %typemap(in) Name *jim start
$typemap(in, (Name *multiname, int num))
// %typemap(in) Name *jim end
}

%inline %{
const char * testJim(Name *jim, int count) {
  if (count != strlen(jim->getNamePtr()->getName()) + 100)
    return "size check failed";
  else
    return jim->getName();
}
%}

//////////////////////////////////////////////////////////////////////////////////////
// A real use case for $typemap

#if defined(SWIGCSHARP)
%typemap(cscode) Space::RenameMe %{
  public static NewName factory(String s) {
  //below should expand to:
  //return new NewName( new Name(s) );
    return new $typemap(cstype, Space::RenameMe)( new $typemap(cstype, Name)(s) ); 
  }
%}
#elif defined(SWIGJAVA)
%typemap(javacode) Space::RenameMe %{
  public static NewName factory(String s) {
  //below should expand to:
  //return new NewName( new Name(s) );
    return new $typemap(jstype, Space::RenameMe)( new $typemap(jstype, Name)(s) ); 
  }
%}
#endif

%rename(NewName) Space::RenameMe;
%inline %{
namespace Space {
  struct RenameMe {
    RenameMe(Name n) : storedName(n) {}
    Name getStoredName() { return storedName; }
  private:
    Name storedName;
  };
}
%}

