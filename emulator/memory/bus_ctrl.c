#include "bus_ctrl.h"
#include "memory_map.h"
#include "memory_ctrl.h"
#include "mmu_ctrl.h"
#include "io_ctrl.h"
#include "m68k.h"

//////////////////////////////////////////////////////////////////////////////////////

typedef struct irq_request_t_
{
	struct irq_request_t_ * p_next;
	irq_done_hdl_t done_hdl;
	uint8_t vector;

} irq_request_t;

//////////////////////////////////////////////////////////////////////////////////////

static uint32_t curr_fc;

static uint8_t rom_memory[ROM_MEMORY_SIZE];

static irq_request_t * irq_req_list;

//////////////////////////////////////////////////////////////////////////////////////

static bool_t is_address_rom(uint32_t address)
{
	return (address < ROM_MEMORY_SIZE);
}

void bus_ctrl_set_fc(unsigned int fc)
{
	curr_fc = fc;
}

uint32_t bus_ctrl_get_fc(void)
{
	return curr_fc;
}

void bus_ctrl_irq_req(uint8_t vector, uint8_t level, irq_done_hdl_t done_hdl)
{
	if( irq_req_list != NULL )
	{
		return;
	}

	irq_request_t * irq_req = ( irq_request_t * )malloc( sizeof( irq_request_t ) );
	if( irq_req != NULL )
	{
		irq_req->done_hdl = done_hdl;
		irq_req->vector = vector;

		irq_req->p_next = irq_req_list;
		irq_req_list = irq_req;

		m68k_set_irq((unsigned int)level);
	}
}

uint32_t bus_ctrl_irq_ack(void)
{
	uint32_t ret = M68K_INT_ACK_SPURIOUS;

	if( irq_req_list != NULL )
	{
		irq_request_t * curr_irq = irq_req_list;

		irq_req_list = irq_req_list->p_next;

		if( curr_irq->vector != NEG_U32 )
		{
			ret = curr_irq->vector + VECTOR_EXT_IRQ;
		}

		if( curr_irq->done_hdl != NULL )
		{
			curr_irq->done_hdl();
		}

		m68k_set_irq(0);

		free( curr_irq );		
	}

	return ret;
}

void bus_ctrl_load_rom( const char * filename )
{
	irq_req_list = NULL;

	FILE * fp = fopen( filename, "rb" );
	if( fp != NULL )
	{
		int size = fread( rom_memory, ROM_MEMORY_SIZE, 1, fp );
		//printf( "Load %d bytes in ROM\n", size );
		fclose( fp );
	}
}

uint32_t bus_ctrl_change_endian(uint32_t input, uint8_t size)
{
	uint32_t ret = 0;
	uint8_t * p_input = (uint8_t *)&input;
	uint8_t * p_output = (uint8_t *)&ret;
	uint8_t index_in = 0;
	uint8_t index_out = size - 1;

	while (size)
	{
		p_output[index_out] = p_input[index_in];
		index_in++;
		index_out--;
		size--;
	}

	return ret;
}

void bus_ctrl_ptr_change_endian(void * p_output, void * p_input, uint8_t size)
{
	uint8_t index_in = 0;
	uint8_t index_out = size - 1;
	uint8_t * pp_output = (uint8_t *)p_output;
	uint8_t * pp_input = (uint8_t *)p_input;

	while (size)
	{
		pp_output[index_out] = pp_input[index_in];
		index_in++;
		index_out--;
		size--;
	}
}

void bus_ctrl_execute_write(uint32_t address, uint32_t data, uint8_t size)
{
	if (is_address_rom(address))
	{
		// write are discarded
	}
	else if (mmu_ctrl_is_selected(address))
	{
		// perform MMU operation
		uint32_t mem_address = mmu_ctrl_get_address( address, BUS_ACCESS_WRITE );
		memory_ctrl_ram_access(mem_address, data, size, BUS_ACCESS_WRITE);
	}
	else if (io_ctrl_is_selected(address))
	{
		/*
		 * I/O handlers exchange logical CPU values, not host byte-array
		 * representations.  Keep the requested access width intact and let the
		 * device model validate it.
		 */
		io_ctrl_write(address, data, size);
	}
	else if (memory_ctrl_upper_ram_is_selected(address))
	{
		memory_ctrl_ram_access(address, data, size, BUS_ACCESS_WRITE);
	}
}

uint32_t bus_ctrl_execute_read(uint32_t address, uint8_t size)
{
	uint32_t data = 0;

	if (is_address_rom(address))
	{
		uint32_t rom_address = address - ROM_BASE_ADDRESS;
		bus_ctrl_ptr_change_endian(&data, &rom_memory[rom_address], size);
	}
	else if (mmu_ctrl_is_selected(address))
	{
		// perform MMU operation
		uint32_t mem_address = mmu_ctrl_get_address( address, BUS_ACCESS_READ );
		data = memory_ctrl_ram_access(mem_address, 0, size, BUS_ACCESS_READ);
	}
	else if (io_ctrl_is_selected(address))
	{
		/*
		 * As for writes, an I/O model returns the logical value observed by
		 * the CPU.  Endian conversion belongs only at byte-array memory
		 * boundaries.
		 */
		data = io_ctrl_read(address, size);
	}
	else if (memory_ctrl_upper_ram_is_selected(address))
	{
		data = memory_ctrl_ram_access(address, 0, size, BUS_ACCESS_READ);
	}

	return data;
}
