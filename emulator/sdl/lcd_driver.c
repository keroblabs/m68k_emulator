#include <SDL2/SDL.h>
#include "sdl_lib_export.h"
#include "sdl_main.h"
#include "sdl_window.h"
#include "sdl_input.h"

#include "types.h"
#include "device.h"
#include "system.h"
#include "lcd.h"

///////////////////////////////////////////////////////////////

#define SDL_SCREEN_WIDTH		_USE_SCREEN_WIDTH
#define SDL_SCREEN_HEIGHT		_USE_SCREEN_HEIGHT

#define LCD_SCREEN_WIDTH		(SDL_SCREEN_WIDTH / PIXEL_X_SCALE)
#define LCD_SCREEN_HEIGHT		(SDL_SCREEN_HEIGHT / PIXEL_Y_SCALE)

#define PIXEL_X_SCALE		2
#define PIXEL_Y_SCALE		2

///////////////////////////////////////////////////////////////

unsigned int mouse_x;
unsigned int mouse_y;
unsigned int mouse_button;
unsigned int mouse_trigg;

static uint32_t window_id;
static uint16_t *lcd_buffer = NULL;

///////////////////////////////////////////////////////////////

void keypad_driver_event_key( uint32_t window_id, uint8_t key_code, uint16_t control_key, uint8_t state );
void mouse_driver_event_mouse( uint32_t window_id, int32_t deltax, int32_t deltay, uint16_t posz );

///////////////////////////////////////////////////////////////

static void sdl_lcd_init( void )
{
	sdl_init();
    window_id = sdl_create_window("Tima Laptop", SDL_SCREEN_WIDTH, SDL_SCREEN_HEIGHT);
    lcd_buffer = sdl_get_buffer( window_id );

	sdl_window_set_key_event( window_id, keypad_driver_event_key );
	sdl_window_set_mouse_delta_event( window_id, mouse_driver_event_mouse );

	mouse_trigg = 0;
}

static bool_t sdl_lcd_init_process( void )
{
    return FALSE;
}

static void sdl_lcd_exit( void )
{
}

static void sdl_lcd_clear_screen( void )
{
}

static void sdl_lcd_set_contrast( uint8_t level )
{
}

static void sdl_lcd_set_backlight( bool_t state )
{
}

static void sdl_lcd_update_window( void )
{
	sdl_event_loop();
}

static void sdl_lcd_do_scroll( uint16_t height )
{
	uint32_t scroll_height = height * PIXEL_Y_SCALE;
	uint32_t lcd_base_scroll;
	uint32_t scroll_size;

	scroll_size = ( SDL_SCREEN_HEIGHT - scroll_height ) * SDL_SCREEN_WIDTH;
	lcd_base_scroll = scroll_height * SDL_SCREEN_WIDTH;

	memcpy( lcd_buffer, &lcd_buffer[ lcd_base_scroll ], scroll_size * sizeof( uint16_t ) );

	lcd_base_scroll = scroll_size;
	scroll_size = scroll_height * SDL_SCREEN_WIDTH;
	memset( &lcd_buffer[ lcd_base_scroll ], 0x00, scroll_size * sizeof( uint16_t ) );
}

static void sdl_lcd_set_pixel( uint16_t in_posx, uint16_t in_posy, uint16_t colour )
{
	uint16_t posx = in_posx * PIXEL_X_SCALE;
	uint16_t posy = in_posy * PIXEL_Y_SCALE;
	uint16_t repeat_x_cnt;
	uint16_t repeat_y_cnt;

	uint32_t lcd_offset = ( posy * SDL_SCREEN_WIDTH ) + posx;

	for( repeat_y_cnt = 0; repeat_y_cnt < PIXEL_Y_SCALE; repeat_y_cnt++ )
	{
		if( ( posy + repeat_y_cnt) >= SDL_SCREEN_HEIGHT )
		{
			break;
		}

		for( repeat_x_cnt = 0; repeat_x_cnt < PIXEL_X_SCALE; repeat_x_cnt++ )
		{
			if( ( posx + repeat_x_cnt ) >= SDL_SCREEN_WIDTH )
			{
				break;
			}

			lcd_buffer[ lcd_offset + repeat_x_cnt ] = colour;
		}

		lcd_offset += SDL_SCREEN_WIDTH;
	}
}

