cmake_minimum_required(VERSION 3.5)
project(LitTorrent)

# Force Debug build type
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(REDIS_BUILD ON)
set(REDIS_ENABLE_TEST ON)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

set(LITTORRENT_ENABLE_TEST ON)

# C++ Standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define compile flags for Debug and Release modes
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

# Include all header files in the include directory
file(GLOB LITTORRENT_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/LitTorrent/*.h")
file(GLOB LITTORRENT_SOURCE_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LitTorrent/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LitTorrent/BEncoding/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/*.cpp"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)

add_executable(LitTorrent ${LITTORRENT_SOURCE_FILES})
target_include_directories(LitTorrent PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LitTorrent 
)

# GTest Configuration
if(LITTORRENT_ENABLE_TEST)
    enable_testing()
    
    # Fetch GTest from GitHub
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    
    # Add test subdirectory
    add_subdirectory(test)
endif()
