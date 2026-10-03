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
                    ./peripherals/uart_emulator.c
                    ./peripherals/timer_emulator.c
                    ./peripherals/vdp9918_emulator.c
                    
                    ./sdl/sdl_input.c
                    ./sdl/sdl_main.c
                    ./sdl/sdl_mouse.c
                    ./sdl/sdl_window.c

                    )

add_executable (    ${MODULE_NAME} 
                    ${SOURCE_FILES} )

target_link_libraries(${MODULE_NAME} SDL2 SDL2main m dl)

# TODO: Add tests and install targets if needed.
