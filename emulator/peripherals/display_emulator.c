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
#include "vdp9958_emulator.h"

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

/* Standard TMS9918A palette, indexed by the 4-bit colour code. */
static const uint16_t tms9918_palette[16] =
{
    APP_RGB(   0,   0,   0 ),   // 0  transparent (rendered as black)
    APP_RGB(   0,   0,   0 ),   // 1  black
    APP_RGB(  33, 200,  66 ),   // 2  medium green
    APP_RGB(  94, 220, 120 ),   // 3  light green
    APP_RGB(  84,  85, 237 ),   // 4  dark blue
    APP_RGB( 125, 118, 252 ),   // 5  light blue
    APP_RGB( 212,  82,  77 ),   // 6  dark red
    APP_RGB(  66, 235, 245 ),   // 7  cyan
    APP_RGB( 252,  85,  84 ),   // 8  medium red
    APP_RGB( 255, 121, 120 ),   // 9  light red
    APP_RGB( 212, 193,  84 ),   // 10 dark yellow
    APP_RGB( 230, 206, 128 ),   // 11 light yellow
    APP_RGB(  33, 176,  59 ),   // 12 dark green
    APP_RGB( 201,  91, 186 ),   // 13 magenta
    APP_RGB( 204, 204, 204 ),   // 14 gray
    APP_RGB( 255, 255, 255 ),   // 15 white
};

//////////////////////////////////////////////////////////////////////////////////////

static do_events_t local_do_event;
static VrEmuTms9918 * vdp9918;
#ifdef M68K_VIDEO_V9958
static Vdp9958 *vdp9958;
#endif
static uint8_t video_mode;

//////////////////////////////////////////////////////////////////////////////////////

static void mouse_driver_event_mouse( uint32_t window_id, int32_t deltax, int32_t deltay, uint16_t posz )
{

}

#define VDP_SCREEN_SCALE        5
#define VDP_SCREEN_OFFSET_X     0
#define VDP_SCREEN_OFFSET_Y     0

static void display_emulator_vdp9918_process( uint32_t window_id )
{
    uint8_t line_buffer[ TMS9918_PIXELS_X ];
    uint16_t vdp_posy;

    local_do_event( window_id);
    
    display_data_t * display_data = ( display_data_t * )sdl_window_get_user_data( window_id );

    if( ++display_data->delay_counter < SDL_SCAN_COUNTER )
    {
        return;
    }

    display_data->delay_counter = 0;

    /*
     * Render the whole frame from the current VRAM snapshot in a single pass.
     * Rendering one scanline at a time across many iterations would sample
     * VRAM while the firmware is still mutating it (echoing characters,
     * moving the cursor, scrolling), producing a torn/garbled image.
     */
    display_data->curr_lcd_line = display_data->lcd_buffer;
    display_data->curr_lcd_line += (SDL_SCREEN_WIDTH * VDP_SCREEN_OFFSET_Y);

    for (vdp_posy = 0; vdp_posy < TMS9918_PIXELS_Y; vdp_posy++)
    {
        vrEmuTms9918ScanLine(vdp9918, (uint8_t)vdp_posy, line_buffer);

        for (int scale_y = 0; scale_y < VDP_SCREEN_SCALE; scale_y++)
        {
            uint16_t lcd_posx = VDP_SCREEN_OFFSET_X;
            uint16_t vdp_posx;

            for (vdp_posx = 0; vdp_posx < TMS9918_PIXELS_X; vdp_posx++)
            {
                uint16_t colour = tms9918_palette[ line_buffer[ vdp_posx ] & 0x0F ];

                for (int scale_x = 0; scale_x < VDP_SCREEN_SCALE; scale_x++)
                {
                    display_data->curr_lcd_line[ lcd_posx++ ] = colour;
                }
            }

            display_data->curr_lcd_line += SDL_SCREEN_WIDTH;
        }
    }

    sdl_update_window(window_id);
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

#ifdef M68K_VIDEO_V9958
static void display_emulator_vdp9958_process(uint32_t window_id)
{
    display_data_t *display_data = (display_data_t *)sdl_window_get_user_data(window_id);
    uint16_t line_buffer[VDP9958_WIDTH_MAX];
    unsigned width, height, scale, offset_x, offset_y, y, x, sy, sx;

    local_do_event(window_id);
    if (++display_data->delay_counter < SDL_SCAN_COUNTER)
        return;
    display_data->delay_counter = 0;

    width = vdp9958_width(vdp9958);
    height = vdp9958_height(vdp9958);
    scale = SDL_SCREEN_WIDTH / width;
    if (scale > SDL_SCREEN_HEIGHT / height)
        scale = SDL_SCREEN_HEIGHT / height;
    if (scale == 0)
        scale = 1;
    offset_x = (SDL_SCREEN_WIDTH - width * scale) / 2;
    offset_y = (SDL_SCREEN_HEIGHT - height * scale) / 2;

    memset(display_data->lcd_buffer, 0, SDL_SCREEN_WIDTH * SDL_SCREEN_HEIGHT * sizeof(uint16_t));
    for (y = 0; y < height; ++y)
    {
        vdp9958_render_line(vdp9958, y, line_buffer);
        for (sy = 0; sy < scale; ++sy)
        {
            uint16_t *dst = display_data->lcd_buffer
                          + (offset_y + y * scale + sy) * SDL_SCREEN_WIDTH + offset_x;
            for (x = 0; x < width; ++x)
                for (sx = 0; sx < scale; ++sx)
                    *dst++ = line_buffer[x];
        }
    }
    vdp9958_frame_complete(vdp9958);
    sdl_update_window(window_id);
}
#endif

static void display_emulator_process( uint32_t window_id )
{
#ifdef M68K_VIDEO_V9958
    display_emulator_vdp9958_process(window_id);
#else
    if (video_mode == 0)
    {
        display_emulator_vga_process(window_id);
    }
    else
    {
        display_emulator_vdp9918_process(window_id);
    }
#endif
}

static uint32_t display_register_access(uint32_t address, uint32_t data, uint8_t size, uint8_t access)
{
    uint32_t vdp_address = address - VDP_CONTROL_ADDR;
    uint32_t ret = 0;

#ifdef M68K_VIDEO_V9958
    if (size != 1 || vdp_address > 3)
        return 0xFF;
    if (access == BUS_ACCESS_READ)
        return vdp9958_port_read(vdp9958, (unsigned)vdp_address);
    vdp9958_port_write(vdp9958, (unsigned)vdp_address, (uint8_t)data);
    return 0;
#else
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
#endif
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

    video_mode = 1;
#ifdef M68K_VIDEO_V9958
    vdp9958 = vdp9958_new();
    if (vdp9958 == NULL)
        return NEG_U32;
#else
    vdp9918 = vrEmuTms9918New();
    if (vdp9918 == NULL)
        return NEG_U32;
#endif

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
#ifdef M68K_VIDEO_V9958
    io_ctrl_set_handler( VDP_CONTROL_ADDR + 3, display_register_access );
#endif

    return display_data->window_id;
}