void sdl_lcd_show_bitmap_rgb( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t * bitmap, int use_transparent )
{
    uint16_t x, y;
    uint16_t colour16;
	uint16_t transparent;
	uint32_t bitmap_offset = 0; 

    if( lcd_buffer == NULL ) return;
	if( use_transparent != -1 )
	{
		transparent = ( uint16_t )use_transparent;
	}

	if( posx >= LCD_SCREEN_WIDTH )
	{
		return;
	}

    for( y = 0; y < height; y++ )
    {
		if( ( posy + y ) >= LCD_SCREEN_HEIGHT )
		{
			break;
		}

		for( x = 0; x < width; x++ )
		{
			if( ( posx + x ) >= LCD_SCREEN_WIDTH )
			{
				break;
			}

			colour16 = bitmap[bitmap_offset + x];

			if( ( colour16 != transparent ) || ( use_transparent == -1 ) )
			{
				sdl_lcd_set_pixel( posx + x, posy + y, colour16 );
			}
		}

		bitmap_offset += width;
    }    
}

static void sdl_lcd_fill_colour( uint16_t in_posx, uint16_t in_posy, uint16_t in_width, uint16_t in_height, uint16_t colour )
{
    uint16_t x, y;
	uint16_t posx = in_posx * PIXEL_X_SCALE;
	uint16_t posy = in_posy * PIXEL_Y_SCALE;
	uint16_t width = in_width * PIXEL_X_SCALE;
	uint16_t height = in_height * PIXEL_Y_SCALE;

    if( lcd_buffer == NULL ) return;

	uint32_t lcd_offset = ( posy * SDL_SCREEN_WIDTH ) + posx;

	if( posy >= SDL_SCREEN_HEIGHT )
	{
		return;
	}
   
    for( y = 0; y < height; y++ )
    {
		if( ( posy + y ) >= SDL_SCREEN_HEIGHT )
		{
			break;
		}

        for( x = 0; x < width; x++ )
        {
			if( ( posx + x ) >= SDL_SCREEN_WIDTH )
			{
				break;
			}
            
			lcd_buffer[ lcd_offset + x ] = colour;
        }

		lcd_offset += SDL_SCREEN_WIDTH;
    }    
}

static int sdl_read_mouse( uint16_t * posx, uint16_t * posy, uint16_t * posz )
{
	if( mouse_trigg == 1 )
	{
		mouse_trigg = 0;

		*posx = ( unsigned short )mouse_x;
		*posy = ( unsigned short )mouse_y;
		*posz = 0;

		if( mouse_button ) *posz = 100;
		return 1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////

static const lcd_driver_t sdl_lcd_driver = 
{
	LCD_SCREEN_WIDTH,
	LCD_SCREEN_HEIGHT,

	sdl_lcd_init,
	sdl_lcd_clear_screen,
	sdl_lcd_exit,
	sdl_lcd_update_window,
	sdl_lcd_init_process,

	sdl_lcd_set_contrast,
	sdl_lcd_set_backlight,
	sdl_lcd_do_scroll,

	NULL,

	NULL,

	sdl_lcd_set_pixel,
	sdl_lcd_show_bitmap_rgb,
	NULL,
	sdl_lcd_fill_colour,

	sdl_read_mouse
};

///////////////////////////////////////////////////////////////

#if defined TIMA_OS
SYSTEM_DRIVER_DEFINE_EX( lcd_driver_t, sdl_lcd_driver, lcd, "lcd", NULL );
#else
lcd_driver_t * lcd_driver = ( lcd_driver_t * )&sdl_lcd_driver;
#endif
