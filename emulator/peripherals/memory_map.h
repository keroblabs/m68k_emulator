#ifndef _memory_map_h_
#define _memory_map_h_

#include "types.h"

enum
{
    HW_BLOCK_UART,
    HW_BLOCK_TIMER,

    HW_BLOCK_KEYPAD,
    HW_BLOCK_VIDEO,
};

#define ROM_BASE_ADDRESS            0
#define RAM_MMU_BASE_ADDRESS        (256 * 1024)
#define RAM_UPPER_BASE_ADDRESS      (512 * 1024)

#define ROM_MEMORY_SIZE             (256 * 1024)
#define RAM_MMU_SIZE                (192 * 1024)
#define RAM_UPPER_SIZE              (512 * 1024)

/* MMU */
#define MMU_BASE_ADDRESS            0x00080000
#define MMU_REGISTER_BANK_BASE      (IO_BASE_ADDRESS + 0xF000)

/* VGA data */
#define VGA_SCAN_BASE_ADDR          0x000DA800
#define VGA_SCAN_LEN                320
#define VGA_SCAN_MAX                (1024 * 1024)

/* IO space */
#define IO_BASE_ADDRESS             0x70000
#define IO_SPACE_SIZE               (64 * 1024)

/* Keep the emulator byte-address layout identical to board_003 firmware. */
#ifndef M68K_DATA_BUS_WIDTH
#define M68K_DATA_BUS_WIDTH         8
#endif

#if (M68K_DATA_BUS_WIDTH == 8)
#define M68K_IO_REG_STRIDE          1u
#define M68K_IO_BYTE_LANE_OFFSET    0u
#elif (M68K_DATA_BUS_WIDTH == 16)
#define M68K_IO_REG_STRIDE          2u
#define M68K_IO_BYTE_LANE_OFFSET    1u
#else
#error "M68K_DATA_BUS_WIDTH must be 8 or 16"
#endif

#define M68K_IO_BYTE_REG_ADDR(base, reg) \
    ((uint32_t)(base) + M68K_IO_BYTE_LANE_OFFSET + ((uint32_t)(reg) * M68K_IO_REG_STRIDE))

#define M68681_0_ADDR               (IO_BASE_ADDRESS + 0x0000)
#define M68681_1_ADDR               (IO_BASE_ADDRESS + 0x0100)

/* Existing generic timer controller: retained temporarily. */
#define TIMER_CTRL_ADDR             (IO_BASE_ADDRESS + 0x0800)

/* VDP space */
#define VDP_CONTROL_ADDR            (IO_BASE_ADDRESS + 0x0C00)

#endif /* _memory_map_h_ */
