#
# Build the .NET documentation

set(DOTNETDOC_OUTPUT_DIR ${CMAKE_BINARY_DIR}/doc/eftl/html/dotnet)
make_directory(${DOTNETDOC_OUTPUT_DIR})

#doxygen stuff
set(TIB_DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/doc/eftl/html/dotnet)
file(MAKE_DIRECTORY ${TIB_DOXYGEN_OUTPUT_DIRECTORY})
    set(TIB_DOXYGEN_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../TIBCO.EFTL/ ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox")

set(TIB_DOXYGEN_IS_PUBLIC_API YES)
set(TIB_DOXYGEN_API_NAME "eFTL&trade;")
set(TIB_DOXYGEN_HTML_OUTPUT "")
set(TIB_OPTIMIZE_FOR_C NO)
set(TIB_DOXYGEN_LAYOUT_FILE "")
set(TIB_DOXYGEN_SHOW_USED_FILES NO)
set(TIB_DOXYGEN_SHOW_FILES NO)
configure_file(${build_common_SOURCE_DIR}/btools/doxygen/doxygen.config ${CMAKE_CURRENT_BINARY_DIR}/doxygen.config)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../TIBCO.EFTL/mainpage.dox.config ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox)

add_custom_command(OUTPUT dotnet_eftl_doc
                    COMMAND ${DOXYGEN} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.config)
install(DIRECTORY ${TIB_DOXYGEN_OUTPUT_DIRECTORY} DESTINATION doc/eftl COMPONENT documentation)

add_custom_target(dotnet_eftl_public_doc DEPENDS dotnet_eftl_doc)
add_dependencies(doc dotnet_eftl_public_doc)
install(DIRECTORY ${TIB_DOXYGEN_OUTPUT_DIRECTORY} DESTINATION sdks/dotnet/eftl/${EFTL_VERSION_MAJOR}.${EFTL_VERSION_MINOR}/doc/html COMPONENT documentation)

