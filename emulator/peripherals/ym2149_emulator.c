#include "ym2149_emulator.h"
#include <stdlib.h>
#include <string.h>
#include "memory_map.h"
#include "bus_ctrl.h"
#include "io_ctrl.h"

/*
 * YM2149 (AY-3-8910 compatible) programmable sound generator.
 *
 * Only the GPIO function is fully modeled.  The audio synthesis path is not
 * generated on the host; the sound-related registers are stored verbatim so
 * the firmware can configure the device (and read the values back to confirm
 * the chip is present) while a future implementation renders audio.
 *
 * Bus interface: the device exposes two byte registers.
 *   offset 0 : address latch  (write selects the internal register 0..15)
 *   offset 1 : data port       (read/write of the latched internal register)
 */

#define YM2149_REG_COUNT              16

#define YM2149_BUS_ADDRESS_LATCH      0u
#define YM2149_BUS_DATA_PORT          1u
#define YM2149_BUS_REG_COUNT          2u

/* Internal register map. */
#define YM2149_R_CHA_TONE_FINE        0x00
#define YM2149_R_CHA_TONE_COARSE      0x01
#define YM2149_R_CHB_TONE_FINE        0x02
#define YM2149_R_CHB_TONE_COARSE      0x03
#define YM2149_R_CHC_TONE_FINE        0x04
#define YM2149_R_CHC_TONE_COARSE      0x05
#define YM2149_R_NOISE_PERIOD         0x06
#define YM2149_R_MIXER                0x07
#define YM2149_R_CHA_AMPLITUDE        0x08
#define YM2149_R_CHB_AMPLITUDE        0x09
#define YM2149_R_CHC_AMPLITUDE        0x0A
#define YM2149_R_ENV_PERIOD_FINE      0x0B
#define YM2149_R_ENV_PERIOD_COARSE    0x0C
#define YM2149_R_ENV_SHAPE            0x0D
#define YM2149_R_IO_PORT_A            0x0E
#define YM2149_R_IO_PORT_B            0x0F

/*
 * Mixer register R7 bits 6/7 select the I/O port data direction.
 * 0 = input, 1 = output (matches real AY/YM silicon).
 */
#define YM2149_MIXER_PORT_A_OUTPUT    (1u << 6)
#define YM2149_MIXER_PORT_B_OUTPUT    (1u << 7)

#define YM2149_INVALID_READ           0xFF

typedef struct ym2149_port_t_
{
    m68681_port_read_t read_callback;
    m68681_port_write_t write_callback;
    void * callback_data;
    uint8_t output_latch;
} ym2149_port_t;

typedef struct ym2149_data_t_
{
    struct ym2149_data_t_ * p_next;

    uint32_t base_address;

    uint8_t latch;
    uint8_t reg[YM2149_REG_COUNT];

    ym2149_port_t port[YM2149_EMULATOR_PORT_COUNT];

} ym2149_data_t;

static ym2149_data_t * ym2149_data_list = NULL;

static ym2149_data_t * ym2149_data_from_address(uint32_t address, uint8_t * bus_reg_out)
{
    ym2149_data_t * current = ym2149_data_list;

    while (current != NULL)
    {
        uint8_t bus_reg;

        for (bus_reg = 0; bus_reg < YM2149_BUS_REG_COUNT; bus_reg++)
        {
            if (address == M68K_IO_BYTE_REG_ADDR(current->base_address, bus_reg))
            {
                if (bus_reg_out != NULL)
                {
                    *bus_reg_out = bus_reg;
                }
                return current;
            }
        }

        current = current->p_next;
    }

    return NULL;
}

static bool_t ym2149_port_is_output(const ym2149_data_t * device, uint8_t port)
{
    uint8_t mask = (port == YM2149_EMULATOR_PORT_A)
                 ? YM2149_MIXER_PORT_A_OUTPUT
                 : YM2149_MIXER_PORT_B_OUTPUT;

    return ((device->reg[YM2149_R_MIXER] & mask) != 0) ? TRUE : FALSE;
}

