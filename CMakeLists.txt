# CMake version requirement
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(CAPSTONE VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add subdirectories to build
add_subdirectory(src/drivers)

# Create main executable
add_executable(main src/drivers/main.c)
target_link_libraries(main PRIVATE drivers m)

# Create gimbal_control executable
add_executable(gimbal_control src/drivers/gimbal_control.c)
target_link_libraries(gimbal_control PRIVATE drivers m pigpio pthread rt)

# Include directories for the project
target_include_directories(gimbal_control PRIVATE ${CMAKE_SOURCE_DIR}/src/drivers)
