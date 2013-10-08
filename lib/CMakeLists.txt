set (PSCA_LIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Build library
set (PSCA_SOURCES
  ${PSCA_LIB_ROOT}/psca.c
  ${PSCA_LIB_ROOT}/psca_malloc.c)


set (PSCA_HEADERS
  ${PSCA_LIB_ROOT}/psca.h)

include_directories (${CMAKE_CURRENT_BINARY_DIR})
add_library (psca STATIC ${PSCA_SOURCES} ${PSCA_HEADERS})

