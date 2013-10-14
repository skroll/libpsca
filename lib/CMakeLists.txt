set (PSCA_LIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Build library
set (PSCA_SOURCES
  ${PSCA_LIB_ROOT}/psca.c)

set (PSCA_HEADERS
  ${PSCA_LIB_ROOT}/psca.h)

include_directories (${PROJECT_BINARY_DIR})

add_library (psca_static STATIC ${PSCA_SOURCES} ${PSCA_HEADERS})

set (PSCA_VERSION_IN "${PSCA_LIB_ROOT}/version.h.in")
set (PSCA_VERSION_OUT "${PROJECT_BINARY_DIR}/psca/version.h")

configure_file (${PSCA_VERSION_IN} ${PSCA_VERSION_OUT})

add_library (psca SHARED ${PSCA_SOURCES} ${PSCA_HEADERS})
set_target_properties (psca PROPERTIES
  VERSION ${PSCA_VERSION_STRING}
  SOVERSION ${PSCA_VERSION_MAJOR})

