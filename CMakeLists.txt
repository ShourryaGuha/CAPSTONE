# CMake version requirement
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(CAPSTONE VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(main src/drivers/main.c)

# Add subdirectories to build
add_subdirectory(src/drivers)
# add_subdirectory(src/kalman_filter)
# add_subdirectory(src/kinematics)

# Link the drivers library to the main executable
target_link_libraries(main PRIVATE drivers)

# Include directories for the project
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/src/drivers)
target_link_libraries(main 
    PRIVATE 
        drivers 
        m
)