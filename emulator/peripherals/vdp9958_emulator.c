/* Yamaha V9958 register-level functional model for MC68K/8 Board 004.
 * Timing is frame-granular, not video-dot/VRAM-wait-state accurate.
 * The 64 KiB expansion memory is available to the command engine only. */
#include "vdp9958_emulator.h"
#include <stdlib.h>
#include <string.h>

struct Vdp9958 {
    uint8_t reg[64], status[10];
    uint8_t vram[VDP9958_VRAM_SIZE], expansion[VDP9958_EXP_SIZE];
    uint8_t palette[16][3];
    uint32_t address;
    uint8_t read_ahead, ctrl_first, ctrl_pending;
    uint8_t palette_first, palette_pending;
    uint16_t tx_x, tx_y;
    uint8_t transfer_command;
};

/* Standard MSX2 palette, R/G/B values in the 3-bit DAC domain. */
static const uint8_t default_palette[16][3] = {
    {0,0,0}, {0,0,0}, {1,6,1}, {3,7,3},
    {1,1,7}, {2,3,7}, {5,1,1}, {2,6,7},
    {7,1,1}, {7,3,3}, {6,6,1}, {6,6,4},
    {1,4,1}, {6,2,5}, {5,5,5}, {7,7,7}
};

static unsigned mode_code(const Vdp9958 *v)
{
    return ((v->reg[0] & 0x0Eu) << 1) | ((v->reg[1] & 8u) >> 2) |
           ((v->reg[1] & 16u) >> 4);
}

/* Values: TEXT1=1, TEXT2=9, G1=0, G2=4, MULTICOLOR=2,
 * G3=8, G4=12, G5=16, G6=20, G7=28. */
static unsigned bitmap_bpp(const Vdp9958 *v)
{
    switch (mode_code(v)) {
    case 12: case 20: return 4;
    case 16: return 2;
    case 28: return 8;
    default: return 0;
    }
}

static unsigned bitmap_width(const Vdp9958 *v)
{
    unsigned m = mode_code(v);
    return (m == 16 || m == 20) ? 512u : 256u;
}

static uint16_t rgb565(unsigned r, unsigned g, unsigned b)
{
    return (uint16_t)(((r * 31u / 7u) << 11) | ((g * 63u / 7u) << 5) | (b * 31u / 7u));
}

static uint16_t palette_pixel(const Vdp9958 *v, unsigned index)
{
    const uint8_t *c = v->palette[index & 15u];
    return rgb565(c[0], c[1], c[2]);
}

static unsigned bitmap_base(const Vdp9958 *v)
{
    /* R2 page selection: G4/G5 use 32KiB pages; G6/G7 use 64KiB. */
    unsigned m = mode_code(v);
    return (m == 12 || m == 16) ? ((v->reg[2] & 0x60u) << 10) :
           ((v->reg[2] & 0x40u) << 10);
}

static uint8_t *bank(Vdp9958 *v, int expansion)
{
    return expansion ? v->expansion : v->vram;
}

static uint8_t pixel_get(const Vdp9958 *v, unsigned x, unsigned y, int expansion)
{
    unsigned bpp = bitmap_bpp(v);
    unsigned width = bitmap_width(v), address, shift;
    const uint8_t *ram = expansion ? v->expansion : v->vram;
    if (!bpp || x >= width || y >= 1024u) return 0;
    address = (y * (width * bpp / 8u) + x * bpp / 8u) &
              (expansion ? 0xffffu : 0x1ffffu);
    shift = 8u - bpp - ((x * bpp) & 7u);
    return (uint8_t)((ram[address] >> shift) & ((1u << bpp) - 1u));
}

