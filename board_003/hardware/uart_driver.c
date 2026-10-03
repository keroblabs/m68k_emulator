#include "uart_driver.h"
#include "memory_map.h"

//////////////////////////////////////////////////////////////////////////////////////

#define UART_DRV_COUNT          (sizeof( uart_list ) / sizeof( uart_register_t *  ) )
#define UART_CLOCK              3072000

#define UART_LINE_STATUS_RX_FULL        ( 1 << 0 )
#define UART_LINE_STATUS_OVERRUN        ( 1 << 1 )
#define UART_LINE_STATUS_TX_HOLD        ( 1 << 5 )
#define UART_LINE_STATUS_TX_EMPTY       ( 1 << 6 )

#define UART_INTR_STATUS_RX_FULL        ( 1 << 0 )
#define UART_INTR_STATUS_TX_DONE        ( 1 << 1 )
#define UART_INTR_STATUS_LINE_CHANGE    ( 1 << 2 )
#define UART_INTR_STATUS_MODEM_CHANGE   ( 1 << 3 )

#define UART_LINE_CTRL_BAUD_REG         ( 1 << 7 )

#define UART_LINE_CTRL_8N2S             ( 0x07 )

#define UART_WAIT_COUNTER               1000

//////////////////////////////////////////////////////////////////////////////////////

typedef struct uart_register_t_
{
    uint8_t data;
    uint8_t intr_enable;
    uint8_t intr_id;
    uint8_t line_ctrl;
    uint8_t modem_ctrl;
    uint8_t line_status;
    uint8_t modem_status;
    uint8_t scratch;

} uart_register_t;

volatile uart_register_t * const uart_list[] = 
{
    ( volatile uart_register_t * )UART_KEYPAD_ADDR,
    ( volatile uart_register_t * )UART_PORT0_ADDR,
    ( volatile uart_register_t * )UART_PORT1_ADDR,
};

//////////////////////////////////////////////////////////////////////////////////////

static void send_byte( volatile uart_register_t * uart, uint8_t data)
{
    volatile uint16_t counter = UART_WAIT_COUNTER;

    while( !(uart->line_status & UART_INTR_STATUS_TX_DONE ) && counter )
    {
        counter--;
    }

    uart->data = data;
}

bool_t uart_driver_init(uint8_t index, uint16_t baud_rate)
{
    if( index >= UART_DRV_COUNT )
    {
        return FALSE;
    }

    if( ( baud_rate < 300 ) || ( baud_rate > 38400 ) )
    {
        return FALSE;
    }

    volatile uart_register_t * uart = uart_list[ index ];
    
    uart->intr_enable = 0;
    uart->modem_ctrl = 0;

    uint32_t clk_div = ( UART_CLOCK >> 4 ) / baud_rate;
    uart->line_ctrl = UART_LINE_CTRL_8N2S | UART_LINE_CTRL_BAUD_REG;
    uart->data = clk_div & 0x0FF;
    uart->intr_enable = ( clk_div >> 8 ) & 0x0FF;
    uart->line_ctrl = UART_LINE_CTRL_8N2S;

    return TRUE;
}

void uart_driver_send_data(uint8_t index, uint8_t data)
{
    if( index >= UART_DRV_COUNT )
    {
        return;
    }

    volatile uart_register_t * uart = uart_list[ index ];

    send_byte( uart, data );
}

uint16_t uart_driver_send_buffer(uint8_t index, uint8_t * buffer, uint16_t size)
{
    if( index >= UART_DRV_COUNT )
    {
        return 0;
    }

    volatile uart_register_t * uart = uart_list[ index ];
    uint16_t counter;

    for( counter = 0; counter < size; counter++ )
    {
        send_byte( uart, buffer[ counter ] );
    }

    return size;
}

bool_t uart_driver_is_tx_busy( uint8_t index )
{
    if( index >= UART_DRV_COUNT )
    {
        return FALSE;
    }

    volatile uart_register_t * uart = uart_list[ index ];

    return( uart->line_status & UART_LINE_STATUS_TX_EMPTY ) ? FALSE : TRUE;
}

bool_t uart_driver_is_rx_pending( uint8_t index )
{
    if( index >= UART_DRV_COUNT )
    {
        return FALSE;
    }

    volatile uart_register_t * uart = uart_list[ index ];

    return( uart->line_status & UART_LINE_STATUS_RX_FULL ) ? TRUE : FALSE;
}

uint8_t uart_driver_read_data( uint8_t index )
{
    if( index >= UART_DRV_COUNT )
    {
        return 0;
    }

    volatile uart_register_t * uart = uart_list[ index ];

    return uart->data;
}
