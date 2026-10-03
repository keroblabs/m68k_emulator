#include "mmu_ctrl.h"
#include "memory_map.h"
#include "memory_ctrl.h"
#include "bus_ctrl.h"

typedef struct mmu_register_t_
{
    uint32_t mask;
    uint32_t offset;

} mmu_register_t;

static mmu_register_t mmu_config;

static uint32_t mmu_register_access(uint32_t address, uint32_t data, uint8_t size, uint8_t access)
{
    uint32_t mmu_reg_addr = address - MMU_REGISTER_BANK_BASE;
    uint8_t * p_mmu_reg = (uint8_t *)&mmu_config;

    bus_ctrl_ptr_change_endian(&p_mmu_reg[mmu_reg_addr], (uint8_t *)&data, size);
}

uint32_t mmu_ctrl_get_address(uint32_t address, uint8_t access)
{
    uint32_t op_address;
    
    op_address  = (address >> 10);
    op_address &= mmu_config.mask;
    op_address += mmu_config.offset;
    op_address += MMU_BASE_ADDRESS;

    op_address <<= 1;

    uint32_t mmu_address = memory_ctrl_ram_access( op_address, 0, sizeof( uint16_t ), BUS_ACCESS_READ );

    uint32_t ret_address = ( mmu_address << 10 ) | ( address & 0x3FF );

    return ret_address;
}

bool_t mmu_ctrl_is_selected(uint32_t address)
{
	return ((address >= RAM_MMU_BASE_ADDRESS) && (address < (RAM_MMU_BASE_ADDRESS + RAM_MMU_SIZE)));
}
