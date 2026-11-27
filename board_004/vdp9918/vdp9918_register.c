#include "vdp9918_register.h"
#include "vdp9918_types.h"
#include "font_6x8_v.h"
#include "memory_map.h"
#ifdef USE_V9958
#include "vdp9958_driver.h"
#endif

//////////////////////////////////////////////////////////////////////////////////////

typedef struct vdp_regs_t_
{
    uint8_t control;
    uint8_t data;

} vdp_regs_t;

//////////////////////////////////////////////////////////////////////////////////////

static volatile vdp_regs_t * vdp_regs = (volatile vdp_regs_t *)VDP9918_ADDR;

//////////////////////////////////////////////////////////////////////////////////////

void vdp_init(void)
{
    vdp_regs = (volatile vdp_regs_t *)VDP9918_ADDR;
#ifdef USE_V9958
    vdp9958_init();
#endif
}

static void char_shift(uint8_t index, uint8_t * char_out)
{
    uint16_t offset = (index - FONT_CHAR_BASE) * FONT_CHAR_WIDTH;
    uint8_t mask;
    uint8_t row_index;

    for (mask = 0; mask < FONT_CHAR_HEIGHT; mask++)
    {
        char_out[mask] = 0;
        for (row_index = 0; row_index < FONT_CHAR_WIDTH; row_index++)
        {
            char_out[mask] <<= 1; 
            char_out[mask] |= (char_map_6x8[offset + row_index] & (1 << mask)) ? 0x01 : 0x00;
        }

        /*
         * The font data is stored 6 bits wide in bit positions 5..0.
         * In TMS9918 TEXT mode each pattern byte is scanned MSB first and
         * only the top six pixels are displayed, so shift the six pixels up
         * into bit positions 7..2.
         */
        char_out[mask] <<= (8 - FONT_CHAR_WIDTH);
    }        
}

void vdp_send_bytes(uint8_t *buffer, uint16_t len)
{
    uint16_t index;

    for (index = 0; index < len; index++)
    {
        vdp_regs->data = buffer[index];
    }
}

void vdp_send_bytes_repeat(uint8_t value, uint16_t len)
{
    uint16_t index;

    for (index = 0; index < len; index++)
    {
        vdp_regs->data = value;
    }
}

void vdp_set_address(uint16_t address)
{
#ifdef USE_V9958
    vdp9958_set_vram_address((uint32_t)address, TRUE);
#else
    /* Second control byte bit 6 selects VRAM write (not read/prefetch). */
    vdp_regs->control = (uint8_t)address;
    vdp_regs->control = (uint8_t)(((address >> 8) & 0x3Fu) | 0x40u);
#endif
}

void vdp_set_register(uint8_t address, uint8_t value)
{
#ifdef USE_V9958
    vdp9958_write_register(address, value);
#else
    /* Preserve all eight bits of the value; R0/R1 have meaningful bit 7. */
    vdp_regs->control = value;
    vdp_regs->control = (uint8_t)((address & 0x07u) | 0x80u);
#endif
}

void vdp_load_characters(void)
{
    uint8_t single_char[FONT_CHAR_HEIGHT];
    uint16_t char_index;

    vdp_set_address(TMS_VRAM_PATT_ADDRESS);

    for (char_index = FONT_CHAR_BASE; char_index <= 0xFF; char_index++)
    {
        char_shift((uint8_t)char_index, single_char);
        vdp_send_bytes(single_char, FONT_CHAR_HEIGHT);
    }
}

void vdp_set_glyph_pattern(uint8_t glyph, const uint8_t * pattern)
{
    uint16_t offset = TMS_VRAM_PATT_ADDRESS + ((uint16_t)(glyph - FONT_CHAR_BASE) * FONT_CHAR_HEIGHT);

    vdp_set_address(offset);
    vdp_send_bytes((uint8_t *)pattern, FONT_CHAR_HEIGHT);
}

void vdp_set_colour(uint8_t fore, uint8_t back)
{
    vdp_regs->data = (fore << 4) | (back & 0x0F);
}

void vdp_set_name_table_address(uint16_t address)
{
    vdp_set_register(TMS_REG_NAME_TABLE, (uint8_t)(address >> 10));
}

void vdp_set_colour_table_address(uint16_t address)
{
    vdp_set_register(TMS_REG_COLOR_TABLE, (uint8_t)(address >> 6));
}

void vdp_set_pattern_table_address(uint16_t address)
{
    vdp_set_register(TMS_REG_PATTERN_TABLE, (uint8_t)(address >> 11));
}

void vdp_set_sprite_attr_table_address(uint16_t address)
{
    vdp_set_register(TMS_REG_SPRITE_ATTR_TABLE, (uint8_t)(address >> 7));
}

void vdp_set_sprite_patt_table_address(uint16_t address)
{
    vdp_set_register(TMS_REG_SPRITE_PATT_TABLE, (uint8_t)(address >> 11));
}

void vdp_set_reg_colour(uint8_t fore, uint8_t back)
{
    uint8_t vdp_colour = (fore << 4) | (back & 0x0F);
    vdp_set_register(TMS_REG_FG_BG_COLOR, vdp_colour);
}
