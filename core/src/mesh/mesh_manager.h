#pragma once

#include <Python.h>  // has to be the first included header
#include <map>

#include "control/dihu_context.h"
#include "function_space/function_space.h"

namespace Partition{
class Manager;
};
namespace Mesh
{
class NodePositionsTester;

/**
 * This class creates and stores all used meshes.
 * Each mesh can be defined in the python config under "Meshes" with a name and other properties.
 * Various components of the program can later
 * request their mesh by a call to mesh(name).
 * If a mesh was not defined earlier, it is created on the fly when it is requested.
 */
class Manager
{
public:
  //! constructor
  Manager(PyObject *specificSettings);

  //! store the pointer to the partition manager
  void setPartitionManager(std::shared_ptr<Partition::Manager> partitionManager);
  
  //! return previously created mesh or create on the fly
  template<typename FunctionSpaceType=None>
  std::shared_ptr<Mesh> mesh(PyObject *settings);

  //! check if a mesh with the given name is stored
  bool hasMesh(std::string meshName);

  //! create a mesh not from python config but directly by calling an appropriate construtor. 
  //! With this e.g. meshes from node positions can be created.
  template<typename FunctionSpaceType, typename ...Args>
  std::shared_ptr<Mesh> createMesh(std::string name, Args && ...args);
  
  friend class NodePositionsTester;    ///< a class used for testing

private:
  //! store settings for all meshes that are specified in specificSettings_
  void storePreconfiguredMeshes();

  std::shared_ptr<Partition::Manager> partitionManager_;  ///< the partition manager object
  
  PyObject *specificSettings_;    ///< python object containing the value of the python config dict with corresponding key, for meshManager
  int numberAnonymousMeshes_;     ///< how many meshes without a given name in the python config are contained in meshes_. These have a key "anonymous<no>"

  std::map<std::string, PyObject *> meshConfiguration_;         ///< the python dicts for the meshes that were defined under "Meshes"
  std::map<std::string, std::shared_ptr<Mesh>> meshes_;    ///< the managed meshes with their string key
};

template<>
std::shared_ptr<Mesh> Manager::
mesh<None>(PyObject *settings);

};    // namespace

#include "mesh/mesh_manager.tpp"
