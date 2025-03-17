cmake_minimum_required(VERSION 3.20)
project(Conqorial-Core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add option for distribution mode
option(DISTRIBUTION_MODE "Enable distribution mode" OFF)

file(GLOB_RECURSE CORE_SOURCES "src/*.cpp" "src/*.h")
add_library(Conqorial-Core STATIC ${CORE_SOURCES})

target_include_directories(Conqorial-Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
)

# Configure the preprocessor definitions based on build type
if(DISTRIBUTION_MODE)
    target_compile_definitions(Conqorial-Core PUBLIC DISTRIBUTION)
endif()

target_link_libraries(Conqorial-Core)
