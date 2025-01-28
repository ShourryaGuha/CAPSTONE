# executable target
add_executable(main main.c)

# Add the library target for the drivers
add_library(drivers gps.c)

# Specify the include directories
target_include_directories(drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
