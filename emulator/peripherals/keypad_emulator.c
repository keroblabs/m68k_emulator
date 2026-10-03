#include <SDL2/SDL.h>
#include "sdl_lib_export.h"
#include "sdl_main.h"
#include "sdl_window.h"
#include "sdl_input.h"

#include "uart_emulator.h"
#include "memory_map.h"

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

typedef struct keypad_data_t_
{
    struct keypad_data_t_ * p_next;

    buffer_data_t rx_buffer_data;
    uint8_t rx_buffer[ KEYPAD_BUFFER_SIZE ];

    uint16_t local_control_key;
    uint8_t keypad_config;

    uint32_t window_id;
    void * p_uart;

} keypad_data_t;

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

    { EVENT_KEY_UP,         "\x1b[A"  },
    { EVENT_KEY_DOWN,       "\x1b[B"  },
    { EVENT_KEY_RIGHT,      "\x1b[C"  },
    { EVENT_KEY_LEFT,       "\x1b[D"  },
    { 0,                    NULL      }
};

///////////////////////////////////////////////////////////////////////////

static do_events_t local_do_event;

///////////////////////////////////////////////////////////////////////////

static void send_vt100_code( buffer_data_t * keypad_buffer, uint32_t event )
{
    uint8_t counter = 0;

    while( sdl_to_vt100[ counter ].vt100 != NULL )
    {
        if( sdl_to_vt100[ counter ].event == event )
        {
            uint32_t size = strlen( sdl_to_vt100[ counter ].vt100 );
            buffer_write_buffer( keypad_buffer, ( uint8_t * )sdl_to_vt100[ counter ].vt100, size );
            break;
        }

        counter++;
    }
}

void keypad_driver_event_key( uint32_t window_id, uint8_t key_code, uint16_t control_key, uint8_t state )
{
    keypad_data_t * keypad_data = ( keypad_data_t * )sdl_windows_get_heap_data( window_id, HW_BLOCK_KEYPAD );
    if( keypad_data == NULL )
    {
        return;
    }

    if( key_code == 0 )
    {
        if( control_key != keypad_data->local_control_key )
        {
            uint8_t counter;

            if( keypad_data->keypad_config & KEYPAD_CONFIG_SEND_CONTROL )
            {
                for( counter = 0; counter < 8; counter++ )
                {
                    uint8_t shift_bit = 1 << counter;

                    if( ( control_key & shift_bit ) != ( keypad_data->local_control_key & shift_bit ) )
                    {
                        char ctrl_code[10];
                        int len = sprintf( ctrl_code, "\x1b[&%c%c", counter + '0', ( control_key & shift_bit ) ? 'U' : 'D' );
                        buffer_write_buffer( &keypad_data->rx_buffer_data, ( uint8_t * )ctrl_code, ( uint32_t )len );
                    }
                }
            }

            keypad_data->local_control_key = control_key;
        }
    }
    else if( key_code == 1 )
    {
        if( state != KEY_DOWN ) return;

        switch( control_key )
        {
            case EVENT_KEY_BACKSPACE: buffer_write( &keypad_data->rx_buffer_data, 127 );    break;
            case EVENT_KEY_ENTER:     buffer_write( &keypad_data->rx_buffer_data, 13 );     break;
            case EVENT_KEY_ESC:       buffer_write( &keypad_data->rx_buffer_data, '\x1b' ); break;
        }

        if( keypad_data->keypad_config & KEYPAD_CONFIG_VT100 )
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
                    send_vt100_code( keypad_data->p_uart, control_key );
                    break;            
            }
        }
        else
        {
            switch( control_key )
            {
                case EVENT_KEY_UP:
                case EVENT_KEY_DOWN:
                case EVENT_KEY_RIGHT:
                case EVENT_KEY_LEFT:  
                    send_vt100_code( &keypad_data->rx_buffer_data, control_key );  
                    break;            
            }
        }
    }
    else
    {
        if( state != KEY_DOWN ) return;
        
        if(    ( keypad_data->local_control_key & SHIFT_FLAG ) 
            && ( keypad_data->keypad_config & KEYPAD_CONFIG_SHIFT_EFFECT )
            && ( key_code >= 'a' ) && ( key_code <= 'z' ) )
        {
            key_code -= 0x20;
        }
        else if(    ( keypad_data->local_control_key & CTRL_FLAG ) 
                 && ( keypad_data->keypad_config & KEYPAD_CONFIG_CTRL_EFFECT ) 
                 && ( ( ( key_code >= 'a' ) && ( key_code <= 'z' ) ) || ( ( key_code >= 'A' ) && ( key_code <= 'Z' ) ) ) )
        {
            key_code &= 0x1F;
        }

        buffer_write( &keypad_data->rx_buffer_data, key_code );
    }
}

