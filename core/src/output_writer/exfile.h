#pragma once

#include <Python.h>  // has to be the first included header
#include <iostream>
#include <vector>

#include "control/types.h"
#include "output_writer/generic.h"
#include "output_writer/exfile_writer.h"

namespace OutputWriter
{   

class Exfile : public Generic
{
public:
 
  //! constructor
  Exfile(PyObject *specificSettings);
 
  //! write out solution to given filename, if timeStepNo is not -1, this value will be part of the filename
  template<typename DataType>
  void write(DataType &data, int timeStepNo = -1, double currentTime = -1);
  
private:
 
};

};  // namespace

#include "output_writer/exfile.tpp"