#include "swig.h"
#include "swigwarn.h"
#include "swigver.h"

extern  char     *cparse_file;
extern  int       cparse_line;
extern  int       cparse_cplusplus;
extern  int       cparse_start_line;
extern  int       CParse_errors();
extern  void      Swig_cparse_replace_descriptor(String *s);
extern  void      Swig_cparse_cpluscplus(int);

