%module template_typedef_import
%{
#include "template_typedef_cplx2.h"
%}

%import "template_typedef_cplx2.h"

%inline %{

  typedef vfncs::UnaryFunction<double, double> RFunction;
  typedef vfncs::UnaryFunction<Complex, Complex> CFunction;
  

  int my_func_r(RFunction* hello)
    {
      return 0;
    }
  
  int my_func_c(CFunction* hello)
    {
      return 1;
    }  

%}

  



