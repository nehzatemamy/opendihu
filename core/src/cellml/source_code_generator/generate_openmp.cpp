#include "cellml/source_code_generator/source_code_generator.h"

#include <Python.h>  // has to be the first included header

#include "utility/string_utility.h"
#include "output_writer/generic.h"

#include <vector>
#include <iostream>
#include <Vc/Vc>
#include "easylogging++.h"

void CellMLSourceCodeGenerator::
generateSourceFileOpenMP(std::string outputFilename)
{
  std::stringstream simdSource;
  simdSource << "#include <math.h>" << std::endl
    << "#include <omp.h>" << std::endl
    << cellMLCode_.header << std::endl;

  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  simdSource << std::endl << "/* This function was created by opendihu at " << StringUtility::timeToString(&tm)  //std::put_time(&tm, "%d/%m/%Y %H:%M:%S")
    << ".\n * It is designed for " << this->nInstances_ << " instances of the CellML problem.\n "
    << "The \"optimizationType\" is \"openmp\". (Other options are \"vc\" and \"simd\".) */" << std::endl
    << "void computeCellMLRightHandSide("
    << "void *context, double t, double *states, double *rates, double *intermediates, double *parameters)" << std::endl << "{" << std::endl;

  simdSource << "  double VOI = t;   /* current simulation time */" << std::endl;
  simdSource << std::endl << "  /* define constants */" << std::endl
    << "  double CONSTANTS[" << this->nConstants_ << "];" << std::endl;

  // add assignments of constant values
  for (std::string constantAssignmentsLine : constantAssignments_)
  {
    simdSource << "  " << constantAssignmentsLine << std::endl;
  }

  simdSource << std::endl
    << "  #pragma omp parallel for" << std::endl
    << "  for (int i = 0; i < " << this->nInstances_ << "; i++)" << std::endl
    << "  {" << std::endl;

  // loop over lines of cellml code
  for (code_expression_t &codeExpression : cellMLCode_.lines)
  {
    if (codeExpression.type != code_expression_t::commented_out)
    {

      simdSource << "    ";
      codeExpression.visitLeafs([&simdSource,this](CellMLSourceCodeGenerator::code_expression_t &expression, bool isFirstVariable)
      {

        switch(expression.type)
        {
        case code_expression_t::variableName:

          if (expression.code == "CONSTANTS")
          {
            // constants only exist once for all instances
            simdSource << expression.code << "[" << expression.arrayIndex<< "]";
          }
          else
          {
            // all other variables (states, rates, intermediates, parameters) exist for every instance
            simdSource << expression.code << "[" << expression.arrayIndex * this->nInstances_ << "+i]";
          }
          break;

        case code_expression_t::otherCode:
          simdSource << expression.code;
          break;

        case code_expression_t::commented_out:
          simdSource << "  // (not assigning to a parameter) " << expression.code;
          break;

        default:
          break;
        };
      });

      simdSource << std::endl;
    }
  }

  VLOG(2) << "write end of for loop (closing })";
  simdSource << "  }" << std::endl;

  // add footer
  simdSource << cellMLCode_.footer << std::endl;

  // write out source file
  std::ofstream simdSourceFile;
  OutputWriter::Generic::openFile(simdSourceFile, outputFilename);
  if (!simdSourceFile.is_open())
  {
    LOG(FATAL) << "Could not write to file \"" << outputFilename << "\".";
  }
  else
  {
    std::string fileContents = simdSource.str();
    simdSourceFile << fileContents;
    simdSourceFile.close();
  }

  additionalCompileFlags_ = "-fopenmp";
  compilerCommand_ = C_COMPILER_COMMAND;
  sourceFileSuffix_ = ".c";
}
