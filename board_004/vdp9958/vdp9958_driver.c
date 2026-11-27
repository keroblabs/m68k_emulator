#include "vdp9958_driver.h"
#include "memory_map.h"

/* The V9958 has an eight-bit CPU bus. These volatile byte writes must not be
 * widened or combined by the compiler. Board glue must provide VDP /WAIT. */
static volatile uint8_t * const vdp_ports = (volatile uint8_t *)VDP9958_ADDR;

void vdp9958_write_register(uint8_t reg, uint8_t value)
{
    if (reg > 46u || reg == 24u || (reg >= 28u && reg <= 31u)) return;
    vdp_ports[VDP9958_PORT_CONTROL] = value;
    vdp_ports[VDP9958_PORT_CONTROL] = (uint8_t)(0x80u | (reg & 0x3Fu));
}

void vdp9958_init(void)
{
    /* These registers are also reset by the chip's external /RESET input.
     * Do not blank/reset the screen here: the legacy terminal initialises it. */
    vdp9958_write_register(14, 0);
    vdp9958_write_register(15, 0);
    vdp9958_write_register(16, 0);
    vdp9958_write_register(17, 0);
}

void vdp9958_set_vram_address(uint32_t address, bool_t write)
{
    address &= (VDP9958_VRAM_BYTES - 1u);
    vdp9958_write_register(14, (uint8_t)((address >> 14) & 7u));
    vdp_ports[VDP9958_PORT_CONTROL] = (uint8_t)address;
    vdp_ports[VDP9958_PORT_CONTROL] =
        (uint8_t)(((address >> 8) & 0x3Fu) | (write ? 0x40u : 0u));
}

void vdp9958_write_data(uint8_t value)
{
    vdp_ports[VDP9958_PORT_DATA] = value;
}

uint8_t vdp9958_read_data(void)
{
    return vdp_ports[VDP9958_PORT_DATA];
}

void vdp9958_write_bytes(const uint8_t *data, uint16_t count)
{
    while (count--) vdp9958_write_data(*data++);
}

void vdp9958_fill_bytes(uint8_t value, uint16_t count)
{
    while (count--) vdp9958_write_data(value);
}

uint8_t vdp9958_read_status(uint8_t status_index)
{
    vdp9958_write_register(15, (uint8_t)(status_index & 0x0Fu));
    return vdp_ports[VDP9958_PORT_CONTROL];
}

void vdp9958_set_palette(uint8_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    vdp9958_write_register(16, index & 0x0Fu);
    vdp_ports[VDP9958_PORT_PALETTE] = (uint8_t)(((red & 7u) << 4) | (blue & 7u));
    vdp_ports[VDP9958_PORT_PALETTE] = green & 7u;
}

void vdp9958_set_indirect_register(uint8_t reg, bool_t inhibit_increment)
{
    vdp9958_write_register(17, (uint8_t)((reg & 0x3Fu) | (inhibit_increment ? 0x80u : 0u)));
}

void vdp9958_write_indirect(uint8_t value)
{
    vdp_ports[VDP9958_PORT_INDIRECT] = value;
}

bool_t vdp9958_command_ready(void)
{
    return (vdp9958_read_status(2) & 1u) == 0u ? TRUE : FALSE;
}

void vdp9958_command(const uint8_t params[15])
{
    uint8_t i;
    /* Caller controls blocking/timeout policy using command_ready(). */
    vdp9958_set_indirect_register(32, FALSE);
    for (i = 0; i < 15u; i++) vdp9958_write_indirect(params[i]);
}