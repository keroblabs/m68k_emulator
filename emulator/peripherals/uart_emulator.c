#include "uart_emulator.h"
#include "memory_map.h"
#include "bus_ctrl.h"
#include "io_ctrl.h"
#include "buffer.h"

//////////////////////////////////////////////////////////////////////////////////////

#define UART_BUFFER_SIZE                32

#define UART_LINE_STATUS_RX_FULL        ( 1 << 0 )
#define UART_LINE_STATUS_OVERRUN        ( 1 << 1 )
#define UART_LINE_STATUS_TX_HOLD        ( 1 << 5 )
#define UART_LINE_STATUS_TX_EMPTY       ( 1 << 6 )

#define UART_LINE_CTRL_DLAB             ( 1 << 7 )

#define UART_INTR_STATUS_RX_FULL        ( 1 << 0 )
#define UART_INTR_STATUS_TX_DONE        ( 1 << 1 )
#define UART_INTR_STATUS_LINE_CHANGE    ( 1 << 2 )
#define UART_INTR_STATUS_MODEM_CHANGE   ( 1 << 3 )

#define UART_INTR_NO_PENDING            ( 1 << 0 )
#define UART_INTR_PENDING_RX_FULL       ( 2 << 1 )
#define UART_INTR_PENDING_TX_EMPTY      ( 1 << 1 )
#define UART_INTR_PENDING_LINE_STATUS   ( 3 << 1 )
#define UART_INTR_PENDING_MODEN_STATUS  ( 0 << 1 )

#define UART_INTR_VECTOR                25

//////////////////////////////////////////////////////////////////////////////////////

enum
{
    UART_DATA_REGISTER,
    UART_INT_EN_REGISTER,
    UART_INT_STATUS_REGISTER,
    UART_LINE_CTRL_REGISTER,
    UART_MODEM_CTRL_REGISTER,
    UART_LINE_STATUS_REGISTER,
    UART_MODEM_STATUS_REGISTER,
    UART_SCRATCH_REGISTER,
};

enum
{
    UART_BAUD_RATE_LSB_REGISTER,
    UART_BAUD_RATE_MSB_REGISTER,
};

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

typedef struct uart_data_t_
{
    struct uart_data_t_ * p_next;

    uint32_t address;
    uint8_t holding_reg;

    uint8_t intr_vector;
    uint16_t baud_rate;

    uart_register_t uart_registers;

} uart_data_t;

//////////////////////////////////////////////////////////////////////////////////////

static uart_data_t * uart_data_list = NULL;
static uint8_t uart_index = 0;

//////////////////////////////////////////////////////////////////////////////////////

static uart_data_t * uart_data_get_from_address( uint32_t address )
{
    uart_data_t * search_data = uart_data_list;

    while( search_data != NULL )
    {
        if( ( address >= search_data->address ) && ( address < ( search_data->address + sizeof( uart_register_t ) ) ) )
        {
            return search_data;
        }

        search_data = search_data->p_next;
    }

    return NULL;
}

static uint32_t uart_data_register_access( uart_data_t * uart_data, uint32_t data, uint8_t access )
{
    uart_register_t * uart_registers = &uart_data->uart_registers;
    uint32_t ret = 0;

    if( access == BUS_ACCESS_WRITE )
    {
        if( uart_registers->line_ctrl & UART_LINE_CTRL_DLAB )
        {
            uart_data->baud_rate &= 0xFF00;
            uart_data->baud_rate |= ( data & 0x0FF );
        }
        else
        {
            uart_data->holding_reg = ( uint8_t )( data & 0x0FF );
            uart_data->uart_registers.line_status |= UART_LINE_STATUS_TX_HOLD;

            if( uart_data->uart_registers.intr_id == UART_INTR_PENDING_TX_EMPTY )
            {
                uart_data->uart_registers.intr_id = UART_INTR_NO_PENDING;
            }
        }
    }
    else
    {
        if( uart_registers->line_ctrl & UART_LINE_CTRL_DLAB )
        {
            data = uart_data->baud_rate & 0x0FF;
        }
        else
        {
            ret = uart_data->uart_registers.data;
            uart_data->uart_registers.line_status &= ~UART_LINE_STATUS_RX_FULL;
            if( uart_data->uart_registers.intr_id == UART_INTR_PENDING_RX_FULL )
            {
                uart_data->uart_registers.intr_id = UART_INTR_NO_PENDING;
            }
        }
    }

    return ret;
}

static uint32_t uart_data_intr_en_register_access( uart_data_t * uart_data, uint32_t data, uint8_t access )
{
    uart_register_t * uart_registers = &uart_data->uart_registers;
    uint32_t ret = 0;

    if( access == BUS_ACCESS_WRITE )
    {
        if( uart_registers->line_ctrl & UART_LINE_CTRL_DLAB )
        {
            uart_data->baud_rate &= 0x00FF;
            uart_data->baud_rate |= ( data & 0x0FF ) << 8;
        }
        else
        {
            uart_data->uart_registers.intr_enable = data & 0x0FF;
        }
    }
    else
    {
        if( uart_registers->line_ctrl & UART_LINE_CTRL_DLAB )
        {
            data = ( uart_data->baud_rate >> 8 ) & 0x0FF;
        }
        else
        {
            data = uart_data->uart_registers.intr_enable;
        }
    }

    return data;
}

