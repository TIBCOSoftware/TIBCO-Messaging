include(${CMAKE_SOURCE_DIR}/version.cmake)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.config
               ${CMAKE_CURRENT_BINARY_DIR}/fixed/src/version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/eftl.h.config
               ${CMAKE_CURRENT_BINARY_DIR}/fixed/src/eftl.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/tibeftl.dll.rc.config
               ${CMAKE_CURRENT_BINARY_DIR}/fixed/src/tibeftl.dll.rc)

file(COPY ${CMAKE_CURRENT_BINARY_DIR}/fixed/src/eftl.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/src/tib)

file(GLOB_RECURSE tibeftl_c_files src/*.c)

list(APPEND tibeftl_c_files ${CMAKE_CURRENT_BINARY_DIR}/fixed/src/tibeftl.dll.rc)

include_directories(BEFORE ${OPENSSL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/fixed/src)

add_definitions(-DCJSON_HIDE_SYMBOLS -UCJSON_API_VISIBILITY)

add_library(tibeftl SHARED ${tibeftl_c_files})

set_target_properties(tibeftl PROPERTIES
            VERSION   1.${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}.${EFTL_VERSION_UPDATE}
            SOVERSION 1)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" AND NOT CMAKE_SYSTEM STREQUAL "Darwin")
    target_link_libraries(tibeftl
        PRIVATE pthread
        PRIVATE m
        PRIVATE ${WINSOCK_LIBRARIES}
        PRIVATE ${OPENSSL_LIBRARIES})
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(tibeftl
        PRIVATE ${WINSOCK_LIBRARIES}
        PRIVATE ${OPENSSL_LIBRARIES})
endif ()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fixed/src/eftl.h DESTINATION include/tib COMPONENT development)

install(TARGETS tibeftl COMPONENT runtime EXPORT tibco-dev
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${TIB_LIB64}
    ARCHIVE DESTINATION ${TIB_LIB64} COMPONENT development)

# Install source into samples directory.
file(GLOB_RECURSE eftl_c_client_sources fixed/src/*.c)
fix_directory(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/fixed ${eftl_c_client_sources})
get_fix_target_name(fixed_dir_target ${CMAKE_CURRENT_BINARY_DIR}/fixed)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/fixed/src
        DESTINATION samples/c
        COMPONENT development
        PATTERN "*config*" EXCLUDE)
else()
    install(DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/fixed/src
        DESTINATION samples/c
        COMPONENT development
        PATTERN "*config*" EXCLUDE)
endif()

# doxygen stuff.
set(TIB_DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/doc/eftl/html)
set(TIB_DOXYGEN_SRC_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(TIB_DOXYGEN_BUILD_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${TIB_DOXYGEN_OUTPUT_DIRECTORY})
set(TIB_DOXYGEN_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/tib/eftl.h")
set(TIB_DOXYGEN_IS_PUBLIC_API YES)
set(TIB_DOXYGEN_API_NAME "eFTL&trade;")
set(TIB_DOXYGEN_HTML_OUTPUT c)
set(TIB_OPTIMIZE_FOR_C YES)
set(TIB_DOXYGEN_SHOW_USED_FILES YES)
set(TIB_DOXYGEN_SHOW_FILES YES)

configure_file(${build_common_SOURCE_DIR}/btools/doxygen/eftl_doxygen.config ${CMAKE_CURRENT_BINARY_DIR}/eftl_doxygen.config)
add_custom_command(OUTPUT eftl_doc
                    COMMAND ${DOXYGEN} ${CMAKE_CURRENT_BINARY_DIR}/eftl_doxygen.config)
install(DIRECTORY ${TIB_DOXYGEN_OUTPUT_DIRECTORY} DESTINATION doc/eftl COMPONENT documentation)

add_custom_target(header_public_doc DEPENDS eftl_doc)
add_dependencies(doc header_public_doc)
