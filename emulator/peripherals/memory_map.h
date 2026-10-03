#ifndef _memory_map_h_
#define _memory_map_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

enum
{
    HW_BLOCK_UART,
    HW_BLOCK_TIMER,
    
    HW_BLOCK_KEYPAD,
    HW_BLOCK_VIDEO,

};

//////////////////////////////////////////////////////////////////////////////////////

#define ROM_BASE_ADDRESS            0
#define RAM_MMU_BASE_ADDRESS        (256 * 1024)
#define RAM_UPPER_BASE_ADDRESS      (512 * 1024)

#define ROM_MEMORY_SIZE		        (256 * 1024)
#define RAM_MMU_SIZE		        (192 * 1024)
#define RAM_UPPER_SIZE		        (512 * 1024)

// MMU
#define MMU_BASE_ADDRESS            0x00080000
#define MMU_REGISTER_BANK_BASE		(IO_BASE_ADDRESS + 0xF000)

// VGA data
#define VGA_SCAN_BASE_ADDR          0x000DA800
#define VGA_SCAN_LEN                320
#define VGA_SCAN_MAX                (1024 * 1024)

// IO space
#define IO_BASE_ADDRESS             0x70000
#define IO_SPACE_SIZE               (64 * 1024)

// UART list
#define UART_KEYPAD_ADDR            (IO_BASE_ADDRESS + 0x0000)
#define UART_PORT0_ADDR             (IO_BASE_ADDRESS + 0x0100)
#define UART_PORT1_ADDR             (IO_BASE_ADDRESS + 0x0200)

// timer controller
#define TIMER_CTRL_ADDR             (IO_BASE_ADDRESS + 0x0800)

// VDP space
#define VDP_CONTROL_ADDR            (IO_BASE_ADDRESS + 0x0C00)

//////////////////////////////////////////////////////////////////////////////////////

#endif // _memory_map_h_
