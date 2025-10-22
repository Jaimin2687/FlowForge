# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 4.1.2)

# Reject any attempt to use a toolchain file. We must not use one because
# we could be downloading it here. If the CMAKE_TOOLCHAIN_FILE environment
# variable is set, the cache variable will have been initialized from it.
unset(CMAKE_TOOLCHAIN_FILE CACHE)
unset(ENV{CMAKE_TOOLCHAIN_FILE})

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(json-populate NONE)



include(ExternalProject)
ExternalProject_Add(json-populate
                     "UPDATE_DISCONNECTED" "False" "EXTERNALPROJECT_INTERNAL_ARGUMENT_SEPARATOR" "URL" "https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp" "DOWNLOAD_DIR" "/Users/jaimin/workflow-engine/build/src/utils/json" DOWNLOAD_NO_EXTRACT YES
                    SOURCE_DIR          "/Users/jaimin/workflow-engine/build/_deps/json-src"
                    BINARY_DIR          "/Users/jaimin/workflow-engine/build/_deps/json-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


ExternalProject_Get_Property(json-populate DOWNLOADED_FILE)
get_filename_component(dlFileName "${DOWNLOADED_FILE}" NAME)

ExternalProject_Add_Step(json-populate copyfile
  COMMAND    "/opt/homebrew/bin/cmake" -E copy_if_different
             "<DOWNLOADED_FILE>" "/Users/jaimin/workflow-engine/build/_deps/json-src"
  DEPENDEES  patch
  DEPENDERS  configure
  BYPRODUCTS "/Users/jaimin/workflow-engine/build/_deps/json-src/${dlFileName}"
  COMMENT    "Copying file to SOURCE_DIR"
)

