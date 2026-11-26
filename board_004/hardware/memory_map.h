#ifndef _memory_map_h_
#define _memory_map_h_

#include "types.h"

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

/*
 * External data-bus width affects where an 8-bit peripheral appears in the
 * CPU byte address space; it does not change the peripheral access width.
 *
 * 68008-style 8-bit bus: MC68681 D0-D7, RS1..RS4 <- A0..A3
 *   register N -> base + N
 *
 * 68000/68010-style 16-bit bus, MC68681 on D0-D7 (lower byte lane):
 *   RS1..RS4 <- A1..A4 and chip select qualified with LDS
 *   register N -> base + 1 + (N * 2)
 */
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

/* Two MC68681 dual UARTs. Each allocation leaves ample room for either bus mapping. */
#define M68681_0_ADDR               (IO_BASE_ADDRESS + 0x0000)
#define M68681_1_ADDR               (IO_BASE_ADDRESS + 0x0100)

/* 0x70200-0x703ff is currently available/reserved. */

/* YM2149 PSG (address latch + data port). Shares 0x0400 window. */
#define YM2149_ADDR                 (IO_BASE_ADDRESS + 0x0400)

/* GPIO space */
#define GPIO_INPUT_ADDR             (IO_BASE_ADDRESS + 0x0400)
#define GPIO_OUTPUT_ADDR            (IO_BASE_ADDRESS + 0x0500)

/* VDP9918 space */
#define VDP9918_ADDR                (IO_BASE_ADDRESS + 0x0C00)

#endif /* _memory_map_h_ */
