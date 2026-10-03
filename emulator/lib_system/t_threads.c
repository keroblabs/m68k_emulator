#include "t_threads.h"

#define _USE_MULTITHREAD

#include <windows.h>

////////////////////////////////////////////////////////////////////////

#define DONT_MEMORY_DEBUG_ENABLED

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

typedef struct _tthread_condition_t
{
    HANDLE  mutex;
    HANDLE  condition;

    bool_t is_continue;

    struct _tthread_condition_t * p_next;

} tthread_condition_t;

typedef struct _tthread_mutex_t
{
    HANDLE  mutex;

} tthread_mutex_t;

typedef struct _tthread_data_t
{
    tthread_handler_t handler;
    tthread_handler_ex_t handler_ex;

    HANDLE  thread;
    DWORD   thread_ID;

    bool_t is_running;
    bool_t is_terminated;

    HANDLE  thread_mutex;

    void * args;

    struct _tthread_data_t * p_next;

} tthread_data_t;

////////////////////////////////////////////////////////////////////////

static uint32_t thread_cnt = 0;

////////////////////////////////////////////////////////////////////////

#if defined _USE_MULTITHREAD
static void * _tthread_entry_point( void * args )
{
    tthread_data_t * new_thread = ( tthread_data_t * )args;

    thread_cnt++;

    if( new_thread->handler )
    {
        new_thread->handler();
    }
    else if( new_thread->handler_ex )
    {
        new_thread->handler_ex( new_thread->args );
    }
    
    thread_cnt--;
    MFREE( new_thread );

    return NULL;
}
#endif

void * tthread_create( tthread_handler_t handler )
{
    tthread_data_t * new_thread = NULL;
    
#if defined _USE_MULTITHREAD

    new_thread = ( tthread_data_t * )MMALLOC( sizeof( tthread_data_t ) );
    if( new_thread == NULL ) return NULL;
    
    new_thread->handler = handler;
    new_thread->handler_ex = NULL;

    new_thread->thread_mutex = CreateMutex( NULL, FALSE, NULL );
    if( new_thread->thread_mutex == NULL )
    {
        MFREE( new_thread );
        return NULL;
    }

    new_thread->handler = CreateThread( NULL,                       // default security attributes
                                        0,                          // default stack size
                                        (LPTHREAD_START_ROUTINE) _tthread_entry_point, 
                                        new_thread,                 // thread function arguments
                                        0,                          // default creation flags
                                        &new_thread->thread_ID);    // receive thread identifier

    if( new_thread->handler != NULL )
    {
        CloseHandle( new_thread->thread_mutex );
        MFREE( new_thread );
        return NULL;
    }
    
    new_thread->is_running = TRUE;
    new_thread->is_terminated = FALSE;
#endif

    return new_thread;
}

void * tthread_create_ex( tthread_handler_ex_t handler, void * args )
{
    tthread_data_t * new_thread = NULL;

#if defined _USE_MULTITHREAD

    new_thread = ( tthread_data_t * )MMALLOC( sizeof( tthread_data_t ) );
    if( new_thread == NULL ) return NULL;
    
    new_thread->handler = NULL;
    new_thread->handler_ex = handler;
    new_thread->args = args;

    new_thread->thread_mutex = CreateMutex( NULL, FALSE, NULL );
    if( new_thread->thread_mutex == NULL )
    {
        MFREE( new_thread );
        return NULL;
    }

    new_thread->handler = CreateThread( NULL,                       // default security attributes
                                        0,                          // default stack size
                                        (LPTHREAD_START_ROUTINE) _tthread_entry_point, 
                                        new_thread,                 // thread function arguments
                                        0,                          // default creation flags
                                        &new_thread->thread_ID);    // receive thread identifier

    if( new_thread->handler != NULL )
    {
        CloseHandle( new_thread->thread_mutex );
        MFREE( new_thread );
        return NULL;
    }
    
    new_thread->is_running = TRUE;
    new_thread->is_terminated = FALSE;
#endif
   
    return new_thread;
}

void tthread_cancel( void * p_thread )
{
    //tthread_data_t * thread = ( tthread_data_t * )p_thread;
    //if( thread == NULL ) return;
    //pthread_kill( thread->thread, SIGALRM );
    //MFREE( thread );
}

void tthread_wait_terminate( void * p_thread )
{
#if defined _USE_MULTITHREAD
    tthread_data_t * thread = ( tthread_data_t * )p_thread;
    //pthread_join( thread->thread, NULL );
#endif
}

void tthread_set_running( void * p_thread, bool_t state )
{
#if defined _USE_MULTITHREAD
	tthread_data_t * thread = ( tthread_data_t * )p_thread;

	WaitForSingleObject( thread->thread_mutex, INFINITE );
	thread->is_running = state;
	ReleaseMutex( thread->thread_mutex );
#endif
}

