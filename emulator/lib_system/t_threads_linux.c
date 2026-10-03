#include "t_threads.h"
#ifdef UNIX
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

////////////////////////////////////////////////////////////////////////

#define DONT_MEMORY_DEBUG_ENABLED

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////

typedef struct _tthread_condition_t
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    
    bool_t is_continue;

    struct _tthread_condition_t * p_next;

} tthread_condition_t;

typedef struct _tthread_mutex_t
{
    pthread_mutex_t mutex;

} tthread_mutex_t;

typedef struct _tthread_data_t
{
    tthread_handler_t handler;
    tthread_handler_ex_t handler_ex;
    pthread_t thread;

    bool_t is_running;
    bool_t is_terminated;

    pthread_mutex_t thread_mutex;
    void * args;

    struct _tthread_data_t * p_next;

} tthread_data_t;

////////////////////////////////////////////////////////////////////////

static uint32_t thread_cnt = 0;

////////////////////////////////////////////////////////////////////////

static void * _tthread_entry_point(void * args)
{
    tthread_data_t * new_thread = (tthread_data_t *)args;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
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
    pthread_mutex_init(&new_thread->thread_mutex, NULL);
    
    if (pthread_create(&new_thread->thread, NULL, _tthread_entry_point, new_thread) != 0)
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

    int error_code;
    
    new_thread = (tthread_data_t *)MMALLOC(sizeof(tthread_data_t));
    if (new_thread == NULL) return NULL;
    
    new_thread->handler = NULL;
    new_thread->handler_ex = handler;
    new_thread->args = args;
    
    pthread_mutex_init(&new_thread->thread_mutex, NULL);
    
    if ((error_code = pthread_create(&new_thread->thread, NULL, _tthread_entry_point, new_thread)) != 0)
    {
        MFREE(new_thread);
        printf("Failed to create thread: %d\n", error_code);
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
    pthread_kill(thread->thread, SIGALRM);
    MFREE(thread);
}

void tthread_wait_terminate(void * p_thread)
{
    tthread_data_t * thread = (tthread_data_t *)p_thread;
    pthread_join(thread->thread, NULL);
}

void tthread_set_running(void * p_thread, bool_t state)
{
	tthread_data_t * thread = (tthread_data_t *)p_thread;

	pthread_mutex_lock(&thread->thread_mutex);
	thread->is_running = state;
	pthread_mutex_unlock(&thread->thread_mutex);
}

void tthread_set_terminated(void * p_thread, bool_t state)
{
	tthread_data_t * thread = (tthread_data_t *)p_thread;

	pthread_mutex_lock(&thread->thread_mutex);
	thread->is_terminated = state;
	pthread_mutex_unlock(&thread->thread_mutex);
}

bool_t tthread_is_running(void * p_thread)
{
    bool_t ret = FALSE;

	tthread_data_t * thread = (tthread_data_t *)p_thread;

	pthread_mutex_lock(&thread->thread_mutex);
	ret = thread->is_running;
	pthread_mutex_unlock(&thread->thread_mutex);

	return ret;
}

bool_t tthread_is_terminated(void * p_thread)
{
    bool_t ret = TRUE;

    tthread_data_t * thread = (tthread_data_t *)p_thread;

	pthread_mutex_lock(&thread->thread_mutex);
	ret = thread->is_terminated;
	pthread_mutex_unlock(&thread->thread_mutex);

	return ret;
}

void tthread_sleep_ms(int timeInMs)
{
	uint32_t usecs = 1000 * (uint32_t)timeInMs;
    usleep(usecs);
}

void tthread_sleep_secs(int timeInSecs)
{
	uint32_t usecs = 1000 * 1000 * (uint32_t)timeInSecs;
    usleep(usecs);
}

void * tthread_mutex_init(void)
{
    tthread_mutex_t * mutex = NULL;
    
    mutex = (tthread_mutex_t *)MMALLOC(sizeof(tthread_mutex_t));
    if (mutex == NULL) return NULL;
    memset(mutex, 0x00, sizeof(tthread_mutex_t));
    
    pthread_mutex_init(&mutex->mutex, NULL);

    return mutex;
}

void tthread_mutex_lock(void * p_mutex)
{
    tthread_mutex_t * mutex = (tthread_mutex_t *)p_mutex;
    pthread_mutex_lock(&mutex->mutex);
}

void tthread_mutex_unlock(void * p_mutex)
{
    tthread_mutex_t * mutex = (tthread_mutex_t *)p_mutex;
    pthread_mutex_unlock(&mutex->mutex);
}

void * tthread_condition_create(void)
{
    tthread_condition_t * cond = NULL;
    
    cond = (tthread_condition_t *)MMALLOC(sizeof(tthread_condition_t));
    if (cond == NULL) return NULL;
    memset(cond, 0x00, sizeof(tthread_condition_t));

    pthread_cond_init(&cond->condition, NULL);
    pthread_mutex_init(&cond->mutex, NULL);
    cond->is_continue = FALSE;

    return cond;
}

void tthread_condition_wait(void * p_cond)
{
    tthread_condition_t * cond = (tthread_condition_t *)p_cond;
    
    pthread_mutex_lock(&cond->mutex);
    
    while(cond->is_continue == FALSE)
    {
        pthread_cond_wait(&cond->condition, &cond->mutex);
    }
    
    cond->is_continue = FALSE;
    pthread_mutex_unlock(&cond->mutex);
}

void tthread_condition_wait_timeout(void * p_cond, uint32_t timeout_ms)
{
    tthread_condition_t * cond = (tthread_condition_t *)p_cond;
	struct timespec max_wait = {0, 0};

    if (timeout_ms != NEG_U32)
    {
        (void)clock_gettime(0, &max_wait);

        if (timeout_ms >= 1000)
        {
            max_wait.tv_sec += (timeout_ms / 1000);
        }
        else
        {
            max_wait.tv_nsec += (timeout_ms * 1000 * 1000);
        }
    }

    pthread_mutex_lock(&cond->mutex);

    while(cond->is_continue == FALSE)
    {
        if (timeout_ms != NEG_U32)
        {
            pthread_cond_timedwait(&cond->condition, &cond->mutex, &max_wait);
        }
        else
        {
            pthread_cond_wait(&cond->condition, &cond->mutex);
        }
    }

    cond->is_continue = FALSE;
    pthread_mutex_unlock(&cond->mutex);
}

void tthread_condition_signal(void * p_cond)
{
    tthread_condition_t * cond = (tthread_condition_t *)p_cond;
    
    pthread_mutex_lock(&cond->mutex);
    cond->is_continue = TRUE;
    pthread_cond_broadcast(&cond->condition);
    pthread_mutex_unlock(&cond->mutex);
}

uint32_t mm_threads(void)
{
	return thread_cnt;
}
#endif
