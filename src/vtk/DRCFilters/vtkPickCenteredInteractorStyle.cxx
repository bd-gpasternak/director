/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPickCenteredInteractorStyle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPickCenteredInteractorStyle.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCallbackCommand.h"
#include "vtkExtractEdges.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkVectorOperators.h"


#include "vtkSmartPointer.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkPickCenteredInteractorStyle);

namespace {

void GetRightVandUpV(double *p, vtkCamera *cam, vtkRenderWindow* window,
                                               double *rightV, double *upV)
{
  int i;

  // Compute the horizontal & vertical scaling ('scalex' and 'scaley')
  // factors as function of the down point & camera params.
  double from[3];
  cam->GetPosition(from);

  // construct a vector from the viewing position to the picked point
  double vec[3];
  for(i=0; i<3; i++)
    {
    vec[i] = p[i] - from[i];
    }

  // Get shortest distance 'l' between the viewing position and
  // plane parallel to the projection plane that contains the 'DownPt'.
  double atV[4];
  cam->GetViewPlaneNormal(atV);
  vtkMath::Normalize(atV);
  double l = -vtkMath::Dot(vec, atV);

  double view_angle = cam->GetViewAngle() * vtkMath::Pi() / 180.0;
  double w = window->GetSize()[0];
  double h = window->GetSize()[1];
  double scalex = w/h*((2*l*tan(view_angle/2))/2);
  double scaley =     ((2*l*tan(view_angle/2))/2);

  // construct the camera offset vector as function of delta mouse X & Y.
  cam->GetViewUp(upV);
  vtkMath::Cross(upV, atV, rightV);
  vtkMath::Cross(atV, rightV, upV); // (make sure 'upV' is orthogonal
                                    //  to 'atV' & 'rightV')
  vtkMath::Normalize(rightV);
  vtkMath::Normalize(upV);

  for(i=0; i<3; i++)
    {
    rightV[i] = rightV[i] * scalex;
    upV   [i] = upV   [i] * scaley;
    }
}

void MoveCamera(vtkCamera* camera, double* offset)
{
  double p[3], f[3];
  camera->GetPosition(p);
  camera->GetFocalPoint(f);

  double newP[3], newF[3];
  for(int i=0;i<3;i++)
    {
    newP[i] = p[i] + offset[i];
    newF[i] = f[i] + offset[i];
    }

  camera->SetPosition(newP);
  camera->SetFocalPoint(newF);
}

}


//----------------------------------------------------------------------------
vtkPickCenteredInteractorStyle::vtkPickCenteredInteractorStyle()
{
  this->RotationFactor = 10.0;
  this->ZoomFactor = 10.0;
  this->CustomCenterOfRotation[0] = 0.0;
  this->CustomCenterOfRotation[1] = 0.0;
  this->CustomCenterOfRotation[2] = 0.0;

  this->SetMouseInteraction(vtkCommand::LeftButtonPressEvent, VTKIS_ROTATE);
  this->SetMouseInteraction(vtkCommand::MiddleButtonPressEvent, VTKIS_PAN);
  this->SetMouseInteraction(vtkCommand::RightButtonPressEvent, VTKIS_DOLLY);
  this->SetMouseShiftInteraction(vtkCommand::LeftButtonPressEvent, VTKIS_PAN);
  this->SetMouseShiftInteraction(vtkCommand::MiddleButtonPressEvent, VTKIS_PAN);
  this->SetMouseShiftInteraction(vtkCommand::RightButtonPressEvent, VTKIS_DOLLY);
}

//----------------------------------------------------------------------------
vtkPickCenteredInteractorStyle::~vtkPickCenteredInteractorStyle()
{

}

