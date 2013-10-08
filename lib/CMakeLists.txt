set (PSCA_LIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Build options
set (PSCA_DEFAULT_BLOCK_SIZE "64 * 1024" CACHE STRING "Default block allocation size")
set (PSCA_BIG_ALLOC_MULTIPLIER "2" CACHE STRING "Mulitplier for block allocations greater than default")
set (PSCA_ALLOC_METHOD "MALLOC" CACHE STRING "Allocation method for blocks")

# If using a GUI, this will use a combo box for the PSCA_ALLOC_METHOD option
set_property (CACHE PSCA_ALLOC_METHOD PROPERTY STRINGS MALLOC MMAP NONE)

configure_file ("${PSCA_LIB_ROOT}/psca_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/psca_config.h")

# Build library
set (PSCA_SOURCES
  ${PSCA_LIB_ROOT}/psca.c)

set (PSCA_HEADERS
  ${PSCA_LIB_ROOT}/psca.h
  ${CMAKE_CURRENT_BINARY_DIR}/psca_config.h)

set_property (SOURCE ${PSCA_LIB_ROOT}/psca.c PROPERTY COMPILE_DEFINITIONS PSCA_USE_CONFIG_H)

include_directories (${CMAKE_CURRENT_BINARY_DIR})
add_library (psca STATIC ${PSCA_SOURCES} ${PSCA_HEADERS})

