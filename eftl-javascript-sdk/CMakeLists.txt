cmake_minimum_required(VERSION 2.8)

# Replace version tokens
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eftl.js
               ${CMAKE_CURRENT_BINARY_DIR}/eftl_v.js)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/eftl_js.copyright
               ${CMAKE_CURRENT_BINARY_DIR}/eftl_js.copyright)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(COMMAND_STR "uglifyjs.cmd")
else()
    set(COMMAND_STR "uglifyjs")
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eftl_minimized
  COMMAND ${COMMAND_STR} ${CMAKE_CURRENT_BINARY_DIR}/eftl_v.js -o ${CMAKE_CURRENT_BINARY_DIR}/eftl_min_o.js -c --no-mangle
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/eftl_minimized)

# copy the minified file as 'eftl.js'
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eftl.js
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/eftl_min_o.js
    ${CMAKE_CURRENT_BINARY_DIR}/eftl.js
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/eftl_minimized)

add_custom_target(jslibout ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/eftl.js)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/*.js)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eftl.js DESTINATION sdks/javascript/lib COMPONENT development)

