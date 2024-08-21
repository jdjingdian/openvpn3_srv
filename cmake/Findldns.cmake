find_path(LDNS_INCLUDE_DIR NAMES ldns/ldns.h)
find_library(LDNS_LIBRARY NAMES ldns)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
        ldns DEFAULT_MSG
        LDNS_LIBRARY LDNS_INCLUDE_DIR
)

if(LDNS_LIBRARY AND NOT TARGET ldns::ldns)
    add_library(ldns::ldns UNKNOWN IMPORTED)
    set_target_properties(ldns::ldns PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LDNS_INCLUDE_DIR}")
    set_target_properties(ldns::ldns PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${LDNS_LIBRARY}")
endif()

mark_as_advanced(LDNS_INCLUDE_DIR LDNS_LIBRARY)
