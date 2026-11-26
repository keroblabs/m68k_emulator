#include "cli.h"
#include "cli_input.h"
#include "vdp9918_vt100.h"
#include "debug.h"

//////////////////////////////////////////////////////////////////////////////////////

static cli_data_t cli_data;
static cli_command_t cli_command;
static cli_instance_t cli_instance;

static void api_puts_interface(void * p_data, const char *s);
static uint16_t api_even_interface(void * p_data, char * keycode);

//////////////////////////////////////////////////////////////////////////////////////

static const cli_api_data_t io_api_list = 
{
    api_puts_interface,
    api_even_interface,
};

static void api_puts_interface(void * p_data, const char *s)
{
    vdp9918_vt100_write((char *)s);
}

static uint16_t api_even_interface(void * p_data, char * keycode)
{
    return cli_input_get_event(keycode);
}

static void cli_null_command(void * p_data, int argc, char **argv)
{
    cli_data_t * p_cli = ( cli_data_t * )p_data;

    for( int i = 0; i < argc; i++ )
    {
        cli_printf(p_cli, "%d = %s\n", i, argv[i]);
    }

	cli_printf(p_cli, "Command executed\n");
}

void cli_ctrl_process(void)
{
    cli_input_process();
    cli_process(&cli_data);
}

void cli_ctrl_init(void)
{
    cli_input_init();
    vdp9918_vt100_init();

    cli_init(&cli_data);
    cli_command_add(&cli_data, &cli_command, NULL, cli_null_command, &cli_data);
    cli_instance_add(&cli_data, &cli_instance, &io_api_list, NULL, ECHO_ENABLED);
}
