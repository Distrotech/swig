%module template_default



%warnfilter(801) ns1::Traits::c; /* Ruby, wrong constant name */
namespace ns1 {
  namespace ns2 {
  
    %feature("hello") A<int>;
    %feature("hello") A<double,double>;
    %feature("hi") A<int,int>;
    %feature("hi") A<double>;
    
    %extend A<int> 
    {
      int foo() { return 1; }
    }
    
    %extend A<double,double> 
    {
      int foo() { return 1; }
    }
    
    %extend A<int,int> 
    {
      int bar() { return 1; }
    }
    
    %extend A<double> 
    {
      int bar() { return 1; }
    }

    %extend N<double> 
    {
      int bar() { return 1; }
    }
  }
}

%inline %{
  
  namespace ns1 {
    namespace ns2 {
      
      struct Parm
      {
      };
      
      template <class T1, class T2 = T1>
      class A
      {
	
#ifdef SWIG      
	%typemap(in) A *  { /* in A */ }
	%typemap(out) A *  { /* out A */ }
#endif
      };

      typedef unsigned int category;
      
      const category one = 1;
      const category two = 1;
      
      
      template <class T1, category C = one, class T2 = Parm>
      class N
      {
	
#ifdef SWIG      
	%typemap(in) N *  { /* in N */ }
	%typemap(out) N *  { /* out N */ }
#endif
      };
    }
  }
%}

      


%template(A_p) ns1::ns2::A<double,ns1::ns2::Parm>;
%template(N_1p) ns1::ns2::N<int>;


namespace ns1 {
  namespace ns2 {
    %template(A_ii) A<int, int>;       
    %template(A_d) A<double>;
    %template(N_d) N<double>;
    
  }
}


%inline %{
  namespace ns1 {
    namespace ns2 {    
      namespace ns3 {  

	struct B : A<int> 
	{
	};
	
	struct C : N<double,ns1::ns2::one> 
	{	  
	};
	
	
	A<int> *get_a1(A<int> *a) {
	  return a;
	}
	
	A<int,int> *get_a2(A<int,int> *a) {
	  return a;
	}

      }
    }
  }
%}

%inline %{
  namespace ns1 {    
    struct Traits
    {
      static const ns2::category c = ns2::one;
    };
    namespace ns4 {    
      
      template <class T>
      struct D : ns2::N<double,T::c>
      {
	D()
	{
	}
	
      };

      template <class T1, class T2 = D<T1> >
      struct Base : T2
      {
      };
    }
  }
%}


%template(Do) ns1::ns4::D<ns1::Traits>;
%template(Bo) ns1::ns4::Base<ns1::Traits, ns1::ns4::D<ns1::Traits> >;



%inline %{
  namespace ns1 {    
    namespace ns5 {    
      
      struct Der : ns4::Base<Traits>
      {
      };
    }
  }
  
%}
