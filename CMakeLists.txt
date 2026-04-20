cmake_minimum_required(VERSION 3.25)
project(a320_cfm56_xp12 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(CompilerOptions)

# ── Dependencies via FetchContent ──────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.2.1
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.5
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(mpack
    GIT_REPOSITORY https://github.com/ludocode/mpack.git
    GIT_TAG        v1.1.1
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(fmt nlohmann_json asio Catch2 mpack)

# Asio header-only interface target
add_library(asio_iface INTERFACE)
target_include_directories(asio_iface INTERFACE "${asio_SOURCE_DIR}/asio/include")
target_compile_definitions(asio_iface INTERFACE ASIO_STANDALONE)

# ImGui (compiled as a static lib — we add only what we need)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()
add_library(imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
)
target_include_directories(imgui PUBLIC "${imgui_SOURCE_DIR}")
target_apply_compiler_options(imgui)

# MPack header-only shim (mpack is single-header)
add_library(mpack_iface INTERFACE)
target_include_directories(mpack_iface INTERFACE "${mpack_SOURCE_DIR}/src")

# ── X-Plane SDK ────────────────────────────────────────────────────────────────
set(XPLMSDK_ROOT "${CMAKE_SOURCE_DIR}/sdk")
find_package(XPLMSDK REQUIRED)

# ── Main plugin shared library ─────────────────────────────────────────────────
file(GLOB_RECURSE PLUGIN_SOURCES
    "src/plugin/*.cpp"
    "src/core/*.cpp"
    "src/systems/**/*.cpp"
    "src/network/*.cpp"
    "src/debug/*.cpp"
)

add_library(a320_plugin MODULE ${PLUGIN_SOURCES})
target_apply_compiler_options(a320_plugin)

target_include_directories(a320_plugin PRIVATE
    "${XPLMSDK_INCLUDE_DIRS}"
    src
)

target_link_libraries(a320_plugin PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    asio_iface
    imgui
    mpack_iface
    ${XPLMSDK_LIBRARIES}
    $<$<BOOL:${WIN32}>:ws2_32>
    $<$<BOOL:${WIN32}>:mswsock>
)

# Platform-specific linker flags
if(DEFINED XPLMSDK_LINK_FLAGS)
    target_link_options(a320_plugin PRIVATE ${XPLMSDK_LINK_FLAGS})
endif()

# Linux: export only the five plugin entry points
if(UNIX AND NOT APPLE)
    target_link_options(a320_plugin PRIVATE
        -Wl,--version-script="${CMAKE_SOURCE_DIR}/cmake/plugin.version"
    )
endif()

# Rename output: Win=win.xpl  Linux=lin.xpl  Mac=mac.xpl
if(WIN32)
    set_target_properties(a320_plugin PROPERTIES
        PREFIX "" SUFFIX ".xpl"
        OUTPUT_NAME "win"
    )
elseif(APPLE)
    set_target_properties(a320_plugin PROPERTIES
        PREFIX "" SUFFIX ".xpl"
        OUTPUT_NAME "mac"
        BUNDLE FALSE
    )
else()
    set_target_properties(a320_plugin PROPERTIES
        PREFIX "" SUFFIX ".xpl"
        OUTPUT_NAME "lin"
    )
endif()

# ── Unit tests (no X-Plane dependency) ────────────────────────────────────────
file(GLOB_RECURSE TEST_SOURCES "tests/unit/*.cpp")
if(TEST_SOURCES)
    # Core modules that have no XPLM dependency at compile time (UNIT_TESTING guards the call-sites)
    set(TEST_CORE_SOURCES
        src/core/Config.cpp
        src/core/Logger.cpp
        src/core/EventBus.h   # header-only, listed for IDE visibility
    )

    add_executable(a320_tests
        ${TEST_SOURCES}
        src/core/Config.cpp
        src/core/Logger.cpp
        src/core/DataRefManager.cpp
        src/systems/electrical/ElectricalSystem.cpp
        src/systems/electrical/Generator.cpp
        src/systems/electrical/Battery.cpp
        src/systems/electrical/ACBus.cpp
        src/systems/electrical/DCBus.cpp
        src/systems/pneumatics/PneumaticSystem.cpp
        src/systems/pneumatics/BleedSource.cpp
        src/systems/pneumatics/Pack.cpp
        src/systems/pneumatics/AntiIce.cpp
        src/systems/pneumatics/Pressurization.cpp
    )
    target_apply_compiler_options(a320_tests)
    target_compile_definitions(a320_tests PRIVATE UNIT_TESTING=1)
    target_include_directories(a320_tests PRIVATE src)
    target_link_libraries(a320_tests PRIVATE
        Catch2::Catch2WithMain
        fmt::fmt
        nlohmann_json::nlohmann_json
    )
    # On MinGW, statically bundle the C++ runtime so the test binary is self-contained
    # and CTest's discovery step can run it without MinGW DLLs in PATH.
    if(MINGW)
        target_link_options(a320_tests PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
    include(CTest)
    include(Catch)
    catch_discover_tests(a320_tests)
endif()
