#ifndef _m68681_emulator_h_
#define _m68681_emulator_h_

#include "types.h"

enum
{
    M68681_EMULATOR_CHANNEL_A = 0,
    M68681_EMULATOR_CHANNEL_B,
    M68681_EMULATOR_CHANNEL_COUNT
};

typedef uint8_t (*m68681_port_read_t)(void * p_data);
typedef void (*m68681_port_write_t)(void * p_data, uint8_t data);

void m68681_emulator_set_port_callbacks(void * p_device, m68681_port_read_t read_callback, m68681_port_write_t write_callback, void * p_data);

void * m68681_emulator_init(uint32_t base_address);
void * m68681_emulator_get_channel(void * p_device, uint8_t channel);

/* Host-side serial endpoints used by the SDL keypad and debug console. */
bool_t m68681_emulator_rx_can_accept(void * p_channel);
bool_t m68681_emulator_send_rx_byte(void * p_channel, uint8_t data);
bool_t m68681_emulator_tx_pending(void * p_channel);
uint8_t m68681_emulator_get_tx_byte(void * p_channel);

/* Exposes modeled interrupt state for later bus/IRQ integration. */
bool_t m68681_emulator_irq_pending(void * p_device);
uint8_t m68681_emulator_interrupt_vector(void * p_device);

/*
 * Advances the counter/timer from the host 3.6864 MHz tick source and raises a
 * CPU interrupt when a timer-mode period elapses. Call periodically from the
 * emulator main loop.
 */
void m68681_emulator_timer_poll(void * p_device);

#endif /* _m68681_emulator_h_ */
