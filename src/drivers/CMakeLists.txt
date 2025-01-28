# Find all .c files in the drivers directory
file(GLOB DRIVER_SOURCES *.c)

# Add the library target for the drivers
add_library(drivers ${DRIVER_SOURCES})

# Specify the include directories
target_include_directories(drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
