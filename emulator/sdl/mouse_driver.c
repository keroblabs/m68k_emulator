#include <SDL2/SDL.h>
#include "sdl_lib_export.h"
#include "sdl_main.h"
#include "sdl_window.h"
#include "sdl_input.h"

#include "types.h"
#include "device.h"
#include "system.h"
#include "buffer.h"

///////////////////////////////////////////////////////////////

#define DEFAULT_BUFFER_SIZE         64
#define MOUSE_MAX_DEV               2
#define SCALE_RANGE                 4096

///////////////////////////////////////////////////////////////////////////

typedef struct _command_pos_t_
{
	uint16_t val_x;
	uint16_t val_y;
} param_pos_t;

typedef struct mouse_config_t_
{
	param_pos_t min_pos;
	param_pos_t max_pos;
	param_pos_t curr_pos;
    uint16_t posz;

} mouse_config_t;

typedef struct mouse_drv_t_
{
    bool_t is_init;

    buffer_data_t output_buffer_data;
    uint8_t output_buffer[ DEFAULT_BUFFER_SIZE ];

    buffer_data_t command_buffer_data;
    uint8_t command_buffer[ DEFAULT_BUFFER_SIZE ];

    int32_t scale_mul;

    int16_t raw_posx;
    int16_t raw_posy;
    uint8_t raw_posz;

} mouse_drv_t;

///////////////////////////////////////////////////////////////

static mouse_drv_t mouse_drv_list[ MOUSE_MAX_DEV ];

///////////////////////////////////////////////////////////////////////////

void mouse_send_coordinates( mouse_drv_t * mouse_drv )
{
    if (mouse_drv->is_init == FALSE ) return;
    
    buffer_write( &mouse_drv->output_buffer_data, 0x04 );
    buffer_write( &mouse_drv->output_buffer_data, ( uint8_t )mouse_drv->raw_posx & 0x0FF );

    buffer_write( &mouse_drv->output_buffer_data, 0x05 );
    buffer_write( &mouse_drv->output_buffer_data, ( uint8_t )mouse_drv->raw_posy & 0x0FF );

    buffer_write( &mouse_drv->output_buffer_data, 0x06 );
    buffer_write( &mouse_drv->output_buffer_data, ( uint8_t )mouse_drv->raw_posz & 0x0FF );
}

void mouse_driver_event_mouse( uint32_t window_id, int32_t deltax, int32_t deltay, uint16_t posz )
{
    for( int index = 0; index < MOUSE_MAX_DEV; index++ )
    {
        mouse_drv_t * mouse_drv = &mouse_drv_list[ index ];

        mouse_drv->raw_posx = ( int16_t )( ( deltax * mouse_drv->scale_mul ) / SCALE_RANGE );
        mouse_drv->raw_posy = ( int16_t )( ( deltay * mouse_drv->scale_mul ) / SCALE_RANGE );
        mouse_drv->raw_posz = posz ? 100 : 0;

        mouse_send_coordinates( mouse_drv );
    }
}

///////////////////////////////////////////////////////////////////////////

static bool_t mouse_validate( uint32_t index )
{
    if( index < MOUSE_MAX_DEV ) return TRUE;
    return FALSE;
}

static void mouse_global_init( void )
{
    for( int index = 0; index < MOUSE_MAX_DEV; index++ )
    {
        mouse_drv_t * mouse_drv = &mouse_drv_list[ index ];
        mouse_drv->is_init = FALSE;
    }
}

static void mouse_init( uint32_t index )
{
    mouse_drv_t * mouse_drv = &mouse_drv_list[ index ];

    memset( mouse_drv, 0x00, sizeof( mouse_drv_t ) );

    buffer_init( &mouse_drv->output_buffer_data, mouse_drv->output_buffer, DEFAULT_BUFFER_SIZE );
    buffer_init( &mouse_drv->command_buffer_data, mouse_drv->command_buffer, DEFAULT_BUFFER_SIZE );

    mouse_drv->scale_mul = SCALE_RANGE;
    mouse_drv->is_init = TRUE;
}

static uint32_t mouse_read_byte( uint32_t index, uint8_t *buffer, uint32_t max_size )
{
    mouse_drv_t * mouse_drv = &mouse_drv_list[ index ];
    uint32_t size = buffer_get_size( &mouse_drv->output_buffer_data );

    if( buffer != NULL )
    {
        size = buffer_read_buffer( &mouse_drv->output_buffer_data, buffer, max_size );
    }

    return size;
}

static void mouse_command_process( mouse_drv_t * mouse_drv )
{
    uint32_t read_size = buffer_get_size( &mouse_drv->command_buffer_data );

    if( read_size == 0 ) return;

    uint8_t cmd_byte = buffer_peek( &mouse_drv->command_buffer_data );

    switch( cmd_byte )
    {
        case 0x50:
            if( read_size >= ( 2 + sizeof( int32_t )) )
            {
                buffer_read( &mouse_drv->command_buffer_data ); // take command from buffer
                uint8_t next_param = buffer_read( &mouse_drv->command_buffer_data );

                switch( next_param )
                {
                    case 0x01:
                        buffer_read_buffer( &mouse_drv->command_buffer_data, ( uint8_t * )&mouse_drv->scale_mul, sizeof( int32_t ) );
                        break;
                }
            }
            break;

        case 0x51:
            if( read_size >= 2 )
            {
                buffer_read( &mouse_drv->command_buffer_data ); // take command from buffer
                uint8_t cursor_state = buffer_read( &mouse_drv->command_buffer_data );
                SDL_ShowCursor( cursor_state ? SDL_DISABLE : SDL_ENABLE );
            }
            break;

        default:
            buffer_read( &mouse_drv->command_buffer_data ); // take command from buffer
            break;            
    }
}

static void mouse_process( void )
{
    for( int index = 0; index < MOUSE_MAX_DEV; index++ )
    {
        mouse_drv_t * mouse_drv = &mouse_drv_list[ index ];
        mouse_command_process( mouse_drv );
    }
}

static uint32_t mouse_write_byte( uint32_t index, uint8_t *buffer, uint32_t max_size )
{
    mouse_drv_t * mouse_drv = &mouse_drv_list[ index ];
    buffer_write_buffer( &mouse_drv->command_buffer_data, buffer, max_size );

	return max_size;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t mouse_device =
{
    "mouse*",
    &_in_use,
    
    mouse_init,
    NULL,
    NULL,
    mouse_validate,
    NULL,
    NULL,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    mouse_read_byte,
    mouse_write_byte,
    
};

DECLARE_DEVICE_SECTION( mouse_device );
DECLARE_PROCESS_SECTION( mouse_process );
DECLARE_INIT_DEVICE_SECTION( mouse_global_init );