static void ym2149_port_write(ym2149_data_t * device, uint8_t port, uint8_t data)
{
    ym2149_port_t * p = &device->port[port];

    p->output_latch = data;

    /* Only an output port drives the host write callback. */
    if ((ym2149_port_is_output(device, port) == TRUE) && (p->write_callback != NULL))
    {
        p->write_callback(p->callback_data, data);
    }
}

static uint8_t ym2149_port_read(ym2149_data_t * device, uint8_t port)
{
    ym2149_port_t * p = &device->port[port];

    /* An input port samples the host read callback; an output port reads back
     * the value the firmware last drove. */
    if ((ym2149_port_is_output(device, port) == FALSE) && (p->read_callback != NULL))
    {
        return p->read_callback(p->callback_data);
    }

    return p->output_latch;
}

static uint8_t ym2149_data_read(ym2149_data_t * device)
{
    switch (device->latch)
    {
        case YM2149_R_IO_PORT_A:
            return ym2149_port_read(device, YM2149_EMULATOR_PORT_A);

        case YM2149_R_IO_PORT_B:
            return ym2149_port_read(device, YM2149_EMULATOR_PORT_B);

        default:
            return device->reg[device->latch];
    }
}

static void ym2149_data_write(ym2149_data_t * device, uint8_t data)
{
    device->reg[device->latch] = data;

    switch (device->latch)
    {
        case YM2149_R_IO_PORT_A:
            ym2149_port_write(device, YM2149_EMULATOR_PORT_A, data);
            break;

        case YM2149_R_IO_PORT_B:
            ym2149_port_write(device, YM2149_EMULATOR_PORT_B, data);
            break;

        default:
            /* Tone/noise/envelope/amplitude configuration is stored only. */
            break;
    }
}

static uint32_t ym2149_register_access(uint32_t address, uint32_t data, uint8_t size, uint8_t access)
{
    uint8_t bus_reg = 0;
    ym2149_data_t * device = ym2149_data_from_address(address, &bus_reg);

    /* The YM2149 is an 8-bit device; wide accesses are intentionally invalid. */
    if ((device == NULL) || (size != sizeof(uint8_t)))
    {
        return YM2149_INVALID_READ;
    }

    if (access == BUS_ACCESS_WRITE)
    {
        if (bus_reg == YM2149_BUS_ADDRESS_LATCH)
        {
            device->latch = (uint8_t)(data & 0x0Fu);
        }
        else
        {
            ym2149_data_write(device, (uint8_t)data);
        }
        return 0;
    }

    if (bus_reg == YM2149_BUS_ADDRESS_LATCH)
    {
        return device->latch;
    }

    return ym2149_data_read(device);
}

void * ym2149_emulator_init(uint32_t base_address)
{
    ym2149_data_t * device = (ym2149_data_t *)malloc(sizeof(ym2149_data_t));
    uint8_t bus_reg;

    if (device == NULL)
    {
        return NULL;
    }

    memset(device, 0x00, sizeof(ym2149_data_t));
    device->base_address = base_address;

    /* Both ports default to inputs after reset (mixer R7 bits 6/7 clear). */
    device->p_next = ym2149_data_list;
    ym2149_data_list = device;

    for (bus_reg = 0; bus_reg < YM2149_BUS_REG_COUNT; bus_reg++)
    {
        io_ctrl_set_handler(M68K_IO_BYTE_REG_ADDR(base_address, bus_reg), ym2149_register_access);
    }

    return device;
}

void ym2149_emulator_set_port_callbacks(void * p_device, uint8_t port,
                                        m68681_port_read_t read_callback,
                                        m68681_port_write_t write_callback,
                                        void * p_data)
{
    ym2149_data_t * device = (ym2149_data_t *)p_device;

    if ((device == NULL) || (port >= YM2149_EMULATOR_PORT_COUNT))
    {
        return;
    }

    device->port[port].read_callback = read_callback;
    device->port[port].write_callback = write_callback;
    device->port[port].callback_data = p_data;
}
