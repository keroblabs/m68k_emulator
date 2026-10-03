#include "t_threads.h"
#ifdef USE_WINDOWS
#include <windows.h>

////////////////////////////////////////////////////////////////////////

#define DONT_MEMORY_DEBUG_ENABLED

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

typedef struct _tthread_condition_t
{
    CRITICAL_SECTION     mutex;
    CONDITION_VARIABLE   condition;

    bool_t is_continue;

    struct _tthread_condition_t * p_next;

} tthread_condition_t;

typedef struct _tthread_mutex_t
{
    HANDLE mutex;

} tthread_mutex_t;

typedef struct _tthread_data_t
{
    tthread_handler_t handler;
    tthread_handler_ex_t handler_ex;

    HANDLE  thread;
    DWORD   thread_ID;

    bool_t is_running;
    bool_t is_terminated;

    CRITICAL_SECTION thread_mutex;

    void * args;

    struct _tthread_data_t * p_next;

} tthread_data_t;

////////////////////////////////////////////////////////////////////////

static uint32_t thread_cnt = 0;

////////////////////////////////////////////////////////////////////////

static void * _tthread_entry_point(void * args)
{
    tthread_data_t * new_thread = (tthread_data_t *)args;

    thread_cnt++;

    if (new_thread->handler)
    {
        new_thread->handler();
    }
    else if (new_thread->handler_ex)
    {
        new_thread->handler_ex(new_thread->args);
    }
    
    thread_cnt--;
    MFREE(new_thread);

    return NULL;
}

void * tthread_create(tthread_handler_t handler)
{
    tthread_data_t * new_thread = NULL;
    
    new_thread = (tthread_data_t *)MMALLOC(sizeof(tthread_data_t));
    if (new_thread == NULL) return NULL;
    
    new_thread->handler = handler;
    new_thread->handler_ex = NULL;

    InitializeCriticalSection(&new_thread->thread_mutex);

    new_thread->thread = CreateThread(NULL,                       // default security attributes
                                        0,                          // default stack size
                                        (LPTHREAD_START_ROUTINE) _tthread_entry_point, 
                                        new_thread,                 // thread function arguments
                                        0,                          // default creation flags
                                        &new_thread->thread_ID);    // receive thread identifier

    if (new_thread->thread == NULL)
    {
        MFREE(new_thread);
        return NULL;
    }
    
    new_thread->is_running = TRUE;
    new_thread->is_terminated = FALSE;

    return new_thread;
}

void * tthread_create_ex(tthread_handler_ex_t handler, void * args)
{
    tthread_data_t * new_thread = NULL;

    new_thread = (tthread_data_t *)MMALLOC(sizeof(tthread_data_t));
    if (new_thread == NULL) return NULL;
    
    new_thread->handler = NULL;
    new_thread->handler_ex = handler;
    new_thread->args = args;

    InitializeCriticalSection(&new_thread->thread_mutex);

    new_thread->thread = CreateThread(NULL,                       // default security attributes
                                        0,                          // default stack size
                                        (LPTHREAD_START_ROUTINE) _tthread_entry_point, 
                                        new_thread,                 // thread function arguments
                                        0,                          // default creation flags
                                        &new_thread->thread_ID);    // receive thread identifier

    if (new_thread->thread == NULL)
    {
        MFREE(new_thread);
        return NULL;
    }
    
    new_thread->is_running = TRUE;
    new_thread->is_terminated = FALSE;
   
    return new_thread;
}

void tthread_cancel(void * p_thread)
{
    tthread_data_t * thread = (tthread_data_t *)p_thread;
    if (thread == NULL) return;
    TerminateThread(thread->thread, 0);
    MFREE(thread);
}

void tthread_wait_terminate(void * p_thread)
{
    tthread_data_t * thread = (tthread_data_t *)p_thread;
    WaitForSingleObject(thread->thread, INFINITE);
}

void tthread_set_running(void * p_thread, bool_t state)
{
	tthread_data_t * thread = (tthread_data_t *)p_thread;

	EnterCriticalSection(&thread->thread_mutex);
	thread->is_running = state;
	LeaveCriticalSection(&thread->thread_mutex);
}

