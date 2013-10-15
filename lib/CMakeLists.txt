set (PSCA_LIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${PROJECT_BINARY_DIR})

# Generate version.h {{{
  set (PSCA_VERSION_IN "${PSCA_LIB_ROOT}/version.h.in")
  set (PSCA_VERSION_OUT "${PROJECT_BINARY_DIR}/psca/version.h")
  configure_file (${PSCA_VERSION_IN} ${PSCA_VERSION_OUT})
# }}}

# Library sources {{{
  set (PSCA_SOURCES ${PSCA_LIB_ROOT}/psca.c)
  set (PSCA_HEADERS ${PSCA_LIB_ROOT}/psca.h)
  set (PSCA_PSCA_HEADERS ${PSCA_VERSION_OUT} ${PSCA_EXPORT_HEADER})
# }}}

# Build static library {{{
  add_library (psca_static STATIC ${PSCA_SOURCES} ${PSCA_HEADERS} ${PSCA_PSCA_HEADERS})

  set_target_properties (psca_static
                         PROPERTIES
                         OUTPUT_NAME "psca")
# }}}

# Build shared library {{{
  add_library (psca SHARED ${PSCA_SOURCES} ${PSCA_HEADERS})

  set_target_properties (psca
                         PROPERTIES
                         VERSION       ${PSCA_VERSION_STRING}
                         SOVERSION     ${PSCA_VERSION_MAJOR})
# }}}

# Install targets {{{
  install (TARGETS psca psca_static
           RUNTIME DESTINATION bin
           LIBRARY DESTINATION lib
           ARCHIVE DESTINATION lib)

  install (FILES ${PSCA_HEADERS}
           DESTINATION include)

  install (FILES ${PSCA_PSCA_HEADERS}
           DESTINATION include/psca)
# }}}

