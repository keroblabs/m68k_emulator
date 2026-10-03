#include <SDL2/SDL.h>
#include "sdl_lib_export.h"
#include "sdl_main.h"
#include "sdl_window.h"
#include "sdl_input.h"

#include "memory_map.h"
#include "keypad_emulator.h"
#include "memory_ctrl.h"
#include "bus_ctrl.h"
#include "io_ctrl.h"

#include "vdp9918_emulator.h"

//////////////////////////////////////////////////////////////////////////////////////

#define SDL_SCREEN_WIDTH		    _USE_SCREEN_WIDTH
#define SDL_SCREEN_HEIGHT		    _USE_SCREEN_HEIGHT

#define BLACK_COLOUR_DEFAULT        APP_RGB(  32,  32,  32 )
#define WHITE_COLOUR_DEFAULT        APP_RGB( 255, 255, 255 )

#define SDL_SCAN_COUNTER            64

//////////////////////////////////////////////////////////////////////////////////////

typedef struct display_data_t_
{
    uint32_t window_id;
    uint16_t *lcd_buffer;
    uint16_t *curr_lcd_line;
    uint32_t scan_address;
    uint16_t posy;
    uint16_t vdp_posy;
    uint16_t delay_counter;

} display_data_t;

//////////////////////////////////////////////////////////////////////////////////////

static const uint16_t pixel_lores[16] =
{
    BLACK_COLOUR_DEFAULT,       // 0
    APP_RGB( 128,  25,  96 ),   // 1
    APP_RGB(  65,  45, 225 ),   // 2
    APP_RGB( 200,  60, 255 ),   // 3

    APP_RGB(   0, 128,  16 ),   // 4
    APP_RGB( 128, 128, 128 ),   // 5
    APP_RGB(  36, 128, 255 ),   // 6
    APP_RGB( 160, 160, 255 ),   // 7

    APP_RGB(  80,  80,   0 ),   // 8
    APP_RGB( 240,  96,   0 ),   // 9
    APP_RGB( 190, 190, 190 ),   // 10
    APP_RGB( 255, 128, 255 ),   // 11

    APP_RGB(   0, 200,   0 ),   // 12
    APP_RGB( 200, 200,   0 ),   // 13
    APP_RGB(  80, 255, 128 ),   // 14
    WHITE_COLOUR_DEFAULT        // 15
};

//////////////////////////////////////////////////////////////////////////////////////

static do_events_t local_do_event;
static VrEmuTms9918 * vdp9918;
static uint8_t video_mode;

//////////////////////////////////////////////////////////////////////////////////////

static void mouse_driver_event_mouse( uint32_t window_id, int32_t deltax, int32_t deltay, uint16_t posz )
{

}

#define VDP_SCREEN_SCALE        5
#define VDP_SCREEN_OFFSET_X     0
#define VDP_SCREEN_OFFSET_Y     0

static void display_emulator_vpd9918_process( uint32_t window_id )
{
    uint32_t counter;
    uint8_t line_buffer[ TMS9918_PIXELS_X ];

    local_do_event( window_id);
    
    display_data_t * display_data = ( display_data_t * )sdl_window_get_user_data( window_id );

    if( ++display_data->delay_counter < SDL_SCAN_COUNTER )
    {
        return;
    }

    display_data->delay_counter = 0;

    vrEmuTms9918ScanLine(vdp9918, display_data->vdp_posy, line_buffer);
    uint16_t lcd_posx = 0;
    uint16_t vdp_posx = 0;

    for (int scale_y = 0; scale_y < VDP_SCREEN_SCALE; scale_y++)
    {
        lcd_posx = VDP_SCREEN_OFFSET_X;

        for (vdp_posx = 0; vdp_posx < TMS9918_PIXELS_X; vdp_posx++)    
        {
            for (int scale_x = 0; scale_x < VDP_SCREEN_SCALE; scale_x++)    
            {
                display_data->curr_lcd_line[ lcd_posx++ ] = pixel_lores[ vdp_posx ];
            }
        }

        display_data->curr_lcd_line += SDL_SCREEN_WIDTH;
    }
    
    display_data->vdp_posy++;
    
    if (display_data->vdp_posy >= TMS9918_PIXELS_Y)
    {
        display_data->vdp_posy = 0;
        display_data->curr_lcd_line = display_data->lcd_buffer;
        display_data->curr_lcd_line += (SDL_SCREEN_WIDTH * VDP_SCREEN_OFFSET_Y);
        sdl_update_window(window_id);
    }
}