static void pixel_put(Vdp9958 *v, unsigned x, unsigned y, uint8_t color,
                      unsigned operation, int expansion)
{
    unsigned bpp = bitmap_bpp(v), width = bitmap_width(v), address, shift, mask;
    uint8_t *ram = bank(v, expansion), old, result;
    if (!bpp || x >= width || y >= 1024u) return;
    address = (y * (width * bpp / 8u) + x * bpp / 8u) &
              (expansion ? 0xffffu : 0x1ffffu);
    shift = 8u - bpp - ((x * bpp) & 7u);
    mask = (1u << bpp) - 1u;
    old = (uint8_t)((ram[address] >> shift) & mask);
    color &= (uint8_t)mask;
    if ((operation & 8u) && color == 0) return; /* transparent logic */
    switch (operation & 7u) {
    case 1: result = old & color; break;
    case 2: result = old | color; break;
    case 3: result = old ^ color; break;
    case 4: result = (uint8_t)(~color) & (uint8_t)mask; break;
    default: result = color; break;
    }
    ram[address] = (uint8_t)((ram[address] & ~(mask << shift)) | (result << shift));
}

static unsigned reg16(const Vdp9958 *v, unsigned lo)
{
    return (unsigned)v->reg[lo] | ((unsigned)v->reg[lo + 1u] << 8);
}

static void command_step(Vdp9958 *v, uint8_t data)
{
    unsigned dx = reg16(v, 36), dy = reg16(v, 38);
    unsigned nx = reg16(v, 40), ny = reg16(v, 42);
    unsigned arg = v->reg[45], cmd = v->transfer_command;
    int xd = (arg & 4u) ? -1 : 1, yd = (arg & 8u) ? -1 : 1;
    unsigned xx = (unsigned)((int)dx + (int)v->tx_x * xd);
    unsigned yy = (unsigned)((int)dy + (int)v->tx_y * yd);
    unsigned bpp = bitmap_bpp(v), pixels = cmd == 0xF0u ? 8u / bpp : 1u;
    unsigned i;
    for (i = 0; i < pixels; ++i) {
        uint8_t color = cmd == 0xF0u ?
            (uint8_t)((data >> (8u - bpp * (i + 1u))) & ((1u << bpp) - 1u)) : data;
        pixel_put(v, (unsigned)((int)xx + (int)i * xd), yy, color,
                  cmd == 0xB0u ? v->reg[46] & 15u : 0u, (arg & 0x20u) != 0);
    }
    v->tx_x = (uint16_t)(v->tx_x + pixels);
    if (v->tx_x >= nx) { v->tx_x = 0; v->tx_y++; }
    if (v->tx_y >= ny) {
        v->transfer_command = 0;
        v->status[2] &= (uint8_t)~0x81u; /* TR, CE */
    }
}

static void execute_command(Vdp9958 *v, uint8_t value)
{
    unsigned sx = reg16(v, 32), sy = reg16(v, 34), dx = reg16(v, 36), dy = reg16(v, 38);
    unsigned nx = reg16(v, 40), ny = reg16(v, 42), arg = v->reg[45];
    unsigned op = value & 15u, kind = value & 0xf0u;
    int xd = (arg & 4u) ? -1 : 1, yd = (arg & 8u) ? -1 : 1;
    unsigned x, y;
    v->transfer_command = 0;
    v->status[2] &= (uint8_t)~0x81u;
    if (kind == 0) return; /* STOP */
    if (kind == 0x40u) { /* POINT -> S7 */
        v->status[7] = pixel_get(v, sx, sy, (arg & 0x10u) != 0);
        return;
    }
    if (kind == 0x50u) { /* PSET */
        pixel_put(v, dx, dy, v->reg[44], op, (arg & 0x20u) != 0);
        return;
    }
    if (kind == 0xB0u || kind == 0xF0u) { /* LMMC/HMMC */
        if (!nx || !ny || !bitmap_bpp(v)) return;
        v->transfer_command = (uint8_t)kind;
        v->tx_x = 0;
        v->tx_y = 0;
        v->status[2] |= 0x81u; /* TR, CE: waiting for CPU */
        command_step(v, v->reg[44]); /* first byte is R44 */
        return;
    }
    if (!nx || !ny || !bitmap_bpp(v)) return;
    if (kind != 0x80u && kind != 0x90u && kind != 0xC0u &&
        kind != 0xD0u && kind != 0xE0u) return;
    /* A functional, immediate command engine. Real V9958 sets CE until done.
     * Reverse-axis overlapping copies must set DIX/DIY as in the hardware. */
    for (y = 0; y < ny; y++) {
        for (x = 0; x < nx; x++) {
            unsigned dest_x = (unsigned)((int)dx + (int)x * xd);
            unsigned dest_y = (unsigned)((int)dy + (int)y * yd);
            uint8_t color = v->reg[44];
            if (kind == 0x90u || kind == 0xD0u || kind == 0xE0u) {
                unsigned src_x = kind == 0xE0u ? dest_x : (unsigned)((int)sx + (int)x * xd);
                unsigned src_y = (unsigned)((int)sy + (int)y * yd);
                color = pixel_get(v, src_x, src_y, (arg & 0x10u) != 0);
            }
            if (kind == 0xC0u) {
                unsigned bpp = bitmap_bpp(v);
                unsigned shift = 8u - bpp - ((dest_x * bpp) & 7u);
                color = (uint8_t)((v->reg[44] >> shift) & ((1u << bpp) - 1u));
            }
            pixel_put(v, dest_x, dest_y, color,
                      (kind == 0x80u || kind == 0x90u) ? op : 0u,
                      (arg & 0x20u) != 0);
        }
    }
}

