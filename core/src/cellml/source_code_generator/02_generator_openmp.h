#pragma once

#include <Python.h>  // has to be the first included header

#include "cellml/source_code_generator/01_generator_simd.h"

class CellmlSourceCodeGeneratorOpenMp : public CellmlSourceCodeGeneratorSimd
{
public:
  //! constructor of parent class
  using CellmlSourceCodeGeneratorSimd::CellmlSourceCodeGeneratorSimd;

protected:

  //! write the source file with openmp support
  void generateSourceFileOpenMP(std::string outputFilename);


};
