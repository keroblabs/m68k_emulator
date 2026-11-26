#ifndef system_timer_h_
#define system_timer_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

void system_timer_init( void );
uint64_t system_timer_get_us( void );

/*
 * Returns a free-running 64-bit counter clocked as if driven by the MC68681's
 * 3.6864 MHz crystal (X1/CLK). Derived from the host microsecond timer so the
 * firmware sees the same counter progression as on real hardware.
 */
uint64_t system_timer_get_ct_ticks( void );

//////////////////////////////////////////////////////////////////////////////////////

#endif /* system_timer_h_ */
