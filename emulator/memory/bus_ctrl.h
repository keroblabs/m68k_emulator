#ifndef _bus_ctrl_h_
#define _bus_ctrl_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

enum
{
    BUS_FC_USER_DATA = 1,
    BUS_FC_USER_PROGRAM = 2,
    BUS_FC_SUPERVISOR_DATA = 5,
    BUS_FC_SUPERVISOR_PROGRAM = 6,
    BUS_FC_INTERRUPT_ACK = 7,
};

enum
{
    BUS_ACCESS_READ,
    BUS_ACCESS_WRITE
};

enum
{
    VECTOR_EXT_IRQ = 24,
    VECTOR_TRAP = 32,
    VECTOR_USER = 64,
};

#define UNUSED_MEMORY_PATTERN		0xA0A0A0A0

//////////////////////////////////////////////////////////////////////////////////////

typedef void ( *irq_done_hdl_t )( void );

//////////////////////////////////////////////////////////////////////////////////////

void bus_ctrl_set_fc(unsigned int fc);

uint32_t bus_ctrl_get_fc(void);

uint32_t bus_ctrl_change_endian(uint32_t input, uint8_t size);

void bus_ctrl_ptr_change_endian(void * p_output, void * p_input, uint8_t size);

void bus_ctrl_execute_write(uint32_t address, uint32_t data, uint8_t size);

uint32_t bus_ctrl_execute_read(uint32_t address, uint8_t size);

void bus_ctrl_load_rom( const char * filename );

void bus_ctrl_irq_req(uint8_t vector, uint8_t level, irq_done_hdl_t done_hdl);

uint32_t bus_ctrl_irq_ack(void);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _bus_ctrl_h_
