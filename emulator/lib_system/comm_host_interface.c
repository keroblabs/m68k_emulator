#include "comm_host_interface.h"
#include "socket_library.h"
#include "console_local.h"
#include "buffer.h"

//////////////////////////////////////////////////////////////////////////////////////

#define IFACE_BUFFER_SIZE           16384
#define MAX_PENDING_IFACE           (4 * sizeof(void*))

//////////////////////////////////////////////////////////////////////////////////////

typedef bool_t (*api_rx_check_ptr_t)(void * p_data);
typedef char (*api_getc_ptr_t)(void * p_data);
typedef void (*api_putc_ptr_t)(void * p_data, char c);
typedef uint32_t (*api_accept_ptr_t)(uint32_t handler);
typedef uint32_t (*api_prepare_ptr_t)(uint32_t param);
typedef void (*api_close_ptr_t)(void * p_data);

typedef struct _iface_api_t_
{
	api_rx_check_ptr_t 	api_check_func;
	api_getc_ptr_t  	api_getc_func;
	api_putc_ptr_t  	api_putc_func;
	api_accept_ptr_t	api_accept_func;
	api_prepare_ptr_t   api_prepare_func;
	api_close_ptr_t     api_close_func;

} iface_api_t;

typedef struct thread_iface_comm_t_
{
    uint32_t accept_hdl;

    void * thread_hdl;
    void * parent;
    void * mutex;
    bool_t is_running;
    
    const iface_api_t * iface_api;
    buffer_data_t buffer_data;
    uint8_t buffer[IFACE_BUFFER_SIZE];

} thread_iface_comm_t;

typedef struct thread_iface_t_
{
    thread_iface_comm_t rx_thread_data;
    thread_iface_comm_t tx_thread_data;

    struct thread_iface_t_ * p_next;

} thread_iface_t;

typedef struct thread_data_t_
{
    void * thread_accept;
    void * accept_mutex;
    thread_iface_t * thread_list;

    buffer_data_t iface_buffer_data;
    thread_iface_t * iface_buffer[MAX_PENDING_IFACE];
    const iface_api_t * iface_api;

    uint32_t iface_hdl;

    struct thread_data_t_ * p_next;

} thread_data_t;

//////////////////////////////////////////////////////////////////////////////////////

static thread_data_t * local_thread_list;

//////////////////////////////////////////////////////////////////////////////////////

static bool_t socket_check_rx(void * p_data)
{
    thread_iface_comm_t * comm_data = (thread_iface_comm_t *)p_data;
    return socket_read_data(comm_data->accept_hdl, NULL, 0) != 0;
}

static char socket_getc_rx(void * p_data)
{
    char c = 0;
    thread_iface_comm_t * comm_data = (thread_iface_comm_t *)p_data;
    socket_read_data(comm_data->accept_hdl, (uint8_t *)&c, sizeof(c));
    return c;
}

static void socket_putc_tx(void * p_data, char c)
{
    thread_iface_comm_t * comm_data = (thread_iface_comm_t *)p_data;
    socket_write_data(comm_data->accept_hdl, (uint8_t *)&c, sizeof(c));
}

static uint32_t socket_accept(uint32_t handler)
{
    return socket_accept_port(handler);
}

static uint32_t socket_prepare(uint32_t param)
{
    socket_library_init();
    return socket_listen_port((uint16_t)param);
}

static void socket_close(void * p_data)
{
    thread_iface_comm_t * comm_data = (thread_iface_comm_t *)p_data;
    socket_close_port(comm_data->accept_hdl);
}


static bool_t local_check_rx(void * p_data)
{
    return console_local_check_input();
}

static char local_getc_rx(void * p_data)
{
    return console_local_read_key();
}

static void local_putc_tx(void * p_data, char c)
{
    console_local_output(&c, sizeof(c));
}

static uint32_t local_accept(uint32_t handler)
{
    return console_local_wait_accept();
}

static uint32_t local_prepare(uint32_t param)
{
    console_local_init();
    return console_local_prepare();
}

static void local_close(void * p_data)
{
}

//////////////////////////////////////////////////////////////////////////////////////

static const iface_api_t local_iface_api = 
{
    local_check_rx,
    local_getc_rx,
    local_putc_tx,
    local_accept,
    local_prepare,
    local_close
};

