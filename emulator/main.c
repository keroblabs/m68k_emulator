#include "types.h"
#include "m68k.h"
#include "sdl_main.h"
#include "sdl_lib_export.h"
#include "sdl_window.h"

#include "bus_ctrl.h"

#include "display_emulator.h"
#include "keypad_emulator.h"
#include "m68681_emulator.h"
#include "ym2149_emulator.h"
#ifdef USE_KEYBOARD_MATRIX
#include "keyboard_matrix_emulator.h"
#endif

#include "system_timer.h"

#include "memory_map.h"

#define BINARY_FILE         "/home/marcio/Projects/emulators/m68k_emulator/board_004/make/m68k_rom.bin"

#ifndef M68K_EMULATE_68010
#define M68K_EMULATE_68010 0
#endif

/*
 * Simple host adapter for the YM2149 GPIO ports.  Port A output is looped back
 * to Port A/B inputs so firmware can drive a value and sense it, exercising the
 * shared MC68681 port callback contract without needing dedicated host wiring.
 */
#ifndef USE_KEYBOARD_MATRIX
static uint8_t ym2149_gpio_loopback_value = 0x00;

static uint8_t ym2149_gpio_read(void * p_data)
{
    (void)p_data;
    return ym2149_gpio_loopback_value;
}

static void ym2149_gpio_write(void * p_data, uint8_t data)
{
    (void)p_data;
    ym2149_gpio_loopback_value = data;
}
#endif

int main(int argc, char * argv[])
{
    void * p_duart0;
    void * p_duart1;
    void * p_uart_keypad;
    void * p_uart_debug;
    void * p_uart_port1;
    void * p_ym2149;

    (void)argc;
    (void)argv;

    uint32_t window_id = display_emulator_init();

    p_duart0 = m68681_emulator_init(M68681_0_ADDR);
    p_duart1 = m68681_emulator_init(M68681_1_ADDR);

    p_uart_keypad = m68681_emulator_get_channel(p_duart0, M68681_EMULATOR_CHANNEL_A);
    p_uart_debug  = m68681_emulator_get_channel(p_duart0, M68681_EMULATOR_CHANNEL_B);
    p_uart_port1  = m68681_emulator_get_channel(p_duart1, M68681_EMULATOR_CHANNEL_A);

    /* YM2149 PSG: GPIO is fully modeled; audio config is stored for later. */
    p_ym2149 = ym2149_emulator_init(YM2149_ADDR);

#ifdef USE_KEYBOARD_MATRIX
    (void)p_uart_keypad;
    /*
     * Scanned key matrix: YM2149 Port A drives the row decoder / column mux and
     * the selected cell is sensed on MC68681 (device 0) IP5.
     */
    keyboard_matrix_emulator_init(p_ym2149, p_duart0);
#else
    keypad_emulator_init(window_id, p_uart_keypad);

    ym2149_emulator_set_port_callbacks(p_ym2149, YM2149_EMULATOR_PORT_A,
                                       ym2149_gpio_read, ym2149_gpio_write, NULL);
    ym2149_emulator_set_port_callbacks(p_ym2149, YM2149_EMULATOR_PORT_B,
                                       ym2149_gpio_read, ym2149_gpio_write, NULL);
#endif

    /* Host time source feeding the MC68681 counter/timer emulation. */
    system_timer_init();

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
        /*
         * Run the CPU and service the MC68681 timer many times before touching
         * SDL.  The display refresh inside sdl_event_loop() is comparatively
         * expensive; polling the timer only once per refresh would throttle
         * interrupt delivery and make the firmware's millisecond clock run
         * slow.  Servicing the timer in a tight inner batch keeps the 1 ms
         * interrupt flowing (and drains any backlog) independently of the
         * display frame rate.
         */
        for (int step = 0; step < 256; step++)
        {
            m68k_execute(64);

            /* MC68681 device 0 provides the 1 ms system-timer interrupt. */
            m68681_emulator_timer_poll(p_duart0);
        }

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
