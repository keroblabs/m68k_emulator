#include "types.h"
#include "m68k.h"
#include "sdl_main.h"

#include "bus_ctrl.h"

#include "display_emulator.h"
#include "keypad_emulator.h"
#include "uart_emulator.h"
#include "timer_emulator.h"

#include "memory_map.h"

//////////////////////////////////////////////////////////////////////////////////////

#define BINARY_FILE         "/home/marcio/Projects/emulators/m68k_emulator/board_003/make/m68k_rom.bin"

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    uint32_t window_id = display_emulator_init();
    keypad_emulator_init( window_id );
    timer_emulator_init( window_id );

    bus_ctrl_load_rom(BINARY_FILE);

    void * p_uart_debug = uart_emulator_init( UART_PORT0_ADDR );

    m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();

    //m68k_board_common( BINARY_FILE );
    while( 1 )
    {
        m68k_execute( 32 );
        sdl_event_loop();

        if( p_uart_debug && ( uart_emulator_tx_pending( p_uart_debug ) ) )
        {
            uint8_t tx_byte = uart_emulator_get_tx_byte( p_uart_debug );
            printf( "%c", tx_byte );
        }
    }
    return 0;
}