static void keypad_command_process( uint32_t window_id )
{
    void * p_uc = sdl_window_get_user_data( window_id );
    keypad_data_t * keypad_data = ( keypad_data_t * ) sdl_windows_get_heap_data( window_id, HW_BLOCK_KEYPAD );

    local_do_event( window_id );

    uint8_t command = 0;
    while( uart_emulator_tx_pending( keypad_data->p_uart ) == TRUE )
    {
        command = uart_emulator_get_tx_byte( keypad_data->p_uart );
        
        switch( command )
        {
            case 0x30: keypad_data->keypad_config &= ~KEYPAD_CONFIG_SEND_CONTROL; break;
            case 0x31: keypad_data->keypad_config |= KEYPAD_CONFIG_SEND_CONTROL;  break;
            case 0x40: keypad_data->keypad_config &= ~KEYPAD_CONFIG_SHIFT_EFFECT; break;
            case 0x41: keypad_data->keypad_config |= KEYPAD_CONFIG_SHIFT_EFFECT;  break;
            case 0x42: keypad_data->keypad_config &= ~KEYPAD_CONFIG_CTRL_EFFECT;  break;
            case 0x43: keypad_data->keypad_config |= KEYPAD_CONFIG_CTRL_EFFECT;   break;
            case 0x44: keypad_data->keypad_config &= ~KEYPAD_CONFIG_CAPS_EFFECT;  break;
            case 0x45: keypad_data->keypad_config |= KEYPAD_CONFIG_CAPS_EFFECT;   break;
            case 0x46: keypad_data->keypad_config &= ~KEYPAD_CONFIG_VT100;        break;
            case 0x47: keypad_data->keypad_config |= KEYPAD_CONFIG_VT100;         break;
        }
    }

    if( ( uart_emulator_is_rx_done( keypad_data->p_uart ) == TRUE ) && ( buffer_is_empty( &keypad_data->rx_buffer_data ) == FALSE ) )
    {
        uint8_t rx_byte = buffer_read( &keypad_data->rx_buffer_data );
        uart_emulator_send_rx_byte( keypad_data->p_uart, rx_byte );
    }
}

void keypad_emulator_init( uint32_t window_id )
{
    keypad_data_t * keypad_data = ( keypad_data_t * )sdl_windows_malloc( window_id, HW_BLOCK_KEYPAD, sizeof( keypad_data_t ) );
    if( keypad_data == NULL )
    {
        return;
    }

    keypad_data->p_uart = uart_emulator_init( UART_KEYPAD_ADDR );
    if( keypad_data->p_uart == NULL )
    {
        return;
    }

    buffer_init( &keypad_data->rx_buffer_data, keypad_data->rx_buffer, KEYPAD_BUFFER_SIZE );

    keypad_data->keypad_config = KEYPAD_CONFIG_SHIFT_EFFECT | KEYPAD_CONFIG_CTRL_EFFECT | KEYPAD_CONFIG_VT100;

    local_do_event = sdl_window_set_background_event( window_id, keypad_command_process );
}
