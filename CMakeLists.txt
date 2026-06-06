cmake_minimum_required(VERSION 3.20)
project(cs2_bhop_dll LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Output DLL to build dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# MinHook via FetchContent (no vcpkg needed)
include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG master
)
FetchContent_MakeAvailable(minhook)

# ImGui - fetch and add sources manually for DX11 + Win32 backend (for overlay menu)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.9  # recent stable
)
FetchContent_MakeAvailable(imgui)

# Collect ImGui sources for our build (we will use DX11 + Win32 for overlay)
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

add_library(cs2_bhop SHARED
    src/dllmain.cpp
    src/cheat.cpp
    src/menu.cpp
    src/hooks.cpp
    src/utils.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(cs2_bhop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk
    ${minhook_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(cs2_bhop PRIVATE
    minhook
    d3d11
    dxgi
    user32
    gdi32
)

# UTF-8 and no min/max macros
target_compile_definitions(cs2_bhop PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# Release optimizations
if(MSVC)
    target_compile_options(cs2_bhop PRIVATE
        $<$<CONFIG:Release>:/O2 /Oi /Ot /GL /Gy>
        /W3
    )
    target_link_options(cs2_bhop PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
endif()

message(STATUS "CS2 Bhop DLL configured. Use -DBUILD_WITH_IMGUI_OVERLAY=ON if needed (default on).")