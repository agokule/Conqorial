cmake_minimum_required(VERSION 3.20)
project(Conqorial-Client)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add option for distribution mode (should match the one in core)
option(DISTRIBUTION_MODE "Enable distribution mode" OFF)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

if(DISTRIBUTION_MODE)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Distribution")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Distribution")
endif()

file(GLOB_RECURSE CLIENT_SOURCES "src/*.cpp" "src/*.h")

add_executable(Conqorial-Client ${CLIENT_SOURCES}
  dependencies/imgui/imgui.cpp
  dependencies/imgui/imgui_draw.cpp
  dependencies/imgui/imgui_tables.cpp
  dependencies/imgui/imgui_widgets.cpp
  dependencies/imgui/imgui_demo.cpp
  dependencies/imgui/backends/imgui_impl_sdl3.cpp
  dependencies/imgui/backends/imgui_impl_sdlrenderer3.cpp
  dependencies/imgui/misc/cpp/imgui_stdlib.cpp
  dependencies/implot/implot.cpp
  dependencies/implot/implot_items.cpp
)

add_subdirectory(dependencies/SDL-3.2.8 EXCLUDE_FROM_ALL)
add_subdirectory(../core "${CMAKE_BINARY_DIR}/core" EXCLUDE_FROM_ALL)

target_include_directories(Conqorial-Client PRIVATE
  src
  dependencies/imgui
  dependencies/imgui/backends
  dependencies/imgui/misc/cpp
  dependencies/implot
)

# Configure the preprocessor definitions based on build type
if(DISTRIBUTION_MODE)
    target_compile_definitions(Conqorial-Client PUBLIC DISTRIBUTION)
endif()

# on Web targets, we need CMake to generate a HTML webpage. 
if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
  set(EMMAKE_CXX_CFLAGS "${EMMAKE_CXX_CFLAGS}")
  if (DISTRIBUTION_MODE)
    set(EMMAKE_CXX_CFLAGS "${EMMAKE_CXX_CFLAGS} -03")
  endif()
  set_target_properties(Conqorial-Client PROPERTIES LINK_FLAGS "${EMMAKE_CXX_CFLAGS}")
endif()

target_link_libraries(Conqorial-Client 
  SDL3::SDL3
  Conqorial-Core
)

