/*******************************************************************************
 * Simplified Wrapper and Interface Generator  (SWIG)
 * 
 * Author : David Beazley
 *
 * Department of Computer Science        
 * University of Chicago
 * 1100 E 58th Street
 * Chicago, IL  60637
 * beazley@cs.uchicago.edu
 *
 * Please read the file LICENSE for the copyright and terms by which SWIG
 * can be used and distributed.
 *******************************************************************************/

/**************************************************************************
 * $Header$
 *
 * python.h
 *
 * Header file for Python module.   Warning ; this is work in progress.
 **************************************************************************/

class PYTHON : public Language {
protected:
  void add_method(char *name, char *function, int kw);
  char *usage_func(char *, SwigType *, ParmList *);
  void emitAddPragmas(String *output, char* name, char* spacing);
public :

  // Don't change any of this
  virtual void main(int, char *argv[]);
  virtual int top(Node *); 
  virtual int functionWrapper(Node *);
  virtual int constantWrapper(Node *);
  virtual int variableWrapper(Node *);

  virtual int membervariableDeclaration(Node *);
  virtual int memberconstantDeclaration(Node *);
  virtual int memberfunctionDeclaration(Node *);
  virtual int publicconstructorDeclaration(Node *);
  virtual int publicdestructorDeclaration(Node *);

  virtual void set_module(char *);
  virtual void add_native(char *, char *, SwigType *, ParmList *);
  virtual void create_command(char *, char *);
  virtual void import_start(char *);
  virtual void import_end();

  // C++ extensions---for creating shadow classes
  
  virtual void cpp_open_class(char *classname, char *rname, char *ctype, int strip);
  virtual void cpp_close_class();
  virtual void cpp_inherit(char **baseclass, int mode = 0);


  virtual void cpp_class_decl(char *, char *,char *);
  virtual void pragma(char *, char *, char *);
  virtual void add_typedef(SwigType *t, char *name);
};

#define PYSHADOW_MEMBER  0x2







