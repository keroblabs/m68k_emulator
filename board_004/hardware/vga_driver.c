#include "vga_driver.h"
#include "memory_map.h"

//////////////////////////////////////////////////////////////////////////////////////

#define VGA_SCREEN_WIDTH		640
#define VGA_SCREEN_HEIGHT		480

//////////////////////////////////////////////////////////////////////////////////////

static volatile uint8_t * frame_buffer;

//////////////////////////////////////////////////////////////////////////////////////

void vga_driver_init(void)
{
    frame_buffer = (volatile uint8_t *)VGA_SCAN_BASE_ADDR;
}

uint16_t vga_get_width(void)
{
    return VGA_SCREEN_WIDTH;
}

uint16_t vga_get_height(void)
{
    return VGA_SCREEN_HEIGHT;
}

void vga_do_scroll(uint16_t height)
{
	uint32_t lcd_base_scroll;
	uint32_t scroll_size;

	scroll_size = (VGA_SCREEN_HEIGHT - height) * (VGA_SCREEN_WIDTH >> 1);
	lcd_base_scroll = height * (VGA_SCREEN_WIDTH >> 1);

	memcpy((uint8_t *)frame_buffer, (uint8_t *)&frame_buffer[lcd_base_scroll], scroll_size * sizeof(uint8_t));

	lcd_base_scroll = scroll_size;
	scroll_size = height * (VGA_SCREEN_WIDTH >> 1);
	memset((uint8_t *)&frame_buffer[lcd_base_scroll], 0x00, scroll_size * sizeof(uint8_t));
}

void vga_set_pixel(uint16_t posx, uint16_t posy, uint8_t colour)
{
    if (posx >= VGA_SCREEN_WIDTH) return;
    if (posy >= VGA_SCREEN_HEIGHT) return;

	uint32_t offset = (posy * (VGA_SCREEN_WIDTH >> 1)) + (posx >> 1);
    volatile uint8_t temp_pixel = frame_buffer[offset];

    if (!(posx & 0x01))
    {
        temp_pixel &= 0x0F;
        temp_pixel |= (colour << 4);
    }
    else
    {
        temp_pixel &= 0xF0;
        temp_pixel |= colour & 0x0F;
    }

    frame_buffer[offset] = temp_pixel;
}

void vga_show_bitmap_rgb(uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint8_t * bitmap, int use_transparent)
{

}

void vga_fill_colour(uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint8_t colour)
{
    uint16_t cnt_x;

    if (posx >= VGA_SCREEN_WIDTH) return;
    if (posy >= VGA_SCREEN_HEIGHT) return;

    if ((posx + width) >= VGA_SCREEN_WIDTH)
    {
        width = VGA_SCREEN_WIDTH - posx;
    }

    if ((posy + height) >= VGA_SCREEN_HEIGHT)
    {
        height = VGA_SCREEN_HEIGHT - posy;
    }

    uint8_t pixel_colour = (colour & 0x0F) | (colour << 4);

    uint32_t offset = posy * (VGA_SCREEN_WIDTH >> 1) + (posx >> 1);
    volatile uint8_t * temp_line = &frame_buffer[offset];
    uint16_t pixel_width = width >> 1;

    while (height)
    {
        for (cnt_x = 0; cnt_x < pixel_width; cnt_x++)
        {
            temp_line[cnt_x] = pixel_colour;
        }

        temp_line += (VGA_SCREEN_WIDTH >> 1);
        height--;
    }
}

uint8_t * vga_get_video_buffer(void)
{
    return (uint8_t *)frame_buffer;
}
