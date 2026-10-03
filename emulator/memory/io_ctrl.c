#include "io_ctrl.h"
#include "bus_ctrl.h"
#include "memory_map.h"

//////////////////////////////////////////////////////////////////////////////////////

static io_hdl_t io_handler_list[IO_SPACE_SIZE];

//////////////////////////////////////////////////////////////////////////////////////

void io_ctrl_init(void)
{
	memset(io_handler_list, 0x00, sizeof(io_handler_list));
}

void io_ctrl_set_handler(uint32_t address, io_hdl_t handler)
{
	uint32_t io_address = address - IO_BASE_ADDRESS;

	if (io_address < IO_SPACE_SIZE)
	{
		io_handler_list[io_address] = handler;
	}
}

uint32_t io_ctrl_read(uint32_t address, uint8_t size)
{
	uint32_t data = UNUSED_MEMORY_PATTERN;
	uint32_t io_address = address - IO_BASE_ADDRESS;

	if (io_handler_list[io_address] != NULL)
	{
		data = io_handler_list[io_address](address, 0, size, BUS_ACCESS_READ);
	}

	return data;
}

void io_ctrl_write(uint32_t address, uint32_t data, uint8_t size)
{
	uint32_t io_address = address - IO_BASE_ADDRESS;

	if (io_handler_list[io_address] != NULL)
	{
		io_handler_list[io_address](address, data, size, MEMORY_ACCESS_WRITE);
	}
}

bool_t io_ctrl_is_selected(uint32_t address)
{
	return ((address >= IO_BASE_ADDRESS) && (address < (IO_BASE_ADDRESS + IO_SPACE_SIZE)));
}
