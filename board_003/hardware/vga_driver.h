#ifndef _vga_driver_h_
#define _vga_driver_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

enum
{
	COLOUR_BLACK,
	COLOUR_RED,
	COLOUR_DARK_BLUE,
	COLOUR_PURPLE,
	COLOUR_DARK_GREEN,
	COLOUR_DARK_GRAY,
	COLOUR_MEDIUM_BLUE,
	COLOUR_LIGHT_BLUE,
	COLOUR_BROWN,
	COLOUR_ORANGE,
	COLOUR_GRAY,
	COLOUR_PINK,
	COLOUR_GREEN,
	COLOUR_YELLOW,
	COLOUR_AQUA,
	COLOUR_WHITE,
};

//////////////////////////////////////////////////////////////////////////////////////

void vga_driver_init(void);

void vga_do_scroll(uint16_t height);

void vga_set_pixel(uint16_t in_posx, uint16_t in_posy, uint8_t colour);

void vga_show_bitmap_rgb(uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint8_t * bitmap, int use_transparent);

void vga_fill_colour(uint16_t in_posx, uint16_t in_posy, uint16_t in_width, uint16_t in_height, uint8_t colour);

void vga_show_bitmap_1bb(uint16_t posx, uint16_t posy, uint16_t width, uint16_t heigth, 
					     uint8_t *input, uint8_t back_colour, uint8_t fore_colour);

uint8_t * vga_get_video_buffer(void);

uint16_t vga_get_width(void);
uint16_t vga_get_height(void);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _vga_driver_h_
