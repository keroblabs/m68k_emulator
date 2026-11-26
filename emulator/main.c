#include "types.h"
#include "m68k.h"
#include "sdl_main.h"
#include "sdl_lib_export.h"
#include "sdl_window.h"

#include "bus_ctrl.h"

#include "display_emulator.h"
#include "keypad_emulator.h"
#include "m68681_emulator.h"
#include "timer_emulator.h"

#include "memory_map.h"

#define BINARY_FILE         "/home/marcio/Projects/emulators/m68k_emulator/board_004/make/m68k_rom.bin"

#ifndef M68K_EMULATE_68010
#define M68K_EMULATE_68010 0
#endif

int main(int argc, char * argv[])
{
    void * p_duart0;
    void * p_duart1;
    void * p_uart_keypad;
    void * p_uart_debug;
    void * p_uart_port1;

    (void)argc;
    (void)argv;

    uint32_t window_id = display_emulator_init();

    p_duart0 = m68681_emulator_init(M68681_0_ADDR);
    p_duart1 = m68681_emulator_init(M68681_1_ADDR);

    p_uart_keypad = m68681_emulator_get_channel(p_duart0, M68681_EMULATOR_CHANNEL_A);
    p_uart_debug  = m68681_emulator_get_channel(p_duart0, M68681_EMULATOR_CHANNEL_B);
    p_uart_port1  = m68681_emulator_get_channel(p_duart1, M68681_EMULATOR_CHANNEL_A);

    keypad_emulator_init(window_id, p_uart_keypad);

    /* Keep the existing generic system timer active during this migration. */
    timer_emulator_init(window_id);

    bus_ctrl_load_rom(BINARY_FILE);

    m68k_init();
#if M68K_EMULATE_68010
    m68k_set_cpu_type(M68K_CPU_TYPE_68010);
#else
    /*
     * Musashi has no separate 68008 instruction core.  The 68000 core is the
     * correct ISA/core baseline; M68K_DATA_BUS_WIDTH selects the external
     * board address mapping independently.
     */
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
#endif
    m68k_pulse_reset();

    while (1)
    {
        m68k_execute(32);

        if (sdl_event_loop() == 0)
        {
            /* The SDL window received a close/quit request. */
            break;
        }

        if ((p_uart_debug != NULL) &&
            (m68681_emulator_tx_pending(p_uart_debug) == TRUE))
        {
            uint8_t tx_byte = m68681_emulator_get_tx_byte(p_uart_debug);
            printf("%c", tx_byte);
            fflush(stdout);
        }

        /*
         * UART_PORT1 has no host attachment yet.  Drain transmitted bytes so
         * its TxRDY state behaves like a connected transmitter rather than
         * permanently back-pressuring firmware after one byte.
         */
        if ((p_uart_port1 != NULL) &&
            (m68681_emulator_tx_pending(p_uart_port1) == TRUE))
        {
            (void)m68681_emulator_get_tx_byte(p_uart_port1);
        }
    }

    sdl_terminate();

    return 0;
}
