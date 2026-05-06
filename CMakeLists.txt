# CMakeLists.txt for CUBES AI Learning Simulation
# 
# Build instructions:
#   cd build
#   cmake ..
#   make
#
# Requirements:
#   brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer

cmake_minimum_required(VERSION 3.10)
project(CUBES_SIMULATION VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# SDL2 Paths (macOS Homebrew)
# Code uses: #include <SDL2/SDL_ttf.h>
# So we need /usr/local/include in the path (which contains SDL2/)
# ============================================================================

if(APPLE)
    # Homebrew paths
    set(SDL2_INCLUDE_PATH /usr/local/include /opt/homebrew/include)
    set(SDL2_LIB_PATH /usr/local/lib /opt/homebrew/lib)
    
    # Find SDL2 headers (look for SDL2/SDL.h in the include paths)
    find_path(SDL2_BASE_INCLUDE SDL2/SDL.h
        PATHS ${SDL2_INCLUDE_PATH}
        NO_DEFAULT_PATH)
    
    if(SDL2_BASE_INCLUDE)
        message(STATUS "Found SDL2 headers at: ${SDL2_BASE_INCLUDE}")
        # SDL2_BASE_INCLUDE is now /usr/local/include (contains SDL2/)
        include_directories(${SDL2_BASE_INCLUDE})
    else()
        message(FATAL_ERROR "SDL2 headers not found. Install with: brew install sdl2")
    endif()
    
    # Find libraries
    find_library(SDL2_LIB SDL2 PATHS ${SDL2_LIB_PATH} NO_DEFAULT_PATH)
    find_library(SDL2_IMAGE_LIB SDL2_image PATHS ${SDL2_LIB_PATH} NO_DEFAULT_PATH)
    find_library(SDL2_TTF_LIB SDL2_ttf PATHS ${SDL2_LIB_PATH} NO_DEFAULT_PATH)
    find_library(SDL2_MIXER_LIB SDL2_mixer PATHS ${SDL2_LIB_PATH} NO_DEFAULT_PATH)
    
    if(NOT SDL2_LIB)
        message(FATAL_ERROR "SDL2 library not found")
    endif()
else()
    # Linux: use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# ============================================================================
# Include directories
# ============================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/include/menu
    ${SDL2_INCLUDE_DIR}
)

# ============================================================================
# Source files
# ============================================================================

file(GLOB CORE_SRC "src/core/*.cpp")
file(GLOB RENDERING_SRC "src/rendering/*.cpp")
file(GLOB MENU_SRC "src/menu/*.cpp")
set(MAIN_SRC "src/main.cpp")

# Remove brain_visualizer from build (feature removed)
list(FILTER RENDERING_SRC EXCLUDE REGEX ".*brain_visualizer.*")

# ============================================================================
# Create executable
# ============================================================================

add_executable(simulation ${CORE_SRC} ${RENDERING_SRC} ${MENU_SRC} ${MAIN_SRC})

# ============================================================================
# Link libraries
# ============================================================================

if(APPLE)
    target_link_libraries(simulation
        ${SDL2_LIB}
        ${SDL2_IMAGE_LIB}
        ${SDL2_TTF_LIB}
        ${SDL2_MIXER_LIB}
    )
else()
    target_link_libraries(simulation
        ${SDL2_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_MIXER_LIBRARIES}
    )
endif()

# ============================================================================
# Print configuration
# ============================================================================

message(STATUS "============================")
message(STATUS "CUBES Simulation Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
if(APPLE)
    message(STATUS "  SDL2 Library: ${SDL2_LIB}")
else()
    message(STATUS "  SDL2 Libraries: ${SDL2_LIBRARIES}")
endif()
message(STATUS "============================")
