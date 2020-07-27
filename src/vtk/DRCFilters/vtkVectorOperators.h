#ifndef __vtkVectorOperators_h
#define __vtkVectorOperators_h

#include "vtkVector.h"

namespace {

vtkVector3d operator-(const vtkVector3d &lhs, const vtkVector3d &rhs)
{
  return vtkVector3d(lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]);
}

vtkVector3d operator-(const vtkVector3d &rhs)
{
  return vtkVector3d(-rhs[0], -rhs[1], -rhs[2]);
}

vtkVector3d operator+(const vtkVector3d &lhs, const vtkVector3d &rhs)
{
  return vtkVector3d(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]);
}

vtkVector3d operator*(const vtkVector3d &lhs, const double rhs)
{
  return vtkVector3d(lhs[0] * rhs, lhs[1] * rhs, lhs[2] * rhs);
}

}

#endif
