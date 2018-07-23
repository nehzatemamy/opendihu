#pragma once

#include <Python.h>  // has to be the first included header
#include <array>

#include "control/types.h"
#include "mesh/mesh.h"

namespace Mesh
{

/**
 * A structured mesh, i.e. it has a fixed number of elements in x,y and z direction.
 * This mesh type knows its number of elements.
 */
template<int D>
class Structured : public MeshOfDimension<D>
{
public:
 // TODO: constructor not needed? remove
  //! constructor from number of elements in coordinate directions
  //Structured(std::array<element_no_t, D> &nElements);

  //! constructor from python settings
  Structured(PyObject *specificSettings);

  //! get number of elements in a given coordinate direction
  element_no_t nElementsPerCoordinateDirectionLocal(int dimension) const;

  //! get the array with all numbers of elements per coordinate direction
  std::array<element_no_t, D> nElementsPerCoordinateDirectionLocal() const;

  //! get the total number of elements
  element_no_t nLocalElements() const;

protected:
  std::array<element_no_t, D> nElementsPerCoordinateDirectionLocal_;    ///< the number of stored elements in each coordinate direction (the locally computed portion)
};

};    // namespace

#include "mesh/structured.tpp"