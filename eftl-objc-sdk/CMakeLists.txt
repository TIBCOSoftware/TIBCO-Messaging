if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Classes/version.h.config
                   ${CMAKE_CURRENT_BINARY_DIR}/version.h)
    file(GLOB_RECURSE eftl_objc_sources *.c *.h *.m)
    if(CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG")
        set(IOS_BUILD_TYPE Debug)
        file(MAKE_DIRECTORY
                ${CMAKE_CURRENT_BINARY_DIR}/Debug-iphoneos
                ${CMAKE_CURRENT_BINARY_DIR}/Debug-iphonesimulator
                ${CMAKE_CURRENT_BINARY_DIR}/Debug-iphoneuniversal)
    else()
        set(IOS_BUILD_TYPE Release)
        file(MAKE_DIRECTORY
                ${CMAKE_CURRENT_BINARY_DIR}/Release-iphoneos
                ${CMAKE_CURRENT_BINARY_DIR}/Release-iphonesimulator
                ${CMAKE_CURRENT_BINARY_DIR}/Release-iphoneuniversal)
    endif()

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/eftl.xcodeproj DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Classes DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Headers DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
    add_custom_command(OUTPUT ${IOS_BUILD_TYPE}-iphoneos/libeftl.a
                        COMMAND xcodebuild 
                            -project ${CMAKE_CURRENT_BINARY_DIR}/eftl.xcodeproj
                            -target eftl-iOS
                            -configuration ${IOS_BUILD_TYPE}
                            SYMROOT=${CMAKE_CURRENT_BINARY_DIR}
                            DSTROOT=${CMAKE_CURRENT_BINARY_DIR}
                            build
                        DEPENDS ${eftl_objc_sources})
    add_custom_target(eftl_lib_objc_eftl ALL DEPENDS ${IOS_BUILD_TYPE}-iphoneos/libeftl.a)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${IOS_BUILD_TYPE}-iphoneuniversal/eFTL.framework
            COMPONENT runtime
            DESTINATION sdks/ios)

# Required for eFTL mac package
#if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${IOS_BUILD_TYPE}-iphoneuniversal/eFTL.framework
#            COMPONENT runtime
#            DESTINATION eftl/Library/Frameworks)

#    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${IOS_BUILD_TYPE}-iphoneuniversal/eFTL.framework
#            COMPONENT runtime
#            DESTINATION eftl/sdks/ios)
#endif()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mainpage.dox.config ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox)

    set(TIB_DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/doc/eftl/html)
    set(TIB_DOXYGEN_SRC_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    file(MAKE_DIRECTORY ${TIB_DOXYGEN_OUTPUT_DIRECTORY}/c)
    set(TIB_DOXYGEN_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Headers ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox")
    set(TIB_DOXYGEN_IS_PUBLIC_API YES)
    set(TIB_DOXYGEN_API_NAME "eFTL&trade;")
    configure_file(${common_SOURCE_DIR}/btools/doxygen/doxygen.config ${CMAKE_CURRENT_BINARY_DIR}/eftl/eftl_doxygen.config)
    add_custom_command(OUTPUT eftl_doc
                        COMMAND ${DOXYGEN} ${CMAKE_CURRENT_BINARY_DIR}/eftl/eftl_doxygen.config)
    install(DIRECTORY ${TIB_DOXYGEN_OUTPUT_DIRECTORY} DESTINATION doc/eftl COMPONENT documentation)

#if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # required for mac package
#    install(DIRECTORY ${TIB_DOXYGEN_OUTPUT_DIRECTORY} DESTINATION eftl/doc/eftl COMPONENT documentation)
#endif()

    add_custom_target(header_eftl_doc DEPENDS eftl_doc)
    add_dependencies(doc header_eftl_doc)

endif()
