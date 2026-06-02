cmake_minimum_required(VERSION 3.18)
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

project(dfg_place_schedule VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(ORTOOLS_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../or-tools_x86_64_debian-sid_cpp_v9.10.4067"
    CACHE PATH "Local OR-Tools C++ package root")
list(APPEND CMAKE_PREFIX_PATH "${ORTOOLS_ROOT}")

find_package(ortools REQUIRED CONFIG)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_RPATH "${ORTOOLS_ROOT}/lib")
set(CMAKE_INSTALL_RPATH "${ORTOOLS_ROOT}/lib")

add_library(dfg_ilp
  dfg/dfg.cpp
  dfg/parser.cpp
  dfg/validator.cpp
  model/variables.cpp
  model/solution.cpp
  model/objective.cpp
  model/ilp_solver.cpp
  model/constraints/timing.cpp
  model/constraints/legal_slice.cpp
  model/constraints/slice_icu.cpp
  model/constraints/stream.cpp
  visualization/exporter.cpp
)

target_include_directories(dfg_ilp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dfg_ilp PUBLIC ortools::ortools)

add_executable(solve tools/solve.cpp)
target_link_libraries(solve PRIVATE dfg_ilp)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
