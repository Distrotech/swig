%module template_opaque
%include std_vector.i

%{
  namespace A 
  {
    struct OpaqueStruct 
    {
    };
  }

  enum Hello { hi, hello };
      
%}

  
%inline {
namespace A {
  struct OpaqueStruct;
  typedef struct OpaqueStruct OpaqueType;
  typedef enum Hello Hi;
  typedef std::vector<OpaqueType> OpaqueVectorType;
  typedef std::vector<Hi> OpaqueVectorEnum;
  
  void FillVector(OpaqueVectorType& v) 
  {
    for (size_t i = 0; i < v.size(); ++i) {
      v[i] = OpaqueStruct();
    }
  }

  void FillVector(const OpaqueVectorEnum& v) 
  {
  }
}
}

%template(OpaqueVectorType) std::vector<A::OpaqueType>; 
