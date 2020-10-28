cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(ed7039e)

find_package(lcm REQUIRED)
include(${LCM_USE_FILE})

option(INSTALL_PYTHON_DEPS "Whether to build and install dependencies for Python nodes")
option(INSTALL_BINARY_NODES "Whether to build and install binary nodes")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic -pedantic-errors -Wpedantic -Wno-unused-parameter -Wno-format -Wno-format-security")

if(INSTALL_PYTHON_DEPS)
  find_package(PythonInterp REQUIRED)
  set(python_args PYTHON_SOURCES python_sources)
  lcm_wrap_types(${python_args} messages.lcm)
  lcm_install_python(${python_sources})
endif()

# Common build instuctions
lcm_wrap_types(
  C_SOURCES c_sources
  C_HEADERS c_headers
  ${python_args}
  messages.lcm) # TODO: GLOB all *.lcm and build common lib
lcm_add_library(lcmtypes C STATIC ${c_sources} ${c_headers})
# don't leak source files into install
target_include_directories(lcmtypes INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

if(INSTALL_BINARY_NODES)
  add_executable(lcm-source-dwm lcm-source-dwm.c)
  target_link_libraries(lcm-source-dwm lcmtypes m pthread)

  install(TARGETS lcm-source-dwm
    EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    INCLUDES DESTINATION include)
endif()
