#include <assert.h>
#include <stdio.h>
#include "vdp9958_emulator.h"

static void reg(Vdp9958 *v, unsigned r, unsigned value)
{
    vdp9958_port_write(v, 0, (uint8_t)value);
    vdp9958_port_write(v, 0, (uint8_t)(r | 0x80u));
}
static void address(Vdp9958 *v, unsigned at, int wr)
{
    reg(v, 14, at >> 14);
    vdp9958_port_write(v, 0, (uint8_t)at);
    vdp9958_port_write(v, 0, (uint8_t)(((at >> 8) & 63u) | (wr ? 64u : 0u)));
}
static void cmd(Vdp9958 *v, const uint8_t values[15])
{
    reg(v, 17, 32);
    for (unsigned i = 0; i < 15; i++) vdp9958_port_write(v, 3, values[i]);
}
int main(void)
{
    Vdp9958 *v = vdp9958_new();
    uint16_t pixels[VDP9958_WIDTH_MAX];
    uint8_t fill[15] = {0};
    assert(v != NULL);
    /* Direct register keeps bit 7; indirect auto-increments. */
    reg(v, 7, 0xf3);
    assert(vdp9958_peek_register(v, 7) == 0xf3);
    reg(v, 17, 18);
    vdp9958_port_write(v, 3, 0x35);
    vdp9958_port_write(v, 3, 0x56);
    assert(vdp9958_peek_register(v, 18) == 0x35);
    assert(vdp9958_peek_register(v, 19) == 0x56);
    /* R14 address extension; dummy-read buffer is pre-fetched. */
    reg(v, 0, 0x0e); /* G7 = 28 */
    address(v, 0x12345, 1);
    vdp9958_port_write(v, 1, 0x94);
    assert(vdp9958_peek_vram(v, 0x12345) == 0x94);
    address(v, 0x12345, 0);
    assert(vdp9958_port_read(v, 1) == 0x94);
    /* Second bank remains VRAM; expansion cannot be addressed via CPU ports. */
    assert(vdp9958_peek_expansion(v, 0x2345) == 0);
    /* R15 status and acknowledgement clears VBlank. */
    vdp9958_frame_complete(v);
    reg(v, 15, 0);
    assert(vdp9958_port_read(v, 0) & 0x80);
    assert(!(vdp9958_port_read(v, 0) & 0x80));
    reg(v, 15, 1);
    assert((vdp9958_port_read(v, 0) & 0x0e) == 4);
    /* Palette writes are pairs, then index wraps. */
    reg(v, 16, 15);
    vdp9958_port_write(v, 2, 0x75);
    vdp9958_port_write(v, 2, 0x03);
    assert(vdp9958_peek_register(v, 16) == 0);
    /* G4 4-bit 256px packed bitmap, logical fill, command-ready. */
    reg(v, 0, 0x06); /* G4 = 12 */
    reg(v, 1, 0x40);
    reg(v, 2, 0x1f);
    assert(vdp9958_width(v) == 256);
    assert(vdp9958_height(v) == 192);
    fill[4] = 8; fill[6] = 2; fill[8] = 4; fill[10] = 2;
    fill[12] = 0x66; fill[14] = 0xc0; /* HMMV 4x2 */
    cmd(v, fill);
    assert(vdp9958_peek_vram(v, 2 * 128 + 4) == 0x66);
    assert(vdp9958_peek_vram(v, 3 * 128 + 4) == 0x66);
    reg(v, 15, 2);
    assert(!(vdp9958_port_read(v, 0) & 1));
    vdp9958_render_line(v, 2, pixels);
    assert(pixels[8] == pixels[9]);
    /* Destination can be expansion DRAM via R45.MXD (bit 5). */
    fill[4] = 0; fill[6] = 0; fill[8] = 4; fill[10] = 1;
    fill[12] = 0x99; fill[13] = 0x20; cmd(v, fill);
    assert(vdp9958_peek_expansion(v, 0) == 0x99);
    assert(vdp9958_peek_vram(v, 0) != 0x99);
    /* HMMC transfers packed bytes; first R44 value starts the command. */
    fill[12] = 0xab; fill[13] = 0; fill[14] = 0xf0;
    fill[4] = 0; fill[6] = 0; fill[8] = 4; fill[10] = 1;
    cmd(v, fill);
    reg(v, 15, 2);
    assert(vdp9958_port_read(v, 0) & 1);
    reg(v, 44, 0xcd);
    assert(vdp9958_peek_vram(v, 0) == 0xab);
    assert(vdp9958_peek_vram(v, 1) == 0xcd);
    assert(!(vdp9958_port_read(v, 0) & 1));
    vdp9958_free(v);
    puts("V9958 register/VRAM/palette/command tests passed");
    return 0;
}