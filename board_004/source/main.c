#include "debug.h"
#include "timer.h"
#include "cli_ctrl.h"
#include "ym2149_driver.h"

int main( void )
{
    timer_data_t test_timer;
    uint32_t counter = 0;

    timer_Init();    
    debug_init();
    cli_ctrl_init();

    debug_printf( "M68k System 01\n" );

#ifndef USE_KEYBOARD_MATRIX
    if (ym2149_init() == TRUE)
    {
        uint8_t gpio_echo;

        /* Exercise the GPIO: drive Port A as output and read it back. */
        ym2149_port_configure(YM2149_PORT_A, YM2149_PORT_OUTPUT);
        ym2149_port_write(YM2149_PORT_A, 0x5A);
        gpio_echo = ym2149_port_read(YM2149_PORT_A);

        debug_printf("YM2149 present, GPIO echo = 0x%02X\n", gpio_echo);
    }
    else
    {
        debug_printf("YM2149 not detected\n");
    }
#endif

    timer_Start(&test_timer, 1000);

    while( 1 )
    {
        cli_ctrl_process();

        if(timer_Check(&test_timer) == TRUE)
        {
            timer_Reload(&test_timer);
            debug_printf("Counter = %d\n", counter);
            counter++;
        }
    }
}
