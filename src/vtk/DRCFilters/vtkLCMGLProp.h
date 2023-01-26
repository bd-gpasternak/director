/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLCMGLProp - An orthonormal frame representation
// .SECTION Description
// An orthonormal frame representation for use with the vtkFrameWidget

// .SECTION See Also
// vtkFrameWidget vtkFrameWidget


#ifndef __vtkLCMGLProp_h
#define __vtkLCMGLProp_h

#include "vtkProp.h"

#include <vtkDRCFiltersModule.h>

class VTKDRCFILTERS_EXPORT vtkLCMGLProp : public vtkProp
{
public:
  // Description:
  // Instantiate the class.
  static vtkLCMGLProp *New();

  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkLCMGLProp,vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void UpdateGLData(const char* data);

  virtual double *GetBounds() override;

  // Description:
  // Methods supporting, and required by, the rendering process.
  virtual void ReleaseGraphicsResources(vtkWindow*) override;
  virtual int RenderOpaqueGeometry(vtkViewport*) override;
  virtual int RenderOverlay(vtkViewport*) override;
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  virtual int HasTranslucentPolygonalGeometry() override;

protected:
  vtkLCMGLProp();
  virtual ~vtkLCMGLProp() override;

private:

  class vtkInternal;
  vtkInternal* Internal;

  vtkLCMGLProp(const vtkLCMGLProp&) =delete;
  void operator=(const vtkLCMGLProp&) =delete;
};

#endif
