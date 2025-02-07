# Find all .c files in the drivers directory
file(GLOB DRIVER_SOURCES *.c)

# Add the library target for the drivers
#add_library(drivers ${DRIVER_SOURCES})
add_library(drivers laserTOF.c gps.c)

# Specify the include directories
target_include_directories(drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link the pigpio library (make sure it's installed and accessible)
target_link_libraries(drivers pigpio)
target_link_libraries(drivers m)