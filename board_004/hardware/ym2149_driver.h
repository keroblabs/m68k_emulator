#ifndef _ym2149_driver_h_
#define _ym2149_driver_h_

#include "types.h"

enum
{
    YM2149_PORT_A = 0,
    YM2149_PORT_B,
    YM2149_PORT_COUNT
};

enum
{
    YM2149_PORT_INPUT = 0,
    YM2149_PORT_OUTPUT
};

enum
{
    YM2149_CHANNEL_A = 0,
    YM2149_CHANNEL_B,
    YM2149_CHANNEL_C,
    YM2149_CHANNEL_COUNT
};

/* Prepares the PSG for use and returns TRUE when the device answers a
 * register read-back self-test (used to sense the chip is present). */
bool_t ym2149_init(void);

/* GPIO: configure a port as input or output and transfer a byte. */
void ym2149_port_configure(uint8_t port, uint8_t direction);
void ym2149_port_write(uint8_t port, uint8_t data);
uint8_t ym2149_port_read(uint8_t port);

/*
 * Audio configuration. The host does not synthesize sound yet; these writes
 * only store the configuration in the device for a future implementation.
 */
void ym2149_set_tone_period(uint8_t channel, uint16_t period);
void ym2149_set_noise_period(uint8_t period);
void ym2149_set_amplitude(uint8_t channel, uint8_t amplitude);
void ym2149_set_envelope(uint16_t period, uint8_t shape);

#endif /* _ym2149_driver_h_ */
