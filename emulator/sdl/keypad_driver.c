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
#define CTRL_KEY(k)                     ((k) & 0x1f)

#define KEYPAD_CONFIG_SEND_CONTROL      0x01
#define KEYPAD_CONFIG_SHIFT_EFFECT      0x02
#define KEYPAD_CONFIG_CTRL_EFFECT       0x04
#define KEYPAD_CONFIG_CAPS_EFFECT       0x08
#define KEYPAD_CONFIG_VT100             0x10

///////////////////////////////////////////////////////////////////////////

typedef struct sdl_to_vt100_t_
{
    uint32_t event;
    const char * vt100;

} sdl_to_vt100_t;

static const sdl_to_vt100_t sdl_to_vt100[] = 
{
    { EVENT_KEY_HOME,       "\x1b[1~" },
    { EVENT_KEY_END,        "\x1b[4~" },
    { EVENT_KEY_DELETE,     "\x1b[3~" },
    { EVENT_KEY_PAGEUP,     "\x1b[5~" },
    { EVENT_KEY_PAGEDOWN,   "\x1b[6~" },

    { EVENT_KEY_UP,         "\x1b[A" },
    { EVENT_KEY_DOWN,       "\x1b[B" },
    { EVENT_KEY_RIGHT,      "\x1b[C" },
    { EVENT_KEY_LEFT,       "\x1b[D" },
    { 0,                    NULL     }
};

///////////////////////////////////////////////////////////////////////////

static uint16_t local_control_key;

static buffer_data_t keypad_buffer_data;
static uint8_t keypad_buffer[ KEYPAD_BUFFER_SIZE ];

static buffer_data_t command_buffer_data;
static uint8_t command_buffer[ KEYPAD_BUFFER_SIZE ];

static uint8_t keypad_config;

///////////////////////////////////////////////////////////////////////////

static void send_vt100_code( buffer_data_t * buffer_data, uint32_t event )
{
    uint8_t counter = 0;

    while( sdl_to_vt100[ counter ].vt100 != NULL )
    {
        if( sdl_to_vt100[ counter ].event == event )
        {
            uint32_t size = strlen( sdl_to_vt100[ counter ].vt100 );
            buffer_write_buffer( buffer_data, ( uint8_t * )sdl_to_vt100[ counter ].vt100, size );
            break;
        }

        counter++;
    }
}

void keypad_driver_event_key( uint32_t window_id, uint8_t key_code, uint16_t control_key, uint8_t state )
{
    if( key_code == 0 )
    {
        if( control_key != local_control_key )
        {
            uint8_t counter;

            if( keypad_config & KEYPAD_CONFIG_SEND_CONTROL )
            {
                for( counter = 0; counter < 8; counter++ )
                {
                    uint8_t shift_bit = 1 << counter;

                    if( ( control_key & shift_bit ) != ( local_control_key & shift_bit ) )
                    {
                        buffer_write( &keypad_buffer_data, '\x1b' );
                        buffer_write( &keypad_buffer_data, '[' );
                        buffer_write( &keypad_buffer_data, '&' );
                        buffer_write( &keypad_buffer_data, counter + '0' );
                        buffer_write( &keypad_buffer_data, ( control_key & shift_bit ) ? 'U' : 'D' );
                    }
                }
            }

            local_control_key = control_key;
        }
    }
    else if( key_code == 1 )
    {
        if( state != KEY_DOWN ) return;

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
        }

        if( keypad_config & KEYPAD_CONFIG_VT100 )
        {
            switch( control_key )
            {
                case EVENT_KEY_HOME:
                case EVENT_KEY_END:
                case EVENT_KEY_DELETE:
                case EVENT_KEY_PAGEUP:
                case EVENT_KEY_PAGEDOWN:
                case EVENT_KEY_UP:
                case EVENT_KEY_DOWN:
                case EVENT_KEY_RIGHT:
                case EVENT_KEY_LEFT:
                    send_vt100_code( &keypad_buffer_data, control_key );
                    break;            
            }
        }
        else
        {
            switch( control_key )
            {
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
    else
    {
        if( state != KEY_DOWN ) return;
        
        if(    ( local_control_key & SHIFT_FLAG ) 
            && ( keypad_config & KEYPAD_CONFIG_SHIFT_EFFECT )
            && ( key_code >= 'a' ) && ( key_code <= 'z' ) )
        {
            key_code -= 0x20;
        }
        else if(    ( local_control_key & CTRL_FLAG ) 
                 && ( keypad_config & KEYPAD_CONFIG_CTRL_EFFECT ) 
                 && ( ( ( key_code >= 'a' ) && ( key_code <= 'z' ) ) || ( ( key_code >= 'A' ) && ( key_code <= 'Z' ) ) ) )
        {
            key_code &= 0x1F;
        }

        buffer_write( &keypad_buffer_data, key_code );
    }
}

static void keypad_command_process( void )
{
    while( buffer_is_empty( &command_buffer_data ) == FALSE ) 
    {
        uint8_t command = buffer_read( &command_buffer_data );

        switch( command )
        {
            case 0x30:
                keypad_config &= ~KEYPAD_CONFIG_SEND_CONTROL;
                break;

            case 0x31:
                keypad_config |= KEYPAD_CONFIG_SEND_CONTROL;
                break;

            case 0x40:
                keypad_config &= ~KEYPAD_CONFIG_SHIFT_EFFECT;
                break;

            case 0x41:
                keypad_config |= KEYPAD_CONFIG_SHIFT_EFFECT;
                break;

            case 0x42:
                keypad_config &= ~KEYPAD_CONFIG_CTRL_EFFECT;
                break;

            case 0x43:
                keypad_config |= KEYPAD_CONFIG_CTRL_EFFECT;
                break;

            case 0x44:
                keypad_config &= ~KEYPAD_CONFIG_CAPS_EFFECT;
                break;

            case 0x45:
                keypad_config |= KEYPAD_CONFIG_CAPS_EFFECT;
                break;

            case 0x46:
                keypad_config &= ~KEYPAD_CONFIG_VT100;
                break;

            case 0x47:
                keypad_config |= KEYPAD_CONFIG_VT100;
                break;
        }
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
    buffer_init( &command_buffer_data, command_buffer, KEYPAD_BUFFER_SIZE );

    keypad_config = KEYPAD_CONFIG_SHIFT_EFFECT | KEYPAD_CONFIG_CTRL_EFFECT | KEYPAD_CONFIG_VT100;
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
    return buffer_write_buffer( &command_buffer_data, buffer, max_size );
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
DECLARE_PROCESS_SECTION( keypad_command_process );
