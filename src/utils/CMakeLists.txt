include(FetchContent)

set(JSON_Install OFF CACHE INTERNAL "")
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders OFF CACHE INTERNAL "")

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/json
)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
endif()

add_library(json_parser
    JSONParser.cpp
    JSONParser.h
)

target_include_directories(json_parser 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${json_SOURCE_DIR})