static void write_reg(Vdp9958 *v, unsigned r, uint8_t value)
{
    if (r >= 64 || r == 24 || (r >= 28 && r <= 31) || r > 46) return;
    v->reg[r] = value;
    if (r == 14) v->address = (v->address & 0x3fffu) | ((value & 7u) << 14);
    if (r == 46) execute_command(v, value);
    if (r == 44 && v->transfer_command) command_step(v, value);
}

Vdp9958 *vdp9958_new(void)
{
    Vdp9958 *v = (Vdp9958 *)calloc(1, sizeof(*v));
    if (v) vdp9958_reset(v);
    return v;
}

void vdp9958_free(Vdp9958 *v) { free(v); }

void vdp9958_reset(Vdp9958 *v)
{
    if (!v) return;
    memset(v->reg, 0, sizeof(v->reg));
    memset(v->status, 0, sizeof(v->status));
    memcpy(v->palette, default_palette, sizeof(v->palette));
    v->address = 0;
    v->ctrl_pending = v->palette_pending = v->read_ahead = 0;
    v->transfer_command = 0;
    v->status[1] = 0x04u; /* V9958 device ID = 2 in S1 bits 1..3 */
    v->status[2] = 0x0cu; /* constant bits 2 and 3 */
    /* DRAM intentionally not cleared by reset. */
}

static void advance_address(Vdp9958 *v)
{
    /* MSX1-compatible modes wrap every 16KiB; new modes traverse 128KiB. */
    unsigned mask = (mode_code(v) == 0 || mode_code(v) == 1 ||
                     mode_code(v) == 2 || mode_code(v) == 4) ? 0x3fffu : 0x1ffffu;
    v->address = (v->address & ~mask) | ((v->address + 1u) & mask);
    v->address &= 0x1ffffu;
    v->reg[14] = (uint8_t)((v->reg[14] & ~7u) | ((v->address >> 14) & 7u));
}

