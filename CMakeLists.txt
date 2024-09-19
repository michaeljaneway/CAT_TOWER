cmake_minimum_required(VERSION 3.21)

# ========================================================================
# Commands to download emsdk 3.1.64 if not downloaded
# ========================================================================

if (NOT EXISTS ../emsdk)
    execute_process(COMMAND git clone --depth 1 --branch 3.1.64 https://github.com/emscripten-core/emsdk
        WORKING_DIRECTORY ..)
endif()

# ========================================================================
# Install (if uninstalled) and activate emsdk
# ========================================================================

execute_process(COMMAND emsdk.bat install latest
WORKING_DIRECTORY ../emsdk
RESULT_VARIABLE cmd_result
OUTPUT_VARIABLE cmd_ver
OUTPUT_STRIP_TRAILING_WHITESPACE
OUTPUT_QUIET)

execute_process(COMMAND emsdk.bat activate latest
WORKING_DIRECTORY ../emsdk
RESULT_VARIABLE cmd_result
OUTPUT_VARIABLE cmd_ver
OUTPUT_STRIP_TRAILING_WHITESPACE
OUTPUT_QUIET)

# ========================================================================
# Create Project
# ========================================================================
project(microjam20)

# Set C++ (CXX) Standard to 2020
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
file(GLOB SOURCES "src/*.cpp" "src/*/*.cpp" "include/*.hpp" "include/*/*.hpp" "include/*.h" "include/*/*.h")
add_executable(${PROJECT_NAME} ${SOURCES})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# ========================================================================
# Download & Install Dependencies
# ========================================================================

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY          https://github.com/raysan5/raylib.git
    GIT_TAG                 master
    OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
    raygui
    GIT_REPOSITORY          https://github.com/raysan5/raygui.git
    GIT_TAG                 master
    OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
    flecs
    GIT_REPOSITORY          https://github.com/SanderMertens/flecs.git
    GIT_TAG                 master
    OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
    json
    GIT_REPOSITORY          https://github.com/nlohmann/json.git
    GIT_TAG                 v3.11.3
    OVERRIDE_FIND_PACKAGE
)

# Dont build supplied examples & games 
set(BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES         OFF CACHE BOOL "" FORCE)

# Platform is a cache var
set(PLATFORM "Web" CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)
FetchContent_MakeAvailable(raygui)

option(FLECS_SHARED "Build shared flecs lib" ON)
option(FLECS_STATIC "Build static flecs lib" OFF)
FetchContent_MakeAvailable(flecs)

FetchContent_MakeAvailable(json)

# ========================================================================
# Includes and Libs
# ========================================================================

target_include_directories(
    ${PROJECT_NAME} 
    PRIVATE 
    
    "${CMAKE_SOURCE_DIR}/include"
    "${raylib_SOURCE_DIR}/include"
    "${raygui_SOURCE_DIR}/src"
    "${flecs_SOURCE_DIR}/include"
    "${json_SOURCE_DIR}/include"
)

target_link_libraries(
    ${PROJECT_NAME}
    
    raylib
    flecs
    nlohmann_json::nlohmann_json
)

# ========================================================================
# Web
# ========================================================================

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX .html)    
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sASSERTIONS=1 -sUSE_GLFW=3 -sALLOW_MEMORY_GROWTH -sTOTAL_STACK=128MB -sFETCH -sSTACK_SIZE=32MB -sINITIAL_MEMORY=64MB --shell-file \"${CMAKE_SOURCE_DIR}/minshell.html\"")

    # Map assets to root of .data file (only if assets folder exists)
    if (EXISTS "${CMAKE_SOURCE_DIR}/assets")
        set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets/@/")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${ASSETS_DIR}\"")
    endif()

    # ========================================================================
    # Create itch.io zip package
    # ========================================================================

    add_custom_command(TARGET ${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E echo "copying ${PROJECT_NAME}.html to index.html"
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.html ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html
        COMMAND ${CMAKE_COMMAND} -E echo "packaging files to zip"
        COMMAND cd "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" && ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/itch_package.zip" --format=zip -- "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.js" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.wasm" && cd ..
        COMMAND ${CMAKE_COMMAND} -E echo "deleting index.html after packaging"
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html
    )
endif()

