#include "uart_driver.h"
#include "m68681_driver.h"

#define UART_DRV_COUNT          (sizeof(uart_list) / sizeof(uart_list[0]))

typedef struct
{
    uint8_t device;
    uint8_t channel;
} uart_map_t;

/*
 * Logical board UARTs are deliberately kept stable for the rest of the
 * firmware while the hardware beneath them changes to two MC68681 DUARTs.
 */
static const uart_map_t uart_list[] =
{
    { M68681_DEVICE_0, M68681_CHANNEL_A }, /* UART_KEYPAD */
    { M68681_DEVICE_0, M68681_CHANNEL_B }, /* UART_PORT0  */
    { M68681_DEVICE_1, M68681_CHANNEL_A }, /* UART_PORT1  */
};

static void send_byte(const uart_map_t * uart, uint8_t data)
{
    while (m68681_channel_tx_ready(uart->device, uart->channel) == FALSE)
    {
        /* Polling is intentional; the UART API is not interrupt driven yet. */
    }

    m68681_channel_write(uart->device, uart->channel, data);
}

bool_t uart_driver_init(uint8_t index, uint16_t baud_rate)
{
    if (index >= UART_DRV_COUNT)
    {
        return FALSE;
    }

    return m68681_channel_init(uart_list[index].device, uart_list[index].channel, baud_rate);
}

void uart_driver_send_data(uint8_t index, uint8_t data)
{
    if (index < UART_DRV_COUNT)
    {
        send_byte(&uart_list[index], data);
    }
}

uint16_t uart_driver_send_buffer(uint8_t index, uint8_t * buffer, uint16_t size)
{
    uint16_t counter;

    if (index >= UART_DRV_COUNT)
    {
        return 0;
    }

    for (counter = 0; counter < size; counter++)
    {
        send_byte(&uart_list[index], buffer[counter]);
    }

    return size;
}

bool_t uart_driver_is_tx_busy(uint8_t index)
{
    if (index >= UART_DRV_COUNT)
    {
        return FALSE;
    }

    return (m68681_channel_tx_ready(uart_list[index].device, uart_list[index].channel) == TRUE) ? FALSE : TRUE;
}

bool_t uart_driver_is_rx_pending(uint8_t index)
{
    if (index >= UART_DRV_COUNT)
    {
        return FALSE;
    }

    return m68681_channel_rx_ready(uart_list[index].device, uart_list[index].channel);
}

uint8_t uart_driver_read_data(uint8_t index)
{
    if (index >= UART_DRV_COUNT)
    {
        return 0;
    }

    return m68681_channel_read(uart_list[index].device, uart_list[index].channel);
}
