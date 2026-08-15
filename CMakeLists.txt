cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(WiiTanksNative C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization flags
if(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(-O3 -march=native -flto)
endif()

# Find system ENet
find_package(PkgConfig REQUIRED)
pkg_check_modules(ENET REQUIRED libenet)

# Include Raylib via FetchContent (using master branch compatible with modern CMake)
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ENET_INCLUDE_DIRS}
    ${raylib_SOURCE_DIR}/src
)

# Source files
file(GLOB_RECURSE SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(wii_tanks ${SOURCES})

target_link_libraries(wii_tanks PRIVATE
    raylib
    ${ENET_LIBRARIES}
    m
    pthread
    dl
)

# Copy assets to build directory
add_custom_command(TARGET wii_tanks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:wii_tanks>/assets
)
