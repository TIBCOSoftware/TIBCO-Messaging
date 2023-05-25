configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure_js.cmake ${CMAKE_CURRENT_BINARY_DIR}/configure_js.cmake)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(COMMAND_STR "${TIB_THIRDPARTY}/all/${CMAKE_BUILD_TYPE_FIXED}/uglifyjs.cmd")
else()
    set(COMMAND_STR "${TIB_THIRDPARTY}/all/${CMAKE_BUILD_TYPE_FIXED}/node_modules/uglify-js/bin/uglifyjs")
endif()

# generate minified file
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eftl_minimized
    COMMAND ${CMAKE_COMMAND} -DEFTL_VERSION_MAJOR=${EFTL_VERSION_MAJOR}
                             -DEFTL_VERSION_MINOR=${EFTL_VERSION_MINOR}
                             -DEFTL_VERSION_UPDATE=${EFTL_VERSION_UPDATE}
                             -DEFTL_VERSION_RELEASE_TYPE=${EFTL_VERSION_RELEASE_TYPE}
                             -DEFTL_VERSION_BUILD=${EFTL_VERSION_BUILD}
                             -P ${CMAKE_CURRENT_BINARY_DIR}/configure_js.cmake
    COMMAND ${COMMAND_STR} ${CMAKE_CURRENT_BINARY_DIR}/eftl_v.js -o ${CMAKE_CURRENT_BINARY_DIR}/eftl_min_o.js -c --no-mangle
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/eftl_minimized
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/eftl.js ${CMAKE_CURRENT_SOURCE_DIR}/eftl_js.copyright
            ${CMAKE_CURRENT_SOURCE_DIR}/configure_js.cmake)

# copy the minified file as 'eftl.js'
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eftl.js
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/eftl_min_o.js
    ${CMAKE_CURRENT_BINARY_DIR}/eftl.js
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/eftl_minimized)

add_custom_target(jslibout ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/eftl.js)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/*.js)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eftl.js DESTINATION sdks/javascript/lib COMPONENT development)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/javascript DESTINATION doc/eftl/html COMPONENT documentation)

