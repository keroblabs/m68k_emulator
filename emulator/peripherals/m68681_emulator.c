#include "m68681_emulator.h"
#include <stdlib.h>
#include <string.h>
#include "memory_map.h"
#include "bus_ctrl.h"
#include "io_ctrl.h"

#define M68681_REG_COUNT               16
#define M68681_RX_FIFO_SIZE            3

#define M68681_REG_MR_A                0x00
#define M68681_REG_SR_A_CSRA           0x01
#define M68681_REG_CR_A                0x02
#define M68681_REG_RHR_A_THR_A         0x03
#define M68681_REG_IPCR_ACR            0x04
#define M68681_REG_ISR_IMR             0x05
#define M68681_REG_CTUR                0x06
#define M68681_REG_CTLR                0x07
#define M68681_REG_MR_B                0x08
#define M68681_REG_SR_B_CSRB           0x09
#define M68681_REG_CR_B                0x0A
#define M68681_REG_RHR_B_THR_B         0x0B
#define M68681_REG_IVR                 0x0C
#define M68681_REG_IP_OPCR             0x0D
#define M68681_REG_START_OPSET         0x0E
#define M68681_REG_STOP_OPRESET        0x0F

#define M68681_SR_RX_READY              (1u << 0)
#define M68681_SR_FIFO_FULL             (1u << 1)
#define M68681_SR_TX_READY              (1u << 2)
#define M68681_SR_TX_EMPTY              (1u << 3)
#define M68681_SR_OVERRUN               (1u << 4)

#define M68681_ISR_TX_A                 (1u << 0)
#define M68681_ISR_RX_A                 (1u << 1)
#define M68681_ISR_BREAK_A              (1u << 2)
#define M68681_ISR_COUNTER_TIMER        (1u << 3)
#define M68681_ISR_TX_B                 (1u << 4)
#define M68681_ISR_RX_B                 (1u << 5)
#define M68681_ISR_BREAK_B              (1u << 6)
#define M68681_ISR_INPUT_CHANGE         (1u << 7)

#define M68681_CR_MISC_MASK             0x70
#define M68681_CR_RESET_MR              0x10
#define M68681_CR_RESET_RX              0x20
#define M68681_CR_RESET_TX              0x30
#define M68681_CR_RESET_ERROR           0x40
#define M68681_CR_TX_MASK               0x0C
#define M68681_CR_TX_ENABLE             0x04
#define M68681_CR_TX_DISABLE            0x08
#define M68681_CR_RX_MASK               0x03
#define M68681_CR_RX_ENABLE             0x01
#define M68681_CR_RX_DISABLE            0x02

#define M68681_MR1_RX_IRQ_FIFO_FULL     (1u << 6)
#define M68681_ACR_TIMER_MODE            (1u << 6)

#define M68681_RESET_IVR                 0x0F
#define M68681_INVALID_READ              0xFF

typedef struct m68681_channel_t_
{
    uint8_t mode[2];
    uint8_t mode_pointer;
    uint8_t clock_select;

    bool_t rx_enabled;
    bool_t tx_enabled;

    uint8_t rx_fifo[M68681_RX_FIFO_SIZE];
    uint8_t rx_head;
    uint8_t rx_tail;
    uint8_t rx_count;
    bool_t rx_overrun;

    uint8_t tx_holding;
    bool_t tx_pending;
} m68681_channel_t;

typedef struct m68681_data_t_
{
    struct m68681_data_t_ * p_next;

    uint32_t base_address;
    m68681_channel_t channel[M68681_EMULATOR_CHANNEL_COUNT];

    uint8_t acr;
    uint8_t imr;
    uint8_t ivr;
    uint8_t ipcr;
    uint8_t input_port;
    uint8_t opcr;
    uint8_t output_port;

    uint16_t timer_preload;
    uint16_t timer_counter;
    bool_t timer_running;
    bool_t timer_irq;
} m68681_data_t;

static m68681_data_t * m68681_data_list = NULL;

static m68681_data_t * m68681_data_from_address(uint32_t address, uint8_t * reg_out)
{
    m68681_data_t * current = m68681_data_list;

    while (current != NULL)
    {
        uint8_t reg;

        for (reg = 0; reg < M68681_REG_COUNT; reg++)
        {
            if (address == M68K_IO_BYTE_REG_ADDR(current->base_address, reg))
            {
                if (reg_out != NULL)
                {
                    *reg_out = reg;
                }
                return current;
            }
        }

        current = current->p_next;
    }

    return NULL;
}

