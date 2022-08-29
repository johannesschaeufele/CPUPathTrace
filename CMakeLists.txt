cmake_minimum_required(VERSION 3.13)

project(PathTrace LANGUAGES CXX)

option(PATHTRACE_BUILD_DEMO "Build the demo application" ON)
option(PATHTRACE_BUILD_BENCHMARK "Build benchmarks for the project" OFF)
option(PATHTRACE_BUILD_TESTS "Build tests for the project" OFF)
option(PATHTRACE_BUILD_FUZZING "Build fuzzers for the project" OFF)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  add_compile_options(-Wno-missing-braces)
endif()

# Main library
file(GLOB_RECURSE PATHTRACE_LIBRARY_SOURCES src/*.cpp)
add_library(PathTrace ${PATHTRACE_LIBRARY_SOURCES})
target_include_directories(PathTrace PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

find_package(PNG REQUIRED)
target_link_libraries(PathTrace PRIVATE PNG::PNG)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(PathTrace PRIVATE Threads::Threads)

# Demo application
if(PATHTRACE_BUILD_DEMO)
  set(PATHTRACE_DEMO_SOURCES demo/main.cpp)
  add_executable(PathTraceDemo ${PATHTRACE_DEMO_SOURCES})
  target_link_libraries(PathTraceDemo PRIVATE PathTrace)
endif()

# Tests
if(PATHTRACE_BUILD_TESTS)
  enable_testing()
  file(GLOB_RECURSE PATHTRACE_TEST_SOURCES test/*.cpp)
  add_executable(PathTraceTest ${PATHTRACE_TEST_SOURCES})
  target_include_directories(PathTraceTest PRIVATE test)
  target_link_libraries(PathTraceTest PRIVATE PathTrace)

  find_package(GTest REQUIRED)
  target_link_libraries(PathTraceTest PRIVATE GTest::GTest)
endif()

# Fuzzing
if(PATHTRACE_BUILD_FUZZING)
  file(GLOB_RECURSE PATHTRACE_FUZZ_TARGETS fuzz/target_*.cpp)

  foreach(target ${PATHTRACE_FUZZ_TARGETS})
    get_filename_component(target_name ${target} NAME_WE)
    set(target_executable "fuzz_${target_name}")

    set(target_sources "")
    list(APPEND target_sources ${target})
    add_executable(${target_executable} ${target_sources})
    target_include_directories(${target_executable} PRIVATE fuzz)
    target_link_libraries(${target_executable} PRIVATE PathTrace)

    target_compile_options(${target_executable} PRIVATE -g -fno-omit-frame-pointer -fsanitize=address,undefined,fuzzer)
    target_link_options(${target_executable} PRIVATE -fsanitize=address,undefined,fuzzer)
  endforeach()
endif()

# Benchmarks
if(PATHTRACE_BUILD_BENCHMARK)
  file(GLOB_RECURSE PATHTRACE_BENCHMARK_SOURCES benchmark/*.cpp)
  add_executable(PathTraceBenchmark ${PATHTRACE_BENCHMARK_SOURCES})
  target_include_directories(PathTraceBenchmark PRIVATE benchmark)
  target_link_libraries(PathTraceBenchmark PRIVATE PathTrace)

  find_package(benchmark REQUIRED)
  target_link_libraries(PathTraceBenchmark PRIVATE benchmark::benchmark)
endif()

# Install
set(project_config "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(targets_export_name ${PROJECT_NAME}Targets)
configure_file(cmake/Config.cmake.in "${project_config}" @ONLY)

set(config_install_dir "lib/cmake/${PROJECT_NAME}")

install(
  TARGETS PathTrace
  EXPORT "${targets_export_name}"
  ARCHIVE DESTINATION "lib/"
  LIBRARY DESTINATION "lib/"
  RUNTIME DESTINATION "bin/"
  INCLUDES DESTINATION "include/")

install(
  DIRECTORY include/PathTrace
  DESTINATION "include/"
  FILES_MATCHING PATTERN "*.h")

install(
  FILES "${project_config}"
  DESTINATION "${config_install_dir}")

install(
  EXPORT "${targets_export_name}"
  NAMESPACE "${PROJECT_NAME}::"
  DESTINATION "${config_install_dir}")
