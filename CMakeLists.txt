cmake_minimum_required(VERSION 3.20)
project(hello_world_core LANGUAGES C)

# Enable FetchContent
include(FetchContent)

# Fetch libretro-common for headers
FetchContent_Declare(
    libretro-common
    GIT_REPOSITORY https://github.com/libretro/libretro-common.git
    GIT_TAG master
)
FetchContent_MakeAvailable(libretro-common)

# Define the shared library
add_library(hello_world_core SHARED src/lib.c)

# Set include directories
target_include_directories(hello_world_core PRIVATE
    ${libretro-common_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set compile definitions
target_compile_definitions(hello_world_core PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

# Ensure the output is a .dll with no prefix
set_target_properties(hello_world_core PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hello_world_core"
    SUFFIX ".dll"
)

# Set C standard
set_property(TARGET hello_world_core PROPERTY C_STANDARD 99)