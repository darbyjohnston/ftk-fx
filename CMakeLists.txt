cmake_minimum_required(VERSION 3.31)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0091 NEW)

file(READ "lib/fx/Core/Version.h" VERSION_H)
string(REGEX MATCH "VERSION_MAJOR ([0-9]*)" _ ${VERSION_H})
set(FX_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_MINOR ([0-9]*)" _ ${VERSION_H})
set(FX_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_PATCH ([0-9]*)" _ ${VERSION_H})
set(FX_VERSION_PATCH ${CMAKE_MATCH_1})

project(
    ftk-fx
    VERSION ${FX_VERSION_MAJOR}.${FX_VERSION_MINOR}.${FX_VERSION_PATCH}
    DESCRIPTION "An interactive particle FX application."
    LANGUAGES CXX C)

#-------------------------------------------------------------------------------
# Options

set(FX_PROGRAMS ON CACHE BOOL "Build programs")
set(FX_TESTS ON CACHE BOOL "Build tests")

#-------------------------------------------------------------------------------
# Internal configuration

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

if(FX_TESTS)
    set(CTEST_OUTPUT_ON_FAILURE ON)
    enable_testing()
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>DLL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
endif()

if(APPLE)
    set(CMAKE_FIND_FRAMEWORK LAST)
endif()

if(WIN32)
elseif(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
    set(CMAKE_BUILD_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#-------------------------------------------------------------------------------
# Git submodules

find_package(Git REQUIRED)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/tlRender/CMakeLists.txt")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_SUBMODULE_RESULT)
    if(NOT GIT_SUBMODULE_RESULT EQUAL 0)
        message(FATAL_ERROR "git submodule update failed with ${GIT_SUBMODULE_RESULT}")
    endif()
endif()

#-------------------------------------------------------------------------------
# Sub-directories

# tlRender is built from source, and brings feather-tk with it. Both are here
# as submodules rather than as installed packages so that a debugger can step
# from a particle straight down into the widget that drew it -- the same reason
# DJV is laid out this way.
#
# Nothing in fx links tlRender yet; the application layer uses ftkUI and ftkGL
# directly. tlRender is what the plate reader, the sprite textures, and the EXR
# output will be built on.
add_subdirectory(deps/tlRender)
add_library(tlRender::tlCore ALIAS tlCore)
add_library(tlRender::tlGL ALIAS tlGL)
add_library(tlRender::tlIO ALIAS tlIO)
add_library(tlRender::tlTimeline ALIAS tlTimeline)
add_library(tlRender::tlUI ALIAS tlUI)

add_subdirectory(lib/fx)
if(FX_PROGRAMS)
    add_subdirectory(bin/ftk-fx)
endif()
if(FX_TESTS)
    add_subdirectory(tests)
endif()
