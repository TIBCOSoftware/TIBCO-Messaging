file(COPY messaging DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY setup.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY README.md DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/messaging/eftl/connection.py ${CMAKE_CURRENT_BINARY_DIR}/messaging/eftl/connection.py)

set(EFTL_PY_CLIENT_LIB "${CMAKE_CURRENT_BINARY_DIR}/eftl-1.1.0-py3-none-any.whl")

add_custom_command(OUTPUT ${EFTL_PY_CLIENT_LIB}
    COMMAND ${Python3_EXECUTABLE} setup.py sdist bdist_wheel -d ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/messaging/eftl/connection.py)

add_custom_target(pylibout ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/eftl-1.1.0-py3-none-any.whl)
set_target_properties(pylibout PROPERTIES ADDITIONAL_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/dist;${CMAKE_CURRENT_BINARY_DIR}/build;${CMAKE_CURRENT_BINARY_DIR}/eftl.egg-info")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eftl-1.1.0-py3-none-any.whl DESTINATION sdks/python3/lib COMPONENT runtime)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/messaging/eftl/doc/connection.m.html DESTINATION doc/eftl/html/python COMPONENT documentation)
