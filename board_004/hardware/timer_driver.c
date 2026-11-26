#include "timer_driver.h"
#include "irq_handlers.h"
#include "m68681_driver.h"

//////////////////////////////////////////////////////////////////////////////////////

/*
 * The MC68681 counter/timer is clocked by a 3.6864 MHz crystal (X1/CLK).
 * In timer mode the counter-ready interrupt rate is Clock / (2 * preload),
 * so a full interrupt period spans 2 * preload input clocks.
 *   3686400 / (2 * 1843) = 1000 Hz  ->  1 ms tick.
 */
#define M68681_TIMER_DEVICE     M68681_DEVICE_0
#define M68681_TIMER_PRELOAD    1843

//////////////////////////////////////////////////////////////////////////////////////

static volatile uint32_t _ms_timer;

//////////////////////////////////////////////////////////////////////////////////////

static void timer_irq(void)
{
    _ms_timer++;
    m68681_timer_acknowledge(M68681_TIMER_DEVICE);
}

bool_t timer_driver_init(void)
{
    _ms_timer = 0;
    intr_handler_set(VECTOR_EXT_IRQ + 0, timer_irq);

    m68681_set_interrupt_vector(M68681_TIMER_DEVICE, VECTOR_EXT_IRQ);
    m68681_timer_configure(M68681_TIMER_DEVICE, M68681_TIMER_PRELOAD, M68681_TIMER_MODE_X1);
    m68681_set_interrupt_mask(M68681_TIMER_DEVICE, M68681_INT_COUNTER_TIMER);
    m68681_timer_start(M68681_TIMER_DEVICE);

    return TRUE;
}

uint32_t timer_driver_get_ms(void)
{
    return _ms_timer;
}