void tthread_set_terminated(void * p_thread, bool_t state)
{
	tthread_data_t * thread = (tthread_data_t *)p_thread;

	EnterCriticalSection(&thread->thread_mutex);
	thread->is_terminated = state;
	LeaveCriticalSection(&thread->thread_mutex);
}

bool_t tthread_is_running(void * p_thread)
{
    bool_t ret = FALSE;

	tthread_data_t * thread = (tthread_data_t *)p_thread;

	EnterCriticalSection(&thread->thread_mutex);
	ret = thread->is_running;
	LeaveCriticalSection(&thread->thread_mutex);

	return ret;
}

bool_t tthread_is_terminated(void * p_thread)
{
    bool_t ret = TRUE;

    tthread_data_t * thread = (tthread_data_t *)p_thread;

	EnterCriticalSection(&thread->thread_mutex);
	ret = thread->is_terminated;
	LeaveCriticalSection(&thread->thread_mutex);

	return ret;
}

void tthread_sleep_ms(int timeInMs)
{
    DWORD usecs = (uint32_t)timeInMs;
    Sleep(usecs);
}

void tthread_sleep_secs(int timeInSecs)
{
	DWORD usecs = 1000 * (uint32_t)timeInSecs;
    Sleep(usecs);
}

void * tthread_mutex_init(void)
{
    tthread_mutex_t * mutex = NULL;
    
    mutex = (tthread_mutex_t *)MMALLOC(sizeof(tthread_mutex_t));
    if (mutex == NULL) return NULL;
    memset(mutex, 0x00, sizeof(tthread_mutex_t));
    mutex->mutex = CreateMutex(NULL, FALSE, NULL);

    return mutex;
}

void tthread_mutex_lock(void * p_mutex)
{
    tthread_mutex_t * mutex = (tthread_mutex_t *)p_mutex;
    WaitForSingleObject(mutex->mutex, INFINITE);
}

void tthread_mutex_unlock(void * p_mutex)
{
    tthread_mutex_t * mutex = (tthread_mutex_t *)p_mutex;
    ReleaseMutex(mutex->mutex);
}

void * tthread_condition_create(void)
{
    tthread_condition_t * cond = NULL;
    
    cond = (tthread_condition_t *)MMALLOC(sizeof(tthread_condition_t));
    if (cond == NULL) return NULL;
    memset(cond, 0x00, sizeof(tthread_condition_t));

    InitializeConditionVariable(&cond->condition);
    InitializeCriticalSection(&cond->mutex);
    cond->is_continue = FALSE;

    return cond;
}

void tthread_condition_wait(void * p_cond)
{
    tthread_condition_t * cond = (tthread_condition_t *)p_cond;
    
    EnterCriticalSection(&cond->mutex);
    
    while(cond->is_continue == FALSE)
    {
        SleepConditionVariableCS(&cond->condition, &cond->mutex, INFINITE);
    }
    
    cond->is_continue = FALSE;
    LeaveCriticalSection(&cond->mutex);
}

void tthread_condition_wait_timeout(void * p_cond, uint32_t timeout_ms)
{
    tthread_condition_t * cond = (tthread_condition_t *)p_cond;
    
    EnterCriticalSection(&cond->mutex);
    
    while(cond->is_continue == FALSE)
    {
        SleepConditionVariableCS(&cond->condition, &cond->mutex, timeout_ms);
    }
    
    cond->is_continue = FALSE;
    LeaveCriticalSection(&cond->mutex);
}

void tthread_condition_signal(void * p_cond)
{
    tthread_condition_t * cond = (tthread_condition_t *)p_cond;
    
    EnterCriticalSection(&cond->mutex);
    cond->is_continue = TRUE;
    WakeConditionVariable(&cond->condition);
    LeaveCriticalSection(&cond->mutex);
}

uint32_t mm_threads(void)
{
	return thread_cnt;
}
#endif
