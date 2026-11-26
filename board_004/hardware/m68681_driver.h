#ifndef _m68681_driver_h_
#define _m68681_driver_h_

#include "types.h"

enum
{
    M68681_DEVICE_0 = 0,
    M68681_DEVICE_1,
    M68681_DEVICE_COUNT
};

enum
{
    M68681_CHANNEL_A = 0,
    M68681_CHANNEL_B,
    M68681_CHANNEL_COUNT
};

/* ACR[6:4] timer-mode/clock-source values. */
#define M68681_TIMER_MODE_IP2          0x40
#define M68681_TIMER_MODE_IP2_DIV16    0x50
#define M68681_TIMER_MODE_X1           0x60
#define M68681_TIMER_MODE_X1_DIV16     0x70

/* Interrupt status/mask register bits. */
#define M68681_INT_TX_A                  0x01
#define M68681_INT_RX_A                  0x02
#define M68681_INT_BREAK_A               0x04
#define M68681_INT_COUNTER_TIMER         0x08
#define M68681_INT_TX_B                  0x10
#define M68681_INT_RX_B                  0x20
#define M68681_INT_BREAK_B               0x40
#define M68681_INT_INPUT_CHANGE          0x80

bool_t m68681_channel_init(uint8_t device, uint8_t channel, uint16_t baud_rate);
bool_t m68681_channel_tx_ready(uint8_t device, uint8_t channel);
bool_t m68681_channel_rx_ready(uint8_t device, uint8_t channel);
void m68681_channel_write(uint8_t device, uint8_t channel, uint8_t data);
uint8_t m68681_channel_read(uint8_t device, uint8_t channel);

void m68681_set_interrupt_vector(uint8_t device, uint8_t vector);
void m68681_set_interrupt_mask(uint8_t device, uint8_t mask);
uint8_t m68681_get_interrupt_status(uint8_t device);

/*
 * Counter/timer access used by timer_driver to generate the 1 ms system tick.
 * Device 0 is clocked by a 3.6864 MHz crystal (X1/CLK).
 */
bool_t m68681_timer_configure(uint8_t device, uint16_t preload, uint8_t timer_mode);
void m68681_timer_start(uint8_t device);
void m68681_timer_acknowledge(uint8_t device);

#endif /* _m68681_driver_h_ */
