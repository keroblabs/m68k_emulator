#ifndef _DRAW_H
#define _DRAW_H

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

typedef struct _font_data_t
{
	uint16_t width;
	uint16_t height;
	uint16_t offset;
	uint16_t size;

	const uint8_t * data;

} font_data_t;

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

void draw_bitmap_1bb(uint16_t posx, uint16_t posy, uint16_t width, uint16_t heigth, 
					 uint8_t *input, uint8_t back_colour, uint8_t fore_colour);

void draw_text(uint16_t posx, uint16_t posy, char * text, uint16_t fore_colour, uint16_t back_colour);

void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t colour);

uint32_t draw_get_text_height(char * text);
uint32_t draw_get_text_width(char * text);

void draw_clear_screen(void);

void draw_fill_colour(uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint8_t colour);

void draw_do_scroll(uint16_t height);

void draw_gr_character(uint8_t character, 
						uint16_t posx, uint16_t posy, 
						uint16_t fore_colour, uint16_t back_colour, 
						bool_t transparent);

//////////////////////////////////////////////////////////////////////////////////////

#endif
