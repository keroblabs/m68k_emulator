# CMakeList.txt : CMake project for wss_core, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

set(MODULE_NAME         cpu_emulator)
set(CMAKE_BUILD_TYPE    Debug)

project(${MODULE_NAME})

add_definitions(-D_USE_SCREEN_WIDTH=1280)
add_definitions(-D_USE_SCREEN_HEIGHT=960)
#add_definitions(-DTIMA_USE_CONFIG)
add_definitions(-DMAC_OS_EMULATOR)
add_definitions(-D_USE_LINUX)

# External board bus width is independent of the Musashi CPU core model.
# 8  = MC68008-style external bus; byte-wide devices use sequential addresses.
# 16 = MC68000/MC68010-style external bus; board_003 places 8-bit devices on D0-D7/LDS.
set(M68K_DATA_BUS_WIDTH "8" CACHE STRING "External M68K data bus width (8 or 16)")
set_property(CACHE M68K_DATA_BUS_WIDTH PROPERTY STRINGS 8 16)
if(NOT M68K_DATA_BUS_WIDTH STREQUAL "8" AND NOT M68K_DATA_BUS_WIDTH STREQUAL "16")
    message(FATAL_ERROR "M68K_DATA_BUS_WIDTH must be 8 or 16")
endif()
add_definitions(-DM68K_DATA_BUS_WIDTH=${M68K_DATA_BUS_WIDTH})

# Select the Yamaha V9958 or preserve the existing TMS9918 peripheral.
# V9958 is wired as four consecutive 8-bit ports on Board 004.
option(M68K_VIDEO_V9958 "Use Yamaha V9958 video" OFF)
if(M68K_VIDEO_V9958)
    if(NOT M68K_DATA_BUS_WIDTH STREQUAL "8")
        message(FATAL_ERROR "Board 004 V9958 wiring requires an 8-bit CPU bus")
    endif()
    add_definitions(-DM68K_VIDEO_V9958=1)
endif()

# A 68008 uses the 68000 instruction/core model. Set this option to use
# Musashi's 68010 core while retaining the separately-selected bus width.
option(M68K_EMULATE_68010 "Use Musashi MC68010 CPU model" OFF)
if(M68K_EMULATE_68010)
    add_definitions(-DM68K_EMULATE_68010=1)
else()
    add_definitions(-DM68K_EMULATE_68010=0)
endif()

# Select the keyboard model: OFF = UART keypad; ON = YM2149/MC68681 key matrix.
option(USE_KEYBOARD_MATRIX "Use the YM2149/MC68681 scanned key matrix instead of the UART keypad" ON)
if(USE_KEYBOARD_MATRIX)
    add_definitions(-DUSE_KEYBOARD_MATRIX=1)
endif()

link_directories(       ~/Projects/GIT_Dev/SDL/build)

include_directories(    ~/Projects/GIT_Dev/SDL/build/include
                       	~/Projects/GIT_Dev/SDL/build/include/SDL2
                      	~/Projects/GIT_Dev/unicorn/include
                        
                        ./
                        ./m68k_core
                        ./m68k_core/softfloat

                        ./lib_cli
                        ./lib_system
                        ./memory
                        ./peripherals
                        ./sdl
                        
                    )

# Add source to this project's executable.
set(                SOURCE_FILES

                    ./main.c 

                    ./m68k_core/m68kcpu.c
                    ./m68k_core/m68kdasm.c
                    #./m68k_core/m68kfpu.c
                    ./m68k_core/m68kops.c
                    ./m68k_core/softfloat/softfloat.c
                    
                    ./lib_cli/cli.c
                    ./lib_cli/commands.c
                    #./lib_cli/command_arm_core.c

                    ./lib_system/buffer.c
                    ./lib_system/comm_host_interface.c
                    ./lib_system/console_local_linux.c
                    ./lib_system/pipe.c
                    ./lib_system/pool.c
                    ./lib_system/socket_library_linux.c
					./lib_system/system_timer.c
                    ./lib_system/t_threads_linux.c 

                    ./memory/bus_ctrl.c
                    ./memory/io_ctrl.c
                    ./memory/memory_ctrl.c
                    ./memory/mmu_ctrl.c

                    ./peripherals/display_emulator.c
                    ./peripherals/emulator_api.c
                    ./peripherals/keypad_emulator.c
                    ./peripherals/m68681_emulator.c
                    ./peripherals/uart_emulator.c
                    ./peripherals/ym2149_emulator.c
                    ./peripherals/keyboard_matrix_emulator.c
                    ./peripherals/vdp9918_emulator.c
                    ./peripherals/vdp9958_emulator.c
                    
                    ./sdl/sdl_input.c
                    ./sdl/sdl_main.c
                    ./sdl/sdl_mouse.c
                    ./sdl/sdl_window.c

                    )

add_executable (    ${MODULE_NAME} 
                    ${SOURCE_FILES} )

target_link_libraries(${MODULE_NAME} SDL2 SDL2main m dl)

# The V9958 core can be exercised headlessly, without SDL or m68k-gcc.
include(CTest)
if(BUILD_TESTING)
    add_executable(vdp9958_test ./../tests/vdp9958_test.c ./peripherals/vdp9958_emulator.c)
    target_include_directories(vdp9958_test PRIVATE ./peripherals)
    add_test(NAME vdp9958_core COMMAND vdp9958_test)
endif()
