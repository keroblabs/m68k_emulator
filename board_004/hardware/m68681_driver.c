#include "m68681_driver.h"
#include "memory_map.h"

#define M68681_REG_MR_A              0x00
#define M68681_REG_SR_A_CSRA         0x01
#define M68681_REG_CR_A              0x02
#define M68681_REG_RHR_A_THR_A       0x03
#define M68681_REG_IPCR_ACR          0x04
#define M68681_REG_ISR_IMR           0x05
#define M68681_REG_CTUR              0x06
#define M68681_REG_CTLR              0x07
#define M68681_REG_MR_B              0x08
#define M68681_REG_SR_B_CSRB         0x09
#define M68681_REG_CR_B              0x0A
#define M68681_REG_RHR_B_THR_B       0x0B
#define M68681_REG_IVR               0x0C
#define M68681_REG_IP_OPCR           0x0D
#define M68681_REG_START_OPSET       0x0E
#define M68681_REG_STOP_OPRESET      0x0F

#define M68681_SR_RX_READY            (1u << 0)
#define M68681_SR_TX_READY            (1u << 2)

#define M68681_CR_RESET_MR            0x10
#define M68681_CR_RESET_RX            0x20
#define M68681_CR_RESET_TX            0x30
#define M68681_CR_ENABLE_RX_TX        0x05

/* 8 data bits, no parity, normal mode, one stop bit. */
#define M68681_MR1_8N                 0x13
#define M68681_MR2_1_STOP             0x00

#define M68681_ACR_BRG_SET2           0x80
#define M68681_ACR_TIMER_MASK         0x70
#define M68681_INVALID_CODE           0xFF

static const uint32_t m68681_base[M68681_DEVICE_COUNT] =
{
    M68681_0_ADDR,
    M68681_1_ADDR
};

typedef struct
{
    uint8_t acr;
    uint16_t baud[M68681_CHANNEL_COUNT];
    bool_t channel_init[M68681_CHANNEL_COUNT];
} m68681_state_t;

static m68681_state_t m68681_state[M68681_DEVICE_COUNT];

/*
 * Every MC68681 register transaction is deliberately an 8-bit access.
 * M68K_IO_BYTE_REG_ADDR() supplies the physical byte-lane mapping for the
 * selected external bus width; the C access itself remains uint8_t.
 */
static volatile uint8_t * m68681_reg(uint8_t device, uint8_t reg)
{
    uintptr_t address = (uintptr_t)M68K_IO_BYTE_REG_ADDR(m68681_base[device], reg);
    return (volatile uint8_t *)address;
}

static uint8_t m68681_status_reg(uint8_t channel)
{
    return (channel == M68681_CHANNEL_A) ? M68681_REG_SR_A_CSRA : M68681_REG_SR_B_CSRB;
}

static uint8_t m68681_mode_reg(uint8_t channel)
{
    return (channel == M68681_CHANNEL_A) ? M68681_REG_MR_A : M68681_REG_MR_B;
}

static uint8_t m68681_command_reg(uint8_t channel)
{
    return (channel == M68681_CHANNEL_A) ? M68681_REG_CR_A : M68681_REG_CR_B;
}

static uint8_t m68681_data_reg(uint8_t channel)
{
    return (channel == M68681_CHANNEL_A) ? M68681_REG_RHR_A_THR_A : M68681_REG_RHR_B_THR_B;
}

static uint8_t m68681_baud_code(uint16_t baud_rate, uint8_t set)
{
    switch (baud_rate)
    {
        case 50:    return (set == 0) ? 0x0 : M68681_INVALID_CODE;
        case 75:    return (set == 1) ? 0x0 : M68681_INVALID_CODE;
        case 110:   return 0x1;
        /* 134.5 baud cannot be represented by the integer API. */
        case 150:   return (set == 1) ? 0x3 : M68681_INVALID_CODE;
        case 200:   return (set == 0) ? 0x3 : M68681_INVALID_CODE;
        case 300:   return 0x4;
        case 600:   return 0x5;
        case 1200:  return 0x6;
        case 1050:  return (set == 0) ? 0x7 : M68681_INVALID_CODE;
        case 2000:  return (set == 1) ? 0x7 : M68681_INVALID_CODE;
        case 2400:  return 0x8;
        case 4800:  return 0x9;
        case 7200:  return (set == 0) ? 0xA : M68681_INVALID_CODE;
        case 1800:  return (set == 1) ? 0xA : M68681_INVALID_CODE;
        case 9600:  return 0xB;
        case 38400: return (set == 0) ? 0xC : M68681_INVALID_CODE;
        case 19200: return (set == 1) ? 0xC : M68681_INVALID_CODE;
        default:    return M68681_INVALID_CODE;
    }
}

static bool_t m68681_any_channel_initialized(const m68681_state_t * state)
{
    return (state->channel_init[M68681_CHANNEL_A] || state->channel_init[M68681_CHANNEL_B]) ? TRUE : FALSE;
}

/*
 * ACR[7] selects one baud-rate set for both channels of a DUART.  To avoid
 * changing that shared selection while another live channel is running, the
 * driver chooses the set when the first channel is initialized and thereafter
 * requires subsequent channels to be representable in the same set.
 */