static const iface_api_t socket_iface_api = 
{
    socket_check_rx,
    socket_getc_rx,
    socket_putc_tx,
    socket_accept,
    socket_prepare,
    socket_close
};

//////////////////////////////////////////////////////////////////////////////////////

static bool_t thread_rx_tx_is_running(thread_iface_comm_t * thread_comm)
{
    bool_t ret = TRUE;
    tthread_mutex_lock(thread_comm->mutex);
    ret = thread_comm->is_running;
    tthread_mutex_unlock(thread_comm->mutex);
    return ret;
}

static void thread_rx_data(void * param)
{
    thread_iface_comm_t * thread_comm = (thread_iface_comm_t *)param;

    while (thread_rx_tx_is_running(thread_comm))
    {
        while (thread_comm->iface_api->api_check_func(thread_comm) == TRUE)
        {
            buffer_write(&thread_comm->buffer_data, thread_comm->iface_api->api_getc_func(thread_comm));
        }

        tthread_sleep_ms(10);
    }
}

static void thread_tx_data(void * param)
{
    thread_iface_comm_t * thread_comm = (thread_iface_comm_t *)param;

    while (thread_rx_tx_is_running(thread_comm))
    {
        while (buffer_is_empty(&thread_comm->buffer_data) == FALSE)
        {
            char tx_data = (char)buffer_read(&thread_comm->buffer_data);
            thread_comm->iface_api->api_putc_func(thread_comm, tx_data);
        }

        tthread_sleep_ms(10);
    }
}

static void create_tx_rx_thread(uint32_t accept_hdl, tthread_handler_ex_t thread_handler, thread_iface_comm_t * thread_comm, thread_data_t * thread_parent)
{
    thread_comm->accept_hdl = accept_hdl;
    thread_comm->parent     = thread_parent;
    thread_comm->is_running = TRUE;
    thread_comm->mutex      = tthread_mutex_init();
    thread_comm->iface_api  = thread_parent->iface_api;

    buffer_init(&thread_comm->buffer_data, thread_comm->buffer, IFACE_BUFFER_SIZE);

    thread_comm->thread_hdl = tthread_create_ex(thread_handler, thread_comm);
}

static void create_socket_connection(uint32_t socket, thread_data_t * thread_data)
{
    thread_iface_t * thread_iface = (thread_iface_t *)malloc(sizeof(thread_iface_t));
    if (thread_iface != NULL)
    {
        create_tx_rx_thread(socket, thread_rx_data, &thread_iface->rx_thread_data, thread_data);
        create_tx_rx_thread(socket, thread_tx_data, &thread_iface->tx_thread_data, thread_data);

        tthread_mutex_lock(thread_data->accept_mutex);
        thread_iface->p_next = thread_data->thread_list;
        thread_data->thread_list = thread_iface;
        tthread_mutex_unlock(thread_data->accept_mutex);

        buffer_write_pointer(&thread_data->iface_buffer_data, thread_iface);
    }
}

static void thread_check_connection(void * param)
{
    thread_data_t * thread_data = (thread_data_t *)param;

    while (1)
    {
        uint32_t socket_accepted = thread_data->iface_api->api_accept_func(thread_data->iface_hdl);

        if ((socket_accepted != SOCKET_NOT_READY) && (socket_accepted != SOCKET_FAIL_LISTEN))
        {
            create_socket_connection(socket_accepted, thread_data);
        }   
        
        tthread_sleep_ms(10);
    }
}

static void thread_tx_rx_stop(thread_iface_comm_t * thread_comm)
{
    tthread_mutex_lock(thread_comm->mutex);
    thread_comm->is_running = FALSE;
    tthread_mutex_unlock(thread_comm->mutex);
}

static const iface_api_t * interface_prepare(uint32_t param, uint32_t * listen)
{
    const iface_api_t * ret_iface = NULL;

    ret_iface = (param == 0) ? &local_iface_api : &socket_iface_api;
    *listen = ret_iface->api_prepare_func(param);

    return ret_iface;
}

void * comm_interface_pool(void * p_iface)
{
    thread_data_t * thread_data = (thread_data_t *)p_iface;
    return buffer_read_pointer(&thread_data->iface_buffer_data);
}

bool_t comm_interface_check_rx(void * p_data)
{
    thread_iface_t * thread_iface = (thread_iface_t *)p_data;
    return buffer_is_empty(&thread_iface->rx_thread_data.buffer_data) ? FALSE : TRUE;
}

