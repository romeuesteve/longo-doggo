cmake_minimum_required(VERSION 3.25)

project(LongoDoggo LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(LONGO_DOGGO_BUILD_GAME "Build the Longo Doggo executable when its sources are present" ON)
option(LONGO_DOGGO_BUILD_TESTS "Build the raylib-free gameplay smoke test" ON)
set(LONGO_DOGGO_PLATFORM "auto" CACHE STRING "Build platform: auto, native, or web")
set_property(CACHE LONGO_DOGGO_PLATFORM PROPERTY STRINGS auto native web)

if(LONGO_DOGGO_PLATFORM STREQUAL "auto")
    if(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(LONGO_DOGGO_PLATFORM "web")
    else()
        set(LONGO_DOGGO_PLATFORM "native")
    endif()
endif()

if(LONGO_DOGGO_BUILD_TESTS AND LONGO_DOGGO_PLATFORM STREQUAL "native")
    enable_testing()
    add_executable(longo_game_smoke tests/game_smoke.c
        src/core/world.c src/core/solid.c src/core/events.c src/core/view.c src/objects/dog.c src/objects/hole.c src/objects/box.c src/objects/door.c src/objects/button.c src/objects/items.c src/objects/house.c src/objects/fx.c src/objects/butterfly.c src/objects/flower.c src/objects/dialogue.c src/objects/transition.c src/objects/title.c src/room_tiles.c src/sprites.c src/level_data.c)
    target_compile_features(longo_game_smoke PRIVATE c_std_99)
    target_include_directories(longo_game_smoke PRIVATE src src/core)
    add_test(NAME longo_game_smoke COMMAND longo_game_smoke)
endif()

if(NOT LONGO_DOGGO_PLATFORM STREQUAL "native" AND NOT LONGO_DOGGO_PLATFORM STREQUAL "web")
    message(FATAL_ERROR "LONGO_DOGGO_PLATFORM must be auto, native, or web")
endif()

if(LONGO_DOGGO_PLATFORM STREQUAL "web")
    if(NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        message(FATAL_ERROR "The web build must be configured through emcmake with the Emscripten toolchain")
    endif()

    # raylib uses these cache variables to select its HTML5/WebAssembly backend.
    set(PLATFORM "Web" CACHE STRING "raylib platform" FORCE)
    set(OPENGL_VERSION "ES 2.0" CACHE STRING "raylib graphics backend" FORCE)
else()
    # The documented native command uses CMake's MinGW Makefiles generator.
    set(PLATFORM "Desktop" CACHE STRING "raylib platform" FORCE)
endif()

# Keep the dependency self-contained and avoid pulling in examples or a system GLFW.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(USE_EXTERNAL_GLFW "OFF" CACHE STRING "" FORCE)

include(FetchContent)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
)
FetchContent_MakeAvailable(raylib)

set(LONGO_DOGGO_SOURCES
    src/main.c
    src/core/world.c
    src/core/solid.c
    src/core/events.c
    src/core/view.c
    src/objects/dog.c
    src/objects/hole.c
    src/objects/box.c
    src/objects/door.c
    src/objects/button.c
    src/objects/items.c
    src/objects/house.c
    src/objects/fx.c
    src/objects/butterfly.c
    src/objects/flower.c
    src/objects/dialogue.c
    src/objects/transition.c
    src/objects/title.c
    src/core/view.c
    src/render.c
    src/sprites.c
    src/level_data.c
    src/room_tiles.c
)

set(LONGO_DOGGO_MISSING_SOURCES)
foreach(source IN LISTS LONGO_DOGGO_SOURCES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        list(APPEND LONGO_DOGGO_MISSING_SOURCES "${source}")
    endif()
endforeach()

if(LONGO_DOGGO_BUILD_GAME AND NOT LONGO_DOGGO_MISSING_SOURCES)
    add_executable(longo_doggo ${LONGO_DOGGO_SOURCES})
    target_compile_features(longo_doggo PRIVATE c_std_99)
    target_link_libraries(longo_doggo PRIVATE raylib)

    # Keep a runnable build self-contained: the runtime assets live under
    # assets/exported-assets and are copied beside the executable.
    set(LONGO_RUNTIME_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets/exported-assets")
    if(EXISTS "${LONGO_RUNTIME_ASSETS}")
        add_custom_command(TARGET longo_doggo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:longo_doggo>/assets/exported-assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${LONGO_RUNTIME_ASSETS}"
                    "$<TARGET_FILE_DIR:longo_doggo>/assets/exported-assets"
            COMMENT "Copying runtime assets beside longo_doggo")
    endif()

    # The UI font (Renogare, see assets/fonts for its license) is repo-local.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts")
        add_custom_command(TARGET longo_doggo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:longo_doggo>/assets/fonts"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
                    "$<TARGET_FILE_DIR:longo_doggo>/assets/fonts"
            COMMENT "Copying fonts beside longo_doggo")
    endif()

    if(LONGO_DOGGO_PLATFORM STREQUAL "web")
        # Emscripten emits the companion .js and .wasm files beside this HTML shell.
        set_target_properties(longo_doggo PROPERTIES SUFFIX ".html")
    endif()
else()
    if(LONGO_DOGGO_BUILD_GAME)
        message(STATUS "Longo Doggo executable is not added; missing source files:")
        foreach(source IN LISTS LONGO_DOGGO_MISSING_SOURCES)
            message(STATUS "  ${source}")
        endforeach()
        message(STATUS "raylib is configured and can still be built to validate the platform setup.")
    endif()
endif()
