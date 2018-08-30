#include "function_space/function_space.h"

#include <cmath>
#include <array>
#include <sstream>

#include "easylogging++.h"

namespace FunctionSpace
{

template<typename MeshType, typename BasisFunctionType>
std::array<dof_no_t,FunctionSpaceFunction<MeshType,BasisFunctionType>::nDofsPerElement()> FunctionSpace<MeshType,BasisFunctionType>::
getElementDofNosLocal(element_no_t elementNo) const
{
  std::array<dof_no_t,FunctionSpaceFunction<MeshType,BasisFunctionType>::nDofsPerElement()> dofNosLocal;
  for (int dofIndex = 0; dofIndex < FunctionSpaceFunction<MeshType,BasisFunctionType>::nDofsPerElement(); dofIndex++)
  {
    dofNosLocal[dofIndex] = this->getDofNo(elementNo, dofIndex);
  }
  return dofNosLocal;
}

template<typename MeshType, typename BasisFunctionType>
void FunctionSpace<MeshType,BasisFunctionType>::
getElementDofNosLocal(element_no_t elementNo, std::vector<dof_no_t> &globalDofNos) const
{
  globalDofNos.resize(FunctionSpaceFunction<MeshType,BasisFunctionType>::nDofsPerElement());
  for (int dofIndex = 0; dofIndex < FunctionSpaceFunction<MeshType,BasisFunctionType>::nDofsPerElement(); dofIndex++)
  {
    globalDofNos[dofIndex] = this->getDofNo(elementNo, dofIndex);
  }
}


};  // namespace