static uint8_t m68681_channel_status(const m68681_channel_t * channel)
{
    uint8_t status = 0;

    if (channel->rx_enabled && (channel->rx_count != 0))
    {
        status |= M68681_SR_RX_READY;
    }

    if (channel->rx_enabled && (channel->rx_count >= M68681_RX_FIFO_SIZE))
    {
        status |= M68681_SR_FIFO_FULL;
    }

    if (channel->tx_enabled && (channel->tx_pending == FALSE))
    {
        status |= M68681_SR_TX_READY | M68681_SR_TX_EMPTY;
    }

    if (channel->rx_overrun)
    {
        status |= M68681_SR_OVERRUN;
    }

    return status;
}

static bool_t m68681_rx_interrupt_pending(const m68681_channel_t * channel)
{
    if (channel->rx_enabled == FALSE)
    {
        return FALSE;
    }

    if ((channel->mode[0] & M68681_MR1_RX_IRQ_FIFO_FULL) != 0)
    {
        return (channel->rx_count >= M68681_RX_FIFO_SIZE) ? TRUE : FALSE;
    }

    return (channel->rx_count != 0) ? TRUE : FALSE;
}

static uint8_t m68681_interrupt_status(const m68681_data_t * device)
{
    uint8_t status = 0;
    const m68681_channel_t * channel_a = &device->channel[M68681_EMULATOR_CHANNEL_A];
    const m68681_channel_t * channel_b = &device->channel[M68681_EMULATOR_CHANNEL_B];

    if (channel_a->tx_enabled && (channel_a->tx_pending == FALSE))
    {
        status |= M68681_ISR_TX_A;
    }
    if (m68681_rx_interrupt_pending(channel_a))
    {
        status |= M68681_ISR_RX_A;
    }

    if (device->timer_irq)
    {
        status |= M68681_ISR_COUNTER_TIMER;
    }

    if (channel_b->tx_enabled && (channel_b->tx_pending == FALSE))
    {
        status |= M68681_ISR_TX_B;
    }
    if (m68681_rx_interrupt_pending(channel_b))
    {
        status |= M68681_ISR_RX_B;
    }

    /* Break and input-change sources are modeled as inactive for now. */
    (void)M68681_ISR_BREAK_A;
    (void)M68681_ISR_BREAK_B;
    (void)M68681_ISR_INPUT_CHANGE;

    return status;
}

static void m68681_reset_receiver(m68681_channel_t * channel)
{
    channel->rx_enabled = FALSE;
    channel->rx_head = 0;
    channel->rx_tail = 0;
    channel->rx_count = 0;
    channel->rx_overrun = FALSE;
}

static void m68681_reset_transmitter(m68681_channel_t * channel)
{
    channel->tx_enabled = FALSE;
    channel->tx_pending = FALSE;
    channel->tx_holding = 0;
}

static void m68681_command_write(m68681_channel_t * channel, uint8_t data)
{
    switch (data & M68681_CR_MISC_MASK)
    {
        case M68681_CR_RESET_MR:
            channel->mode_pointer = 0;
            break;

        case M68681_CR_RESET_RX:
            m68681_reset_receiver(channel);
            break;

        case M68681_CR_RESET_TX:
            m68681_reset_transmitter(channel);
            break;

        case M68681_CR_RESET_ERROR:
            channel->rx_overrun = FALSE;
            break;

        default:
            break;
    }

    switch (data & M68681_CR_TX_MASK)
    {
        case M68681_CR_TX_ENABLE:
            channel->tx_enabled = TRUE;
            break;

        case M68681_CR_TX_DISABLE:
            channel->tx_enabled = FALSE;
            break;

        default:
            break;
    }

    switch (data & M68681_CR_RX_MASK)
    {
        case M68681_CR_RX_ENABLE:
            channel->rx_enabled = TRUE;
            break;

        case M68681_CR_RX_DISABLE:
            channel->rx_enabled = FALSE;
            break;

        default:
            break;
    }
}

static uint8_t m68681_mode_access(m68681_channel_t * channel, uint8_t data, uint8_t access)
{
    uint8_t ret = channel->mode[channel->mode_pointer];

    if (access == BUS_ACCESS_WRITE)
    {
        channel->mode[channel->mode_pointer] = data;
        ret = data;
    }

    /* Any MR1 access advances the independent channel pointer to MR2. */
    if (channel->mode_pointer == 0)
    {
        channel->mode_pointer = 1;
    }

    return ret;
}

