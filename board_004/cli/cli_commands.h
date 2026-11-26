#ifndef _cli_commands_h_
#define _cli_commands_h_

//////////////////////////////////////////////////////////////////////////////////////

#include "cli.h"
#include "pipe.h"
#include "types.h"

//////////////////////////////////////////////////////////////////////////////////////

#if 0
typedef struct comm_data_t_
{
    cli_instance_t          cli_instance;
    
    uint32_t                io_index;
    pipe_data_t             history_pipe;
    message_data_t          io_message;
    char                    pending_char;
    const cli_api_data_t *  io_api;

} comm_data_t;

typedef struct app_data_t_
{
    cli_data_t  cli_data;
    uint32_t    listen_socket;

} app_data_t;
#endif

//////////////////////////////////////////////////////////////////////////////////////

bool_t commands_init(app_data_t * app_data, void * p_data, uint16_t port_num );

void commands_add_command(app_data_t * app_data, char * command, cmd_func_ptr_t func);
void commands_process(app_data_t * app_data);

//////////////////////////////////////////////////////////////////////////////////////

#endif // _cli_commands_h_
