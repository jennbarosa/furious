cmake_minimum_required(VERSION 3.25)
project(FURIOUS VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_Declare(
    nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG v1.2.1
)

FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG v0.9.7
)

FetchContent_Declare(
    imgui_test_engine
    GIT_REPOSITORY https://github.com/ocornut/imgui_test_engine.git
    GIT_TAG main
)

FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG develop
)

FetchContent_MakeAvailable(glfw imgui miniaudio json nfd magic_enum imgui_test_engine lua sol2)

set(LUA_SRC_DIR ${lua_SOURCE_DIR}/src)
add_library(lua_static STATIC
    ${LUA_SRC_DIR}/lapi.c
    ${LUA_SRC_DIR}/lcode.c
    ${LUA_SRC_DIR}/lctype.c
    ${LUA_SRC_DIR}/ldebug.c
    ${LUA_SRC_DIR}/ldo.c
    ${LUA_SRC_DIR}/ldump.c
    ${LUA_SRC_DIR}/lfunc.c
    ${LUA_SRC_DIR}/lgc.c
    ${LUA_SRC_DIR}/llex.c
    ${LUA_SRC_DIR}/lmem.c
    ${LUA_SRC_DIR}/lobject.c
    ${LUA_SRC_DIR}/lopcodes.c
    ${LUA_SRC_DIR}/lparser.c
    ${LUA_SRC_DIR}/lstate.c
    ${LUA_SRC_DIR}/lstring.c
    ${LUA_SRC_DIR}/ltable.c
    ${LUA_SRC_DIR}/ltm.c
    ${LUA_SRC_DIR}/lundump.c
    ${LUA_SRC_DIR}/lvm.c
    ${LUA_SRC_DIR}/lzio.c
    ${LUA_SRC_DIR}/lauxlib.c
    ${LUA_SRC_DIR}/lbaselib.c
    ${LUA_SRC_DIR}/lcorolib.c
    ${LUA_SRC_DIR}/ldblib.c
    ${LUA_SRC_DIR}/liolib.c
    ${LUA_SRC_DIR}/lmathlib.c
    ${LUA_SRC_DIR}/loadlib.c
    ${LUA_SRC_DIR}/loslib.c
    ${LUA_SRC_DIR}/lstrlib.c
    ${LUA_SRC_DIR}/ltablib.c
    ${LUA_SRC_DIR}/lutf8lib.c
    ${LUA_SRC_DIR}/linit.c
)

target_include_directories(lua_static PUBLIC ${LUA_SRC_DIR})
if(WIN32)
    target_compile_definitions(lua_static PRIVATE LUA_USE_WINDOWS)
elseif(APPLE)
    target_compile_definitions(lua_static PRIVATE LUA_USE_MACOSX)
else()
    target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)
    target_link_libraries(lua_static PRIVATE m dl)
endif()

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${miniaudio_SOURCE_DIR})

find_package(OpenGL REQUIRED)

if(WIN32)
    find_package(PkgConfig)
else()
    find_package(PkgConfig REQUIRED)
endif()

if(PkgConfig_FOUND)
    pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
    pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
    pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
    pkg_check_modules(SWRESAMPLE REQUIRED IMPORTED_TARGET libswresample)
    pkg_check_modules(RUBBERBAND REQUIRED IMPORTED_TARGET rubberband)
    set(FFMPEG_LIBS
        PkgConfig::AVCODEC PkgConfig::AVFORMAT PkgConfig::AVUTIL
        PkgConfig::SWSCALE PkgConfig::SWRESAMPLE
    )
    set(RUBBERBAND_LIB PkgConfig::RUBBERBAND)
else()
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
    find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
    find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
    find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
    find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)

    find_library(AVCODEC_LIB avcodec)
    find_library(AVFORMAT_LIB avformat)
    find_library(AVUTIL_LIB avutil)
    find_library(SWSCALE_LIB swscale)
    find_library(SWRESAMPLE_LIB swresample)
    find_library(RUBBERBAND_LIBRARY rubberband)

    if(NOT AVCODEC_LIB OR NOT AVFORMAT_LIB OR NOT AVUTIL_LIB OR NOT SWSCALE_LIB OR NOT SWRESAMPLE_LIB)
        message(FATAL_ERROR "FFmpeg libraries not found. Install FFmpeg development libraries (e.g. via vcpkg).")
    endif()
    if(NOT RUBBERBAND_LIBRARY)
        message(FATAL_ERROR "Rubberband library not found. Install rubberband (e.g. via vcpkg).")
    endif()

    add_library(ffmpeg_libs INTERFACE)
    target_include_directories(ffmpeg_libs INTERFACE
        ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR}
        ${SWSCALE_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR}
    )
    target_link_libraries(ffmpeg_libs INTERFACE
        ${AVCODEC_LIB} ${AVFORMAT_LIB} ${AVUTIL_LIB}
        ${SWSCALE_LIB} ${SWRESAMPLE_LIB}
    )

    add_library(rubberband_lib INTERFACE)
    target_link_libraries(rubberband_lib INTERFACE ${RUBBERBAND_LIBRARY})

    set(FFMPEG_LIBS ffmpeg_libs)
    set(RUBBERBAND_LIB rubberband_lib)
endif()

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_SOURCE_DIR}/include  # For our custom imconfig.h
)

target_compile_definitions(imgui PUBLIC
    IMGUI_USER_CONFIG="furious/imconfig.h"
)

target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_library(imgui_with_test_engine STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_capture_tool.cpp
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_context.cpp
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_coroutine.cpp
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_engine.cpp
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_exporters.cpp
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_perftool.cpp
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_ui.cpp
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine/imgui_te_utils.cpp
)