void vdp9958_port_write(Vdp9958 *v, unsigned port, uint8_t value)
{
    if (!v) return;
    switch (port) {
    case 0: /* control: first byte value/LSB; second register/write/read */
        if (!v->ctrl_pending) { v->ctrl_first = value; v->ctrl_pending = 1; }
        else {
            v->ctrl_pending = 0;
            if (value & 0x80u) write_reg(v, value & 0x3fu, v->ctrl_first);
            else {
                v->address = ((uint32_t)(v->reg[14] & 7u) << 14) |
                             ((uint32_t)(value & 0x3fu) << 8) | v->ctrl_first;
                if (!(value & 0x40u)) {
                    v->read_ahead = v->vram[v->address];
                    advance_address(v);
                }
            }
        }
        break;
    case 1:
        v->ctrl_pending = 0;
        v->vram[v->address & 0x1ffffu] = value;
        v->read_ahead = value;
        advance_address(v);
        break;
    case 2:
        v->ctrl_pending = 0;
        if (!v->palette_pending) {
            v->palette_first = value;
            v->palette_pending = 1;
        } else {
            unsigned i = v->reg[16] & 15u;
            v->palette[i][0] = (v->palette_first >> 4) & 7u;
            v->palette[i][1] = value & 7u;
            v->palette[i][2] = v->palette_first & 7u;
            v->reg[16] = (uint8_t)((i + 1u) & 15u);
            v->palette_pending = 0;
        }
        break;
    case 3:
        v->ctrl_pending = 0;
        if ((v->reg[17] & 0x3fu) != 17u) {
            write_reg(v, v->reg[17] & 0x3fu, value);
        }
        if (!(v->reg[17] & 0x80u))
            v->reg[17] = (uint8_t)((v->reg[17] & 0x80u) | ((v->reg[17] + 1u) & 0x3fu));
        break;
    default: break;
    }
}

uint8_t vdp9958_port_read(Vdp9958 *v, unsigned port)
{
    uint8_t result;
    unsigned i;
    if (!v) return 0xffu;
    if (port == 1) {
        v->ctrl_pending = 0;
        result = v->read_ahead;
        v->read_ahead = v->vram[v->address & 0x1ffffu];
        advance_address(v);
        return result;
    }
    if (port != 0) return 0xffu;
    v->ctrl_pending = 0;
    i = v->reg[15] & 15u;
    if (i > 9) return 0xffu;
    result = v->status[i];
    if (i == 0) v->status[0] &= 0x1fu; /* clear VBlank/overflow/collision */
    if (i == 1) v->status[1] &= (uint8_t)~1u; /* line IRQ latch */
    return result;
}

unsigned vdp9958_width(const Vdp9958 *v)
{
    if (!v) return 256;
    if (mode_code(v) == 9) return 512; /* Text 2: 80 columns */
    return bitmap_bpp(v) ? bitmap_width(v) : 256;
}

unsigned vdp9958_height(const Vdp9958 *v)
{
    if (!v) return 192;
    return (v->reg[9] & 0x80u) ? 212u : 192u;
}

/* Text 1 and legacy Graphics I/II rendering, plus bitmap G4-G7.
 * Sprites, scrolling, interlace and YJK encoding are future accuracy gates. */
