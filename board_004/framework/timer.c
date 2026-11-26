#include "timer.h"
#include "timer_driver.h"

///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////

#define _MS_TIMER ( timer_driver_get_ms() )

///////////////////////////////////////////////////////////////////////////////////////

uint32_t timer_get_MS( void )
{
	return _MS_TIMER;
}

void timer_Init( void )
{
	timer_driver_init();
}

void timer_Start( timer_data_t * timer_entry, uint32_t period )
{
	uint32_t curr_ms = _MS_TIMER;

	timer_entry->reload_ms = period;
    timer_entry->expire_ms = curr_ms + period;
    timer_entry->status = TIMER_INIT | TIMER_RUN;
}

void timer_Reload( timer_data_t * timer_entry )
{
	timer_Start( timer_entry, timer_entry->reload_ms );
}

void timer_Stop( timer_data_t * timer_entry )
{
    timer_entry->status = TIMER_STOPPED;
}

void timer_Expire( timer_data_t * timer_entry )
{
    timer_entry->status = TIMER_EXPIRED;
}

bool_t timer_Check( timer_data_t * timer_entry )
{
	uint32_t curr_ms = _MS_TIMER;

	if( timer_entry->status & TIMER_STOPPED )       return( FALSE );
	else if( timer_entry->status & TIMER_EXPIRED )	return( TRUE );

    if( timer_entry->expire_ms < curr_ms )
    {
		if( ( curr_ms - timer_entry->expire_ms ) < 0x7FFFFFFF ) return( TRUE );
    }
	else if( timer_entry->expire_ms > curr_ms )
	{
		if( ( timer_entry->expire_ms - curr_ms ) > 0x7FFFFFFF ) return( TRUE );
	}

    return( FALSE );
}

uint32_t timer_Get_Counter( timer_data_t * timer_entry )
{
	uint32_t curr_ms = _MS_TIMER;

	if( timer_entry->expire_ms <= curr_ms )
	{
		if( ( curr_ms - timer_entry->expire_ms ) > 0x7FFFFFFF )
		{
			return( ( 0xFFFFFFFF - curr_ms ) + timer_entry->expire_ms ) + 1;
		}
	}
	else
	{
		if( ( timer_entry->expire_ms - curr_ms ) < 0x7FFFFFFF )
		{
			return( timer_entry->expire_ms - curr_ms );
		}
	}

	return( 0L );

}

