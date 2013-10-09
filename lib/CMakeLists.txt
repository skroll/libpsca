set (PSCA_LIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Build library
set (PSCA_SOURCES
  ${PSCA_LIB_ROOT}/psca.c
  ${PSCA_LIB_ROOT}/psca_malloc.c)

set (PSCA_HEADERS
  ${PSCA_LIB_ROOT}/psca.h)

include_directories (${CMAKE_CURRENT_BINARY_DIR})

add_library (psca_static STATIC ${PSCA_SOURCES} ${PSCA_HEADERS})

add_library (psca SHARED ${PSCA_SOURCES} ${PSCA_HEADERS})
set_target_properties (psca PROPERTIES
  VERSION ${PSCA_VERSION_STRING}
  SOVERSION ${PSCA_VERSION_MAJOR})

