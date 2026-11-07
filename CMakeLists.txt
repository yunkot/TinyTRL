cmake_minimum_required(VERSION 3.16)

project(TinyTRL
  VERSION 0.1.0
  DESCRIPTION "Tiny Template and Runtime Library - a compact STL replacement"
  LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build the example applications alongside the library. Defaults to ON when
# TinyTRL is the top-level project, and OFF when it is pulled in via
# add_subdirectory() by someone else's project (so consumers don't get extra
# executables they didn't ask for).
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(TINYTRL_DEFAULT_EXAMPLES ON)
else()
  set(TINYTRL_DEFAULT_EXAMPLES OFF)
endif()
option(TINYTRL_BUILD_EXAMPLES "Build the example applications." ${TINYTRL_DEFAULT_EXAMPLES})

# Single-config generators (Ninja, Makefiles - used with GCC/Clang) read the
# build type from CMAKE_BUILD_TYPE at configure time. Default it to Release so
# an unconfigured build is already optimized.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Multi-config generators (Visual Studio, Xcode) ignore CMAKE_BUILD_TYPE and
# are driven by `cmake --build --config <Cfg>` instead. Listing Release first
# mirrors the single-config default and makes IDEs default to an optimized
# build. (Note: `cmake --build` with no --config still defaults to Debug, so
# pass --config explicitly on multi-config generators.)
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release;Debug;RelWithDebInfo;MinSizeRel"
    CACHE STRING "Configurations for multi-config generators." FORCE)
endif()

# TinyTRL is designed to work in freestanding, exception-free environments.
if(MSVC)
  set(TINYTRL_NO_EXCEPTIONS_FLAG "/EHs-c-")
else()
  set(TINYTRL_NO_EXCEPTIONS_FLAG "-fno-exceptions")
endif()

add_library(TinyTRL
  src/TinyTRL_Math.cpp
  src/TinyTRL_Strings.cpp
  src/TinyTRL_Timing.cpp
  src/TinyTRL_Streams.cpp)

add_library(TinyTRL::TinyTRL ALIAS TinyTRL)

target_compile_features(TinyTRL PUBLIC cxx_std_20)
target_include_directories(TinyTRL
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_options(TinyTRL PUBLIC ${TINYTRL_NO_EXCEPTIONS_FLAG})

set_target_properties(TinyTRL PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON)

if(WIN32)
  # Suppress the min/max macros from <windows.h> and CRT deprecation warnings.
  target_compile_definitions(TinyTRL PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS TinyTRL
  EXPORT TinyTRLTARGETS
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT TinyTRLTARGETS
  FILE TinyTRLTARGETS.cmake
  NAMESPACE TinyTRL::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyTRL)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TinyTRLConfig.cmake"
  "${CMAKE_BINARY_DIR}/TinyTRLConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyTRL)

write_basic_package_version_file(
  "${CMAKE_BINARY_DIR}/TinyTRLPackageVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  "${CMAKE_BINARY_DIR}/TinyTRLConfig.cmake"
  "${CMAKE_BINARY_DIR}/TinyTRLPackageVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TinyTRL)

if(TINYTRL_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
