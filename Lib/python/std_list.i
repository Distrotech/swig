//
// std::list
// Python implementation

%include std_container.i

// List

%define %std_list_methods(list)
  %std_sequence_methods(list)
  
  void pop_front();
  void push_front(const value_type& x);
  		
  void remove(const value_type& x);
  void unique();
  void reverse();
  void sort();
  
  void merge(list& x);
%enddef


%define %std_list_methods_val(list)
  %std_sequence_methods_val(list)
  
  void pop_front();
  void push_front(value_type x);
  		
  void remove(value_type x);
  void unique();
  void reverse();
  void sort();
  
  void merge(list& x);
%enddef

// ------------------------------------------------------------------------
// std::list
// 
// The aim of all that follows would be to integrate std::list with 
// Python as much as possible, namely, to allow the user to pass and 
// be returned Python tuples or lists.
// const declarations are used to guess the intent of the function being
// exported; therefore, the following rationale is applied:
// 
//   -- f(std::list<T>), f(const std::list<T>&):
//      the parameter being read-only, either a Python sequence or a
//      previously wrapped std::list<T> can be passed.
//   -- f(std::list<T>&), f(std::list<T>*):
//      the parameter may be modified; therefore, only a wrapped std::list
//      can be passed.
//   -- std::list<T> f(), const std::list<T>& f():
//      the list is returned by copy; therefore, a Python sequence of T:s 
//      is returned which is most easily used in other Python functions
//   -- std::list<T>& f(), std::list<T>* f():
//      the list is returned by reference; therefore, a wrapped std::list
//      is returned
//   -- const std::list<T>* f(), f(const std::list<T>*):
//      for consistency, they expect and return a plain list pointer.
// ------------------------------------------------------------------------

%{
#include <list>
%}

%fragment("StdListTraits","header",fragment="StdSequenceTraits")
%{
  namespace swigpy {
    template <class T >
    struct traits_asptr<std::list<T> >  {
      typedef std::list<T> list_type;
      typedef T value_type;
      static int asptr(PyObject *obj, list_type **lis) {
	return traits_asptr_stdseq<list_type>::asptr(obj, lis);
      }
    };

    template <class T>
    struct traits_from<std::list<T> > {
      typedef std::list<T> list_type;
      static PyObject *from(const list_type& vec) {
	return traits_from_stdseq<list_type>::from(vec);
      }
    };
  }
%}

// exported classes

namespace std {

  template<class T > class list {
  public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;

    %traits_swigtype(T);

    %fragment(SWIG_Traits_frag(std::list<T >), "header",
	      fragment=SWIG_Traits_frag(T),
	      fragment="StdListTraits") {
      namespace swigpy {
	template <>  struct traits<std::list<T > > {
	  typedef pointer_category category;
	  static const char* type_name() {
	    return "std::list<" #T " >";
	  }
	};
      }
    }

    %typemap_traits_ptr(SWIG_CCode(LIST), std::list<T >);
  
    %std_list_methods(std::list<T >);
    %pysequence_methods(std::list<T >);
  };

  template<class T > class list<T*> {
  public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type reference;
    typedef value_type const_reference;

    %fragment(SWIG_Traits_frag(std::list<T* >), "header",
	      fragment="StdListTraits") {
      namespace swigpy {
	template <>  struct traits<std::list<T* > > {
	  typedef value_category category;
	  static const char* type_name() {
	    return "std::list<" #T " * >";
	  }
	};
      }
    }

    %typemap_traits_ptr(SWIG_CCode(LIST), std::list<T* >);

    %std_list_methods_val(std::list<T* >);
    %pysequence_methods_val(std::list<T* >);
  };

  // Add the order operations <,>,<=,=> as needed
  
  %define %std_order_list(T)
    %std_comp_methods(list<T>);
  %enddef
  
  %apply_otypes(%std_order_list);
}


%define %std_list_ptypen(...) 
%template() std::list<__VA_ARGS__ >;
%enddef

%apply_cpptypes(%std_list_ptypen);