static uint8_t m68681_receive_read(m68681_channel_t * channel)
{
    uint8_t data = 0;

    if (channel->rx_count != 0)
    {
        data = channel->rx_fifo[channel->rx_tail];
        channel->rx_tail = (uint8_t)((channel->rx_tail + 1u) % M68681_RX_FIFO_SIZE);
        channel->rx_count--;
    }

    return data;
}

static void m68681_transmit_write(m68681_channel_t * channel, uint8_t data)
{
    if (channel->tx_enabled && (channel->tx_pending == FALSE))
    {
        channel->tx_holding = data;
        channel->tx_pending = TRUE;
    }
}

static uint8_t m68681_register_read(m68681_data_t * device, uint8_t reg)
{
    switch (reg)
    {
        case M68681_REG_MR_A:
            return m68681_mode_access(&device->channel[M68681_EMULATOR_CHANNEL_A], 0, BUS_ACCESS_READ);

        case M68681_REG_SR_A_CSRA:
            return m68681_channel_status(&device->channel[M68681_EMULATOR_CHANNEL_A]);

        case M68681_REG_CR_A:
            return 0;

        case M68681_REG_RHR_A_THR_A:
            return m68681_receive_read(&device->channel[M68681_EMULATOR_CHANNEL_A]);

        case M68681_REG_IPCR_ACR:
        {
            uint8_t ret = device->ipcr;
            device->ipcr = 0;
            return ret;
        }

        case M68681_REG_ISR_IMR:
            return m68681_interrupt_status(device);

        case M68681_REG_CTUR:
            return (uint8_t)(device->timer_counter >> 8);

        case M68681_REG_CTLR:
            return (uint8_t)device->timer_counter;

        case M68681_REG_MR_B:
            return m68681_mode_access(&device->channel[M68681_EMULATOR_CHANNEL_B], 0, BUS_ACCESS_READ);

        case M68681_REG_SR_B_CSRB:
            return m68681_channel_status(&device->channel[M68681_EMULATOR_CHANNEL_B]);

        case M68681_REG_CR_B:
            return 0;

        case M68681_REG_RHR_B_THR_B:
            return m68681_receive_read(&device->channel[M68681_EMULATOR_CHANNEL_B]);

        case M68681_REG_IVR:
            return device->ivr;

        case M68681_REG_IP_OPCR:
            return device->input_port;

        case M68681_REG_START_OPSET:
            device->timer_counter = device->timer_preload;
            device->timer_running = TRUE;
            return 0;

        case M68681_REG_STOP_OPRESET:
            device->timer_irq = FALSE;
            if ((device->acr & M68681_ACR_TIMER_MODE) == 0)
            {
                device->timer_running = FALSE;
            }
            return 0;

        default:
            return M68681_INVALID_READ;
    }
}

static void m68681_register_write(m68681_data_t * device, uint8_t reg, uint8_t data)
{
    switch (reg)
    {
        case M68681_REG_MR_A:
            (void)m68681_mode_access(&device->channel[M68681_EMULATOR_CHANNEL_A], data, BUS_ACCESS_WRITE);
            break;

        case M68681_REG_SR_A_CSRA:
            device->channel[M68681_EMULATOR_CHANNEL_A].clock_select = data;
            break;

        case M68681_REG_CR_A:
            m68681_command_write(&device->channel[M68681_EMULATOR_CHANNEL_A], data);
            break;

        case M68681_REG_RHR_A_THR_A:
            m68681_transmit_write(&device->channel[M68681_EMULATOR_CHANNEL_A], data);
            break;

        case M68681_REG_IPCR_ACR:
            device->acr = data;
            break;

        case M68681_REG_ISR_IMR:
            device->imr = data;
            break;

        case M68681_REG_CTUR:
            device->timer_preload = (uint16_t)((device->timer_preload & 0x00FFu) | ((uint16_t)data << 8));
            break;

        case M68681_REG_CTLR:
            device->timer_preload = (uint16_t)((device->timer_preload & 0xFF00u) | data);
            device->timer_counter = device->timer_preload;
            break;

        case M68681_REG_MR_B:
            (void)m68681_mode_access(&device->channel[M68681_EMULATOR_CHANNEL_B], data, BUS_ACCESS_WRITE);
            break;

        case M68681_REG_SR_B_CSRB:
            device->channel[M68681_EMULATOR_CHANNEL_B].clock_select = data;
            break;

        case M68681_REG_CR_B:
            m68681_command_write(&device->channel[M68681_EMULATOR_CHANNEL_B], data);
            break;

        case M68681_REG_RHR_B_THR_B:
            m68681_transmit_write(&device->channel[M68681_EMULATOR_CHANNEL_B], data);
            break;

        case M68681_REG_IVR:
            device->ivr = data;
            break;

        case M68681_REG_IP_OPCR:
            device->opcr = data;
            break;

        case M68681_REG_START_OPSET:
            device->output_port |= data;
            break;

        case M68681_REG_STOP_OPRESET:
            device->output_port &= (uint8_t)~data;
            break;

        default:
            break;
    }
}

