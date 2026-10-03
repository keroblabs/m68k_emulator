#ifndef _m68681_emulator_h_
#define _m68681_emulator_h_

#include "types.h"

enum
{
    M68681_EMULATOR_CHANNEL_A = 0,
    M68681_EMULATOR_CHANNEL_B,
    M68681_EMULATOR_CHANNEL_COUNT
};

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

#endif /* _m68681_emulator_h_ */
