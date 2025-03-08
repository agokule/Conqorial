cmake_minimum_required(VERSION 3.20)
project(Conqorial-Client)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

file(GLOB_RECURSE CLIENT_SOURCES "src/*.cpp" "src/*.h")

add_executable(Conqorial-Client ${CLIENT_SOURCES})
add_subdirectory(dependencies/SDL-3.2.8 EXCLUDE_FROM_ALL)
target_include_directories(Conqorial-Client PRIVATE src)

# on Web targets, we need CMake to generate a HTML webpage. 
if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
endif()

target_link_libraries(Conqorial-Client SDL3::SDL3)


