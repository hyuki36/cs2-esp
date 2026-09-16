cmake_minimum_required(VERSION 3.20)
project(cs2_external LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Requires: Visual Studio 2022 + Windows SDK (MSVC). No MinGW.
# Build: cmake -B build -A x64 ; cmake --build build --config Release

include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

add_executable(cs2_external
  src/main.cpp
  src/cs2.cpp
  src/overlay.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(cs2_external PRIVATE
  src
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(cs2_external PRIVATE
  d3d11
  d3dcompiler
  dwmapi
)

target_compile_definitions(cs2_external PRIVATE
  UNICODE _UNICODE
  NOMINMAX WIN32_LEAN_AND_MEAN
)

if(MSVC)
  target_compile_options(cs2_external PRIVATE /W3 /EHsc /MP)
endif()

set_target_properties(cs2_external PROPERTIES OUTPUT_NAME cs2esp)
