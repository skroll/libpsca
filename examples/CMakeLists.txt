set (PSCA_EXAMPLES_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include_directories (${PROJECT_SOURCE_DIR}/lib)
include_directories (${PROJECT_BINARY_DIR})

set (PSCA_AUTO_SOURCES ${PSCA_EXAMPLES_ROOT}/auto.c)

add_executable (psca_auto ${PSCA_AUTO_SOURCES})
add_dependencies (psca_auto psca)
target_link_libraries (psca_auto psca)

