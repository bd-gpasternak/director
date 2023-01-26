/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFrameWidgetRepresentation - An orthonormal frame representation
// .SECTION Description
// An orthonormal frame representation for use with the vtkFrameWidget

// .SECTION See Also
// vtkFrameWidget vtkFrameWidget


#ifndef __vtkFrameWidgetRepresentation_h
#define __vtkFrameWidgetRepresentation_h

#include "vtkWidgetRepresentation.h"

#include <vtkDRCFiltersModule.h>

class vtkTransform;
class vtkDataSet;
class vtkActor;

class VTKDRCFILTERS_EXPORT vtkFrameWidgetRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkFrameWidgetRepresentation *New();

  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkFrameWidgetRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Return the transform describing the frame.  Changes to this transform
  // will be reflected in the rendered axes.
  vtkTransform* GetTransform();

  // Description:
  // Set the transform describing the frame.  Note that this method does not
  // copy the transform, rather it increments the transforms reference count
  // and uses it directly.  As a side effect of using the transform directly
  // it will set PostMultiply to true on the transform.  This method calls
  // vtkProp3D::SetUserTransform(t) on its internal vtkAxesActor.
  virtual void SetTransform(vtkTransform* t);

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void BuildRepresentation() override;
  virtual int ComputeInteractionState(int X, int Y, int modify=0)
    override;
  virtual void StartWidgetInteraction(double e[2]) override;
  virtual void WidgetInteraction(double e[2]) override;

  virtual bool OnMouseHover(double e[2]);
  virtual bool HighlightActor(vtkDataSet* dataset);

  virtual double *GetBounds() override;

  virtual void GetActors(vtkPropCollection* propCollection)
    override;

  void SetTranslateAxisEnabled(int axisId, bool enabled);
  void SetRotateAxisEnabled(int axisId, bool enabled);

  void SetRotationActor(int axisId, vtkActor* actor);

  // Description:
  // Methods supporting, and required by, the rendering process.
  virtual void ReleaseGraphicsResources(vtkWindow*) override;
  virtual int RenderOpaqueGeometry(vtkViewport*) override;
  virtual int RenderOverlay(vtkViewport*) override;
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*)
    override;
  virtual int HasTranslucentPolygonalGeometry() override;

  // Description:
  // The interaction state may be set from a widget (e.g., vtkBoxWidget2) or
  // other object. This controls how the interaction with the widget
  // proceeds. Normally this method is used as part of a handshaking
  // process with the widget: First ComputeInteractionState() is invoked that
  // returns a state based on geometric considerations (i.e., cursor near a
  // widget feature), then based on events, the widget may modify this
  // further.
  void SetInteractionState(int state);

  // Description:
  // Set the size of the axes in world coordinates.
  vtkSetMacro(WorldSize, double);
  vtkGetMacro(WorldSize, double);

  // Description:
  // Set the pick tolerance
  vtkSetMacro(PickTolerance, double);
  vtkGetMacro(PickTolerance, double);

  // Description:
  // Use a tube filter instead of only drawing lines.
  vtkBooleanMacro(UseTubeFilter, bool);
  vtkSetMacro(UseTubeFilter, bool);
  vtkGetMacro(UseTubeFilter, bool);

  enum {Outside=0,Translating,TranslatingInPlane,Rotating};

protected:
  vtkFrameWidgetRepresentation();
  virtual ~vtkFrameWidgetRepresentation() override;

  bool UseTubeFilter;
  int TranslateAxis;
  int RotateAxis;
  double PickTolerance;
  double WorldSize;
  double LastEventPosition[2];
  double InteractionStartWorldPoint[3];



  virtual void Translate(double e[2]);
  virtual void TranslateInPlane(double e[2]);
  virtual void Rotate(double e[2]);

private:

  class vtkInternal;
  vtkInternal* Internal;

  vtkFrameWidgetRepresentation(const vtkFrameWidgetRepresentation&)
    =delete;
  void operator=(const vtkFrameWidgetRepresentation&)
    =delete;
};

#endif
