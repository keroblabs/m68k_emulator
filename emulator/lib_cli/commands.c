#include "commands.h"
#include "comm_host_interface.h"

//////////////////////////////////////////////////////////////////////////////////////

#define MAX_SAVED_COMMANDS          16
#define MAX_SAVED_PRINTS            8
#define CLI_MALLOC                  malloc
#define CLI_FREE                    free

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

static bool_t cli_history_handler(void * p_data, uint8_t command, char * input, char * output)
{
    bool_t ret = FALSE;
    uint16_t len = 0;
    comm_instance_t * comm_instance = (comm_instance_t *)p_data;

    if (command == HISTORY_ADD)
    {
        pipe_write(&comm_instance->history_pipe, input, (uint16_t)strlen(input));
        return TRUE;
    }

    if (pipe_is_empty(&comm_instance->history_pipe) == TRUE)
    {
        return FALSE;
    }

    uint8_t last_index = pipe_get_count(&comm_instance->history_pipe) - 1;

    switch (command)
    {
        case HISTORY_LAST:
            strcpy(output, pipe_get_at(&comm_instance->history_pipe, last_index, &len));
            ret = TRUE;
            break;

        case HISTORY_DISPLAY:
            break;

        case HISTORY_SEARCH:
            break;
    }

    return ret;
}

static void save_print_func(void * p_data, const char * text)
{
    app_data_t *app_data = (app_data_t *)p_data;
    pipe_write(&app_data->debug_pipe, (uint8_t *)text, (uint16_t)strlen(text));
}

static char * dump_print_func(void *p_data, uint8_t index)
{
    app_data_t *app_data = (app_data_t *)p_data;
    uint16_t size = 0;
    char * ret = pipe_get_at(&app_data->debug_pipe, index, &size);
    if (ret != NULL)
    {
        ret[size] = 0;
    }

    return ret;
}

static void help_func(void * p_data, int argc, char **argv)
{
    app_data_t * app_data = (app_data_t *)p_data;
    
    for (int i = 0; i < argc; i++)
    {
        cli_printf(&app_data->cli_data, "%d = %s\n", i, argv[i]);
    }

	cli_printf(&app_data->cli_data, "HELP function executed\n");
}

static void blink_func(void * p_data, int argc, char **argv)
{
    app_data_t * app_data = (app_data_t *)p_data;

	cli_printf(&app_data->cli_data, "BLINK function executed\n");
}

static void exit_func(void * p_data, int argc, char **argv)
{
    app_data_t * app_data = (app_data_t *)p_data;

    //interface_close(0, comm_data->io_index);
    exit(0);
}

static void close_func(void * p_data, int argc, char **argv)
{
    app_data_t * app_data = (app_data_t *)p_data;
    cli_instance_t * cli_instance = app_data->cli_data.instance_curr;

    cli_instance->pending_close = TRUE;
}

static void add_default_commands(app_data_t * app_data)
{
    commands_add_command(app_data, "help", help_func);
    commands_add_command(app_data, "blink", blink_func);
    commands_add_command(app_data, "exit", exit_func);
    commands_add_command(app_data, "close", close_func);
}

static void process_cli_instances(app_data_t * app_data)
{
    cli_data_t * cli_data = &app_data->cli_data;
	cli_instance_t * search_instance = cli_data->instance_list;

	while (search_instance != NULL)
	{
        cli_process(search_instance);

        if (search_instance->pending_close == TRUE)
        {
            cli_instance_remove(search_instance);
            CLI_FREE(search_instance);
            break;
        }

		search_instance = search_instance->p_next;
	}    
}

static void process_interfaces(app_data_t * app_data)
{
    uint8_t iface_index = 0;

    while (iface_index < INTERFACE_MAX)
    {
        void * new_comm =  comm_interface_pool(app_data->p_iface[iface_index]);

        if (new_comm != NULL)
        {
            //new_cli_instance(app_data, index, new_comm);
            comm_instance_t * comm_instance = (comm_instance_t *)CLI_MALLOC(sizeof(comm_instance_t));
            if (comm_instance == NULL)
            {
                return;
            }

            memset(comm_instance, 0x00, sizeof(comm_instance_t));
            pipe_init(&comm_instance->history_pipe, MAX_SAVED_COMMANDS);

            cli_instance_add(&app_data->cli_data, &comm_instance->cli_instance, new_comm, ECHO_ENABLED);            
        }

        iface_index++;
    }
}

bool_t commands_init(app_data_t * app_data, void * p_data, uint16_t port_num)
{
    bool_t ret = TRUE;

    memset(app_data, 0x00, sizeof(app_data_t));
    
    app_data->cli_data.history_func     = cli_history_handler;
    // the saving pipe is crashing when freeing old texts, needs investigation
    app_data->cli_data.save_print_func  = NULL; // save_print_func; 
    app_data->cli_data.dump_print_func  = dump_print_func;
    app_data->cli_data.p_data           = p_data;

    pipe_init(&app_data->debug_pipe, MAX_SAVED_PRINTS);

    add_default_commands(app_data);

    uint8_t index = 0;

    while (index < INTERFACE_MAX)
    {
        uint32_t new_port = index ? port_num : 0; // interface 0 is console
        app_data->p_iface[index] = comm_interface_open(new_port);
        index++;
    }    

    return ret;
}

void commands_add_command(app_data_t * app_data, char * command, cmd_func_ptr_t func)
{
    cli_command_t * cli_command = (cli_command_t *)CLI_MALLOC(sizeof(cli_command_t));
    if (cli_command != NULL)
    {
        cli_command_add(&app_data->cli_data, cli_command, command, func);
    }
}

void commands_process(app_data_t * app_data)
{
    process_interfaces(app_data);
    process_cli_instances(app_data);
}
