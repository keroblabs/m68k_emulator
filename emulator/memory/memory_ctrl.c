#include "bus_ctrl.h"
#include "memory_map.h"
#include "memory_ctrl.h"
#include "io_ctrl.h"

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

static uint8_t ram_memory[RAM_UPPER_SIZE];

//////////////////////////////////////////////////////////////////////////////////////

bool_t memory_ctrl_upper_ram_is_selected(uint32_t address)
{
	return ((address >= RAM_UPPER_BASE_ADDRESS) && (address < (RAM_UPPER_BASE_ADDRESS + RAM_UPPER_SIZE)));
}

uint32_t memory_ctrl_ram_access(uint32_t address, uint32_t data, uint8_t size, uint8_t access)
{
	uint32_t ret = 0;
	uint32_t ram_address = address - RAM_UPPER_BASE_ADDRESS;

	if (access == MEMORY_ACCESS_READ)
	{
		bus_ctrl_ptr_change_endian(&ret, &ram_memory[ram_address], size);
	}
	else
	{
		bus_ctrl_ptr_change_endian(&ram_memory[ram_address], &data, size);
	}

	return ret;
}

uint32_t memory_ctrl_ram_read_block(uint32_t address, uint8_t * buffer, uint32_t size)
{
	uint32_t ram_address = address - RAM_UPPER_BASE_ADDRESS;
	memcpy(buffer, &ram_memory[ram_address], size);	
}

void memory_ctrl_init(void)
{
}
