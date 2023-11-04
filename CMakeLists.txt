# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(cUSB C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(cUSB)

# compile duckyscript
add_custom_command( OUTPUT compdd.h
                    COMMAND python3 ${CMAKE_SOURCE_DIR}/ducky-compile.py
                    DEPENDS ${CMAKE_SOURCE_DIR}/ducky-compile.py
                    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/payload.dd
                    COMMENT "Compiling C code from duckyscript" VERBATIM)

add_custom_target(ducky-compile DEPENDS compdd.h)
add_dependencies(cUSB ducky-compile)

pico_set_program_name(cUSB "cUSB")
pico_set_program_version(cUSB "0.1")

pico_enable_stdio_uart(cUSB 1)

# begin tinyusb copy
target_link_libraries(cUSB PUBLIC pico_stdlib tinyusb_device tinyusb_board)
target_link_libraries(cUSB PUBLIC pico_multicore)

target_sources(cUSB PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(cUSB PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})
# end tinyusb copy

pico_add_extra_outputs(cUSB)

