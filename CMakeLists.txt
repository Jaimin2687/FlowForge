cmake_minimum_required(VERSION 3.15)
project(FlowForge)
set(CMAKE_CXX_STANDARD 17)

# Add utils subdirectory first to set up json library
add_subdirectory(src/utils)

# Add plugins
add_subdirectory(plugins)

add_executable(flowforge
    src/main.cpp
    src/WorkflowManager.cpp
    src/Workflow.cpp
    src/PluginLoader.cpp
    src/Logger.cpp
    src/ThreadPool.cpp
    src/Storage.cpp
    src/RuleEngine.cpp
)

target_include_directories(flowforge 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/utils
)

target_link_libraries(flowforge
    PRIVATE
        dl
        json_parser
        curl
)
