cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(ed7039e)

find_package(lcm REQUIRED)
include(${LCM_USE_FILE})

find_package(PythonInterp REQUIRED)
set(python_args PYTHON_SOURCES python_sources)

lcm_wrap_types(
  C_SOURCES c_sources
  C_HEADERS c_headers
  ${python_args}
  dwm.lcm)

lcm_add_library(lcmtypes C STATIC ${c_sources} ${c_headers})
target_include_directories(lcmtypes INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>) # don't leak source files into install

add_executable(dwm_send dwm_send.c)
target_link_libraries(dwm_send lcmtypes m)

install(TARGETS lcmtypes dwm_send
  EXPORT ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib${LIB_SUFFIX}
  ARCHIVE DESTINATION lib${LIB_SUFFIX}
  INCLUDES DESTINATION include)
lcm_install_python(${python_sources})
install(PROGRAMS dwm_recv.py DESTINATION bin)