target_include_directories(imgui_with_test_engine PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_test_engine_SOURCE_DIR}/imgui_test_engine
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(imgui_with_test_engine PUBLIC
    IMGUI_USER_CONFIG="furious/imconfig.h"
    FURIOUS_ENABLE_TEST_ENGINE
    IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL=1
)

target_link_libraries(imgui_with_test_engine PUBLIC glfw OpenGL::GL)

add_library(furious_lib STATIC
    src/application.cpp
    src/ui/main_window.cpp
    src/ui/viewport.cpp
    src/ui/timeline.cpp
    src/ui/transport_controls.cpp
    src/ui/profiler_window.cpp
    src/ui/patterns_window.cpp
    src/core/project.cpp
    src/core/project_data.cpp
    src/core/tempo.cpp
    src/core/timeline_data.cpp
    src/core/command.cpp
    src/core/pattern.cpp
    src/core/pattern_library.cpp
    src/core/pattern_evaluator.cpp
    src/audio/audio_clip.cpp
    src/audio/audio_engine.cpp
    src/audio/audio_buffer.cpp
    src/audio/audio_decoder.cpp
    src/video/source_library.cpp
    src/video/video_decoder.cpp
    src/video/video_engine.cpp
    src/scripting/script_engine.cpp
    src/scripting/lua_bindings.cpp
    src/core/pitch_track.cpp
    src/core/pitch_library.cpp
    src/audio/pitch_estimator.cpp
    src/audio/pitch_shifter.cpp
    src/ui/pitch_editor_window.cpp
)

set_target_properties(furious_lib PROPERTIES UNITY_BUILD ON)

target_precompile_headers(furious_lib PRIVATE
    <string>
    <vector>
    <memory>
    <cstdint>
    <optional>
    <unordered_map>
    <algorithm>
    <cmath>
    <chrono>
    <expected>
    <nlohmann/json.hpp>
)

target_include_directories(furious_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${sol2_SOURCE_DIR}/include
)

target_link_libraries(furious_lib PUBLIC
    imgui
    miniaudio
    nlohmann_json::nlohmann_json
    nfd
    magic_enum::magic_enum
    lua_static
    ${FFMPEG_LIBS}
    ${RUBBERBAND_LIB}
)

if(WIN32)
    target_link_libraries(furious_lib PUBLIC psapi)
endif()

add_executable(furious src/main.cpp)
target_link_libraries(furious PRIVATE furious_lib)

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(furious_tests
        tests/tempo_test.cpp
        tests/project_test.cpp
        tests/project_data_test.cpp
        tests/timeline_test.cpp
        tests/timeline_data_test.cpp
        tests/audio_test.cpp
        tests/audio_buffer_test.cpp
        tests/audio_decoder_test.cpp
        tests/source_library_test.cpp
        tests/video_test.cpp
        tests/effects_test.cpp
        tests/script_engine_test.cpp
        tests/command_test.cpp
        tests/pattern_test.cpp
        tests/pattern_evaluator_test.cpp
    )

    target_link_libraries(furious_tests PRIVATE
        furious_lib
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(furious_tests)

    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS furious_tests furious_integration_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests..."
    )
endif()

option(BUILD_INTEGRATION_TESTS "Build integration tests" ON)
if(BUILD_INTEGRATION_TESTS)
    add_library(furious_lib_testable STATIC
        src/application.cpp
        src/ui/main_window.cpp
        src/ui/viewport.cpp
        src/ui/timeline.cpp
        src/ui/transport_controls.cpp
        src/ui/profiler_window.cpp
        src/ui/patterns_window.cpp
        src/core/project.cpp
        src/core/project_data.cpp
        src/core/tempo.cpp
        src/core/timeline_data.cpp
        src/core/command.cpp
        src/core/pattern.cpp
        src/core/pattern_library.cpp
        src/core/pattern_evaluator.cpp
        src/audio/audio_clip.cpp
        src/audio/audio_engine.cpp
        src/audio/audio_buffer.cpp
        src/audio/audio_decoder.cpp
        src/video/source_library.cpp
        src/video/video_decoder.cpp
        src/video/video_engine.cpp
        src/scripting/script_engine.cpp
        src/scripting/lua_bindings.cpp
        src/core/pitch_track.cpp
        src/core/pitch_library.cpp
        src/audio/pitch_estimator.cpp
        src/audio/pitch_shifter.cpp
        src/ui/pitch_editor_window.cpp
    )

    target_include_directories(furious_lib_testable PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${sol2_SOURCE_DIR}/include
    )

    target_link_libraries(furious_lib_testable PUBLIC
        imgui_with_test_engine
        miniaudio
        nlohmann_json::nlohmann_json
        nfd
        magic_enum::magic_enum
        lua_static
        ${FFMPEG_LIBS}
        ${RUBBERBAND_LIB}
    )

    target_precompile_headers(furious_lib_testable REUSE_FROM furious_lib)

    add_executable(furious_integration_tests
        tests/integration/integration_main.cpp
        tests/integration/timeline_tests.cpp
        tests/integration/viewport_tests.cpp
        tests/integration/transport_tests.cpp
        tests/integration/source_tests.cpp
        tests/integration/clip_tests.cpp
        tests/integration/sync_tests.cpp
        tests/integration/user_flow_tests.cpp
        tests/integration/undo_redo_tests.cpp
        tests/integration/pattern_tests.cpp
        tests/integration/audio_engine_tests.cpp
    )

    target_link_libraries(furious_integration_tests PRIVATE
        furious_lib_testable
    )

    add_test(NAME integration_tests COMMAND furious_integration_tests)
endif()
