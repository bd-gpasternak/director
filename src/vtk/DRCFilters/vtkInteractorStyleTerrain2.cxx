/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTerrain2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleTerrain2.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkInteractorStyleTerrain2);

//----------------------------------------------------------------------------
vtkInteractorStyleTerrain2::vtkInteractorStyleTerrain2()
{
  this->RotationFactor = 10.0;
  this->ZoomFactor = 10.0;

  this->SetMouseInteraction(vtkCommand::LeftButtonPressEvent, VTKIS_ROTATE);
  this->SetMouseInteraction(vtkCommand::MiddleButtonPressEvent, VTKIS_PAN);
  this->SetMouseInteraction(vtkCommand::RightButtonPressEvent, VTKIS_DOLLY);
  this->SetMouseShiftInteraction(vtkCommand::LeftButtonPressEvent, VTKIS_PAN);
  this->SetMouseShiftInteraction(vtkCommand::MiddleButtonPressEvent, VTKIS_PAN);
  this->SetMouseShiftInteraction(vtkCommand::RightButtonPressEvent, VTKIS_DOLLY);
}

//----------------------------------------------------------------------------
vtkInteractorStyleTerrain2::~vtkInteractorStyleTerrain2()
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::OnMouseMove() 
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
void vtkInteractorStyleTerrain2::SetMouseInteraction(int button, int interactionMode)
{
  if (this->ValidateButtonInteraction(button, interactionMode))
    {
    this->MouseInteractionMap[button] = interactionMode;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::SetMouseShiftInteraction(int button, int interactionMode)
{
  if (this->ValidateButtonInteraction(button, interactionMode))
    {
    this->MouseShiftInteractionMap[button] = interactionMode;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkInteractorStyleTerrain2::ValidateButtonInteraction(int button, int interactionMode)
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
void vtkInteractorStyleTerrain2::OnLeftButtonDown()
{
  this->OnMouseButtonDown(vtkCommand::LeftButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::OnLeftButtonUp()
{
  this->OnMouseButtonUp(vtkCommand::LeftButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::OnMiddleButtonDown()
{
  this->OnMouseButtonDown(vtkCommand::MiddleButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::OnMiddleButtonUp()
{
  this->OnMouseButtonUp(vtkCommand::MiddleButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::OnRightButtonDown()
{
  this->OnMouseButtonDown(vtkCommand::RightButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::OnRightButtonUp()
{
  this->OnMouseButtonUp(vtkCommand::RightButtonPressEvent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::OnMouseButtonDown(int button)
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
void vtkInteractorStyleTerrain2::OnMouseButtonUp(int button)
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
void vtkInteractorStyleTerrain2::OnMouseWheelForward()
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
void vtkInteractorStyleTerrain2::OnMouseWheelBackward()
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
void vtkInteractorStyleTerrain2::Rotate()
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

  // Move the camera. 
  // Make sure that we don't hit the north pole singularity.

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->Azimuth( a );

  double dop[3], vup[3];

  camera->GetDirectionOfProjection( dop );
  vtkMath::Normalize( dop );
  camera->GetViewUp( vup );
  vtkMath::Normalize( vup );

  double angle = vtkMath::DegreesFromRadians( acos(vtkMath::Dot( dop, vup) ) );
  //printf("current angle: %.2f.  elvation delta: %.2f\n", angle, e);

  if ( ( angle + e ) > 177.0) {
    e = 177.0 - angle;
  }
  else if ( (angle + e < 3.0)) {
    e = 3.0 - angle;
  }

  camera->Elevation( e );

  if ( this->AutoAdjustCameraClippingRange )
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::Pan()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  // Get the vector of motion

  double fp[3], focalPoint[3], pos[3], v[3], p1[4], p2[4];

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetPosition( pos );
  camera->GetFocalPoint( fp );

  this->ComputeWorldToDisplay(fp[0], fp[1], fp[2], 
                              focalPoint);


  int eventPos[2] = {rwi->GetEventPosition()[0], rwi->GetEventPosition()[1]};
  int lastEventPos[2] = {rwi->GetLastEventPosition()[0], rwi->GetLastEventPosition()[1]};


  if (rwi->GetControlKey())
    {
    int mouseDelta[2] = {eventPos[0] - lastEventPos[0], eventPos[1] - lastEventPos[1]};
    if(abs( mouseDelta[0] ) >= abs( mouseDelta[1] ))
      {
      eventPos[1] = lastEventPos[1];
      }
    else
      {
      eventPos[0] = lastEventPos[0];
      }
    }

  this->ComputeDisplayToWorld(eventPos[0], eventPos[1],
                              focalPoint[2],
                              p1);

  this->ComputeDisplayToWorld(lastEventPos[0], lastEventPos[1],
                              focalPoint[2],
                              p2);

  for (int i=0; i<3; i++)
    {
    v[i] = p2[i] - p1[i];
    pos[i] += v[i];
    fp[i] += v[i];
    }

  camera->SetPosition( pos );
  camera->SetFocalPoint( fp );

  if (rwi->GetLightFollowCamera()) 
    {
    this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::Dolly()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  double *center = this->CurrentRenderer->GetCenter();

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  double dyf = this->ZoomFactor * dy / center[1];
  this->Dolly(pow(1.1, dyf));
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTerrain2::Dolly(double value)
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  vtkRenderWindowInteractor *rwi = this->Interactor;

  if (camera->GetParallelProjection())
    {
    camera->SetParallelScale(camera->GetParallelScale() / value);
    }
  else
    {
    camera->Dolly(value);
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
void vtkInteractorStyleTerrain2::OnChar()
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
void vtkInteractorStyleTerrain2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Rotation Factor: " << this->RotationFactor << "\n";
  os << indent << "Zoom Factor: " << this->ZoomFactor << "\n";
}
