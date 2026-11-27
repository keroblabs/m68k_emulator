#ifndef BOARD004_VDP9958_DRIVER_H
#define BOARD004_VDP9958_DRIVER_H

#include "types.h"

/* Board 004 wiring: A0/A1 decoded to CPU ports in the order below.
 * The physical V9958 MODE0/MODE1 pins must match this mapping. */
#define VDP9958_PORT_CONTROL   0u
#define VDP9958_PORT_DATA      1u
#define VDP9958_PORT_PALETTE   2u
#define VDP9958_PORT_INDIRECT  3u

#define VDP9958_VRAM_BYTES       (128UL * 1024UL)
#define VDP9958_EXP_VRAM_BYTES   (64UL * 1024UL)

/* Direct control-register writes are two transactions: value, 0x80|register.
 * Keep the pair atomic against any ISR that reads VDP status. */
void vdp9958_init(void);
void vdp9958_write_register(uint8_t reg, uint8_t value);
void vdp9958_set_vram_address(uint32_t address, bool_t write);
void vdp9958_write_data(uint8_t value);
uint8_t vdp9958_read_data(void);
void vdp9958_write_bytes(const uint8_t *data, uint16_t count);
void vdp9958_fill_bytes(uint8_t value, uint16_t count);
uint8_t vdp9958_read_status(uint8_t status_index);
void vdp9958_set_palette(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void vdp9958_set_indirect_register(uint8_t reg, bool_t inhibit_increment);
void vdp9958_write_indirect(uint8_t value);

/* R32..R46: SX, SY, DX, DY, NX, NY (little-endian pairs), color, argument, command.
 * Poll S2.CE before issuing a new command; the caller must supply valid coordinates. */
#define VDP9958_CMD_STOP    0x00u
#define VDP9958_CMD_POINT   0x40u
#define VDP9958_CMD_PSET    0x50u
#define VDP9958_CMD_SRCH    0x60u
#define VDP9958_CMD_LINE    0x70u
#define VDP9958_CMD_LMMV    0x80u
#define VDP9958_CMD_LMMM    0x90u
#define VDP9958_CMD_LMCM    0xA0u
#define VDP9958_CMD_LMMC    0xB0u
#define VDP9958_CMD_HMMV    0xC0u
#define VDP9958_CMD_HMMM    0xD0u
#define VDP9958_CMD_YMMM    0xE0u
#define VDP9958_CMD_HMMC    0xF0u

bool_t vdp9958_command_ready(void);
void vdp9958_command(const uint8_t params[15]);

#endif