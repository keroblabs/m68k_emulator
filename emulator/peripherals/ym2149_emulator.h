#ifndef _ym2149_emulator_h_
#define _ym2149_emulator_h_

#include "types.h"

/* The YM2149 GPIO ports reuse the MC68681 port callback contract. */
#include "m68681_emulator.h"

enum
{
    YM2149_EMULATOR_PORT_A = 0,
    YM2149_EMULATOR_PORT_B,
    YM2149_EMULATOR_PORT_COUNT
};

void * ym2149_emulator_init(uint32_t base_address);

/*
 * Attaches a host read/write callback pair to one of the YM2149 GPIO ports.
 * The same m68681_port_read_t / m68681_port_write_t signatures are used so the
 * MC68681 host adapters can be shared.  When the firmware configures the port
 * as an input only the read callback is exercised; when configured as an
 * output only the write callback is exercised.
 */
void ym2149_emulator_set_port_callbacks(void * p_device, uint8_t port,
                                        m68681_port_read_t read_callback,
                                        m68681_port_write_t write_callback,
                                        void * p_data);

#endif /* _ym2149_emulator_h_ */
