#ifndef _vdp9918_driver_h_
#define _vdp9918_driver_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

/* Dedicated glyph slot used to render the terminal cursor. 0x7F (DEL) is not
 * part of the printable font, so it is free to hold a custom cursor pattern. */
#define VDP_CURSOR_GLYPH        0x7F

enum
{
    VDP_CURSOR_BLOCK = 0,
    VDP_CURSOR_UNDERLINE,
};

//////////////////////////////////////////////////////////////////////////////////////

void vdp9918_text_mode_init(void);

void vdp9918_text_set_cursor_shape(uint8_t shape);

void vdp9918_text_clear_screen(void);

void vdp9918_text_char_out(char c_out, uint16_t char_posx, uint16_t char_posy, uint16_t fore_colour, uint16_t back_colour, bool_t is_transparent);
void vdp9918_text_string_out(const char * text, uint16_t char_posx, uint16_t char_posy, uint16_t fore_colour, uint16_t back_colour, bool_t is_transparent);

uint32_t vdp9918_get_text_height(char * text);

uint32_t vdp9918_get_text_width(char * text);

void vdp9918_text_clear_line(uint16_t char_posx, uint16_t char_posy);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _vdp9918_driver_h_
