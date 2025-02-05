cmake_minimum_required(VERSION 3.5.0)
project(test VERSION 0.1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Add library
add_library(radp SHARED ${SRC})

# Add executables
add_executable(radp_test main.cpp ${SRC})
add_executable(radp_test_c main-c.cpp ${SRC})

# Link libraries

# Include directories for targets
target_include_directories(radp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(radp_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(radp_test_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Enable testing
include(CTest)
enable_testing()

# CPack configuration
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
