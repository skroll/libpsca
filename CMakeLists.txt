project (psca)
cmake_minimum_required (VERSION 2.8.0 FATAL_ERROR)

set (PSCA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set (PSCA_VERSION_MAJOR 0)
set (PSCA_VERSION_MINOR 0)
set (PSCA_VERSION_PATCH 1)
set (PSCA_VERSION_STRING "${PSCA_VERSION_MAJOR}.${PSCA_VERSION_MINOR}.${PSCA_VERSION_PATCH}")

add_subdirectory (lib)
add_subdirectory (examples)

