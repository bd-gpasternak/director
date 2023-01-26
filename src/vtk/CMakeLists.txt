find_package(VTK
  COMPONENTS
    CommonCore
    CommonExecutionModel
)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

set(config_name directorvtk)
if(WIN32)
  set(config_dir cmake/${config_name})
else()
  set(config_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${config_name})
endif()

vtk_module_find_modules(vtk_module_files ${CMAKE_CURRENT_SOURCE_DIR})

vtk_module_scan(
  MODULE_FILES        ${vtk_module_files}
  REQUEST_MODULES     DIRECTOR::DRCFilters
  PROVIDES_MODULES    my_modules
  ENABLE_TESTS        "${BUILD_TESTING}")

vtk_module_build(
  MODULES             ${my_modules}
  INSTALL_EXPORT      DIRECTOR
  CMAKE_DESTINATION   "${config_dir}"
  VERSION             "${DIRECTOR_VERSION}"
  SOVERSION           "1")

if(VTK_WRAP_PYTHON)
  vtk_module_wrap_python(
    MODULES             ${my_modules}
    INSTALL_EXPORT      DIRECTOR
    PYTHON_PACKAGE      "director"
    CMAKE_DESTINATION   "${config_dir}"
    BUILD_STATIC        OFF)
endif()
