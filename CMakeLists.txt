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

if(NOT LONGO_DOGGO_PLATFORM STREQUAL "native" AND NOT LONGO_DOGGO_PLATFORM STREQUAL "web")
    message(FATAL_ERROR "LONGO_DOGGO_PLATFORM must be auto, native, or web")
endif()

# ---------------------------------------------------------------------
# The headless simulation core: one library shared by the game and every
# test or diagnostic target, so they all exercise the exact sources the
# game ships.
# ---------------------------------------------------------------------
set(LONGO_CORE_SOURCES
    src/core/world.c
    src/core/solid.c
    src/core/undo.c
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
    src/room_tiles.c
    src/sprites.c
    src/level_data.c
)

add_library(longo_core STATIC ${LONGO_CORE_SOURCES})
target_compile_features(longo_core PUBLIC c_std_99)
target_include_directories(longo_core PUBLIC src src/core)
if(UNIX AND NOT APPLE)
    target_link_libraries(longo_core PUBLIC m)
endif()

if(LONGO_DOGGO_BUILD_TESTS AND LONGO_DOGGO_PLATFORM STREQUAL "native")
    enable_testing()

    add_executable(longo_game_smoke tests/game_smoke.c)
    target_link_libraries(longo_game_smoke PRIVATE longo_core)

    # One test per gameplay rule, so a failure names the rule.  Keep this
    # list in step with the registry at the bottom of tests/game_smoke.c
    # (the binary also prints it via `longo_game_smoke --list`).
    set(LONGO_SMOKE_SCENARIOS
        title_flow_room_order
        tutorial_dialogue_gates_play
        movement_and_chain
        room_load_rules
        apple_pear_length
        walls_and_push_rules
        buttons_door_win_retry
        retry_reloads_room
        fx_pools_reuse_dead_slots
        undo
        door_open_window_solidity)
    add_test(NAME smoke_all COMMAND longo_game_smoke)
    foreach(scenario IN LISTS LONGO_SMOKE_SCENARIOS)
        add_test(NAME smoke_${scenario} COMMAND longo_game_smoke ${scenario})
    endforeach()

    # The checks must stay active in Release: this scenario fails by
    # design, so a build where -DNDEBUG strips the asserts would let it
    # "pass" and ctest flags exactly that regression.
    add_test(NAME smoke_fail_probe COMMAND longo_game_smoke fail_probe)
    set_tests_properties(smoke_fail_probe PROPERTIES WILL_FAIL TRUE)

    # Diagnostic, not a ctest: replays the live-verification input
    # sequence and prints the states for manual review.
    add_executable(longo_playthrough tests/playthrough.c)
    target_link_libraries(longo_playthrough PRIVATE longo_core)
endif()

# ---------------------------------------------------------------------
# The game executable: raylib front-end on top of the shared core.
# raylib is only fetched when the game is actually being built.
# ---------------------------------------------------------------------
if(LONGO_DOGGO_BUILD_GAME)
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

    # raylib's web backend builds inline JS with EM_ASM, which the Emscripten
    # compiler only accepts in GNU C mode (-std=gnu*); the game targets keep
    # strict ISO C99.
    set_property(TARGET raylib PROPERTY C_EXTENSIONS ON)

    set(LONGO_GAME_SOURCES
        src/main.c
        src/render.c
    )

    set(LONGO_GAME_MISSING_SOURCES)
    foreach(source IN LISTS LONGO_GAME_SOURCES)
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
            list(APPEND LONGO_GAME_MISSING_SOURCES "${source}")
        endif()
    endforeach()

    if(NOT LONGO_GAME_MISSING_SOURCES)
        add_executable(longo_doggo ${LONGO_GAME_SOURCES})
        target_compile_features(longo_doggo PRIVATE c_std_99)
        target_link_libraries(longo_doggo PRIVATE longo_core raylib)

        # Keep a runnable build self-contained: the runtime assets live under
        # assets/exported-assets and are copied beside the executable. On web
        # they are packed into the Emscripten virtual filesystem instead.
        set(LONGO_RUNTIME_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets/exported-assets")
        if(LONGO_DOGGO_PLATFORM STREQUAL "web")
            if(EXISTS "${LONGO_RUNTIME_ASSETS}")
                target_link_options(longo_doggo PRIVATE
                    "SHELL:--preload-file ${LONGO_RUNTIME_ASSETS}@assets/exported-assets")
            endif()
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts")
                target_link_options(longo_doggo PRIVATE
                    "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts@assets/fonts")
            endif()
            target_link_options(longo_doggo PRIVATE "SHELL:-sALLOW_MEMORY_GROWTH=1")
            # itch.io embeds the generated HTML directly, so the shell is a bare
            # canvas that fills the frame; the default shell would show its
            # Emscripten logo and resize controls inside the game.
            target_link_options(longo_doggo PRIVATE
                "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/src/web/shell.html")
        elseif(EXISTS "${LONGO_RUNTIME_ASSETS}")
            add_custom_command(TARGET longo_doggo POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                        "$<TARGET_FILE_DIR:longo_doggo>/assets/exported-assets"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${LONGO_RUNTIME_ASSETS}"
                        "$<TARGET_FILE_DIR:longo_doggo>/assets/exported-assets"
                COMMENT "Copying runtime assets beside longo_doggo")
        endif()

        # The UI font (Renogare, see assets/fonts for its license) is repo-local.
        if(LONGO_DOGGO_PLATFORM STREQUAL "native" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts")
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
            set_target_properties(longo_doggo PROPERTIES SUFFIX ".html"
                LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/web/shell.html")
        endif()
    else()
        message(STATUS "Longo Doggo executable is not added; missing source files:")
        foreach(source IN LISTS LONGO_GAME_MISSING_SOURCES)
            message(STATUS "  ${source}")
        endforeach()
    endif()
endif()
