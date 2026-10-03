#ifndef _emulator_api_h_
#define _emulator_api_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

/* Read data from RAM, ROM, or a device */
unsigned int cpu_read_byte(unsigned int address);

unsigned int cpu_read_word(unsigned int address);

unsigned int cpu_read_long(unsigned int address);

unsigned int cpu_read_word_dasm(unsigned int address);

unsigned int cpu_read_long_dasm(unsigned int address);

/* Write data to RAM or a device */
void cpu_write_byte(unsigned int address, unsigned int value);

void cpu_write_word(unsigned int address, unsigned int value);

void cpu_write_long(unsigned int address, unsigned int value);

/* Called when the CPU changes the function code pins */
void cpu_set_fc(unsigned int fc);

/* Called when the CPU acknowledges an interrupt */
int cpu_irq_ack(int level);

/* Called when the CPU pulses the RESET line */
void cpu_pulse_reset(void);

void cpu_instr_callback(int pc);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _emulator_api_h_

