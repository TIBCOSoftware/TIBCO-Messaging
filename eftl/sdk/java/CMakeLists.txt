configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/com/tibco/eftl/Version.java.config
               ${CMAKE_CURRENT_BINARY_DIR}/eftl_config/com/tibco/eftl/Version.java)

file(GLOB_RECURSE eftl_client_java_sources src/*.java ${CMAKE_CURRENT_BINARY_DIR}/eftl_config/*.java)
set(eftl_client_java_sources ${eftl_client_java_sources} PARENT_SCOPE)

set(CMAKE_JAVA_COMPILE_FLAGS -source 1.8 -target 1.8)
add_jar(eftl_client_java
        SOURCES ${eftl_client_java_sources}
        OUTPUT_NAME tibeftl)
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    install_jar(eftl_client_java lib)
endif()

#if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#  install_jar(eftl_client_java eftl/lib)
#  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tibeftl.jar DESTINATION eftl/lib COMPONENT development)
#endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tibeftl.jar
            DESTINATION sdks/java/lib COMPONENT runtime)

# # javadoc stuff.
set(JAVA_PACKAGE_NAMES com.tibco.eftl)
set(JAVADOC_OUTPUT_DIR ${CMAKE_BINARY_DIR}/doc/eftl/html/java)

set(JAVADOC_SOURCE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(TO_NATIVE_PATH "${JAVADOC_SOURCE_DIRS}"
        JAVADOC_SOURCE_PATH)
if(NOT WIN32)
    string(REGEX REPLACE ";" ":" JAVADOC_SOURCE_PATH "${JAVADOC_SOURCE_PATH}")
endif()

set(JAVADOC_CLASS_DIRS ${CMAKE_CURRENT_BINARY_DIR}/tibeftl.jar)
file(TO_NATIVE_PATH "${JAVADOC_CLASS_DIRS}"
        JAVADOC_CLASS_PATH)
if(NOT WIN32)
    string(REGEX REPLACE ";" ":" JAVADOC_CLASS_PATH "${JAVADOC_CLASS_PATH}")
endif()

file(MAKE_DIRECTORY ${JAVADOC_OUTPUT_DIR})
set(TIB_CLIENT_API_NAME "eFTL&trade;")
set(TIB_CLIENT_NAME_VERSION "${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}.${EFTL_VERSION_UPDATE}")
configure_file(${build_common_SOURCE_DIR}/btools/javadoc/overview.html.config ${CMAKE_BINARY_DIR}/eftl/overview.html)
add_custom_command(OUTPUT hydra_doc
                    COMMAND ${Java_JAVADOC_EXECUTABLE} VERBATIM
                    -d ${JAVADOC_OUTPUT_DIR}
                    -sourcepath "${JAVADOC_SOURCE_PATH}"
                    -classpath "${JAVADOC_CLASS_PATH}"
                    -overview ${CMAKE_BINARY_DIR}/eftl/overview.html
                    -doctitle "TIBCO eFTL&trade; ${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}.${EFTL_VERSION_UPDATE} ${TIB_VERSION_RELEASE_TYPE}"
                    -header "TIBCO eFTL&trade; ${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}.${EFTL_VERSION_UPDATE} ${TIB_VERSION_RELEASE_TYPE}"
                    -footer "<small>Copyright &copy; ${TIB_COPYRIGHT_YEARS} Cloud Software Group, Inc., All rights reserved.</small>"
                    -public -quiet -notimestamp
                    ${JAVA_PACKAGE_NAMES})

install(DIRECTORY ${JAVADOC_OUTPUT_DIR} DESTINATION doc/eftl/html COMPONENT documentation)

add_custom_target(eftl_client_java_public_doc DEPENDS hydra_doc)
add_dependencies(doc eftl_client_java_public_doc)
