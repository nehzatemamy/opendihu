#include "basis_function/lagrange.h"

namespace BasisFunction
{
  
template<int order>
constexpr int Lagrange<order>::
nDofsPerNode()
{
  return 1;
}

// linear Lagrange basis
template<>
constexpr int Lagrange<1>::
nDofsPerBasis()
{
  return 2;
}

// quadratic Lagrange basis
template<>
constexpr int Lagrange<2>::
nDofsPerBasis()
{
  return 3;
}

};  // namespace