void vdp9958_render_line(const Vdp9958 *v, unsigned y, uint16_t *out)
{
    unsigned width, m, x, bg, fg, base, bpp, tile_row, row;
    if (!v || !out) return;
    width = vdp9958_width(v);
    m = mode_code(v);
    bg = v->reg[7] & 15u;
    fg = v->reg[7] >> 4;
    for (x = 0; x < width; x++) out[x] = palette_pixel(v, bg);
    if (!(v->reg[1] & 0x40u) || y >= vdp9958_height(v)) return;
    bpp = bitmap_bpp(v);
    if (bpp) {
        base = bitmap_base(v);
        for (x = 0; x < width; x++) {
            unsigned index = pixel_get(v, x, y, 0);
            if (m == 28) {
                /* G7 / SCREEN 8: 3:3:2 RGB, independent of programmable palette. */
                unsigned address = (base + y * width + x) & 0x1ffffu;
                index = v->vram[address];
                unsigned r = (index >> 2) & 7u;
                unsigned g = (index >> 5) & 7u;
                unsigned b = (index & 3u) * 7u / 3u;
                out[x] = rgb565(r, g, b);
            } else {
                /* The bitmap base selects the visible page; CPU VRAM accesses
                 * retain their own independent address counter. */
                unsigned address = (base + y * (width * bpp / 8u) + x * bpp / 8u) & 0x1ffffu;
                unsigned shift = 8u - bpp - ((x * bpp) & 7u);
                uint8_t color = (v->vram[address] >> shift) & ((1u << bpp) - 1u);
                out[x] = palette_pixel(v, color);
            }
        }
        return;
    }
    tile_row = y >> 3;
    row = y & 7u;
    if (m == 1 || m == 9) { /* text 40/80 columns, 6 pixel glyphs */
        unsigned cols = (m == 9) ? 80u : 40u;
        unsigned pad = (m == 9) ? 16u : 8u;
        unsigned names = ((unsigned)(v->reg[2] & 0x7fu) << 10) & 0x1ffffu;
        unsigned patterns = ((unsigned)(v->reg[4] & 0x3fu) << 11) & 0x1ffffu;
        for (x = 0; x < cols; x++) {
            unsigned glyph = v->vram[(names + tile_row * cols + x) & 0x1ffffu];
            uint8_t bits = v->vram[(patterns + glyph * 8u + row) & 0x1ffffu];
            unsigned z;
            for (z = 0; z < 6u; z++) {
                unsigned at = pad + x * 6u + z;
                if (at < width) out[at] = palette_pixel(v, (bits & (0x80u >> z)) ? fg : bg);
            }
        }
    } else if (m == 0 || m == 4 || m == 8) { /* graphic 1/2/3 pattern modes */
        unsigned names = ((unsigned)(v->reg[2] & 15u) << 10) & 0x1ffffu;
        unsigned patterns = ((unsigned)(v->reg[4] & 7u) << 11) & 0x1ffffu;
        unsigned colors = ((unsigned)v->reg[3] << 6) & 0x1ffffu;
        unsigned gfx2 = (m != 0);
        for (x = 0; x < 32u; x++) {
            unsigned glyph = v->vram[(names + tile_row * 32u + x) & 0x1ffffu];
            unsigned third = gfx2 ? ((tile_row >> 3) * 2048u) : 0u;
            unsigned patt_off = third + glyph * 8u + row;
            uint8_t bits = v->vram[(patterns + patt_off) & 0x1ffffu];
            uint8_t color = v->vram[(colors + (gfx2 ? patt_off : glyph / 8u)) & 0x1ffffu];
            unsigned z;
            for (z = 0; z < 8u; z++) {
                unsigned c = (bits & (0x80u >> z)) ? (color >> 4) : (color & 15u);
                out[x * 8u + z] = palette_pixel(v, c ? c : bg);
            }
        }
    } else if (m == 2) { /* multicolor */
        unsigned names = ((unsigned)(v->reg[2] & 15u) << 10) & 0x1ffffu;
        unsigned patterns = ((unsigned)(v->reg[4] & 7u) << 11) & 0x1ffffu;
        for (x = 0; x < 32u; x++) {
            unsigned glyph = v->vram[(names + tile_row * 32u + x) & 0x1ffffu];
            unsigned off = (tile_row & 3u) * 2u + ((y & 4u) >> 2);
            uint8_t c = v->vram[(patterns + glyph * 8u + off) & 0x1ffffu];
            unsigned z;
            for (z = 0; z < 8u; z++) out[x * 8u + z] = palette_pixel(v, (z < 4u) ? (c >> 4) : (c & 15u));
        }
    }
}

void vdp9958_frame_complete(Vdp9958 *v)
{
    if (v) v->status[0] |= 0x80u; /* VBlank is generated even with IRQ disabled */
}

int vdp9958_irq_pending(const Vdp9958 *v)
{
    return v && (((v->status[0] & 0x80u) && (v->reg[1] & 0x20u)) ||
                 ((v->status[1] & 1u) && (v->reg[0] & 0x10u)));
}

uint8_t vdp9958_peek_vram(const Vdp9958 *v, uint32_t address)
{
    return v ? v->vram[address & 0x1ffffu] : 0xffu;
}
uint8_t vdp9958_peek_expansion(const Vdp9958 *v, uint32_t address)
{
    return v ? v->expansion[address & 0xffffu] : 0xffu;
}
uint8_t vdp9958_peek_register(const Vdp9958 *v, unsigned reg)
{
    return (v && reg < 64) ? v->reg[reg] : 0xffu;
}