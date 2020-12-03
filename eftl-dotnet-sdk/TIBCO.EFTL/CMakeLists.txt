if (CMAKE_SYSTEM_NAME STREQUAL "Windows")

## configure the version config specific files
file(STRINGS "versions.txt" version_list)
foreach(version ${version_list})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TIBCO.EFTL.${version}.config
        ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.${version}.config @ONLY)
endforeach(version)

endif()


set(EFTL_KEY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/key.snk")
set(EFTL_CLIENT_LIB "${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.dll")
set(EFTL_CLIENT_NETSTANDARD_LIB "${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/netstandard2.0/TIBCO.EFTL.dll")
set(EFTL_CLIENT_LIB "${EFTL_CLIENT_LIB}" PARENT_SCOPE)
set(EFTL_CLIENT_NETSTANDARD_DOC "${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/netstandard2.0/TIBCO.EFTL.xml")
set(EFTL_CLIENT_DOC "${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.xml")
set(EFTL_CLIENT_DOC "${EFTL_CLIENT_DOC}" PARENT_SCOPE)
set(EFTL_ASSEMBLY_VERSION "${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}.${EFTL_VERSION_UPDATE}.${EFTL_VERSION_BUILD}")
SET(EFTL_TRADE "TIB, TIBCO, TIBCO eFTL, and the TIBCO logo are either registered trademarks or trademarks of TIBCO Software Inc. in the United States and/or other countries.")
set(TITLE, "TIBCO EFTL .NET class library")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.cs.template ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/Version.cs)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/commonDotnetCoreClientLibs.targets.template ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/commonDotnetCoreClientLibs.targets)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/impl)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/json)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/websocket)

file(GLOB EFTL_cs_files_cmake *.cs *.csproj *.snk)
file(COPY ${EFTL_cs_files_cmake} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL)

file(GLOB EFTL_cs_files_impl ${CMAKE_CURRENT_SOURCE_DIR}/impl/*.cs)
file(COPY ${EFTL_cs_files_impl} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/impl)
file(GLOB EFTL_cs_files_json ${CMAKE_CURRENT_SOURCE_DIR}/json/*.cs)
file(COPY ${EFTL_cs_files_json} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/json)
file(GLOB EFTL_cs_files_websocket ${CMAKE_CURRENT_SOURCE_DIR}/websocket/*.cs)
file(COPY ${EFTL_cs_files_websocket} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/websocket)


add_custom_command(OUTPUT ${EFTL_CLIENT_LIB}
    COMMAND ${DOTNET_CMD} build --configuration Release /nowarn:CS0618 /nowarn:CS1591 /nowarn:CS1574 ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/TIBCO.EFTL.csproj
    COMMAND ${CMAKE_COMMAND} -E copy ${EFTL_CLIENT_NETSTANDARD_LIB} ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${EFTL_CLIENT_NETSTANDARD_DOC} ${CMAKE_CURRENT_BINARY_DIR}/
    DEPENDS ${EFTL_cs_files_cmake} ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/Version.cs ${EFTL_cs_files_impl} ${EFTL_cs_files_json} ${EFTL_cs_files_websocket}
    ) 


if (CMAKE_SYSTEM_NAME STREQUAL "Windows")

SET(POL_COMP "TIBCO Software, Inc.")
SET(POL_COPY "Copyright (c) ${TIB_COPYRIGHT_YEARS} TIBCO Software Inc.  ALL RIGHTS RESERVED")
SET(POL_PROD "TIBCO EFTL")
SET(POL_TITLE "Publisher Policy for TIBCO EFTL client class library.")
SET(POL_VERSION "${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}.${EFTL_VERSION_UPDATE}.${EFTL_VERSION_BUILD}")


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

add_custom_target(TIBCETUSEFTL_policy ALL DEPENDS ${POLICY_FILE_LIST})

endif (CMAKE_SYSTEM_NAME STREQUAL "Windows")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL.dll ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/netstandard2.0/TIBCO.EFTL.xml
        DESTINATION sdks/dotnet/bin COMPONENT runtime)
add_custom_target(CETUS_EFTL_classlib ALL DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/TIBCO.EFTL/Version.cs
    TIBCO.EFTL.dll)

add_library(TIBCO.EFTL SHARED IMPORTED)

install(FILES ${EFTL_CLIENT_DOC} DESTINATION bin COMPONENT documentation)
