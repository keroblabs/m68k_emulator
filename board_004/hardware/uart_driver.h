#ifndef _uart_driver_h_
#define _uart_driver_h_

#include "types.h"

enum
{
    UART_KEYPAD,
    UART_PORT0,
    UART_PORT1
};

bool_t uart_driver_init(uint8_t index, uint16_t baud_rate);
void uart_driver_send_data(uint8_t index, uint8_t data);
uint16_t uart_driver_send_buffer(uint8_t index, uint8_t * buffer, uint16_t size);
bool_t uart_driver_is_tx_busy(uint8_t index);
bool_t uart_driver_is_rx_pending(uint8_t index);
uint8_t uart_driver_read_data(uint8_t index);

#endif /* _uart_driver_h_ */
