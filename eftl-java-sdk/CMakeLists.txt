cmake_minimum_required(VERSION 2.8)
find_package(Java)
include(UseJava)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/com/tibco/eftl/Version.java.config
               ${CMAKE_CURRENT_BINARY_DIR}/eftl_config/com/tibco/eftl/Version.java)

file(GLOB_RECURSE eftl_client_java_sources src/*.java ${CMAKE_CURRENT_BINARY_DIR}/eftl_config/*.java)
set(eftl_client_java_sources ${eftl_client_java_sources} PARENT_SCOPE)

set(CMAKE_JAVA_COMPILE_FLAGS -source 1.6 -target 1.6)
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
