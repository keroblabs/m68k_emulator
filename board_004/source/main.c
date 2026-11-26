#include "debug.h"
#include "timer.h"
#include "cli_ctrl.h"

int main( void )
{
    timer_data_t test_timer;
    uint32_t counter = 0;

    timer_Init();    
    debug_init();
    cli_ctrl_init();

    debug_printf( "M68k System 01\n" );

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