uint16_t comm_interface_rx_pending(void * p_data)
{
    thread_iface_t * thread_iface = (thread_iface_t *)p_data;
    return buffer_get_size(&thread_iface->rx_thread_data.buffer_data);
}

uint8_t comm_interface_get_rx(void * p_data)
{
    thread_iface_t * thread_iface = (thread_iface_t *)p_data;
    return buffer_read(&thread_iface->rx_thread_data.buffer_data);
}

uint16_t comm_interface_buffer_rx(void * p_data, uint8_t * buffer, uint16_t size)
{
    thread_iface_t * thread_iface = (thread_iface_t *)p_data;
    uint16_t counter;

    for (counter = 0; counter < size; counter++)
    {
        if (buffer_is_empty(&thread_iface->rx_thread_data.buffer_data)) break;
        buffer[counter] = buffer_read(&thread_iface->rx_thread_data.buffer_data);
    }
    
    return counter;
}

void comm_interface_put_tx(void * p_data, uint8_t c)
{
    thread_iface_t * thread_iface = (thread_iface_t *)p_data;
    buffer_write(&thread_iface->tx_thread_data.buffer_data, c);
}

void comm_interface_buffer_tx(void * p_data, uint8_t * buffer, uint16_t size)
{
    thread_iface_t * thread_iface = (thread_iface_t *)p_data;
    buffer_write_buffer(&thread_iface->tx_thread_data.buffer_data, buffer, size);
}

void comm_interface_close(void * p_data)
{
    thread_iface_t * thread_iface = (thread_iface_t *)p_data;

    thread_data_t * parent = (thread_data_t *)thread_iface->rx_thread_data.parent;
    thread_iface_t * thread_search = parent->thread_list;
    thread_iface_t * prev_thread = NULL;

    parent->iface_api->api_close_func(&thread_iface->rx_thread_data);

    thread_tx_rx_stop(&thread_iface->rx_thread_data);
    thread_tx_rx_stop(&thread_iface->tx_thread_data);

    tthread_wait_terminate(thread_iface->rx_thread_data.thread_hdl);
    tthread_wait_terminate(thread_iface->tx_thread_data.thread_hdl);

    while (thread_search != NULL)
    {
        if (thread_search == thread_iface)
        {
            if (prev_thread != NULL)
            {
                prev_thread->p_next = thread_search->p_next;
            }
            else
            {
                parent->thread_list = thread_search;
            }

            free(thread_search);
            break;
        }

        prev_thread = thread_search;
        thread_search = thread_search->p_next;
    }
}

void * comm_interface_open(uint32_t param)
{
    thread_data_t * thread_data = (thread_data_t *)malloc(sizeof(thread_data_t));
    if (thread_data != NULL)
    {
        thread_data->iface_api = interface_prepare(param, &thread_data->iface_hdl);

        if (thread_data->iface_hdl < 0)
        {
            // make the socket unusable
            free(thread_data);
            return NULL;
        }

        buffer_init(&thread_data->iface_buffer_data, (uint8_t *)thread_data->iface_buffer, MAX_PENDING_IFACE);

        thread_data->accept_mutex = tthread_mutex_init();
        thread_data->thread_accept = tthread_create_ex(thread_check_connection, thread_data);

        thread_data->p_next = local_thread_list;
        local_thread_list = thread_data;
    }

    return thread_data;
}

void * comm_interface_connect(uint8_t * ip_addr, uint32_t port)
{
    thread_data_t * thread_data = (thread_data_t *)malloc(sizeof(thread_data_t));

    if (thread_data != NULL)
    {
        thread_data->iface_api = interface_prepare(port, &thread_data->iface_hdl);
        if (thread_data->iface_hdl < 0)
        {
            // make the socket unusable
            free(thread_data);
            return NULL;
        }

        uint32_t connect_socket = socket_connect_addr(ip_addr, (uint16_t)port);
        if (connect_socket == 0)
        {
            free(thread_data);
            return NULL;
        }

        buffer_init(&thread_data->iface_buffer_data, (uint8_t *)thread_data->iface_buffer, MAX_PENDING_IFACE);
        thread_data->accept_mutex = tthread_mutex_init();
        create_socket_connection(connect_socket, thread_data);

        thread_data->p_next = local_thread_list;
        local_thread_list = thread_data;
    }

    return thread_data;
}

void comm_interface_init(void)
{
    local_thread_list = NULL;
}