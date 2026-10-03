#include "bus_ctrl.h"
#include "m68k.h"

static bool_t trace_on = FALSE;

/* Read data from RAM, ROM, or a device */
unsigned int cpu_read_byte(unsigned int address)
{
	return (unsigned int)bus_ctrl_execute_read(address, sizeof(uint8_t));
}

unsigned int cpu_read_word(unsigned int address)
{
	return (unsigned int)bus_ctrl_execute_read(address, sizeof(uint16_t));
}

unsigned int cpu_read_long(unsigned int address)
{
	return (unsigned int)bus_ctrl_execute_read(address, sizeof(uint32_t));
}

unsigned int cpu_read_word_dasm(unsigned int address)
{
	return (unsigned int)bus_ctrl_execute_read(address, sizeof(uint16_t));
}

unsigned int cpu_read_long_dasm(unsigned int address)
{
	return (unsigned int)bus_ctrl_execute_read(address, sizeof(uint32_t));
}

/* Write data to RAM or a device */
void cpu_write_byte(unsigned int address, unsigned int value)
{
	bus_ctrl_execute_write(address, value, sizeof(uint8_t));
}

void cpu_write_word(unsigned int address, unsigned int value)
{
	bus_ctrl_execute_write(address, value, sizeof(uint16_t));
}

void cpu_write_long(unsigned int address, unsigned int value)
{
	bus_ctrl_execute_write(address, value, sizeof(uint32_t));
}

/* Called when the CPU changes the function code pins */
void cpu_set_fc(unsigned int fc)
{
	bus_ctrl_set_fc(fc);
}

/* Called when the CPU acknowledges an interrupt */
int cpu_irq_ack(int level)
{
	return bus_ctrl_irq_ack();
}

/* Called when the CPU pulses the RESET line */
void cpu_pulse_reset(void)
{
}

void cpu_instr_callback(int pc)
{
	(void)pc;
/* The following code would print out instructions as they are executed */
#if 1
	if( pc == 0x34d2 )
	{
		//trace_on = TRUE;
	}
	if( pc == 0x2854 )
	{
		//trace_on = FALSE;
	}

	if( trace_on == TRUE )
	{
		static char buff[100];
		//static char buff2[100];
		//static unsigned int pc;
		static unsigned int instr_size;

		pc = m68k_get_reg(NULL, M68K_REG_PC);
		instr_size = m68k_disassemble(buff, pc, M68K_CPU_TYPE_68000);
		//make_hex(buff2, pc, instr_size);
		printf("E %03x: %-20s: %s\n", pc, "", buff);
		fflush(stdout);
		//trace_on = FALSE;
	}
#endif
}
