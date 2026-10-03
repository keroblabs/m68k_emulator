#include "cli_commands.h"
#include "cli_interface.h"
#include "interface.h"
#include "message.h"
#include "vt100_ctrl.h"

//////////////////////////////////////////////////////////////////////////////////////

#define TIME_INTERFACE_INDEX        0
#define COMM_INTERFACE_COUNT        1
#define DEFAULT_TIMA_HANDLER        2048

//////////////////////////////////////////////////////////////////////////////////////

static message_data_t io_message;
static char pending_key;
static bool_t interface_open;
static bool_t is_init = FALSE;
static uint32_t iface_count = 0;

//////////////////////////////////////////////////////////////////////////////////////

static void api_puts_interface(void * p_data, const char *s);
static uint16_t api_even_interface(void * p_data, char * keycode);

//////////////////////////////////////////////////////////////////////////////////////

static const cli_api_data_t io_api_list = 
{
    api_puts_interface,
    api_even_interface,
};

//////////////////////////////////////////////////////////////////////////////////////

static void api_puts_interface(void * p_data, const char *s)
{
    // interface_output(s, MODE_TEXT_NORMAL);
    vt100_ctrl_write((char *)s);
}

static uint16_t api_even_interface(void * p_data, char * keycode)
{
    *keycode = pending_key;
    return message_Get(&io_message);
}

static void cli_post_message( uint16_t message, char c )
{
    message_Post(&io_message, message);

    if(message == INPUT_EVENT_KEYPRESS)
    {
        pending_key = c;
    }
}

const cli_api_data_t * cli_interface_get(uint8_t index)
{
    if(index >= COMM_INTERFACE_COUNT)
    {
        return NULL;
    }

    return &io_api_list;
}

uint32_t cli_interface_pool(uint8_t index, uint32_t iface_hdl)
{
    if(index >= COMM_INTERFACE_COUNT)
    {
        return INTERFACE_INVALID;
    }

    uint32_t ret_handler = INTERFACE_NOT_READY;

    if( interface_open == FALSE )
    {
        interface_open = TRUE;
        iface_count++;
        ret_handler = DEFAULT_TIMA_HANDLER + iface_count;
    }

    return ret_handler;
}

void cli_interface_attach(void)
{
    interface_attach(cli_post_message);
}

uint32_t cli_interface_init(uint8_t index, uint32_t param)
{
    vt100_ctrl_init();
    if(index >= COMM_INTERFACE_COUNT)
    {
        return INTERFACE_INVALID;
    }

    if(is_init == FALSE)
    {
        is_init = TRUE;
        message_Init(&io_message);
    }

    interface_open = FALSE;


    return DEFAULT_TIMA_HANDLER;
}

void cli_interface_close(uint8_t index, uint32_t iface_hdl)
{
    interface_open = FALSE;
}
