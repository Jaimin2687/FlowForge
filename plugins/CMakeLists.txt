cmake_minimum_required(VERSION 3.14)
project(FlowForgePlugins)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Root include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/utils)

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

# CURL (required for EmailPlugin and MessagePlugin)
find_package(CURL REQUIRED)

# ZLIB (required for CompressAction)
find_package(ZLIB REQUIRED)

# Optional: libarchive (for advanced compression)
find_library(LIBARCHIVE archive)
find_path(LIBARCHIVE_INCLUDE archive.h)

if(LIBARCHIVE AND LIBARCHIVE_INCLUDE)
    message(STATUS "Found libarchive: ${LIBARCHIVE} (headers: ${LIBARCHIVE_INCLUDE})")
else()
    message(WARNING "libarchive not found; CompressAction will use system zip command instead.")
endif()

# ------------------------------------------------------------------------------
# EmailPlugin Plugin
# ------------------------------------------------------------------------------

add_library(EmailPlugin SHARED EmailPlugin.cpp)
target_include_directories(EmailPlugin PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(EmailPlugin PRIVATE ${CURL_LIBRARIES})
target_compile_definitions(EmailPlugin PRIVATE CURL_STATICLIB)

# ------------------------------------------------------------------------------
# MessagePlugin Plugin
# ------------------------------------------------------------------------------

add_library(MessagePlugin SHARED MessagePlugin.cpp)
target_include_directories(MessagePlugin PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(MessagePlugin PRIVATE ${CURL_LIBRARIES})
target_compile_definitions(MessagePlugin PRIVATE CURL_STATICLIB)

# ------------------------------------------------------------------------------
# CompressAction Plugin
# ------------------------------------------------------------------------------

add_library(CompressAction SHARED CompressAction.cpp)
target_include_directories(CompressAction PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Always link Zlib
target_link_libraries(CompressAction PRIVATE ZLIB::ZLIB)

# Link libarchive if found
if(LIBARCHIVE AND LIBARCHIVE_INCLUDE)
    target_include_directories(CompressAction PRIVATE ${LIBARCHIVE_INCLUDE})
    target_compile_definitions(CompressAction PRIVATE LIBARCHIVE_PRESENT=1)
    target_link_libraries(CompressAction PRIVATE ${LIBARCHIVE})
endif()

# ------------------------------------------------------------------------------
# Plugin Output Directory
# ------------------------------------------------------------------------------

set(PLUGIN_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/plugins)

foreach(tgt EmailPlugin MessagePlugin CompressAction)
    set_target_properties(${tgt} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIR}
        SUFFIX ".so"
    )
endforeach()

message(STATUS "✅ All plugins will be built into: ${PLUGIN_OUTPUT_DIR}")
