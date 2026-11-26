#include "cli_commands.h"
#include "cli_interface.h"
#include "interface.h"
#include "message.h"
#include "editor.h"
#include "file_io.h"
#include "draw_screen.h"
#include "graphics.h"

//////////////////////////////////////////////////////////////////////////////////////

#define MAX_SAVED_COMMANDS          16
#define MAX_SAVED_PRINTS            8

//////////////////////////////////////////////////////////////////////////////////////

static comm_data_t tima_comm;

//////////////////////////////////////////////////////////////////////////////////////

static bool_t cli_history_handler(void * p_data, uint8_t command, char * input, char * output)
{
    bool_t ret = FALSE;
    uint16_t len = 0;
    comm_data_t * comm_data = (comm_data_t * )p_data;

    if( command == HISTORY_ADD )
    {
        pipe_send_buffer( &comm_data->history_pipe, (uint8_t *)input, strlen(input));
        return TRUE;
    }

    if( pipe_is_empty( &comm_data->history_pipe ) == TRUE )
    {
        return FALSE;
    }

    uint8_t last_index = pipe_get_used( &comm_data->history_pipe ) - 1;

    switch( command )
    {
        case HISTORY_LAST:
            strcpy( output, (char *)pipe_get_at( &comm_data->history_pipe, last_index, &len ) );
            ret = TRUE;
            break;

        case HISTORY_DISPLAY:
            break;

        case HISTORY_SEARCH:
            break;
    }

    return ret;
}

static void help_func(void * p_data, int argc, char **argv)
{
    app_data_t * app_data = ( app_data_t * )p_data;
    
    for( int i = 0; i < argc; i++ )
    {
        cli_printf(&app_data->cli_data, "%d = %s\n", i, argv[i]);
    }

	cli_printf(&app_data->cli_data, "HELP function executed\n");
}

static void blink_func(void * p_data, int argc, char **argv)
{
    app_data_t * app_data = ( app_data_t * )p_data;

	cli_printf(&app_data->cli_data, "BLINK function executed\n");
}

static void exit_func(void * p_data, int argc, char **argv)
{
    app_data_t * app_data = ( app_data_t * )p_data;
    cli_instance_t * instance = (cli_instance_t *)app_data->cli_data.instance_curr;
    comm_data_t * comm_data = (comm_data_t *)instance->p_data;

    cli_interface_close(0, comm_data->io_index);
    exit(0);
}

/*
static void edit_func(void * p_data, int argc, char **argv)
{
    bool_t is_loop = TRUE;
    interface_scroll_state(FALSE);

    editor_config * edit_data = initEditor();
    interface_attach( editorPostMessage );
    editorGetScreenSize(&edit_data->screen_rows, &edit_data->screen_cols);

    if(argc > 1)
    {
        file_io_open_file(argv[1], edit_data);
    }

    editorSetStatusMessage(" Ctrl-Q quit | Ctrl-S save");

    while(is_loop)
    {
        graphics_process();
        interface_read_key_process();
        is_loop = editorProcessEvents();
    }

    interface_scroll_state(TRUE);
    interface_clear_screen();
    cli_interface_attach();
}
*/

static void add_default_commands(app_data_t * app_data)
{
    commands_add_command(app_data, "help", help_func);
    commands_add_command(app_data, "blink", blink_func);
    commands_add_command(app_data, "exit", exit_func);
    //commands_add_command(app_data, "edit", edit_func);
}

static void new_cli_instance(app_data_t *app_data, uint8_t interface, uint32_t socket)
{
    comm_data_t * comm_data = &tima_comm;

    pipe_init(&comm_data->history_pipe, "SAVE_CMD", MAX_SAVED_COMMANDS);
    comm_data->io_api = cli_interface_get(interface);
    comm_data->io_index = socket;

    cli_instance_add(&app_data->cli_data, &comm_data->cli_instance, comm_data->io_api, comm_data, ECHO_ENABLED);
}

static void process_cli(app_data_t * app_data)
{
    cli_process(&app_data->cli_data);
}

static void process_interfaces(app_data_t * app_data)
{
    //uint8_t index = 0;

    if( app_data->listen_socket != -1 )
    {
        uint32_t socket_accepted = cli_interface_pool(0, app_data->listen_socket);

        if( socket_accepted == INTERFACE_CHECK_FAIL )
        {
            cli_interface_close(0, app_data->listen_socket);
            app_data->listen_socket = -1;
        }
        else if( socket_accepted != INTERFACE_NOT_READY )
        {
            new_cli_instance(app_data, 0, socket_accepted);
        }
    }
}

bool_t commands_init(app_data_t * app_data, void * p_data, uint16_t port_num )
{
    bool_t ret = TRUE;

    memset(app_data, 0x00, sizeof(app_data_t));
    
    app_data->cli_data.history_func = cli_history_handler;
    app_data->cli_data.save_print_func = NULL;
    app_data->cli_data.dump_print_func = NULL;
    app_data->cli_data.p_data = p_data;

    add_default_commands(app_data);

    app_data->listen_socket = cli_interface_init(0, port_num);

    if( app_data->listen_socket < 0 )
    {
        // make the socket unusable
        app_data->listen_socket = -1;
        ret = FALSE;
    }
    else
    {
        cli_interface_attach();
    }

    return ret;
}

void commands_add_command(app_data_t * app_data, char * command, cmd_func_ptr_t func)
{
    cli_command_t * cli_command = (cli_command_t *)malloc(sizeof(cli_command_t));
    if(cli_command != NULL)
    {
        cli_command_add(&app_data->cli_data, cli_command, command, func, app_data);
    }
}

void commands_process(app_data_t * app_data)
{
    process_interfaces(app_data);
    process_cli(app_data);
}
