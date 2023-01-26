find_package(OpenGL REQUIRED)
find_package(Eigen REQUIRED)
include_directories(${EIGEN_INCLUDE_DIRS})

set(classes
  vtkDepthImageUtils
  vtkGridSource
  vtkFrameWidget
  vtkFrameWidgetRepresentation
  vtkInteractorStyleTerrain2
  vtkPickCenteredInteractorStyle
)

vtk_module_add_module(DIRECTOR::DRCFilters
  CLASSES ${classes})
