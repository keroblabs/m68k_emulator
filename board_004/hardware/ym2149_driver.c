#include "ym2149_driver.h"
#include "memory_map.h"

/* Bus registers: address latch then data port. */
#define YM2149_BUS_ADDRESS_LATCH     0u
#define YM2149_BUS_DATA_PORT         1u

/* Internal register map. */
#define YM2149_R_CHA_TONE_FINE       0x00
#define YM2149_R_CHA_TONE_COARSE     0x01
#define YM2149_R_NOISE_PERIOD        0x06
#define YM2149_R_MIXER               0x07
#define YM2149_R_CHA_AMPLITUDE       0x08
#define YM2149_R_ENV_PERIOD_FINE     0x0B
#define YM2149_R_ENV_PERIOD_COARSE   0x0C
#define YM2149_R_ENV_SHAPE           0x0D
#define YM2149_R_IO_PORT_A           0x0E
#define YM2149_R_IO_PORT_B           0x0F

/* Mixer R7 data-direction bits: 1 = output, 0 = input. */
#define YM2149_MIXER_PORT_A_OUTPUT   (1u << 6)
#define YM2149_MIXER_PORT_B_OUTPUT   (1u << 7)

/* Shadow of the mixer register so port direction changes are non-destructive. */
static uint8_t ym2149_mixer_shadow;

static volatile uint8_t * ym2149_bus_reg(uint8_t bus_reg)
{
    uintptr_t address = (uintptr_t)M68K_IO_BYTE_REG_ADDR(YM2149_ADDR, bus_reg);
    return (volatile uint8_t *)address;
}

static void ym2149_write_reg(uint8_t reg, uint8_t data)
{
    *ym2149_bus_reg(YM2149_BUS_ADDRESS_LATCH) = (uint8_t)(reg & 0x0Fu);
    *ym2149_bus_reg(YM2149_BUS_DATA_PORT) = data;
}

static uint8_t ym2149_read_reg(uint8_t reg)
{
    *ym2149_bus_reg(YM2149_BUS_ADDRESS_LATCH) = (uint8_t)(reg & 0x0Fu);
    return *ym2149_bus_reg(YM2149_BUS_DATA_PORT);
}

bool_t ym2149_init(void)
{
    uint8_t probe;

    /* Default mixer: tone/noise disabled, both ports inputs. */
    ym2149_mixer_shadow = 0x3F;
    ym2149_write_reg(YM2149_R_MIXER, ym2149_mixer_shadow);

    /* Self-test: a writable register must read back what was written. */
    ym2149_write_reg(YM2149_R_CHA_TONE_FINE, 0xA5);
    probe = ym2149_read_reg(YM2149_R_CHA_TONE_FINE);
    ym2149_write_reg(YM2149_R_CHA_TONE_FINE, 0x00);

    return (probe == 0xA5) ? TRUE : FALSE;
}

void ym2149_port_configure(uint8_t port, uint8_t direction)
{
    uint8_t mask;

    if (port >= YM2149_PORT_COUNT)
    {
        return;
    }

    mask = (port == YM2149_PORT_A) ? YM2149_MIXER_PORT_A_OUTPUT : YM2149_MIXER_PORT_B_OUTPUT;

    if (direction == YM2149_PORT_OUTPUT)
    {
        ym2149_mixer_shadow |= mask;
    }
    else
    {
        ym2149_mixer_shadow &= (uint8_t)~mask;
    }

    ym2149_write_reg(YM2149_R_MIXER, ym2149_mixer_shadow);
}

void ym2149_port_write(uint8_t port, uint8_t data)
{
    if (port == YM2149_PORT_A)
    {
        ym2149_write_reg(YM2149_R_IO_PORT_A, data);
    }
    else if (port == YM2149_PORT_B)
    {
        ym2149_write_reg(YM2149_R_IO_PORT_B, data);
    }
}

uint8_t ym2149_port_read(uint8_t port)
{
    if (port == YM2149_PORT_A)
    {
        return ym2149_read_reg(YM2149_R_IO_PORT_A);
    }
    if (port == YM2149_PORT_B)
    {
        return ym2149_read_reg(YM2149_R_IO_PORT_B);
    }
    return 0;
}

void ym2149_set_tone_period(uint8_t channel, uint16_t period)
{
    uint8_t fine_reg;

    if (channel >= YM2149_CHANNEL_COUNT)
    {
        return;
    }

    /* Each channel uses a fine/coarse register pair (2 registers apart). */
    fine_reg = (uint8_t)(YM2149_R_CHA_TONE_FINE + (channel * 2u));
    ym2149_write_reg(fine_reg, (uint8_t)(period & 0xFFu));
    ym2149_write_reg((uint8_t)(fine_reg + 1u), (uint8_t)((period >> 8) & 0x0Fu));
}

void ym2149_set_noise_period(uint8_t period)
{
    ym2149_write_reg(YM2149_R_NOISE_PERIOD, (uint8_t)(period & 0x1Fu));
}

void ym2149_set_amplitude(uint8_t channel, uint8_t amplitude)
{
    if (channel >= YM2149_CHANNEL_COUNT)
    {
        return;
    }

    ym2149_write_reg((uint8_t)(YM2149_R_CHA_AMPLITUDE + channel), (uint8_t)(amplitude & 0x1Fu));
}

void ym2149_set_envelope(uint16_t period, uint8_t shape)
{
    ym2149_write_reg(YM2149_R_ENV_PERIOD_FINE, (uint8_t)(period & 0xFFu));
    ym2149_write_reg(YM2149_R_ENV_PERIOD_COARSE, (uint8_t)((period >> 8) & 0xFFu));
    ym2149_write_reg(YM2149_R_ENV_SHAPE, (uint8_t)(shape & 0x0Fu));
}
