#include "vdp9918_driver.h"
#include "vdp9918_register.h"
#include "vdp9918_types.h"
//#include "font_6x8_v.h"
#include "memory_map.h"

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

static uint16_t prev_text_address;
static uint16_t prev_fore_colout;
static uint16_t prev_back_colour;

//////////////////////////////////////////////////////////////////////////////////////

void vpd9918_text_mode_init(void)
{
    prev_text_address = 0xFFFF;

    vdp_set_register(TMS_REG_0, TMS_R0_MODE_TEXT);
    vdp_set_register(TMS_REG_1, TMS_R1_MODE_TEXT | TMS_R1_RAM_16K);

    vdp_set_name_table_address(TMS_VRAM_NAME_ADDRESS);
    vdp_set_colour_table_address(TMS_VRAM_COLOR_ADDRESS);
    vdp_set_pattern_table_address(TMS_VRAM_PATT_ADDRESS);
    vdp_set_sprite_attr_table_address(TMS_VRAM_SPRITE_ATTR_ADDRESS);
    vdp_set_sprite_patt_table_address(TMS_VRAM_SPRITE_PATT_ADDRESS);
    vdp_set_reg_colour(TMS_COLOUR_WHITE, TMS_COLOUR_LT_BLUE);

    vdp_load_characters();
}

void vdp9918_text_clear_line(uint16_t char_posx, uint16_t char_posy)
{
    uint16_t char_addr = ((char_posy * TMS9918_PIXELS_X) + char_posx) + TMS_VRAM_NAME_ADDRESS;
    uint16_t total_char = (TMS9918_PIXELS_X / FONT_CHAR_WIDTH) - char_posx;

    vdp_set_address(char_addr);
    prev_text_address = 0x0FFFF;
    vpd_send_bytes_repeat(0x20, total_char);
}

void vdp9918_text_string_out(const char * text, uint16_t char_posx, uint16_t char_posy, uint16_t fore_colour, uint16_t back_colour, bool_t is_transparent)
{
    uint16_t char_addr = ((char_posy * TMS9918_PIXELS_X) + char_posx) + TMS_VRAM_NAME_ADDRESS;
    uint16_t index;
    uint16_t len;

    len = (uint16_t)strlen(text);
    vdp_set_address(char_addr);
    prev_text_address = 0x0FFFF;

    for (index = 0; index < len; index++)
    {
        uint8_t c_out;

        if (text[index] < FONT_CHAR_BASE)
        {
            c_out = '?';
        }
        else
        {
            c_out = text[index] - FONT_CHAR_BASE;
        }

        vdp_send_bytes(&c_out, 1);
    }
}

void vdp9918_text_char_out(char c_out, uint16_t char_posx, uint16_t char_posy, uint16_t fore_colour, uint16_t back_colour, bool_t is_transparent)
{
    uint16_t char_addr = ((char_posy * TMS9918_PIXELS_X) + char_posx) + TMS_VRAM_NAME_ADDRESS;

    if (prev_text_address != char_addr)
    {
        prev_text_address = char_addr + 1;
        vdp_set_address(char_addr);
    }

    c_out -= FONT_CHAR_BASE;

    vdp_send_bytes(&c_out, 1);
}

uint32_t vdp9918_get_text_height(char * text)
{
	uint32_t i;
	uint32_t len;

	len = 0;

	for (i = 0; i < strlen(text); i++)
	{
		if (len == 0) len = FONT_CHAR_HEIGHT;

		if (text[i] == 0x0a)
		{
			len += FONT_CHAR_HEIGHT;
		}
	}

	return len;
}

uint32_t vdp9918_get_text_width(char * text)
{
	uint32_t i;
	uint32_t len;
	uint32_t max_len;

	len = 0;
	max_len = 0;

	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == 0x0D)
		{
			if (max_len < len) max_len = len;
			len = 0;
		}
		else
		{
			len += FONT_CHAR_WIDTH;
		}
	}

	if (max_len < len) max_len = len;
	return max_len;
}

