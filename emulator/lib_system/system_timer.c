#include "types.h"
#if defined(UNIX)
#include <sys/time.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "system_timer.h"

///////////////////////////////////////////////////////////////////////////

#if defined(UNIX)
static struct timeval ms_start;
#else
static LARGE_INTEGER n_PerformanceFreq;
static uint64_t timer_reference;
#endif

static bool_t is_init = FALSE;

///////////////////////////////////////////////////////////////////////////

#if !defined(UNIX)
static uint64_t get_performance_counter(void)
{
    LARGE_INTEGER n_PerformanceCount;
	if( QueryPerformanceCounter( &n_PerformanceCount ) == FALSE )
	{
		return 0;
	}

    return ( ( uint64_t )( n_PerformanceCount.LowPart / n_PerformanceFreq.QuadPart ) );
}
#endif

void system_timer_init( void )
{
    if( is_init == TRUE )
    {
        return;
    }
    #if defined(UNIX)
	is_init = TRUE;
    gettimeofday( &ms_start, NULL );    
    #else
	if( QueryPerformanceFrequency( &n_PerformanceFreq ) == TRUE )
    {
        n_PerformanceFreq.QuadPart /= 1000000;
        is_init = TRUE;
    }

    timer_reference = get_performance_counter();
    #endif
}

uint64_t system_timer_get_us( void )
{
    if( is_init == FALSE )
    {
        return 0;
    }

    #if defined(UNIX)
    struct timeval ms_value;
    long mtime, seconds, useconds;
    
    gettimeofday( &ms_value, NULL );
    
    seconds = ms_value.tv_sec - ms_start.tv_sec;
    useconds = ms_value.tv_usec - ms_start.tv_usec;
    mtime = ( seconds * 1000000 ) + useconds;
    
    return ( uint32_t )mtime;    
    #else
    return (get_performance_counter() - timer_reference) >> 2;
    #endif
}

uint64_t system_timer_get_ct_ticks( void )
{
    if( is_init == FALSE )
    {
        return 0;
    }

    #if defined(UNIX)
    struct timeval ms_value;
    uint64_t useconds_total;

    gettimeofday( &ms_value, NULL );

    useconds_total = ( uint64_t )( ms_value.tv_sec - ms_start.tv_sec ) * 1000000ull
                   + ( uint64_t )( ms_value.tv_usec - ms_start.tv_usec );
    #else
    uint64_t useconds_total = system_timer_get_us();
    #endif

    /*
     * 3.6864 MHz = 3686400 ticks/s.
     * ticks = us * 3686400 / 1000000 = us * 2304 / 625 (exact, integer-safe).
     */
    return ( useconds_total * 2304ull ) / 625ull;
}