static uint32_t uart_register_access(uint32_t address, uint32_t data, uint8_t size, uint8_t access)
{
    uint32_t ret = 0;
    uart_data_t * uart_data = uart_data_get_from_address( address );

    if( uart_data != NULL )
    {
        uint32_t uart_address = address - uart_data->address;

        switch( uart_address )
        {
            case UART_DATA_REGISTER:
                ret = uart_data_register_access( uart_data, data, access );
                break;

            case UART_INT_EN_REGISTER:
                ret = uart_data_intr_en_register_access( uart_data, data, access );
                break;

            case UART_INT_STATUS_REGISTER:
                ret = uart_data->uart_registers.intr_id;
                if( ( access == BUS_ACCESS_READ ) && ( uart_data->uart_registers.intr_id == UART_INTR_PENDING_TX_EMPTY ) )
                {
                    uart_data->uart_registers.intr_id = UART_INTR_NO_PENDING;
                }
                break;

            case UART_LINE_CTRL_REGISTER:
                uart_data->uart_registers.line_ctrl = ( access == BUS_ACCESS_WRITE ) ? ( data & 0x0FF ) : uart_data->uart_registers.modem_ctrl;
                ret = uart_data->uart_registers.modem_ctrl;
                break;

            case UART_MODEM_CTRL_REGISTER:
                uart_data->uart_registers.modem_ctrl = ( access == BUS_ACCESS_WRITE ) ? ( data & 0x0FF ) : uart_data->uart_registers.modem_ctrl;
                ret = uart_data->uart_registers.modem_ctrl;
                break;

            case UART_LINE_STATUS_REGISTER:
                uart_data->uart_registers.line_status = ( access == BUS_ACCESS_WRITE ) ? ( data & 0x0FF ) : uart_data->uart_registers.line_status;
                ret = uart_data->uart_registers.line_status;
                if( ( access == BUS_ACCESS_READ ) && ( uart_data->uart_registers.intr_id == UART_INTR_PENDING_LINE_STATUS ) )
                {
                    uart_data->uart_registers.intr_id = UART_INTR_NO_PENDING;
                }
                break;

            case UART_MODEM_STATUS_REGISTER:
                uart_data->uart_registers.modem_status = ( access == BUS_ACCESS_WRITE ) ? ( data & 0x0FF ) : uart_data->uart_registers.modem_status;
                ret = uart_data->uart_registers.modem_status;
                if( ( access == BUS_ACCESS_READ ) && ( uart_data->uart_registers.intr_id == UART_INTR_PENDING_MODEN_STATUS ) )
                {
                    uart_data->uart_registers.intr_id = UART_INTR_NO_PENDING;
                }
                break;

            case UART_SCRATCH_REGISTER:
                uart_data->uart_registers.scratch = ( access == BUS_ACCESS_WRITE ) ? ( data & 0x0FF ) : uart_data->uart_registers.scratch;
                ret = uart_data->uart_registers.scratch;
                break;

        }
    }

    return ret;
}

void uart_emulator_send_rx_byte( void * p_uart, uint8_t data )
{
    uart_data_t * uart_data = ( uart_data_t * )p_uart;

    if( uart_data->uart_registers.line_status & UART_LINE_STATUS_RX_FULL )
    {
        uart_data->uart_registers.line_status |= UART_LINE_STATUS_OVERRUN;

        if( uart_data->uart_registers.intr_enable & UART_INTR_STATUS_LINE_CHANGE )
        {
            uart_data->uart_registers.intr_id = UART_INTR_PENDING_LINE_STATUS;
            // generate interrupt
        }
    }
    else
    {
        uart_data->uart_registers.line_status |= UART_LINE_STATUS_RX_FULL;
    }

    uart_data->uart_registers.data = data;

    if( uart_data->uart_registers.intr_enable & UART_INTR_STATUS_RX_FULL )
    {
        uart_data->uart_registers.intr_id = UART_INTR_PENDING_RX_FULL;
        // generate interrupt
    }
}

bool_t uart_emulator_is_rx_done( void * p_uart )
{
    uart_data_t * uart_data = ( uart_data_t * )p_uart;
    return ( uart_data->uart_registers.line_status & UART_LINE_STATUS_RX_FULL ) ? FALSE : TRUE;
}

bool_t uart_emulator_tx_pending( void * p_uart )
{
    uart_data_t * uart_data = ( uart_data_t * )p_uart;
    return ( uart_data->uart_registers.line_status & UART_LINE_STATUS_TX_HOLD ) ? TRUE : FALSE;
}

uint8_t uart_emulator_get_tx_byte( void * p_uart )
{
    uart_data_t * uart_data = ( uart_data_t * )p_uart;
    uart_data->uart_registers.line_status &= ~UART_LINE_STATUS_TX_HOLD;
    uart_data->uart_registers.line_status |= UART_LINE_STATUS_TX_EMPTY;

    if( uart_data->uart_registers.intr_enable & UART_INTR_STATUS_TX_DONE )
    {
        uart_data->uart_registers.intr_id = UART_INTR_PENDING_TX_EMPTY;
        // generate interrupt
    }

    return uart_data->holding_reg;
}

void * uart_emulator_init( uint32_t base_address )
{
    uart_data_t * uart_data = ( uart_data_t * )malloc( sizeof( uart_data_t ) );
    if( uart_data == NULL )
    {
        return NULL;
    }

    memset( uart_data, 0x00, sizeof( uart_data_t ) );

    uart_data->address = base_address;
    uart_data->intr_vector = UART_INTR_VECTOR + uart_index;
    
    uart_index++;

    uart_data->uart_registers.intr_id = UART_INTR_NO_PENDING;
    uart_data->uart_registers.line_status = UART_LINE_STATUS_TX_EMPTY;

    uart_data->p_next = uart_data_list;
    uart_data_list = uart_data;

    for( int i = 0; i < 8; i++ )
    {
        io_ctrl_set_handler( base_address + i, uart_register_access );
    }

    return uart_data;
}
