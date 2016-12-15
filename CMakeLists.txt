cmake_minimum_required(VERSION 2.8)
project(amp_device_stub)

include(${CMAKE_CURRENT_SOURCE_DIR}/unabto/build/cmake/unabto_project.cmake)

set(src
  ${unabto_src}
  ${unabto_module_cli_gopt_src}
  ${unabto_module_fingerprint_acl_src}
  ${unabto_module_read_hex}
  ${UNABTO_MODULES_SRC_DIR}/util/read_hex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/unabto_main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/unabto_application.c
)

include_directories(
  ${unabto_include_directories}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_definitions(${unabto_definitions})

add_executable(${PROJECT_NAME} ${src})

target_link_libraries(${PROJECT_NAME} ${unabto_link_libraries})

install( TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
