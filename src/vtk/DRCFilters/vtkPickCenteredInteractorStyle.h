/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPickCenteredInteractorStyle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPickCenteredInteractorStyle - manipulate camera in scene with natural view up (e.g., terrain)
// .SECTION Description
// vtkPickCenteredInteractorStyle is based on the vtkInteractorStyleTerrain
// but has been modified to orbit and dolly about a custom pick point.

// .SECTION See Also
// vtkInteractorObserver vtkInteractorStyle vtk3DWidget

#ifndef __vtkPickCenteredInteractorStyle_h
#define __vtkPickCenteredInteractorStyle_h

#include "vtkInteractorStyle.h"

#include <vtkDRCFiltersModule.h>
#include <map>


class VTKDRCFILTERS_EXPORT vtkPickCenteredInteractorStyle : public vtkInteractorStyle
{
public:

  static vtkPickCenteredInteractorStyle *New();

  vtkTypeMacro(vtkPickCenteredInteractorStyle,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove() override;
  virtual void OnLeftButtonDown() override;
  virtual void OnLeftButtonUp() override;
  virtual void OnMiddleButtonDown() override;
  virtual void OnMiddleButtonUp() override;
  virtual void OnRightButtonDown() override;
  virtual void OnRightButtonUp() override;
  virtual void OnMouseWheelForward() override;
  virtual void OnMouseWheelBackward() override;
  virtual void OnChar() override;

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion.
  virtual void Rotate() override;
  virtual void Pan() override;
  virtual void Dolly() override;

  // Dolly by the given value.  See vtkCamera::Dolly().
  void Dolly(double value);

  void SetMouseInteraction(int button, int interactionMode);
  void SetMouseShiftInteraction(int button, int interactionMode);

  vtkGetMacro(RotationFactor, double);
  vtkSetMacro(RotationFactor, double);

  vtkGetMacro(ZoomFactor, double);
  vtkSetMacro(ZoomFactor, double);

  vtkSetVector3Macro(CustomCenterOfRotation, double);
  vtkGetVector3Macro(CustomCenterOfRotation, double);

  double ComputeScale(const double position[3], vtkRenderer *renderer);

protected:
  vtkPickCenteredInteractorStyle();
  virtual ~vtkPickCenteredInteractorStyle() override;

  void OnMouseButtonDown(int button);
  void OnMouseButtonUp(int button);

  bool ValidateButtonInteraction(int button, int interactionMode);

  double RotationFactor;
  double ZoomFactor;
  double CustomCenterOfRotation[3];

  std::map<int, int> MouseInteractionMap;
  std::map<int, int> MouseShiftInteractionMap;

private:
  vtkPickCenteredInteractorStyle(const vtkPickCenteredInteractorStyle&)
    =delete;
  void operator=(const vtkPickCenteredInteractorStyle&)
    =delete;

};

#endif

