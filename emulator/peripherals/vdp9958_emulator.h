#ifndef M68K_VDP9958_EMULATOR_H
#define M68K_VDP9958_EMULATOR_H

#include <stdint.h>

typedef struct Vdp9958 Vdp9958;

#define VDP9958_WIDTH_MAX    512
#define VDP9958_HEIGHT_MAX   212
#define VDP9958_VRAM_SIZE    0x20000u
#define VDP9958_EXP_SIZE     0x10000u

Vdp9958 *vdp9958_new(void);
void vdp9958_free(Vdp9958 *vdp);
void vdp9958_reset(Vdp9958 *vdp);

/* Board 004 port order: 0=control/status, 1=VRAM data, 2=palette,
 * 3=indirect register. Only byte-wide accesses have defined behaviour. */
void vdp9958_port_write(Vdp9958 *vdp, unsigned port, uint8_t value);
uint8_t vdp9958_port_read(Vdp9958 *vdp, unsigned port);
unsigned vdp9958_width(const Vdp9958 *vdp);
unsigned vdp9958_height(const Vdp9958 *vdp);
void vdp9958_render_line(const Vdp9958 *vdp, unsigned y, uint16_t *rgb565);
void vdp9958_frame_complete(Vdp9958 *vdp);
int vdp9958_irq_pending(const Vdp9958 *vdp);

/* Diagnostics and headless tests: VRAM and expansion VRAM are separate. */
uint8_t vdp9958_peek_vram(const Vdp9958 *vdp, uint32_t address);
uint8_t vdp9958_peek_expansion(const Vdp9958 *vdp, uint32_t address);
uint8_t vdp9958_peek_register(const Vdp9958 *vdp, unsigned reg);

#endif