//----------------------------------------------------------------------------
double vtkPickCenteredInteractorStyle::ComputeScale(const double position[3], vtkRenderer *renderer)
{
  // Find the cursor scale factor such that 1 data unit length
  // equals 1 screen pixel at the cursor's distance from the camera.
  // Start by computing the height of the window at the cursor position.
  double worldHeight = 1.0;
  vtkCamera *camera = renderer->GetActiveCamera();
  if (camera->GetParallelProjection())
    {
    worldHeight = 2*camera->GetParallelScale();
    }
  else
    {
    vtkMatrix4x4 *matrix = camera->GetViewTransformMatrix();
    // Get a 3x3 matrix with the camera orientation
    double cvz[3];
    cvz[0] = matrix->GetElement(2, 0);
    cvz[1] = matrix->GetElement(2, 1);
    cvz[2] = matrix->GetElement(2, 2);

    double cameraPosition[3];
    camera->GetPosition(cameraPosition);

    double v[3];
    v[0] = cameraPosition[0] - position[0];
    v[1] = cameraPosition[1] - position[1];
    v[2] = cameraPosition[2] - position[2];

    worldHeight = 2*(vtkMath::Dot(v,cvz)
                     * tan(0.5*camera->GetViewAngle()/57.296));
    }

  // Compare world height to window height.
  int windowHeight = renderer->GetSize()[1];
  double scale = 1.0;
  if (windowHeight > 0)
    {
    scale = worldHeight/windowHeight;
    }

  return scale;
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::SetMouseInteraction(int button, int interactionMode)
{
  if (this->ValidateButtonInteraction(button, interactionMode))
    {
    this->MouseInteractionMap[button] = interactionMode;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::SetMouseShiftInteraction(int button, int interactionMode)
{
  if (this->ValidateButtonInteraction(button, interactionMode))
    {
    this->MouseShiftInteractionMap[button] = interactionMode;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkPickCenteredInteractorStyle::ValidateButtonInteraction(int button, int interactionMode)
{
  if (button != vtkCommand::LeftButtonPressEvent
      && button != vtkCommand::MiddleButtonPressEvent
      && button != vtkCommand::RightButtonPressEvent)
    {
    vtkErrorMacro("Unknown button:" << button);
    return false;
    }
  if (interactionMode != VTKIS_PAN
      && interactionMode != VTKIS_ROTATE
      && interactionMode != VTKIS_DOLLY)
  {
    vtkErrorMacro("Unknown interaction mode:" << interactionMode);
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnLeftButtonDown()
{
  this->OnMouseButtonDown(vtkCommand::LeftButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnLeftButtonUp()
{
  this->OnMouseButtonUp(vtkCommand::LeftButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnMiddleButtonDown()
{
  this->OnMouseButtonDown(vtkCommand::MiddleButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnMiddleButtonUp()
{
  this->OnMouseButtonUp(vtkCommand::MiddleButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnRightButtonDown()
{
  this->OnMouseButtonDown(vtkCommand::RightButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnRightButtonUp()
{
  this->OnMouseButtonUp(vtkCommand::RightButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnMouseButtonDown(int button)
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], 
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->GrabFocus(this->EventCallbackCommand);
  if (this->State != VTKIS_NONE)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  if (rwi->GetShiftKey())
    {
    this->StartState(this->MouseShiftInteractionMap[button]);
    }
  else
    {
    this->StartState(this->MouseInteractionMap[button]);
    }
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnMouseButtonUp(int button)
{
  if (this->State == this->MouseInteractionMap[button]
      || this->State == this->MouseShiftInteractionMap[button]) {
      this->StopState();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
    }
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State)
    {
    case VTKIS_ROTATE:
      this->FindPokedRenderer(x, y);
      this->Rotate();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_PAN:
      this->FindPokedRenderer(x, y);
      this->Pan();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::Rotate()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  int dx = - ( rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0] );
  int dy = - ( rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1] );
  int *size = this->CurrentRenderer->GetRenderWindow()->GetSize();
  double a = this->RotationFactor * 18.0 * dx / static_cast<double>(size[0]);
  double e = this->RotationFactor * 18.0 * dy / static_cast<double>(size[1]);
  
  if (rwi->GetControlKey()) 
    {
    if(abs( dx ) >= abs( dy ))
      {
      e = 0.0;
      }
    else
      {
      a = 0.0;
      }
    }

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();


  double dop[3], vup[3];

  camera->GetDirectionOfProjection( dop );
  vtkMath::Normalize( dop );
  camera->GetViewUp( vup );
  vtkMath::Normalize( vup );

  double angle = vtkMath::DegreesFromRadians( acos(vtkMath::Dot( dop, vup) ) );
  //printf("current angle: %.2f.  elvation delta: %.2f\n", angle, e);

  // Clip elevation angle to ensure we don't hit the north pole singularity.
  if ( ( angle + e ) > 177.0 ||
       ( angle + e ) < 3.0 )
    {
    e = 0.0;
    }


  double *focalPoint = camera->GetFocalPoint();
  double *viewUp = camera->GetViewUp();
  double *position = camera->GetPosition();
  double axis[3];
  axis[0] = -camera->GetViewTransformMatrix()->GetElement(0,0);
  axis[1] = -camera->GetViewTransformMatrix()->GetElement(0,1);
  axis[2] = -camera->GetViewTransformMatrix()->GetElement(0,2);

  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->Identity();
  transform->Translate(this->CustomCenterOfRotation[0], this->CustomCenterOfRotation[1], this->CustomCenterOfRotation[2]);
  transform->RotateWXYZ(a, viewUp); // Azimuth
  transform->RotateWXYZ(e, axis);   // Elevation
  transform->Translate(-this->CustomCenterOfRotation[0], -this->CustomCenterOfRotation[1], -this->CustomCenterOfRotation[2]);

  double newPosition[3];
  transform->TransformPoint(position,newPosition); // Transform Position
  double newFocalPoint[3];
  transform->TransformPoint(focalPoint, newFocalPoint); // Transform Focal Point

  camera->SetPosition(newPosition);
  camera->SetFocalPoint(newFocalPoint);

  if ( this->AutoAdjustCameraClippingRange )
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }

  rwi->Render();
}


//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::Pan()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();


  double w = rwi->GetRenderWindow()->GetSize()[0];
  double h = rwi->GetRenderWindow()->GetSize()[1];

  int dx = rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0];
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];

  double dxf = 2.0 * dx / w;
  double dyf = 2.0 * dy / h;

  double rightV[3], upV[3];
  GetRightVandUpV(this->CustomCenterOfRotation, camera, rwi->GetRenderWindow(),
                        rightV, upV);

  double offset[3];
  for(int i=0; i<3; i++)
    {
    offset[i] = (-dxf * rightV[i] +
                 -dyf * upV   [i]);
    }

  MoveCamera(camera, offset);

  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }


  if (rwi->GetLightFollowCamera()) 
    {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::Dolly()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  double h = rwi->GetRenderWindow()->GetSize()[1];
  double dyf = this->ZoomFactor * 2.0 * dy / h;
  this->Dolly(pow(1.1, dyf));
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::Dolly(double value)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();

  if (camera->GetParallelProjection())
    {
      camera->SetParallelScale(camera->GetParallelScale() / value);
    }
  else
    {
    double from[3];
    camera->GetPosition(from);

    double movec[3];
    for(int i=0; i<3; i++)
      {
      movec[i] = this->CustomCenterOfRotation[i] - from[i];
      }

    double offset[3];
    for(int i=0; i<3; i++)
      {
      offset[i] = movec[i] * (value - 1);
      }

    MoveCamera(camera, offset);

    // Update focal point using projection of center of rotation to view direction
    double viewDirection[3];
    camera->GetDirectionOfProjection(viewDirection);

    vtkVector3d linePoint1(camera->GetPosition());
    vtkVector3d linePoint2(camera->GetFocalPoint());
    vtkVector3d lineVector = linePoint2 - linePoint1;
    vtkVector3d pt(this->CustomCenterOfRotation);

    double pcoord =  vtkVector3d(pt - linePoint1).Dot(lineVector) / lineVector.Dot(lineVector);
    vtkVector3d projectedFocalPoint = linePoint1 + (lineVector * pcoord);
    camera->SetFocalPoint(projectedFocalPoint.GetData());

    if (this->AutoAdjustCameraClippingRange)
      {
      this->CurrentRenderer->ResetCameraClippingRange();
      }
    }

  if (rwi->GetLightFollowCamera()) 
    {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
  
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnMouseWheelForward()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
  double factor = this->ZoomFactor * 0.2 * this->MouseWheelMotionFactor;
  this->Dolly(pow(1.1, factor));
  this->EndDolly();
  this->ReleaseFocus();
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnMouseWheelBackward()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
  double factor = this->ZoomFactor * -0.2 * this->MouseWheelMotionFactor;
  this->Dolly(pow(1.1, factor));
  this->EndDolly();
  this->ReleaseFocus();
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::OnChar()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (rwi->GetKeyCode())
    {
    default:
      this->Superclass::OnChar();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkPickCenteredInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Rotation Factor: " << this->RotationFactor << "\n";
  os << indent << "Zoom Factor: " << this->ZoomFactor << "\n";
}
