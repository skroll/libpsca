project (psca)
cmake_minimum_required (VERSION 2.8.0 FATAL_ERROR)

set (PSCA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set (PSCA_VERSION_MAJOR 0)
set (PSCA_VERSION_MINOR 0)
set (PSCA_VERSION_PATCH 1)
set (PSCA_VERSION_STRING "${PSCA_VERSION_MAJOR}.${PSCA_VERSION_MINOR}.${PSCA_VERSION_PATCH}")

# Build documentation
find_package (Doxygen)

if (DOXYGEN_FOUND)
  set (PSCA_DOXYGEN_IN "${PSCA_ROOT}/doxy.config.in")
  set (PSCA_DOXYGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/doxy.config")

  configure_file (${PSCA_DOXYGEN_IN} ${PSCA_DOXYGEN_OUT})
  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${PSCA_DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif (DOXYGEN_FOUND)

add_subdirectory (lib)
add_subdirectory (examples)

