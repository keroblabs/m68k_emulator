#ifndef _vdp9918_register_h_
#define _vdp9918_register_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

#define TMS_VRAM_NAME_ADDRESS          0x3800
#define TMS_VRAM_COLOR_ADDRESS         0x0000
#define TMS_VRAM_PATT_ADDRESS          0x2000
#define TMS_VRAM_SPRITE_ATTR_ADDRESS   0x3B00
#define TMS_VRAM_SPRITE_PATT_ADDRESS   0x1800

#define FONT_CHAR_WIDTH                 6
#define FONT_CHAR_HEIGHT                8
#define FONT_CHAR_BASE                  0x20

//////////////////////////////////////////////////////////////////////////////////////

void vdp_send_bytes(uint8_t *buffer, uint16_t len);

void vpd_send_bytes_repeat(uint8_t value, uint16_t len);

void vdp_set_address(uint16_t address);

void vdp_set_register(uint8_t address, uint8_t value);

void vdp_load_characters(void);

void vdp_set_colour(uint8_t fore, uint8_t back);

void vdp_set_name_table_address(uint16_t address);

void vdp_set_colour_table_address(uint16_t address);

void vdp_set_pattern_table_address(uint16_t address);

void vdp_set_sprite_attr_table_address(uint16_t address);

void vdp_set_sprite_patt_table_address(uint16_t address);

void vdp_set_reg_colour(uint8_t fore, uint8_t back);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _vdp9918_register_h_