void tthread_set_terminated( void * p_thread, bool_t state )
{
#if defined _USE_MULTITHREAD
	tthread_data_t * thread = ( tthread_data_t * )p_thread;

	WaitForSingleObject( thread->thread_mutex, INFINITE );
	thread->is_terminated = state;
	ReleaseMutex( thread->thread_mutex );
#endif
}

bool_t tthread_is_running( void * p_thread )
{
    bool_t ret = FALSE;

#if defined _USE_MULTITHREAD
	tthread_data_t * thread = ( tthread_data_t * )p_thread;

	WaitForSingleObject( thread->thread_mutex, INFINITE );
	ret = thread->is_running;
	ReleaseMutex( thread->thread_mutex );
#endif

	return ret;
}

bool_t tthread_is_terminated( void * p_thread )
{
    bool_t ret = TRUE;

#if defined _USE_MULTITHREAD
    tthread_data_t * thread = ( tthread_data_t * )p_thread;

	WaitForSingleObject( thread->thread_mutex, INFINITE );
	ret = thread->is_terminated;
	ReleaseMutex( thread->thread_mutex );
#endif

	return ret;
}

void tthread_sleep_ms( int timeInMs )
{
#if defined _USE_MULTITHREAD
    DWORD usecs = (uint32_t)timeInMs;
    Sleep( usecs );
#endif
}

void tthread_sleep_secs( int timeInSecs )
{
#if defined _USE_MULTITHREAD
	DWORD usecs = 1000 * (uint32_t)timeInSecs;
    Sleep( usecs );
#endif
}

void * tthread_mutex_init( void )
{
    tthread_mutex_t * mutex = NULL;
    
#if defined _USE_MULTITHREAD
    mutex = ( tthread_mutex_t * )MMALLOC( sizeof( tthread_mutex_t ) );
    if( mutex == NULL ) return NULL;
    memset( mutex, 0x00, sizeof( tthread_mutex_t ) );
    
    mutex->mutex = CreateMutex( NULL, FALSE, NULL );
#endif

    return mutex;
}

void tthread_mutex_lock( void * p_mutex )
{
#if defined _USE_MULTITHREAD
    tthread_mutex_t * mutex = ( tthread_mutex_t * )p_mutex;
    WaitForSingleObject( mutex->mutex, INFINITE );
#endif
}

void tthread_mutex_unlock( void * p_mutex )
{
#if defined _USE_MULTITHREAD
    tthread_mutex_t * mutex = ( tthread_mutex_t * )p_mutex;
    ReleaseMutex( mutex->mutex );
#endif
}

void * tthread_condition_create( void )
{
    tthread_condition_t * cond = NULL;
    
#if 0 // defined _USE_MULTITHREAD
    cond = ( tthread_condition_t * )MMALLOC( sizeof( tthread_condition_t ) );
    if( cond == NULL ) return NULL;
    memset( cond, 0x00, sizeof( tthread_condition_t ) );

    pthread_cond_init( &cond->condition, NULL );
    pthread_mutex_init( &cond->mutex, NULL );
    cond->is_continue = FALSE;
#endif

    return cond;
}

void tthread_condition_wait( void * p_cond )
{
#if 0 // defined _USE_MULTITHREAD
    tthread_condition_t * cond = ( tthread_condition_t * )p_cond;
    
    pthread_mutex_lock( &cond->mutex );
    
    while( cond->is_continue == FALSE )
    {
        pthread_cond_wait( &cond->condition, &cond->mutex );
    }
    
    cond->is_continue = FALSE;
    pthread_mutex_unlock( &cond->mutex );
#endif
}

void tthread_condition_wait_timeout( void * p_cond, uint32_t timeout_ms )
{
#if 0 // defined _USE_MULTITHREAD

    tthread_condition_t * cond = ( tthread_condition_t * )p_cond;
	struct timespec max_wait = {0, 0};

    if( timeout_ms != NEG_U32 )
    {
        (void)clock_gettime( CLOCK_REALTIME, &max_wait );

        if( timeout_ms >= 1000 )
        {
            max_wait.tv_sec += ( timeout_ms / 1000 );
        }
        else
        {
            max_wait.tv_nsec += ( timeout_ms * 1000 * 1000 );
        }
    }

    pthread_mutex_lock( &cond->mutex );

    while( cond->is_continue == FALSE )
    {
        if( timeout_ms != NEG_U32 )
        {
            pthread_cond_timedwait( &cond->condition, &cond->mutex, &max_wait );
        }
        else
        {
            pthread_cond_wait( &cond->condition, &cond->mutex );
        }
    }

    cond->is_continue = FALSE;
    pthread_mutex_unlock( &cond->mutex );
#endif
}

void tthread_condition_signal( void * p_cond )
{
#if 0 // defined _USE_MULTITHREAD
    tthread_condition_t * cond = ( tthread_condition_t * )p_cond;
    
    pthread_mutex_lock( &cond->mutex );
    cond->is_continue = TRUE;
    pthread_cond_broadcast( &cond->condition );
    pthread_mutex_unlock( &cond->mutex );
#endif
}

uint32_t mm_threads( void )
{
	return thread_cnt;
}
