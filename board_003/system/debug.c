#include "debug.h"
#include "uart_driver.h"
#include "lib_c.h"
#include <stdarg.h>

//#ifndef DEV_PRINTK
//#define DEV_PRINTK                "/dev/tty90"
//#endif

///////////////////////////////////////////////////////////////

#define UART_DEBUG_INDEX        1
#define UART_DEBUG_BAURRATE     19200

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

void debug_init( void )
{
    uart_driver_init( UART_DEBUG_INDEX, UART_DEBUG_BAURRATE );
}

int debug_printf( const char *fmt, ... )
{
    va_list args;
    int printed_len;
    
    /* Emit the output into the temporary buffer */
    va_start(args, fmt);
    printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args);
    va_end(args);

    uart_driver_send_buffer( UART_DEBUG_INDEX, ( uint8_t * )vsf_printk_buf, strlen( vsf_printk_buf ) );
    return printed_len;
}



