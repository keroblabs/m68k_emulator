#include "timer_driver.h"
#include "irq_handlers.h"
#include "memory_map.h"

//////////////////////////////////////////////////////////////////////////////////////

typedef struct _timer_register_t
{
    uint32_t free_counter;
    uint32_t flags;

} timer_register_t;

//////////////////////////////////////////////////////////////////////////////////////

#define FREE_RUNNING_COUNT      1000

//////////////////////////////////////////////////////////////////////////////////////

static volatile uint32_t _ms_timer;
static volatile timer_register_t * timer_register;

//////////////////////////////////////////////////////////////////////////////////////

static void timer_irq(void)
{
    _ms_timer++;
}

bool_t timer_driver_init(void)
{
    _ms_timer = 0;
    intr_handler_set(VECTOR_EXT_IRQ + 0, timer_irq);

    timer_register = ( volatile timer_register_t * )TIMER_CTRL_ADDR;
    timer_register->free_counter = FREE_RUNNING_COUNT;
    timer_register->flags = TRUE;
}

uint32_t timer_driver_get_ms(void)
{
    return _ms_timer;
}
