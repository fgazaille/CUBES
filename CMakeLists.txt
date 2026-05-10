cmake_minimum_required(VERSION 3.10)
project(CUBES_SIMULATION VERSION 2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra")

# ============================================================================
# Find Raylib
# ============================================================================

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /usr/local /opt/homebrew)
endif()

find_package(raylib QUIET)

if(NOT raylib_FOUND)
    # Manual fallback
    if(APPLE)
        find_library(RAYLIB_LIB raylib PATHS /usr/local/lib /opt/homebrew/lib)
        find_path(RAYLIB_INCLUDE raylib.h PATHS /usr/local/include /opt/homebrew/include)
        if(RAYLIB_LIB AND RAYLIB_INCLUDE)
            add_library(raylib UNKNOWN IMPORTED)
            set_target_properties(raylib PROPERTIES
                IMPORTED_LOCATION "${RAYLIB_LIB}"
                INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_INCLUDE}"
            )
            message(STATUS "Found Raylib: ${RAYLIB_LIB}")
        else()
            message(FATAL_ERROR "Raylib not found. Install with: brew install raylib")
        endif()
    else()
        message(FATAL_ERROR "Raylib not found. Install: sudo apt install libraylib-dev")
    endif()
endif()

# ============================================================================
# Include directories
# ============================================================================

set(PROJECT_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/include/menu
    ${CMAKE_CURRENT_SOURCE_DIR}/include/debug
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# ============================================================================
# Source files
# ============================================================================

file(GLOB CORE_SRC      "src/core/*.cpp")
file(GLOB RENDERING_SRC "src/rendering/*.cpp")
file(GLOB MENU_SRC      "src/menu/*.cpp")
file(GLOB DEBUG_SRC     "src/debug/*.cpp")
set(MAIN_SRC "src/main.cpp")

# ============================================================================
# Executable
# ============================================================================

add_executable(simulation ${CORE_SRC} ${RENDERING_SRC} ${MENU_SRC} ${DEBUG_SRC} ${MAIN_SRC})

target_include_directories(simulation PRIVATE ${PROJECT_INCLUDES})

if(TARGET raylib::raylib)
    target_link_libraries(simulation raylib::raylib)
elseif(TARGET raylib)
    target_link_libraries(simulation raylib)
else()
    target_link_libraries(simulation ${RAYLIB_LIBRARIES})
endif()

# ============================================================================
# Testing
# ============================================================================

enable_testing()
add_executable(test_neural_network tests/test_neural_network.cpp src/core/neural_network.cpp)
target_include_directories(test_neural_network PRIVATE ${PROJECT_INCLUDES})
add_test(NAME NeuralNetworkTests COMMAND test_neural_network)

# ============================================================================
# Install
# ============================================================================

install(TARGETS simulation DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/CUBES)

message(STATUS "============================")
message(STATUS "CUBES Simulation v2.0 (Raylib)")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "============================")
