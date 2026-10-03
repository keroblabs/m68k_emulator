#include "vdp9918_register.h"
#include "vdp9918_types.h"
#include "font_6x8_v.h"
#include "memory_map.h"

//////////////////////////////////////////////////////////////////////////////////////

typedef struct vdp_regs_t_
{
    uint8_t control;
    uint8_t data;

} vdp_regs_t;

//////////////////////////////////////////////////////////////////////////////////////

static volatile vdp_regs_t * vdp_regs;

//////////////////////////////////////////////////////////////////////////////////////

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

void vpd_send_bytes_repeat(uint8_t value, uint16_t len)
{
    uint16_t index;

    for (index = 0; index < len; index++)
    {
        vdp_regs->data = value;
    }
}

void vdp_set_address(uint16_t address)
{
    vdp_regs->control = address & 0x0FF;
    vdp_regs->control = address >> 8;
}

void vdp_set_register(uint8_t address, uint8_t value)
{
    vdp_regs->control = value & 0x07F;
    vdp_regs->control = (address & 0x07F) | 0x80;
}

void vdp_load_characters(void)
{
    uint8_t single_char[FONT_CHAR_HEIGHT];
    uint8_t char_index;
    uint8_t byte_index;

    vdp_set_address(TMS_VRAM_SPRITE_PATT_ADDRESS);

    for (char_index = FONT_CHAR_BASE; char_index <= 0xFF; char_index++)
    {
        char_shift(char_index, single_char);
        vdp_send_bytes(single_char, FONT_CHAR_HEIGHT);
    }
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
