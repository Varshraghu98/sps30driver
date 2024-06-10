cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sps30drivertest)

# Add the executable target
#add_executable(app src/main.c)

target_sources(app PRIVATE src/main.c 
src/sps30.c
src/sensirion_common.c
src/hal.c)


# Optionally, if you have header files, you can include their directories
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
