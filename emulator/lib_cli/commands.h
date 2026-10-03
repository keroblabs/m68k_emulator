#ifndef _cli_commands_h_
#define _cli_commands_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "cli.h"
#include "pipe.h"
#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

#define INTERFACE_MAX       2

//////////////////////////////////////////////////////////////////////////////////////

typedef struct comm_data_t_
{
    cli_instance_t          cli_instance;
    
    uint32_t                io_index;
    uint8_t                 iface_index;
    pipe_data_t             history_pipe;

} comm_data_t;

typedef struct comm_instance_t_
{
    cli_instance_t      cli_instance;

    pipe_data_t         history_pipe;
    void *              p_data;

    struct comm_instance_t_ *p_next;
    
} comm_instance_t;

typedef struct app_data_t_
{
    cli_data_t          cli_data;
    void *              p_iface[INTERFACE_MAX];

    // uint32_t            listen_socket[INTERFACE_MAX];
    pipe_data_t         debug_pipe;

} app_data_t;

//////////////////////////////////////////////////////////////////////////////////////

bool_t commands_init(app_data_t * app_data, void * p_data, uint16_t port_num);

void commands_add_command(app_data_t * app_data, char * command, cmd_func_ptr_t func);
void commands_process(app_data_t * app_data);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _cli_commands_h_