static void display_emulator_vga_process( uint32_t window_id )
{
    uint32_t counter;
    uint8_t line_buffer[ VGA_SCAN_LEN ];

    local_do_event( window_id);
    
    display_data_t * display_data = ( display_data_t * )sdl_window_get_user_data( window_id );

    if( ++display_data->delay_counter < SDL_SCAN_COUNTER )
    {
        return;
    }

    display_data->delay_counter = 0;

    memory_ctrl_ram_read_block( display_data->scan_address, line_buffer, VGA_SCAN_LEN );

    uint16_t lcd_posx = 0;

    for( counter = 0; counter < VGA_SCAN_LEN; counter++ )
    {
        uint8_t colour = line_buffer[ counter ];

        display_data->curr_lcd_line[ lcd_posx++ ] = pixel_lores[ colour >> 4 ];
        display_data->curr_lcd_line[ lcd_posx++ ] = pixel_lores[ colour >> 4 ];
        display_data->curr_lcd_line[ lcd_posx++ ] = pixel_lores[ colour & 0x0F ];
        display_data->curr_lcd_line[ lcd_posx++ ] = pixel_lores[ colour & 0x0F ];
    }

    if( display_data->posy & 0x01 )
    {
        display_data->scan_address += VGA_SCAN_LEN;
    }

    display_data->curr_lcd_line += SDL_SCREEN_WIDTH;
    display_data->posy++;

    if (display_data->scan_address >= VGA_SCAN_MAX )
    {
        display_data->posy = 0;
        display_data->scan_address = VGA_SCAN_BASE_ADDR;
        display_data->curr_lcd_line = display_data->lcd_buffer;

        sdl_update_window( window_id );
    }
}

static void display_emulator_process( uint32_t window_id )
{
    if (video_mode == 0)
    {
        display_emulator_vga_process(window_id);
    }
    else
    {
        display_emulator_vpd9918_process(window_id);
    }
}

static uint32_t display_register_access(uint32_t address, uint32_t data, uint8_t size, uint8_t access)
{
    uint32_t vdp_address = address - VDP_CONTROL_ADDR;
    uint32_t ret = 0;

    switch (vdp_address)
    {
        case 0:
            if (access == BUS_ACCESS_READ)
            {
                ret = vrEmuTms9918ReadStatus(vdp9918);
            }
            else
            {
                vrEmuTms9918WriteAddr(vdp9918, (uint8_t)data & 0x0FF);
            }
            break;

        case 1:
            if (access == BUS_ACCESS_READ)
            {
                ret = vrEmuTms9918ReadData(vdp9918);
            }
            else
            {
                vrEmuTms9918WriteData(vdp9918, (uint8_t)data & 0x0FF);
            }
            break;

        case 2:
            if (access == BUS_ACCESS_READ)
            {
                ret = video_mode;
            }
            else
            {
                video_mode = data & 0x01;
            }
            break;
    }

    return ret;
}

uint32_t display_emulator_init( void )
{
	sdl_init();

    uint32_t window_id = sdl_create_window("M68K Emulator", SDL_SCREEN_WIDTH, SDL_SCREEN_HEIGHT);
    if( window_id == NEG_U32 )
    {
        return NEG_U32;
    }

    display_data_t * display_data = ( display_data_t * )sdl_windows_malloc( window_id, HW_BLOCK_VIDEO, sizeof( display_data_t ) );
    if( display_data == NULL )
    {
        return NEG_U32;
    }

    video_mode = 0;
    vdp9918 = vrEmuTms9918New();

    display_data->window_id = window_id;
    display_data->lcd_buffer = sdl_get_buffer( display_data->window_id );
    display_data->curr_lcd_line = display_data->lcd_buffer;
    display_data->curr_lcd_line += (SDL_SCREEN_WIDTH * VDP_SCREEN_OFFSET_Y);
    display_data->scan_address = VGA_SCAN_BASE_ADDR;
    display_data->posy = 0;
    display_data->vdp_posy = 0;
    display_data->delay_counter = 0;

    sdl_window_set_user_data( display_data->window_id, display_data );
	sdl_window_set_key_event( display_data->window_id, keypad_driver_event_key );
	sdl_window_set_mouse_delta_event( display_data->window_id, mouse_driver_event_mouse );

    local_do_event = sdl_window_set_background_event( display_data->window_id, display_emulator_process );

    io_ctrl_set_handler( VDP_CONTROL_ADDR + 0, display_register_access );
    io_ctrl_set_handler( VDP_CONTROL_ADDR + 1, display_register_access );
    io_ctrl_set_handler( VDP_CONTROL_ADDR + 2, display_register_access );

    return display_data->window_id;
}
