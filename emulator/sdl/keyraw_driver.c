#include <SDL2/SDL.h>
#include "sdl_lib_export.h"
#include "sdl_main.h"
#include "sdl_window.h"
#include "sdl_input.h"

#include "types.h"
#include "device.h"
#include "system.h"
#include "buffer.h"

///////////////////////////////////////////////////////////////////////////

#define KEYPAD_BUFFER_SIZE              20

///////////////////////////////////////////////////////////////////////////

static buffer_data_t keypad_buffer_data;
static uint8_t keypad_buffer[ KEYPAD_BUFFER_SIZE ];

///////////////////////////////////////////////////////////////////////////

void keypad_driver_event_key( uint32_t window_id, uint8_t key_code, uint16_t control_key, uint8_t state )
{
    if( key_code == 0 )
    {
        buffer_write( &keypad_buffer_data, 0x01 );
        buffer_write( &keypad_buffer_data, ( uint8_t )control_key );
    }
    else if( key_code == 1 )
    {
        bool_t valid_key = FALSE;

        switch( control_key )
        {
            case EVENT_KEY_BACKSPACE:
            case EVENT_KEY_ENTER:
            case EVENT_KEY_ESC:
            case EVENT_KEY_UP:
            case EVENT_KEY_DOWN:
            case EVENT_KEY_RIGHT:
            case EVENT_KEY_LEFT:
                valid_key = TRUE;
                break;

            default:
                valid_key = FALSE;
                break;            
        }

        if( valid_key == TRUE )
        {
            buffer_write( &keypad_buffer_data, ( state == KEY_DOWN ) ? 0x02 : 0x03 );

            switch( control_key )
            {
                case EVENT_KEY_BACKSPACE:
                    buffer_write( &keypad_buffer_data, 127 );
                    break;

                case EVENT_KEY_ENTER:
                    buffer_write( &keypad_buffer_data, 13 );
                    break;

                case EVENT_KEY_ESC:
                    buffer_write( &keypad_buffer_data, '\x1b' );
                    break;

                case EVENT_KEY_UP:
                    buffer_write( &keypad_buffer_data, 11 );
                    break;

                case EVENT_KEY_DOWN:
                    buffer_write( &keypad_buffer_data, 10 );
                    break;

                case EVENT_KEY_RIGHT:
                    buffer_write( &keypad_buffer_data, 21 );
                    break;

                case EVENT_KEY_LEFT:
                    buffer_write( &keypad_buffer_data, 8 );
                    break;            
            }
        }

    }
    else if( ( key_code >= 0x20 ) && ( key_code <= 125 ) )
    {
        buffer_write( &keypad_buffer_data, ( state == KEY_DOWN ) ? 0x02 : 0x03 );
        buffer_write( &keypad_buffer_data, key_code );
    }
}

///////////////////////////////////////////////////////////////////////////

static bool_t validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    return FALSE;
}

static void keypad_init( uint32_t index )
{
    buffer_init( &keypad_buffer_data, keypad_buffer, KEYPAD_BUFFER_SIZE );
}

static uint32_t keypad_read_byte( uint32_t socket, uint8_t *buffer, uint32_t max_size )
{
	int ret;

	ret = (int)buffer_get_size( &keypad_buffer_data );
	if( ( buffer != NULL ) && ( ret != 0 ) )
	{
		ret = (int)buffer_read_buffer( &keypad_buffer_data, buffer, max_size );
	}

    return ret;
}

static uint32_t keypad_write_byte( uint32_t socket, uint8_t *buffer, uint32_t max_size )
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t keypad_device =
{
    "keypad*",
    &_in_use,
    
    keypad_init,
    NULL,
    NULL,
    validate,
    NULL,
    NULL,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    keypad_read_byte,
    keypad_write_byte,
    
};

DECLARE_DEVICE_SECTION( keypad_device );
