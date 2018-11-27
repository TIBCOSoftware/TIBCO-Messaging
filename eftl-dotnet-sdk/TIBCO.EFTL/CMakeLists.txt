if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(EFTL_CLASSLIB_FLAGS "/debug+")
else()
    SET(EFTL_CLASSLIB_FLAGS "/optimize+")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

## configure the version config specific files
file(STRINGS "versions.txt" version_list)
foreach(version ${version_list})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TIBCO.EFTL.${version}.config
        ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.${version}.config @ONLY)
endforeach(version)


set(EFTL_KEY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/key.snk")
set(EFTL_CLIENT_LIB "${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.dll")
set(EFTL_CLIENT_LIB "${EFTL_CLIENT_LIB}" PARENT_SCOPE)
set(EFTL_CLIENT_DOC "${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.xml")
set(EFTL_CLIENT_DOC "${EFTL_CLIENT_DOC}" PARENT_SCOPE)
set(EFTL_ASSEMBLY_VERSION "${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}.${EFTL_VERSION_UPDATE}.${EFTL_VERSION_BUILD}")
SET(EFTL_TRADE "TIB, TIBCO, TIBCO eFTL, and the TIBCO logo are either registered trademarks or trademarks of TIBCO Software Inc. in the United States and/or other countries.")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Properties/AssemblyInfo.cs.template ${CMAKE_CURRENT_BINARY_DIR}/AssemblyInfo.cs)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.cs.template ${CMAKE_CURRENT_BINARY_DIR}/Version.cs)

file(GLOB_RECURSE EFTL_cs_files_cmake *.cs ${CMAKE_CURRENT_BINARY_DIR}/*.cs)

# Do the C# build
foreach(EFTL_cs_file ${EFTL_cs_files_cmake})
    file(TO_NATIVE_PATH ${EFTL_cs_file} EFTL_cs_file_native)
    set(EFTL_cs_files_native ${EFTL_cs_files_native} ${EFTL_cs_file_native})
endforeach()

add_custom_command(OUTPUT ${EFTL_CLIENT_LIB}
    COMMAND csc /nowarn:1591 /doc:${EFTL_CLIENT_DOC} /nologo /platform:x64 /out:${EFTL_CLIENT_LIB} ${EFTL_CLASSLIB_FLAGS} /unsafe /target:library /keyfile:${EFTL_KEY_FILE} ${EFTL_cs_files_native} 
    DEPENDS ${EFTLclr_cs_files} ${EFTL_cs_files_cmake}
    ) 

SET(POL_COMP "TIBCO Software, Inc.")
SET(POL_COPY "Copyright (c) ${TIB_COPYRIGHT_YEARS} TIBCO Software Inc.  ALL RIGHTS RESERVED")
SET(POL_PROD "TIBCO EFTL")
SET(POL_TITLE "Publisher Policy for TIBCO EFTL client class library.")
SET(POL_VERSION "${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}.${EFTL_VERSION_UPDATE}.${EFTL_VERSION_BUILD}")


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.DLL ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.xml
        DESTINATION sdks/dotnet/bin COMPONENT runtime)

foreach(version ${version_list})
    install(FILES  ${CMAKE_CURRENT_BINARY_DIR}/policy.${version}.TIBCO.EFTL.dll ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.${version}.config
            DESTINATION sdks/dotnet/bin COMPONENT runtime)
endforeach(version)

#
# Build the policy dlls
# Anytime FTL is backward compatible and a new MAJOR.MINOR version is created,
# then the policy configuration files must be updated and previous version
# must be added.
#
foreach(version ${version_list})
    add_custom_command(OUTPUT policy.${version}.TIBCO.EFTL.dll
                    COMMAND al.exe /platform:x64 /nologo /comp:${POL_COMP} /copy:${POL_COPY} /prod:${POL_PROD} /productv:${version}.0.0 
                    /title:${POL_TITLE} /trade:${EFTL_TRADE} /link:TIBCO.EFTL.${version}.config /out:policy.${version}.TIBCO.EFTL.dll /keyfile:${EFTL_KEY_FILE} /version:${POL_VERSION})
    list(APPEND POLICY_FILE_LIST policy.${version}.TIBCO.EFTL.dll)
endforeach(version)

add_custom_target(CETUS_EFTL_classlib ALL DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/AssemblyInfo.cs
    ${CMAKE_CURRENT_BINARY_DIR}/Version.cs
    TIBCO.EFTL.dll)

add_custom_target(TIBCETUSEFTL_policy ALL DEPENDS ${POLICY_FILE_LIST})

add_library(TIBCO.FTL SHARED IMPORTED)

install(FILES ${EFTL_CLIENT_DOC} DESTINATION bin COMPONENT documentation)
