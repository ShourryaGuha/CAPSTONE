# CMake version requirement
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(CAPSTONE VERSION 1.0)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add subdirectories to build
add_subdirectory(src/drivers)
add_subdirectory(src/kalman_filter)
add_subdirectory(src/kinematics)