static uint32_t m68681_register_access(uint32_t address, uint32_t data, uint8_t size, uint8_t access)
{
    uint8_t reg = 0;
    m68681_data_t * device = m68681_data_from_address(address, &reg);

    /* The physical MC68681 is an 8-bit device. Wide accesses are intentionally invalid. */
    if ((device == NULL) || (size != sizeof(uint8_t)))
    {
        return M68681_INVALID_READ;
    }

    if (access == BUS_ACCESS_WRITE)
    {
        m68681_register_write(device, reg, (uint8_t)data);
        return 0;
    }

    return m68681_register_read(device, reg);
}

void * m68681_emulator_init(uint32_t base_address)
{
    m68681_data_t * device = (m68681_data_t *)malloc(sizeof(m68681_data_t));
    uint8_t reg;

    if (device == NULL)
    {
        return NULL;
    }

    memset(device, 0x00, sizeof(m68681_data_t));
    device->base_address = base_address;
    device->ivr = M68681_RESET_IVR;

    device->p_next = m68681_data_list;
    m68681_data_list = device;

    for (reg = 0; reg < M68681_REG_COUNT; reg++)
    {
        io_ctrl_set_handler(M68K_IO_BYTE_REG_ADDR(base_address, reg), m68681_register_access);
    }

    return device;
}

void * m68681_emulator_get_channel(void * p_device, uint8_t channel)
{
    m68681_data_t * device = (m68681_data_t *)p_device;

    if ((device == NULL) || (channel >= M68681_EMULATOR_CHANNEL_COUNT))
    {
        return NULL;
    }

    return &device->channel[channel];
}

bool_t m68681_emulator_rx_can_accept(void * p_channel)
{
    m68681_channel_t * channel = (m68681_channel_t *)p_channel;

    if (channel == NULL)
    {
        return FALSE;
    }

    return (channel->rx_enabled && (channel->rx_count < M68681_RX_FIFO_SIZE)) ? TRUE : FALSE;
}

bool_t m68681_emulator_send_rx_byte(void * p_channel, uint8_t data)
{
    m68681_channel_t * channel = (m68681_channel_t *)p_channel;

    if ((channel == NULL) || (channel->rx_enabled == FALSE))
    {
        return FALSE;
    }

    if (channel->rx_count >= M68681_RX_FIFO_SIZE)
    {
        channel->rx_overrun = TRUE;
        return FALSE;
    }

    channel->rx_fifo[channel->rx_head] = data;
    channel->rx_head = (uint8_t)((channel->rx_head + 1u) % M68681_RX_FIFO_SIZE);
    channel->rx_count++;
    return TRUE;
}

bool_t m68681_emulator_tx_pending(void * p_channel)
{
    m68681_channel_t * channel = (m68681_channel_t *)p_channel;

    return ((channel != NULL) && channel->tx_pending) ? TRUE : FALSE;
}

uint8_t m68681_emulator_get_tx_byte(void * p_channel)
{
    m68681_channel_t * channel = (m68681_channel_t *)p_channel;
    uint8_t data = 0;

    if ((channel != NULL) && channel->tx_pending)
    {
        data = channel->tx_holding;
        channel->tx_pending = FALSE;
    }

    return data;
}

bool_t m68681_emulator_irq_pending(void * p_device)
{
    m68681_data_t * device = (m68681_data_t *)p_device;

    if (device == NULL)
    {
        return FALSE;
    }

    return ((m68681_interrupt_status(device) & device->imr) != 0) ? TRUE : FALSE;
}

uint8_t m68681_emulator_interrupt_vector(void * p_device)
{
    m68681_data_t * device = (m68681_data_t *)p_device;

    return (device != NULL) ? device->ivr : M68681_RESET_IVR;
}
