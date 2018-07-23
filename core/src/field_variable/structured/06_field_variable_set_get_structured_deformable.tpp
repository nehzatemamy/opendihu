#include "field_variable/structured/06_field_variable_set_get_structured_deformable.h"

#include <sstream>
#include "utility/string_utility.h"
#include <cassert>

namespace FieldVariable
{


//! copy the values from another field variable of the same type
template<int D,typename BasisFunctionType,int nComponents>
void FieldVariableSetGet<BasisOnMesh::BasisOnMesh<Mesh::StructuredDeformableOfDimension<D>,BasisFunctionType>,nComponents>::
setValues(FieldVariable<BasisOnMesh::BasisOnMesh<Mesh::StructuredDeformableOfDimension<D>,BasisFunctionType>,nComponents> &rhs)
{
  this->values_ = rhs.partitionedPetsVec();
}

};