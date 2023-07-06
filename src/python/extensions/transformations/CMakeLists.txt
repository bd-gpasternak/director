include_directories(${PYTHON_INCLUDE_DIR})
# include_directories(${PYTHON_LIBRARY})

find_package (Python COMPONENTS NumPy)
find_package (PythonLibs REQUIRED)

if (NOT Python_NumPy_FOUND)
  message(WARNING "Optimized transformations C extension module is disabled because numpy is not found.")
  return()
endif()

if (Python_NumPy_VERSION VERSION_LESS 1.7)
  message(WARNING "Optimized transformations C extension module is disabled because numpy version < 1.7 was found.")
  return()
endif()


include_directories(${Python_NumPy_INCLUDE_DIRS})

add_library(_transformations SHARED transformations.c)
target_link_libraries(_transformations ${PYTHON_LIBRARIES})
if(NOT WIN32)
  find_library(M_LIBRARY NAMES m)
  if(M_LIBRARY)
    target_link_libraries(_transformations ${M_LIBRARY})
  endif()
endif()

if(WIN32 AND NOT CYGWIN)
  set_target_properties(_transformations PROPERTIES SUFFIX ".pyd")
else()
  set_target_properties(_transformations PROPERTIES PREFIX "" SUFFIX ".so")
endif()

set_target_properties(_transformations PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${DD_INSTALL_PYTHON_DIR}/director/thirdparty")
install(TARGETS _transformations DESTINATION ${DD_INSTALL_PYTHON_DIR}/director/thirdparty)
