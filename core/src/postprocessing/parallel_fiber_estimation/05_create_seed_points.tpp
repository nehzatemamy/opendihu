#include "postprocessing/parallel_fiber_estimation/parallel_fiber_estimation.h"

namespace Postprocessing
{

template<typename BasisFunctionType>
void ParallelFiberEstimation<BasisFunctionType>::
createSeedPoints(const std::array<bool,4> &subdomainIsAtBorder, int seedPointsZIndex, const std::vector<Vec3> &nodePositions, std::vector<Vec3> &seedPoints)
{
  // nodePositions contains all node positions in the current 3D mesh
  // naming of borders: (0-,1-,0+,1+)
  //     ___1+__
  //    |   |   |
  // 0- |___|___| 0+
  // ^  |   |   |
  // |  |___|___|
  // +-->   1-

  LOG(DEBUG) << "createSeedPoints, seedPointsZIndex: " << seedPointsZIndex << ", subdomainIsAtBorder: " << std::boolalpha << subdomainIsAtBorder;
  LOG(DEBUG) << "nBorderPointsXNew: " << nBorderPointsXNew_;

  std::vector<std::array<int,2>> seedPointPositionDebug;

  int subdomainNNodesX = nBorderPointsXNew_;
  int subdomainNNodesY = nBorderPointsXNew_;

  // boundary indices for face0Minus and face0Plus (vertical direction)
  int iBeginVertical = 0;
  int iEndVertical = nBorderPointsXNew_;

  if (subdomainIsAtBorder[(int)Mesh::face_t::face1Minus])
    iBeginVertical += 1;

  if (subdomainIsAtBorder[(int)Mesh::face_t::face1Plus])
    iEndVertical -= 1;

  // boundary indices for face1Minus and face1Plus (horizontal direction)
  int iBeginHorizontal = 0;
  int iEndHorizontal = nBorderPointsXNew_;

  if (subdomainIsAtBorder[(int)Mesh::face_t::face0Minus])
    iBeginHorizontal += 1;

  if (subdomainIsAtBorder[(int)Mesh::face_t::face0Plus])
    iEndHorizontal -= 1;

  LOG(DEBUG) << "seedPoints: starting with faces";

  // face0Minus
  if (!subdomainIsAtBorder[(int)Mesh::face_t::face0Minus])
  {
    for (int i = iBeginVertical; i < iEndVertical; i++)
    {
      seedPoints.push_back(nodePositions[seedPointsZIndex*subdomainNNodesX*subdomainNNodesY + i*subdomainNNodesX + 0]);
      seedPointPositionDebug.push_back(std::array<int,2>({0,i}));
    }
  }

  // face0Plus
  if (!subdomainIsAtBorder[(int)Mesh::face_t::face0Plus])
  {
    for (int i = iBeginVertical; i < iEndVertical; i++)
    {
      seedPoints.push_back(nodePositions[seedPointsZIndex*subdomainNNodesX*subdomainNNodesY + i*subdomainNNodesX + (subdomainNNodesX-1)]);
      seedPointPositionDebug.push_back(std::array<int,2>({subdomainNNodesX-1,i}));
    }
  }

  // face1Minus (with corner points)
  if (!subdomainIsAtBorder[(int)Mesh::face_t::face1Minus])
  {
    for (int i = iBeginHorizontal; i < iEndHorizontal; i++)
    {
      seedPoints.push_back(nodePositions[seedPointsZIndex*subdomainNNodesX*subdomainNNodesY + i]);
      seedPointPositionDebug.push_back(std::array<int,2>({i,0}));
    }
  }

  // face1Plus (with corner points)
  if (!subdomainIsAtBorder[(int)Mesh::face_t::face1Plus])
  {
    for (int i = iBeginHorizontal; i < iEndHorizontal; i++)
    {
      seedPoints.push_back(nodePositions[seedPointsZIndex*subdomainNNodesX*subdomainNNodesY + (subdomainNNodesY-1)*subdomainNNodesX + i]);
      seedPointPositionDebug.push_back(std::array<int,2>({i,subdomainNNodesY-1}));
    }
  }


  LOG(DEBUG) << "seedPoints: starting with horizontal center line, streamlineIndex = " << seedPoints.size();

  // horizontal center line (with corner points)
  for (int i = iBeginHorizontal; i < iEndHorizontal; i++)
  {
    seedPoints.push_back(nodePositions[seedPointsZIndex*subdomainNNodesX*subdomainNNodesY + int(subdomainNNodesY/2)*subdomainNNodesX + i]);
    seedPointPositionDebug.push_back(std::array<int,2>({i,int(subdomainNNodesY/2)}));
  }

  LOG(DEBUG) << "seedPoints: starting with vertical center line, streamlineIndex = " << seedPoints.size();

  // vertical center line (with corner points and center point)
  for (int i = iBeginVertical; i < iEndVertical; i++)
  {
    seedPoints.push_back(nodePositions[seedPointsZIndex*subdomainNNodesX*subdomainNNodesY + i*subdomainNNodesX + int(subdomainNNodesX/2)]);
    seedPointPositionDebug.push_back(std::array<int,2>({int(subdomainNNodesX/2),i}));
  }

  LOG(DEBUG) << "seedPoints: end, streamlineIndex = " << seedPoints.size();


  // create seed points at corners for border points
  //     ___1+__
  //    |   |   |
  // 0- |___|___| 0+
  // ^  |   |   |
  // |  |___|___|
  // +-->   1-

  std::vector<std::pair<int,int>> coordinates =
  {
    std::pair<int,int>{1,1},    // bottom left
    std::pair<int,int>{subdomainNNodesX-2, 1},    // bottom right
    std::pair<int,int>{1, subdomainNNodesY-2},    // top left
    std::pair<int,int>{subdomainNNodesX-2, subdomainNNodesY-2}    // top right
  };

  for (std::vector<std::pair<int,int>>::iterator iter = coordinates.begin(); iter != coordinates.end(); iter++)
  {
    int i = iter->first;
    int j = iter->second;
    LOG(DEBUG) << "(i,j) = " << i << "," << j << ", index: " << seedPointsZIndex*subdomainNNodesX*subdomainNNodesY + j*subdomainNNodesX + i
      << ", size of nodePositions: " << nodePositions.size();
    seedPoints.push_back(nodePositions[seedPointsZIndex*subdomainNNodesX*subdomainNNodesY + j*subdomainNNodesX + i]);
    seedPointPositionDebug.push_back(std::array<int,2>({i,j}));
  }

  LOG(DEBUG) << "seedPoints: after cornerStreamlines, streamlineIndex = " << seedPoints.size();

#ifndef NDEBUG
  LOG(DEBUG) << "seed point positions: ";
  for (int i = 0; i < seedPointPositionDebug.size(); i++)
  {
    LOG(DEBUG) << "  " << i << " at " << seedPointPositionDebug[i] << ": " << seedPoints[i];
  }
#endif
}

template<typename BasisFunctionType>
void ParallelFiberEstimation<BasisFunctionType>::
extractSeedPointsFromBorderPoints(const std::array<std::array<std::vector<std::vector<Vec3>>,4>,8> &borderPointsSubdomain, std::array<std::vector<Vec3>,4> cornerStreamlines, const std::array<bool,4> &subdomainIsAtBorder,
                                  bool streamlineDirectionUpwards, std::vector<Vec3> &seedPoints)
{
  int seedPointsZIndex = 0;
  if (streamlineDirectionUpwards)
  {
    seedPointsZIndex = nBorderPointsZ_-1;
  }
  // boundary indices for face0Minus and face0Plus (vertical direction)
  int iBeginVertical = 0;
  int iEndVertical = nBorderPointsXNew_;

  if (subdomainIsAtBorder[(int)Mesh::face_t::face1Minus])
    iBeginVertical += 1;

  if (subdomainIsAtBorder[(int)Mesh::face_t::face1Plus])
    iEndVertical -= 1;

  // boundary indices for face1Minus and face1Plus (horizontal direction)
  int iBeginHorizontal = 0;
  int iEndHorizontal = nBorderPointsXNew_;

  if (subdomainIsAtBorder[(int)Mesh::face_t::face0Minus])
    iBeginHorizontal += 1;

  if (subdomainIsAtBorder[(int)Mesh::face_t::face0Plus])
    iEndHorizontal -= 1;

  LOG(DEBUG) << "seedPoints: starting with faces";

  // face0Minus
  if (!subdomainIsAtBorder[(int)Mesh::face_t::face0Minus])
  {
    for (int i = iBeginVertical; i < iEndVertical; i++)
    {
      int subdomainIndex = (i < nBorderPointsX_-1? 0 : 2) + (streamlineDirectionUpwards? 4 : 0);
      int streamlineIndex = i % (nBorderPointsX_-1);
      seedPoints.push_back(borderPointsSubdomain[subdomainIndex][(int)Mesh::face_t::face0Minus][seedPointsZIndex][streamlineIndex]);
    }
  }

  // face0Plus
  if (!subdomainIsAtBorder[(int)Mesh::face_t::face0Plus])
  {
    for (int i = iBeginVertical; i < iEndVertical; i++)
    {
      int subdomainIndex = (i < nBorderPointsX_-1? 1 : 3) + (streamlineDirectionUpwards? 4 : 0);
      int streamlineIndex = i % (nBorderPointsX_-1);
      seedPoints.push_back(borderPointsSubdomain[subdomainIndex][(int)Mesh::face_t::face0Plus][seedPointsZIndex][streamlineIndex]);
    }
  }

  // face1Minus (with corner points)
  if (!subdomainIsAtBorder[(int)Mesh::face_t::face1Minus])
  {
    for (int i = iBeginHorizontal; i < iEndHorizontal; i++)
    {
      int subdomainIndex = (i < nBorderPointsX_-1? 0 : 1) + (streamlineDirectionUpwards? 4 : 0);
      int streamlineIndex = i % (nBorderPointsX_-1);
      seedPoints.push_back(borderPointsSubdomain[subdomainIndex][(int)Mesh::face_t::face1Minus][seedPointsZIndex][streamlineIndex]);
    }
  }

  // face1Plus (with corner points)
  if (!subdomainIsAtBorder[(int)Mesh::face_t::face1Plus])
  {
    for (int i = iBeginHorizontal; i < iEndHorizontal; i++)
    {
      int subdomainIndex = (i < nBorderPointsX_-1? 2 : 3) + (streamlineDirectionUpwards? 4 : 0);
      int streamlineIndex = i % (nBorderPointsX_-1);
      seedPoints.push_back(borderPointsSubdomain[subdomainIndex][(int)Mesh::face_t::face1Plus][seedPointsZIndex][streamlineIndex]);
    }
  }

  // horizontal center line (with corner points)
  for (int i = iBeginHorizontal; i < iEndHorizontal; i++)
  {
    int subdomainIndex = (i < nBorderPointsX_-1? 0 : 1) + (streamlineDirectionUpwards? 4 : 0);
    int streamlineIndex = i % (nBorderPointsX_-1);
    seedPoints.push_back(borderPointsSubdomain[subdomainIndex][(int)Mesh::face_t::face1Plus][seedPointsZIndex][streamlineIndex]);
  }

  // vertical center line (with corner points and center point)
  for (int i = iBeginVertical; i < iEndVertical; i++)
  {
    int subdomainIndex = (i < nBorderPointsX_-1? 1 : 3) + (streamlineDirectionUpwards? 4 : 0);
    int streamlineIndex = i % (nBorderPointsX_-1);
    seedPoints.push_back(borderPointsSubdomain[subdomainIndex][(int)Mesh::face_t::face0Minus][seedPointsZIndex][streamlineIndex]);
  }

  // corner streamlines
  if (streamlineDirectionUpwards)
  {
    seedPointsZIndex = nBorderPointsZNew_-1;
  }
  for (int i = 0; i < 4; i++)
  {
    seedPoints.push_back(cornerStreamlines[i][seedPointsZIndex]);
  }
}

} // namespace