static bool_t m68681_select_baud(uint8_t device, uint16_t baud_rate, uint8_t * code)
{
    m68681_state_t * state = &m68681_state[device];
    uint8_t current_set = (state->acr & M68681_ACR_BRG_SET2) ? 1u : 0u;
    uint8_t selected_code = m68681_baud_code(baud_rate, current_set);

    if (selected_code != M68681_INVALID_CODE)
    {
        *code = selected_code;
        return TRUE;
    }

    if (m68681_any_channel_initialized(state) == TRUE)
    {
        return FALSE;
    }

    current_set ^= 1u;
    selected_code = m68681_baud_code(baud_rate, current_set);
    if (selected_code == M68681_INVALID_CODE)
    {
        return FALSE;
    }

    if (current_set != 0)
    {
        state->acr |= M68681_ACR_BRG_SET2;
    }
    else
    {
        state->acr &= (uint8_t)~M68681_ACR_BRG_SET2;
    }

    *m68681_reg(device, M68681_REG_IPCR_ACR) = state->acr;
    *code = selected_code;
    return TRUE;
}

bool_t m68681_channel_init(uint8_t device, uint8_t channel, uint16_t baud_rate)
{
    uint8_t baud_code;
    uint8_t command;

    if ((device >= M68681_DEVICE_COUNT) || (channel >= M68681_CHANNEL_COUNT))
    {
        return FALSE;
    }

    if (m68681_select_baud(device, baud_rate, &baud_code) == FALSE)
    {
        return FALSE;
    }

    command = m68681_command_reg(channel);

    /* Put the channel into a known state before changing its format/clocks. */
    *m68681_reg(device, command) = M68681_CR_RESET_RX;
    *m68681_reg(device, command) = M68681_CR_RESET_TX;
    *m68681_reg(device, command) = M68681_CR_RESET_MR;

    *m68681_reg(device, m68681_mode_reg(channel)) = M68681_MR1_8N;
    *m68681_reg(device, m68681_mode_reg(channel)) = M68681_MR2_1_STOP;
    *m68681_reg(device, m68681_status_reg(channel)) = (uint8_t)((baud_code << 4) | baud_code);
    *m68681_reg(device, command) = M68681_CR_ENABLE_RX_TX;

    m68681_state[device].baud[channel] = baud_rate;
    m68681_state[device].channel_init[channel] = TRUE;
    return TRUE;
}

bool_t m68681_channel_tx_ready(uint8_t device, uint8_t channel)
{
    if ((device >= M68681_DEVICE_COUNT) || (channel >= M68681_CHANNEL_COUNT))
    {
        return FALSE;
    }

    return ((*m68681_reg(device, m68681_status_reg(channel)) & M68681_SR_TX_READY) != 0) ? TRUE : FALSE;
}

bool_t m68681_channel_rx_ready(uint8_t device, uint8_t channel)
{
    if ((device >= M68681_DEVICE_COUNT) || (channel >= M68681_CHANNEL_COUNT))
    {
        return FALSE;
    }

    return ((*m68681_reg(device, m68681_status_reg(channel)) & M68681_SR_RX_READY) != 0) ? TRUE : FALSE;
}

void m68681_channel_write(uint8_t device, uint8_t channel, uint8_t data)
{
    if ((device < M68681_DEVICE_COUNT) && (channel < M68681_CHANNEL_COUNT))
    {
        *m68681_reg(device, m68681_data_reg(channel)) = data;
    }
}

uint8_t m68681_channel_read(uint8_t device, uint8_t channel)
{
    if ((device >= M68681_DEVICE_COUNT) || (channel >= M68681_CHANNEL_COUNT))
    {
        return 0;
    }

    return *m68681_reg(device, m68681_data_reg(channel));
}

void m68681_set_interrupt_vector(uint8_t device, uint8_t vector)
{
    if (device < M68681_DEVICE_COUNT)
    {
        *m68681_reg(device, M68681_REG_IVR) = vector;
    }
}

void m68681_set_interrupt_mask(uint8_t device, uint8_t mask)
{
    if (device < M68681_DEVICE_COUNT)
    {
        *m68681_reg(device, M68681_REG_ISR_IMR) = mask;
    }
}

uint8_t m68681_get_interrupt_status(uint8_t device)
{
    if (device >= M68681_DEVICE_COUNT)
    {
        return 0;
    }

    return *m68681_reg(device, M68681_REG_ISR_IMR);
}

bool_t m68681_timer_configure(uint8_t device, uint16_t preload, uint8_t timer_mode)
{
    if (device >= M68681_DEVICE_COUNT)
    {
        return FALSE;
    }

    if (((timer_mode & M68681_ACR_TIMER_MASK) < M68681_TIMER_MODE_IP2) ||
        ((timer_mode & (uint8_t)~M68681_ACR_TIMER_MASK) != 0))
    {
        return FALSE;
    }

    m68681_state[device].acr &= (uint8_t)~M68681_ACR_TIMER_MASK;
    m68681_state[device].acr |= timer_mode;

    *m68681_reg(device, M68681_REG_IPCR_ACR) = m68681_state[device].acr;
    *m68681_reg(device, M68681_REG_CTUR) = (uint8_t)(preload >> 8);
    *m68681_reg(device, M68681_REG_CTLR) = (uint8_t)preload;
    return TRUE;
}

void m68681_timer_start(uint8_t device)
{
    volatile uint8_t dummy;

    if (device < M68681_DEVICE_COUNT)
    {
        /* Reading register 14 is the MC68681 start-counter/timer command. */
        dummy = *m68681_reg(device, M68681_REG_START_OPSET);
        (void)dummy;
    }
}

void m68681_timer_acknowledge(uint8_t device)
{
    volatile uint8_t dummy;

    if (device < M68681_DEVICE_COUNT)
    {
        /* In timer mode this read resets the C/T interrupt; it does not stop the timer. */
        dummy = *m68681_reg(device, M68681_REG_STOP_OPRESET);
        (void)dummy;
    }
}
