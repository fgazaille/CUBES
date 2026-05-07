# CMakeLists.txt for CUBES AI Learning Simulation

cmake_minimum_required(VERSION 3.10)
project(CUBES_SIMULATION VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra")

# Install prefix
if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix" FORCE)
endif()

# ============================================================================
# Find SDL2 packages (cross-platform)
# ============================================================================

# Try to find SDL2 using find_package (works if SDL2 provides CMake config)
# Fall back to pkg-config for Linux, or manual search for macOS
find_package(SDL2 QUIET)
find_package(SDL2_image QUIET)
find_package(SDL2_ttf QUIET)
find_package(SDL2_mixer QUIET)

if(SDL2_FOUND AND SDL2_IMAGE_FOUND AND SDL2_TTF_FOUND AND SDL2_MIXER_FOUND)
    message(STATUS "Found SDL2 via find_package")
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS})
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_MIXER_LIBRARIES})
else()
    # Fall back to pkg-config (Linux) or manual search (macOS)
    if(APPLE)
        # macOS Homebrew paths
        find_library(SDL2_LIB SDL2 PATHS /usr/local/lib /opt/homebrew/lib)
        find_library(SDL2_IMAGE_LIB SDL2_image PATHS /usr/local/lib /opt/homebrew/lib)
        find_library(SDL2_TTF_LIB SDL2_ttf PATHS /usr/local/lib /opt/homebrew/lib)
        find_library(SDL2_MIXER_LIB SDL2_mixer PATHS /usr/local/lib /opt/homebrew/lib)
        
        # Find SDL2 headers
        find_path(SDL2_BASE_INCLUDE SDL2/SDL.h PATHS /usr/local/include /opt/homebrew/include)
        if(SDL2_BASE_INCLUDE)
            set(SDL2_INCLUDE_DIRS ${SDL2_BASE_INCLUDE})
        else()
            message(FATAL_ERROR "SDL2 headers not found. Install with: brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer")
        endif()
        set(SDL2_LIBRARIES ${SDL2_LIB} ${SDL2_IMAGE_LIB} ${SDL2_TTF_LIB} ${SDL2_MIXER_LIB})
    else()
        # Linux: use pkg-config
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED sdl2)
        pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
        pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
        pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
        set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS})
        set(SDL2_LIBRARIES ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_MIXER_LIBRARIES})
    endif()
endif()

# ============================================================================
# Include directories (target-specific)
# ============================================================================

set(PROJECT_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/include/menu
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
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
# Include directories (target-specific)
# ============================================================================

target_include_directories(simulation PRIVATE
    ${PROJECT_INCLUDES}
    ${SDL2_INCLUDE_DIRS}
)

# ============================================================================
# Link libraries
# ============================================================================

target_link_libraries(simulation
    ${SDL2_LIBRARIES}
)

# ============================================================================
# Testing
# ============================================================================

enable_testing()
add_executable(test_neural_network tests/test_neural_network.cpp src/core/neural_network.cpp)
target_include_directories(test_neural_network PRIVATE ${PROJECT_INCLUDES})
add_test(NAME NeuralNetworkTests COMMAND test_neural_network)

# ============================================================================
# Install targets
# ============================================================================

install(TARGETS simulation DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/CUBES)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/Player.png 
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/Food.png 
        DESTINATION share/CUBES/assets OPTIONAL)

# ============================================================================
# Print configuration
# ============================================================================

message(STATUS "============================")
message(STATUS "CUBES Simulation Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  SDL2 Include Dirs: ${SDL2_INCLUDE_DIRS}")
message(STATUS "  SDL2 Libraries: ${SDL2_LIBRARIES}")
message(STATUS "============================")
