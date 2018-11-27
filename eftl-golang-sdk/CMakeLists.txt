file(GLOB_RECURSE eftl_golang_client_sources *.go)
fix_directory(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/fixed ${eftl_golang_client_sources})
get_fix_target_name(fixed_dir_target ${CMAKE_CURRENT_BINARY_DIR}/fixed)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/tibco.com/eftl/eftl.go.config
               ${CMAKE_CURRENT_BINARY_DIR}/fixed/src/tibco.com/eftl/eftl.go)


install(DIRECTORY 
    ${CMAKE_CURRENT_BINARY_DIR}/fixed/src
    DESTINATION samples/golang 
    COMPONENT development
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "eftl.go.config" EXCLUDE)

install(FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt
        DESTINATION samples/golang/
        COMPONENT development)

