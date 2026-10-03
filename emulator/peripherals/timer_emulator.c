#include "timer_emulator.h"
#include "memory_map.h"
#include "bus_ctrl.h"
#include "io_ctrl.h"
#include "system_timer.h"
#include "sdl_lib_export.h"
#include "sdl_window.h"

//////////////////////////////////////////////////////////////////////////////////////

#define TIMER_IRQ_LEVEL         0x05
#define TIMER_IRQ_VECTOR        0

//////////////////////////////////////////////////////////////////////////////////////

static bool_t timer_enabled;
static uint32_t down_counter;
static uint64_t prev_timer;

static do_events_t local_do_event;

//////////////////////////////////////////////////////////////////////////////////////

static uint32_t timer_register_access( uint32_t address, uint32_t data, uint8_t size, uint8_t access )
{
    uint32_t timer_address = address - TIMER_CTRL_ADDR;
    uint32_t ret = 0;

    if( timer_address == 0 )
    {
        // free runner
        if( access == BUS_ACCESS_READ )
        {
            ret = down_counter;
        }
        else
        {
            down_counter = data;
        }
    }
    else if( timer_address == 4 )
    {
        // control register
        if( access == BUS_ACCESS_READ )
        {
            ret = timer_enabled ? 0x01 : 0x00;
        }
        else
        {
            timer_enabled = ( data & 0x01 ) ? TRUE : FALSE;
        }
    }

    return ret;
}

static void timer_emulator_process( uint32_t window_id )
{
    local_do_event( window_id );

    uint64_t new_timer = system_timer_get_us();
    uint64_t diff_timer = new_timer - prev_timer;

    if( diff_timer > down_counter )
    {
        prev_timer = new_timer;

        if( timer_enabled == TRUE )
        {
            // generate interrupt
            bus_ctrl_irq_req(TIMER_IRQ_VECTOR, TIMER_IRQ_LEVEL, NULL);
        }
    }
}

void timer_emulator_init( uint32_t window_id )
{
    system_timer_init();

    timer_enabled = FALSE;
    down_counter = 0;
    prev_timer = system_timer_get_us();

    for( int i = 0; i < 8; i++ )
    {
        io_ctrl_set_handler( TIMER_CTRL_ADDR + i, timer_register_access );
    }

    local_do_event = sdl_window_set_background_event( window_id, timer_emulator_process );
}
