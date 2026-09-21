cmake_minimum_required(VERSION 3.16)

# Ensure CMake 3.30+ compatibility for older submodules (like World)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# Auto-patch submodules for C++20 compatibility on clean checkouts
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/inicpp/include/inicpp/types.h")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/lib/inicpp/include/inicpp/types.h" _inicpp_types)
    if (NOT _inicpp_types MATCHES "#include <cstdint>")
        string(REPLACE "#include <variant>" "#include <variant>\n#include <cstdint>" _inicpp_types "${_inicpp_types}")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/lib/inicpp/include/inicpp/types.h" "${_inicpp_types}")
    endif ()
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/inicpp/include/inicpp/option.h")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/lib/inicpp/include/inicpp/option.h" _inicpp_content)
    if (NOT _inicpp_content MATCHES "add_to_list<ValueType>")
        string(REPLACE "for (auto &&item : list) { add_to_list(item); }" "for (auto &&item : list) { add_to_list<ValueType>(item); }" _inicpp_content "${_inicpp_content}")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/lib/inicpp/include/inicpp/option.h" "${_inicpp_content}")
    endif ()
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/ColorCout/includes/ColorCout.hpp")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/lib/ColorCout/includes/ColorCout.hpp" _colorcout_content)
    if (NOT _colorcout_content MATCHES "#include <cstdint>")
        string(REPLACE "#include <iostream>" "#include <iostream>\n#include <cstdint>" _colorcout_content "${_colorcout_content}")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/lib/ColorCout/includes/ColorCout.hpp" "${_colorcout_content}")
    endif ()
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/ftxui/cmake/ftxui_set_options.cmake")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/lib/ftxui/cmake/ftxui_set_options.cmake" _ftxui_content)
    if (_ftxui_content MATCHES "\"-Werror\"")
        string(REPLACE "target_compile_options(\${library} PRIVATE \"-Werror\")" "# target_compile_options(\${library} PRIVATE \"-Werror\")" _ftxui_content "${_ftxui_content}")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/lib/ftxui/cmake/ftxui_set_options.cmake" "${_ftxui_content}")
    endif ()
endif ()

# Set Version, Release need cmake
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)

# use git version as library version
find_package(Git QUIET)
if (Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE _git_version
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
            OUTPUT_VARIABLE PROJECT_GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
else ()
    set(_git_version 0)
    set(PROJECT_GIT_HASH "")
endif ()

project(lessampler LANGUAGES C CXX VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${_git_version})

set(CMAKE_CXX_STANDARD 20)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHsc")
endif ()

# Add support for -DDEBUG_MODE=ON
set(DEBUG_SUFFIX)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    ADD_DEFINITIONS(-DDEBUG_MODE)
    set(DEBUG_SUFFIX "d")
endif ()

# Generate Configure file
configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/lessconfig.h"
)

if (MSVC)
    configure_file(
            "${PROJECT_SOURCE_DIR}/assets/lessampler.rc.in"
            "${PROJECT_BINARY_DIR}/lessampler.rc"
    )
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

find_package(Threads REQUIRED)

set(WORLD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WORLD_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(INICPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(INICPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)
set(ENABLE_MPEG OFF CACHE BOOL "" FORCE)
set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "" FORCE)

# Only for rapidjson
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "" FORCE)
set(RAPIDJSON_ENABLE_INSTRUMENTATION_OPT OFF CACHE BOOL "" FORCE)

# subdirectory
add_subdirectory(lib/World EXCLUDE_FROM_ALL)
add_subdirectory(lib/dialog EXCLUDE_FROM_ALL)
add_subdirectory(lib/inicpp EXCLUDE_FROM_ALL)
add_subdirectory(lib/ftxui EXCLUDE_FROM_ALL)
add_subdirectory(lib/rapidjson EXCLUDE_FROM_ALL)
add_subdirectory(lib/sndfile EXCLUDE_FROM_ALL)

# include header files
include_directories(
        src
        lib/dialog
        lib/ColorCout/includes
        lib/World/src
        lib/World/tools
        lib/ftxui/include
        lib/inicpp/include
        lib/rapidjson/include
        lib/sndfile/include
        ${PROJECT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}/lib/sndfile/include
)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(tools)
